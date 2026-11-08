"""Native production classifier and fixed-track MF/waveOut lifecycle tests.

Audio test uses a temporary silent-output fixture, not the user's game files.
Source extraction is mocked; Windows decoding/output and the full module are real.
"""
import ast
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from test_chapstats_pages import ROOT, block


def compile_run(source, folder, media=False):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                  'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    (folder / 'fixture.cpp').write_text(source, encoding='utf-8')
    libs = '/link windowscodecs.lib mfplat.lib mfreadwrite.lib mfuuid.lib propsys.lib winmm.lib ole32.lib shell32.lib' if media else ''
    (folder / 'run.bat').write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                                 f'cl /nologo /EHsc /std:c++17 /utf-8 /D_CRT_SECURE_NO_WARNINGS /I"{ROOT / "src/skeeto/core"}" fixture.cpp /Fe:fixture.exe {libs}\n'
                                 'if errorlevel 1 exit /b 1\nfixture.exe "%CD%"\nexit /b %errorlevel%\n', encoding='utf-8')
    subprocess.run(['cmd', '/c', 'run.bat'], cwd=folder, check=True, timeout=60)


def classifier():
    cpp = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    source = r'''
#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "skeeto_memread.h"
struct Pawn {int team;uint8_t life,incap,hanging;};
struct TabPrArr{int base;int stride;};static TabPrArr g_tbPrTeam{-1,0},g_tbPrAlive{-1,0},g_tbPrIncap{-1,0};
static void TabPrResolveOffsets(){}static void* TabPrEntity(){return nullptr;}
static bool TabPrRead(void*,const TabPrArr&,int,int*){return false;}static bool EntReadable(void*){return true;}
static Pawn players[33]{};static void* raw[33]{};static bool present[33]{};static int local=1;
static int g_offTeam=0,g_offLifeState=4,g_offIsIncapacitated=5,g_offHanging=6;
struct PlayerInfo{char name[8];};
static int EngLocal(){return local;}
static void* EntGet(int i){return present[i]?(raw[i]?raw[i]:&players[i]):nullptr;}
static bool EngPlayerInfo(int i,PlayerInfo* info){info->name[0]='x';return present[i];}
'''
    source += block(cpp, r'^static bool LastMusicReadField\(')
    source += block(cpp, r'^static bool LastMusicShouldPlay\(') + block(cpp, r'^static bool LastMusicRoster\(')
    source += r'''
int main(){int cases=0;
 for(int total=0;total<=32;++total)for(int alive=0;alive<=total;++alive)for(int up=0;up<=alive;++up)
 for(int self=0;self<3;++self)for(int failed=0;failed<2;++failed)for(int mode=0;mode<5;++mode){
  bool a=self>=1&&alive>0,s=self==2&&up>0;
  bool expected=total>=2&&(mode==0?(failed||alive==0):mode==1?alive==1:mode==2?up==1:mode==3?alive==1&&a:up==1&&s);
  assert(LastMusicShouldPlay(mode,total,alive,up,a,s,failed)==expected);++cases;
 }
 int total,alive,up;bool sa,su;
 for(int i=1;i<=4;++i){present[i]=true;players[i]={2,0,0,0};}
 assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&total==4&&alive==4&&up==4&&sa&&su);
 players[2].incap=1;players[3].hanging=1;players[4].life=2;
 assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&total==4&&alive==3&&up==1&&sa&&su);
 local=2;assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&sa&&!su);
 local=4;assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&!sa&&!su);
 players[4].team=1;players[3].team=3;assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&total==2&&alive==2&&up==1&&!sa);
 g_offHanging=-1;assert(!LastMusicRoster(&total,&alive,&up,&sa,&su));
 // Match the reported offsets and partial 2/1/1 snapshot. Entity three
 // crosses readable VirtualQuery regions with different protections.
 SYSTEM_INFO sys{};GetSystemInfo(&sys);const size_t page=sys.dwPageSize;
 assert(page>=4096);const size_t stride=page*4;
 auto memory=(uint8_t*)VirtualAlloc(nullptr,stride*4,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(memory);
 g_offTeam=228;g_offLifeState=327;g_offIsIncapacitated=7849;g_offHanging=9708;local=1;
 for(int i=1;i<=4;++i){raw[i]=memory+(i-1)*stride;auto b=(uint8_t*)raw[i];
  *(int*)(b+g_offTeam)=2;b[g_offLifeState]=i==1?0:2;b[g_offIsIncapacitated]=0;b[g_offHanging]=0;}
 DWORD old=0;auto split=(uint8_t*)raw[3]+page;
 assert(VirtualProtect(split,page,PAGE_READONLY,&old));
 assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&total==4&&alive==1&&up==1&&sa&&su);
 assert(LastMusicShouldPlay(2,total,alive,up,sa,su,false));
 assert(VirtualProtect(split,page,PAGE_READWRITE,&old));
 for(int i=1;i<=4;++i){auto b=(uint8_t*)raw[i];b[g_offLifeState]=0;b[g_offIsIncapacitated]=i==1?0:1;}
 assert(VirtualProtect(split,page,PAGE_READONLY,&old));
 assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&total==4&&alive==4&&up==1);
 assert(LastMusicShouldPlay(2,total,alive,up,sa,su,false));
 for(int i=1;i<=4;++i)((uint8_t*)raw[i])[g_offLifeState]=2;
 assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&total==4&&alive==0&&up==0);
 assert(LastMusicShouldPlay(0,total,alive,up,sa,su,false));
 ((uint8_t*)raw[1])[g_offLifeState]=0;
 // A truly unreadable survivor field still invalidates the full sample.
 assert(VirtualProtect(split,page,PAGE_NOACCESS,&old));
 assert(!LastMusicRoster(&total,&alive,&up,&sa,&su));
 // A spectator needs only its team field, not survivor-only offsets.
 *(int*)((uint8_t*)raw[3]+g_offTeam)=1;
 assert(LastMusicRoster(&total,&alive,&up,&sa,&su)&&total==3&&alive==1&&up==1);
 assert(VirtualProtect(split,page,PAGE_READWRITE,&old));
 // A four-byte team value may itself straddle two readable regions.
 int teamValue=2,readTeam=0;memcpy(memory+page-2,&teamValue,sizeof(teamValue));
 assert(VirtualProtect(memory+page,page,PAGE_READONLY,&old));
 assert(LastMusicReadField(memory,(int)page-2,&readTeam,sizeof(readTeam))&&readTeam==2);
 assert(VirtualProtect(memory+page,page,PAGE_READWRITE|PAGE_GUARD,&old));
 assert(!LastMusicReadField(memory,(int)page-2,&readTeam,sizeof(readTeam)));
 assert(VirtualProtect(memory+page,page,PAGE_READWRITE,&old));
 assert(!LastMusicReadField(memory,-1,&readTeam,sizeof(readTeam)));
 assert(!LastMusicReadField(memory,0,&readTeam,sizeof(readTeam)+1));
 assert(!LastMusicReadField((void*)(UINTPTR_MAX-1),4,&readTeam,sizeof(readTeam)));
 assert(VirtualFree(memory,0,MEM_RELEASE));
 assert(!LastMusicShouldPlay(5,4,1,1,true,true,false));assert(!LastMusicShouldPlay(2,4,1,2,true,true,false));
 printf("PASS: %d five-mode cases; roster filters, reported offsets across readable regions, spectator filtering, split-field reads, unreadable/guard/overflow rejection\n",cases);
}
'''
    with tempfile.TemporaryDirectory(prefix='skeeto-last-count-') as temp:
        compile_run(source, Path(temp))


def paint_gates():
    cpp = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    source = r'''
#include <Windows.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <initializer_list>
static bool g_optLastMusic=false,g_menuVisible=false,g_menuParked=false,features=true,g_lastMusicAutoStop=true,g_lastMusicNotice=true;
static int g_menuPage=0,kMenuPageLastMusic=104,g_lastMusicVolume=100,g_lastMusicMode=2,g_lastMusicAdjustSel=0;
static DWORD g_lastMusicNoticeBorn=0,g_lastMusicNoticeUntil=0;static unsigned publishedStart=0;
static std::atomic_bool g_ready{true},g_paintInGame{true},g_paintLoading{false},g_lastMusicRoundFailed{false},g_lastMusicReset{false},g_lastMusicEventWanted{false};
static DWORD tick=10000;static int configures=0,pumps=0,rosters=0,starts=0,upCount=1,totalCount=4,aliveCount=3;static bool rosterValid=true,enabled=false,active=false,allow=false;
static DWORD TestTick(){return tick;}
#define GetTickCount TestTick
static bool SkeetoFeaturesOn(){return features;}
static void SkeetoLastMusicConfigure(bool e,int v){++configures;enabled=e;assert(v==100);}
static void SkeetoLastMusicSetActive(bool a){active=a;if(a)++starts;else publishedStart=0;}
static void SkeetoLastMusicResourcePump(bool a){++pumps;allow=a;}
static int SkeetoLastMusicState(){return 1;}
static unsigned SkeetoLastMusicStartedGeneration(){return active?publishedStart:0;}
static void MenuMarkDirty(){}
static bool LastMusicRoster(int* t,int* a,int* s,bool* sa,bool* ss){++rosters;*t=totalCount;*a=aliveCount;*s=upCount;*sa=aliveCount>0;*ss=upCount>0;return rosterValid;}
'''
    pump = cpp[cpp.rindex('static void LastMusicPaintPump() {'):]
    source += block(cpp, r'^static bool LastMusicShouldPlay\(') + block(cpp[cpp.rindex('static bool LastMusicNoticeWanted() {'):], r'^static bool LastMusicNoticeWanted\(') + block(pump, r'^static void LastMusicPaintPump\(')
    level = block(cpp, r'^static void __fastcall Hooked_LevelInitPreEntity\(')
    source += r'''
static std::atomic_bool g_run{true};
static void (*g_origLevelInitPre)(void*,void*,const char*)=nullptr;
static void SurvivorGlowUnhook(){}
static void ListenOnLevelInit(){}
namespace skvpkexp{struct Controller{unsigned enabled=0;void SetMap(const char*){}};static Controller g_controller;}
namespace skload{static std::atomic<DWORD> g_ownerThread{0};}
static std::atomic<unsigned> g_campaignMapEpoch{0};static std::atomic_bool g_tpsAimResetRequested{false};
static void MainUiConsumeExpertPending(){}static void MainUiQuickTestLoadoutLevelInit(const char*){}
#define GetModuleHandleA(name) ((HMODULE)1)
'''
    source += level.split('\tCrashMark(')[0] + '}\n'
    source += r'''
int main(){
 LastMusicPaintPump();assert(configures==0&&pumps==0&&rosters==0);
 g_optLastMusic=true;LastMusicPaintPump();assert(enabled&&!active&&rosters==0);
 tick+=500;LastMusicPaintPump();assert(active&&rosters==1&&g_lastMusicEventWanted);
 assert(!LastMusicNoticeWanted());publishedStart=1;++tick;LastMusicPaintPump();assert(LastMusicNoticeWanted()&&g_lastMusicNoticeUntil==tick+5000);
 const DWORD noticeEnd=g_lastMusicNoticeUntil;tick+=998;LastMusicPaintPump();assert(rosters==1&&g_lastMusicNoticeUntil==noticeEnd);++tick;LastMusicPaintPump();assert(rosters==2&&starts==1);
 upCount=2;tick+=1000;LastMusicPaintPump();assert(!active);
 upCount=1;tick+=1000;LastMusicPaintPump();assert(active&&starts==2);
 g_lastMusicAutoStop=false;upCount=2;tick+=1000;LastMusicPaintPump();assert(active);
 g_lastMusicAutoStop=true,g_lastMusicNotice=true;tick+=1000;LastMusicPaintPump();assert(!active);
 upCount=1;tick+=1000;LastMusicPaintPump();assert(active&&starts==3);rosters=2;
 assert(g_lastMusicNoticeUntil==0);g_paintLoading=true;tick+=250;LastMusicPaintPump();assert(!active&&!allow&&rosters==2);
 g_paintLoading=false;tick+=1000;LastMusicPaintPump();assert(active&&rosters==3);
 g_lastMusicReset=true;tick+=250;LastMusicPaintPump();assert(!active&&rosters==3);
 tick+=999;LastMusicPaintPump();assert(rosters==3);++tick;LastMusicPaintPump();assert(active&&rosters==4);
 g_optLastMusic=false;LastMusicPaintPump();assert(!enabled&&!active&&!g_lastMusicEventWanted);
 const int stopped=configures;for(int i=0;i<100;++i){tick+=1000;LastMusicPaintPump();}assert(configures==stopped&&rosters==4);
 g_menuVisible=true;g_menuPage=kMenuPageLastMusic;LastMusicPaintPump();assert(enabled&&!active&&rosters==4);
 g_lastMusicAdjustSel=2;assert(!LastMusicNoticeWanted());g_menuVisible=false;LastMusicPaintPump();assert(!enabled&&!active);
 g_optLastMusic=true;features=true;g_menuVisible=true;g_lastMusicAdjustSel=2;assert(LastMusicNoticeWanted());g_lastMusicNotice=false;assert(!LastMusicNoticeWanted());g_lastMusicNotice=true;features=false;assert(!LastMusicNoticeWanted());g_menuVisible=false;LastMusicPaintPump();assert(!enabled&&rosters==4);
 // Map initialization must rearm even if no loading Paint or reset event was delivered.
 features=true;g_optLastMusic=true;g_lastMusicAutoStop=false;
 totalCount=4;aliveCount=4;upCount=1;tick+=1000;LastMusicPaintPump();tick+=1000;LastMusicPaintPump();assert(active);
 for(int mode : {2,0}){
  g_lastMusicMode=mode;aliveCount=mode==0?0:4;upCount=mode==0?0:1;
  g_lastMusicReset=true;LastMusicPaintPump();tick+=1000;LastMusicPaintPump();assert(active);
  const int before=starts;
  Hooked_LevelInitPreEntity(nullptr,nullptr,"test_map");
  LastMusicPaintPump();assert(!active);tick+=1000;LastMusicPaintPump();assert(active&&starts==before+1);
 }
 puts("PASS: production Paint cadence, trigger/auto-stop, actual-start notice, disabled/menu gates and LevelInit rearming without loading Paint/events");
}
'''
    with tempfile.TemporaryDirectory(prefix='skeeto-last-paint-') as temp:
        compile_run(source, Path(temp))


def notice_draw():
    cpp = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    quiet = block(cpp, r'^\tif \(!g_hudPostMissionQuiet && !gameUiVisible && HudAnyCustomPaintWanted\(\)\)')
    assert 'LastMusicNoticePaint()' not in quiet, 'mission_lost quiet must not suppress the Rainie notice'
    assert 'if (!gameUiVisible && !HudCustomHidden() && LastMusicNoticeWanted()) LastMusicNoticePaint();' in cpp
    source = r'''
#include <Windows.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cwchar>
#include <algorithm>
static bool g_optLastMusic=true,g_lastMusicNotice=true,g_menuVisible=false,g_menuParked=false,g_lastMusicHudAbs=false,features=true;
static int kMenuPageLastMusic=104,g_menuPage=104,g_lastMusicAdjustSel=0,g_lastMusicHudScale=100,g_lastMusicHudX=0,g_lastMusicHudY=120,g_lastMusicHudFontTall=0;
static unsigned long g_lastMusicHudFont=0;static DWORD tick=11000,g_lastMusicNoticeBorn=10000,g_lastMusicNoticeUntil=15000;
static std::atomic_bool g_run{true},g_paintInGame{true},g_paintLoading{false};static void* g_engine=(void*)1;
static int sw=1920,sh=1080,fontCalls=0,measureCalls=0,fillCalls=0,textCalls=0,blueCalls=0;static bool english=false;
static DWORD TestTick(){return tick;}
#define GetTickCount TestTick
static bool SkeetoFeaturesOn(){return features;}static bool MenuEnsureSurf(){return true;}
static int HudClampScale(int s){return std::clamp(s,50,200);}static int HudFontPx(int s){return s;}
static int HudScalePx(int px,int sc){return MulDiv(px,sc,100);}
static constexpr int kFontFlagAntiAlias=1,kFontFlagDropShadow=2,kFontFlagOutline=4;
static unsigned long SurfMakeYahei(int px,int,int){++fontCalls;return px;}
enum class UiId {LastMusicNoticeText};
static const wchar_t* UiText(UiId,const wchar_t* cn){return english?L"Will we win?":cn;}
static int SurfTextWFont(unsigned long font,const wchar_t* text){++measureCalls;return (int)wcslen(text)*(int)font/2;}
static bool SurfGetScreenSize(int* w,int* h){*w=sw;*h=sh;return true;}
static void HudFinalizePos(bool* abs,int* ox,int* oy,int,int w,int h,int pw,int ph,int* x,int* y){assert(pw<=w&&ph<=h);*x=std::clamp(*abs?*ox:(w-pw)/2+*ox,0,w-pw);*y=std::clamp(*oy,0,h-ph);}
static void HudPanelShadow(int,int,int,int){}
static void SurfColor(int r,int g,int b,int){if(r==64&&g==144&&b==255)++blueCalls;}
static void SurfFill(int x,int y,int x1,int y1){assert(x>=0&&y>=0&&x1<=sw&&y1<=sh&&x1>=x&&y1>=y);++fillCalls;}
static void HudCornerBrackets(int,int,int,int,int,COLORREF c,int){assert(c==RGB(64,144,255));}
static void SurfTextAt(unsigned long,int x,int y,COLORREF,const wchar_t*){assert(x>=0&&y>=0&&x<sw&&y<sh);++textCalls;}
static void SurfOutline(int,int,int,int){}
'''
    source += block(cpp[cpp.rindex('static bool LastMusicNoticeWanted() {'):], r'^static bool LastMusicNoticeWanted\(')
    source += block(cpp[cpp.rindex('static void LastMusicNoticePaint() {'):], r'^static void LastMusicNoticePaint\(')
    source += r'''
int main(){
 for(int i=0;i<100;++i)LastMusicNoticePaint();assert(fontCalls==1&&measureCalls==1&&fillCalls==300&&textCalls==100&&blueCalls==100);
 english=true;LastMusicNoticePaint();assert(measureCalls==2&&fontCalls==1);
 const int widths[]={640,1280,1920,2560},heights[]={480,720,1080,1080},scales[]={50,100,200};
 for(int i=0;i<4;++i){sw=widths[i];sh=heights[i];for(int scale:scales){g_lastMusicHudScale=scale;g_lastMusicHudAbs=true;g_lastMusicHudX=7680;g_lastMusicHudY=4320;LastMusicNoticePaint();}}
 tick=15000;int drawn=textCalls;LastMusicNoticePaint();assert(textCalls==drawn);
 g_menuVisible=true;g_lastMusicAdjustSel=2;LastMusicNoticePaint();assert(textCalls==drawn+1);
 g_optLastMusic=false;LastMusicNoticePaint();assert(textCalls==drawn+1);
 g_optLastMusic=true;g_lastMusicNotice=false;LastMusicNoticePaint();assert(textCalls==drawn+1);
 g_lastMusicNotice=true;g_paintLoading=true;LastMusicNoticePaint();assert(textCalls==drawn+1);
 puts("PASS: production notice draw gate, 5s expiry/preview, blue accent, font/text caching, CN/EN layout at 4 resolutions and 50/100/200% (Surface fixture)");
}
'''
    with tempfile.TemporaryDirectory(prefix='skeeto-last-notice-') as temp:
        compile_run(source, Path(temp))


def media():
    ffmpeg = shutil.which('ffmpeg')
    if not ffmpeg:
        raise RuntimeError('Existing ffmpeg required for optional real-media fixture')
    tree = ast.parse((ROOT / 'tools/test_background_media.py').read_text(encoding='utf-8'))
    source = next(node.value.value for node in ast.walk(tree) if isinstance(node, ast.Assign)
                  and any(isinstance(t, ast.Name) and t.id == 'source' for t in node.targets))
    source = source[:source.index('int main(int argc,char** argv){')]
    source = source.replace('#define waveOutPause TestPause', r'''
static std::atomic_int writes{0},peak{0};
static MMRESULT TestWrite(HWAVEOUT output,LPWAVEHDR header,UINT size){
 const short* pcm=(const short*)header->lpData;int max=0;for(DWORD i=0;i<header->dwBufferLength/2;++i){int v=pcm[i];if(v<0)v=-v;if(v>max)max=v;}
 int old=peak.load();while(old<max&&!peak.compare_exchange_weak(old,max)){}
 ++writes;ZeroMemory(header->lpData,header->dwBufferLength);return waveOutWrite(output,header,size);
}
#define waveOutWrite TestWrite
#define waveOutPause TestPause''')
    source = source.replace('DlcCacheExtract* DlcCacheExtractBegin(const char*,const wchar_t*,unsigned){return nullptr;}', r'''
static wchar_t nativeTrack[520]{};static int begins=0,pumps=0,cancels=0;static bool missing=false;
struct DlcCacheExtract {int phase=0;};
DlcCacheExtract* DlcCacheExtractBegin(const char* path,const wchar_t* ext,unsigned limit){
 assert(!strcmp(path,"sound/skeeto/the last man/Rainie_Love.mp3")&&!wcscmp(ext,L".mp3")&&limit==128u*1024u*1024u);++begins;return missing?nullptr:new DlcCacheExtract();
}''')
    source = source.replace('int DlcCacheExtractPump(DlcCacheExtract*,unsigned,wchar_t*,size_t){return -1;}',
                            'int DlcCacheExtractPump(DlcCacheExtract* e,unsigned budget,wchar_t* out,size_t n){assert(budget==256u*1024u);++pumps;if(!e->phase++){return 0;}wcscpy_s(out,n,nativeTrack);return 1;}')
    source = source.replace('void DlcCacheExtractCancel(DlcCacheExtract*){}', 'void DlcCacheExtractCancel(DlcCacheExtract* e){if(e){++cancels;delete e;}}')
    source += r'''
static void waitWrites(int count){for(int i=0;i<1500&&writes<count;++i)Sleep(10);if(writes<count){printf("writes=%d wanted=%d state=%d active=%d\n",writes.load(),count,SkeetoLastMusicState(),g_lastMusicActive.load());fflush(stdout);}assert(writes>=count);}
static void waitFinish(){for(int i=0;i<1500&&g_lastMusicFinished.load()!=g_lastMusicGeneration.load();++i)Sleep(10);assert(g_lastMusicFinished.load()==g_lastMusicGeneration.load());}
int main(int argc,char** argv){assert(argc==2);MultiByteToWideChar(CP_ACP,0,argv[1],-1,nativeTrack,520);wcscat_s(nativeTrack,L"\\Rainie_Love.mp3");
 SkeetoLastMusicConfigure(false,100);SkeetoLastMusicResourcePump(true);assert(begins==0);
 SkeetoLastMusicConfigure(true,100);SkeetoLastMusicResourcePump(false);assert(begins==0);
 SkeetoLastMusicResourcePump(true);assert(begins==1&&pumps==1&&SkeetoLastMusicState()==0);
 SkeetoLastMusicResourcePump(true);assert(pumps==2&&SkeetoLastMusicState()==1);
 for(int i=0;i<10;++i)SkeetoLastMusicResourcePump(true);assert(begins==1&&pumps==2);
 SkeetoLastMusicLifecyclePump();SkeetoLastMusicSetActive(true);waitFinish();int full=peak.load();assert(full>1000&&SkeetoLastMusicStartedGeneration()==g_lastMusicGeneration.load());int once=writes;Sleep(700);assert(writes==once);
 SkeetoLastMusicConfigure(true,50);peak=0;SkeetoLastMusicSetActive(true);waitFinish();int half=peak.load();assert(half>full*0.45&&half<full*0.55);
 SkeetoLastMusicConfigure(true,0);peak=0;SkeetoLastMusicSetActive(true);waitFinish();assert(peak==0);
 SkeetoLastMusicSetActive(true);waitWrites(writes+1);const int before=resets;SkeetoLastMusicSetActive(false);assert(SkeetoLastMusicStartedGeneration()==0);Sleep(150);assert(resets>before);int stopped=writes;Sleep(150);assert(writes==stopped);
 // Reload/cancel are bounded, and an unavailable track has no retry storm.
 missing=true;SkeetoLastMusicReload();SkeetoLastMusicResourcePump(true);assert(SkeetoLastMusicState()==-1);int attempted=begins;
 for(int i=0;i<100;++i)SkeetoLastMusicResourcePump(true);assert(begins==attempted);
 missing=false;SkeetoLastMusicReload();SkeetoLastMusicResourcePump(true);SkeetoLastMusicResourcePump(true);assert(SkeetoLastMusicState()==1);
 SkeetoLastMusicSetActive(true);waitWrites(writes+1);
 SkeetoLastMusicConfigure(false,100);SkeetoLastMusicResourcePump(true);SkeetoLastMusicLifecyclePump();assert(!g_lastMusicWorker&&!g_lastMusicWake);
 SkeetoLastMusicConfigure(true,100);SkeetoLastMusicResourcePump(true);SkeetoLastMusicResourcePump(true);SkeetoLastMusicLifecyclePump();SkeetoLastMusicSetActive(true);waitWrites(writes+1);
 SkeetoLastMusicShutdown();assert(!g_lastMusicWorker&&!g_lastMusicWake);
 printf("PASS: fixed GAME path, 256KiB budget, disabled/loading gates, no rescan storm; real MF MP3 plays once/drains without looping, retrigger, 100/50/0 PCM, stop, reload, disable/re-enable, shutdown (writes=%d)\n",writes.load());
}
'''
    with tempfile.TemporaryDirectory(prefix='skeeto-last-media-') as temp:
        folder = Path(temp)
        subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-f', 'lavfi', '-i',
                        'sine=frequency=1000:sample_rate=44100:duration=0.3', '-c:a', 'libmp3lame', '-b:a', '128k',
                        str(folder / 'Rainie_Love.mp3')], check=True)
        compile_run(source, folder, media=True)


if __name__ == '__main__':
    classifier()
    paint_gates()
    notice_draw()
    media()
