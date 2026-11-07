"""Capture real damage-number Surface draws; this is not an in-game visual test."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

from test_chapstats_pages import ROOT, block


def run():
    proxy = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8")
    settings = proxy[proxy.index("static int g_optClientDmgCritThresh ="):
                     proxy.index("static bool g_optClientDirectorHud =")]
    source = r'''
#include <Windows.h>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>
#include "skeeto_damage_events.h"
static DWORD clockNow=1000;
#define GetTickCount() clockNow
static std::atomic_bool g_run{true};
static void* g_engine=(void*)1;
static void* g_surf=(void*)1;
static bool g_optClientDmgNum=true,g_optWorldFriendlyFireDmgNum=false,g_menuVisible=false,g_menuParked=false;
static bool g_optClientInfectedHp=false;
static int g_offOrigin=-1,g_hpTrackEnt=0,g_hudFontTall=12;
static DWORD g_hpTrackUntil=0;
static unsigned long g_dmgFont=1,g_hudFont=3,g_dmgCritFont=2;
static unsigned long g_dmgFriendlyFireFont=4;
static int g_dmgFontTall=24,g_dmgCritFontTall=34;
static int g_dmgFriendlyFireFontTall=48;
static int criticalFontRequests=0,friendlyFireFontRequests=0;
static bool SkeetoFeaturesOn(){return true;}
static bool EngInGame(){return true;}
static bool MenuEnsureSurf(){return true;}
static void SurfEnsureDmgFont(){}
static void SurfEnsureHudFont(){}
static void SurfEnsureDmgCritFont(){++criticalFontRequests;}
static void SurfEnsureFriendlyFireDmgFont(){++friendlyFireFontRequests;}
static void* EntGet(int){return nullptr;}
static bool IsExec(void*){return true;}
static bool LocalPlayWorldToScreen(const float* p,float* x,float* y,int,int){
 *x=400;*y=300-p[2];return true;
}
struct DmgNumSlot {int ent,dmg;float x,y,z,lift,ang;DWORD born;unsigned char pal;bool friendlyFire;};
static constexpr int kDmgLifeMs=1000,kDmgPalN=1;
static const int kDmgPal[kDmgPalN][3]={{80,200,240}};
static DmgNumSlot g_dmg[2]{};
static int g_dmgN=0;
static void ClientUxDmgReset(){g_dmgN=0;}
struct Draw {std::wstring text;unsigned long font;int r,g,b;};
static std::vector<Draw> draws;
static unsigned long currentFont=0;
static int currentR=0,currentG=0,currentB=0;
static void __fastcall FakeSize(void*,void*,int* w,int* h){*w=1920;*h=1080;}
static void __fastcall FakeFont(void*,void*,unsigned long f){currentFont=f;}
static void __fastcall FakeColor(void*,void*,int r,int g,int b,int){currentR=r;currentG=g;currentB=b;}
static void __fastcall FakePos(void*,void*,int,int){}
static void __fastcall FakePrint(void*,void*,const wchar_t* text,int len,int){
 draws.push_back({std::wstring(text,len),currentFont,currentR,currentG,currentB});
}
static void* VGet(void*,int slot){
 switch(slot){case 35:return (void*)FakeSize;case 17:return (void*)FakeFont;
 case 19:return (void*)FakeColor;case 20:return (void*)FakePos;case 22:return (void*)FakePrint;}
 return nullptr;
}
'''
    source += settings + block(proxy, r"^static void ClientUxPaintDmgNums\(\) \{")
    source += '\nstatic bool ParseThresholdLine(const char* line){int v=0;\n'
    for key in ("client_dmg_crit_thresh", "client_dmg_crit_thresh2"):
        source += block(proxy, re.escape(f'if (sscanf(line, "{key}=%d", &v) == 1)'))
    source += '\nreturn false;}\n'
    source += r'''
static void Check(int damage,const wchar_t* text,int tier){
 draws.clear();criticalFontRequests=0;g_dmgN=1;g_optClientDmgNum=true;
 g_dmg[0]={1,damage,0,0,0,40,0,clockNow-10,0};
 ClientUxPaintDmgNums();assert(!draws.empty());
 const auto& core=draws.back();
 assert(core.text==text);assert(core.font==(tier?2u:1u));
 assert(draws.size()==(tier?5u:2u));
 assert(criticalFontRequests==(tier?1:0));
 if(tier==2)assert(core.r==255&&core.g==64&&core.b==64);
 if(tier==1)assert(core.r==255&&core.g==220&&core.b==48);
 if(tier==0)assert(core.r==80&&core.g==200&&core.b==240);
}
int main(){
 ClampClientDmgCritThresholds();assert(ClientDmgCritThreshValue()==100&&ClientDmgCritThresh2Value()==200);
 Check(99,L"99",0);Check(100,L"100!",1);Check(200,L"200!",1);
 Check(201,L"201!!!",2);Check(9999,L"9999!!!",2);
 g_optClientDmgCritThresh=0;Check(9999,L"9999",0);
 g_optClientDmgNum=false;draws.clear();ClientUxPaintDmgNums();assert(draws.empty()&&g_dmgN==0);
 // All valid and malformed indices: consistent display, ordered thresholds and exact boundaries.
 const int firstValues[]={0,50,80,100,150,200};
 const int secondValues[]={100,150,200,250,300,400,500};
 const int damageValues[]={0,49,50,79,80,99,100,149,150,199,200,201,249,250,251,
  299,300,301,399,400,401,499,500,501,9999};
 int classifications=0;
 for(int a=-3;a<9;++a)for(int b=-3;b<10;++b){
  g_optClientDmgCritThresh=a;g_optClientDmgCritThresh2=b;
  const int first=firstValues[a<0?0:a>5?5:a];
  const int rawSecond=secondValues[b<0?0:b>6?6:b];
  const int second=rawSecond<first?first:rawSecond;
  assert(ClientDmgCritThresh2Value()==second);
  ClampClientDmgCritThresholds();
  assert(ClientDmgCritThreshValue()==first&&ClientDmgCritThresh2Value()==second);
  assert(secondValues[g_optClientDmgCritThresh2]==second);
  for(int damage:damageValues){
   int expected=first==0||damage<first?0:damage>second?2:1;
   assert(ClientDmgCritTier(damage)==expected);++classifications;
  }
 }
 // Settings parsing is order-independent; normalization happens after all lines have loaded.
 for(int reverse=0;reverse<2;++reverse){
  g_optClientDmgCritThresh=3;g_optClientDmgCritThresh2=2;
  assert(ParseThresholdLine(reverse?"client_dmg_crit_thresh2=0":"client_dmg_crit_thresh=5"));
  assert(ParseThresholdLine(reverse?"client_dmg_crit_thresh=5":"client_dmg_crit_thresh2=0"));
  ClampClientDmgCritThresholds();assert(g_optClientDmgCritThresh2==2);
  Check(199,L"199",0);Check(200,L"200!",1);Check(201,L"201!!!",2);
 }
 assert(ParseThresholdLine("client_dmg_crit_thresh2=-1"));assert(g_optClientDmgCritThresh2==0);
 assert(ParseThresholdLine("client_dmg_crit_thresh2=999"));assert(g_optClientDmgCritThresh2==6);
 assert(!ParseThresholdLine("client_dmg_crit_thresh_other=1"));
 Check(500,L"500!",1);Check(501,L"501!!!",2);
 assert(skeeto_damage::FriendlyFireDamageFontPx(24)==48);
 auto checkFriendlyFire=[](int damage,const wchar_t* expected,int r,int g,int b){
  draws.clear();criticalFontRequests=0;friendlyFireFontRequests=0;
  g_dmgN=1;g_optClientDmgNum=false;g_optWorldFriendlyFireDmgNum=true;
  g_dmg[0]={1,damage,0,0,0,40,0,clockNow-10,0,true};
  ClientUxPaintDmgNums();assert(!draws.empty());
  const auto& core=draws.back();assert(core.text==expected);
  assert(core.font==4&&g_dmgFriendlyFireFontTall==2*g_dmgFontTall);
  assert(core.r==r&&core.g==g&&core.b==b);
  assert(friendlyFireFontRequests==1&&criticalFontRequests==0);
 };
 checkFriendlyFire(14,L"-14!",255,220,48);
 checkFriendlyFire(15,L"-15!!!",255,64,64);
 g_optWorldFriendlyFireDmgNum=false;draws.clear();g_dmgN=1;
 ClientUxPaintDmgNums();assert(draws.empty()&&g_dmgN==0);
 std::printf("PASS: %d threshold classifications and both settings load orders\n",classifications);
 std::puts("PASS: actual damage-number text, colors, shared font, draw budget and disabled state");
}
'''
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output(
        [str(vswhere), "-latest", "-products", "*", "-requires",
         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        text=True,
    ).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-damage-tiers-") as temp:
        folder = Path(temp)
        (folder / "fixture.cpp").write_text(source, encoding="utf-8")
        (folder / "run.bat").write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
            'if errorlevel 1 exit /b 1\n'
            f'cl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX /D_CRT_SECURE_NO_WARNINGS /I"{ROOT / "src/skeeto/core"}" fixture.cpp /Fe:fixture.exe\n'
            'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n',
            encoding="utf-8",
        )
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)


if __name__ == "__main__":
    run()
