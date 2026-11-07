"""Exercise the real deferred datacache pump with controlled engine providers.

These tests validate policy and dispatch, not the game's cache implementation.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

HARNESS = r'''
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#define GetTickCount FakeTick
static DWORD now = 10000;
static DWORD FakeTick() { return now; }
static bool features = true, ingame = true, loading = false;
static bool g_hudPostMissionQuiet = false;
static void* g_engine = (void*)1;
static bool SkeetoFeaturesOn() { return features; }
static bool EngInGame() { return ingame; }
static bool EngDrawingLoading() { return loading; }
static bool g_optClientAutoClearDatacache = false;
static bool g_optMemoryHud = false;
static int g_memoryHudAbs = 0, g_memoryHudAlign = 0, g_memoryHudOffX = 0, g_memoryHudOffY = 0;
static int g_memoryHudFont = 18, g_memoryHudScale = 100, g_memoryHudStyle = 0, g_memoryHudInfo = 1;
static unsigned g_memoryLastReleasedBytes = 0;
static unsigned g_memoryDcBytes = 0, g_memoryDcLimitBytes = 0, g_memoryDcLockedBytes = 0;
static unsigned long long g_memoryAvailBytes = 0, g_memoryWorkingSetBytes = 0, g_memoryPrivateBytes = 0;
static int g_memoryEnts = 0, g_memoryBridgeState = 0;
static bool g_memoryHaveDc = false, g_memoryHaveAv = false, g_memoryHaveEnts = false;
static bool g_memoryHaveProcess = false;
static std::atomic_bool g_memoryFreeScanWanted{false}, g_memoryFreeScanValid{false};
static std::atomic<unsigned long long> g_memoryLargestFreeBytes{0};
static std::atomic<unsigned> g_memoryFreeScanGeneration{1}, g_memoryFreeSnapshotGeneration{0};
static unsigned queryCalls = 0;
static uintptr_t queryAddress[1024]{};
static bool malformedRegion = false;
static void FakeSystemInfo(SYSTEM_INFO* s) {
    s->lpMaximumApplicationAddress = (void*)(130u * 4096u - 1u);
}
static SIZE_T FakeVirtualQuery(const void* address, MEMORY_BASIC_INFORMATION* m, SIZE_T) {
    queryAddress[queryCalls++] = (uintptr_t)address;
    m->BaseAddress = (void*)address;
    m->RegionSize = malformedRegion ? 0 : 4096;
    m->State = MEM_FREE;
    return sizeof(*m);
}
#define GetSystemInfo FakeSystemInfo
#define VirtualQuery FakeVirtualQuery
static std::atomic_bool g_memoryClearDcPending{false}, g_memoryClearDcManual{false};
static DWORD g_memoryClearDcLastAt = 0, g_memoryHudSampleAt = 0;
static bool g_memoryClearDcHaveLast = false, g_memoryDcClearing = false;
static bool dcReady = true, clearOk = true;
static unsigned used = 1900u*1048576u, limit = 2048u*1048576u;
static unsigned calls = 0, released = 123u*1048576u;
static unsigned long long avail = 512ull*1048576ull;
static bool MemoryHudReadDc(unsigned* u, unsigned* l) {
    if (!dcReady) return false; *u = used; *l = limit; return true;
}
static bool MemoryHudFlushDc(unsigned* bytes) {
    ++calls; *bytes = released; return clearOk;
}
static bool FakeGlobalMemoryStatusEx(MEMORYSTATUSEX* s) { s->ullAvailVirtual = avail; return true; }
#define GlobalMemoryStatusEx FakeGlobalMemoryStatusEx
static void ConEcho(const char*) {}
static void Log(const char*, ...) {}
'''

FIXTURES = r'''
static bool Check(unsigned got, unsigned want, const char* label) {
    if (got == want) return true;
    printf("%s: got %u expected %u\n", label, got, want); return false;
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    int test = atoi(argv[1]); bool ok = true;
    if (test == 0) {
        g_optClientAutoClearDatacache = true;
        g_memoryClearDcPending = true;
        MemoryDatacachePump();
        ok &= Check(calls, 1, "automatic clear without HUD or L4N");
        now += 599999; g_memoryClearDcPending = true;
        MemoryDatacachePump(); ok &= Check(calls, 1, "automatic cooldown");
        ++now; g_memoryClearDcPending = true;
        MemoryDatacachePump(); ok &= Check(calls, 2, "automatic cooldown boundary");
    } else if (test == 1) {
        g_memoryClearDcManual = true; MemoryDatacachePump();
        ok &= Check(calls, 1, "manual works with both options off");
        now += 4999; g_memoryClearDcManual = true; MemoryDatacachePump();
        ok &= Check(calls, 1, "manual spam throttled");
        ++now; g_memoryClearDcManual = true; MemoryDatacachePump();
        ok &= Check(calls, 2, "manual cooldown boundary");
    } else if (test == 2) {
        g_optClientAutoClearDatacache = true;
        used = 100*1048576u; g_memoryClearDcPending = true;
        MemoryDatacachePump(); ok &= Check(calls, 0, "stale automatic pressure rejected");
        avail = 299ull*1048576ull; g_memoryClearDcPending = true;
        MemoryDatacachePump(); ok &= Check(calls, 1, "low address space pressure");
    } else if (test == 3) {
        loading = true; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(calls, 0, "no cleanup during loading");
        loading = false; features = false; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(calls, 0, "master suppression");
        features = true; dcReady = false; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(calls, 0, "unsupported interface rejected");
        dcReady = true; ingame = false; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(calls, 0, "disconnect rejected");
        ingame = true; g_memoryDcClearing = true; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(calls, 0, "reentry rejected");
    } else if (test == 4) {
        now = 0xfffffff0u; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(calls, 1, "first clear near wrap");
        now += 5000; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(calls, 2, "tick wrap cooldown");
    } else if (test == 10) {
        MemoryHudSetScanWanted(true); MemoryHudWorkerPump();
        unsigned old = g_memoryFreeScanGeneration.load();
        unsigned before = queryCalls;
        MemoryHudSetScanWanted(false); MemoryHudSetScanWanted(true);
        MemoryHudWorkerPump();
        ok &= Check((unsigned)queryAddress[before], 0, "rapid off/on restarts partial scan");
        g_memoryFreeSnapshotGeneration = old;
        g_memoryFreeScanValid = true;
        ok &= Check(MemoryHudHaveFreeSnapshot(), 0, "late publication from old generation is ignored");
    } else if (test == 9) {
        MemoryHudRow rows[7]{};
        g_memoryHudInfo = 0;
        ok &= Check(MemoryHudBuildRows(rows, false), 3, "basic information has exactly three rows");
        g_memoryHudInfo = 1; g_optMemoryHud = true;
        ok &= Check(MemoryHudBuildRows(rows, true), 7, "detailed information in preview");
        ok &= Check(wcscmp(rows[4].value, L"--") == 0, 1, "enabled unavailable process is not fake");
        g_memoryHaveDc = true; g_memoryDcBytes = 124u*1048576u;
        g_memoryDcLimitBytes = 512u*1048576u; g_memoryDcLockedBytes = 32u*1048576u;
        g_memoryFreeScanValid = true; g_memoryLargestFreeBytes = 256ull*1048576ull;
        g_memoryFreeScanWanted = true;
        g_memoryFreeSnapshotGeneration = g_memoryFreeScanGeneration.load();
        g_memoryClearDcHaveLast = true; g_memoryClearDcLastAt = now - 23000u;
        g_memoryLastReleasedBytes = 96u*1048576u;
        MemoryHudBuildRows(rows, false);
        ok &= Check(wcscmp(rows[0].value, L"124/512 MB") == 0, 1, "DC total stays on its own row");
        ok &= Check(wcscmp(rows[1].label, L"锁定") == 0, 1, "locked cache has an explanatory label below DC");
        ok &= Check(wcscmp(rows[1].value, L"32 MB") == 0 && wcscmp(rows[1].secondValue, L"92 MB") == 0, 1, "locked and unlocked share the detail row");
        ok &= Check(wcscmp(rows[2].label, L"AV") == 0, 1, "AV follows DC detail");
        ok &= Check(wcscmp(rows[3].label, L"最大连续内存") == 0, 1, "largest free block is explained below AV");
        ok &= Check(wcscmp(rows[3].value, L"256 MB") == 0, 1, "completed free-space snapshot rendered");
        ok &= Check(rows[3].color == RGB(248,194,88), 1, "largest free block warns at 256 MB");
        ok &= Check(rows[0].base != rows[2].base && rows[2].base != rows[5].base, 1, "metric groups have distinct colors");
        g_memoryHaveProcess = true; g_memoryWorkingSetBytes = 850ull*1048576ull;
        g_memoryPrivateBytes = 1100ull*1048576ull;
        MemoryHudBuildRows(rows, false);
        ok &= Check(wcscmp(rows[4].label, L"实际占用") == 0, 1, "working set has explanatory label");
        ok &= Check(rows[4].base == RGB(255,255,255) && rows[4].color == RGB(255,255,255), 1, "process metrics use neutral white");
        ok &= Check(wcscmp(rows[4].value, L"850 MB") == 0 && wcscmp(rows[4].secondValue, L"1100 MB") == 0, 1, "private commit stays beside working set");
        ok &= Check(wcsstr(rows[6].value, L"23") != nullptr, 1, "clear history age rendered");
        g_memoryHaveAv = g_memoryHaveEnts = true;
        g_memoryDcBytes = 400u*1048576u; g_memoryAvailBytes = 400ull*1048576ull;
        g_memoryEnts = 1650; g_memoryLargestFreeBytes = 128ull*1048576ull;
        MemoryHudBuildRows(rows, false);
        ok &= Check(rows[0].color == RGB(248,194,88), 1, "DC warning threshold retained");
        ok &= Check(rows[2].color == RGB(248,194,88), 1, "AV warning threshold retained");
        ok &= Check(rows[5].color == RGB(248,194,88), 1, "ENTS warning threshold retained");
        ok &= Check(rows[3].color == RGB(255,86,72), 1, "largest free block danger at 128 MB");
        g_memoryDcBytes = 461u*1048576u; g_memoryAvailBytes = 230ull*1048576ull;
        g_memoryEnts = 1900;
        MemoryHudBuildRows(rows, false);
        ok &= Check(rows[0].color == RGB(255,86,72), 1, "DC danger threshold retained");
        ok &= Check(rows[2].color == RGB(255,86,72), 1, "AV danger threshold retained");
        ok &= Check(rows[5].color == RGB(255,86,72), 1, "ENTS danger threshold retained");
        ok &= Check(rows[1].color == rows[1].base, 1, "cache details do not inherit pressure colors");
        g_memoryLargestFreeBytes = 512ull*1048576ull;
        MemoryHudBuildRows(rows, false);
        ok &= Check(rows[3].color == rows[3].base, 1, "largest free block recovers to normal color");
    } else if (test == 8) {
        g_memoryFreeScanWanted = true;
        MemoryHudWorkerPump();
        ok &= Check(queryCalls <= 64, 1, "bounded work per worker tick");
        ok &= Check(g_memoryFreeScanValid, 0, "partial scan not published");
        MemoryHudWorkerPump(); MemoryHudWorkerPump();
        ok &= Check(g_memoryFreeScanValid, 1, "complete snapshot published");
        ok &= Check((unsigned)g_memoryLargestFreeBytes.load(), 4096, "largest free region");
        unsigned before = queryCalls; MemoryHudWorkerPump();
        ok &= Check(queryCalls, before, "completed scans throttled");
        g_memoryFreeScanWanted = false; MemoryHudWorkerPump();
        ok &= Check(g_memoryFreeScanValid, 0, "disabled module clears snapshot");
        ok &= Check(queryCalls, before, "disabled module does not query");
        malformedRegion = true; g_memoryFreeScanWanted = true; MemoryHudWorkerPump();
        ok &= Check(g_memoryFreeScanValid, 0, "zero-size region rejected");
    } else if (test == 7) {
        unsigned mask = 0;
        ok &= Check(MemoryHudLoadSetting("l4n_usage_hud_offset_x=57", &mask), 1, "legacy own setting consumed");
        ok &= Check(g_memoryHudOffX, 57, "legacy position preserved");
        MemoryHudLoadSetting("memory_hud_offset_x=19", &mask);
        MemoryHudLoadSetting("l4n_usage_hud_offset_x=81", &mask);
        ok &= Check(g_memoryHudOffX, 19, "new key wins independent of legacy order");
        MemoryHudLoadSetting("memory_hud_info=0", &mask);
        ok &= Check(g_memoryHudInfo, 0, "information mode persisted");
        ok &= Check(MemoryHudLoadSetting("l4n_game_usage=1", &mask), 0, "no L4N option access");
    } else if (test == 6) {
        g_optClientAutoClearDatacache = true;
        const unsigned limits[] = {512u, 1024u, 2048u};
        for (unsigned mb : limits) {
            limit = mb * 1048576u;
            unsigned threshold = (unsigned)((9ull * limit + 9ull) / 10ull);
            g_memoryClearDcHaveLast = false; calls = 0; used = threshold - 1;
            g_memoryClearDcPending = true; MemoryDatacachePump();
            ok &= Check(calls, 0, "below 90 percent");
            used = threshold; g_memoryClearDcPending = true; MemoryDatacachePump();
            ok &= Check(calls, 1, "at 90 percent for each configured limit");
        }
        limit = 0; used = 2000u*1048576u; calls = 0; g_memoryClearDcHaveLast = false;
        g_memoryClearDcPending = true; MemoryDatacachePump();
        ok &= Check(calls, 0, "unknown limit does not trigger percentage cleanup");
    } else if (test == 5) {
        clearOk = false; g_memoryClearDcManual = true;
        MemoryDatacachePump(); ok &= Check(g_memoryClearDcHaveLast, 0, "failure does not start cooldown");
        clearOk = true; released = 0; g_optClientAutoClearDatacache = true;
        g_memoryClearDcManual = g_memoryClearDcPending = true;
        MemoryDatacachePump(); ok &= Check(calls, 2, "coalesced manual and automatic requests");
        ok &= Check(g_memoryClearDcHaveLast, 1, "zero-byte success starts cooldown");
        ok &= Check(g_memoryDcClearing, 0, "clearing guard restored after failure and success");
    } else return 2;
    return ok ? 0 : 1;
}
'''


class MemoryHud(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        policy = block(source, r'^static bool MemoryHudPressure\(')
        pump = block(source, r'^static void MemoryDatacachePump\(\) \{')
        settings = block(source, r'^static bool MemoryHudLoadSetting\(')
        worker = block(source, r'^static void MemoryHudWorkerPump\(\) \{')
        lifecycle = block(source, r'^static void MemoryHudSetScanWanted\(')
        lifecycle += block(source, r'^static bool MemoryHudHaveFreeSnapshot\(')
        import re
        row_struct = block(source, r'^struct MemoryHudRow \{') + ';'
        row_builder = block(source, r'^static int MemoryHudBuildRows\(')
        color = block(source, r'^static COLORREF MemoryHudRatioColor\(')
        ids = sorted(set(re.findall(r'UiId::(\w+)', row_builder)))
        ui = 'enum class UiId {' + ','.join(ids) + '};\n'
        ui += 'static const wchar_t* UiText(UiId, const wchar_t* fallback) { return fallback; }\n'
        cls.temp = tempfile.TemporaryDirectory(prefix='skeeto-memory-')
        out = Path(cls.temp.name); cpp = out / 'test.cpp'
        cpp.write_text(HARNESS + '\n' + lifecycle + ui + row_struct + color + row_builder
                       + settings + worker + policy + pump + FIXTURES, encoding='utf-8')
        vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                      'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property',
                                      'installationPath'], text=True).strip()
        cls.exe = out / 'test.exe'; bat = out / 'compile.bat'
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                       f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /Fe"{cls.exe}" '
                       f'/Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n', encoding='utf-8')
        result = subprocess.run(['cmd', '/c', str(bat)], capture_output=True, text=True, errors='replace')
        if result.returncode: raise RuntimeError(result.stdout + result.stderr)

    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()

    def case(self, n):
        result = subprocess.run([str(self.exe), str(n)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_auto_clear_is_independent_and_throttled(self): self.case(0)
    def test_manual_clear_and_spam_cooldown(self): self.case(1)
    def test_automatic_pressure_is_rechecked(self): self.case(2)
    def test_unsafe_lifecycle_and_reentry_do_not_clear(self): self.case(3)
    def test_tick_wrap(self): self.case(4)
    def test_failed_zero_byte_and_coalesced_clears(self): self.case(5)
    def test_percentage_threshold_for_configured_cache_limits(self): self.case(6)
    def test_own_ini_migration_and_new_key_precedence(self): self.case(7)
    def test_bounded_address_scan_and_snapshot_lifecycle(self): self.case(8)
    def test_basic_detailed_rows_and_real_data_display(self): self.case(9)
    def test_rapid_toggle_and_late_snapshot_publication(self): self.case(10)


if __name__ == '__main__': unittest.main()
