"""Exercise the real GameUI addon snapshot reader against an x86 engine fixture."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, marker):
    start = source.index(marker)
    at = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[at] == "{") - (source[at] == "}")
        at += 1
    return source[start:at] + "\n"


def main():
    core = ROOT / "src/skeeto/core"
    native = (core / "skeeto_mainui_native.h").read_text(encoding="utf-8-sig")
    proxy = (core / "skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    prefix = r'''
#include <Windows.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
static bool g_optMainMenuUi=true,g_mainUiDraw=true,g_mainUiOwned=true;
static bool g_mainUiEditing=false,g_mainUiConnected=false,g_mainUiInGame=false,g_mainUiLoading=false;
static int g_mainUiAddonsState=-1;
static DWORD g_mainUiAddonsReadAt=0;
static unsigned char* fixtureModule=nullptr;
static unsigned char* fixtureObject=nullptr;
static DWORD fixtureNow=1000;
static int patternHits=1,scans=0;
static HMODULE FixtureModule(const char*){return (HMODULE)fixtureModule;}
static DWORD FixtureNow(){return fixtureNow;}
static bool PtrCommitted(const void* p){MEMORY_BASIC_INFORMATION m{};return p&&VirtualQuery(p,&m,sizeof m)&&m.State==MEM_COMMIT&&!(m.Protect&(PAGE_NOACCESS|PAGE_GUARD));}
static bool IsExec(const void* p){MEMORY_BASIC_INFORMATION m{};return p&&VirtualQuery(p,&m,sizeof m)&&(m.Protect&PAGE_EXECUTE_READWRITE);}
static bool IfaceAlive(void* p){return PtrCommitted(p)&&PtrCommitted(*(void**)p)&&IsExec(**(void***)p);}
static bool BackgroundPtrInModule(const void* p,HMODULE m){return (uintptr_t)p>=(uintptr_t)m&&(uintptr_t)p<(uintptr_t)m+0x1000;}
static int CountPat(const char*,const char*,uintptr_t* out,int){++scans;out[0]=(uintptr_t)fixtureModule+0x200;return patternHits;}
static void Log(const char*,...){}
#define GetModuleHandleA FixtureModule
#define GetTickCount FixtureNow
'''
    body = function(proxy, "static int NativeAddonsLoadState()")
    body += function(proxy, "static void MainUiAddonsStop()")
    body += function(native, "static int MainUiAddonsRead()")
    suffix = r'''
static void MakeModule(){
 fixtureModule=(unsigned char*)VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);assert(fixtureModule);
 auto dos=(PIMAGE_DOS_HEADER)fixtureModule;dos->e_lfanew=0x80;
 auto nt=(PIMAGE_NT_HEADERS)(fixtureModule+0x80);nt->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;nt->OptionalHeader.SizeOfImage=0x1000;
 fixtureModule[0x200]=0xe8;int rel=0x300-0x205;memcpy(fixtureModule+0x201,&rel,4);
 fixtureModule[0x300]=0xa1;auto slot=(uintptr_t)fixtureModule+0x400;memcpy(fixtureModule+0x301,&slot,4);fixtureModule[0x305]=0xc3;
 *(void**)(fixtureModule+0x400)=fixtureObject;*(void**)fixtureObject=fixtureModule+0x500;*(void**)(fixtureModule+0x500)=fixtureModule+0x300;
 MainUiAddonsStop();fixtureNow+=200;
}
int main(){
 static_assert(sizeof(void*)==4,"x86 fixture required");
 fixtureObject=(unsigned char*)VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(fixtureObject);MakeModule();
 // Completion is engine-owned. Paint polls once per 100 ms, never rescans code.
 assert(MainUiAddonsRead()==1&&scans==1);
 fixtureObject[0x2d0]=1;fixtureNow+=99;assert(MainUiAddonsRead()==1&&scans==1);
 ++fixtureNow;assert(MainUiAddonsRead()==0&&scans==1);
 fixtureObject[0x2d0]=0;fixtureNow+=100;assert(MainUiAddonsRead()==1);
 // An unavailable/replaced singleton or corrupt flag cannot report completion.
 *(void**)(fixtureModule+0x400)=nullptr;fixtureNow+=100;assert(MainUiAddonsRead()==-1);
 *(void**)(fixtureModule+0x400)=fixtureObject;fixtureObject[0x2d0]=2;fixtureNow+=100;assert(MainUiAddonsRead()==-1);
 fixtureObject[0x2d0]=1;fixtureNow+=100;assert(MainUiAddonsRead()==0);
 // Feedback recovery reads the same native flag without owning/showing MainUI.
 g_optMainMenuUi=false;g_mainUiInGame=true;assert(NativeAddonsLoadState()==0);
 fixtureObject[0x2d0]=0;assert(NativeAddonsLoadState()==1);g_optMainMenuUi=true;g_mainUiInGame=false;
 int boundScans=scans;for(int i=0;i<100000;++i)assert(NativeAddonsLoadState()==1);
 assert(scans==boundScans&&skmem::g_queries.load()==0);
 for(int gate=0;gate<6;++gate){
  bool* flag=gate==0?&g_optMainMenuUi:gate==1?&g_mainUiOwned:gate==2?&g_mainUiConnected:gate==3?&g_mainUiInGame:gate==4?&g_mainUiLoading:&g_mainUiEditing;
  bool before=*flag;*flag=!before;int count=scans;assert(MainUiAddonsRead()==-1&&g_mainUiAddonsReadAt==0&&scans==count);*flag=before;
 }
 fixtureObject[0x2d0]=0;assert(MainUiAddonsRead()==1);
 // Ambiguous/missing signatures and unsupported getters stay unknown.
 for(int matches:{0,2}){MakeModule();patternHits=matches;assert(MainUiAddonsRead()==-1);}
 patternHits=1;MakeModule();fixtureModule[0x305]=0x90;assert(MainUiAddonsRead()==-1);
 MakeModule();((PIMAGE_NT_HEADERS)(fixtureModule+0x80))->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;assert(MainUiAddonsRead()==-1);
 // A disappearing page is handled locally, without faulting the whole menu.
 MakeModule();DWORD old=0;assert(VirtualProtect(fixtureObject,4096,PAGE_NOACCESS,&old));assert(MainUiAddonsRead()==-1);assert(VirtualProtect(fixtureObject,4096,old,&old));
 // Deadline comparisons survive GetTickCount rollover.
 MakeModule();fixtureObject[0x2d0]=0;fixtureNow=0xffffffe0u;assert(MainUiAddonsRead()==1);
 fixtureObject[0x2d0]=1;fixtureNow=20;assert(MainUiAddonsRead()==1);fixtureNow=68;assert(MainUiAddonsRead()==0);
 puts("MainUI addon state: engine completion, polling, isolation, compatibility failures and rollover passed.");
}
'''
    # Initializer lists in the fixture need the standard header explicitly.
    source = '#include "' + (core / "skeeto_memread.h").as_posix() + '"\n#include <initializer_list>\n' + prefix + body + suffix
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                  "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-mainui-addon-") as folder:
        work = Path(folder)
        (work / "fixture.cpp").write_text(source, encoding="utf-8")
        (work / "run.bat").write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvars32.bat" >nul\n'
            'if errorlevel 1 exit /b 1\n'
            'cl /nologo /EHsc /std:c++17 /W3 /WX fixture.cpp /Fe:fixture.exe\n'
            'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "run.bat"], cwd=work, check=True)


if __name__ == "__main__":
    main()
