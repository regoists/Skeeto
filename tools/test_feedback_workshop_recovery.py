"""Exercise real catalog recovery with delayed GAME mounts and a fake Paint clock."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
from test_chapstats_pages import block

ROOT = Path(__file__).resolve().parents[1]


def main():
    core = ROOT / "src/skeeto/core"
    style = (core / "skeeto_style.cpp").read_text(encoding="utf-8-sig")
    proxy = (core / "skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    prefix = r'''
#include <Windows.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <new>
#include <string>
#include <vector>
static DWORD fixtureNow=1000;
#define GetTickCount() fixtureNow
struct File { std::string name,json; };
static std::vector<File> files;
static int firsts=0,nexts=0,closes=0,reads=0,fullScans=0,nativeReads=0,precaches=0;
static int mountState=1,cursor=0;
static bool fsAvailable=true,features=true;
struct IFileSystem {
 const char* FindFirstEx(const char*,const char*,int* handle){++firsts;cursor=0;*handle=1;return files.empty()?nullptr:files[0].name.c_str();}
 const char* FindNext(int){++nexts;++cursor;return cursor<(int)files.size()?files[cursor].name.c_str():nullptr;}
 bool FindIsDirectory(int){return false;}
 void FindClose(int){++closes;}
};
using FileFindHandle_t=int;
static IFileSystem fixtureFs;
static IFileSystem* FsGet(){return fsAvailable?&fixtureFs:nullptr;}
static bool FsReadGAME(const char* name,char** buf,int* len,unsigned int maxBytes=8u*1024u*1024u){
 ++reads;for(const auto& f:files)if(std::string("skeeto/")+f.name==name){
  if(f.json.size()>maxBytes)return false;*len=(int)f.json.size();*buf=(char*)malloc(*len+1);memcpy(*buf,f.json.c_str(),*len+1);return true;
 }return false;
}
static DlcCatalog g_cat{};
static int g_styleLimit=64,g_lastEngineJson=0,g_lastLooseJson=0,g_lastReloadMs=0;
static bool g_styleLimitPending=false,g_catalogLoaded=false,g_engineGlobTried=false;
static DWORD g_engineGlobRetryAt=0;
static void AddStyleFromJson(const char*);
static void ScanLooseDir(const char*){}
static int ScanEngineGlob(const char*,const char*){++fullScans;for(const auto& f:files)AddStyleFromJson(f.json.c_str());return (int)files.size();}
static void* g_engine=(void*)1;
static char g_gameL4d2Dir[]="game";
static std::atomic_bool g_paintConnected{true},g_paintInGame{true},g_paintLoading{false};
static std::atomic_bool g_mapEnterGameThreadResetPending{false};
static std::atomic_bool g_mapEnterCvarReapplyPending{false};
static bool g_listening=true,g_mapRoundStartSeen=true;
static void* g_events=(void*)1;
static void* g_listenerManagerAtRegistration=(void*)1;
static void ClearFeedbackQueuesOnMapBoundary(){}
static void ResetHealthCache(){}
static void EnsureListeners(){}
static void HudPostMissionQuietClear(const char*){}
static void ClientUxApplyMapStableCvars(){}
static bool SkeetoFeaturesOn(){return features;}
static int NativeAddonsLoadState(){++nativeReads;return mountState;}
static void PrecacheSelectedParticles(const char*){++precaches;}
static void Log(const char*,...){}
'''
    parser = style[style.index("static const char* SkipWs("):style.index("static IFileSystem* FsGet()")]
    functions = "\n".join(block(style, p) for p in [
        r"^int DlcStyleLimit\(", r"^bool DlcStyleLimitPending\(", r"^void DlcSetStyleLimit\(",
        r"^static void AddStyleFromJson\(const char\* json\) \{", r"^static int CmpStyle\(",
        r"^static bool LooksLikeStyleJsonName\(",
        r"^void DlcReloadCatalog\(", r"^bool DlcEnsureCatalog\(", r"^bool DlcEngineGlobTried\(",
        r"^bool DlcEngineCatalogReady\(", r"^int DlcLastEngineJsonCount\(", r"^int DlcLastLooseJsonCount\(",
        r"^int DlcLastReloadMs\(", r"^const DlcCatalog\* DlcGetCatalog\(",
        r"^const DlcStyle\* DlcFindStyle\(", r"^int DlcCountChannel\(", r"^const char\* DlcChannelIdAt\(",
        r"^void DlcSetSelected\(", r"^const char\* DlcGetSelected\(", r"^int DlcIndexOfSelected\(",
        r"^const char\* DlcCycleChannel\(",
    ])
    scan = style[style.index("struct DlcGameGlobScan {"):style.index("struct DlcCacheExtract {")]
    state = proxy[proxy.index("static bool g_feedbackCatalogRecoveryDone"):proxy.index("static bool VtProtectWrite(", proxy.index("static bool g_feedbackCatalogRecoveryDone"))]
    # Keep the scheduler isolated from unrelated hooks declared after it.
    assert "static void FeedbackCatalogPump()" in state
    setting = re.search(r"static bool g_optFeedbackWorkshopRecovery = .*?;", proxy).group()
    assert proxy.count("g_optFeedbackWorkshopRecovery = false;") == 2
    assert 'fprintf(f, "feedback_workshop_recovery=%d\\n", g_optFeedbackWorkshopRecovery ? 1 : 0);' in proxy
    assert 'sscanf(line, "feedback_workshop_recovery=%d", &v)' in proxy
    map_reset = block(proxy, r"^static void PumpGameThreadMapEnterReset\(\) \{")
    suffix = r'''
static File Json(const char* id){return {std::string("skeeto_")+id+".json",std::string("{\"id\":\"")+id+"\",\"name\":\""+id+"\",\"channel\":\"ci\",\"kill\":{\"sound\":\"test.wav\"}}"};}
static void Frame(DWORD delta=16){fixtureNow+=delta;PumpGameThreadMapEnterReset();FeedbackCatalogPump();}
static void Finish(){for(int i=0;i<5000&&!g_feedbackCatalogRecoveryDone;++i)Frame();assert(g_feedbackCatalogRecoveryDone);}
int main(int argc,char** argv){
 _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
 int test=argc>1?atoi(argv[1]):0;
 DlcSetSelected("ci","wanted");
 if(test==0){
  g_paintConnected=false;g_paintInGame=false;files={Json("other")};
  for(int i=0;i<100000;++i)Frame();
  assert(nativeReads==0&&firsts==0&&reads==0&&fullScans==1);
  assert(!strcmp(DlcGetSelected("ci"),"wanted"));return 0;
 }
 g_optFeedbackWorkshopRecovery=true;
 if(test==1||test==2){
  if(test==2){files={Json("other")};DlcReloadCatalog("game",true);}
  for(int i=0;i<1000;++i)Frame(); // Workshop remains late beyond old 1.5-second window.
  assert(firsts==0&&reads==0&&nativeReads>0&&nativeReads<35);
  files.push_back(Json("wanted"));mountState=0;Frame(500);
  assert(firsts==0&&reads==0);Frame();assert(firsts==0);Frame(500);assert(firsts==1&&reads==0);Finish();assert(DlcFindStyle("wanted"));
  assert(!strcmp(DlcGetSelected("ci"),"wanted")&&precaches==1);
  int n=nativeReads,r=reads;for(int i=0;i<100000;++i)Frame();
  assert(nativeReads==n&&reads==r&&firsts==1&&closes==1);return 0;
 }
 if(test==3||test==8||test==9||test==10){
  mountState=0;files={Json("other"),Json("wanted")};g_paintLoading=true;Frame();assert(!nativeReads&&!firsts);
  g_paintLoading=false;Frame();Frame(500);assert(firsts==1);Frame();assert(reads==1);
  if(test==3)g_optFeedbackWorkshopRecovery=false;
  if(test==8)features=false;
  if(test==9)g_paintLoading=true;
  if(test==10)mountState=1;
  Frame();assert(closes==1&&!g_feedbackCatalogRecoveryRunning);
  int r=reads,n=nativeReads;for(int i=0;i<1000;++i)Frame();
  assert(reads==r);if(test==10)assert(nativeReads<=n+33);else assert(nativeReads==n);
  g_optFeedbackWorkshopRecovery=true;features=true;g_paintLoading=false;mountState=0;
  Frame(500);Finish();assert(DlcFindStyle("wanted")&&precaches==1);return 0;
 }
 if(test==4){
  mountState=-1;fixtureNow=0xffffffe0u;Frame(0);int n=nativeReads;
  Frame(100);assert(nativeReads==n&&!firsts);Frame(500);assert(nativeReads==n+1);
  mountState=0;fsAvailable=false;Frame(500);assert(!firsts&&!g_feedbackCatalogRecoveryDone);
  Frame();assert(!firsts&&!g_feedbackCatalogRecoveryDone);
  fsAvailable=true;files={Json("wanted")};Frame(500);Finish();assert(DlcFindStyle("wanted"));return 0;
 }
 if(test==5){
  mountState=0;files={Json("wanted")};DlcReloadCatalog("game",true);DlcSetStyleLimit(96);Frame();assert(!firsts&&DlcStyleLimitPending());
  g_paintInGame=false;g_paintConnected=false;Finish();assert(firsts==1&&!DlcStyleLimitPending()&&!precaches);return 0;
 }
 if(test==6){
  mountState=0;files={{"skeeto_bad.json","{\"name\":\"bad\"}"}};Finish();
  assert(!DlcFindStyle("wanted")&&!strcmp(DlcGetSelected("ci"),"wanted"));
  assert(!strcmp(DlcCycleChannel("ci"),"wanted")&&!strcmp(DlcGetSelected("ci"),"wanted"));return 0;
 }
 if(test==11){
  mountState=0;Frame();assert(firsts==0); // Native completion queued the path update.
  Frame();assert(!firsts);files={Json("wanted")};Frame(500);Finish();assert(DlcFindStyle("wanted")&&firsts==1);return 0;
 }
 if(test==12){
  mountState=0;files={Json("other"),Json("wanted")};DlcReloadCatalog("game",true);DlcSetStyleLimit(96);
  g_paintInGame=false;g_paintConnected=false;Frame();Frame(500);assert(firsts==1);
  g_optFeedbackWorkshopRecovery=false;Frame();
  assert(!DlcStyleLimitPending()&&fullScans==2&&DlcFindStyle("wanted"));return 0;
 }
 if(test==13){
  mountState=0;files={Json("other"),Json("wanted")};Frame();Frame(500);Frame();
  assert(g_feedbackCatalogRecoveryRunning && reads==1 && firsts==1);
  const int r=reads;g_mapEnterGameThreadResetPending=true;Frame(); // No loading Paint before this map boundary.
  assert(!g_mapEnterGameThreadResetPending && closes==1 && reads==r && !g_feedbackCatalogRecoveryRunning);
  Frame();assert(firsts==1 && reads==r);Frame(500);assert(firsts==2 && reads==r);
  Finish();assert(DlcFindStyle("wanted") && closes==2 && precaches==1);return 0;
 }
 if(test==14){
  mountState=0;files={Json("wanted")};Finish();const int n=nativeReads;
  g_mapEnterGameThreadResetPending=true;Frame(500);
  assert(g_feedbackCatalogRecoveryDone && firsts==1 && nativeReads==n && precaches==1);return 0;
 }
 if(test==15){
  mountState=0;files={Json("wanted")};Frame();assert(!firsts);
  g_mapEnterGameThreadResetPending=true;Frame(500);
  assert(!firsts && !g_feedbackCatalogRecoveryRunning);Frame(500);assert(firsts==1);Finish();return 0;
 }
 mountState=0;for(int i=0;i<4100;++i)files.push_back({"skeeto_bad.json","{}"});Finish();
 assert(reads==4096&&closes==1&&precaches==0);int r=reads;Frame(1000);assert(reads==r);
 puts("bounded malformed catalog retained selection and stopped");
}
'''
    source = '#define _CRT_SECURE_NO_WARNINGS\n#include "' + (core / "skeeto_style.h").as_posix() + '"\n' + prefix + parser + functions + scan + setting + state + map_reset + suffix
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-feedback-recovery-") as folder:
        work = Path(folder)
        (work / "fixture.cpp").write_text(source, encoding="utf-8")
        (work / "compile.bat").write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvars32.bat" >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /W3 /WX fixture.cpp /Fe:fixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "compile.bat"], cwd=work, check=True)
        for case in range(16):
            print("case", case, flush=True)
            subprocess.run([str(work / "fixture.exe"), str(case)], cwd=work, check=True, timeout=25)
    print("PASS feedback workshop recovery: disabled, late/partial mounts, cancellation, rollover, limits and saved IDs")


if __name__ == "__main__":
    main()
