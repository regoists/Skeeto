"""Exercise the production ammo-selection path and large-HUD preview gates."""
import os,re
from pathlib import Path
import subprocess,tempfile,unittest
from test_chapstats_pages import ROOT,block

HARNESS=r'''
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <initializer_list>
static bool g_optAmmoHud=true,g_optAmmoHudSecClip=true,g_optAmmoHudClipOnly=false;
static bool preview=false,layout=false,quiet=false;
static std::atomic_bool g_run{true};
static void* g_engine=(void*)1;
static int g_ammoHudScale=100;static unsigned long g_ammoFontId=1;
static int g_offActiveWeapon=0,held=100;
static int mask=0,primaryReads=0,secondaryReads=0,shownClip=-1,shownReserve=-1,shownSide=-1;
struct TeamHudSlot{int wep=1,side=2,priWepIdx=100,sideWepIdx=101,clip=-1,ammo=-1,sideClip=-1;};
struct GearCache{DWORD nextReadAt=999;};static GearCache g_teamHudGearCache[33];
static bool HudMenuPage(int page){return page==21&&preview;}
static bool HudLayoutSubPage(){return layout;}
static bool HudLayoutPeer(bool opt,int){return layout&&opt&&!preview;}
static bool IdleSwapHudQuietActive(){return quiet;}
static bool SkeetoFeaturesOn(){return true;}
static bool EngInGame(){return true;}static bool EngDrawingLoading(){return false;}
static bool MenuEnsureSurf(){return true;}static void AmmoHudEnsureFonts(){}
static bool SurfGetScreenSize(int* w,int* h){*w=1920;*h=1080;return true;}
static int EngLocal(){return 1;}static void* EntGet(int){return &held;}
static bool EntReadable(void*){return true;}static bool PtrCommitted(void*){return true;}
static int EntIndexFromHandle(int handle){return handle;}
static void TeamHudReadWepsCached(void*,int,TeamHudSlot*,unsigned){}
static void TeamHudReadWeps(void*,TeamHudSlot*){}
static bool TeamHudIsGun(int kind){return kind==1||kind==2;}
static bool TeamHudIsPrimary(int kind){return kind==1;}
static void TeamHudReadGunAmmo(void*,int index,int,int* clip,int* reserve){
 if(index==100){++primaryReads;*clip=30;if(reserve)*reserve=180;}
 else {++secondaryReads;*clip=15;}
}
static bool g_optTabBoard=true,g_tabBoardShow=false,g_optChapStats=true,g_csHeld=false;
static int page=99;
enum{ kMenuPageTabBoard=10,kMenuPageTabBoardStyle=11,kMenuPageChapStats=12,kMenuPageChapStatsStyle=13,kMenuPageChapStatsSize=14,kMenuPageChapStatsDisplay=15};
static bool PanelMenuPage(int p){return page==p;}
'''
FIXTURE=r'''
static void sample(int active){held=active;mask=primaryReads=secondaryReads=0;SampleAmmo();}
int main(){
 sample(100);if(mask!=1||shownClip!=30||shownReserve!=180||primaryReads!=1||secondaryReads)return 1;
 sample(101);if(mask!=2||shownSide!=15||primaryReads||secondaryReads!=1)return 2;
 for(int active:{0,102,103}){sample(active);if(mask||primaryReads||secondaryReads)return 3;}
 g_optAmmoHud=false;sample(101);if(mask!=2)return 4;
 sample(100);if(mask||primaryReads)return 5;
 g_optAmmoHud=true;g_optAmmoHudSecClip=false;sample(101);if(mask||secondaryReads)return 6;
 g_optAmmoHudSecClip=true;layout=true;sample(102);if(mask!=3)return 7;
 layout=false;preview=true;g_optAmmoHud=false;sample(102);if(mask!=3)return 8;
 preview=false;quiet=true;sample(101);if(mask)return 9;
 layout=true;page=99;if(ChapStatsLiveWanted()||BoardVisible())return 10;
 g_csHeld=g_tabBoardShow=true;if(!ChapStatsLiveWanted()||!BoardVisible())return 11;
 g_csHeld=g_tabBoardShow=false;g_optTabBoard=g_optChapStats=false;
 for(int p:{12,13,14,15}){page=p;if(!ChapStatsLiveWanted())return 12;}
 for(int p:{10,11}){page=p;if(!BoardVisible())return 13;}
 return 0;
}
'''

class AmmoFollowActive(unittest.TestCase):
 def test_switching_options_preview_and_large_panel_exclusion(self):
  hud=(ROOT/'src/skeeto/core/skeeto_hud.cpp').read_text(encoding='utf-8-sig')
  proxy=(ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
  ammo=block(hud,r'^void AmmoHudPaintHud\(\) \{')
  ammo=ammo[:ammo.index('\n\twchar_t clipBuf')].replace('void AmmoHudPaintHud()', 'static void SampleAmmo()')
  ammo+='\n mask=(drawPri?1:0)|(drawSec?2:0);shownClip=clip;shownReserve=ammo;shownSide=secClip;\n}\n'
  chap=block(proxy,r'^static bool ChapStatsLiveWanted\(\) \{').replace('HudMenuPage(', 'PanelMenuPage(')
  board=block(proxy,r'^static void TabBoardPaint\(\) \{')
  board=board[:board.index('\n\tif (!g_run')].replace('static void TabBoardPaint()', 'static bool BoardVisible()').replace('HudMenuPage(', 'PanelMenuPage(').replace('return;', 'return false;')+'\nreturn true;\n}\n'
  with tempfile.TemporaryDirectory(prefix='skeeto-ammo-') as directory:
   out=Path(directory);cpp=out/'test.cpp';exe=out/'test.exe'
   cpp.write_text(HARNESS+ammo+chap+board+FIXTURE,encoding='utf-8')
   vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
   vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
   bat=out/'compile.bat'
   bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /Fe"{exe}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n',encoding='utf-8')
   result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace');self.assertEqual(result.returncode,0,result.stdout+result.stderr)
   result=subprocess.run([str(exe)],capture_output=True,text=True);self.assertEqual(result.returncode,0,result.stdout+result.stderr)

if __name__=='__main__':unittest.main()
