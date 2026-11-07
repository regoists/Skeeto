"""Verify production eye-position reads at actual readable/no-access boundaries."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8")
    at = source.index("bool LocalEyePos(float* outEye) {")
    end = source.index("\n}", at) + 2
    fixture = r'''
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include "skeeto_memread.h"
static void* g_entlist = (void*)1;
static void* entity = nullptr;
static int g_offOrigin = 0, g_offViewOffset = -1, local = 1;
static int EngLocal() { return local; }
static void* EntGet(int) { return entity; }
static bool EntReadable(void* p) { return skmem::Probe(p); }
static void require(bool value, const char* why) { if (!value) { fprintf(stderr, "FAIL: %s\n", why); exit(1); } }
'''
    fixture += source[at:end]
    fixture += r'''
static int call(float* out) {
    __try { return LocalEyePos(out) ? 1 : 0; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return -1; }
}
int main() {
    auto* pages = (uint8_t*)VirtualAlloc(nullptr, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    require(pages != nullptr, "fixture allocation"); DWORD old = 0;
    require(VirtualProtect(pages + 4096, 4096, PAGE_NOACCESS, &old) != FALSE, "fixture inaccessible field");
    entity = pages + 4080; const float origin[] = {1, 2, 3}; memcpy(entity, origin, sizeof(origin));
    float out[] = {90, 91, 92}; g_offOrigin = 32;
    require(call(out) == 0, "unreadable origin returns failure rather than raising an access violation");
    require(out[0] == 90 && out[1] == 91 && out[2] == 92, "failed origin does not publish partial output");
    require(skmem::g_faults == 1, "origin failure is observable in existing memory counters");
    g_offOrigin = 0; g_offViewOffset = 32;
    require(call(out) == 0 && out[0] == 90 && out[1] == 91 && out[2] == 92, "failed view offset preserves caller output");
    require(skmem::g_faults == 2, "view-offset failure is counted");
    g_offViewOffset = -1;
    require(call(out) == 1 && out[0] == 1 && out[1] == 2 && out[2] == 65, "missing view-offset retains the established height fallback");
    entity = pages; g_offOrigin = 0; g_offViewOffset = 32;
    memcpy(entity, origin, sizeof(origin)); const float view[] = {4, 5, 6}; memcpy(pages + 32, view, sizeof(view));
    require(call(out) == 1 && out[0] == 5 && out[1] == 7 && out[2] == 9, "valid origin and offset are summed");
    g_offOrigin = -1; require(call(out) == 0, "missing origin is unavailable");
    g_offOrigin = 0; entity = nullptr; require(call(out) == 0, "missing local entity is unavailable");
    local = 0; require(call(out) == 0, "disconnected local slot is unavailable");
    require(call(nullptr) == 0, "null output is rejected");
    VirtualFree(pages, 0, MEM_RELEASE);
    puts("PASS local eye: guarded fields, atomic output, valid coordinates and unchanged missing-netvar behavior");
}
'''
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-eye-") as tmp:
        folder = Path(tmp)
        (folder / "fixture.cpp").write_text(fixture, encoding="utf-8")
        (folder / "build.bat").write_text('@echo off\ncall "' + vs + '\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 /I "' + str(ROOT / 'src/skeeto/core') + '" fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "build.bat"], cwd=folder, check=True)


if __name__ == "__main__":
    main()
