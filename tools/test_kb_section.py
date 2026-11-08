"""Exercise production keyboard-list mounting with unavailable/overridden paths."""
import os
from pathlib import Path
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
#include <string>
static DWORD testNow=10000;
#define GetTickCount() testNow
static bool g_optKbListSection=true, g_kbSecApplied=false, g_kbSecWaiting=false;
static bool g_kbSecGaveUp=false, g_kbSecCleanPosted=false, g_kbSecFileReady=false;
static int g_kbSecSig=-1, g_kbSecFails=0, g_kbSecReaddLog=4;
static DWORD g_kbSecNextAt=0;
static std::atomic<int> g_kbSecReq{0}, g_kbSecResult{0};
static std::atomic_bool g_paintLoading{false}, g_uiEnglish{false};
static char g_kbSecJobDir[MAX_PATH]{}, g_kbSecJobData[65536]{};
static int g_kbSecJobLen=0, reads=0;
static bool mounted=false, readable=false;
static std::string original="stock";
static void Log(const char*,...) {}
static void KbListSectionDir(char* out,size_t) { strcpy_s(out,MAX_PATH,"D:\\game\\skeeto_kbact"); }
static void FsKbActRemovePath(const char*) { mounted=false; }
static void FsKbActAddPath(const char*) {}
static bool FsKbActSectionPresent(const char*) { return mounted; }
static bool FsKbActSectionAtHead(const char*) { return mounted; }
static bool FsKbActVerify(const char*,int) { return readable; }
static int FsKbActReadOriginal(const char*,char** out,int* len) {
    ++reads; *len=(int)original.size(); *out=(char*)malloc(original.size()+1);
    memcpy(*out,original.c_str(),original.size()+1); return 1;
}
static void KbListSectionBuild(char* out,int cap) { strcpy_s(out,cap,"ours\n"); }
'''

FIXTURES = r'''
int main(int argc,char** argv) {
    if(argc!=2 && argc!=3) return 2;
    int test=atoi(argv[1]);
    if(test==3) {
        FILE* f=nullptr; if(argc!=3 || fopen_s(&f,argv[2],"rb") || !f) return 7;
        original.clear();
        char bytes[1024]; size_t n=0;
        while((n=fread(bytes,1,sizeof(bytes),f))>0) original.append(bytes,n);
        const bool failed=ferror(f)!=0; fclose(f); if(failed) return 9;
    }
    KbListSectionTick();
    if(g_kbSecReq!=1 || reads!=1) return 3;
    if(test==3) {
        bool bom=original.size()>=3 && (unsigned char)original[0]==0xef
            && (unsigned char)original[1]==0xbb && (unsigned char)original[2]==0xbf;
        std::string expected=bom ? original.substr(0,3)+"ours\n"+original.substr(3)
            : "ours\n"+original;
        return g_kbSecJobLen==(int)expected.size()
            && !memcmp(g_kbSecJobData,expected.data(),expected.size()) ? 0:8;
    }
    g_kbSecReq=0; g_kbSecResult=1;
    mounted=(test!=0); readable=(test==2 || test==4);
    KbListSectionTick();
    if(test==4) {
        if(!g_kbSecApplied) return 10;
        original="late voice mod\n+mouse_menu Orders";
        mounted=false; testNow+=5000; KbListSectionTick();
        ++testNow; KbListSectionTick();
        const std::string expected="ours\n"+original;
        return reads==1 && g_kbSecReq==0 && g_kbSecApplied ? 0:11;
    }
    if(test==2) return g_kbSecApplied ? 0:4;
    if(g_kbSecApplied) { puts("unverified mount reported success"); return 5; }
    testNow+=2000; KbListSectionTick();
    if(reads!=1 || g_kbSecReq!=0) { puts("mount retry rewrote ready file"); return 6; }
    return 0;
}
'''

PATH_HARNESS = r'''
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
enum { PATH_ADD_TO_HEAD=0 };
struct IFileSystem {
    std::string paths;
    bool includedPacks=false;
    int removes=0, adds=0;
    int GetSearchPath(const char*,bool packs,char* out,int cap) {
        includedPacks=packs; strncpy_s(out,cap,paths.c_str(),_TRUNCATE); return (int)paths.size()+1;
    }
    bool RemoveSearchPath(const char* own,const char*) {
        ++removes; std::string needle=own;
        size_t at=paths.find(needle);
        if(at==std::string::npos) return false;
        size_t count=needle.size();
        if(at+count<paths.size() && paths[at+count]==';') ++count;
        else if(at && paths[at-1]==';') { --at; ++count; }
        paths.erase(at,count); return true;
    }
    void AddSearchPath(const char* own,const char*,int,int) {
        ++adds;
        if(paths.find(own)!=std::string::npos) return; // deduplicates, does not move
        paths=std::string(own)+(paths.empty()?"":";"+paths);
    }
};
static IFileSystem filesystem;
static IFileSystem* FsGet() { return &filesystem; }
'''

PATH_FIXTURES = r'''
int main() {
    const char* own="D:\\game\\skeeto_kbact";
    filesystem.paths="D:\\game\\addons\\voice.vpk;D:\\game\\skeeto_kbact;D:\\game\\left4dead2";
    if(FsKbActSectionAtHead(own)) return 1;
    if(!filesystem.includedPacks) return 2;
    FsKbActAddPath(own);
    if(filesystem.removes!=1 || filesystem.adds!=1 || !FsKbActSectionAtHead(own)) return 3;
    filesystem.paths="d:/GAME/skeeto_kbact/;D:\\game\\addons\\voice.vpk";
    if(!FsKbActSectionAtHead(own)) return 4;
    filesystem.paths="D:\\game\\skeeto_kbact_other;D:\\game\\skeeto_kbact";
    if(FsKbActSectionAtHead(own)) return 5;
    return 0;
}
'''

class KeyboardSection(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source=(ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        cls.temp=tempfile.TemporaryDirectory(prefix='skeeto-kb-'); out=Path(cls.temp.name)
        cpp=out/'test.cpp'; cpp.write_text(HARNESS+block(source,r'^static void KbListSectionTick\(\) \{')+FIXTURES,encoding='utf-8')
        vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
        vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
        cls.exe=out/'test.exe'; bat=out/'compile.bat'
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /Fe"{cls.exe}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n',encoding='utf-8')
        result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
        if result.returncode: raise RuntimeError(result.stdout+result.stderr)
        style=(ROOT/'src/skeeto/core/skeeto_style.cpp').read_text(encoding='utf-8-sig')
        path_cpp=out/'paths.cpp'
        path_cpp.write_text(PATH_HARNESS
            +block(style,r'^static void KbActPathNorm\(')
            +block(style,r'^bool FsKbActSectionAtHead\(')
            +block(style,r'^void FsKbActAddPath\(')+PATH_FIXTURES,encoding='utf-8')
        cls.path_exe=out/'paths.exe'
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /Fe"{cls.path_exe}" /Fo"{out / "paths.obj"}" "{path_cpp}"\nexit /b %errorlevel%\n',encoding='utf-8')
        result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
        if result.returncode: raise RuntimeError(result.stdout+result.stderr)
    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()
    def case(self,n):
        r=subprocess.run([str(self.exe),str(n)],capture_output=True,text=True)
        self.assertEqual(r.returncode,0,r.stdout+r.stderr)
    def test_unregistered_path_is_not_success(self): self.case(0)
    def test_overridden_path_is_not_success(self): self.case(1)
    def test_verified_path_is_success(self): self.case(2)
    def test_late_voice_mod_does_not_trigger_priority_fight(self): self.case(4)
    def test_later_addon_priority_is_restored(self):
        result=subprocess.run([str(self.path_exe)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
    def test_original_mod_bytes_are_preserved(self):
        for directory in ('【极简语言表单】','2865494768'):
            mod=ROOT/'reference/Other Mod'/directory/'scripts/kb_act.lst'
            if not mod.is_file(): continue # local ignored evidence is optional in other checkouts
            with self.subTest(mod=directory):
                # Pass a local ASCII fixture path to the ANSI-only test harness.
                fixture=Path(self.temp.name)/f'{directory == "2865494768"}.lst'
                fixture.write_bytes(mod.read_bytes())
                result=subprocess.run([str(self.exe),'3',str(fixture)],capture_output=True,text=True)
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)

class KeyboardSectionContents(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source=(ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        harness=r'''#include <windows.h>
#include <cstdio>
#include <cstring>
#include "skeeto_ui_text.h"
'''
        fixture=r'''
int main() {
    char out[65536]{};
    KbListSectionBuild(out,(int)sizeof(out));
    char* entry=strstr(out,"\"skeeto_viewmodel_mirror\"");
    if(!entry || !strstr(entry,"全局镜像反转")) return 1;
    g_uiEnglish.store(true);
    KbListSectionBuild(out,(int)sizeof(out));
    entry=strstr(out,"\"skeeto_viewmodel_mirror\"");
    if(!entry || !strstr(entry,"Global viewmodel mirroring")) return 2;
    return 0;
}
'''
        cls.temp=tempfile.TemporaryDirectory(prefix='skeeto-kb-contents-'); out=Path(cls.temp.name)
        cpp=out/'test.cpp'; cpp.write_text(harness+block(source,r'^static void KbListSectionBuild\(char\* out, int cap\) \{')+fixture,encoding='utf-8')
        vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
        vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
        cls.exe=out/'test.exe'; bat=out/'compile.bat'
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /I"{ROOT / "src/skeeto/core"}" /Fe"{cls.exe}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n',encoding='utf-8')
        result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
        if result.returncode: raise RuntimeError(result.stdout+result.stderr)
    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()
    def test_generated_list_exposes_global_mirror_toggle_in_both_languages(self):
        result=subprocess.run([str(self.exe)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)

if __name__=='__main__': unittest.main()
