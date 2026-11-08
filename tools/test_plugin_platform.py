"""Build real x86 DLL fixtures and exercise the actual optional plugin host."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
from test_nativeui_chat_binding import definition

ROOT = Path(__file__).resolve().parents[1]
SDK = ROOT / "src/skeeto/plugin_sdk/include"
CORE = ROOT / "src/skeeto/core"

FIXTURE = r'''
#include "skeeto_plugin_api.h"
#include <Windows.h>
#include <limits>
static int enabled=0,actions=0;static double number=0.25;
static int32_t SKT_CALL Ready(void*,const SktHostApi* host){
 if(!host || host->size<sizeof(SktHostApi) || !host->plugin_directory_utf8)return 31;
 return MODE==3?13:0;
}
static int32_t SKT_CALL Build(void*,uint32_t page_id,SktMenuPage* p){
 if(MODE==15 && page_id)return 23;
 p->title_zh="Fixture";p->title_en="Fixture";
 if(MODE==4){p->count=p->capacity+1;return 0;}
 p->count=page_id?1:(MODE==13 || (MODE==16 && !actions))?12:6;
 for(unsigned i=0;i<p->count;i++){p->items[i]={};p->items[i].size=sizeof(SktMenuItem);p->items[i].id=i+1;p->items[i].label_en="Item";}
 p->items[0].type=SKT_MENU_INFO;p->items[0].text_en=actions?"changed":"initial";
 if(page_id)return 0;
 p->items[1].type=SKT_MENU_TOGGLE;p->items[1].value=enabled;
 p->items[2].type=SKT_MENU_NUMBER;p->items[2].value=number;p->items[2].min_value=0;p->items[2].max_value=1;p->items[2].step=0.01;
 p->items[3].type=SKT_MENU_CHOICE;p->items[3].choices_en="Quiet|Normal|Loud";p->items[3].value=1;
 p->items[4].type=SKT_MENU_SUBMENU;p->items[4].page_id=9;
 p->items[5].type=SKT_MENU_ACTION;p->items[5].flags=SKT_MENU_DISABLED;
 if(MODE==5)p->items[2].step=0;
 if(MODE==6)p->items[2].id=2;
 if(MODE==7)p->items[0].label_en="\xff";
 if(MODE==8)p->items[3].choices_en="One||Three";
 if(MODE==9)p->items[2].value=std::numeric_limits<double>::quiet_NaN();
 if(MODE==10)p->items[0].size=1;
 if(MODE==11){static char long_label[400];for(int i=0;i<399;i++)long_label[i]='a';long_label[399]=0;p->items[0].label_en=long_label;}
 if(MODE==13){p->items[10].type=SKT_MENU_SUBMENU;p->items[10].page_id=9;}
 if(MODE==16 && !page_id && !actions)p->items[10].type=SKT_MENU_ACTION;
 return 0;
}
static int32_t SKT_CALL Action(void*,uint32_t,uint32_t id,double v){++actions;if(id==2)enabled=(int)v;if(id==3)number=v;return MODE==12?19:0;}
extern "C" __declspec(dllexport) int32_t SKT_CALL SkeetoPlugin_Query(uint32_t host,SktPluginInfo* p){
 if((host>>16)!=1 || p->size<sizeof(*p))return 1;
 if(MODE==14){auto stop=(void (__cdecl*)())GetProcAddress(GetModuleHandleW(nullptr),"StopHost");if(stop)stop();}
 p->size=sizeof(*p);p->api_version=MODE==1?0x20000:MODE==2?0x10001:SKT_PLUGIN_API_VERSION;
 p->id=MODE==17?"Fixture.Example":MODE==18?"fixture.example.":"fixture.example";p->name_en="Fixture";p->author="Tests";p->version="1";
 if(MODE>=19 && MODE<=21){static char boundary_id[49]{};const int n=MODE+27;for(int i=0;i<n;++i)boundary_id[i]='a';p->id=boundary_id;}
 p->context=nullptr;p->on_host_ready=Ready;p->build_page=Build;p->on_action=Action;return 0;
}
'''

HARNESS = r'''
#include "skeeto_plugins.h"
#include <cassert>
#include <cwchar>
#include <atomic>
#include <cmath>
std::atomic_bool g_run{true};
extern "C" __declspec(dllexport) void __cdecl StopHost(){g_run=false;}
std::atomic_bool g_ready{true},g_uiEnglish{false};
static bool g_menuVisible=true,g_menuParked=false,g_optPluginPlatform=false,g_settingsDirty=false,g_optForceMenuBind=true;
static int g_menuPage=134,g_menuSub=0,g_menuPagePrev=-1;
static int g_navMenuSel=0,g_radarMenuSel=0,g_vmSelected=0;
static constexpr int kMenuPageViewModel=999;
static int NavMenuPageFirstSetting(int,int){return 0;}
static int RadarMenuSettingIndex(int,int,int){return 0;}
static int ViewModelMenuPageSelection(int){return 0;}
static DWORD g_settingsSaveAt=0;
static constexpr int kMenuPagePluginPlatform=134,kMenuPagePluginPage=135,kMenuItemsPerPage=9,kPaintUiPanels=2;
static void MenuMarkDirty(){}
static int MenuPageCountForItems(int n){return n?(n+8)/9:1;}
#include "menu_adapter.inc"
int wmain(int argc,wchar_t** argv){
 assert(argc==3);const int mode=_wtoi(argv[2]);
 skplugins::Start(argv[1],mode!=100);
 if(mode==100){assert(!skplugins::Started() && skplugins::Count()==0 && !skplugins::NeedsPaint());assert(!GetModuleHandleW(L"fixture.dll"));return 0;}
 if(mode==101){assert(skplugins::Started() && skplugins::Count()==0 && !skplugins::NeedsPaint());return 0;}
 if(mode==102){assert(skplugins::Count()==32 && skplugins::Limited());return 0;}
 if(mode==103){assert(skplugins::Count()==2);assert(skplugins::At(1)->state==skplugins::State::DuplicateId);return 0;}
 if(mode==113){assert(skplugins::Count()==2);assert(skplugins::At(1)->state==skplugins::State::DuplicateId);return 0;}
 if(mode==118){
  assert(skplugins::Count()==18);g_menuPagePrev=kMenuPagePluginPlatform;g_menuSub=2;
  PluginMenuStep(1);MenuSubSync();assert(g_menuPage==kMenuPagePluginPage && g_pluginMenuSlot==17);
  PluginMenuStep(0);MenuSubSync();assert(g_menuPage==kMenuPagePluginPlatform && g_menuSub==2);
  PluginMenuStep(0);MenuSubSync();assert(g_menuPage==0 && g_menuSub==2);return 0;
 }
 if(mode==109){assert(skplugins::Count()==1 && skplugins::Stats().loads==1 && !skplugins::NeedsPaint());assert(!GetModuleHandleW(L"zremaining.dll"));return 0;}
 assert(skplugins::Count()==1);const auto* r=skplugins::At(0);assert(r);
 if(mode==1 || mode==2){assert(r->state==skplugins::State::ApiMismatch);return 0;}
 if(mode==104){assert(r->state==skplugins::State::EntryMissing);return 0;}
 if(mode==105){assert(r->state==skplugins::State::LoadError && r->error==ERROR_BAD_EXE_FORMAT);return 0;}
 if(mode==108){assert(r->state==skplugins::State::LoadError && r->error==ERROR_MOD_NOT_FOUND);return 0;}
 if(mode==114 || mode==117){assert(r->state==skplugins::State::InvalidDescriptor);return 0;}
 assert(r->state==skplugins::State::Loaded);assert(skplugins::NeedsPaint());
 if(mode==115 || mode==116)assert(strlen(r->id)==(mode==115?46:47));
 skplugins::PaintPump(false);assert(!skplugins::NeedsPaint());
 if(mode==3){assert(r->state==skplugins::State::InitError && r->error==13);return 0;}
 if(mode==107){
  g_menuPage=kMenuPagePluginPage;assert(MenuEatsButtonCode(28));g_menuPage=0;assert(!MenuEatsButtonCode(28));
  g_menuPage=kMenuPagePluginPlatform;
  PluginMenuStep(1);assert(g_optPluginPlatform && skplugins::Stats().loads==1);
  PluginMenuEnter(0);PluginMenuPaintPump(2);assert(skplugins::CurrentPage().count==12);
  PluginMenuStep(3);assert(!skplugins::Pending());PluginMenuAdjust(1);assert(skplugins::Pending());
  PluginMenuPaintPump(2);assert(fabs(skplugins::CurrentPage().items[2].value-0.26)<1e-9);
  g_menuSub=1;PluginMenuStep(2);PluginMenuPaintPump(2);assert(g_pluginMenuDepth==1 && skplugins::CurrentPage().page_id==9);
  PluginMenuStep(0);assert(g_menuSub==0);PluginMenuPaintPump(2);
  assert(g_menuSub==1 && g_pluginMenuDepth==0 && skplugins::CurrentPage().page_id==0);
  auto c=skplugins::Stats();g_uiEnglish=true;PluginMenuPaintPump(2);assert(skplugins::Stats().builds==c.builds+1);
  PluginMenuPaintPump(2);assert(skplugins::Stats().builds==c.builds+1);
  PluginMenuRefresh();g_run=false;PluginMenuPaintPump(2);assert(skplugins::Stats().builds==c.builds+1);return 0;
 }
 if(mode==110){
  PluginMenuEnter(0);PluginMenuPaintPump(2);PluginMenuStep(5);PluginMenuPaintPump(2);
  assert(r->state==skplugins::State::PageError && g_pluginMenuDepth==1);
  PluginMenuStep(0);assert(g_menuPage==kMenuPagePluginPlatform && g_pluginMenuDepth==0);return 0;
 }
 if(mode==111){
  PluginMenuEnter(0);PluginMenuPaintPump(2);assert(skplugins::CurrentPage().count==12);
  g_menuSub=1;PluginMenuStep(2);PluginMenuPaintPump(2);
  assert(skplugins::CurrentPage().count==6 && g_menuSub==0);return 0;
 }
 assert(r->state==skplugins::State::Active);assert(skplugins::RequestPage(0,0));assert(!skplugins::RequestPage(0,9));
 skplugins::PaintPump(false);
 if(mode>=4 && mode<=11){assert(r->state==skplugins::State::PageError && !skplugins::CurrentPage().valid);assert(!skplugins::RequestPage(0,0));return 0;}
 if(mode==106){
  assert(!strcmp(r->id,"skeeto.hello_menu"));assert(skplugins::CurrentPage().count==6);
  assert(skplugins::RequestAction(2,1));skplugins::PaintPump(false);assert(skplugins::CurrentPage().items[1].value==1);
  assert(skplugins::RequestPage(0,1));skplugins::PaintPump(true);assert(skplugins::CurrentPage().count==2);return 0;
 }
 const auto& p=skplugins::CurrentPage();assert(p.valid && p.count==6 && p.items[3].choice_count==3);
 auto before=skplugins::Stats();
 for(int i=0;i<50;i++)skplugins::PaintPump(false);
 auto after=skplugins::Stats();assert(before.builds==after.builds && before.actions==after.actions && !skplugins::NeedsPaint());
 assert(!skplugins::RequestAction(1,0)); // Read-only.
 assert(!skplugins::RequestAction(6,0)); // Disabled.
 assert(!skplugins::RequestAction(3,2)); // Out of range.
 assert(!skplugins::RequestAction(4,1.5)); // Non-integral choice.
 assert(skplugins::RequestAction(2,1));skplugins::PaintPump(false);
 if(mode==12){assert(r->state==skplugins::State::ActionError && r->error==19 && !p.valid);return 0;}
 assert(p.valid && p.items[1].value==1 && !wcscmp(p.items[0].text,L"changed"));
 assert(skplugins::RequestPage(0,9));skplugins::PaintPump(true);assert(p.valid && p.page_id==9 && p.count==1);
 assert(skplugins::Count()==1); // No implicit re-scan.
 return 0;
}
'''


def main():
    assert (CORE / "skeeto_plugins.cpp").is_file(), "plugin host implementation missing"
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    assert vs, "MSVC x86 required"
    with tempfile.TemporaryDirectory(prefix="skeeto-plugin-test-") as directory:
        tmp = Path(directory)
        (tmp / "fixture.cpp").write_text(FIXTURE, encoding="utf-8")
        (tmp / "host.cpp").write_text(HARNESS, encoding="utf-8")
        menu=(CORE/"skeeto_plugins_menu.h").read_text(encoding="utf-8")
        prefix=menu.split('static const wchar_t* PluginStateText',1)[0]
        helpers=['PluginMenuSnapshotReady','PluginMenuPageCount','PluginMenuRefresh','PluginMenuPaintPump','PluginMenuEnter','PluginMenuStep','PluginMenuAdjust']
        key_handler=definition((CORE/"skeeto_proxy.cpp").read_text(encoding="utf-8"),'MenuEatsButtonCode')
        sub_sync=definition((CORE/"skeeto_proxy.cpp").read_text(encoding="utf-8"),'MenuSubSync')
        (tmp/"menu_adapter.inc").write_text(prefix+'\n'.join(definition(menu,name) for name in helpers)+'\n'+key_handler+'\n'+sub_sync,encoding="utf-8")
        (tmp / "c_header.c").write_text('#include "skeeto_plugin_api.h"\n_Static_assert(sizeof(void*)==4,"x86");\n_Static_assert(sizeof(SktPluginInfo)==44,"ABI");\n_Static_assert(sizeof(SktMenuItem)==80,"ABI");\nint main(void){return 0;}\n', encoding="utf-8")
        commands = [f'@call "{vs}/VC/Auxiliary/Build/vcvarsall.bat" x86 >nul', '@if errorlevel 1 exit /b 1']
        flags = f'/nologo /MT /EHsc /std:c++17 /utf-8 /W3 /WX /D_CRT_SECURE_NO_WARNINGS /I"{SDK}" /I"{CORE}"'
        commands += [f'cl {flags} host.cpp "{CORE / "skeeto_plugins.cpp"}" /Fehost.exe', '@if errorlevel 1 exit /b 1', f'cl /nologo /TC /std:c11 /W3 /WX /I"{SDK}" c_header.c /Fec_header.exe', '@if errorlevel 1 exit /b 1']
        future=tmp/"future_sdk";future.mkdir()
        header=(SDK/"skeeto_plugin_api.h").read_text(encoding="utf-8")
        (future/"skeeto_plugin_api.h").write_text(header.replace('UINT32_C(0x00010000)','UINT32_C(0x00010001)').replace('SktOnAction on_action;','SktOnAction on_action;\n    uint32_t optional_tail;'),encoding="utf-8")
        future_flags=flags.replace(f'/I"{SDK}"',f'/I"{future}"')
        commands += [f'cl {future_flags} host.cpp "{CORE / "skeeto_plugins.cpp"}" /Fehost_minor.exe', '@if errorlevel 1 exit /b 1']
        (tmp/'missing_dep.cpp').write_text('extern "C" __declspec(dllexport) int Needed(){return 1;}\n',encoding='utf-8')
        (tmp/'needs.cpp').write_text('#include "skeeto_plugin_api.h"\nextern "C" __declspec(dllimport) int Needed();\nextern "C" __declspec(dllexport) int32_t SKT_CALL SkeetoPlugin_Query(uint32_t,SktPluginInfo*){return Needed();}\n',encoding='utf-8')
        commands += [f'cl {flags} /LD missing_dep.cpp /FeSkeetoTestMissing_20261025.dll','@if errorlevel 1 exit /b 1',f'cl {flags} /LD needs.cpp SkeetoTestMissing_20261025.lib /Feneeds.dll','@if errorlevel 1 exit /b 1']
        for mode in range(22):
            commands += [f'cl {flags} /LD /DMODE={mode} fixture.cpp /Fefixture{mode}.dll', '@if errorlevel 1 exit /b 1']
        (tmp / "build.cmd").write_text('\n'.join(commands), encoding="utf-8")
        built = subprocess.run(["cmd", "/c", "build.cmd"], cwd=tmp, capture_output=True)
        if built.returncode:
            print(built.stdout.decode("utf-8", errors="replace"));print(built.stderr.decode("utf-8", errors="replace"))
        assert built.returncode == 0, "MSVC fixture build failed"
        # Copy the SDK out of the repository and build its sample independently.
        independent=tmp/"independent_sdk"
        shutil.copytree(SDK.parent,independent,ignore=shutil.ignore_patterns('out'))
        subprocess.run(["cmd","/c",str(independent/"examples/HelloMenu/build.bat"),str(tmp/"sample")],check=True,capture_output=True)
        failed=[]
        for mode in [100, 101, *range(13), 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 113, 114, 115, 116, 117, 118]:
            root = tmp / f"root{mode}"
            plugins = root / "bin/skeeto/plugins"
            if mode != 101:
                plugins.mkdir(parents=True)
                if mode == 102:
                    for n in range(35):
                        (plugins / f"invalid{n:02d}.dll").write_bytes(b"invalid")
                else:
                    shutil.copyfile(tmp / f"fixture{mode if mode in range(13) else 0}.dll", plugins / "fixture.dll")
                    if mode == 103:
                        shutil.copyfile(tmp / "fixture0.dll", plugins / "zduplicate.dll")
                    if mode == 113:
                        shutil.copyfile(tmp / "fixture17.dll", plugins / "zduplicate.dll")
                    if mode == 114:
                        shutil.copyfile(tmp / "fixture18.dll", plugins / "fixture.dll")
                    if mode in (115,116,117):
                        shutil.copyfile(tmp / f"fixture{mode-96}.dll", plugins / "fixture.dll")
                    if mode == 118:
                        for n in range(17):
                            shutil.copyfile(tmp / "fixture0.dll", plugins / f"zduplicate{n:02d}.dll")
                    if mode == 104:
                        shutil.copyfile(tmp / "c_header.exe", plugins / "fixture.dll")
                    if mode == 105:
                        image=bytearray((plugins/"fixture.dll").read_bytes());pe=int.from_bytes(image[60:64],'little');image[pe+4:pe+6]=(0x8664).to_bytes(2,'little');(plugins/"fixture.dll").write_bytes(image)
                    if mode == 106:
                        shutil.copyfile(tmp/"sample/HelloMenu.dll",plugins/"fixture.dll")
                    if mode == 107:
                        shutil.copyfile(tmp/"fixture13.dll",plugins/"fixture.dll")
                    if mode in (110,111):
                        shutil.copyfile(tmp/f"fixture{15 if mode==110 else 16}.dll",plugins/"fixture.dll")
                    if mode == 108:
                        shutil.copyfile(tmp/'needs.dll',plugins/'fixture.dll')
                    if mode == 109:
                        shutil.copyfile(tmp/'fixture14.dll',plugins/'fixture.dll')
                        shutil.copyfile(tmp/'fixture0.dll',plugins/'zremaining.dll')
            result=subprocess.run([str(tmp / "host.exe"), str(root), str(mode)],timeout=20)
            if result.returncode:failed.append(mode)
        assert not failed,f"failed cases: {failed}"
        # Same unchanged API-1.0 DLL, host recompiled with a larger API-1.1 tail.
        subprocess.run([str(tmp/"host_minor.exe"),str(tmp/"root0"),"0"],check=True,timeout=20)
    print("plugin host: x86 C ABI, default-off, one-shot loading, API/ID failures, limits, menu validation and dispatch passed")


if __name__ == "__main__":
    main()
