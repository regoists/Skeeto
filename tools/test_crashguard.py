"""Exercise the real crashguard patch transaction in an x86 Windows harness.

Checks rollback before writes, signed 2 GiB conversion, and setting bounds.
This does not validate game startup timing or GPU/resource behavior.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_chapstats_pages import block

ROOT = Path(__file__).resolve().parents[1]

HARNESS = r'''
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include "skeeto_memread.h"
static unsigned protectCalls = 0, failAt = 0, failAt2 = 0;
static BOOL TestProtect(void* p, SIZE_T n, DWORD protection, DWORD* previous) {
    const unsigned call = ++protectCalls;
    if (call == failAt || call == failAt2) return FALSE;
    return VirtualProtect(p, n, protection, previous);
}
#define VirtualProtect TestProtect
'''

CASES = r'''
static bool check(bool ok, const char* reason) {
    if (!ok) printf("%s\n", reason);
    return ok;
}
int main() {
    bool ok = true;
    auto* memory = (unsigned char*)VirtualAlloc(nullptr, 8192, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    if (!memory) return 2;
    unsigned first=10, second=20;
    memcpy(memory,&first,4); memcpy(memory+4096,&second,4);
    CrashGuardPatch changes[2]{};
    CrashGuardChange(changes[0],memory,10,100,4);
    CrashGuardChange(changes[1],memory+4096,99,200,4);
    ok &= check(!CrashGuardCommit(changes,2).committed,"mismatch must abort");
    ok &= check(*(unsigned*)memory==10 && *(unsigned*)(memory+4096)==20 && protectCalls==0,"mismatch touched memory");
    CrashGuardChange(changes[1],memory+4096,20,200,4);
    failAt=2; protectCalls=0;
    ok &= check(!CrashGuardCommit(changes,2).committed,"protection failure must abort");
    ok &= check(*(unsigned*)memory==10 && *(unsigned*)(memory+4096)==20,"partial write after protection failure");
    MEMORY_BASIC_INFORMATION info{}; VirtualQuery(memory,&info,sizeof(info));
    ok &= check(info.Protect==PAGE_READWRITE,"protection was not restored");
    failAt=3; protectCalls=0;
    auto transientRestore=CrashGuardCommit(changes,2);
    ok &= check(transientRestore.committed && !transientRestore.protectionWarning,"transient protection restore failure was not retried");
    ok &= check(*(unsigned*)memory==100 && *(unsigned*)(memory+4096)==200,"restore failure interrupted committed writes");
    failAt=0;
    DWORD restored=0; VirtualProtect(memory+4096,4096,PAGE_READWRITE,&restored);
    memcpy(memory,&first,4); memcpy(memory+4096,&second,4);
    failAt=3; failAt2=4; protectCalls=0;
    auto persistentRestore=CrashGuardCommit(changes,2);
    ok &= check(persistentRestore.committed && persistentRestore.protectionWarning,"persistent protection restore failure lost committed status or warning");
    ok &= check(*(unsigned*)memory==100 && *(unsigned*)(memory+4096)==200,"persistent restore failure interrupted committed writes");
    VirtualQuery(memory+4096,&info,sizeof(info));
    ok &= check(info.Protect==PAGE_EXECUTE_READWRITE,"fixture did not preserve injected protection failure");
    failAt=failAt2=0; VirtualProtect(memory+4096,4096,PAGE_READWRITE,&restored);
    memcpy(memory,&first,4); memcpy(memory+4096,&second,4);
    failAt=0; protectCalls=0;
    ok &= check(CrashGuardCommit(changes,2).committed,"valid patch failed");
    ok &= check(*(unsigned*)memory==100 && *(unsigned*)(memory+4096)==200,"successful patch did not write all values");
    CrashGuardChange(changes[0],memory,100,100,4); protectCalls=0;
    ok &= check(CrashGuardCommit(changes,1).committed && protectCalls==0,"unchanged bytes must not acquire write permission");
    // The actual x86 signed division sequence found in DataCacheSize_f/SetSize.
    const unsigned char conversion[]={0x8b,0x44,0x24,0x04,0x99,0x81,0xe2,0xff,0xff,0x0f,0x00,0x03,0xc2,0xc1,0xf8,0x14,0xc3};
    memcpy(memory,conversion,sizeof(conversion)); DWORD previous=0;
    TestProtect(memory,4096,PAGE_EXECUTE_READ,&previous);
    auto convert=(unsigned(__cdecl*)(unsigned))memory;
    ok &= check(convert(0x80000000u)!=2048,"fixture must expose signed 2GiB bug");
    CrashGuardChange(changes[0],memory+14,0xf8,0xe8,1);
    ok &= check(CrashGuardCommit(changes,1).committed,"conversion patch failed");
    for(unsigned mb: {64u,512u,1024u,2048u})
        ok &= check(convert(mb<<20)==mb,"megabytes conversion failed");
    ok &= check(convert(0xffffffffu)==0,"native minus-one rounding changed");
    ok &= check(CrashGuardNormalize(65535,false)==65535 && CrashGuardNormalize(131072,false)==131072,"index settings rejected");
    ok &= check(CrashGuardNormalize(2048,true)==2048 && CrashGuardNormalize(512,true)==512,"cache settings rejected");
    ok &= check(CrashGuardNormalize(-1,true)==0 && CrashGuardNormalize(99999,false)==0,"invalid setting enabled a patch");
    VirtualFree(memory,0,MEM_RELEASE);
    return ok ? 0 : 1;
}
'''


class CrashGuardTests(unittest.TestCase):
    def test_production_transaction_and_numeric_boundaries(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        self.assertTrue('struct CrashGuardPatch {' in source, 'crashguard transaction is not implemented')
        pieces = [block(source, r'^struct CrashGuardPatch \{') + ';',
                  block(source, r'^struct CrashGuardCommitResult \{') + ';']
        for name in ['CrashGuardReadable', 'CrashGuardChange', 'CrashGuardCommit', 'CrashGuardNormalize']:
            pieces.append(block(source, rf'^static [^\n]+ {name}\([^\n]*\) \{{'))
        self.compile_run('\n'.join(pieces), CASES[CASES.index('int main()'):])

    def test_native_index_init_default_conflict_late_and_real_capacity(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        start = source.index('struct CrashGuardPatch {')
        end = source.index('static int CrashGuardApplyCache()', start)
        setup = r'''
#include <atomic>
static int g_optCrashGuardCacheMb=0, g_optCrashGuardIndexCount=0;
static std::atomic_bool g_run{true};
static void Log(const char*, ...) {}
static HMODULE fixtureModule=nullptr;
static unsigned moduleReads=0, moduleLoads=0;
static HMODULE FixtureModule(const char*) { ++moduleReads; return fixtureModule; }
static HMODULE FixtureLoad(const char*, HANDLE, DWORD) { ++moduleLoads; return nullptr; }
#define GetModuleHandleA FixtureModule
#define LoadLibraryExA FixtureLoad
static bool CmdLineHasFlag(const char*) { return false; }
static void SkeetoGameRoot(char* root,size_t) { root[0]=0; }
'''
        fixture = r'''
static unsigned originalCalls=0, observedCapacity=0;
static void __fastcall FixtureOriginalInit(void* object,void*) {
    ++originalCalls;
    observedCapacity=*(unsigned*)((unsigned char*)object+0x3C8);
}
static void prepare(unsigned char* base, bool vulkan=false) {
    memset(base,0,0xFF000);
    auto* dos=(IMAGE_DOS_HEADER*)base;dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=64;
    auto* nt=(IMAGE_NT_HEADERS*)(base+64);nt->Signature=IMAGE_NT_SIGNATURE;
    nt->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    nt->FileHeader.TimeDateStamp=vulkan?0x6A440DF3:0x6A440E0A;
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;nt->OptionalHeader.SizeOfImage=0xFF000;
    const int d=vulkan?-32:0;
    const unsigned char ctor[]={0x53,0x56,0x8B,0xF1,0x57,0x8D,0x4E,0x04,0xC7,0x06};
    const unsigned char call[]={0x0F,0xB6,0xC0,0x50,0x68};
    const unsigned char prologue[]={0x56,0x8B,0xF1,0x57,0x6A,0x24};
    const unsigned char count[]={0xC7,0x86,0xC8,0x03,0,0};
    const unsigned char bytes[]={0xC7,0x86,0xCC,0x03,0,0};
    memcpy(base+0xD9F0+d,ctor,sizeof(ctor));memcpy(base+0xCCD1+d,call,sizeof(call));
    memcpy(base+0xCC90+d,prologue,sizeof(prologue));memcpy(base+0xDB24+d,count,sizeof(count));
    memcpy(base+0xDB1A+d,bytes,sizeof(bytes));
    *(uintptr_t*)(base+0xD9FA+d)=(uintptr_t)(base+0x752DC);
    *(uintptr_t*)(base+0xCE488)=(uintptr_t)(base+0x752DC);
    base[0x8D70+d]=0xB8;base[0x8D75+d]=0xC3;
    *(unsigned*)(base+0x8D71+d)=32768;*(unsigned*)(base+0xCCD6+d)=32768;
    *(unsigned*)(base+0xDB2A+d)=32768;*(unsigned*)(base+0xDB20+d)=65536;
    *(unsigned*)(base+0xCE488+0x3C8)=32768;*(unsigned*)(base+0xCE488+0x3CC)=65536;
    fixtureModule=(HMODULE)base;
    g_crashGuardIndexState=kCrashGuardNoChange;g_crashGuardCacheState=kCrashGuardNoChange;
    g_crashGuardIndexEntryOwned=false;originalCalls=0;observedCapacity=0;
}
int main() {
    auto* base=(unsigned char*)VirtualAlloc(nullptr,0xFF000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!base)return 2;
    bool ok=true;
    prepare(base);CrashGuardBootstrap();
    ok &= check(moduleReads==0 && moduleLoads==0 && !g_crashGuardIndexEntryOwned,"No change touched shader loading/Hook");
    for(int preset: {32768,65535,131072}) {
        prepare(base);g_optCrashGuardIndexCount=preset;CrashGuardBootstrap();
        ok &= check(g_crashGuardIndexState==kCrashGuardArmed,"native Init Hook not armed");
        g_crashGuardIndexInit=(CrashGuardIndexInitFn)&FixtureOriginalInit;
        ((CrashGuardIndexInitFn)(base+0xCC90))(base+0xCE488);
        unsigned expected=preset==32768?32768u:preset==65535?65536u:131072u;
        ok &= check(g_crashGuardIndexState==kCrashGuardApplied && originalCalls==1,"patch not applied before original Init");
        ok &= check(observedCapacity==expected && *(unsigned*)(base+0xCCD6)==expected && *(unsigned*)(base+0x8D71)==expected,"requested value substituted for actual capacity");
        ok &= check(*(unsigned*)(base+0xCE488+0x3CC)==expected*2u,"index bytes and count diverged");
        ok &= check(!g_crashGuardIndexEntryOwned && base[0xCC90]==0x56,"one-shot entry was not restored");
    }
    prepare(base);g_optCrashGuardIndexCount=131072;*(void**)(base+0xCE488+0x1EC)=(void*)1;
    CrashGuardBootstrap();
    ok &= check(g_crashGuardIndexState==kCrashGuardLate && base[0xCC90]==0x56,"late injection modified a live buffer");
    prepare(base);*(unsigned*)(base+0xCCD6)=123;CrashGuardBootstrap();
    g_crashGuardIndexInit=(CrashGuardIndexInitFn)&FixtureOriginalInit;
    ((CrashGuardIndexInitFn)(base+0xCC90))(base+0xCE488);
    ok &= check(g_crashGuardIndexState==kCrashGuardFailed && originalCalls==1,"conflict did not preserve original initialization");
    ok &= check(*(unsigned*)(base+0x8D71)==32768 && *(unsigned*)(base+0xCE488+0x3C8)==32768 && *(unsigned*)(base+0xCCD6)==123,"conflict caused a partial patch");
    prepare(base);base[0xCC90]=0xE9;CrashGuardBootstrap();
    ok &= check(g_crashGuardIndexState==kCrashGuardConflict && base[0xCC90]==0xE9,"overwrote another platform Hook");
    prepare(base,true);
    ok &= check(CrashGuardShaderDelta((HMODULE)base)==-32 && CrashGuardIndexLayout((HMODULE)base,-32),"Vulkan offsets rejected");
    auto* nt=(IMAGE_NT_HEADERS*)(base+64);nt->FileHeader.TimeDateStamp=0;
    ok &= check(CrashGuardShaderDelta((HMODULE)base)==1,"unknown binary accepted");
    VirtualFree(base,0,MEM_RELEASE);
    return ok?0:1;
}
'''
        self.compile_run(source[start:end], fixture, setup)

    def compile_run(self, production, cases, setup=''):
        with tempfile.TemporaryDirectory(prefix='skeeto-crashguard-') as temporary:
            out = Path(temporary)
            cpp = out / 'test.cpp'
            cpp.write_text('#include <initializer_list>\n'+HARNESS+'\n'+setup+production+CASES.split('int main()',1)[0]+cases, encoding='utf-8')
            vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
            executable = out / 'test.exe'
            bat = out / 'compile.bat'
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                f'cl /nologo /EHsc /std:c++17 /utf-8 /W4 /I"{ROOT / "src/skeeto/core"}" /Fe"{executable}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n', encoding='utf-8')
            result = subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result = subprocess.run([str(executable)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)


if __name__ == '__main__':
    unittest.main()
