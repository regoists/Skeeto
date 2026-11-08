// Offline checks for src/skeeto/core/skeeto_memread.h (MSVC x86).
// Also reproduces the P0 mechanism: VirtualQuery cost grows with region size.
#include "../src/skeeto/core/skeeto_memread.h"
#include <cstdio>
#include <cstdlib>

static int g_failed = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++g_failed; } } while (0)

static double QueryMicros(const void* p, int rounds) {
	LARGE_INTEGER f, a, b;
	QueryPerformanceFrequency(&f);
	MEMORY_BASIC_INFORMATION m{};
	QueryPerformanceCounter(&a);
	for (int i = 0; i < rounds; ++i) VirtualQuery(p, &m, sizeof(m));
	QueryPerformanceCounter(&b);
	return (double)(b.QuadPart - a.QuadPart) * 1e6 / (double)f.QuadPart / rounds;
}

static double ProbeMicros(const void* p, int rounds) {
	LARGE_INTEGER f, a, b;
	QueryPerformanceFrequency(&f);
	int v = 0;
	QueryPerformanceCounter(&a);
	for (int i = 0; i < rounds; ++i) v += EntRead(p, 0x40, &v) ? 1 : 0;
	QueryPerformanceCounter(&b);
	return (double)(b.QuadPart - a.QuadPart) * 1e6 / (double)f.QuadPart / rounds;
}

static void TestRegionScaling() {
	const size_t big = 64u << 20;
	auto* region = static_cast<uint8_t*>(VirtualAlloc(nullptr, big, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	CHECK(region != nullptr);
	if (!region) return;
	for (size_t i = 0; i < big; i += 0x1000) region[i] = 1; // fault in every page
	auto* smallRegion = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x20000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	CHECK(smallRegion != nullptr);
	const double head = QueryMicros(region + 0x1000, 200);
	const double tail = QueryMicros(region + big - 0x1000, 2000);
	const double smallUs = smallRegion ? QueryMicros(smallRegion, 2000) : 0.0;
	const double probe = ProbeMicros(region + 0x1000, 200000);
	std::printf("VirtualQuery 64MiB region head=%.2fus tail=%.2fus small(128KiB)=%.2fus; EntRead in big region=%.4fus\n",
		head, tail, smallUs, probe);
	CHECK(probe < 1.0);
	if (smallRegion) VirtualFree(smallRegion, 0, MEM_RELEASE);
	VirtualFree(region, 0, MEM_RELEASE);
}

static void TestReadsAndFaults() {
	auto* page = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x3000, MEM_RESERVE, PAGE_NOACCESS));
	CHECK(page != nullptr);
	if (!page) return;
	CHECK(VirtualAlloc(page, 0x1000, MEM_COMMIT, PAGE_READWRITE) != nullptr);
	int value = 0x12345678;
	memcpy(page + 0x10, &value, sizeof(value));
	skmem::ResetCounters();

	int out = 0;
	CHECK(EntRead(page, 0x10, &out) && out == 0x12345678);
	CHECK(EntLive(page) && EntFieldOk(page + 0xFFF));
	CHECK(!EntRead(page, -4, &out));
	CHECK(!EntRead(page, skmem::kEntFieldMaxOff + 1, &out));
	CHECK(!EntRead((const void*)nullptr, 0, &out));
	CHECK(!EntLive((const void*)0x1234));

	// Reserved but uncommitted page: faults are caught and counted.
	const uint32_t faults0 = skmem::g_faults.load();
	CHECK(!EntFieldOk(page + 0x1000));
	out = 7;
	CHECK(!EntRead(page, 0x1000, &out) && out == 7);
	uint8_t block[16]{};
	CHECK(!EntReadBlock(page, 0xFF8, block, sizeof(block))); // crosses into the reserved page
	CHECK(EntReadBlock(page, 0xFF0, block, sizeof(block)));
	CHECK(skmem::g_faults.load() >= faults0 + 3);

	// Write helper.
	CHECK(EntWrite(page, 0x20, 42) && *(int*)(page + 0x20) == 42);
	CHECK(!EntWrite(page, 0x1000, 42));

	// PAGE_NOACCESS committed page.
	CHECK(VirtualAlloc(page + 0x2000, 0x1000, MEM_COMMIT, PAGE_NOACCESS) != nullptr);
	CHECK(!EntFieldOk(page + 0x2000));
	CHECK(!skmem::ReadableRange(page + 0x2000, 4));

	// Range semantics: whole range, not just the first byte.
	CHECK(skmem::ReadableRange(page, 0x1000));
	CHECK(!skmem::ReadableRange(page + 0xFFC, 8));
	CHECK(!skmem::ReadableRange(page, 0));
	CHECK(!skmem::ReadableRange((const void*)0xFFFFFFF0u, 0x20));

	// Guard page: read fails and the guard bit is re-armed.
	DWORD old = 0;
	CHECK(VirtualProtect(page, 0x1000, PAGE_READWRITE | PAGE_GUARD, &old));
	CHECK(!skmem::ReadableRange(page, 4));
	const uint32_t rearm0 = skmem::g_guardRearmed.load();
	CHECK(!EntFieldOk(page));
	CHECK(skmem::g_guardRearmed.load() == rearm0 + 1);
	MEMORY_BASIC_INFORMATION m{};
	CHECK(VirtualQuery(page, &m, sizeof(m)) && (m.Protect & PAGE_GUARD));
	CHECK(!EntFieldOk(page)); // still guarded
	CHECK(VirtualProtect(page, 0x1000, PAGE_READWRITE, &old));

	// Strings.
	char text[8]{};
	memcpy(page + 0x100, "abc", 4);
	CHECK(skmem::CopyCStr(text, sizeof(text), (const char*)page + 0x100) && !strcmp(text, "abc"));
	memset(page + 0xFF8, 'x', 8); // runs into the reserved page without a terminator
	char longer[32]{};
	CHECK(!skmem::CopyCStr(longer, sizeof(longer), (const char*)page + 0xFF8) && longer[0] == 0);
	memcpy(page + 0x200, "abcdefghij", 11);
	CHECK(!skmem::CopyCStr(text, sizeof(text), (const char*)page + 0x200) && text[7] == 0);

	// Executable check and per-site query counting.
	CHECK(skmem::Executable((const void*)&TestReadsAndFaults));
	CHECK(!skmem::Executable(page));
	CHECK(skmem::g_queries.load() > 0);
	VirtualFree(page, 0, MEM_RELEASE);
}

static void TestSpanProbe() {
	// Pages: [0] RW, [1] RW, [2] NOACCESS, [3] RW, [4] reserved only.
	auto* base = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x5000, MEM_RESERVE, PAGE_NOACCESS));
	CHECK(base != nullptr);
	if (!base) return;
	CHECK(VirtualAlloc(base, 0x2000, MEM_COMMIT, PAGE_READWRITE) != nullptr);
	CHECK(VirtualAlloc(base + 0x2000, 0x1000, MEM_COMMIT, PAGE_NOACCESS) != nullptr);
	CHECK(VirtualAlloc(base + 0x3000, 0x1000, MEM_COMMIT, PAGE_READWRITE) != nullptr);

	CHECK(skmem::SpanProbe(base, 0x2000));               // exactly the two RW pages
	CHECK(skmem::SpanProbe(base + 0x1FFF, 1));           // last byte of a page
	CHECK(!skmem::SpanProbe(base + 0x1FFF, 2));          // last byte lands in the NOACCESS page
	CHECK(!skmem::SpanProbe(base, 0x4000));              // NOACCESS page in the middle is caught per page
	CHECK(!skmem::SpanProbe(base + 0x3000, 0x2000));     // runs into the reserved page
	CHECK(!skmem::SpanProbe(base, 0));                   // empty span is not "readable"
	CHECK(!skmem::SpanProbe((const void*)0xFFFFFFF0u, 0x40)); // address overflow
	CHECK(!skmem::SpanProbe((const void*)0x1000, 4));    // implausible low address

	// Cost shape: one guarded read per page, no region query.
	skmem::ResetCounters();
	CHECK(skmem::SpanProbe(base, 0x2000));
	CHECK(skmem::g_queries.load() == 0);
	VirtualFree(base, 0, MEM_RELEASE);
}

static void TestExecCache() {
	using skmem::ExecutableAtTime;
	const void* code = (const void*)&TestSpanProbe;

	// Pure rule: unarmed always flushes; armed compares with wraparound-safe arithmetic.
	CHECK(skmem::ExecCacheFlushDue(0x90000000u, 0, false));           // ~41 days uptime, never armed
	CHECK(!skmem::ExecCacheFlushDue(0x90000000u, 0x90000000u + 2000u, true));
	CHECK(skmem::ExecCacheFlushDue(0x90000000u + 2000u, 0x90000000u + 2000u, true));
	CHECK(!skmem::ExecCacheFlushDue(0xFFFFF000u, 0x000007D0u, true)); // deadline wrapped past 2^32
	CHECK(skmem::ExecCacheFlushDue(0x000007D0u, 0x000007D0u, true));

	// High uptime (LONG)now < 0: the first call must still arm a real deadline.
	skmem::ResetExecCacheForTest();
	skmem::ResetCounters();
	DWORD now = 0x90000000u;
	CHECK(ExecutableAtTime(code, 1, now));
	CHECK(skmem::g_execFlushArmed.load());
	CHECK(skmem::g_queries.load() == 1);
	CHECK(ExecutableAtTime(code, 1, now + 1));                        // cache hit, no new query
	CHECK(ExecutableAtTime(code, 1, now + 1999u));
	CHECK(skmem::g_queries.load() == 1 && skmem::g_execCacheHits.load() == 2);
	CHECK(ExecutableAtTime(code, 1, now + 2000u));                    // deadline: flush and re-query
	CHECK(skmem::g_queries.load() == 2);

	// Deadline that wraps around 2^32.
	skmem::ResetExecCacheForTest();
	skmem::ResetCounters();
	now = 0xFFFFF000u;
	CHECK(ExecutableAtTime(code, 1, now));
	CHECK(ExecutableAtTime(code, 1, now + 1000u));
	CHECK(skmem::g_queries.load() == 1);
	CHECK(ExecutableAtTime(code, 1, now + 2000u));                    // wrapped to 0x000007D0
	CHECK(skmem::g_queries.load() == 2);

	// Negative answers are never cached.
	skmem::ResetExecCacheForTest();
	skmem::ResetCounters();
	auto* data = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	CHECK(data != nullptr);
	if (data) {
		now = 1000u;
		CHECK(!ExecutableAtTime(data, 2, now));
		CHECK(!ExecutableAtTime(data, 2, now));
		CHECK(skmem::g_queries.load() == 2);
	}

	// A stale "yes" survives only until the deadline, then the real protection is seen.
	auto* page = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READ));
	CHECK(page != nullptr);
	if (page) {
		skmem::ResetExecCacheForTest();
		now = 5000u;
		CHECK(ExecutableAtTime(page, 3, now));
		DWORD old = 0;
		CHECK(VirtualProtect(page, 0x1000, PAGE_NOACCESS, &old));
		CHECK(ExecutableAtTime(page, 3, now + 1000u));                // documented stale window
		CHECK(!ExecutableAtTime(page, 3, now + 2000u));               // flushed: sees NOACCESS
		VirtualFree(page, 0, MEM_RELEASE);
	}
	if (data) VirtualFree(data, 0, MEM_RELEASE);
}

int main() {
	TestReadsAndFaults();
	TestSpanProbe();
	TestExecCache();
	TestRegionScaling();
	if (g_failed) {
		std::printf("test_memread: %d failure(s)\n", g_failed);
		return 1;
	}
	std::printf("test_memread: all checks passed\n");
	return 0;
}
