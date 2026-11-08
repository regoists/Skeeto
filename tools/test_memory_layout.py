"""Exercise production memory HUD geometry with bounded text measurements."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

HARNESS = r'''
#include <windows.h>
#include <cwchar>
#include <vector>
#include <string>
#include <cstdio>
struct MemoryHudRow { const wchar_t* label; wchar_t value[128]; COLORREF base,color; const wchar_t* secondLabel; wchar_t secondValue[64]; };
static int g_memoryHudScale=100,g_memoryHudLayoutFont=14,g_memoryHudAbs=0,g_memoryHudOffX=0,g_memoryHudOffY=0,g_memoryHudAlign=0;
static unsigned long g_memoryHudFontId=0; static int g_memoryHudFontTall=0;
static bool detailed=true;
namespace UiId { enum { MemoryHudCardTitle, MemoryHudBrand }; }
static const wchar_t* UiText(int,const wchar_t* text) { return text; }
static int HudClampScale(int v) { return v; }
static int HudScalePx(int v,int scale) { return v*scale/100; }
static unsigned long HudAcquireLayoutFont(int,unsigned long*,int*,int) { return 1; }
static int SurfTextWFont(unsigned long,const wchar_t* text) { return (int)wcslen(text)*8; }
static int panelWidth=0,panelHeight=0,shadows=0,accents=0,corners=0;
struct Draw { int x,y; std::wstring text; }; static std::vector<Draw> draws;
static void SurfTextAt(unsigned long,int x,int y,COLORREF,const wchar_t* text) { draws.push_back({x,y,text}); }
static void HudFinalizePos(int*,int*,int*,int,int,int,int w,int h,int* x,int* y) { panelWidth=w;panelHeight=h;*x=*y=0; }
static void HudPanelShadow(int,int,int,int) { ++shadows; }
static void HudGlassBody(int,int,int,int) {}
static void HudSideAccent(int,int,int,COLORREF,int) { ++accents; }
static void HudCornerBrackets(int,int,int,int,int,COLORREF,int) { ++corners; }
static void SurfColor(int,int,int,int) {}
static void SurfFill(int,int,int,int) {}
static int MemoryHudBuildRows(MemoryHudRow (&rows)[7],bool) {
 const wchar_t* labels[]={L"DC",L"锁定",L"AV",L"最大连续内存",L"实际占用",L"ENTS",L"CLR"};
 const wchar_t* values[]={L"2048/2048 MB",L"1999 MB",L"2223 MB",L"1875 MB",L"9999 MB",L"2048",L"1024 MB / 999999s"};
 for(int i=0;i<7;++i){rows[i].label=labels[i];wcscpy_s(rows[i].value,values[i]);rows[i].base=rows[i].color=RGB(1,2,3);}
 rows[1].secondLabel=L"未锁定";wcscpy_s(rows[1].secondValue,L"49 MB");
 rows[4].secondLabel=L"私有提交";wcscpy_s(rows[4].secondValue,L"9999 MB");
 if(!detailed){ rows[1]=rows[2];rows[2]=rows[5];return 3; }
 return 7;
}
'''
FIXTURE = r'''
int main(){
 for(int info=0;info<2;++info)for(int style=0;style<2;++style){
  detailed=info!=0;draws.clear();shadows=accents=corners=0;
  MemoryHudPaintLayout(1920,1080,false,style!=0);
  if(shadows!=1||accents!=1||corners!=1)return 1;
  int brands=0;for(const auto& d:draws)if(d.text==L"Skeeto")++brands;
  if(brands!=(style==0?1:0))return 7;
  for(size_t i=0;i<draws.size();++i){
   const auto& a=draws[i];const int end=a.x+SurfTextWFont(1,a.text.c_str());
   if(a.x<8||end>panelWidth-8||a.y<8||a.y+14>panelHeight-8)return 2;
   for(size_t j=i+1;j<draws.size();++j)if(draws[j].y==a.y&&draws[j].x<end)return 3;
  }
  if(detailed){
   int dc=-1,clr=-2,av=-3,ents=-4,lockedX=-1,lockedY=-1;
   for(const auto& d:draws){if(d.text==L"DC")dc=d.y;if(d.text==L"CLR")clr=d.y;if(d.text==L"AV")av=d.y;if(d.text==L"ENTS")ents=d.y;if(d.text==L"锁定"){lockedX=d.x;lockedY=d.y;}}
   if(dc!=clr||av!=ents)return 4;
   bool adjacent=false;for(const auto& d:draws)if(d.text.find(L"1999 MB")==0)adjacent=d.x==lockedX+SurfTextWFont(1,L"锁定 ")&&d.y==lockedY;
   if(!adjacent)return 5;
   for(const auto& d:draws)if(d.text==L"1024 MB / 999999s"||d.text==L"2048"||d.text==L"49 MB")if(d.x+SurfTextWFont(1,d.text.c_str())!=panelWidth-8)return 6;
  }
 }
 return 0;
}
'''

class MemoryLayout(unittest.TestCase):
    def test_pairs_stay_on_same_line_without_overlap(self):
        source=(ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        with tempfile.TemporaryDirectory(prefix='skeeto-layout-') as directory:
            out=Path(directory); cpp=out/'layout.cpp'; exe=out/'layout.exe'
            cpp.write_text(HARNESS+block(source,r'^static void MemoryHudPaintLayout\(')+FIXTURE,encoding='utf-8')
            vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
            vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
            bat=out/'compile.bat'
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /Fe"{exe}" /Fo"{out / "layout.obj"}" "{cpp}"\nexit /b %errorlevel%\n',encoding='utf-8')
            result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            result=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)

if __name__=='__main__': unittest.main()
