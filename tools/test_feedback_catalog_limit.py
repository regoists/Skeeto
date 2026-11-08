"""Test real feedback JSON parsing, admission limits and safe reload gating."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

HARNESS = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cctype>
#include <string>
#include <vector>
static DlcCatalog g_cat{};
static int g_styleLimit=64;
static bool g_styleLimitPending=false;
static int g_lastEngineJson=0,g_lastLooseJson=0,g_lastReloadMs=0;
static bool g_catalogLoaded=false,g_engineGlobTried=false;
static DWORD g_engineGlobRetryAt=0;
static bool available=true;
static void* FsGet(){return available ? (void*)1 : nullptr;}
static std::vector<std::string> loose,game;
static int scans=0;
static void AddStyleFromJson(const char*);
static void ScanLooseDir(const char*){++scans;for(const auto& s:loose)AddStyleFromJson(s.c_str());}
static int ScanEngineGlob(const char*,const char*){++scans;for(const auto& s:game)AddStyleFromJson(s.c_str());return (int)game.size();}
'''
FIXTURE = r'''
static std::string json(int n,const char* name="original"){
 char data[512];snprintf(data,sizeof(data),"{\"id\":\"style_%d\",\"name\":\"%s\",\"channel\":\"ci\",\"sort\":%d,\"kill\":{\"sound\":\"skeeto/test.wav\"}}",n,name,n);return data;
}
int main(int argc,char** argv){
 int test=argc>1?atoi(argv[1]):0;
 for(int i=0;i<150;++i)game.push_back(json(i));
 if(test==0){
  for(int limit:{64,96,128}){
   DlcSetStyleLimit(limit);DlcReloadCatalog("game",true);
   if(g_cat.count!=limit||strcmp(g_cat.styles[limit-1].id,("style_"+std::to_string(limit-1)).c_str()))return 1;
   AddStyleFromJson(json(0,"updated").c_str());
   if(g_cat.count!=limit||strcmp(g_cat.styles[0].name,"updated"))return 2;
  }
  return 0;
 }
 if(test==1){
  if(DlcStyleLimit()!=64)return 3;
  if(!DlcEnsureCatalog("game",true)||g_cat.count!=64)return 4;
  strcpy_s(g_cat.ciId,"style_2");
  int before=scans;DlcSetStyleLimit(96);
  if(!DlcStyleLimitPending()||DlcEnsureCatalog("game",false)||scans!=before||g_cat.count!=64)return 5;
  if(!DlcEnsureCatalog("game",true)||g_cat.count!=96||DlcStyleLimitPending()||strcmp(g_cat.ciId,"style_2"))return 6;
  before=scans;if(DlcEnsureCatalog("game",true)||scans!=before)return 7;
  return 0;
 }
 DlcSetStyleLimit(128);DlcReloadCatalog("game",true);
 DlcSetStyleLimit(999);if(DlcStyleLimit()!=64)return 8;
 available=false;int before=scans;if(DlcEnsureCatalog("game",true)||scans!=before)return 9;
 available=true;Sleep(510);
 if(!DlcEnsureCatalog("game",true)||g_cat.count!=64)return 10;
 AddStyleFromJson("{\"name\":\"invalid\"}");if(g_cat.count!=64)return 11;
 return 0;
}
'''

class FeedbackCatalogLimit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source=(ROOT/'src/skeeto/core/skeeto_style.cpp').read_text(encoding='utf-8-sig')
        parser=source[source.index('static const char* SkipWs('):source.index('static IFileSystem* FsGet()')]
        functions='\n'.join(block(source,pattern) for pattern in [
            r'^int DlcStyleLimit\(',r'^bool DlcStyleLimitPending\(',r'^void DlcSetStyleLimit\(',
            r'^static void AddStyleFromJson\(const char\* json\) \{',r'^static int CmpStyle\(',
            r'^void DlcReloadCatalog\(',r'^bool DlcEnsureCatalog\('])
        cls.temp=tempfile.TemporaryDirectory(prefix='skeeto-catalog-');out=Path(cls.temp.name)
        cpp=out/'catalog.cpp';cls.exe=out/'catalog.exe'
        cpp.write_text('#include "'+(ROOT/'src/skeeto/core/skeeto_style.h').as_posix()+'"\n'+HARNESS+parser+functions+FIXTURE,encoding='utf-8')
        vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
        vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
        bat=out/'compile.bat'
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /Fe"{cls.exe}" /Fo"{out / "catalog.obj"}" "{cpp}"\nexit /b %errorlevel%\n',encoding='utf-8')
        result=subprocess.run(['cmd','/c',str(bat)],capture_output=True,text=True,errors='replace')
        if result.returncode:raise RuntimeError(result.stdout+result.stderr)
    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()
    def case(self,n):
        result=subprocess.run([str(self.exe),str(n)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
    def test_all_capacity_tiers_and_duplicate_updates(self):self.case(0)
    def test_limit_changes_wait_for_safe_reload(self):self.case(1)
    def test_invalid_setting_and_interface_retry(self):self.case(2)

if __name__=='__main__':unittest.main()
