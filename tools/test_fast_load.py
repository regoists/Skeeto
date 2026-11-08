"""Run the production loading redraw controller in an MSVC x86 fixture.

Verifies passthrough, bounded path reuse, load gating and patch ownership.
Engine/GPU/network boundaries are not reproduced by this offline fixture.
"""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "src/skeeto/core/skeeto_fast_load.h"

FIXTURE = r'''
#define NOMINMAX
#include "skeeto_memread.h"
#include <psapi.h>
static unsigned lengthScans = 0;
static size_t CountLengthScan(const char* p, size_t n) {
    ++lengthScans; return strnlen_s(p,n);
}
#define strnlen_s CountLengthScan
#include "skeeto_fast_load.h"
#undef strnlen_s
#include <cstdio>
#include <cstdlib>
static unsigned draws = 0;
static bool recurse = false;
static unsigned splits = 0;
static void require(bool ok,const char* why);
static unsigned verifications = 0;
static void* __fastcall VerifyCurrent(void*,void*,const char* dir,const char* file,const char* ext,void*,void*) {
    ++verifications;
    require(!std::strcmp(dir,"dir:missing") && !std::strcmp(file,"file:missing") && !std::strcmp(ext,"ext:missing"),"VPK-only verification uses this query rather than stale reuse strings");
    return nullptr;
}
static unsigned finished = 0;
static int asyncInts[3]{};
static float asyncFloats[3]{};
static unsigned asyncReads=0,asyncWrites=0,asyncFinishes=0;
static bool asyncWriteOk=true;
static bool checkAsyncBeforeFinished=false;
static unsigned AsyncIndex(const char* name) {
    if(!std::strcmp(name,"mod_load_mesh_async"))return 0;
    if(!std::strcmp(name,"mod_load_vcollide_async"))return 1;
    return 2;
}
static bool AsyncRead(const char* name,int* n,float* f) {
    ++asyncReads;unsigned i=AsyncIndex(name);*n=asyncInts[i];*f=asyncFloats[i];return true;
}
static bool AsyncWrite(const char* name,int n,float f) {
    ++asyncWrites;if(!asyncWriteOk)return false;
    unsigned i=AsyncIndex(name);asyncInts[i]=n;asyncFloats[i]=f;return true;
}
static void __fastcall AsyncFinish(void*,void*) {
    if(skload::g_nativeAsyncWant.load()&1u)
        require(asyncInts[0]==1,"native pending drain precedes switch restoration");
    ++asyncFinishes;
}
static unsigned reads = 0;
static bool readOk = true;
static void* inlineInnerCode = nullptr;
static void** inlineReturnSlot = nullptr;
// Simulate the admitted inline caller: ESI=filesystem, EBX=pack index. It must
// retain its own caller's EBX/ESI after observing the cursor returned by inner.
__declspec(naked) static void __cdecl InlineFixture(void*,void*,unsigned*,const char*,unsigned*) {
    __asm {
        push ebp
        mov ebp,esp
        push ebx
        push esi
        lea eax,afterInner
        mov edx,inlineReturnSlot
        mov dword ptr [edx],eax
        mov esi,dword ptr [ebp + 8]
        mov ecx,dword ptr [ebp + 12]
        xor ebx,ebx
        push dword ptr [ebp + 20]
        push dword ptr [ebp + 16]
        call dword ptr [inlineInnerCode]
    afterInner:
        mov edx,dword ptr [ebp + 24]
        mov dword ptr [edx],ebx
        pop esi
        pop ebx
        mov esp,ebp
        pop ebp
        ret
    }
}
static BOOL WINAPI NativeRead(HANDLE,void*,DWORD size,DWORD* done,OVERLAPPED*) {
    ++reads; if(done)*done=readOk?size:0; SetLastError(123); return readOk?TRUE:FALSE;
}
static void __fastcall NativeFinished(void*,void*) {
    if(checkAsyncBeforeFinished)
        require(!skload::g_nativePreload.active && asyncInts[0]==0,"switches restored before native loading-finished");
    ++finished;
}
static void __cdecl NativeSplit(const char* path, char* dir, char* file, char* ext) {
    ++splits;
    std::snprintf(dir,260,"dir:%s",path);
    std::snprintf(file,260,"file:%s",path);
    std::snprintf(ext,260,"ext:%s",path);
}
static void __cdecl NativeDraw() {
    ++draws;
    if (recurse) { recurse=false; skload::UpdateAt(101); }
}
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr,"FAIL %s\n",why); std::exit(1); }
}
static bool GuardNext(void* fs,void* store,unsigned& next) {
    __try { next=skload::NextCandidate(fs,0,store,skload::CandidateHash("dir:one","ext:one"),"dir:one","file:one","ext:one");return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static void reset(unsigned mode) {
    skload::g_original = NativeDraw;
    skload::g_mode.store(mode);
    skload::ClearStats(); draws=0;
}
int main() {
    for(unsigned mask=0;mask<128;++mask)for(unsigned measure=0;measure<2;++measure) {
        const unsigned mode=skload::ConfiguredMode(mask&1,mask&2,mask&4,mask&8,mask&16,mask&32,mask&64,measure);
        const unsigned bits=((mask&2)?8u:0u)|((mask&4)?16u:0u)|((mask&8)?32u:0u)|((mask&16)?4u:0u)|((mask&32)?64u:0u)|((mask&64)?128u:0u);
        const unsigned expected=!(mask&1)?0u:(bits?(bits|1u):0u)|(measure?2u:0u);
        require(mode==expected,"master and every individual mechanism compose independently");
    }
    require(skload::ConfiguredMode(true,true,true,true,true,false,false,false)==61,
        "all mechanisms retain the previously validated async combination");
    require(skload::ConfiguredMode(true,true,true,false,true,false,false,false)==29,
        "the original path/VPK/background combination is unchanged");
    reset(0); skload::UpdateAt(0); skload::UpdateAt(1);
    require(draws==2 && !skload::g_stats.calls,"off is immediate native passthrough, without timing or counters");
    reset(1);
    for (DWORD t=0;t<1000;++t) skload::UpdateAt(t);
    require(draws==1000 && skload::g_stats.calls==1000,"native redraw frequency is retained");
    reset(1); skload::UpdateAt(0); skload::UpdateAt(49); skload::UpdateAt(50);
    require(draws==3,"rapid progress changes retain all native redraws");
    reset(1); skload::UpdateAt(0x80000000u); skload::UpdateAt(0x80000031u); skload::UpdateAt(0x80000032u);
    require(draws==3,"high uptime is supported");
    reset(1); skload::UpdateAt(0xfffffff0u); skload::UpdateAt(0x21u); skload::UpdateAt(0x22u);
    require(draws==3,"tick wrap does not drop progress redraws");
    reset(2); skload::UpdateAt(100); skload::UpdateAt(101);
    require(draws==2 && skload::g_stats.drawn==2,"diagnostics alone does not alter engine behavior");
    reset(3); recurse=true; skload::UpdateAt(100);
    require(draws==1 && skload::g_stats.reentries==1,"nested redraw is bounded, not recursive");
    reset(1); skload::UpdateAt(100); skload::ClearStats(); skload::UpdateAt(101);
    require(draws==2 && skload::g_stats.calls==1,"reset and a new load window do not suppress its first redraw");
    auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    require(code!=nullptr,"fixture page allocated");
    const unsigned char waitPattern[]={0x8b,0x40,0x30,0x85,0xc0,0x7e,0x1d,0x8b,0x0d,1,2,3,4,0x83,0xb9,0x1c,1,0,0,0,0x75,0x0e,0x8b,0x0d,5,6,7,8,0x8b,0x11,0x50,0x8b,0x42,0x7c,0xff,0xd0};
    std::memcpy(code,waitPattern,sizeof(waitPattern));
    require(skload::FindWaitSite(code,4096)==code+5,"exact background wait branch found");
    require(!skload::FindWaitSite(code,sizeof(waitPattern)-1),"truncated background branch rejected");
    skload::g_waitSite=code+5;skload::g_waitStatus=skload::Status::Ready;
    reset(5);skload::SetLoading(true);
    require(code[5]==0xeb && skload::g_waitSuppressed,"load skips only the background sleep branch");
    skload::SetLoading(false);
    require(code[5]==0x7e && !skload::g_waitSuppressed,"playing or cancellation restores native wait");
    reset(2);skload::SetLoading(true);
    require(code[5]==0x7e,"diagnostics do not suppress background sleep");
    skload::SetLoading(false);
    reset(5);skload::SetLoading(true);code[5]=0x90;skload::SetLoading(false);
    require(code[5]==0x90 && skload::g_waitStatus==skload::Status::Conflict,"foreign wait branch retained");
    code[5]=0xeb;skload::SuppressWait(false);skload::g_waitSite=nullptr;
    skload::g_splitOriginal=NativeSplit;skload::g_ownerThread=GetCurrentThreadId();
    char d[260],f[260],e[260];
    reset(9);skload::SetLoading(true);
    skload::SplitPath("one",d,f,e);
    const unsigned scansBeforeHit=lengthScans;
    skload::SplitPath("one",d,f,e);
    require(lengthScans==scansBeforeHit,"a cache hit compares input once without rescanning its length");
    require(splits==1 && skload::g_stats.pathHits==1 && !std::strcmp(d,"dir:one") && !std::strcmp(f,"file:one") && !std::strcmp(e,"ext:one"),"same path reuses exact native output strings");
    skload::SplitPath("two",d,f,e);skload::SplitPath("one",d,f,e);
    require(splits==3,"cache contains only the last full input path");
    skload::SetLoading(false);skload::SplitPath("one",d,f,e);
    require(splits==4,"outside loading paths remain native");
    reset(2);skload::SetLoading(true);skload::SplitPath("one",d,f,e);skload::SplitPath("one",d,f,e);
    require(splits==6 && skload::g_stats.pathCandidates==1 && !skload::g_stats.pathHits,"diagnostic baseline observes candidates without reuse");
    reset(2);skload::SetLoading(true);
    for (unsigned i=0;i<5000;++i) skload::SplitPath("one",d,f,e);
    require(skload::g_stats.pathNativeCalls==5000 && skload::g_stats.pathSamples==2,"native timing samples only one in 4096 calls");
    splits=6;
    reset(9);skload::SetLoading(true);char longPath[300];std::memset(longPath,'x',299);longPath[299]=0;
    skload::SplitPath(longPath,d,f,e);skload::SplitPath(longPath,d,f,e);
    require(splits==8 && skload::g_stats.pathBypasses==2,"oversize input passes through without caching truncated keys");
    skload::g_ownerThread=0;skload::SplitPath("one",d,f,e);
    require(splits==9,"other threads use the native splitter");
    skload::g_ownerThread=GetCurrentThreadId();
    reset(1u|16u|0x1000u);skload::SetLoading(true);
    const auto fileRequestBefore=skload::g_pathRequest;
    const auto fileOnlyNativeBefore=splits;
    skload::SplitPath("file_only_a",d,f,e);const auto fileRequestA=skload::g_pathRequest;
    skload::SplitPath("file_only_a",d,f,e);
    require(fileRequestA!=fileRequestBefore && skload::g_pathRequest==fileRequestA,
        "file-only mode uses one input token across repeated native splits");
    require(splits==fileOnlyNativeBefore+2,"file-only tracking does not enable path-result reuse");
    skload::SplitPath("file_only_b",d,f,e);
    require(skload::g_pathRequest!=fileRequestA,"a changed path invalidates exact-file decisions without path reuse");
    reset(9);skload::SetLoading(true);
    char mutablePath[260]="Materials/one.vmt";
    skload::SplitPath(mutablePath,d,f,e);
    strcpy_s(mutablePath,"materials/one.vmt");
    skload::SplitPath(mutablePath,d,f,e);
    require(!skload::g_stats.pathHits && !std::strcmp(d,"dir:materials/one.vmt"),"pointer reuse and case changes are compared by bytes");
    auto* guarded=static_cast<char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    require(guarded!=nullptr,"guard fixture allocated");
    DWORD guardOld=0;
    require(VirtualProtect(guarded+4096,4096,PAGE_NOACCESS,&guardOld)!=0,"input tail is inaccessible");
    char* shortPath=guarded+4094;shortPath[0]='x';shortPath[1]=0;
    skload::SplitPath(shortPath,d,f,e);skload::SplitPath(shortPath,d,f,e);
    require(skload::g_stats.pathHits==1 && !std::strcmp(d,"dir:x"),"short input at a page boundary is not read to the old key length");
    shortPath[0]=0;skload::SplitPath(shortPath,d,f,e);skload::SplitPath(shortPath,d,f,e);
    require(skload::g_stats.pathHits==2 && !std::strcmp(d,"dir:"),"empty key remains distinct and cacheable");
    VirtualFree(guarded,0,MEM_RELEASE);
    skload::SetLoading(false);
    skload::g_readOriginal=NativeRead;
    skload::g_vpkReadReturn=code+700;
    skload::g_ownerThread=GetCurrentThreadId();
    DWORD bytes=0;
    reset(2);skload::SetLoading(true);
    for(unsigned i=0;i<65;++i)
        require(skload::ReadAt(code+700,nullptr,d,1024,&bytes,nullptr)!=FALSE,"read result forwarded");
    require(reads==65 && bytes==1024 && GetLastError()==123 && skload::g_stats.readCalls==65
        && skload::g_stats.readBytes==65*1024 && skload::g_stats.readSmall==65 && skload::g_stats.readSamples==2,
        "VPK diagnostic counts bytes/small reads and samples one in 64 without changing LastError");
    skload::ReadAt(code+701,nullptr,d,4096,&bytes,nullptr);
    skload::SetLoading(false);skload::ReadAt(code+700,nullptr,d,4096,&bytes,nullptr);
    reset(9);skload::SetLoading(true);skload::ReadAt(code+700,nullptr,d,4096,&bytes,nullptr);
    require(reads==68 && !skload::g_stats.readCalls,"unrelated reads, completed loads and diagnostic-off reads stay native");
    reset(2);skload::SetLoading(true);readOk=false;
    require(!skload::ReadAt(code+700,nullptr,d,8192,&bytes,nullptr) && skload::g_stats.readFailures==1
        && !skload::g_stats.readBytes,"failed reads stay failed and are observable");
    readOk=true;skload::g_ownerThread=0;
    skload::ReadAt(code+700,nullptr,d,8192,&bytes,nullptr);
    require(skload::g_stats.readCalls==1,"worker reads do not touch main-thread diagnostic counters");
    skload::g_ownerThread=GetCurrentThreadId();skload::SetLoading(false);
    auto* readSlot=reinterpret_cast<volatile LONG*>(code+704);
    *readSlot=LONG(uintptr_t(&NativeRead));
    require(skload::PatchRead(readSlot,NativeRead,true),"ReadFile IAT hook installs atomically");
    require(skload::PatchRead(readSlot,NativeRead,false) && *readSlot==LONG(uintptr_t(&NativeRead)),"ReadFile IAT restores its own import");
    *readSlot=LONG(uintptr_t(&NativeDraw));
    require(!skload::PatchRead(readSlot,NativeRead,true) && *readSlot==LONG(uintptr_t(&NativeDraw)),"foreign IAT target is preserved");
    const unsigned char readPattern[]={0x8b,0x55,0x10,0x6a,0,0x8d,0x4d,8,0x51,0x8b,0x4d,0x0c,0x52,0x8b,0x50,4,0x51,0x52,0xff,0x15,1,2,3,4,0x8b,0x4d,8,0x01,0x4e,0x0c,0x8b,0x45,0xfc,0x8b,0x55,0xf8};
    std::memcpy(code,readPattern,sizeof(readPattern));
    require(skload::FindReadSite(code,4096)==code+18 && !skload::FindReadSite(code,sizeof(readPattern)-1),"VPK synchronous ReadFile call matches its full ABI context");
    std::memcpy(code+80,readPattern,sizeof(readPattern));
    require(!skload::FindReadSite(code,4096),"ambiguous VPK ReadFile caller is rejected");
    std::memset(code,0,4096);
    const unsigned char pathSignature[]={0x8d,0x8d,0xf0,0xfc,0xff,0xff,0x51,0x50,0xe8,1,2,3,4,0x83,0xc4,0x10,0x8d,0x56,0x20,0x52,0x6a,0};
    std::memcpy(code+3,pathSignature,sizeof(pathSignature));
    require(skload::FindPathSite(code,4096)==code+11,"packed-store split call identified");
    unsigned char pathOriginal[]={0xe8,0,0,0,0};LONG pathRel=LONG(uintptr_t(&NativeSplit)-uintptr_t(code+8));std::memcpy(pathOriginal+1,&pathRel,4);std::memcpy(code+3,pathOriginal,5);
    require(skload::AttachPath(code+3,NativeSplit),"aligned CALL operand can be changed atomically");
    require(skload::DetachPath() && !std::memcmp(code+3,pathOriginal,5),"path CALL restores exact original operand");
    code[4]^=1;unsigned char foreignPath[5];std::memcpy(foreignPath,code+3,5);
    require(!skload::AttachPath(code+3,NativeSplit) && !std::memcmp(code+3,foreignPath,5),"reattach refuses a foreign CALL target");
    require(!skload::AttachPath(code,NativeSplit),"unaligned operands are rejected rather than written non-atomically");
    std::memset(code,0,4096);
    auto* finishSlot=reinterpret_cast<volatile LONG*>(code+600);
    auto finishOriginal=reinterpret_cast<skload::FinishedFn>(&NativeFinished);
    *finishSlot=LONG(uintptr_t(&NativeFinished));
    require(skload::AttachFinished(finishSlot,finishOriginal),"loading-finished vtable pointer installed atomically");
    skload::g_ownerThread=GetCurrentThreadId();skload::g_loading.store(true);skload::g_pathCache.valid=true;
    skload::LoadingFinished(nullptr,nullptr);
    require(finished==1 && skload::g_stats.finishedCalls==1 && !skload::g_loading.load() && !skload::g_pathCache.valid && skload::g_finishedThread.load()==GetCurrentThreadId(),"native completion clears load work without a subsequent Paint");
    require(skload::DetachFinished() && *finishSlot==LONG(uintptr_t(&NativeFinished)),"finished pointer restores exact original method");
    const unsigned char signature[]={0x0a,0xc3,0x74,0x05,0xe8,1,2,3,4,0x8b,0x45,0x08,0x89,0x87,0xac,0,0,0,0xff,0x15};
    std::memcpy(code,signature,sizeof(signature));
    require(skload::FindSite(code,4096)==code+4,"unique complete call site identified");
    require(!skload::FindSite(code,sizeof(signature)-1),"truncated call-site context is rejected");
    std::memcpy(code+40,signature,sizeof(signature));
    require(!skload::FindSite(code,4096),"ambiguous signature is rejected");
    std::memset(code+40,0,sizeof(signature)); code[4]=0xe9;
    require(!skload::FindSite(code,4096),"a previous plugin detour is rejected");
    const unsigned char original[5]={0xe8,1,2,3,4};
    std::memcpy(code,original,5);
    require(skload::Attach(code,NativeDraw),"attach replaces a complete CALL rel32 instruction");
    int32_t rel=0; std::memcpy(&rel,code+1,4);
    require(code+5+rel==reinterpret_cast<unsigned char*>(&skload::ScreenUpdate),"patched CALL reaches the production callback");
    require(skload::Detach() && !std::memcmp(code,original,5),"detach restores exact original bytes");
    require(skload::Attach(code,NativeDraw),"attach before configuration failure edge");
    skload::g_mode.store(0); skload::Configure(0);
    require(!skload::g_installed && !std::memcmp(code,original,5),"off restores an installed patch even when no mode was activated");
    require(skload::Attach(code,NativeDraw),"reattach supported"); code[1]^=1;
    unsigned char foreign[5]; std::memcpy(foreign,code,5);
    require(!skload::Detach() && !std::memcmp(code,foreign,5),"foreign patch is retained and conflict is reported");
    std::memcpy(code,skload::g_patch,5); require(skload::Detach(),"fixture releases its own restored patch");
    // Retail directory nodes, independent of a packed file's data payload.
    struct DirNode { DirNode* next; const char* name; } dirOne{nullptr,"dir:one"};
    struct ExtNode { ExtNode* next; const char* name; DirNode* dirs[43]; } extOne{};
    extOne.name="ext:one";extOne.dirs[0]=&dirOne;
    alignas(4) unsigned char store[0x280]{};
    auto** roots=reinterpret_cast<ExtNode**>(store+0x22c);roots[0]=&extOne;
    reset(25);skload::SetLoading(true);skload::ResetCandidates();
    const auto oneHash=skload::CandidateHash("dir:one","ext:one");
    require(skload::MayContain(store,oneHash),"a present directory never gets rejected");
    require(!skload::MayContain(store,skload::CandidateHash("dir:missing","ext:missing")),"an absent directory can skip the original packed lookup");
    require(skload::g_stats.indexBuilds==1 && skload::g_stats.indexNodes==1,"index builds once per store in a load window");
    skload::CandidateMutationBegin();
    dirOne.name="dir:two";extOne.name="ext:two";
    require(skload::MayContain(store,oneHash),"a concurrent/nested mutation never rejects from an old index");
    skload::CandidateMutationEnd();
    require(skload::MayContain(store,skload::CandidateHash("dir:two","ext:two")) && skload::g_stats.indexBuilds==2,"mutation invalidates even when store address and count are unchanged");
    skload::CandidateMutationBegin();dirOne.name="dir:one";extOne.name="ext:one";skload::CandidateMutationEnd();
    alignas(4) unsigned char emptyA[0x280]{},emptyB[0x280]{},fs[0x200]{};
    void* packs[]{emptyA,emptyB,store};
    *reinterpret_cast<void***>(fs+0x1cc)=packs;
    *reinterpret_cast<unsigned*>(fs+0x1d8)=3;
    auto* expired=static_cast<void**>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS));
    require(expired!=nullptr,"expired-array fixture allocated");
    *reinterpret_cast<void***>(fs+0x1cc)=expired;skload::ResetCandidates();unsigned next=99;
    require(GuardNext(fs,emptyA,next) && next==0,"an expired borrowed pack array cannot fault the loading hook");
    const uint64_t failedLists=skload::g_stats.indexBypasses;
    require(GuardNext(fs,emptyA,next) && next==0 && skload::g_stats.indexBypasses==failedLists,
        "a failed list is not rebuilt repeatedly for the same directory and epoch");
    *reinterpret_cast<void***>(fs+0x1cc)=packs;skload::ResetCandidates();VirtualFree(expired,0,MEM_RELEASE);
    reset(25);skload::SetLoading(true);skload::ResetCandidates();
    require(skload::NextCandidate(fs,0,emptyA,oneHash,"dir:one","file:one","ext:one")==2,"ordered candidates skip a whole absent run in original pack order");
    require(skload::NextCandidate(fs,1,emptyB,oneHash,"dir:one","file:one","ext:one")==2 && skload::NextCandidate(fs,2,store,oneHash,"dir:one","file:one","ext:one")==2
        && skload::g_stats.candidateLists==1,"one directory list is reused without rescanning packs for each lookup");
    require(skload::NextCandidate(fs,0,emptyA,skload::CandidateHash("absent","ext:one"),"absent","file:one","ext:one")==3,"all absent reaches the original loop end");
    skload::CandidateMutationBegin();
    require(skload::NextCandidate(fs,0,emptyA,oneHash,"dir:one","file:one","ext:one")==0,"mutation in flight never advances a loop cursor");
    skload::CandidateMutationEnd();
    skload::g_candidateStatus=skload::Status::Ready;
    skload::g_loopParentReturn=reinterpret_cast<void*>(0x12345678);
    uintptr_t parentStorage[4]{uintptr_t(fs),0,0,0};auto* parent=parentStorage+1;
    uintptr_t child[2]{uintptr_t(parent),uintptr_t(skload::g_loopParentReturn)};
    parent[2]=0;
    require(skload::CandidateSplit("one",d,f,e,emptyA,child)==-1 && parent[2]==1,
        "parent-stack loop advances only to the index before the first possible pack");
    parent[2]=2;
    require(!skload::CandidateSplit("one",d,f,e,store,child) && parent[2]==2 && !std::strcmp(d,"dir:one"),
        "candidate pack keeps the native lookup and exact split outputs");
    *reinterpret_cast<unsigned*>(fs+0x1d8)=257;
    require(skload::NextCandidate(fs,0,emptyA,oneHash,"dir:one","file:one","ext:one")==0,"oversize pack lists stay native without truncated filtering");
    *reinterpret_cast<unsigned*>(fs+0x1d8)=3;
    reset(25);skload::SetLoading(true);skload::ResetCandidates();
    require(!skload::CandidateSplit("one",d,f,e,store) && !std::strcmp(d,"dir:one"),"candidate pass preserves all native split outputs");
    skload::g_candidateStatus=skload::Status::Ready;
    require(!skload::CandidateSplit("missing",d,f,e,store),"unclassified callers keep native lookup instead of the discarded per-pack experiment");
    skload::g_candidateStatus=skload::Status::Off;
    require(!skload::CandidateSplit("missing",d,f,e,store),"an inactive index never rejects if a path detour is still in flight");
    skload::g_candidateStatus=skload::Status::Ready;
    skload::g_ownerThread=0;
    require(!skload::CandidateSplit("missing",d,f,e,store),"other threads retain the original packed lookup");
    skload::g_ownerThread=GetCurrentThreadId();
    skload::ResetCandidates();skload::g_indexBudget=skload::kIndexNodeBudget;
    require(skload::MayContain(store,oneHash),"budget exhaustion never publishes an incomplete negative index");
    skload::ResetCandidates();roots[0]=nullptr;
    require(!skload::MayContain(store,oneHash),"an empty store is safely rejected");
    roots[0]=&extOne;skload::ResetCandidates();
    // Execute the production x86 thunk through an aligned CALL in a small
    // thiscall fixture. The native failure epilogue requires ret16 in the thunk.
    std::memset(code,0x90,4096);unsigned used=0;
    auto emit=[&](const unsigned char* b,unsigned n){std::memcpy(code+used,b,n);used+=n;};
    const unsigned char setup[]={0x55,0x8b,0xec,0x81,0xec,0x10,3,0,0,0x56,0x57,0x8b,0xf9,0x8b,0x75,8,
        0x8d,0x85,0xf4,0xfd,0xff,0xff,0x50,0x8d,0x85,0xf8,0xfe,0xff,0xff,0x50,0x8d,0x85,0xf0,0xfc,0xff,0xff,0x50,0xff,0x75,0x0c};
    emit(setup,sizeof(setup));while((used+1)%4)++used;
    auto* candidateCall=code+used;code[used++]=0xe8;
    LONG originalRel=LONG(uintptr_t(&NativeSplit)-uintptr_t(code+used+4));emit(reinterpret_cast<unsigned char*>(&originalRel),4);
    const unsigned char pass[]={0x83,0xc4,0x10,0xc7,0x06,0x34,0x12,0,0,0x5f,0x8b,0xc6,0x5e,0x8b,0xe5,0x5d,0xc2,8,0};
    emit(pass,sizeof(pass));skload::g_pathRejectSite=code+used;
    const unsigned char reject[]={0x5f,0xc7,0x06,0xff,0xff,0xff,0xff,0xc7,0x46,0x18,0,0,0,0,0x8b,0xc6,0x5e,0x8b,0xe5,0x5d,0xc2,8,0};
    emit(reject,sizeof(reject));FlushInstructionCache(GetCurrentProcess(),code,used);
    require(skload::AttachPath(candidateCall,NativeSplit,true),"candidate thunk uses the existing aligned CALL patch");
    using OpenFixture=void*(__thiscall*)(void*,unsigned*,const char*);
    auto openFixture=reinterpret_cast<OpenFixture>(code);unsigned handle[9]{};
    reset(25);skload::SetLoading(true);skload::ResetCandidates();
    void* opened=openFixture(store,handle,"one");
    require(opened==handle && handle[0]==0x1234,"thunk preserves the original successful continuation and stack");
    handle[6]=123;
    require(openFixture(store,handle,"missing")==handle && handle[0]==0x1234,"an unknown outer caller continues natively with its original stack");
    inlineInnerCode=code;inlineReturnSlot=&skload::g_loopInlineReturn;
    reset(25);skload::SetLoading(true);skload::ResetCandidates();unsigned cursor=99;
    handle[6]=123;InlineFixture(fs,emptyA,handle,"one",&cursor);
    require(cursor==1 && handle[0]==~0u && handle[6]==0,
        "production thunk advances inline EBX while keeping the native failure handle and stack");
    skload::SetLoading(false);reset(17);skload::SetLoading(true);skload::ResetCandidates();
    std::memcpy(skload::g_pathCache.key,"prior",6);skload::g_pathCache.keyLen=5;
    const unsigned splitsBeforeVpkOnly=splits;
    InlineFixture(fs,emptyA,handle,"one",&cursor);
    require(cursor==1 && handle[0]==~0u && skload::g_pathCache.directoryHash==oneHash,
        "VPK-only keeps ordered pack skipping without path-result reuse");
    InlineFixture(fs,emptyA,handle,"one",&cursor);
    require(splits==splitsBeforeVpkOnly+2 && !skload::g_stats.pathHits && !skload::g_stats.pathCandidates
        && !std::strcmp(skload::g_pathCache.key,"prior") && skload::g_pathCache.keyLen==5,
        "VPK-only retains native splits without comparing or updating the path-reuse key");
    skload::SetLoading(false);reset(19);skload::SetLoading(true);skload::ResetCandidates();
    skload::g_candidateFind=reinterpret_cast<skload::FindEntryFn>(&VerifyCurrent);
    skload::g_stats.loopRejected=4095;
    InlineFixture(fs,emptyA,handle,"missing",&cursor);
    require(verifications==1 && skload::g_stats.indexVerified==1 && !skload::g_stats.indexMismatches && cursor==2,
        "VPK-only diagnostic crosses the excluded-pack boundary without a false mismatch");
    skload::SetLoading(false);reset(25);skload::SetLoading(true);skload::ResetCandidates();
    skload::CandidateMutationBegin();packs[0]=store;skload::CandidateMutationEnd();
    InlineFixture(fs,store,handle,"one",&cursor);
    require(cursor==0 && handle[0]==0x1234,"possible pack retains original inline cursor and successful continuation");
    require(skload::DetachPath(),"candidate thunk restores the original split CALL");
    // A third-party CALL conflict must not strand the independent cold hooks.
    for (unsigned i=0;i<6;++i) {
        auto* slot=reinterpret_cast<volatile LONG*>(code+800+i*4);
        *slot=LONG(uintptr_t(&NativeDraw));
        skload::g_mutationSlots[i]={slot,*slot,LONG(uintptr_t(&NativeSplit)),false};
        require(skload::PatchMutation(skload::g_mutationSlots[i],true),"mutation fixture attaches");
    }
    skload::g_pathStatus=skload::Status::Conflict;
    skload::ConfigureExperiments(9);
    for (auto& slot:skload::g_mutationSlots)
        require(!slot.installed && *slot.slot==slot.native,"path conflict cannot retain candidate mutation hooks");
    skload::g_pathStatus=skload::Status::Off;skload::g_pathChecked=true;
    skload::g_candidateChecked=true;skload::g_candidateStatus=skload::Status::Off;
    candidateCall[1]^=1;
    skload::ConfigureExperiments(25);
    require(skload::g_pathStatus==skload::Status::Conflict,"foreign path patch is still reported");
    for (auto& slot:skload::g_mutationSlots)
        require(!slot.installed && *slot.slot==slot.native,"failed path attachment rolls back candidate mutation hooks");
    candidateCall[1]^=1;
    skload::g_pathRejectSite=nullptr;skload::SetLoading(false);
    auto& native=skload::g_nativePreload;
    auto* groupImage=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x500000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    require(groupImage!=nullptr,"empty static-prop map fixture");
    skload::g_staticPropGroups.engine=groupImage;
    skload::g_staticPropGroups.setter=reinterpret_cast<skload::StaticPropGroups::SetGroup>(&NativeDraw);
    native.object=code;native.read=AsyncRead;native.write=AsyncWrite;
    native.finish=reinterpret_cast<decltype(native.finish)>(&AsyncFinish);
    asyncFloats[0]=.5f;asyncFloats[1]=0.f;
    skload::g_nativeAsyncWant.store(7);
    native.Begin();
    require(native.active && asyncWrites==3,"three native switches leased once per load");
    for(unsigned i=0;i<1000;++i)native.Begin();
    require(asyncWrites==3 && asyncReads==3,"repeated loading redraw has no native switch work");
    require(native.Complete() && !native.active && asyncFinishes==1
        && asyncInts[0]==0 && asyncFloats[0]==.5f && asyncInts[1]==0 && asyncInts[2]==0,
        "pending drain restores the original numeric values including fractional values");
    native.Complete();require(asyncFinishes==1,"duplicate finish does no native work");
    native.Begin();asyncInts[1]=2;asyncFloats[1]=2.f;
    require(native.Complete() && asyncInts[1]==2 && skload::g_stats.asyncConflicts==1,
        "a later external switch edit is preserved");
    asyncInts[1]=1;asyncFloats[1]=1.f;
    native.Begin();asyncWriteOk=false;
    require(!native.Complete() && native.active && native.owned[0],
        "failed restoration retains outstanding ownership for an explicit retry");
    asyncWriteOk=true;const unsigned finishesBeforeRetry=asyncFinishes;
    require(native.Complete() && !native.active && asyncFinishes==finishesBeforeRetry+1,
        "retry drains again because failed restoration left async mode enabled for new jobs");
    const unsigned writesBefore=asyncWrites,readsBefore=asyncReads;
    asyncInts[0]=1;asyncFloats[0]=1.f;asyncInts[1]=1;asyncFloats[1]=1.f;asyncInts[2]=1;asyncFloats[2]=1.f;native.Begin();
    require(!native.active && asyncWrites==writesBefore && asyncReads==readsBefore+3,
        "already asynchronous native settings are left alone");
    asyncInts[0]=0;asyncFloats[0]=0.f;
    native.Begin();checkAsyncBeforeFinished=true;
    skload::g_finishedOriginal=reinterpret_cast<skload::FinishedFn>(&NativeFinished);
    skload::LoadingFinished(code,nullptr);
    require(!native.active,"native finished boundary completes the lease");
    checkAsyncBeforeFinished=false;
    native.Begin();asyncWriteOk=false;
    reset(57);skload::g_requestedMode=~0u;
    std::memset(code+3800,0x90,5);
    require(skload::Attach(code+3800,NativeDraw),"redraw fixture attached for deferred-off test");
    skload::Configure(0);
    require(native.active && skload::g_installed,"failed cleanup retains a completion entry");
    asyncWriteOk=true;skload::LoadingFinished(code,nullptr);
    require(!native.active && !skload::g_installed && skload::g_mode==0,
        "later native completion honors the already-requested off without another command");
    native.Begin();asyncWriteOk=false;reset(61);skload::g_requestedMode=~0u;
    require(skload::Attach(code+3800,NativeDraw),"redraw reattached for nonzero retry");
    skload::Configure(29);
    require(native.active && skload::g_status==skload::Status::PatchFailed,"nonzero request observes restoration failure");
    asyncWriteOk=true;skload::g_requestedMode=~0u;skload::Configure(29);
    // This standalone process has no engine VGUI to reattach; Unsupported at
    // that later boundary proves the stale PatchFailed gate was cleared.
    require(!native.active && !skload::g_installed && skload::g_status==skload::Status::Unsupported,
        "explicit nonzero retry cleans recovered ownership before applying the requested configuration");
    asyncInts[0]=asyncInts[1]=asyncInts[2]=0;
    asyncFloats[0]=asyncFloats[1]=asyncFloats[2]=0.f;
    skload::g_nativeAsyncWant.store(2);
    const unsigned collideWrites=asyncWrites;
    native.Begin();
    require(native.active && !native.owned[0] && native.owned[1] && !native.owned[2]
        && asyncWrites==collideWrites+1 && asyncInts[0]==0 && asyncInts[1]==1 && asyncInts[2]==0,
        "collision-only selection leases collision alone");
    require(native.Complete() && !native.active && asyncInts[1]==0,
        "collision-only lease restores only itself");
    skload::g_nativeAsyncWant.store(0);
    const unsigned idleWrites=asyncWrites;
    native.Begin();
    require(!native.active && asyncWrites==idleWrites, "no selected switch does no native work");
    native={};
    VirtualFree(code,0,MEM_RELEASE);
    std::puts("PASS fast load: native redraw/load wait/path reuse/diagnostics/bounds/owner/atomic patch/restore/conflict");
    return 0;
}
'''


def main():
    assert HEADER.exists(), "missing production loading redraw controller"
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                  "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-fast-load-") as tmp:
        folder = Path(tmp)
        (folder / "fixture.cpp").write_text(FIXTURE, encoding="utf-8")
        (folder / "run.bat").write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
            f'cl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 /W3 /WX /I"{HEADER.parent}" fixture.cpp /Fe:fixture.exe\n'
            'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)


if __name__ == "__main__":
    main()
