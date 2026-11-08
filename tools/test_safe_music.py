"""Compile actual sample/latch/mixer routing helpers; no engine/audio simulation."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block


class SafeMusic(unittest.TestCase):
    def test_real_predicate_latch_and_mixer_boundaries(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        extracted = '\n'.join(block(source, pattern) for pattern in [
            r'^static bool SafeMusicSampleMatches\(',
            r'^static bool SafeMusicRequestChapter\(',
            r'^static int\* AudioMixerPercentPtr\(',
            r'^static void SafeMusicPaintPump\(\) \{',
        ])
        harness = r'''
#include <atomic>
#include <cstring>
#include <cstdio>
#include <windows.h>
#define _stricmp _stricmp
static std::atomic_bool g_safeMusicHookWanted{true}, g_safeMusicChapterClaimed{false};
static std::atomic_bool g_safeMusicRequestBusy{false};
static std::atomic_uint g_safeMusicChapterEpoch{1};
static bool ready = false;
static int calls = 0;
static bool SkeetoSafeMusicQueuePlay() { ++calls; return ready; }
static constexpr int kAudioMixerConVarCount = 21, kSoundVolumeSliderCount = 17;
static int g_optSfxVol = 51, g_safeMusicVolume = 77, g_lastMusicVolume = 64, sliders[17]{};
static int* SoundVolumePercentPtr(int i) { return i >= 0 && i < 17 ? &sliders[i] : nullptr; }
static constexpr int kMenuPageSafeMusic=96, kMenuPageSafeMusicList=97;
static bool g_menuVisible=false, g_menuParked=false, g_optSafeMusic=false;
static bool g_safeMusicRandom=false, g_safeMusicPreviewPending=false, g_safeMusicConfigDirty=true;
static int g_menuPage=0, configureCalls=0, modeReads=0, resets=0;
static char g_gameL4d2Dir[32]{}, g_safeMusicSelected[256]{};
static int g_safeMusicCatalog=0;
static std::atomic_bool g_paintConnected{true}, g_safeMusicEventPending{false}, g_soundStartHooked{true};
static std::atomic_bool g_paintLoading{false};
static DWORD testNow=10000;
#define GetTickCount() testNow
static bool SkeetoFeaturesOn() { return true; }
static const char* ReadMpGamemode() { ++modeReads; return "coop"; }
static void SafeMusicResetChapter() { ++resets; }
static void SkeetoSafeMusicConfigure(const char*,bool,int,bool,const char*) { ++configureCalls; }
static bool SkeetoSafeMusicGetCatalog(int*) { return false; }
static void SkeetoSafeMusicResourcePump(bool) {}
static void MenuMarkDirty() {}
'''
        fixture = r'''
int main() {
 if (!SafeMusicSampleMatches("@#music/safe/TheMonstersWithout.wav")) return 1;
 if (!SafeMusicSampleMatches("music\\safe\\themonsterswithout_l4d1.wav")) return 2;
 if (SafeMusicSampleMatches("music/safe/themonsterswithout_s.wav")) return 3;
 if (SafeMusicSampleMatches("xmusic/safe/themonsterswithout.wav")) return 4;
 if (SafeMusicSampleMatches("music/safe/themonsterswithout.wav.bad")) return 5;
 if (SafeMusicSampleMatches(nullptr)) return 6;
 if (SafeMusicRequestChapter() || g_safeMusicChapterClaimed) return 7;
 ready = true;
 if (!SafeMusicRequestChapter() || !g_safeMusicChapterClaimed) return 8;
 int before = calls;
 if (!SafeMusicRequestChapter() || calls != before) return 9;
 g_safeMusicHookWanted = false;
 if (SafeMusicRequestChapter()) return 10;
 if (AudioMixerPercentPtr(-1) || AudioMixerPercentPtr(0) || AudioMixerPercentPtr(21)) return 11;
 if (AudioMixerPercentPtr(1) != &sliders[0] || AudioMixerPercentPtr(17) != &sliders[16]) return 12;
 if (AudioMixerPercentPtr(18) != &g_optSfxVol || AudioMixerPercentPtr(19) != &g_safeMusicVolume) return 13;
 if (AudioMixerPercentPtr(20) != &g_lastMusicVolume) return 18;
 SafeMusicPaintPump();
 configureCalls=modeReads=0;
 testNow+=1000; SafeMusicPaintPump();
 if (configureCalls || modeReads) return 14;
 g_optSafeMusic=true; SafeMusicPaintPump();
 if (configureCalls!=1 || modeReads!=1 || !g_safeMusicHookWanted) return 15;
 g_optSafeMusic=false; SafeMusicPaintPump();
 if (configureCalls!=2 || resets!=1 || g_safeMusicHookWanted) return 16;
 configureCalls=modeReads=0;
 testNow+=1000; SafeMusicPaintPump();
 if (configureCalls || modeReads) return 17;
 return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='skeeto-safe-') as directory:
            out = Path(directory); cpp = out / 'test.cpp'; exe = out / 'test.exe'
            cpp.write_text(harness + extracted + fixture, encoding='utf-8')
            vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
            bat = out / 'compile.bat'
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /Fe"{exe}" /Fo"{out / "test.obj"}" "{cpp}"\n'
                'exit /b %errorlevel%\n', encoding='utf-8')
            result = subprocess.run(['cmd', '/c', str(bat)], capture_output=True, text=True, errors='replace')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__': unittest.main()
