"""Run production failure boundaries in x86 fixtures and the built core.

Only temporary DLLs/directories are used. No game process or player settings.
The startup cases need runtime/dll/skeeto.dll from the current normal build.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

from test_chapstats_pages import block

ROOT = Path(__file__).resolve().parents[1]

WRITES = r'''
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
static unsigned faults=0, requests=0, failRequest=0;
namespace skmem { static void NoteFault(uintptr_t){++faults;} }
static bool PtrCommitted(const void* p){return p!=nullptr;}
static void* CvarParent(void* p){return p;}
static int CvarPickValueLayout(void*){return 0;}
static BOOL Protect(void* p,SIZE_T n,DWORD protection,DWORD* old){
 if(protection==PAGE_READWRITE && ++requests==failRequest)return FALSE;
 return VirtualProtect(p,n,protection,old);
}
#include <intrin.h>
#define VirtualProtect Protect
PRODUCTION
#undef VirtualProtect
static bool faulted=false;
static bool Invoke(unsigned char* p,const char* mode){
 __try {
  if(!strcmp(mode,"single"))return ListenPokeFloat((float*)(p+0x2c),2.f);
  return !strcmp(mode,"float") || !strcmp(mode,"integer")
   ? CvarPokeFloatValue(p,!strcmp(mode,"float")?.066f:4.f)
   : CvarPokePair(p,777,2.f);
 }__except(EXCEPTION_EXECUTE_HANDLER){faulted=true;return false;}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 auto p=(unsigned char*)VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!p)return 3;
 char text[16]="0.5";*(char**)(p+0x24)=text;*(int*)(p+0x28)=16;
 *(int*)(p+0x30)=91;*(float*)(p+0x2c)=.5f;
 const bool normal=!strcmp(argv[1],"normal");DWORD old=0;
 VirtualProtect(p,4096,PAGE_READONLY,&old);
 failRequest=normal?0:(!strcmp(argv[1],"pair-second") || !strcmp(argv[1],"integer"))?2:1;
 const char* mode=normal?"pair":argv[1];bool result=Invoke(p,mode);
 bool ok=!faulted;
 if(normal){
  ok=ok && result && *(int*)(p+0x30)==777 && *(float*)(p+0x2c)==2.f;
  ok=ok && Invoke(p,"float") && *(int*)(p+0x30)==777 && !strcmp(text,"0.066");
  ok=ok && Invoke(p,"integer") && *(int*)(p+0x30)==4 && !strcmp(text,"4") && !faults;
 }else{
  ok=ok && !result && faults==1;
  if(!strcmp(mode,"pair-second"))ok=ok && *(int*)(p+0x30)==777 && *(float*)(p+0x2c)==.5f;
  else if(!strcmp(mode,"integer"))ok=ok && *(int*)(p+0x30)==91 && *(float*)(p+0x2c)==4.f && !strcmp(text,"0.5");
  else ok=ok && *(int*)(p+0x30)==91 && *(float*)(p+0x2c)==.5f;
 }
 MEMORY_BASIC_INFORMATION info{};
 ok=ok && VirtualQuery(p,&info,sizeof(info)) && info.Protect==PAGE_READONLY;
 printf("%s writes %s: fault=%d result=%d recorded=%u\n",ok?"PASS":"FAIL",argv[1],faulted,result,faults);
 VirtualFree(p,0,MEM_RELEASE);return ok?0:1;
}
'''

NATIVE = r'''
#include <windows.h>
#include <cstring>
static int mode=0;static LONG calls=0;
static int __fastcall NativeZero(void*,void*){InterlockedIncrement(&calls);return 0;}
static void* methods[160];static void** object=methods;
extern "C" __declspec(dllexport) void SetMode(int m){mode=m;for(auto& f:methods)f=(void*)&NativeZero;}
extern "C" __declspec(dllexport) LONG Calls(){return calls;}
extern "C" __declspec(dllexport) void* CreateInterface(const char* name,int*){
 if(!strcmp(name,"VEngineClient013"))return mode==1 || mode==3?nullptr:&object;
 if(!strcmp(name,"VClientEntityList003"))return mode==2 || mode==3?nullptr:&object;
 return nullptr; // Optional interfaces deliberately absent; not grounds for rejecting the core.
}
'''

STARTUP = r'''
#include "skeeto_bootstrap_abi.h"
#include <cstdio>
#include <cstring>
static bool Cleaned(){
 FILE* f=fopen("skeeto.log","rb");if(!f)return false;
 char text[16384]{};fread(text,1,sizeof(text)-1,f);fclose(f);
 return strstr(text,"worker exit (clean shutdown)")!=nullptr;
}
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;
 HMODULE engine=LoadLibraryW(L"engine.dll"),client=LoadLibraryW(L"client.dll");
 if(!engine || !client || !LoadLibraryW(L"serverbrowser.dll"))return 3;
 const int mode=_wtoi(argv[2]);
 using Set=void(__cdecl*)(int);using Count=LONG(__cdecl*)();
 ((Set)GetProcAddress(engine,"SetMode"))(mode);((Set)GetProcAddress(client,"SetMode"))(mode);
 HMODULE core=LoadLibraryW(argv[1]);if(!core)return 4;
 auto query=(SktBootstrapQueryFn)GetProcAddress(core,"SkeetoBootstrapQuery");if(!query)return 5;
 const auto id=query();DWORD start=GetTickCount();LONG state=0;
 do{state=InterlockedCompareExchange(&id->state,0,0);
  if(state==SKT_BOOT_READY || state==SKT_BOOT_FAILED)break;Sleep(1);
 }while(GetTickCount()-start<10000u);
 bool cleaned=false;
 if(mode && state==SKT_BOOT_FAILED){
  do{cleaned=Cleaned();if(cleaned)break;Sleep(1);}while(GetTickCount()-start<10000u);
 }
 const LONG calls=((Count)GetProcAddress(engine,"Calls"))()+((Count)GetProcAddress(client,"Calls"))();
 const bool ok=mode?state==SKT_BOOT_FAILED && calls==0 && cleaned:state==SKT_BOOT_READY;
 printf("%s startup mode=%d state=%ld native_calls=%ld cleanup_tail=%d\n",ok?"PASS":"FAIL",mode,state,calls,cleaned);
 return ok?0:1;
}
'''


def main():
    source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    production = "\n".join(block(source, rf"^static [^\n]+ {name}\([^\n]*\) \{{")
                           for name in ("CvarPokePair", "ListenPokeFloat", "CvarPokeFloatValue"))
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = Path(subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
              "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip())
    with tempfile.TemporaryDirectory(prefix="skeeto-core-failures-") as temporary:
        folder = Path(temporary)
        (folder / "writes.cpp").write_text(WRITES.replace("PRODUCTION", production), encoding="utf-8")
        (folder / "native.cpp").write_text(NATIVE, encoding="utf-8")
        (folder / "startup.cpp").write_text(STARTUP, encoding="utf-8")
        commands = [f'@echo off\ncall "{vs / "VC/Auxiliary/Build/vcvarsall.bat"}" x86 >nul']
        for output, cpp, shared in (("writes.exe", "writes.cpp", ""), ("native.dll", "native.cpp", "/LD"), ("startup.exe", "startup.cpp", "")):
            commands.extend([f'cl /nologo /MT /O2 /EHsc /std:c++17 /utf-8 {shared} /I "{ROOT / "src/skeeto/core"}" {cpp} /Fe:{output}', "if errorlevel 1 exit /b 1"])
        (folder / "compile.bat").write_text("\n".join(commands) + "\n", encoding="utf-8")
        subprocess.run(["cmd", "/d", "/c", str(folder / "compile.bat")], cwd=folder, check=True)
        failures = []
        for case in ("normal", "pair-first", "pair-second", "single", "float", "integer"):
            result = subprocess.run([str(folder / "writes.exe"), case], cwd=folder, capture_output=True, text=True, timeout=15)
            print(result.stdout.strip())
            if result.returncode: failures.append((case, result.returncode, result.stderr))
        for case, mode in (("available", 0), ("missing-engine", 1), ("missing-entities", 2), ("missing-both", 3)):
            root = folder / case
            root.mkdir()
            (root / "left4dead2").mkdir()
            (root / "skeeto_setting.ini").write_text("necola_autoload=0\n", encoding="utf-8")
            shutil.copyfile(folder / "startup.exe", root / "startup.exe")
            shutil.copyfile(ROOT / "runtime/dll/skeeto.dll", root / "skeeto.dll")
            for name in ("engine.dll", "client.dll", "serverbrowser.dll"):
                shutil.copyfile(folder / "native.dll", root / name)
            result = subprocess.run([str(root / "startup.exe"), str(root / "skeeto.dll"), str(mode), "-skeeto_debug"], cwd=root, capture_output=True, text=True, timeout=15)
            print(result.stdout.strip())
            if result.returncode: failures.append((case, result.returncode, result.stderr))
        assert not failures, failures
    print("core failure paths: writes and built-core startup passed")


if __name__ == "__main__":
    main()
