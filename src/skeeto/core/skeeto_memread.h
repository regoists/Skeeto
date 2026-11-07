// Skeeto memory access helpers.
// VirtualQuery walks the whole same-attribute region, so its cost grows with the
// region size (about 100us inside a ~31 MiB heap region). Hot paths must not ask
// it per field: entity reads trust the engine-returned pointer and let SEH catch
// faults. Page queries remain for untrusted pointers on cold paths only.
#pragma once
#include <Windows.h>
#include <intrin.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

#pragma intrinsic(_ReturnAddress)

namespace skmem {

constexpr int kEntFieldMaxOff = 0x4000;
constexpr int kProbeSlots = 512;

// Per call-site counters keyed by return address. Lossy under contention by design.
struct ProbeSlot {
	std::atomic<uintptr_t> site;
	std::atomic<uint32_t> queries;
	std::atomic<uint32_t> faults;
	std::atomic<uint64_t> ticks;
};
inline ProbeSlot g_slots[kProbeSlots];
inline std::atomic<uint32_t> g_queries{0};
inline std::atomic<uint32_t> g_faults{0};
inline std::atomic<uint32_t> g_guardRearmed{0};
inline std::atomic<uint32_t> g_slotOverflow{0};
inline std::atomic<uint32_t> g_windowStart{0};

inline ProbeSlot* SlotFor(uintptr_t site) {
	if (!site) return nullptr;
	const uint32_t h = (uint32_t)(site * 2654435761u) >> 23;
	for (int i = 0; i < 16; ++i) {
		ProbeSlot& s = g_slots[(h + (uint32_t)i) & (kProbeSlots - 1)];
		uintptr_t cur = s.site.load(std::memory_order_relaxed);
		if (cur == site) return &s;
		if (!cur) {
			uintptr_t expected = 0;
			if (s.site.compare_exchange_strong(expected, site, std::memory_order_relaxed) || expected == site)
				return &s;
		}
	}
	g_slotOverflow.fetch_add(1, std::memory_order_relaxed);
	return nullptr;
}

// Every Skeeto page query goes through here so its count and cost stay visible.
__declspec(noinline) inline SIZE_T Query(const void* p, MEMORY_BASIC_INFORMATION* m, uintptr_t site) {
	g_queries.fetch_add(1, std::memory_order_relaxed);
	ProbeSlot* s = SlotFor(site);
	if (!s) return VirtualQuery(p, m, sizeof(*m));
	s->queries.fetch_add(1, std::memory_order_relaxed);
	LARGE_INTEGER a, b;
	QueryPerformanceCounter(&a);
	const SIZE_T r = VirtualQuery(p, m, sizeof(*m));
	QueryPerformanceCounter(&b);
	s->ticks.fetch_add((uint64_t)(b.QuadPart - a.QuadPart), std::memory_order_relaxed);
	return r;
}

inline void NoteFault(uintptr_t site) {
	g_faults.fetch_add(1, std::memory_order_relaxed);
	if (ProbeSlot* s = SlotFor(site)) s->faults.fetch_add(1, std::memory_order_relaxed);
}

// Reading a PAGE_GUARD page clears its guard bit. A garbage pointer into another
// thread's stack guard would otherwise stop that stack from growing; re-arm it.
inline LONG FaultFilter(DWORD code, EXCEPTION_POINTERS* info) {
	if (code == STATUS_GUARD_PAGE_VIOLATION) {
		const EXCEPTION_RECORD* rec = info ? info->ExceptionRecord : nullptr;
		if (rec && rec->NumberParameters >= 2) {
			const uintptr_t page = (uintptr_t)rec->ExceptionInformation[1] & ~(uintptr_t)0xFFF;
			MEMORY_BASIC_INFORMATION m{};
			DWORD old = 0;
			if (VirtualQuery((void*)page, &m, sizeof(m)) && m.State == MEM_COMMIT
				&& !(m.Protect & PAGE_GUARD)
				&& VirtualProtect((void*)page, 0x1000, m.Protect | PAGE_GUARD, &old))
				g_guardRearmed.fetch_add(1, std::memory_order_relaxed);
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}
	if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR)
		return EXCEPTION_EXECUTE_HANDLER;
	return EXCEPTION_CONTINUE_SEARCH;
}

inline bool PlausibleAddress(const void* p) {
	return (uintptr_t)p >= 0x10000u;
}

// A successful probe proves the byte is readable now, not that the object is alive:
// freed-but-committed heap still passes, exactly as the old page query did.
inline bool ProbeAt(const void* p, uintptr_t site) {
	if (!PlausibleAddress(p)) return false;
	__try {
		volatile uint8_t v = *(const volatile uint8_t*)p;
		(void)v;
		return true;
	} __except (FaultFilter(GetExceptionCode(), GetExceptionInformation())) {
		NoteFault(site);
		return false;
	}
}

__declspec(noinline) inline bool Probe(const void* p) {
	return ProbeAt(p, (uintptr_t)_ReturnAddress());
}

inline bool CopyAt(void* dst, const void* src, size_t bytes, uintptr_t site) {
	if (!dst || !PlausibleAddress(src) || !bytes) return false;
	if ((uintptr_t)src + bytes < (uintptr_t)src) return false;
	__try {
		memcpy(dst, src, bytes);
		return true;
	} __except (FaultFilter(GetExceptionCode(), GetExceptionInformation())) {
		NoteFault(site);
		return false;
	}
}

__declspec(noinline) inline bool Copy(void* dst, const void* src, size_t bytes) {
	return CopyAt(dst, src, bytes, (uintptr_t)_ReturnAddress());
}

__declspec(noinline) inline bool Write(void* dst, const void* src, size_t bytes) {
	if (!PlausibleAddress(dst) || !src || !bytes) return false;
	if ((uintptr_t)dst + bytes < (uintptr_t)dst) return false;
	__try {
		memcpy(dst, src, bytes);
		return true;
	} __except (FaultFilter(GetExceptionCode(), GetExceptionInformation())) {
		NoteFault((uintptr_t)_ReturnAddress());
		return false;
	}
}

// Always terminates dst. False on fault, null src or missing terminator within cap-1.
__declspec(noinline) inline bool CopyCStr(char* dst, size_t cap, const char* src) {
	if (!dst || !cap) return false;
	dst[0] = 0;
	if (!PlausibleAddress(src)) return false;
	__try {
		for (size_t i = 0; i + 1 < cap; ++i) {
			const char c = src[i];
			dst[i] = c;
			if (!c) return true;
		}
	} __except (FaultFilter(GetExceptionCode(), GetExceptionInformation())) {
		dst[0] = 0;
		NoteFault((uintptr_t)_ReturnAddress());
		return false;
	}
	dst[cap - 1] = 0;
	return false;
}

// Whole [p, p + bytes) readable, proven by guarded reads of the first byte, the last
// byte and one byte per page between. At most bytes/4096 + 2 probes, no region walk.
inline bool SpanProbeAt(const void* p, size_t bytes, uintptr_t site) {
	if (!PlausibleAddress(p) || !bytes) return false;
	const uintptr_t at = (uintptr_t)p;
	if (at + bytes < at) return false;
	const uintptr_t last = at + bytes - 1;
	if (!ProbeAt(p, site)) return false;
	for (uintptr_t page = (at & ~(uintptr_t)0xFFF) + 0x1000; page <= last; page += 0x1000)
		if (!ProbeAt((const void*)page, site)) return false;
	return ProbeAt((const void*)last, site);
}

__declspec(noinline) inline bool SpanProbe(const void* p, size_t bytes) {
	return SpanProbeAt(p, bytes, (uintptr_t)_ReturnAddress());
}

inline bool ReadableProtect(DWORD protect) {
	if (protect & (PAGE_GUARD | PAGE_NOACCESS)) return false;
	return (protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ
		| PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

inline bool ExecutableProtect(DWORD protect) {
	if (protect & (PAGE_GUARD | PAGE_NOACCESS)) return false;
	return (protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE
		| PAGE_EXECUTE_WRITECOPY)) != 0;
}

// Cold paths only: every page of [p, p + bytes) committed and readable.
inline bool ReadableRangeAt(const void* p, size_t bytes, uintptr_t site) {
	if (!PlausibleAddress(p) || !bytes) return false;
	uintptr_t at = (uintptr_t)p;
	const uintptr_t end = at + bytes;
	if (end < at) return false;
	while (at < end) {
		MEMORY_BASIC_INFORMATION m{};
		if (!Query((const void*)at, &m, site) || m.State != MEM_COMMIT || !ReadableProtect(m.Protect))
			return false;
		const uintptr_t next = (uintptr_t)m.BaseAddress + m.RegionSize;
		if (next <= at) return false;
		at = next;
	}
	return true;
}

__declspec(noinline) inline bool ReadableRange(const void* p, size_t bytes) {
	return ReadableRangeAt(p, bytes, (uintptr_t)_ReturnAddress());
}

// Execute rights cannot be proven by reading, so this stays a page query, but only
// positive answers are cached, per 4 KiB page, for kExecCacheMs. Code pages of loaded
// modules do not change protection; the short life bounds a stale "yes" after unload.
constexpr int kExecCacheSlots = 512;
constexpr DWORD kExecCacheMs = 2000;
inline std::atomic<uintptr_t> g_execPages[kExecCacheSlots];
inline std::atomic<DWORD> g_execFlushAt{0};
// The deadline is only meaningful once armed. Comparing against the zero start value
// would be wrong for uptimes of about 24.9 to 49.7 days, where (LONG)now is negative.
inline std::atomic<bool> g_execFlushArmed{false};
inline std::atomic<uint32_t> g_execCacheHits{0};

// Pure rule so it can be tested with arbitrary tick values.
inline bool ExecCacheFlushDue(DWORD now, DWORD flushAt, bool armed) {
	return !armed || (LONG)(now - flushAt) >= 0;
}

// `now` is a parameter only so tests can feed uptimes and wraparound; callers use ExecutableAt.
inline bool ExecutableAtTime(const void* p, uintptr_t site, DWORD now) {
	if (!PlausibleAddress(p)) return false;
	const uintptr_t page = (uintptr_t)p >> 12;
	if (ExecCacheFlushDue(now, g_execFlushAt.load(std::memory_order_relaxed),
			g_execFlushArmed.load(std::memory_order_relaxed))) {
		for (int i = 0; i < kExecCacheSlots; ++i) g_execPages[i].store(0, std::memory_order_relaxed);
		g_execFlushAt.store(now + kExecCacheMs, std::memory_order_relaxed);
		g_execFlushArmed.store(true, std::memory_order_relaxed);
	} else if (g_execPages[page & (kExecCacheSlots - 1)].load(std::memory_order_relaxed) == page) {
		g_execCacheHits.fetch_add(1, std::memory_order_relaxed);
		return true;
	}
	MEMORY_BASIC_INFORMATION m{};
	if (!Query(p, &m, site) || m.State != MEM_COMMIT || !ExecutableProtect(m.Protect)) return false;
	g_execPages[page & (kExecCacheSlots - 1)].store(page, std::memory_order_relaxed);
	return true;
}

inline bool ExecutableAt(const void* p, uintptr_t site) {
	return ExecutableAtTime(p, site, GetTickCount());
}

// Tests only: forget every cached answer and the deadline.
inline void ResetExecCacheForTest() {
	for (int i = 0; i < kExecCacheSlots; ++i) g_execPages[i].store(0, std::memory_order_relaxed);
	g_execFlushAt.store(0, std::memory_order_relaxed);
	g_execFlushArmed.store(false, std::memory_order_relaxed);
	g_execCacheHits.store(0, std::memory_order_relaxed);
}

__declspec(noinline) inline bool Executable(const void* p) {
	return ExecutableAt(p, (uintptr_t)_ReturnAddress());
}

inline void ResetCounters() {
	for (int i = 0; i < kProbeSlots; ++i) {
		g_slots[i].queries.store(0, std::memory_order_relaxed);
		g_slots[i].faults.store(0, std::memory_order_relaxed);
		g_slots[i].ticks.store(0, std::memory_order_relaxed);
	}
	g_queries.store(0, std::memory_order_relaxed);
	g_faults.store(0, std::memory_order_relaxed);
	g_slotOverflow.store(0, std::memory_order_relaxed);
	g_windowStart.store(GetTickCount(), std::memory_order_relaxed);
}

} // namespace skmem

// Entity fields: e must come from the entity list in the current game/Paint call
// chain and off from RecvTable resolution. No page query; a fault reads as failure.
template <class T>
inline bool EntRead(const void* e, int off, T* out) {
	static_assert(std::is_trivially_copyable<T>::value && sizeof(T) <= 64, "plain field only");
	if (!e || !out || off < 0 || off > skmem::kEntFieldMaxOff) return false;
	T tmp;
	if (!skmem::Copy(&tmp, (const uint8_t*)e + off, sizeof(T))) return false;
	*out = tmp;
	return true;
}

inline bool EntReadBlock(const void* e, int off, void* dst, size_t bytes) {
	if (!e || !dst || off < 0 || off > skmem::kEntFieldMaxOff || bytes > (size_t)skmem::kEntFieldMaxOff)
		return false;
	return skmem::Copy(dst, (const uint8_t*)e + off, bytes);
}

template <class T>
inline bool EntWrite(void* e, int off, const T& value) {
	static_assert(std::is_trivially_copyable<T>::value && sizeof(T) <= 64, "plain field only");
	if (!e || off < 0 || off > skmem::kEntFieldMaxOff) return false;
	return skmem::Write((uint8_t*)e + off, &value, sizeof(T));
}

// Same ownership rule as EntRead; replaces a per-call VirtualQuery liveness check
// on an engine-returned entity (or one of its fields) with a guarded one-byte read.
inline bool EntLive(const void* e) { return skmem::Probe(e); }
inline bool EntFieldOk(const void* field) { return skmem::Probe(field); }
