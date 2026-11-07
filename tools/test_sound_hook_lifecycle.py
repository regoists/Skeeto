"""Exercise the production audio detours under retries, toggles and foreign patches."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, signature):
    at = source.index(signature)
    end = source.index("{", at) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[at:end] + "\n"


def main():
    source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8")
    fixture = r'''
#include <Windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using EngineSoundEmitFn = void(*)();
static DWORD g_paintTid = 0, now = 10000;
static DWORD g_soundVolumeInternalRetryAt = 0, g_soundStartRetryAt = 0;
static bool wanted = true, g_optClientIdleFlashlight = false, g_optGunFeel = false;
static bool g_soundVolumeInternalFailed = false, g_soundStartFailed = false;
static std::atomic<bool> g_engineSoundInternalHooked{false}, g_soundStartHooked{false}, g_safeMusicHookWanted{false};
static EngineSoundEmitFn g_origEngineSoundEmitInternal = nullptr;
static uint8_t *g_soundVolumeInternalTarget = nullptr, *g_soundStartTarget = nullptr, *g_soundStartTrampoline = nullptr;
static uint8_t internal[64], start[64];
static int scans = 0, allocations = 0, frees = 0, logs = 0, protects = 0;
static bool allocFail = false, protectFail = false;
static void* retained[4096]{}; static int count = 0;
static void require(bool value, const char* why) { if (!value) { std::fprintf(stderr, "FAIL: %s\n", why); std::exit(1); } }
static bool SoundVolumesWanted() { return wanted; }
static bool PtrCommitted(const void* p) { return p != nullptr; }
static void Log(const char*, ...) { ++logs; }
static void Hooked_EngineSoundEmitInternal() {}
static void SkeetoHookedSoundStart() {}
static int CountPat(const char*, const char* pattern, uintptr_t* hits, int) {
    ++scans; hits[0] = (uintptr_t)(strlen(pattern) > 40 ? start : internal); return 1;
}
static void* FakeAlloc(void* a, SIZE_T n, DWORD type, DWORD protection) {
    ++allocations; if (allocFail) return nullptr;
    void* p = ::VirtualAlloc(a, n, type, protection); require(p != nullptr, "fixture allocation");
    retained[count++] = p; return p;
}
static BOOL FakeFree(void* p, SIZE_T n, DWORD type) {
    ++frees; for (int i = 0; i < count; ++i) if (retained[i] == p) retained[i] = nullptr;
    return ::VirtualFree(p, n, type);
}
static BOOL FakeProtect(void*, SIZE_T, DWORD, DWORD* old) {
    ++protects; if (protectFail) return FALSE; *old = PAGE_EXECUTE_READ; return TRUE;
}
#define VirtualAlloc FakeAlloc
#define VirtualFree FakeFree
#define VirtualProtect FakeProtect
#define GetTickCount() now
'''
    for signature in (
        "static void UninstallSoundVolumeInternalHook() {",
        "static void InstallSoundVolumeInternalHook() {",
        "static void UninstallSoundStartVolumeHook() {",
        "static void InstallSoundStartVolumeHook() {",
    ):
        fixture += function(source, signature)
    fixture += r'''
#undef VirtualAlloc
#undef VirtualFree
#undef VirtualProtect
#undef GetTickCount
static void pump() { InstallSoundVolumeInternalHook(); InstallSoundStartVolumeHook(); }
static void reset() {
    for (int i = 0; i < count; ++i) if (retained[i]) ::VirtualFree(retained[i], 0, MEM_RELEASE);
    count = 0; memset(retained, 0, sizeof(retained));
    scans = allocations = frees = logs = protects = 0; allocFail = protectFail = false; wanted = true;
    g_soundVolumeInternalRetryAt = g_soundStartRetryAt = 0;
    g_soundVolumeInternalFailed = g_soundStartFailed = false;
    g_engineSoundInternalHooked = g_soundStartHooked = g_safeMusicHookWanted = false;
    g_origEngineSoundEmitInternal = nullptr;
    g_soundVolumeInternalTarget = g_soundStartTarget = g_soundStartTrampoline = nullptr;
    memset(internal, 0, sizeof(internal)); memset(start, 0, sizeof(start));
    const uint8_t prologue[] = {0x55, 0x8b, 0xec, 0x81, 0xec, 0x10, 0x03, 0, 0};
    memcpy(internal, prologue, 9); internal[9] = 0x6a;
    memcpy(start, prologue, 9); start[0x30] = 0x8b; start[0x31] = 0x4f; start[0x32] = 0x44;
}
int main() {
    g_paintTid = GetCurrentThreadId();
    reset(); wanted = false; for (int i = 0; i < 1000; ++i) pump();
    require(scans == 0 && allocations == 0, "disabled has no scan or allocation");
    reset(); allocFail = true;
    for (int i = 0; i < 1000; ++i) pump();
    require(scans == 2 && allocations == 2 && logs == 2, "allocation failure is bounded and does not rescan");
    now += 1000; pump(); require(scans == 2 && allocations == 4 && logs == 2, "retry reuses the resolved targets without log flood");
    allocFail = false; now += 1000; pump();
    require(g_engineSoundInternalHooked && g_soundStartHooked, "allocation recovery installs both detours");
    const int installedScans = scans, installedAllocs = allocations;
    for (int i = 0; i < 1000; ++i) pump();
    require(scans == installedScans && allocations == installedAllocs, "installed steady state is scan/allocation free");
    reset(); protectFail = true; for (int i = 0; i < 1000; ++i) pump();
    require(scans == 2 && allocations == 2 && protects == 2 && frees == 0, "protection failure retains at most one trampoline per target");
    now += 1000; pump(); require(scans == 2 && allocations == 2 && protects == 4 && logs == 2, "protection retry reuses trampolines");
    protectFail = false; now += 1000; pump(); require(g_engineSoundInternalHooked && g_soundStartHooked, "protection recovery installs");
    reset();
    for (int i = 0; i < 64; ++i) { wanted = true; pump(); wanted = false; pump(); }
    require(scans == 2 && allocations == 2 && frees == 0, "64 toggle cycles retain exactly two reusable trampolines");
    require(!g_engineSoundInternalHooked && !g_soundStartHooked && internal[0] == 0x55 && start[0] == 0x55, "off restores original bytes");
    internal[0] = start[0] = 0xcc; wanted = true; pump();
    require(internal[0] == 0xcc && start[0] == 0xcc && g_soundVolumeInternalFailed && g_soundStartFailed, "reinstall preserves a foreign patch");
    reset(); pump(); internal[0] = start[0] = 0xcc; wanted = false; pump();
    require(internal[0] == 0xcc && start[0] == 0xcc, "uninstall preserves a foreign patch");
    reset(); pump(); wanted = false; protectFail = true; protects = 0;
    for (int i = 0; i < 1000; ++i) pump();
    require(protects == 2 && g_engineSoundInternalHooked && g_soundStartHooked, "restore failure retries at a bounded cadence");
    protectFail = false; now += 1000; pump(); require(!g_engineSoundInternalHooked && !g_soundStartHooked, "restore recovers without losing ownership");
    reset(); wanted = false; g_safeMusicHookWanted = true; pump();
    require(!g_engineSoundInternalHooked && g_soundStartHooked, "safe-room music independently retains its sound-start hook");
    g_safeMusicHookWanted = false; g_optClientIdleFlashlight = true; pump();
    require(g_engineSoundInternalHooked && !g_soundStartHooked, "idle flashlight independently retains its internal hook");
    g_optClientIdleFlashlight = false; wanted = false; pump();
    reset(); now = 0xfffffff0u; allocFail = true; pump(); now += 999; pump();
    require(allocations == 2, "retry deadline survives tick rollover");
    now += 1; pump(); require(allocations == 4 && scans == 2, "retry is due after rollover");
    reset(); ++g_paintTid; pump(); require(scans == 0 && allocations == 0, "foreign thread cannot install");
    reset(); puts("PASS sound lifecycle: bounded retry, recovery, constant allocations, ownership, shared users and tick rollover");
}
'''
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-sound-lifecycle-") as tmp:
        folder = Path(tmp)
        (folder / "fixture.cpp").write_text(fixture, encoding="utf-8")
        (folder / "build.bat").write_text('@echo off\ncall "' + vs + '\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "build.bat"], cwd=folder, check=True)


if __name__ == "__main__":
    main()
