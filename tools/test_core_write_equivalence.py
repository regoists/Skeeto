"""Differential x86 check of the narrow protection-failure repair.

Compare the three pre-repair helpers at the pinned Git baseline with production
on successful page-protection paths. Use the real parent/layout/read helpers;
this checks preserved behavior, not the validity of a future game's ConVar ABI.
Failure injection and built-core startup are in test_core_failure_paths.py.
All compiled fixtures and test objects live in temporary storage.
"""
from pathlib import Path
import os
import subprocess
import tempfile

from test_chapstats_pages import block

ROOT = Path(__file__).resolve().parents[1]
BASELINE = "69ccf3684789f0f39a4fa782b6b9fa459c7a73d3"
SOURCE = "src/skeeto/core/skeeto_proxy.cpp"
NAMES = ("CvarPokePair", "ListenPokeFloat", "CvarPokeFloatValue")

FIXTURE = r'''
#include "skeeto_memread.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
COMMON
struct Call { unsigned offset, size; DWORD requested, previous; };
static unsigned char* page;
static Call calls[8]; static unsigned callCount;
static BOOL Protect(void* p, SIZE_T n, DWORD protection, DWORD* old) {
 const BOOL ok=VirtualProtect(p,n,protection,old);
 if(!ok || callCount==8)abort();
 calls[callCount++]={unsigned((unsigned char*)p-page),unsigned(n),protection,*old};
 return ok;
}
#define VirtualProtect Protect
namespace before { BEFORE }
namespace after { AFTER }
#undef VirtualProtect
struct Result {
 unsigned char bytes[4096]; char text[80];
 Call calls[8]; unsigned count; DWORD protection;
 bool ok; unsigned faults, queries;
};
static char text[80];
static Result Run(bool repaired,int method,float value,int parentKind,int capacity,DWORD protection,int layout) {
 DWORD old=0; VirtualProtect(page,4096,PAGE_READWRITE,&old);
 memset(page,0xa5,4096); memset(page,0,512);
 auto* parent=parentKind==2?page+256:page;
 if(parentKind)*(void**)(page+0x1c)=parent;
 memset(text,'Z',sizeof(text)); memcpy(text,"0.5",4);
 *(char**)(parent+0x24)=capacity<0?nullptr:text;
 *(int*)(parent+0x28)=capacity;
 *(int*)(parent+0x30)=91; *(float*)(parent+0x2c)=.5f;
 if(layout) { *(unsigned*)(parent+0x2c)=0x7fc00000u; *(float*)(parent+0x28)=.5f; }
 VirtualProtect(page,4096,protection,&old);
 memset(calls,0,sizeof(calls)); callCount=0;
 const unsigned faults=skmem::g_faults.load(), queries=skmem::g_queries.load();
 Result result{};
 if(method==0) result.ok=repaired?after::CvarPokePair(page,-777,value):before::CvarPokePair(page,-777,value);
 else if(method==1) result.ok=repaired?after::CvarPokeFloatValue(page,value):before::CvarPokeFloatValue(page,value);
 else if(repaired) result.ok=after::ListenPokeFloat((float*)(parent+0x2c),value);
 else { before::ListenPokeFloat((float*)(parent+0x2c),value); result.ok=true; }
 result.faults=skmem::g_faults.load()-faults; result.queries=skmem::g_queries.load()-queries;
 memcpy(result.bytes,page,4096); memcpy(result.text,text,sizeof(text));
 memcpy(result.calls,calls,sizeof(calls)); result.count=callCount;
 MEMORY_BASIC_INFORMATION info{}; if(!VirtualQuery(page,&info,sizeof(info)))abort();
 result.protection=info.Protect; return result;
}
int main() {
 page=(unsigned char*)VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
 if(!page)return 2;
 const float values[]={-100000.f,-1.f,-.066f,-0.f,0.f,1.f/128.f,.01f,.066f,.25f,1.f,4.f,100000.f};
 const int capacities[]={-1,0,3,4,16,64,65};
 unsigned cases=0, protections=0;
 for(DWORD protection:{PAGE_READONLY,PAGE_READWRITE})
 for(int parent=0;parent!=3;++parent)
 for(int capacity:capacities)
 for(float value:values)
 for(int method=0;method!=3;++method)
 for(int layout=0;layout!=(method==0?2:1);++layout) {
  const Result a=Run(false,method,value,parent,capacity,protection,layout);
  const Result b=Run(true,method,value,parent,capacity,protection,layout);
  const bool equal=a.ok==b.ok && b.ok && a.count==b.count
   && a.protection==b.protection && b.protection==protection
   && !a.faults && !b.faults && !a.queries && !b.queries
   && !memcmp(a.bytes,b.bytes,sizeof(a.bytes)) && !memcmp(a.text,b.text,sizeof(a.text))
   && !memcmp(a.calls,b.calls,sizeof(a.calls));
  if(!equal) { printf("FAIL method=%d value=%g parent=%d capacity=%d protection=%lu layout=%d\n",method,value,parent,capacity,protection,layout); return 1; }
  ++cases; protections+=b.count;
 }
 VirtualFree(page,0,MEM_RELEASE);
 printf("PASS %u differential cases, %u matching VirtualProtect calls per version; zero helper page queries/faults\n",cases,protections);
 return 0;
}
'''


def function(source, name):
    return block(source, rf"^(?:__declspec\(noinline\) )?(?:static )?[^\n]+ {name}\([^\n]*\) \{{")


def main():
    current = (ROOT / SOURCE).read_text(encoding="utf-8-sig")
    baseline = subprocess.check_output(["git", "show", f"{BASELINE}:{SOURCE}"], cwd=ROOT).decode("utf-8-sig")
    common = "\n".join(function(current, n) for n in ("PtrCommitted", "CvarParent", "CvarPickValueLayout"))
    # Differences here would need their own equivalence evidence.
    assert common == "\n".join(function(baseline, n) for n in ("PtrCommitted", "CvarParent", "CvarPickValueLayout"))
    fixture = FIXTURE.replace("COMMON", common).replace("BEFORE", "\n".join(function(baseline, n) for n in NAMES))
    fixture = fixture.replace("AFTER", "\n".join(function(current, n) for n in NAMES))
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = Path(subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
              "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip())
    with tempfile.TemporaryDirectory(prefix="skeeto-write-equivalence-") as temporary:
        folder = Path(temporary)
        (folder / "equivalence.cpp").write_text(fixture, encoding="utf-8")
        (folder / "compile.bat").write_text(
            f'@echo off\ncall "{vs / "VC/Auxiliary/Build/vcvarsall.bat"}" x86 >nul\n'
            f'cl /nologo /MT /O2 /EHsc /std:c++17 /utf-8 /I "{ROOT / "src/skeeto/core"}" equivalence.cpp /Fe:equivalence.exe\n', encoding="utf-8")
        subprocess.run(["cmd", "/d", "/c", str(folder / "compile.bat")], cwd=folder, check=True)
        subprocess.run([str(folder / "equivalence.exe")], cwd=folder, check=True, timeout=30)
    print(f"baseline {BASELINE}: normal write behavior equivalent within the fixture contract")


if __name__ == "__main__":
    main()
