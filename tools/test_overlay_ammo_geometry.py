"""Exercise the production overlay ammo sampler with captured geometry.

This verifies placement and rotation, not the appearance of the game window.
"""
import os
from pathlib import Path
import subprocess
import tempfile

from test_chapstats_pages import ROOT, block


def run():
    hud = (ROOT / "src/skeeto/core/skeeto_hud.cpp").read_text(encoding="utf-8")
    source = r'''
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
static std::atomic_bool g_run{true};
static void* g_engine = (void*)1;
static bool enabled=true, policy=true, inGame=true, loading=false;
static bool cfgPreview=false, sampleOk=true, screenOk=true;
static int cfgSide=3, cfgScale=100, cfgOffX=0, cfgOffY=0, cfgSw=1920, cfgSh=1080, calls=0;
static bool SkeetoFeaturesOn(){return policy;}
static bool EngInGame(){return inGame;}
static bool EngDrawingLoading(){return loading;}
static bool SurfGetScreenSize(int* w,int* h){*w=cfgSw;*h=cfgSh;return screenOk;}
static bool HudOverlayGetAmmoArcConfig(int* x,int* y,int* sc,int* s,bool* p){
 *x=cfgOffX;*y=cfgOffY;*sc=cfgScale;*s=cfgSide;*p=cfgPreview;return enabled;
}
static bool AmmoGfxSampleClip(int* clip,int* maxClip){*clip=15;*maxClip=30;return sampleOk;}
static void AmmoGfxHealthRgb(float,int* r,int* g,int* b){*r=*g=*b=255;}
static int HudScalePx(int px,int sc){return (px*sc+50)/100;}
struct Geometry {int x,y,radius,thick,clip,maxClip;float a0,a1,frac;bool preview;};
static Geometry geometry{};
static void HudOverlaySetAmmoArc(int,int,int x,int y,int r,int t,float a0,float a1,
 float f,int clip,int maxClip,int,int,int,bool p){
 ++calls;geometry={x,y,r,t,clip,maxClip,a0,a1,f,p};
}
'''
    source += block(hud, r"^void HudOverlayAmmoArcPaint\(\)")
    source += r'''
static bool near(float a,float b){return std::fabs(a-b)<0.001f;}
int main(){
 // The original right-side circle is 76px from the crosshair, independent of size.
 // Rotating that whole arrangement gives these four centers and outward midpoints.
 const int centers[4][2]={{0,76},{0,-76},{-76,0},{76,0}};
 const int scales[]={50,100,200};
 const int screens[][2]={{640,480},{1920,1080},{2560,1080},{1921,1081}};
 const int offsets[][2]={{0,0},{24,-16},{-32,40},{4096,-4096},{-4096,4096}};
 int cases=0;
 for(auto& screen:screens)for(int sc:scales)for(auto& offset:offsets)for(int s=0;s<4;++s){
  cfgSw=screen[0];cfgSh=screen[1];cfgScale=sc;cfgSide=s;cfgOffX=offset[0];cfgOffY=offset[1];
  const int oldCalls=calls;HudOverlayAmmoArcPaint();assert(calls==oldCalls+1);
  assert(geometry.x-cfgOffX==centers[s][0]&&geometry.y-cfgOffY==centers[s][1]);
  assert(geometry.radius==HudScalePx(46,cfgScale)&&geometry.thick==HudScalePx(8,cfgScale));
  const float mid=(geometry.a0+geometry.a1)*0.5f;
  const float dx=std::cos(mid),dy=std::sin(mid);
  // Circle center, arc midpoint and the unadjusted crosshair lie on one line.
  assert(near(centers[s][0]*dy-centers[s][1]*dx,0.f));
  assert(near(centers[s][0]*dx+centers[s][1]*dy,76.f));
  assert(near(geometry.a1-geometry.a0,3.14159265f*0.56f));
  assert(near(geometry.frac,0.5f)&&geometry.clip==15&&geometry.maxClip==30);
  ++cases;
 }
 // Invalid/disabled contexts never publish geometry; preview retains its sample fallback.
 auto hidden=[](){int n=calls;HudOverlayAmmoArcPaint();assert(calls==n);};
 enabled=false;hidden();enabled=true;g_run=false;hidden();g_run=true;
 policy=false;hidden();policy=true;g_engine=nullptr;hidden();g_engine=(void*)1;
 inGame=false;hidden();inGame=true;loading=true;hidden();loading=false;
 screenOk=false;hidden();screenOk=true;sampleOk=false;hidden();
 cfgPreview=true;int n=calls;HudOverlayAmmoArcPaint();assert(calls==n+1);
 assert(geometry.preview&&geometry.clip==18&&geometry.maxClip==30&&near(geometry.frac,0.6f));
 std::printf("PASS: %d four-direction geometry cases and 9 gating/preview cases\n",cases);
}
'''
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output(
        [str(vswhere), "-latest", "-products", "*", "-requires",
         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        text=True,
    ).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-ammo-geometry-") as temp:
        folder = Path(temp)
        (folder / "fixture.cpp").write_text(source, encoding="utf-8")
        (folder / "run.bat").write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
            'if errorlevel 1 exit /b 1\n'
            'cl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX fixture.cpp /Fe:fixture.exe\n'
            'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n',
            encoding="utf-8",
        )
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)

    proxy = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    clamp_add = block(proxy, r"^static int ClampAdd\(")
    migration = block(proxy, r"^\s*if \(hadAmmoArcOverlayOffX && !hadAmmoArcOverlaySide\)")
    migrate_source = r'''
#include <cassert>
#include <cstdio>
typedef unsigned long DWORD;
static DWORD now=42;
static DWORD GetTickCount(){return now;}
static int g_ammoArcOverlayOffX=0,g_ammoArcOverlaySide=3;
static bool hadAmmoArcOverlayOffX=false,hadAmmoArcOverlaySide=false,g_settingsDirty=false;
static DWORD g_settingsSaveAt=0;
'''
    migrate_source += clamp_add + "\nstatic void MigrateLegacyAmmoArc(){\n" + migration + "\n}\n"
    migrate_source += r'''
int main(){
 // Published 1.7.0 stored the complete right-side circle-center offset in X.
 hadAmmoArcOverlayOffX=true;hadAmmoArcOverlaySide=false;g_ammoArcOverlayOffX=76;g_ammoArcOverlaySide=0;
 MigrateLegacyAmmoArc();assert(g_ammoArcOverlayOffX==0&&g_ammoArcOverlaySide==3&&g_settingsDirty&&g_settingsSaveAt==42);
 // Preserve custom placement by removing only the new side anchor.
 g_settingsDirty=false;g_settingsSaveAt=0;g_ammoArcOverlayOffX=100;g_ammoArcOverlaySide=0;
 MigrateLegacyAmmoArc();assert(g_ammoArcOverlayOffX==24&&g_ammoArcOverlaySide==3&&g_settingsDirty);
 // A released config with an explicit side key already uses four-way semantics.
 g_settingsDirty=false;g_ammoArcOverlayOffX=100;g_ammoArcOverlaySide=1;hadAmmoArcOverlaySide=true;
 MigrateLegacyAmmoArc();assert(g_ammoArcOverlayOffX==100&&g_ammoArcOverlaySide==1&&!g_settingsDirty);
 // Missing old X means the new default already recreates the original 76px center gap.
 hadAmmoArcOverlayOffX=false;hadAmmoArcOverlaySide=false;g_ammoArcOverlayOffX=0;g_ammoArcOverlaySide=3;
 MigrateLegacyAmmoArc();assert(g_ammoArcOverlayOffX==0&&g_ammoArcOverlaySide==3&&!g_settingsDirty);
 std::puts("PASS: released ammo-arc position migration preserves default/custom centers and skips four-way settings");
}
'''
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output(
        [str(vswhere), "-latest", "-products", "*", "-requires",
         "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        text=True,
    ).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-ammo-migration-") as temp:
        folder = Path(temp)
        (folder / "fixture.cpp").write_text(migrate_source, encoding="utf-8")
        (folder / "run.bat").write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
            'if errorlevel 1 exit /b 1\n'
            'cl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX fixture.cpp /Fe:fixture.exe\n'
            'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n',
            encoding="utf-8",
        )
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)


if __name__ == "__main__":
    run()
