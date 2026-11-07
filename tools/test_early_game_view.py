"""Execute the admitted retail predicate and production byte ownership in x86."""
import test_fast_load as native

native.FIXTURE = r'''
#define NOMINMAX
#include "skeeto_memread.h"
#include <psapi.h>
static void* failProtectSite=nullptr;
static HMODULE fixtureClient=nullptr,fixtureServer=nullptr;
static BOOL WINAPI FixtureProtect(void* site,SIZE_T n,DWORD access,DWORD* old) {
    if(site==failProtectSite){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    return VirtualProtect(site,n,access,old);
}
static HMODULE WINAPI FixtureModule(const char* name) {
    if(fixtureServer && !strcmp(name,"server.dll"))return fixtureServer;
    return fixtureClient && !strcmp(name,"client.dll")?fixtureClient:GetModuleHandleA(name);
}
#define VirtualProtect FixtureProtect
#define GetModuleHandleA FixtureModule
#include "skeeto_fast_load.h"
#undef GetModuleHandleA
#undef VirtualProtect
#include <cstdio>
#include <cstdlib>
#pragma comment(lib,"psapi.lib")
static void require(bool ok,const char* why) {
    if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}
}
static void __cdecl DrawFixture(){}
static void __fastcall FinishedFixture(void*,void*){}
int main() {
    constexpr size_t size=0x740000;
    auto* image=static_cast<uint8_t*>(VirtualAlloc(nullptr,size,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    require(image!=nullptr,"image allocation");
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    auto* nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(image+0x80);
    nt->Signature=IMAGE_NT_SIGNATURE;nt->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    nt->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    uint8_t body[]{0x8b,0x0d,0,0,0,0,0x85,0xc9,0x74,0x0c,0xe8,0xd1,0x8d,0x1b,0,0x84,0xc0,0x74,3,0x32,0xc0,0xc3,0xb0,1,0xc3};
    uintptr_t global=uintptr_t(image+0x72e700);memcpy(body+2,&global,4);memcpy(image+0xa93d0,body,sizeof(body));
    const uint8_t getter[]{0x8a,0x81,0xf1,0x10,0,0,0xc3};memcpy(image+0x2621b0,getter,sizeof(getter));
    *reinterpret_cast<uintptr_t*>(image+0x72e700)=uintptr_t(image+0x700000);
    auto* frozen=image+0x7010f1;*frozen=1;
    uintptr_t table[67]{};table[66]=uintptr_t(image+0xa93d0);uintptr_t object=uintptr_t(table);
    auto predicate=reinterpret_cast<bool(__cdecl*)()>(image+0xa93d0);
    skload::EarlyGameView patch;
    require(!patch.owned && !predicate(),"default preserves frozen loading screen");
    require(patch.Begin(image,size,&object),"admit native interface and full predicate");
    require(predicate() && *frozen==1 && image[0xa93e1]==0xeb,"scene may display without unfreezing team");
    require(patch.HoldsFrozenPreview(),"map cfg waits while the admitted team freeze is still set");
    *frozen=0;require(!patch.HoldsFrozenPreview(),"unfreeze releases the deferred map cfg");*frozen=1;
    *reinterpret_cast<uintptr_t*>(image+0x72e700)=0;
    require(!patch.HoldsFrozenPreview(),"missing rules object does not stall the map cfg");
    *reinterpret_cast<uintptr_t*>(image+0x72e700)=1;
    require(!patch.HoldsFrozenPreview(),"unreadable rules object does not stall the map cfg");
    *reinterpret_cast<uintptr_t*>(image+0x72e700)=uintptr_t(image+0x700000);
    require(patch.Begin(image,size,&object) && patch.writes==1,"cached edge never repeats writes");
    require(patch.End(image) && !predicate() && !patch.owned && patch.restores==1,"close restores frozen screen gate");
    require(!patch.HoldsFrozenPreview(),"closed early view does not defer map cfg");
    require(patch.End(image) && patch.restores==1,"closed path is inert");
    *frozen=0;require(predicate(),"unfrozen native case unchanged");*frozen=1;
    table[66]++;
    require(!patch.Begin(image,size,&object) && !patch.owned && !predicate(),"foreign interface hook rejected");table[66]--;
    require(!patch.Begin(image,0x1000,&object),"truncated image rejected");
    nt->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;
    require(!patch.Begin(image,size,&object),"other ABI rejected");nt->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;
    image[0x2621b2]=0;
    require(!patch.Begin(image,size,&object),"different frozen field getter rejected");image[0x2621b2]=0xf1;
    require(patch.Begin(image,size,&object),"explicit later edge reopens");
    image[0xa93e1]=0x75;
    require(patch.End(image) && !patch.owned && image[0xa93e1]==0x75 && patch.conflicts==1,"foreign byte survives close");
    image[0xa93e1]=0x74;require(patch.Begin(image,size,&object),"next explicit edge attaches");
    require(patch.End(nullptr) && !patch.owned && image[0xa93e1]==0xeb,"unloaded module relinquishes without late write");
    image[0xa93e1]=0x74;fixtureClient=reinterpret_cast<HMODULE>(image);
    require(skload::g_earlyGameView.Begin(image,size,&object),"configuration retry fixture owns early gate");
    skload::g_ownerThread=GetCurrentThreadId();
    memset(image+0x1000,0x90,5);require(skload::Attach(image+0x1000,DrawFixture),"real redraw attachment for configuration recovery");
    skload::g_checked=true;
    LONG finishSlot=LONG(uintptr_t(&skload::LoadingFinished));
    skload::g_finishedSlot=&finishSlot;skload::g_finishedOriginal=reinterpret_cast<skload::FinishedFn>(&FinishedFixture);
    skload::g_finishedChecked=skload::g_finishedInstalled=true;skload::g_finishedStatus=skload::Status::Ready;
    skload::g_mode=129;skload::g_requestedMode=~0u;
    failProtectSite=image+0xa93e1;skload::Configure(1);
    require(skload::g_earlyGameView.owned && skload::g_mode==129 && skload::g_requestedMode==1
        && skload::g_status==skload::Status::PatchFailed,"failed disable cannot commit an off mode while still displaying early");
    failProtectSite=nullptr;skload::LoadingFinished(nullptr,nullptr);
    require(!skload::g_earlyGameView.owned && image[0xa93e1]==0x74 && skload::g_mode==1,
        "native completion retries the pending nonzero mode after protection recovers");
    fixtureServer=reinterpret_cast<HMODULE>(image);
    const uint8_t readyHead[]{0x55,0x8b,0xec,0x83,0xec,0x2c,0x53,0x8b,0xd9,0x80,0x7b,0x28,0};memcpy(image+0x28d500,readyHead,sizeof(readyHead));
    uint8_t add[]{0xf3,0x0f,0x58,0x05,0,0,0,0};uintptr_t three=uintptr_t(image+0x5d4920);memcpy(add+4,&three,4);memcpy(image+0x28d69f,add,8);
    const uint8_t readyStore[]{0xf3,0x0f,0x11,0x43,0x2c};memcpy(image+0x28d6a7,readyStore,sizeof(readyStore));*reinterpret_cast<float*>(image+0x5d4920)=3.f;
    skload::g_mode=65;skload::g_requestedMode=65;
    require(skload::g_localReadyDelay.Begin(image,size),"local level owns countdown operand");
    skload::Configure(67);
    require(skload::g_localReadyDelay.owned && image[0x28d6a3]==0x1c,"enabling diagnostics preserves the selected local countdown lease");
    skload::Configure(65);
    require(skload::g_localReadyDelay.owned && image[0x28d6a3]==0x1c,"disabling diagnostics also preserves the local lease");
    skload::Configure(1);
    require(!skload::g_localReadyDelay.owned && image[0x28d6a3]==0x20,"removing local-start restores its operand");
    require(skload::g_localReadyDelay.Begin(image,size),"rearm for master disable");skload::Configure(0);
    require(!skload::g_localReadyDelay.owned && image[0x28d6a3]==0x20,"master disable restores local operand");
    require(skload::Detach(),"fixture detaches live ownership");fixtureClient=fixtureServer=nullptr;
    VirtualFree(image,0,MEM_RELEASE);
    std::puts("PASS early game view: native predicate/team frozen/default off/restore/foreign slot/byte/module lifetime");
}
'''
if __name__ == "__main__":native.main()
