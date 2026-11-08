"""Exercise production GAME enumeration and catalog filtering with fake mounts."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block


class Resources(unittest.TestCase):
    def test_game_domain_budget_and_shared_catalog(self):
        style = (ROOT / 'src/skeeto/core/skeeto_style.cpp').read_text(encoding='utf-8-sig')
        music = (ROOT / 'src/skeeto/core/skeeto_background.cpp').read_text(encoding='utf-8-sig')
        extracted = block(style, r'^struct DlcGameGlobScan \{') + ';\n'
        for name in ['DlcGameGlobScanCancel', 'DlcGameGlobScanAbandon', 'DlcGameGlobScanBegin', 'DlcGameGlobScanPump']:
            extracted += block(style, r'^(?:void|int|DlcGameGlobScan\*) ' + name + r'\(') + '\n'
        extracted += block(music, r'^static void SafeMusicCatalogAddName\(')
        harness = r'''
#include <windows.h>
#include <cstring>
#include <new>
#include <cstdio>
using FileFindHandle_t = int;
static int nextCalls = 0, closed = 0;
static bool gameDomain = false;
struct IFileSystem {
 const char* FindFirstEx(const char*, const char* domain, int* h) {
  gameDomain = !strcmp(domain, "GAME"); *h = 0; return "loose.wav";
 }
 const char* FindNext(int h) {
  ++nextCalls;
  return nextCalls == 1 ? "addon.mp3" : nextCalls == 2 ? "LOOSE.WAV" : nullptr;
 }
 bool FindIsDirectory(int) { return false; }
 void FindClose(int) { ++closed; }
};
static IFileSystem fake;
static IFileSystem* FsGet() { return &fake; }
struct SkeetoSafeMusicCatalog {
 unsigned revision; int count; bool truncated, failed; char names[128][256];
};
'''
        fixture = r'''
int main() {
 SkeetoSafeMusicCatalog catalog{};
 auto scan = DlcGameGlobScanBegin("sound/skeeto/safe/*.wav");
 if (!scan || !gameDomain || nextCalls) return 1;
 if (DlcGameGlobScanPump(scan, 1, SafeMusicCatalogAddName, &catalog) != 0 || nextCalls != 1) return 2;
 if (DlcGameGlobScanPump(scan, 1, SafeMusicCatalogAddName, &catalog) != 0 || nextCalls != 2) return 3;
 if (DlcGameGlobScanPump(scan, 1, SafeMusicCatalogAddName, &catalog) != 1 || nextCalls != 3) return 4;
 DlcGameGlobScanCancel(scan);
 if (closed != 1 || catalog.count != 2) return 5;
 SafeMusicCatalogAddName("../escape.wav", &catalog);
 SafeMusicCatalogAddName("sub/child.mp3", &catalog);
 SafeMusicCatalogAddName("bad.txt", &catalog);
 SafeMusicCatalogAddName("sneaky.wav.bad", &catalog);
 if (catalog.count != 2) return 6;
 for (int i=0;i<128;++i) { char name[40]; sprintf_s(name, "track%d.wav",i); SafeMusicCatalogAddName(name,&catalog); }
 if (catalog.count != 128 || !catalog.truncated) return 7;
 nextCalls=0;
 scan=DlcGameGlobScanBegin("sound/skeeto/safe/*.mp3");
 DlcGameGlobScanAbandon(scan);
 if (closed != 1) return 8; // shutdown never calls an engine method.
 return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='skeeto-safe-resources-') as directory:
            out = Path(directory); cpp = out/'test.cpp'; exe = out/'test.exe'
            cpp.write_text(harness+extracted+fixture,encoding='utf-8')
            vswhere = Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = subprocess.check_output([str(vswhere),'-latest','-products','*','-requires',
                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
            bat=out/'compile.bat'
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /Fe"{exe}" /Fo"{out/"test.obj"}" "{cpp}"\n'
                'exit /b %errorlevel%\n',encoding='utf-8')
            result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)


    def test_selected_file_staging_loading_and_generation(self):
        music = (ROOT / 'src/skeeto/core/skeeto_background.cpp').read_text(encoding='utf-8-sig')
        declarations = music[music.index('static SRWLOCK g_safeResourceLock'):music.index('static void SafeMusicCatalogAddName')]
        extracted = declarations + block(music, r'^static void SafeMusicCatalogAddName\(')
        extracted += block(music, r'^static void SafeMusicPublishCatalog\(')
        extracted += block(music, r'^void SkeetoSafeMusicResourcePump\(')
        harness = r'''
#include <windows.h>
#include <atomic>
#include <cstring>
#include <cstdlib>
#include <cstdio>
struct SkeetoSafeMusicCatalog { unsigned revision; int count; bool truncated,failed; char names[128][256]; };
static SRWLOCK g_safeLock=SRWLOCK_INIT;
static SkeetoSafeMusicCatalog g_safeCatalog{};
static std::atomic_bool g_safeEnabled{false},g_safeScan{false};
static std::atomic_uint g_safeGeneration{1};
static int enumerated=0,readCalls=0,closed=0;
static unsigned largestBudget=0;
static char selectedRelative[260]{};
static void SafeMusicWake() {}
struct DlcGameGlobScan { int phase; };
static DlcGameGlobScan* DlcGameGlobScanBegin(const char* glob) {
 ++enumerated; return new DlcGameGlobScan{strstr(glob,"*.mp3")?1:0};
}
static int DlcGameGlobScanPump(DlcGameGlobScan* scan,int budget,void(*fn)(const char*,void*),void* ctx) {
 if(budget!=16) return -1;
 fn(scan->phase?"loose.mp3":"packed.wav",ctx);
 return 1;
}
static void DlcGameGlobScanCancel(DlcGameGlobScan* scan) { delete scan; }
struct DlcCacheExtract { int reads=0; };
static DlcCacheExtract* DlcCacheExtractBegin(const char* relative,const wchar_t* ext,unsigned cap) {
 strcpy_s(selectedRelative,relative);
 return cap==128u*1024u*1024u && !wcscmp(ext,L".wav") ? new DlcCacheExtract():nullptr;
}
static int DlcCacheExtractPump(DlcCacheExtract* extract,unsigned budget,wchar_t* out,size_t count) {
 ++readCalls; if(budget>largestBudget) largestBudget=budget;
 if(++extract->reads<3) return 0;
 wcscpy_s(out,count,L"C:\\private_cache\\selected.wav"); return 1;
}
static void DlcCacheExtractCancel(DlcCacheExtract* extract) { if(extract) ++closed; delete extract; }
'''
        fixture = r'''
int main() {
 SkeetoSafeMusicResourcePump(true);
 if(enumerated || readCalls) return 1;
 g_safeEnabled=true; g_safeScan=true;
 SkeetoSafeMusicResourcePump(false);
 if(enumerated || readCalls) return 2;
 SkeetoSafeMusicResourcePump(true); SkeetoSafeMusicResourcePump(true);
 if(enumerated!=2 || !g_safeCatalogReady || g_safeCatalog.count!=2) return 3;
 strcpy_s(g_safeSourceName,"packed.wav"); g_safeSourceId=1; g_safeSourceGeneration=1;
 SkeetoSafeMusicResourcePump(true);
 if(strcmp(selectedRelative,"sound/skeeto/safe/packed.wav") || readCalls!=1 || g_safeSourcePublishedId) return 4;
 SkeetoSafeMusicResourcePump(false);
 if(readCalls!=1) return 5;
 SkeetoSafeMusicResourcePump(true); SkeetoSafeMusicResourcePump(true);
 if(g_safeSourcePublishedId!=1 || !g_safeSourcePath[0] || largestBudget!=256u*1024u) return 6;
 SkeetoSafeMusicResourcePump(true);
 if(readCalls!=3) return 7;
 g_safeSourceId=2; SkeetoSafeMusicResourcePump(true);
 if(readCalls!=4) return 8;
 g_safeGeneration=2; SkeetoSafeMusicResourcePump(false);
 if(g_safeExtract || closed!=2 || readCalls!=4 || g_safeSourcePublishedId==2) return 9;
 g_safeSourceId=3; g_safeSourceGeneration=2; strcpy_s(g_safeSourceName,"../escape.wav");
 SkeetoSafeMusicResourcePump(true);
 if(g_safeSourcePublishedId!=3 || g_safeSourcePath[0] || readCalls!=4) return 10;
 g_safeEnabled=false; SkeetoSafeMusicResourcePump(true);
 if(g_safeCatalogReady || g_safeExtract || g_safeGlob) return 11;
 return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='skeeto-safe-stage-') as directory:
            out = Path(directory); cpp = out/'test.cpp'; exe = out/'test.exe'
            cpp.write_text(harness+extracted+fixture,encoding='utf-8')
            vswhere = Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = subprocess.check_output([str(vswhere),'-latest','-products','*','-requires',
                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
            bat=out/'compile.bat'
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /Fe"{exe}" /Fo"{out/"test.obj"}" "{cpp}"\n'
                'exit /b %errorlevel%\n',encoding='utf-8')
            result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)

if __name__ == '__main__': unittest.main()
