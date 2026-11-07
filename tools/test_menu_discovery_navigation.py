"""Compile the production navigation host/return wrapper with inert game stubs."""
from pathlib import Path
import importlib.util
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "src/skeeto/core"

PRELUDE = r'''
#include <windows.h>
#include <cassert>
#include <cwchar>
#include <vector>
#include <string>
#include "skeeto_ui_text.h"
#include "skeeto_mainui_services.h"
int g_menuPage=0,g_menuPagePrev=0,g_menuSub=0,g_navMenuSel=0,g_radarMenuSel=0,g_vmSelected=0,g_soundVolumeMenuSel=0;
bool g_menuVisible=true,g_menuParked=false;
int g_sktFontEpoch=1,g_menuFontFlags=0;int g_menuClientCw=1920,g_menuClientCh=1080;constexpr int kMenuMargin=12,kMenuDesignH=720;int g_uiFont=18;float g_menuScale=1;constexpr int kMenuDesignW=600;
int dirty=0,actions=0,refresh=0,measurements=0,searchClicks=0;int g_menuDiscoveryAnchor=-1;
static void MenuMarkDirty(){++dirty;}
static int NavMenuPageFirstSetting(int,int sub){return sub*9;}
static int RadarMenuSettingIndex(int,int sub,int){return sub*9;}
static int ViewModelMenuPageSelection(int sub){return sub;}
void CareerArchiveRefresh(){++refresh;}
static int MenuDesignLineH(){return g_uiFont+4;}
static int MenuFooterReserve(){return MenuDesignLineH()+24;}
static int MenuTextDesignW(const wchar_t* s){++measurements;return int(wcslen(s))*9;}
unsigned active=0,nextTicket=0;MainUiTextResult nextResult=MainUiTextResult::None;wchar_t input[65]{};
MainUiTextRequest MainUiServicesText(const wchar_t*,const char*,int limit){assert(limit==64);if(active)return 0;return active=++nextTicket;}
bool MainUiServicesTextOwnsWindow(MainUiTextRequest,std::uintptr_t){return false;}
void MainUiServicesCancelText(MainUiTextRequest request){if(active==request)active=0;}
MainUiTextResult MainUiServicesTakeText(MainUiTextRequest request,wchar_t* out,int cap){
 if(request!=active||nextResult==MainUiTextResult::None)return MainUiTextResult::None;
 auto result=nextResult;nextResult=MainUiTextResult::None;active=0;if(result==MainUiTextResult::Accepted)wcscpy_s(out,cap,input);return result;
}
static void MenuProcessKey(int);
static void MenuReturnReset();
static void MenuReturnApplyPending(){}
'''

TEST = r'''
static int Find(const wchar_t* name,int page=-1){for(int i=0;i<_countof(kMenuDiscoveryEntries);++i)
 if(!wcscmp(kMenuDiscoveryEntries[i].nameZh,name)&&(page<0||kMenuDiscoveryEntries[i].target.page==page))return i;assert(false);return -1;}
static void Search(const wchar_t* text){wcscpy_s(input,text);nextResult=MainUiTextResult::Accepted;MenuDiscoveryPump();}
static void Select(int entry){g_menuSearch.results[0]={entry,0};g_menuSearch.count=1;g_menuSub=0;MenuProcessKey(2);}
static int paintedBottom=0;
static void Paint(){paintedBottom=0;auto line=[](int y,COLORREF,const wchar_t*){paintedBottom=(std::max)(paintedBottom,y+MenuDesignLineH());};auto noop=[](){};auto pager=[](int){};
 MenuDiscoveryPaint(line,noop,noop,pager,0,0,0,0);}
int main(){
 // New search cancel restores original root subpage, preserving the previous query.
 g_menuSub=0;active=++nextTicket;MenuProcessKey(1);assert(g_menuPage==0&&g_menuReturnDepth==0&&searchClicks==0);active=0;
 MenuProcessKey(1);assert(active&&g_menuPage==kMenuPageSearch&&g_menuReturnDepth==1);assert(searchClicks==1);
 nextResult=MainUiTextResult::Cancelled;WorkerGate();MenuDiscoveryPump();assert(g_menuPage==0&&g_menuReturnDepth==0);g_menuParked=false;
 MenuProcessKey(1);Search(L"viewmodel 镜像");assert(g_menuSearch.count>0);
 // Worker may park the menu with its prior input-ready snapshot after the
 // dialog closes. Its already published result must still reach Paint.
 MenuProcessKey(1);wcscpy_s(input,L"viewmodel 镜像");nextResult=MainUiTextResult::Accepted;
 WorkerGate();assert(g_menuSearchRequest);auto ready=nextResult;nextResult=MainUiTextResult::None;MenuDiscoveryPump();assert(g_menuSearchRequest);nextResult=ready;MenuDiscoveryPump();assert(!wcscmp(g_menuSearch.query,L"viewmodel 镜像"));g_menuParked=false;
 const int mirror=Find(L"全局镜像反转",kMenuPageViewModel);
 Select(mirror);assert(g_menuPage==kMenuPageViewModel&&g_menuSub==0&&g_menuPagePrev==g_menuPage);
 assert(actions==0&&wcsstr(MenuDiscoveryHeading(L"title"),L"[2]"));
 MenuProcessKey(0);assert(g_menuPage==kMenuPageSearch&&!wcscmp(g_menuSearch.query,L"viewmodel 镜像"));
 // A paged result preserves list position and never calls its settings key.
 g_menuSearch.Search(L"");++g_menuDiscoveryRevision;g_menuSub=3;
 const int savedCount=g_menuSearch.count;const int selected=g_menuSearch.results[3*kMenuDiscoveryPerPage].index;
 MenuProcessKey(2);assert(g_menuPage==kMenuDiscoveryEntries[selected].target.page&&actions==0);
 MenuProcessKey(0);assert(g_menuPage==kMenuPageSearch&&g_menuSub==3&&g_menuSearch.count==savedCount);
 // Edit cancel/busy never takes another owner's request and never loses list position.
 MenuProcessKey(1);assert(active);const unsigned owned=active;
 MenuProcessKey(1);assert(active==owned);
 nextResult=MainUiTextResult::Cancelled;MenuDiscoveryPump();assert(g_menuSub==3&&g_menuSearch.count==savedCount);
 active=++nextTicket;MenuProcessKey(1);assert(active&&!g_menuSearchRequest);active=0;
 // A root action is only located, including the root's native close/back exception.
 const int changelog=Find(L"打开 Skeeto 更新日志");Select(changelog);assert(g_menuPage==0&&actions==0);
 MenuProcessKey(0);assert(g_menuPage==kMenuPageSearch&&g_menuVisible);
 // Query/classification snapshots share the existing return stack, even with nested searches.
 Select(changelog);g_menuSub=0;MenuProcessKey(1);Search(L"不存在的结果");assert(!g_menuSearch.count);
 MenuProcessKey(0);MenuProcessKey(0);assert(g_menuPage==kMenuPageSearch&&g_menuSearch.count==savedCount&&!g_menuSearch.query[0]);
 MenuProcessKey(0);assert(g_menuPage==0);g_menuSub=0;MenuProcessKey(1);assert(active);g_menuPage=kMenuPageSearch;g_menuSub=0;
 g_menuSearch.Search(L"");++g_menuDiscoveryRevision;
 Paint();Paint(); // warm layout + text cache
 int measured=measurements;
 for(int i=0;i<100;++i)Paint();assert(measurements==measured); // cached, no list measurements every frame
 for(bool en:{false,true})for(int font:{14,18,30}){g_uiEnglish=en;g_uiFont=font;Paint();}
 g_menuClientCh=720;g_menuScale=2;Paint();
 assert(paintedBottom+MenuFooterReserve()+MenuDesignLineH()+10<=(g_menuClientCh-2*kMenuMargin)/g_menuScale);
 g_menuClientCh=1080;g_menuScale=1;g_uiFont=18;Paint();
 MenuProcessKey(0);g_menuSub=0;MenuProcessKey(1);assert(active);g_menuVisible=false;MenuDiscoveryPump();assert(!active&&!g_menuSearchRequest);
 puts("PASS: production navigation, passive jumps, row anchors, nested history, pagination, query cancel/busy/close and Paint cache");
}
'''


class Navigation(unittest.TestCase):
    def test_production_navigation(self):
        spec = importlib.util.spec_from_file_location("generator", ROOT / "tools/generate_menu_discovery.py")
        gen = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(gen)
        source = (CORE / "skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        all_source = source + "\n" + "\n".join(p.read_text(encoding="utf-8-sig") for p in CORE.glob("*.h"))
        constants = dict(re.findall(r'(?:static\s+)?constexpr int\s+(kMenuPage\w+)\s*=\s*(\d+)', all_source))
        fixture = PRELUDE + "\n".join(f"constexpr int {k}={v};" for k, v in constants.items())
        fixture += "\nstruct MenuReturnFrame{" + gen.body(source, r'^struct MenuReturnFrame\s*\{') + "};\n"
        fixture += 'constexpr int kMenuReturnStackCapacity=16;MenuReturnFrame g_menuReturnStack[16]{};int g_menuReturnDepth=0;\n'
        fixture += 'static void MenuReturnReset(){g_menuReturnDepth=0;}\n'
        fixture += '#include "skeeto_menu_discovery_host.h"\n'
        fixture += 'enum class MenuRootAction:unsigned char {' + gen.body(source, r'^enum class MenuRootAction[^\n]*\{') + '};\n'
        fixture += 'struct MenuRootItem {' + gen.body(source, r'^struct MenuRootItem\s*\{') + '};\n'
        fixture += 'constexpr int kMenuRootItemsPerPage=9;static constexpr MenuRootItem kMenuRootItems[]={' + gen.body(source, r'^static constexpr MenuRootItem kMenuRootItems\[\]\s*=\s*\{') + '};\n'
        fixture += 'static int MenuRootItemCount(){return _countof(kMenuRootItems);}\n'
        fixture += 'static const MenuRootItem* MenuRootItemAt(int subpage,int digit){' + gen.body(source, r'^static const MenuRootItem\* MenuRootItemAt[^\n]*\{') + '}\n'
        fixture += 'static void EngClientCmd(const char*){++searchClicks;}static void MenuToggle(){MenuDiscoveryCancelInput();g_menuVisible=!g_menuVisible;}\n'
        fixture += 'static void SkeetoOpenRootDoc(const char*,const wchar_t*){++actions;}\n'
        key = gen.body(source, r'^static void MenuProcessKeyImpl\(int digit\)\s*\{')
        root = gen.body(key, r'^\s*if \(g_menuPage == kMenuPageRoot\)\s*\{')
        fixture += 'static void MenuProcessKeyImpl(int digit){if(MenuDiscoveryPage(g_menuPage)){MenuDiscoveryKey(digit);return;}if(g_menuPage==0){' + root + '}else if(digit==0)g_menuPage=0;else ++actions;}\n'
        fixture += 'static void WorkerGate(){' + gen.body(source,r'if \(!ingame && g_mainUiMenuContext\.load') + '}\n'
        fixture += 'static void MenuProcessKey(int digit){' + gen.body(source, r'^static void MenuProcessKey\(int digit\)\s*\{') + '}\n' + TEST
        with tempfile.TemporaryDirectory(prefix="skeeto-find-navigation-") as tmp:
            folder = Path(tmp)
            (folder / "fixture.cpp").write_text(fixture, encoding="utf-8")
            vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
            vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
            bat = folder / "compile.bat"
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX /I"{CORE}" fixture.cpp /Fe:fixture.exe user32.lib\nexit /b %errorlevel%\n', encoding="utf-8")
            result = subprocess.run(["cmd", "/c", str(bat)], cwd=folder, capture_output=True, text=True, errors="replace")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(folder / "fixture.exe")], capture_output=True, text=True, errors="replace")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            print(result.stdout.strip())


if __name__ == "__main__":
    unittest.main()
