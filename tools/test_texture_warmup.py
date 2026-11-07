"""Native warm-up entry ABI, owner/loading isolation and owned-slot cleanup."""
import test_fast_load as native

helpers = r'''
static unsigned textureNativeCalls=0;
static void __fastcall TextureNative(void*,void*){++textureNativeCalls;}
static void CheckTextureWarmup(){
    auto& t=sktexturewarm::g_controller;
    require(!t.installed && t.skipped==0,"new texture mechanism defaults off");
    auto* slot=static_cast<volatile LONG*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    require(slot!=nullptr,"fixture vtable slot");
    t.slot=slot;t.native=LONG(uintptr_t(&TextureNative));t.hook=LONG(uintptr_t(&skload::TextureWarmup));*slot=t.native;
    require(t.Patch(true) && t.installed && *slot==t.hook,"atomic entry install");
    using Fn=sktexturewarm::Controller::Native;
    const auto call=[&](){reinterpret_cast<Fn>(uintptr_t(*slot))(nullptr);};
    skload::g_ownerThread=GetCurrentThreadId();skload::g_loading=true;skload::g_mode=0;
    call();require(textureNativeCalls==1 && t.skipped==0,"disabled mode is native passthrough even if entry remains installed");
    skload::g_mode=8193;skload::g_loading=false;
    call();require(textureNativeCalls==2 && t.skipped==0,"normal play retains native warm-up calls");
    skload::g_loading=true;skload::g_ownerThread=GetCurrentThreadId()+1;
    call();require(textureNativeCalls==3 && t.skipped==0,"foreign thread retains native API behavior");
    skload::g_ownerThread=GetCurrentThreadId();call();
    require(textureNativeCalls==3 && t.skipped==1,"only owner loading call omits the optional bulk pass");
    skload::g_mode=1u|0x1000u;call();
    require(textureNativeCalls==4 && t.skipped==1,"VPK experiment bit does not activate texture skipping");
    skload::ClearStats();require(t.skipped==0,"explicit load statistics reset has a bounded counter");
    require(skload::Detach() && !t.installed && *slot==t.native,"production disable restores the owned entry");
    require(t.Patch(true),"re-enable fixture entry");*slot=0x12349876;
    require(!t.Patch(false) && t.status==sktexturewarm::Status::Conflict && *slot==0x12349876,"never overwrite a third-party replacement");
    *slot=t.hook;require(t.Patch(false) && *slot==t.native,"explicit cleanup can finish after ownership is returned");
    require(t.Patch(true),"module unload fixture entry");t.image=reinterpret_cast<uint8_t*>(1);
    require(t.Configure(false,nullptr) && !t.installed && *slot==t.hook,"module unload abandons stale slot without late writes");
    *slot=t.native;VirtualFree(const_cast<LONG*>(slot),0,MEM_RELEASE);t={};
    require(!t.Configure(true,reinterpret_cast<void*>(&skload::TextureWarmup)) && !t.installed
        && t.status==sktexturewarm::Status::Unsupported,"unsupported process does not install a guessed ABI");
    t={};skload::g_mode=0;skload::g_loading=false;
    require(skload::ConfiguredMode(true,false,false,false,false,false,false,false,8)==8193,"independent texture mode encoding");
}
'''
assert native.FIXTURE.count('int main() {') == 1
native.FIXTURE=native.FIXTURE.replace('int main() {',helpers+'\nint main() {\n    CheckTextureWarmup();')
if __name__=='__main__':native.main()
