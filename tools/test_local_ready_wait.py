"""Exercise the production local start-wait operand and ownership in x86.

Native server scheduling, remote clients and real loading are runtime checks.
"""
import test_fast_load as native
from pathlib import Path

source=(Path(__file__).resolve().parents[1]/"src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8")
begin="static void FastLoadBeginLocalReady(bool background) {"
edge=begin+source.split(begin,1)[1].split("static void FastLoadGamePump()",1)[0]

native.FIXTURE = r'''
#define NOMINMAX
#include "skeeto_fast_load.h"
#include <cstdio>
#include <cstdlib>
#pragma comment(lib,"psapi.lib")
static std::atomic_bool g_run{true};
''' + edge + r'''
static void require(bool ok,const char* why) {
    if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}
}
static DWORD WINAPI ForeignEnd(void*) { return skload::EndLocalReadyDelay()?1u:0u; }
static unsigned finishedCalls=0;
static void __fastcall FinishedFixture(void*,void*) {++finishedCalls;}
int main() {
    constexpr size_t size=0x5e0000;
    auto* image=static_cast<uint8_t*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    require(image!=nullptr,"fixture image allocation");
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);
    dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(image+0x80);
    nt->Signature=IMAGE_NT_SIGNATURE;nt->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;nt->OptionalHeader.SizeOfImage=size;
    const uint8_t head[]{0x55,0x8b,0xec,0x83,0xec,0x2c,0x53,0x8b,0xd9,0x80,0x7b,0x28,0};
    memcpy(image+0x28d500,head,sizeof(head));
    uint8_t instruction[]{0xf3,0x0f,0x58,0x05,0,0,0,0};
    uintptr_t constant=uintptr_t(image+0x5d4920);memcpy(instruction+4,&constant,4);
    memcpy(image+0x28d69f,instruction,8);
    const uint8_t store[]{0xf3,0x0f,0x11,0x43,0x2c};
    memcpy(image+0x28d6a7,store,sizeof(store));
    *reinterpret_cast<float*>(image+0x5d4920)=3.f;
    skload::LocalReadyDelay patch;
    require(!patch.owned,"default has no engine influence");
    require(patch.Begin(image,size),"admitted native countdown attaches");
    uint8_t shortened[8];memcpy(shortened,instruction,8);shortened[4]=0x1c;
    require(patch.owned && !memcmp(image+0x28d69f,shortened,8),"only operand low byte changes; instruction boundaries remain");
    require(*reinterpret_cast<float*>(image+0x5d4920)==3.f && *reinterpret_cast<float*>(image+0x5d491c)==0.f,
        "shared constants remain untouched");
    require(!memcmp(image+0x28d6a7,store,sizeof(store)),"native deadline store remains intact");
    require(patch.Begin(image,size) && patch.begins==1,"repeated edge does not write again");
    require(patch.End(image) && !patch.owned && patch.restores==1 && !memcmp(image+0x28d69f,instruction,8),
        "explicit close restores the native operand");
    require(patch.End(image) && patch.restores==1,"repeated close is inert");
    nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;
    require(!patch.Begin(image,size) && !patch.owned,"other ABI rejected before patching");
    nt->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    require(!patch.Begin(image,0x1000),"truncated module rejected");
    *reinterpret_cast<float*>(image+0x5d491c)=1.f;
    require(!patch.Begin(image,size) && !patch.owned,"nonzero alternate constant rejected");
    *reinterpret_cast<float*>(image+0x5d491c)=0.f;
    require(patch.Begin(image,size),"new explicit edge can attach after unsupported input");
    image[0x28d6a3]=0x34;
    patch.End(image);
    require(!patch.owned && image[0x28d6a3]==0x34 && patch.conflicts==1,"third-party operand survives close");
    image[0x28d6a3]=0x20;
    require(patch.Begin(image,size),"next map can attach again");
    require(patch.End(nullptr) && !patch.owned && image[0x28d6a3]==0x1c,
        "unloaded module is relinquished without a late write");
    image[0x28d6a3]=0x20;
    skload::g_ownerThread=GetCurrentThreadId();
    require(skload::g_localReadyDelay.Begin(image,size),"global ownership fixture");
    HANDLE worker=CreateThread(nullptr,0,ForeignEnd,nullptr,0,nullptr);
    require(worker && WaitForSingleObject(worker,5000)==WAIT_OBJECT_0,"foreign shutdown callback returns");
    CloseHandle(worker);
    require(skload::g_localReadyDelay.owned && skload::g_localReadyEndPending.exchange(false)
        && skload::g_localReadyForeignThread==1,"foreign callback publishes one explicit close without writing code");
    require(skload::g_localReadyDelay.End(image) && image[0x28d6a3]==0x20,"owner consumes the close and restores");
    require(skload::g_localReadyDelay.Begin(image,size),"background edge fixture ownership");
    skload::g_mode=64;skload::g_localReadyEndPending=true;
    FastLoadBeginLocalReady(true);
    require(!skload::g_localReadyEndPending && !skload::g_localReadyDelay.owned,
        "background LevelInit consumes a prior close without attaching a new patch");
    image[0x28d6a3]=0x20;
    require(skload::g_localReadyDelay.Begin(image,size),"combined early scene fixture owns native countdown");
    skload::g_mode=193; // master + local ready + early view; visual completion precedes server readiness.
    skload::g_earlyGameView.owned=true;
    skload::g_finishedOriginal=reinterpret_cast<skload::FinishedFn>(&FinishedFixture);
    skload::LoadingFinished(nullptr,nullptr);
    require(finishedCalls==1 && skload::g_localReadyDelay.owned && image[0x28d6a3]==0x1c,
        "early scene completion must not cancel the still-needed local countdown optimization");
    require(skload::g_localReadyDelay.End(image) && image[0x28d6a3]==0x20,
        "later local shutdown/option edge restores the combined lease");
    skload::g_earlyGameView.owned=false;
    VirtualFree(image,0,MEM_RELEASE);
    std::puts("PASS local ready wait: x86/default-off/one-byte operand/constants/restore/conflict/module lifetime");
}
'''

if __name__ == "__main__":
    native.main()
