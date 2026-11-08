"""Run actual main-menu preference/key functions with a fake engine on MSVC x86."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "src/skeeto/core"


def function(source, marker):
    start = source.index(marker)
    opening = source.index("{", start)
    depth = 1
    at = opening + 1
    while depth:
        if source[at] == "{":
            depth += 1
        elif source[at] == "}":
            depth -= 1
        at += 1
    return source[start:at] + "\n"


def main():
    proxy = (CORE / "skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    editor = (CORE / "skeeto_mainui_editor.h").read_text(encoding="utf-8-sig")
    services = (CORE / "skeeto_mainui_services.cpp").read_text(encoding="utf-8-sig")
    declarations = proxy[proxy.index("static const char* kMainUiShortcutCommands"):proxy.index("static int MainUiEngineKeyCode")]
    actions = editor[editor.index("struct MainUiActionDef"):editor.index("static const wchar_t* MainUiActionTitle")]
    source = r'''
#include <Windows.h>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>
static DWORD clockNow=1000;
#define GetTickCount() clockNow
static bool connected=false,inGame=false,loading=false;
static void* g_engine=(void*)1;
static std::atomic_bool g_ready{true};
static bool g_optMainMenuUi=true,g_optMainUiRememberServers=false,g_optMainUiServerQuery=false,g_optMainUiBattleArchive=false;
static bool g_mainUiBackdropFollow=false,g_mainUiFixedShortcuts=true,g_mainUiHotkeySeed=false,g_mainUiInputSeed=false,g_mainUiEditing=false;
static bool g_mainUiShowHints=true,g_mainUiShowVersion=true,g_mainUiShowGameName=true,g_mainUiShowLogo=true,g_mainUiShowAccentLine=true,g_mainUiTextAutoScale=true;
static int g_mainUiBackdropRgb=0,g_mainUiAccent=0,g_mainUiBackdrop=0,g_mainUiQuickMode=0,g_mainUiQuickDifficulty=1;
static int g_mainUiNativeKey=0,g_mainUiHomeKey=0,g_mainUiEditKey=0,g_mainUiKeyCapture=-1,g_mainUiEditTextField=0;
static int g_mainUiFlyout=-1,g_mainUiPanel=0,g_mainUiPressed=-1,g_mainUiFocus=0,g_mainUiNotice=0;
static DWORD g_mainUiBindingsAt=0,g_mainUiShortcutStamp=0,g_mainUiConfigSaveAt=0;
static unsigned g_mainUiTextRevision=0;
static bool g_mainUiConfigDirty=false;
static std::atomic_int g_mainUiRequest{0};
static std::map<std::string,std::string> bindings;
static std::vector<std::string> commands;
static void MainUiSettingsChanged(){g_mainUiConfigDirty=true;}
static void MainUiSetEnabled(bool v){g_optMainMenuUi=v;}
static bool SkeetoFeaturesOn(){return true;}
static bool EngConnected(){return connected;}
static bool EngInGame(){return inGame;}
static bool EngDrawingLoading(){return loading;}
static const char* EngLookupBinding(const char* command){for(auto& p:bindings)if(p.second==command)return p.first.c_str();return nullptr;}
static void EngClientCmdUnrestricted(const char* command){commands.emplace_back(command);char key[32]{},value[96]{};
 if(sscanf(command,"bind %31s %95s",key,value)==2)bindings[key]=value;
 else if(sscanf(command,"unbind \"%31[^\"]\"",key)==1)bindings.erase(key);
}
static bool Utf8(const char* s,wchar_t* out,int cap){return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,out,cap)>0;}
'''
    source += function(services, "bool MainUiExternalValid(")
    source += f'#include "{(CORE / "skeeto_mainui_services.h").as_posix()}"\n'
    source += 'struct Job {int type=0,kind=0;char parameter[512]{};MainUiCareerReport career{};}; static int queuedOpen=-1; static MainUiCareerReport queuedCareer{}; static bool Post(const Job& j){queuedOpen=j.kind;queuedCareer=j.career;return true;}\n'
    source += function(services, "bool MainUiServicesOpen(")
    source += function(services, "bool MainUiServicesOpenReport(")

    source += f'#include "{(CORE / "skeeto_ui_text.h").as_posix()}"\n'
    source += f'#include "{(CORE / "skeeto_mainui_services.h").as_posix()}"\n'
    source += f'#include "{(CORE / "skeeto_mainui_layout.h").as_posix()}"\n'
    source += declarations + function(proxy, "static WORD HoldSemiReloadVirtualKey(")
    source += function(proxy, "static int MainUiEngineKeyCode(")
    for marker in ["static void MainUiRefreshBindings(", "static void MainUiShortcutRequest(", "static void MainUiShortcutsPump(", "static bool MainUiBindShortcut("]:
        source += function(proxy, marker)
    source += function(proxy, "static COLORREF MainUiBackdropColor(COLORREF accent){")
    source += actions
    source += 'int MainUiServicesReadCurrent(char*,int){return 0;} bool MainUiServicesSaveCurrent(const char*){return true;} void MainUiServicesMarkCurrentInvalid(){}\n'
    source += f'#include "{(CORE / "skeeto_mainui_config.h").as_posix()}"\n'
    source += r'''
static int g_mainUiSw=1920,g_mainUiSh=1080,drawCalls=0,registerCalls=0;
static bool g_mainUiMouseMode=false;
static std::atomic_bool g_mainUiUnderlay{false};
static bool MainUiMouse(int*,int*){return false;}
static int MainUiHit(int,int){return -1;}
static bool MainUiElementVisible(int){return true;}
static void MainUiDrawFriends(const MainUiLayoutSlot&,int,int,int,int,bool reg){if(reg)++registerCalls;else ++drawCalls;}
static void MainUiDrawElement(const MainUiLayoutSlot&,int,int,int,int){++drawCalls;}
struct Region{int x,y,w,h,id;};static std::vector<Region> regions;
static void MainUiDrawSlot(const MainUiLayoutSlot& b,int x,int y,int w,int h,bool reg,int){if(reg){++registerCalls;regions.push_back({x,y,w,h,b.id});}else ++drawCalls;}
'''
    source += function(editor, "static void MainUiDrawLayout(bool registration)")
    background = function(proxy, '\t\telse if (g_mainUiPanel==1) {')
    background = background[background.index('{') + 1:background.rfind('}')]
    source += r'''
static bool g_optBackgroundEngine=true,g_optBackgroundRandomStart=false;
static int g_backgroundFit=0;
static wchar_t g_mainUiBgName[]=L"test.mp4";
static const wchar_t* BackgroundFitNameW(int){return L"Cover";}
static void BackgroundPage(bool registerButtons,int sw,int sh){
 float scale=(sw-48.f)/1040.f;float v=(sh-48.f)/600.f;if(v<scale)scale=v;
 auto U=[=](int n){return(int)(n*scale+.5f);};int ox=(sw-U(1040))/2,oy=(sh-U(600))/2,rx=ox+U(312),rw=U(728),hover=-1;
 COLORREF white=RGB(244,245,246),muted=RGB(145,155,167);
 auto text=[](int,int,int,COLORREF,const wchar_t*,int){};
 auto button=[&](int x,int y,int w,int h,int id,const wchar_t*){if(registerButtons)regions.push_back({x,y,w,h,id});};
 auto slider=[&](int y,int action,const wchar_t*){if(registerButtons)regions.push_back({rx+U(250),y-U(4),U(358),U(30),action});};
'''
    source += background + '\n}\n'
    source += r'''
int main(){
 MainUiCareerReport career{};career.stats.infectedKills={true,12345};strcpy_s(career.weaponNames[0],"AK-47");
 assert(MainUiServicesOpenReport(career)&&queuedOpen==4&&queuedCareer.stats.infectedKills.value==12345);
 career.stats.infectedKills.value=9;career.weaponNames[0][0]='x';assert(queuedCareer.stats.infectedKills.value==12345&&!strcmp(queuedCareer.weaponNames[0],"AK-47"));
 assert(MainUiServicesOpen(4,nullptr)&&queuedOpen==4);
 assert(MainUiServicesOpen(3,nullptr)&&queuedOpen==3);
 assert(!MainUiServicesOpen(5,nullptr)&&!MainUiServicesOpen(-1,nullptr));
 MainUiLayoutReset();assert(!MainUiLayoutHasNavigationHint());
 auto& hint=g_mainUiLayout[g_mainUiLayoutCount++];hint=MainUiLayoutBlank();hint.action=935;
 assert(MainUiLayoutHasNavigationHint());hint.visible=0;assert(!MainUiLayoutHasNavigationHint());
 hint.visible=1;hint.opacity=0;assert(!MainUiLayoutHasNavigationHint());
 MainUiLayoutReset();

 // Actual background-page branch: one pause/progress row and disjoint hitboxes.
 const int screens[][2]={{640,480},{1024,768},{1920,1080},{2560,1080}};
 for(const auto& screen:screens){regions.clear();BackgroundPage(true,screen[0],screen[1]);bool pause=false,progress=false;
  for(size_t i=0;i<regions.size();++i){const auto& a=regions[i];assert(a.w>0&&a.h>0&&a.x>=0&&a.y>=0&&a.x+a.w<=screen[0]&&a.y+a.h<=screen[1]);
   if(a.id==928){assert(!pause);pause=true;}if(a.id==929){assert(!progress);progress=true;}
   for(size_t j=i+1;j<regions.size();++j){const auto& b=regions[j];assert(a.x+a.w<=b.x||b.x+b.w<=a.x||a.y+a.h<=b.y||b.y+b.h<=a.y);}
  }assert(pause&&progress);regions.clear();BackgroundPage(false,screen[0],screen[1]);assert(regions.empty());
 }
 // Fixed defaults must never issue bind commands or alter occupied engine keys.
 bindings["F3"]="my_existing_action";auto original=bindings;MainUiShortcutsPump();
 assert(g_mainUiNativeKey==VK_F3&&g_mainUiHomeKey==VK_F4&&g_mainUiEditKey==VK_F5);
 assert(commands.empty()&&bindings==original);
 bindings["F6"]="skeeto_mainmenu_home";clockNow+=250;MainUiShortcutsPump();assert(g_mainUiHomeKey==VK_F4);
 g_mainUiFixedShortcuts=false;clockNow+=250;MainUiShortcutsPump();assert(g_mainUiHomeKey==VK_F6);
 assert(MainUiBindShortcut(1,0));clockNow+=250;MainUiShortcutsPump();assert(!g_mainUiHomeKey&&bindings.count("F6")==0);
 assert(MainUiBindShortcut(1,VK_F7));clockNow+=250;MainUiShortcutsPump();assert(g_mainUiHomeKey==VK_F7);
 g_mainUiRequest=0;connected=true;MainUiShortcutRequest(1);assert(g_mainUiRequest==0);connected=false;
 inGame=true;MainUiShortcutRequest(2);assert(g_mainUiRequest==0);inGame=false;
 loading=true;MainUiShortcutRequest(0);assert(g_mainUiRequest==0);loading=false;
 g_mainUiEditTextField=1;MainUiShortcutRequest(0);assert(g_mainUiRequest==0);g_mainUiEditTextField=0;
 MainUiShortcutRequest(2);assert(g_mainUiRequest==3);g_mainUiRequest=0;MainUiShortcutRequest(2);assert(g_mainUiRequest==0);
 clockNow+=100;g_mainUiEditing=true;MainUiShortcutRequest(2);assert(g_mainUiRequest==5);g_mainUiEditing=false;
 assert(MainUiLayoutParamValid(900,"[2001:db8::1]:27015@27016"));assert(MainUiLayoutParamValid(900,"::1"));
 assert(!MainUiLayoutParamValid(900,"[::1]:0"));assert(!MainUiLayoutParamValid(900,"[::1]:27015;quit"));assert(!MainUiLayoutParamValid(900,"[::1"));
 // All four numeric variable actions are contiguous in the actual picker.
 int indices[128]{};int n=MainUiActionGroupIndices(5,indices);int at=-1;
 for(int i=0;i<n;++i)if(kMainUiActions[indices[i]].action==919)at=i;
 assert(at>=0&&at+3<n);assert(kMainUiActions[indices[at+1]].action==920&&kMainUiActions[indices[at+2]].action==921&&kMainUiActions[indices[at+3]].action==910);
 assert(MainUiActionGroup(27)==MainUiActionGroup(3));
 assert(MainUiActionGroup(928)==MainUiActionGroup(3)&&MainUiActionGroup(929)==MainUiActionGroup(3));
 // Shareable appearance roundtrip, invalid colors leave the active layout intact.
 MainUiLayoutReset();for(int i=0;i<g_mainUiLayoutCount;++i)assert(g_mainUiLayout[i].action!=935);g_mainUiBackdropRgb=RGB(30,60,90);g_mainUiBackdropFollow=true;g_mainUiBackdrop=35;
 for(int action:{928,929}){assert(MainUiLayoutActionValid(action));for(int i=0;i<g_mainUiLayoutCount;++i)assert(g_mainUiLayout[i].action!=action);}
 static char body[kMainUiProfileBytes]{};assert(MainUiConfigCapture(body));
 g_mainUiBackdropRgb=0;g_mainUiBackdropFollow=false;assert(MainUiConfigApply(body));
 assert(g_mainUiBackdropRgb==(int)RGB(30,60,90)&&g_mainUiBackdropFollow&&g_mainUiBackdrop==35);
 assert(MainUiBackdropColor(RGB(4,8,12))==RGB(1,2,3));g_mainUiBackdropFollow=false;assert(MainUiBackdropColor(RGB(1,2,3))==RGB(30,60,90));
 assert(MainUiConfigCapture(body));std::string invalid=body;auto pos=invalid.find("backdrop_rgb=");auto end=invalid.find('\n',pos);invalid.replace(pos,end-pos,"backdrop_rgb=16777216");
 int count=g_mainUiLayoutCount;assert(!MainUiConfigApply(invalid.data()));assert(g_mainUiLayoutCount==count&&g_mainUiBackdropRgb==(int)RGB(30,60,90));
 // Importing someone's scheme must never import their keyboard bindings.
 int key=g_mainUiHomeKey;auto before=bindings;g_mainUiFixedShortcuts=false;assert(MainUiConfigApply(body));assert(g_mainUiHomeKey==key&&bindings==before&&!g_mainUiFixedShortcuts);
 // Local opt-out persists across startup; an older development file missing the key uses fixed defaults.
 g_mainUiFixedShortcuts=false;assert(MainUiConfigCapture(body));g_mainUiFixedShortcuts=true;assert(MainUiConfigApply(body,true));assert(!g_mainUiFixedShortcuts);
 std::string missing=body;pos=missing.find("fixed_shortcuts=");end=missing.find('\n',pos);missing.erase(pos,end-pos+1);assert(MainUiConfigApply(missing.data(),true));assert(g_mainUiFixedShortcuts);
 // Use the production draw/registration function: transparent flyouts suppress every root item.
 g_mainUiFlyout=-1;MainUiDrawLayout(false);MainUiDrawLayout(true);assert(drawCalls>0&&registerCalls>0);
 drawCalls=registerCalls=0;g_mainUiFlyout=12;g_mainUiBackdrop=0;MainUiDrawLayout(false);MainUiDrawLayout(true);assert(!drawCalls&&!registerCalls);
 g_mainUiBackdrop=100;MainUiDrawLayout(false);MainUiDrawLayout(true);assert(!drawCalls&&!registerCalls);
 g_mainUiFlyout=-1;MainUiDrawLayout(false);MainUiDrawLayout(true);assert(drawCalls>0&&registerCalls>0);
 puts("PASS: fixed keys without bind mutation, opt-out/custom bind/unbind, in-game/loading/text gates, duplicate-edge suppression, action grouping, appearance roundtrip and invalid-color rollback, imported scheme key isolation, local opt-out persistence, transparent submenu draw/hit suppression and restore.");
}
'''
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-mainui-preferences-") as temp:
        folder = Path(temp)
        (folder / "fixture.cpp").write_text(source, encoding="utf-8")
        (folder / "run.bat").write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvars32.bat" >nul\nif errorlevel 1 exit /b 1\ncl /nologo /EHsc /std:c++17 /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS fixture.cpp /Fe:fixture.exe /link user32.lib\nif errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)


if __name__ == "__main__":
    main()
