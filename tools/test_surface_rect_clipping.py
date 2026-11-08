"""Exercise production fill submission with retail clipping behavior providers.

Retail DrawFilledRectArray ignores failed clipping; DrawFilledRect skips it.
This is a bounded rendering regression test, not an in-game screenshot test.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

PROVIDERS = r'''
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static constexpr int kVmtSurfDrawFilledRect = 12;
static constexpr int kVmtSurfDrawFilledRectArray = 13;
static bool drawingOk = true;
static bool SurfDrawingTouch() { return drawingOk; }
static bool IsExec(void* p) { return p != nullptr; }
static void Log(const char*, ...) {}
static unsigned char surface[64]{};
static void* g_surf = surface;
'''

NATIVE = r'''
static int arrayCalls = 0, singleCalls = 0, visibleRects = 0, invalidRects = 0;
static SurfFillRect clip{800, 700, 1900, 950};
static bool Clip(SurfFillRect& r) {
    r.x0 = std::max(r.x0, clip.x0); r.y0 = std::max(r.y0, clip.y0);
    r.x1 = std::min(r.x1, clip.x1); r.y1 = std::min(r.y1, clip.y1);
    return r.x0 <= r.x1 && r.y0 <= r.y1;
}
static void __fastcall Single(void*, void*, int x0, int y0, int x1, int y1) {
    ++singleCalls;
    SurfFillRect r{x0, y0, x1, y1};
    if (Clip(r)) ++visibleRects;
}
static void __fastcall Array(void*, void*, SurfFillRect* rs, int n) {
    ++arrayCalls;
    for (int i = 0; i < n; ++i) {
        auto r = rs[i];
        if (!Clip(r)) ++invalidRects; // Retail array still emits these vertices.
        else ++visibleRects;
    }
}
static void* VGet(void*, int slot) {
    return slot == kVmtSurfDrawFilledRect ? (void*)&Single : (void*)&Array;
}
'''

CASES = r'''
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    int which = atoi(argv[1]);
    *(void**)(surface + 0x34) = (void*)1;
    if (which == 0) {
        // Idle native clipping region excludes both ordinary HUD rectangles.
        g_surfFillBatch[0] = {20, 200, 220, 204};
        g_surfFillBatch[1] = {30, 240, 230, 244};
        g_surfFillBatchN = 2;
    } else if (which == 1) {
        g_surfFillBatch[0] = {20, 200, 220, 204};
        g_surfFillBatch[1] = {900, 750, 1000, 760};
        g_surfFillBatchN = 2;
    } else if (which == 2) {
        for (int i = 0; i < 32; ++i) g_surfFillBatch[i] = {900+i, 750, 1000+i, 760};
        g_surfFillBatchN = 32;
    } else if (which == 3) {
        g_surfFillBatchN = 0;
    } else if (which == 4) {
        drawingOk = false;
        g_surfFillBatch[0] = {900, 750, 1000, 760};
        g_surfFillBatchN = 1;
    } else return 2;
    SurfFillFlush();
    bool ok = !arrayCalls && !invalidRects && !g_surfFillBatchN;
    if (which == 0) ok &= visibleRects == 0 && singleCalls == 2;
    if (which == 1) ok &= visibleRects == 1 && singleCalls == 2;
    if (which == 2) ok &= visibleRects == 32 && singleCalls == 32;
    if (which >= 3) ok &= visibleRects == 0 && singleCalls == 0;
    if (!ok) printf("array=%d single=%d visible=%d inverted=%d pending=%d\n",
                    arrayCalls, singleCalls, visibleRects, invalidRects, g_surfFillBatchN);
    return ok ? 0 : 1;
}
'''


class SurfaceRectClipping(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        extracted = block(source, r'^struct SurfFillRect \{') + ';\n'
        for name in ['g_surfFillBatch', 'g_surfFillBatchN', 'g_surfFillBatchBroken']:
            decl = re.search(r'^static [^;\n]+\b' + name + r'[^;\n]*;', source, re.M)
            if decl:
                extracted += decl.group().replace('[kSurfFillBatchMax]', '[32]') + '\n'
        functions = block(source, r'^static void SurfFillEmitImmediate\(') + '\n'
        functions += block(source, r'^static void SurfFillFlush\(\) \{') + '\n'
        cls.temp = tempfile.TemporaryDirectory(prefix='skeeto-rect-clip-')
        out = Path(cls.temp.name)
        cpp = out / 'test.cpp'
        cpp.write_text(PROVIDERS + extracted + NATIVE + functions + CASES, encoding='utf-8')
        vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                     'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                     '-property', 'installationPath'], text=True).strip()
        cls.exe = out / 'test.exe'
        bat = out / 'compile.bat'
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                       f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /Fe"{cls.exe}" '
                       f'/Fo"{out / "test.obj"}" "{cpp}"\n', encoding='ascii')
        result = subprocess.run(['cmd', '/c', str(bat)], capture_output=True,
                                encoding='utf-8', errors='replace')
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def case(self, index):
        result = subprocess.run([str(self.exe), str(index)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_fully_clipped_idle_hud_has_no_color_blocks(self): self.case(0)
    def test_clipped_and_visible_rectangles_are_independent(self): self.case(1)
    def test_full_queue_stays_bounded(self): self.case(2)
    def test_empty_queue_does_not_draw(self): self.case(3)
    def test_unavailable_context_drops_pending_geometry(self): self.case(4)


if __name__ == '__main__':
    unittest.main()
