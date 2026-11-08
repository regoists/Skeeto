"""Compile actual HUD ownership/font functions against controlled Paint providers.

This proves bounded policy and state restoration, not in-game P0 performance.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

PRELUDE = r'''
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
static constexpr int kHudVguiBagMax = 24;
static bool g_hvPanelFaulted = false;
static unsigned g_hvChapterVisibleCalls = 0, g_hvChapterIdentityDrops = 0;
static unsigned g_hvIdentityDropsTotal = 0;
static bool g_hvRepairNeeded = false;
static DWORD g_hvSearchAt = 0;
static int g_hvSearchTries = 3, g_hvFaultAttempts = 0;
static bool g_hvFaultBlocked = false;
static unsigned g_hvChapterFaults = 0;
static constexpr int kHudVguiFaultMax = 3;
static DWORD g_hvFaultRetryAt = 0;
static void Log(const char*, ...) {}
static int px = 35, py = 46, pw = 200, ph = 80;
static bool visible = true, faultPos = false;
static unsigned identity = 123, posWrites = 0, sizeWrites = 0, visWrites = 0, posReads = 0;
static uint32_t HudVguiPanelIdentity(unsigned) { return identity; }
static void HudVguiGetPos(unsigned, int* x, int* y) {
    ++posReads;
    if (faultPos) { g_hvPanelFaulted = true; return; }
    *x = px; *y = py;
}
static void HudVguiGetSize(unsigned, int* w, int* h) { *w = pw; *h = ph; }
static bool HudVguiIsVisible(unsigned) { return visible; }
static void HudVguiSetPos(unsigned, int x, int y) { px = x; py = y; ++posWrites; }
static void HudVguiSetSize(unsigned, int w, int h) { pw = w; ph = h; ++sizeWrites; }
static void HudVguiSetVisible(unsigned, bool v) { visible = v; ++visWrites; }
static void* g_surf = (void*)1;
static bool g_sktUiFontsHold = false;
struct SurfFontCacheEntry { int tall, weight, flags; unsigned long font; };
static SurfFontCacheEntry g_surfFontCache[28]{};
static int g_surfFontCacheN = 0;
static int g_sktFontEpoch = 1, g_dmgFontTall = 0, g_dmgCritFontTall = 0;
static unsigned long g_dmgFont = 0, g_dmgCritFont = 0;
static constexpr int kFontFlagAntiAlias = 1, kFontFlagDropShadow = 2, kFontFlagOutline = 4;
static constexpr int kSurfFontFamilyMax = 6;
static unsigned fontAttempts = 0;
static bool fontOk = false;
static int SurfNormalizeVguiFontTall(int tall) { return tall; }
static unsigned long SurfMakeGameFontRaw(int, int, int) { ++fontAttempts; return fontOk ? fontAttempts : 0; }
static unsigned starts = 0, finishes = 0;
static unsigned char drawing = 0;
static bool panelApiOk = true;
static unsigned rootPanel = 100, rootQueries = 0, pushes = 0, pops = 0, lastPushed = 0;
static int clipLeft = 800, originX = 100, panelDepth = 0;
static bool textQueued = false, textFlushedInContext = false;
static unsigned textFlushes = 0;
using SurfDrawingFn = void(__thiscall*)(void*);
static void __fastcall FakeStart(void*, void*) { ++starts; drawing = 1; }
static void __fastcall FakeFinish(void*, void*) { ++finishes; drawing = 0; }
using SurfGetPanelFn = unsigned int(__thiscall*)(void*);
using SurfPushPanelFn = void(__thiscall*)(void*, unsigned int, bool);
using SurfPopPanelFn = void(__thiscall*)(void*, unsigned int);
static unsigned int __fastcall FakeRoot(void*, void*) { ++rootQueries; return rootPanel; }
static void __fastcall FakePush(void*, void*, unsigned p, bool) {
    ++pushes; ++panelDepth; lastPushed = p; clipLeft = 0; originX = 0;
}
static void __fastcall FakeFlushText(void*, void*) {
    ++textFlushes;
    if (textQueued) { textFlushedInContext = drawing != 0; textQueued = false; }
}
static void __fastcall FakePop(void* self, void*, unsigned) {
    ++pops; --panelDepth; FakeFlushText(self, nullptr);
}
static SurfDrawingFn g_surfStartDrawing = (SurfDrawingFn)FakeStart;
static SurfDrawingFn g_surfFinishDrawing = (SurfDrawingFn)FakeFinish;
static SurfGetPanelFn g_surfGetDrawingPanel = (SurfGetPanelFn)FakeRoot;
static SurfPushPanelFn g_surfPushDrawingPanel = (SurfPushPanelFn)FakePush;
static SurfPopPanelFn g_surfPopDrawingPanel = (SurfPopPanelFn)FakePop;
static SurfDrawingFn g_surfFlushDrawingText = (SurfDrawingFn)FakeFlushText;
static thread_local unsigned int g_surfOwnedDrawingPanel = 0;
static bool SurfEnsureDrawingPanelFns() { return panelApiOk; }
static volatile unsigned char* g_surfInDrawing = &drawing;
static int g_surfDrawingResolved = 1;
static bool MenuEnsureSurf() { return true; }
static bool SurfEnsureDrawingFns() { return true; }
static bool queued = false, flushedWhileDrawing = false;
static void SurfFillFlush() { if (queued) { flushedWhileDrawing = drawing != 0; queued = false; } }
static void SurfDrawColorInvalidate() {}
static void* g_ipanel = (void*)2;
static void* method = (void*)4;
static unsigned execQueries = 0;
static bool apiProbeMode = false, modulePresent = true;
static constexpr int kSurfGetEmbeddedPanel = 6;
static int g_surfDrawingPanelResolved = 0;
static unsigned char nativeImage[4096]{};
static void* apiSlots[4]{};
static void* VGet(void* object, int slot) {
    if (apiProbeMode && object == g_surf) {
        if (slot == 6) return apiSlots[0]; if (slot == 8) return apiSlots[1];
        if (slot == 9) return apiSlots[2]; if (slot == 24) return apiSlots[3];
        return nullptr;
    }
    return method;
}
static bool IsExec(void* p) { ++execQueries; return p != (void*)6; }
static HMODULE FakeDrawingModule(const char*) { return modulePresent ? (HMODULE)nativeImage : nullptr; }
static void InitPanelApiImage() {
    auto* dos = (IMAGE_DOS_HEADER*)nativeImage;
    dos->e_magic = IMAGE_DOS_SIGNATURE; dos->e_lfanew = 128;
    auto* nt = (IMAGE_NT_HEADERS*)(nativeImage + 128);
    nt->Signature = IMAGE_NT_SIGNATURE; nt->OptionalHeader.SizeOfImage = sizeof(nativeImage);
    const unsigned char root[] = {0x8B,0x81,0x44,0x02,0,0,0xC3};
    const unsigned char push[] = {0x55,0x8B,0xEC,0x83,0xEC,0x3C,0xA1};
    const unsigned char pop[] = {0x56,0x57,0x8B,0xF9,0x83,0xBF,0xA4,0x02,0x01,0,0};
    const unsigned char flush[] = {0x55,0x8B,0xEC,0x51,0x56,0x8B,0xF1,0x83,0xBE,0xA4,0x02,0x01,0,0};
    for (int i = 0; i < 4; ++i) apiSlots[i] = nativeImage + 512 * (i+1);
    memcpy(apiSlots[0], root, sizeof(root)); memcpy(apiSlots[1], push, sizeof(push));
    memcpy(apiSlots[2], pop, sizeof(pop)); memcpy(apiSlots[3], flush, sizeof(flush));
    const unsigned char ret8[] = {0xC2,8,0}, ret4[] = {0xC2,4,0};
    memcpy((unsigned char*)apiSlots[1]+0x13B, ret8, sizeof(ret8));
    memcpy((unsigned char*)apiSlots[2]+0x98, ret4, sizeof(ret4));
    ((unsigned char*)apiSlots[3])[0x82] = 0xC3;
    apiProbeMode = true;
}
'''

CASES = r'''
static bool Check(bool ok, const char* label) { if (!ok) printf("%s\n", label); return ok; }
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    int test = atoi(argv[1]); bool ok = true;
    HudVguiBag b{}; HudVguiBagAdd(&b, 1);
    if (test == 0) {
        visible = false;
        HudVguiApplyBag(&b, true); HudVguiApplyBag(&b, false);
        ok &= Check(!visible, "restore must not show a panel that was originally invisible");
        ok &= Check(sizeWrites == 0, "hide does not own the panel size");
    } else if (test == 1) {
        identity = 999;
        HudVguiApplyBag(&b, true);
        ok &= Check(posWrites == 0 && visWrites == 0 && b.n == 0, "aliased panel must be dropped before mutation");
    } else if (test == 2) {
        HudVguiApplyBag(&b, true);
        px = 125; py = 77; pw = 400; ph = 100;
        HudVguiApplyBag(&b, false);
        ok &= Check(px == 125 && py == 77 && pw == 400 && ph == 100, "external relayout must win over an obsolete hide snapshot");
        ok &= Check(!visible, "do not restore visibility after ownership was lost");
    } else if (test == 3) {
        for (int i = 0; i < 100; ++i) HudVguiApplyBag(&b, true);
        ok &= Check(posWrites == 1 && visWrites == 1, "stable hide must not rewrite native state each Paint");
        HudVguiApplyBag(&b, false);
        ok &= Check(px == 35 && py == 46 && visible, "owned state is restored");
        ok &= Check(sizeWrites == 0, "owned restoration never resizes");
    } else if (test == 4) {
        HudVguiApplyBag(&b, true); identity = 456;
        HudVguiApplyBag(&b, false);
        ok &= Check(posWrites == 1 && visWrites == 1 && b.n == 0, "restore must not mutate a replacement panel");
    } else if (test == 5) {
        faultPos = true;
        HudVguiApplyBag(&b, true); g_hvPanelFaulted = false;
        HudVguiApplyBag(&b, true);
        ok &= Check(b.n == 0 && posReads == 1, "faulting cached panel is evicted instead of retried forever");
    } else if (test == 6) {
        for (int i = 0; i < 1000; ++i) SurfAcquireVguiFont(18, 700, 1);
        ok &= Check(fontAttempts == 1, "failed glyph setup must not allocate an HFont each Paint");
    } else if (test == 7) {
        fontOk = true;
        for (int i = 12; i < 60; ++i) SurfAcquireVguiFont(i, 700, 1);
        ok &= Check(fontAttempts == 6 && g_surfFontCacheN == 6, "successful adjustable font family stays bounded");
    } else if (test == 8) {
        drawing = 1; bool owned = false;
        ok &= Check(TestDrawingBegin(&owned), "existing engine context is usable");
        TestDrawingEnd(owned);
        ok &= Check(starts == 0 && finishes == 0 && drawing == 1, "borrowed engine context must not be restarted or closed");
    } else if (test == 9) {
        bool owned = false;
        ok &= Check(TestDrawingBegin(&owned), "closed context can be opened");
        bool nested = false; TestDrawingBegin(&nested); TestDrawingEnd(nested);
        ok &= Check(drawing == 1 && starts == 1 && finishes == 0, "nested drawing borrows the outer context");
        TestDrawingEnd(owned);
        ok &= Check(starts == 1 && finishes == 1 && drawing == 0, "owned context closes exactly once");
    } else if (test == 10) {
        for (int i = 0; i < 3; ++i) { g_hvPanelFaulted = true; HudVguiHandleFault(); }
        ok &= Check(g_hvFaultBlocked && g_hvFaultRetryAt == 0 && g_hvFaultAttempts == 3, "fault recovery ends at a fixed budget");
        ok &= Check(g_hvSearchTries == 3, "fault recovery must not reopen the chapter search budget");
    } else if (test == 11) {
        HudVguiApplyBag(&b, true);
        HudVguiBagAdd(&b, 2);
        ok &= Check(HudVguiBagKeepOwned(&b) && b.n == 1, "map invalidation retains only owned restore snapshots");
        HudVguiApplyBag(&b, false);
        ok &= Check(px == 35 && py == 46 && visible, "persistent panel restores across invalidation");
    } else if (test == 12) {
        for (int i = 0; i < 1000; ++i) HudVguiMethod(3);
        ok &= Check(execQueries == 1, "stable IPanel method validation is cached");
        method = (void*)5;
        ok &= Check(HudVguiMethod(3) == method && execQueries == 2, "changed plugin slot is revalidated immediately");
        method = (void*)6;
        ok &= Check(!HudVguiMethod(3) && g_hvPanelFaulted, "invalid replacement method is not dispatched");
    } else if (test == 13) {
        { SurfPaintDrawingScope scope; scope.active = true; }
        ok &= Check(starts == 0 && finishes == 0, "an empty Paint does not acquire rendering resources");
    } else if (test == 14) {
        { SurfPaintDrawingScope scope; scope.active = true;
          for (int i = 0; i < 1000; ++i) scope.Touch(); queued = true; }
        ok &= Check(starts == 1 && finishes == 1 && flushedWhileDrawing, "final geometry submits before owned context closes");
    } else if (test == 15) {
        SurfPaintDrawingScope scope; scope.active = true;
        scope.Touch(); scope.Suspend();
        ok &= Check(!drawing && finishes == 1, "recursive original Paint suspends the owned context");
        scope.Touch();
        ok &= Check(drawing && starts == 2, "outer drawing can acquire a fresh context after recursive original Paint");
    } else if (test == 16) {
        for (int i = 0; i < 1000; ++i) { SurfEnsureDmgFont(); SurfEnsureDmgCritFont(); }
        ok &= Check(fontAttempts == 2, "dedicated damage fonts have one attempt each per epoch");
        ++g_sktFontEpoch;
        SurfEnsureDmgFont(); SurfEnsureDmgCritFont();
        ok &= Check(fontAttempts == 4, "explicit reload permits one dedicated font retry");
    } else if (test == 17) {
        g_sktUiFontsHold = true;
        SurfEnsureDmgFont(); SurfEnsureDmgCritFont();
        ok &= Check(fontAttempts == 0, "display reset blocks dedicated font allocation");
        g_sktUiFontsHold = false; fontOk = true;
        SurfEnsureDmgFont(); SurfEnsureDmgCritFont();
        ok &= Check(g_dmgFont && g_dmgCritFont && fontAttempts == 2, "font allocation resumes after reset");
    } else if (test == 21) {
        HudVguiBagAdd(&g_hvDmgRecord, 1);
        KillFeedRecordApply(true); KillFeedRecordApply(false);
        ok &= Check(px == 35 && py == 46 && visible && sizeWrites == 0, "native killfeed uses the same visibility/position ownership policy");
    } else if (test == 22) {
        SurfPaintDrawingScope scope; scope.active = true;
        g_surfFillBatchN = 7; g_surfDrawColorValid = true;
        bool isolated = false;
        std::thread worker([&] {
            isolated = !g_surfPaintDrawingScope && g_surfFillBatchN == 0 && !g_surfDrawColorValid;
            g_surfFillBatchN = 2; g_surfDrawColorValid = false;
        }); worker.join();
        ok &= Check(isolated && g_surfFillBatchN == 7 && g_surfDrawColorValid, "GDI thread must not see or overwrite Paint drawing scope/batch/color state");
        g_surfFillBatchN = 0;
    } else if (test >= 18 && test <= 20) {
        uint8_t image[1024]{}; auto* start = image; auto* finish = image + 256;
        auto store = [](uint8_t* p, uint8_t* flag, uint8_t value) {
            p[0] = 0xC6; p[1] = 0x05; p[6] = value;
            uint32_t address = (uint32_t)(uintptr_t)flag;
            memcpy(p + 2, &address, sizeof(address));
        };
        store(start + 30, image + 901, 1); // Unrelated initialization flag.
        store(start + 80, image + 900, 1);
        store(finish + 40, image + 900, 0);
        if (test == 18) {
            ok &= Check(SurfDrawingSharedFlag(start, finish, image, sizeof(image)) == image + 900, "drawing pair must share the same flag, not an arbitrary byte store");
        } else if (test == 19) {
            store(finish + 70, image + 901, 0);
            ok &= Check(!SurfDrawingSharedFlag(start, finish, image, sizeof(image)), "ambiguous shared flags must be rejected");
        } else {
            store(start + 80, image + 900, 0);
            store(start + 90, image + sizeof(image), 1);
            store(finish + 50, image + sizeof(image), 0);
            ok &= Check(!SurfDrawingSharedFlag(start, finish, image, sizeof(image)), "out-of-module flag must be rejected");
        }
    } else if (test == 23) {
        bool own = false; TestDrawingBegin(&own);
        ok &= Check(own && clipLeft == 0 && originX == 0 && pushes == 1,
            "new owned context must select screen root instead of inheriting observer clipping");
        TestDrawingEnd(own);
        ok &= Check(pops == 1 && panelDepth == 0 && finishes == 1,
            "owned screen root must pop before context closes");
    } else if (test == 24) {
        drawing = 1;
        { SurfPaintDrawingScope scope; scope.active = true; scope.Touch(); textQueued = true; }
        ok &= Check(starts == 0 && finishes == 0 && pushes == 0 && pops == 0
            && clipLeft == 800 && originX == 100 && drawing == 1,
            "borrowed context keeps its panel coordinates and ownership");
        ok &= Check(textFlushedInContext && !textQueued,
            "borrowed SKT text submits before handing context back");
    } else if (test == 25) {
        { SurfPaintDrawingScope scope; scope.active = true; scope.Touch(); textQueued = true; }
        ok &= Check(textFlushedInContext && !textQueued && textFlushes == 1
            && pops == 1 && finishes == 1, "native pop flushes owned glyphs before FinishDrawing");
    } else if (test == 26) {
        { SurfPaintDrawingScope scope; scope.active = true; scope.Touch();
          queued = textQueued = true; scope.Suspend();
          ok &= Check(flushedWhileDrawing && textFlushedInContext && panelDepth == 0,
              "recursive engine Paint gets no pending SKT geometry or panel stack entry");
          rootPanel = 200; scope.Touch(); }
        ok &= Check(pushes == 2 && pops == 2 && lastPushed == 200,
            "resume queries the current root instead of reusing a map-sensitive panel");
    } else if (test == 27) {
        { SurfPaintDrawingScope scope; scope.active = true; }
        ok &= Check(rootQueries == 0 && pushes == 0 && pops == 0 && textFlushes == 0,
            "empty Paint does not query a root or touch the engine text buffer");
    } else if (test == 28) {
        rootPanel = 0; bool own = false;
        ok &= Check(!TestDrawingBegin(&own) && !own && starts == 0 && pushes == 0,
            "missing root is rejected before changing native drawing state");
    } else if (test == 29) {
        panelApiOk = false; bool own = false;
        ok &= Check(!TestDrawingBegin(&own) && starts == 0 && pushes == 0,
            "unverified panel API is never called");
    } else if (test == 30) {
        g_surfOwnedDrawingPanel = 100;
        std::thread worker([&] { ok &= Check(g_surfOwnedDrawingPanel == 0,
            "UI thread must not inherit the Paint thread root ownership"); });
        worker.join();
        ok &= Check(g_surfOwnedDrawingPanel == 100, "UI thread cannot consume the Paint root");
    } else if (test >= 31 && test <= 35) {
        InitPanelApiImage();
        if (test == 32) ((unsigned char*)apiSlots[2])[0x99] = 8;
        if (test == 33) apiSlots[1] = (void*)1;
        if (test == 34) ((unsigned char*)apiSlots[0])[2] = 0x45;
        if (test == 35) modulePresent = false;
        bool valid = RetailSurfEnsureDrawingPanelFns();
        ok &= Check(valid == (test == 31), "retail panel ABI guard validates module, getter and x86 stack cleanup");
        const unsigned probes = execQueries;
        for (int i = 0; i < 1000; ++i)
            ok &= RetailSurfEnsureDrawingPanelFns() == valid;
        ok &= Check(execQueries == probes, "ABI guard never re-probes each Paint after success or failure");
    } else return 2;
    return ok ? 0 : 1;
}
'''


class P0HudLifecycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        extracted = block(source, r'^struct HudVguiBag \{') + ';\n'
        extracted += 'static HudVguiBag g_hvDmgRecord{};\n'
        extracted += block(source, r'^struct SurfPaintDrawingScope \{') + ';\n'
        for name in ['g_surfPaintDrawingScope', 'g_surfFillBatchN', 'g_surfDrawColorValid']:
            extracted += re.search(r'^static [^;\n]+\b' + name + r'[^;\n]*;', source, re.M).group() + '\n'
        for pattern in [r'^static void HudVguiBagDropAt\(', r'^static bool HudVguiBagValidateAt\(', r'^static void HudVguiBagAdd\(',
                        r'^static bool HudVguiBagKeepOwned\(', r'^static void HudVguiHandleFault\(', r'^static void\* HudVguiMethod\(',
                        r'^static void HudVguiApplyBag\(', r'^unsigned long SurfAcquireVguiFont\([^\n]+\) \{',
                        r'^static void KillFeedRecordApply\(',
                        r'^static (?:void|bool) SurfDrawingBegin\([^\n]*\) \{', r'^static void SurfDrawingEnd\([^\n]*\) \{',
                        r'^static uint8_t\* SurfDrawingSharedFlag\(',
                        r'^SurfPaintDrawingScope::SurfPaintDrawingScope\(', r'^bool SurfPaintDrawingScope::Touch\(',
                        r'^void SurfPaintDrawingScope::Suspend\(', r'^SurfPaintDrawingScope::~SurfPaintDrawingScope\(',
                        r'^static void SurfEnsureDmgFont\(\) \{', r'^static void SurfEnsureDmgCritFont\(\) \{']:
            extracted += block(source, pattern) + '\n'
        if re.search(r'^static void SurfDrawingFlushText\(', source, re.M):
            extracted = 'static void SurfDrawingFlushText();\n' + extracted
            extracted += block(source, r'^static void SurfDrawingFlushText\([^\n]*\) \{') + '\n'
        if re.search(r'^static bool SurfEnsureDrawingPanelFns\(', source, re.M):
            extracted += '#define GetModuleHandleA FakeDrawingModule\n'
            extracted += block(source, r'^static bool SurfEnsureDrawingPanelFns\(').replace(
                'SurfEnsureDrawingPanelFns()', 'RetailSurfEnsureDrawingPanelFns()') + '\n'
            extracted += '#undef GetModuleHandleA\n'
        else:
            extracted += 'static bool RetailSurfEnsureDrawingPanelFns() { return true; }\n'
        # Both revisions compile: the red run executes the previous unconditional
        # Begin/End behavior, and the green run executes the owned context API.
        if re.search(r'^static bool SurfDrawingBegin\(', source, re.M):
            adapters = 'static bool TestDrawingBegin(bool* o) { return SurfDrawingBegin(o); }\n'
            adapters += 'static void TestDrawingEnd(bool o) { SurfDrawingEnd(o); }\n'
        else:
            adapters = 'static bool TestDrawingBegin(bool* o) { SurfDrawingBegin(); *o = true; return true; }\n'
            adapters += 'static void TestDrawingEnd(bool) { SurfDrawingEnd(); }\n'
        cls.temp = tempfile.TemporaryDirectory(prefix='skeeto-p0-')
        out = Path(cls.temp.name); cpp = out / 'test.cpp'
        cpp.write_text(PRELUDE + extracted + adapters + CASES, encoding='utf-8')
        vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                     'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                                     '-property', 'installationPath'], text=True).strip()
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

    def test_original_invisibility_is_restored(self): self.case(0)
    def test_identity_before_hide(self): self.case(1)
    def test_external_layout_is_preserved(self): self.case(2)
    def test_stable_hide_and_restore(self): self.case(3)
    def test_identity_before_restore(self): self.case(4)
    def test_faulting_panel_is_evicted(self): self.case(5)
    def test_font_failure_has_a_fixed_attempt_budget(self): self.case(6)
    def test_successful_font_family_stays_bounded(self): self.case(7)
    def test_borrowed_engine_context_is_preserved(self): self.case(8)
    def test_nested_context_ownership(self): self.case(9)
    def test_fault_recovery_has_a_fixed_budget(self): self.case(10)
    def test_map_boundary_retains_owned_snapshots(self): self.case(11)
    def test_method_cache_observes_plugin_slot_changes(self): self.case(12)
    def test_empty_paint_does_not_acquire_a_context(self): self.case(13)
    def test_pending_geometry_precedes_context_close(self): self.case(14)
    def test_original_reentry_suspends_and_resumes(self): self.case(15)
    def test_dedicated_font_failure_is_bounded(self): self.case(16)
    def test_dedicated_fonts_observe_display_reset(self): self.case(17)
    def test_surface_drawing_pair_shares_the_same_flag(self): self.case(18)
    def test_ambiguous_surface_flag_is_rejected(self): self.case(19)
    def test_surface_flag_must_belong_to_the_module(self): self.case(20)
    def test_native_killfeed_uses_the_shared_restore_policy(self): self.case(21)
    def test_gdi_and_paint_drawing_state_is_thread_local(self): self.case(22)
    def test_owned_context_binds_screen_root(self): self.case(23)
    def test_borrowed_context_keeps_coordinates_and_submits_text(self): self.case(24)
    def test_owned_context_submits_text_before_finish(self): self.case(25)
    def test_reentry_pops_and_reacquires_current_root(self): self.case(26)
    def test_empty_paint_does_not_touch_panel_or_glyphs(self): self.case(27)
    def test_missing_root_does_not_open_context(self): self.case(28)
    def test_unverified_panel_api_is_rejected(self): self.case(29)
    def test_panel_ownership_is_thread_local(self): self.case(30)
    def test_retail_panel_api_validation_is_cached(self): self.case(31)
    def test_wrong_panel_pop_stack_cleanup_is_rejected(self): self.case(32)
    def test_panel_methods_must_belong_to_retail_module(self): self.case(33)
    def test_unrecognized_root_getter_is_rejected(self): self.case(34)
    def test_missing_panel_module_is_rejected(self): self.case(35)


if __name__ == '__main__': unittest.main()
