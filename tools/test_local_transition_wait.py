"""Production ConVar lease: local transition wait, ownership and restoration."""
import test_fast_load as native
from pathlib import Path
source=(Path(__file__).resolve().parents[1]/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8')
# Stop at the local transition block's boundary. VPK/async configuration below
# has its own fixtures and must not leak unrelated production dependencies here.
pump=source.split('\tconst bool wantTransition=',1)[1].split('\tconst unsigned vpkWant=',1)[0]
pump='\tconst bool wantTransition='+pump
assert 'g_fastLoadLocalTransitionEnabled.store(wantTransition,std::memory_order_release)' in pump, 'Paint must publish the option snapshot only when its state changes'
event_handler=source.split('static void HandleEvent(void* ev) {',1)[1].split('\nstatic void SkeetoEventListener',1)[0]
assert 'g_fastLoadLocalTransitionPending.store(true' in event_handler, 'map_transition must arm the one-shot local wait override'
transition_gate=event_handler.split('if (!SkeetoFeaturesOn()) return;',1)[1].split('InterlockedIncrement(&g_fireCalls);',1)[0]
assert 'g_fastLoadLocalTransitionEnabled.load' in transition_gate, 'Event must consume the owner-thread option snapshot atomically'
assert 'g_optFastLoad' not in transition_gate, 'Event must not read non-atomic settings'
level_init=source.split('static bool __fastcall Hooked_ServerLevelInit',1)[1].split('static void __fastcall Hooked_ServerLevelShutdown',1)[0]
assert 'g_localTransitionWait.Begin' not in level_init, 'the zero wait must not span an entire level'
level_shutdown=source.split('static void __fastcall Hooked_ServerLevelShutdown',1)[1].split('static void EnsureServerGameHooks',1)[0]
restore_at=level_shutdown.find('g_localTransitionWait.End(')
native_shutdown_at=level_shutdown.find('g_origServerLevelShutdown(ecx, edx)')
assert restore_at >= 0 and native_shutdown_at > restore_at, 'restore the native wait before the next level lifecycle begins'

native.FIXTURE = r'''
#define NOMINMAX
#include "skeeto_fast_load.h"
#include <cstdio>
#include <cstdlib>
static int value=8,reads=0,writes=0;
static float number=8.f;
static bool writeOk=true;
static bool Read(const char*,int* n,float* f){++reads;*n=value;*f=number;return true;}
static bool Write(const char*,int n,float f){++writes;if(!writeOk)return false;value=n;number=f;return true;}
static void require(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static bool g_optFastLoad=false,g_optFastLoadLocalTransition=false,features=true,transitionWanted=false;
static bool g_optFastLoadAsyncMesh=false,g_optFastLoadAsyncCollide=false,g_optFastLoadAsyncAnims=false;
static std::atomic_bool g_fastLoadLocalLevel{false},g_fastLoadLocalLevelEdge{false};
static std::atomic_bool g_fastLoadLocalTransitionEnabled{false};
static std::atomic_bool g_fastLoadLocalTransitionPending{false};
static bool SkeetoFeaturesOn(){return features;}
static bool FastLoadAsyncRead(const char* name,int* n,float* f){return Read(name,n,f);}
static bool FastLoadAsyncWrite(const char* name,int n,float f){return Write(name,n,f);}
#define GetModuleHandleA(...) reinterpret_cast<void*>(1)
static void Pump(){
''' + pump + r'''
}
#undef GetModuleHandleA
int main(){
    skload::LocalTransitionWait lease;lease.read=Read;lease.write=Write;
    void* module=reinterpret_cast<void*>(1);
    require(!lease.owned && reads==0 && writes==0,"default has no native effects");
    require(lease.Begin(module) && lease.owned && number==0.f,"remove native extra 8-second wait");
    require(lease.Begin(module) && reads==1 && writes==1,"repeated enable does no extra work");
    require(lease.End(module) && !lease.owned && value==8 && number==8.f,"restore exact original numeric pair");
    require(lease.End(module) && writes==2,"repeated close is inert");
    value=0;number=0.f;
    require(lease.Begin(module) && !lease.owned && number==0.f,"already disabled wait is not acquired");
    value=1;number=1.f;
    require(lease.Begin(module) && lease.owned && number==0.f,"short native pause is reduced to zero");
    require(lease.End(module) && value==1 && number==1.f,"restore an existing short native pause");
    value=12;number=12.5f;lease.Begin(module);value=4;number=4.f;
    require(lease.End(module) && value==4 && lease.conflicts==1,"preserve an external change while enabled");
    value=12;number=12.5f;lease.Begin(module);writeOk=false;
    require(!lease.End(module) && lease.owned,"failed restoration retains ownership");
    writeOk=true;require(lease.End(module) && value==12 && number==12.5f,"explicit retry restores nondefault original");
    value=8;number=8.f;lease.Begin(module);int before=reads;
    require(lease.End(nullptr) && !lease.owned && reads==before,"module unload never dereferences or writes stale server state");
    require(!lease.Begin(nullptr),"no server module cannot acquire a lease");
    value=8;number=8.f;g_optFastLoad=g_optFastLoadLocalTransition=true;
    Pump();require(!skload::g_localTransitionWait.owned && number==8.f,"remote client never acquires local server value");
    g_fastLoadLocalLevel=true;g_fastLoadLocalLevelEdge=true;Pump();
    require(number==8.f && !skload::g_localTransitionWait.owned,"enabling in a live chapter leaves native wait unchanged");
    g_fastLoadLocalTransitionPending=true;Pump();
    require(number==0.f && skload::g_localTransitionWait.owned,"map_transition event acquires the one-shot wait before shutdown");
    before=reads;for(int i=0;i<10000;++i)Pump();
    require(reads==before,"steady Paint performs no native reads or writes");
    require(number==0.f,"armed transition keeps the override only until shutdown");
    // Model a foreign-thread shutdown immediately followed by LevelInit before Paint runs.
    g_fastLoadLocalLevel=false;g_fastLoadLocalLevelEdge=true;g_fastLoadLocalTransitionPending=false;
    g_fastLoadLocalLevel=true;g_fastLoadLocalLevelEdge=true;Pump();
    require(number==8.f && !skload::g_localTransitionWait.owned,"next local level edge restores a lease when Paint missed shutdown");
    g_fastLoadLocalTransitionPending=true;Pump();
    require(number==0.f && skload::g_localTransitionWait.owned,"the new chapter can acquire its own transition lease");
    skload::g_localTransitionWait.End(module);require(number==8.f,"level shutdown restores the wait before the next chapter");
    g_optFastLoad=false;Pump();require(number==8.f && !skload::g_localTransitionWait.owned,"master off restores without losing individual choice");
    g_optFastLoad=true;Pump();g_optFastLoadLocalTransition=false;Pump();
    require(number==8.f && !skload::g_localTransitionWait.owned,"individual off restores");
    g_optFastLoadLocalTransition=true;Pump();g_fastLoadLocalLevel=false;g_fastLoadLocalLevelEdge=true;Pump();
    require(number==8.f && !skload::g_localTransitionWait.owned,"shutdown edge restores and cannot reacquire on remote client");
    g_fastLoadLocalTransitionPending=true;Pump();
    require(number==8.f && !g_fastLoadLocalTransitionPending,"stale transition request is consumed without acquiring while disabled");
    std::puts("PASS local transition wait: zero delay, original pair, external changes, failure and unload");
}
'''
if __name__ == "__main__":
    native.main()
