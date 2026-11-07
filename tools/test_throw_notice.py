"""Exercise production RecvProxy queue and patch ownership in an MSVC x86 fixture."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(source, signature):
    at=source.index(signature); start=source.index("{",at); depth=1; end=start+1
    while depth:
        depth += (source[end]=="{")-(source[end]=="}"); end+=1
    return source[at:end]
def main():
    source=(ROOT/"src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8")
    globals=source[source.index("struct ThrowNoticeProp {"):source.index("static void ThrowNoticeHudPump();")]
    packet=source[source.index("struct RecvProxyDataPod {"):source.index("// Recv thread: original proxy first")]
    fixture=r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <intrin.h>
namespace skmem { static unsigned faults=0; static void NoteFault(uintptr_t) { ++faults; } }
struct ClientClass {};
using RecvVarProxyFn=void(*)(const void*,void*,void*);
'''+globals+packet+function(source,"static void ThrowNoticeRecv(const void* data,")+"\n"+function(source,"static void ThrowNoticePatch(bool wanted) {")+r'''
static unsigned nativeCalls=0;
static void Native(const void* data,void*,void* out){++nativeCalls;int wire=static_cast<const RecvProxyDataPod*>(data)->valueInt;*static_cast<int*>(out)=wire==-1?-1:wire+0x10000;}
static void Foreign(const void*,void*,void*){}
static void require(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
int main(){
    alignas(void*) uint8_t prop[64]{};
    auto slot=reinterpret_cast<RecvVarProxyFn*>(prop+0x20);*slot=Native;
    g_throwNoticeProps[0]={prop,Native};g_throwNoticePropCount=1;
    RecvProxyDataPod data{};data.recvProp=prop;data.valueInt=(7<<12)|2;data.objectId=50;int out=0;
    ThrowNoticeRecv(&data,nullptr,&out);
    require(nativeCalls==1&&out==data.valueInt+0x10000&&g_throwNoticeQueued==0,"off preserves native output and publishes nothing");
    ThrowNoticePatch(true);require(*slot==ThrowNoticeRecv,"on installs the exact receive property");g_throwNoticeWanted=true;
    data.valueInt=-1;ThrowNoticeRecv(&data,nullptr,&out);require(g_throwNoticeQueued==0,"invalid thrower does not publish");
    data.valueInt=(7<<12)|2;
    for(int i=0;i<33;++i)ThrowNoticeRecv(&data,nullptr,&out);
    require(g_throwNoticeQueued==32&&nativeCalls==35,"queue is bounded and native always runs");
    require(g_throwNoticeQueue[0].index==50&&g_throwNoticeQueue[0].thrower==data.valueInt+0x10000,"native-decoded serial-bearing thrower is retained");
    AcquireSRWLockExclusive(&g_throwNoticeLock);ThrowNoticeRecv(&data,nullptr,&out);ReleaseSRWLockExclusive(&g_throwNoticeLock);
    require(g_throwNoticeQueued==32&&nativeCalls==36,"busy producer drops rather than blocking receive thread");
    ThrowNoticePatch(false);require(*slot==Native&&!g_throwNoticeWanted,"off restores native proxy and disables publication");
    auto count=g_throwNoticeQueued;ThrowNoticeRecv(&data,nullptr,&out);require(g_throwNoticeQueued==count,"no publication after off");
    ThrowNoticePatch(true);*slot=Foreign;ThrowNoticePatch(false);
    require(*slot==Foreign&&skmem::faults==1,"foreign proxy is preserved and conflict is observable");
    std::puts("PASS throwable receive: native passthrough, bounds, invalid owner, nonblocking queue, disable, foreign ownership");
}
'''
    fixture=fixture.replace("int main(){", "static void ThrowTests(){")
    laser_globals=source[source.index("static std::atomic<int> g_laserRoundOp"):source.index("static void LaserRoundHudResetPos();")]
    fixture+=r'''
static DWORD fakeNow=10000;static unsigned checks=0;static int liveIndex=0;
static void* g_engine=reinterpret_cast<void*>(1);static void* g_entlist=reinterpret_cast<void*>(1);
static std::atomic<bool> g_run{true},g_paintInGame{false},g_paintLoading{false};
static bool g_optPauseSampling=false;static int g_offModelIndex=320,g_offEffects=224,g_offOwnerEntity=312;
static bool SkeetoFeaturesOn(){return true;}
static int EntHighestIndex(){return 256;}
static bool LaserRoundLiveSight(int index){++checks;return index==liveIndex;}
#define GetTickCount() fakeNow
static int LaserRoundModelKey(){return 274;}
'''+laser_globals+function(source,"static void LaserRoundModelRecv(const void* data,")+"\n"+function(source,"static void LaserRoundPatch(bool wanted) {")+"\n"+function(source,"static void LaserRoundApplyCommand() {")+"\n"+function(source,"static void LaserRoundNoticeTick() {")
    fixture+=r'''
static ClientClass upgradeClass{},otherClass{};static ClientClass* candidateClass=&upgradeClass;
static int candidateModel=274,candidateEffects=0;static unsigned candidateOwner=~0u;
static bool candidateExists=true,candidateReadable=true;static unsigned candidateReads=0;
static constexpr int kEfNoDraw=32;
static ClientClass* EntClientClass(int){return candidateClass;}
static void* EntGet(int){return candidateExists?&candidateModel:nullptr;}
template<class T> static bool EntRead(void*,int offset,T* out){
    ++candidateReads;if(!candidateReadable)return false;
    *out=static_cast<T>(offset==g_offModelIndex?candidateModel:offset==g_offEffects?candidateEffects:candidateOwner);return true;
}
'''+function(source,"static bool LaserRoundLiveSight(int index) {").replace("LaserRoundLiveSight(","ProductionLaserRoundLiveSight(")
    fixture+=r'''
static void CandidateTests(){
    g_laserUpgradeClass=&upgradeClass;g_laserModelKey=274;
    require(ProductionLaserRoundLiveSight(53),"upgrade class and chapter model key confirm laser");
    candidateClass=&otherClass;candidateReads=0;require(!ProductionLaserRoundLiveSight(53)&&candidateReads==0,"other classes skip entity fields");
    candidateClass=&upgradeClass;candidateModel=275;require(!ProductionLaserRoundLiveSight(53),"another upgrade model cannot trigger");
    candidateModel=274;candidateEffects=kEfNoDraw;require(!ProductionLaserRoundLiveSight(53),"hidden laser cannot trigger");
    candidateEffects=0;candidateOwner=2;require(!ProductionLaserRoundLiveSight(53),"owned laser cannot trigger");
    candidateOwner=0;require(ProductionLaserRoundLiveSight(53),"unowned zero handle is valid");
    candidateExists=false;require(!ProductionLaserRoundLiveSight(53),"removed entity is skipped");
    candidateExists=true;candidateReadable=false;require(!ProductionLaserRoundLiveSight(53),"failed boundary read is skipped");candidateReadable=true;
    std::puts("PASS laser candidates: exact class/model, hidden, owned, removed and unreadable");
}
'''
    fixture+=r'''
static int g_siPresenceTotal=4,g_siPresenceSmokers=2;static bool g_siPresenceHaveSample=true;
static DWORD g_siPresenceHudSampleAt=10000;static bool g_hudPostMissionQuiet=false;
static bool EngInGame(){return g_paintInGame.load();}static bool EngDrawingLoading(){return g_paintLoading.load();}
struct PrField {int base;};
static PrField g_tbPrTeam{0},g_tbPrAlive{1},g_tbPrZc{2},g_tbPrGhost{3},g_tbPrConnected{4};
static int slots[33][5]{};
static void TabPrResolveOffsets(){}
static void* TabPrEntity(){return slots;}
static bool TabPrRead(void*,PrField field,int slot,int* out){*out=slots[slot][field.base];return true;}
'''+function(source,"static void SiPresenceHudResetSample() {")
    sample=function(source,"static void SiPresenceHudSampleImpl() {")
    # Exercise the production PlayerResource branch, including disconnected slots.
    fixture+=sample[:sample.index("\t// Fallback: entity slots")]+"}\n"
    fixture+=r'''
static void NativeModel(const void* data,void*,void* out){*static_cast<int*>(out)=static_cast<const RecvProxyDataPod*>(data)->valueInt;}
int main(){
    ThrowTests();
    CandidateTests();
    alignas(void*) uint8_t prop[64]{};auto slot=reinterpret_cast<RecvVarProxyFn*>(prop+0x20);*slot=NativeModel;
    g_laserModelProp=prop;g_laserModelOriginal=NativeModel;
    g_optLaserRoundNotice=true;g_paintInGame=true;
    LaserRoundNoticeTick();require(g_laserRoundPhase==1&&g_laserRoundGen==1,"first usable Paint arms without round_start");
    require(*slot==LaserRoundModelRecv,"model receive hook installed");
    for(int i=0;i<40&&g_laserRoundPhase==1;++i){fakeNow+=400;unsigned before=checks;LaserRoundNoticeTick();require(checks-before<=128,"per-Paint baseline scan is capped");}
    require(g_laserRoundPhase==2&&checks==256,"one complete baseline sweep then passive wait");
    auto before=checks;for(int i=0;i<100;++i){fakeNow+=1600;LaserRoundNoticeTick();}
    require(checks==before,"no repeated table scan while no laser is received");
    RecvProxyDataPod data{};int out=0;data.objectId=100;data.valueInt=273;
    LaserRoundModelRecv(&data,nullptr,&out);LaserRoundNoticeTick();require(checks==before&&out==273,"unrelated model is native passthrough without candidate checks");
    liveIndex=100;
    data.valueInt=274;LaserRoundModelRecv(&data,nullptr,&out);LaserRoundNoticeTick();
    require(g_laserRoundShowUntil&&g_laserRoundPhase==0&&*slot==NativeModel,"late model notification shows once and restores hook");
    before=checks;fakeNow+=11000;LaserRoundNoticeTick();LaserRoundNoticeTick();
    require(checks==before&&!g_laserRoundShowUntil,"one notice per chapter, no rescanning after success");
    LaserRoundPost(2);LaserRoundNoticeTick();require(g_laserRoundPhase==0,"round end stops scanning");
    LaserRoundPost(1);LaserRoundNoticeTick();require(g_laserRoundPhase==1,"new round rearms scanning");
    g_paintLoading=true;LaserRoundNoticeTick();require(!g_laserRoundInGame&&!g_laserRoundPhase,"loading drops old chapter state");
    g_paintLoading=false;LaserRoundNoticeTick();require(g_laserRoundInGame&&g_laserRoundPhase==1,"reconnect rearms without an event");
    liveIndex=0;g_laserRoundPhase=2;fakeNow+=400;
    for(int i=1;i<=160;++i){data.objectId=i;LaserRoundModelRecv(&data,nullptr,&out);}
    for(int i=0;i<7;++i){before=checks;LaserRoundNoticeTick();require(checks-before<=32,"network candidate flood is capped at32 per Paint");}
    require(!g_laserPendingAny,"bounded notification flood drains without losing its tail");
    g_optLaserRoundNotice=false;before=checks;LaserRoundNoticeTick();require(!g_laserRoundShowUntil&&checks==before,"off has no scan or visible residue");
    require(*slot==NativeModel&&!g_laserModelWanted,"off restores model receive proxy");
    const unsigned faultsBeforeLaser=skmem::faults;
    LaserRoundPatch(true);*slot=Foreign;LaserRoundPatch(false);require(*slot==Foreign&&skmem::faults==faultsBeforeLaser+1,"foreign model proxy is preserved");
    LaserRoundPatch(false);require(skmem::faults==faultsBeforeLaser+1&&!g_laserModelWanted,"lost ownership cannot retry restoration every Paint");
    *slot=NativeModel;g_optLaserRoundNotice=true;g_laserRoundInGame=true;fakeNow=0xfffffff0u;
    LaserRoundPost(1);LaserRoundNoticeTick();g_laserRoundPhase=2;g_laserRoundArmAt=fakeNow-300u;liveIndex=100;data.objectId=100;
    LaserRoundModelRecv(&data,nullptr,&out);LaserRoundNoticeTick();require(g_laserRoundShowUntil,"notice arms across tick rollover");
    fakeNow+=5000u;LaserRoundNoticeTick();require(g_laserRoundShowUntil,"notice survives rollover before deadline");
    fakeNow+=5001u;LaserRoundNoticeTick();require(!g_laserRoundShowUntil,"notice expires correctly after rollover");
    g_optLaserRoundNotice=false;LaserRoundNoticeTick();
    g_hudPostMissionQuiet=true;g_siPresenceHudSampleAt=fakeNow;
    SiPresenceHudSampleImpl();require(!g_siPresenceTotal&&!g_siPresenceSmokers&&!g_siPresenceHaveSample&&!g_siPresenceHudSampleAt,"wipe clears both counts before the sampling throttle");
    g_siPresenceTotal=7;SiPresenceHudSampleImpl();require(!g_siPresenceTotal,"quiet cannot refill stale resource data");
    g_hudPostMissionQuiet=false;SiPresenceHudSampleImpl();require(g_siPresenceHudSampleAt==fakeNow,"next round resumes sampling");
    for(int i=5;i<=7;++i){slots[i][0]=3;slots[i][1]=1;slots[i][2]=2;slots[i][4]=1;}
    fakeNow+=500;SiPresenceHudSampleImpl();require(g_siPresenceTotal==3&&g_siPresenceHaveSample,"three connected live Boomers count");
    for(int i=5;i<=7;++i)slots[i][4]=0;
    fakeNow+=500;SiPresenceHudSampleImpl();require(g_siPresenceTotal==0&&g_siPresenceHaveSample,"restart stale alive/class fields cannot count disconnected slots");
    slots[6][4]=1;slots[6][2]=1;fakeNow+=500;SiPresenceHudSampleImpl();require(g_siPresenceTotal==1&&g_siPresenceSmokers==1,"reconnected slot resumes counting");
    g_paintLoading=true;g_siPresenceTotal=5;SiPresenceHudSampleImpl();require(!g_siPresenceTotal,"loading clears count residue");
    std::puts("PASS notice lifecycle: single baseline, passive late laser, bounded flood, close, stale disconnected SI and recount");
}
'''
    vswhere=Path(os.environ["ProgramFiles(x86)"])/"Microsoft Visual Studio/Installer/vswhere.exe"
    vs=subprocess.check_output([str(vswhere),"-latest","-products","*","-requires","Microsoft.VisualStudio.Component.VC.Tools.x86.x64","-property","installationPath"],text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-throw-notice-") as tmp:
        folder=Path(tmp);(folder/"fixture.cpp").write_text(fixture,encoding="utf-8")
        (folder/"run.bat").write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 /W3 /WX "'+str(folder/"fixture.cpp")+'" /Fo"'+str(folder/"fixture.obj")+'" /Fe"'+str(folder/"fixture.exe")+'"\nif errorlevel 1 exit /b 1\n"'+str(folder/"fixture.exe")+'"\nexit /b %errorlevel%\n',encoding="utf-8")
        subprocess.run(["cmd","/c",str(folder/"run.bat")],cwd=ROOT,check=True)
if __name__=="__main__":main()
