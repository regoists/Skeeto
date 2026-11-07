"""Exercise production background upload/draw through Source; no game or GPU."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(text, signature):
    start=text.index(signature); at=text.index('{',start); end=at+1; depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]
def main():
    text=(ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8')
    fixture=r'''
#include <windows.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
struct SkeetoBackgroundFrame { const unsigned char* bgra; int width,height,fit; unsigned serial; };
static int rawCalls=0,uploads=0,draws=0,blackFills=0,creates=0;
static bool bgrx=true,drawOk=true,surfaceReady=true;
static unsigned char uploaded[16]{};
static void* g_surf=(void*)1;
static constexpr int kVmtSurfCreateNewTextureID=43;
static int g_backgroundTextureId=-1,g_backgroundTextureW=0,g_backgroundTextureH=0;
static unsigned g_backgroundTextureSerial=0;
static unsigned char* g_backgroundSurfaceRgba=nullptr;static size_t g_backgroundSurfaceRgbaBytes=0;
static bool g_backgroundUsingD3d=false,g_backgroundUsingBik=false,g_backgroundDrawBlack=false;
static bool g_backgroundReplaceArmed=true,g_backgroundReplacedThisPaint=false,g_backgroundReplaceLogged=false,g_backgroundBikLogged=false;
static int g_backgroundLastBackend=0,g_backgroundBikTextureId=9;
static int g_backgroundScreenW=640,g_backgroundScreenH=480,g_backgroundScreenX=0,g_backgroundScreenY=0;
static int g_backgroundDrawX0=0,g_backgroundDrawY0=0,g_backgroundDrawX1=640,g_backgroundDrawY1=480;
static float g_backgroundDrawU0=0,g_backgroundDrawV0=0,g_backgroundDrawU1=1,g_backgroundDrawV1=1;
static bool MenuEnsureSurf(){return surfaceReady;}
static int __fastcall create(void*,void*,bool){++creates;return 7;}
static void* VGet(void*,int){return (void*)&create;}
static bool IsExec(void* p){return p!=nullptr;}
static bool SurfSetTextureBGRX(int,const unsigned char* data,int,int){if(!bgrx)return false;++uploads;memcpy(uploaded,data,16);return true;}
static void SurfSetTextureRGBA(int,const unsigned char* data,int,int,int){++uploads;memcpy(uploaded,data,16);}
static void SurfColor(int,int,int,int){}
static void SurfFill(int,int,int,int){++blackFills;}
static bool SurfDrawTexturedUv(int,int,int,int,int,float,float,float,float){++draws;return drawOk;}
static void Log(const char*,...){}
static bool BackgroundEnsureD3DTexture(const SkeetoBackgroundFrame&){++rawCalls;return false;}
static bool BackgroundD3DDraw(){++rawCalls;return true;}
static bool BackgroundConsoleOpen(){return false;}
static bool BackgroundServerBrowserOpen(){return false;}
static bool BackgroundOptionsDialogOpen(){return false;}
static bool SkeetoBackgroundAcquireFrame(SkeetoBackgroundFrame*){assert(false);return false;}
'''
    if 'static bool BackgroundEnsureSurfaceTexture(const SkeetoBackgroundFrame& frame) {' in text:
        fixture+=function(text,'static bool BackgroundEnsureSurfaceTexture(const SkeetoBackgroundFrame& frame) {')+'\n'
    fixture+=function(text,'static bool BackgroundEnsureTexture(const SkeetoBackgroundFrame& frame) {')+'\n'
    fixture+=function(text,'static bool BackgroundDrawReplacement() {')+'\n'
    fixture+=r'''
int main(){
 unsigned char pixels[16]={1,2,3,9,4,5,6,9,7,8,9,9,10,11,12,9};
 SkeetoBackgroundFrame frame{pixels,2,2,0,1};
 assert(BackgroundEnsureTexture(frame));assert(rawCalls==0);assert(creates==1&&uploads==1);
 assert(BackgroundEnsureTexture(frame)&&uploads==1); // same frame does not upload again
 assert(BackgroundDrawReplacement()&&draws==1&&g_backgroundLastBackend==1&&g_backgroundReplacedThisPaint&&!g_backgroundReplaceArmed);
 // Reenter the menu after a demo/map boundary; reuse Surface id, upload next frame.
 g_backgroundTextureSerial=0;frame.serial=2;assert(BackgroundEnsureTexture(frame)&&creates==1&&uploads==2);
 bgrx=false;frame.serial=3;assert(BackgroundEnsureTexture(frame)&&uploads==3);
 assert(uploaded[0]==3&&uploaded[1]==2&&uploaded[2]==1&&uploaded[3]==255);
 assert(uploaded[12]==12&&uploaded[14]==10&&uploaded[15]==255); // exact BGRA -> RGBA
 g_backgroundDrawBlack=true;assert(BackgroundDrawReplacement()&&blackFills==1);
 g_backgroundUsingBik=true;assert(BackgroundDrawReplacement()&&g_backgroundLastBackend==3); // native BIK retained
 g_backgroundUsingBik=false;drawOk=false;assert(!BackgroundDrawReplacement()); // stock paint remains available
 surfaceReady=false;assert(!BackgroundEnsureTexture(frame));assert(rawCalls==0);
 free(g_backgroundSurfaceRgba);puts("PASS Source background: upload reuse, BGRA colors, menu reentry, fit, BIK and failure fallback; no direct D3D");
}
'''
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-bg-render-') as tmp:
        folder=Path(tmp);(folder/'fixture.cpp').write_text(fixture,encoding='utf-8')
        (folder/'build.bat').write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 fixture.cpp user32.lib /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n',encoding='utf-8')
        subprocess.run(['cmd','/c','build.bat'],cwd=folder,check=True)
if __name__=='__main__':main()
