#include "skeeto_elim_persist.h"
#include "skeeto_runtime.h"

#include <Windows.h>
#include <cstdio>
#include <cstring>

std::atomic<int> g_elimSi{0};
std::atomic<int> g_elimCi{0};
std::atomic<int> g_elimSiMelee{0};
std::atomic<int> g_elimSkeet{0};
std::atomic<int> g_elimMeleeSkeet{0};
std::atomic<int> g_elimHs{0};
std::atomic<int> g_elimHsPool{0};
std::atomic<int> g_elimTank{0};
std::atomic<int> g_elimWitch{0};
std::atomic<bool> g_elimPersistActive{false};
std::atomic<bool> g_elimPersistDirty{false};
std::atomic<bool> g_elimPersistLoadPending{false};
std::atomic<bool> g_elimPersistSavePending{false};
std::atomic<bool> g_elimPersistLoaded{false};

// =============================================================================
// Elim persist — kill-stats file, independent of Career Archive.
// Load once (acquire). Bad/missing file: mark dirty+loaded so next save rewrites.
// Never fopen on Paint. Worker save only after MarkChanged.
// =============================================================================
static void ElimPersistFilePath(char* out, size_t outN, bool temp) {
	if (!out || outN == 0) return;
	out[0] = 0;
	char dir[MAX_PATH]{};
	SkeetoGameRoot(dir, sizeof(dir));
	if (!dir[0]) return;
	snprintf(out, outN, "%s\\skeeto_elim_stats.dat%s", dir, temp ? ".tmp" : "");
	out[outN - 1] = 0;
}

static int ElimPersistClamp(long long v) {
	if (v < 0) return 0;
	if (v > 2147483647LL) return 2147483647;
	return (int)v;
}

void ElimPersistLoad() {
	if (!g_elimPersistActive.load(std::memory_order_acquire)
		|| g_elimPersistLoaded.load(std::memory_order_acquire)) return;
	char path[MAX_PATH]{};
	ElimPersistFilePath(path, sizeof(path), false);
	if (!path[0]) return;
	FILE* f = fopen(path, "r");
	if (!f) {
		g_elimPersistDirty.store(true, std::memory_order_release);
		g_elimPersistLoaded.store(true, std::memory_order_release);
		return;
	}
	int version = 0;
	unsigned int seen = 0;
	long long v[9]{};
	char line[96]{};
	while (fgets(line, sizeof(line), f)) {
		long long n = 0;
		if (sscanf(line, "version=%d", &version) == 1) continue;
		if (sscanf(line, "si=%lld", &n) == 1) { v[0] = n; seen |= 1u << 0; continue; }
		if (sscanf(line, "ci=%lld", &n) == 1) { v[1] = n; seen |= 1u << 1; continue; }
		if (sscanf(line, "melee=%lld", &n) == 1) { v[2] = n; seen |= 1u << 2; continue; }
		if (sscanf(line, "skeet=%lld", &n) == 1) { v[3] = n; seen |= 1u << 3; continue; }
		if (sscanf(line, "melee_skeet=%lld", &n) == 1) { v[4] = n; seen |= 1u << 4; continue; }
		if (sscanf(line, "headshots=%lld", &n) == 1) { v[5] = n; seen |= 1u << 5; continue; }
		if (sscanf(line, "headshot_pool=%lld", &n) == 1) { v[6] = n; seen |= 1u << 6; continue; }
		if (sscanf(line, "tank=%lld", &n) == 1) { v[7] = n; seen |= 1u << 7; continue; }
		if (sscanf(line, "witch=%lld", &n) == 1) { v[8] = n; seen |= 1u << 8; continue; }
	}
	fclose(f);
	// v1 files carry only the first seven counters: keep their values, leave tank/witch at
	// zero and mark dirty so the next save rewrites the file as v2.
	const bool v1Ok = version == 1 && seen == 0x7Fu;
	const bool v2Ok = version == 2 && seen == 0x1FFu;
	if (!v1Ok && !v2Ok) {
		g_elimPersistDirty.store(true, std::memory_order_release);
		g_elimPersistLoaded.store(true, std::memory_order_release);
		return;
	}
	if (v1Ok) g_elimPersistDirty.store(true, std::memory_order_release);
	g_elimSi.store(ElimPersistClamp(v[0]), std::memory_order_relaxed);
	g_elimCi.store(ElimPersistClamp(v[1]), std::memory_order_relaxed);
	g_elimSiMelee.store(ElimPersistClamp(v[2]), std::memory_order_relaxed);
	g_elimSkeet.store(ElimPersistClamp(v[3]), std::memory_order_relaxed);
	g_elimMeleeSkeet.store(ElimPersistClamp(v[4]), std::memory_order_relaxed);
	g_elimHs.store(ElimPersistClamp(v[5]), std::memory_order_relaxed);
	g_elimHsPool.store(ElimPersistClamp(v[6]), std::memory_order_relaxed);
	g_elimTank.store(ElimPersistClamp(v[7]), std::memory_order_relaxed);
	g_elimWitch.store(ElimPersistClamp(v[8]), std::memory_order_relaxed);
	g_elimPersistLoaded.store(true, std::memory_order_release);
}

// Worker-only. Event/Paint paths only set one atomic dirty flag; disk I/O is
// limited to existing map/session/process lifecycle boundaries.
void ElimPersistSaveWorker() {
	if (!g_elimPersistDirty.exchange(false, std::memory_order_acq_rel)) return;
	char path[MAX_PATH]{}, temp[MAX_PATH]{};
	ElimPersistFilePath(path, sizeof(path), false);
	ElimPersistFilePath(temp, sizeof(temp), true);
	if (!path[0] || !temp[0]) {
		g_elimPersistDirty.store(true, std::memory_order_release);
		return;
	}
	FILE* f = fopen(temp, "w");
	if (!f) {
		g_elimPersistDirty.store(true, std::memory_order_release);
		return;
	}
	fprintf(f,
		"version=2\nsi=%d\nci=%d\nmelee=%d\nskeet=%d\nmelee_skeet=%d\nheadshots=%d\nheadshot_pool=%d\ntank=%d\nwitch=%d\n",
		g_elimSi.load(std::memory_order_relaxed),
		g_elimCi.load(std::memory_order_relaxed),
		g_elimSiMelee.load(std::memory_order_relaxed),
		g_elimSkeet.load(std::memory_order_relaxed),
		g_elimMeleeSkeet.load(std::memory_order_relaxed),
		g_elimHs.load(std::memory_order_relaxed),
		g_elimHsPool.load(std::memory_order_relaxed),
		g_elimTank.load(std::memory_order_relaxed),
		g_elimWitch.load(std::memory_order_relaxed));
	const bool streamOk = !ferror(f);
	const bool closeOk = fclose(f) == 0;
	const bool writeOk = streamOk && closeOk;
	if (!writeOk || !MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		DeleteFileA(temp);
		g_elimPersistDirty.store(true, std::memory_order_release);
	}
}

void ElimPersistMarkChanged() {
	if (g_elimPersistActive.load(std::memory_order_relaxed))
		g_elimPersistDirty.store(true, std::memory_order_release);
}

void ElimPersistDeactivate() {
	g_elimPersistLoadPending.store(false, std::memory_order_release);
	if (g_elimPersistActive.exchange(false, std::memory_order_acq_rel)
		&& g_elimPersistDirty.load(std::memory_order_acquire))
		g_elimPersistSavePending.store(true, std::memory_order_release);
}

