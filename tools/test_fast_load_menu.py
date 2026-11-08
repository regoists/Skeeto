"""Execute the production loading menu/settings wiring in an x86 fixture.
No VGUI/input rendering is reproduced here; runtime UI remains separate.
"""
from pathlib import Path
import test_fast_load as native

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8")
draw=source.split('} else if (g_menuPage == kMenuPageFastLoad || g_menuPage == kMenuPageFastLoadExperiments) {',1)[1].split('} else if (g_menuPage == kMenuPageViewModel)',1)[0]
assert 'pageNav(MenuPageSubCount(g_menuPage))' in draw,'multi-page loading settings must draw the shared red navigation hint'

def between(start, end):
    return source.split(start, 1)[1].split(end, 1)[0]

table = "enum class FastLoadMenuAction" + between("enum class FastLoadMenuAction", "static constexpr MenuRootItem kMenuRootItems")
transition_row=between("{UiId::FastLoadLocalTransition,", "static const FastLoadMenuItem*")
assert "取消本地过关切图等待(Beta)" in transition_row
assert "此功能目前有bug，可能会造成出安全屋后异常触发加载。" not in transition_row
assert "此功能目前有bug，可能会造成出安全屋后异常触发加载。" in draw
assert "RGB(255, 56, 56)" in draw and "FastLoadLocalTransitionBug" in draw
english=(ROOT / "src/skeeto/core/skeeto_ui_en.inc").read_text(encoding="utf-8")
assert 'L"[5] Skip local chapter transition wait (Beta) %s"' in english
assert "This feature currently has a bug and may trigger an unexpected load after leaving the safe room." in english
# Include the production fast-load parser through the chat-timer key.
start = source.index('\tif (sscanf(line, "fast_load_master=')
end = source.index('g_optFastLoadLessTextureWarmup = v != 0; return true; }', start) + len('g_optFastLoadLessTextureWarmup = v != 0; return true; }')
settings = source[start:end]
save = 'fprintf(f, "fast_load_master=' + between('fprintf(f, "fast_load_master=', ';') + ';'
start = source.index('static void MenuProcessKeyImpl(int digit) {')
end = source.index('\n\tif (g_menuPage == kMenuPageViewModel)', start)
key = source[start:end] + '\n}'

native.FIXTURE = r'''
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include "skeeto_fast_load.h"
#include "skeeto_ui_text.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static bool g_optFastLoad=false,g_optFastLoadPathReuse=false,g_optFastLoadVpkSkip=false,
            g_optFastLoadAsyncMesh=false,g_optFastLoadAsyncCollide=false,g_optFastLoadAsyncAnims=false,g_fastLoadAsyncSplitSeen=false,
            g_optFastLoadWait=false,g_optFastLoadLocalReady=false,g_optFastLoadEarlyView=false,g_optFastLoadLocalTransition=false,g_optFastLoadDiag=false,g_optFastLoadChatTime=false,
            g_optFastLoadLessModelData=false,g_optFastLoadLessTouch=false,g_optFastLoadLessPreload=false,g_optFastLoadMapPacks=false,g_optFastLoadFileFilter=false,g_optFastLoadLessTextureWarmup=false;
static constexpr int kMenuItemsPerPage=9;
static constexpr int MenuPageCountForItems(int n){return n>0?(n+8)/9:1;}
static std::atomic_bool g_fastLoadReport{false},g_fastLoadReset{false};
static constexpr int kMenuPageFastLoad=124,kMenuPageFastLoadExperiments=130;
static bool g_menuVisible=true;
static int g_menuPage=124,g_menuSub=0;
static unsigned saves=0,sounds=0;
static void SaveSettings(){++saves;}
static void EngClientCmd(const char*){++sounds;}
static void MenuSubSync(){}
static bool MenuDiscoveryPage(int){return false;}
static void MenuDiscoveryKey(int){}
static void require(bool ok,const char* why) {
    if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}
}
''' + table + key + "\nstatic bool Parse(const char* line){int v=0;" + settings + "\nreturn false;}" + "\nstatic void Write(FILE* f){" + save + "}" + r'''
static unsigned mode(){const bool async=g_optFastLoadAsyncMesh||g_optFastLoadAsyncCollide||g_optFastLoadAsyncAnims;
    return skload::ConfiguredMode(g_optFastLoad,g_optFastLoadPathReuse,
    g_optFastLoadVpkSkip,async,g_optFastLoadWait,g_optFastLoadLocalReady,g_optFastLoadEarlyView,g_optFastLoadDiag,(g_optFastLoadLessModelData?1u:0u)|(g_optFastLoadLessTouch?2u:0u)|(g_optFastLoadLessPreload?4u:0u)|(g_optFastLoadLessTextureWarmup?8u:0u),(g_optFastLoadMapPacks?1u:0u)|(g_optFastLoadFileFilter?2u:0u));}
int main(){
    require(!mode(),"fixture starts with every mechanism off");
    for(const char* old:{"fast_load_enabled=1","fast_load_native_async=1","fast_load_diagnostics=1"})
        require(!Parse(old) && !mode(),"old option keys are neither read nor aliased");
    bool* page1[]={&g_optFastLoad,&g_optFastLoadPathReuse,&g_optFastLoadVpkSkip,
        &g_optFastLoadAsyncMesh,&g_optFastLoadAsyncCollide,&g_optFastLoadWait,&g_optFastLoadEarlyView,&g_optFastLoadLocalReady};
    const unsigned expected[]={0,9,25,57,57,61,189,253};
    for(int i=0;i<8;++i){MenuProcessKeyImpl(i+1);require(*page1[i] && saves==unsigned(i+1) && mode()==expected[i],"main row toggles its actual runtime setting and saves");}
    require(_countof(kFastLoadMenuItems)==13 && _countof(kFastLoadExperimentItems)==8
        && !FastLoadMenuItemAt(0,0) && !FastLoadMenuItemAt(0,10) && FastLoadMenuItemAt(1,4)
        && !FastLoadMenuItemAt(1,5) && !FastLoadMenuItemAt(2,1),"main table owns 9+4 bounds");
    require(FastLoadMenuItemAt(0,9)->action==FastLoadMenuAction::OpenExperiments
        && FastLoadMenuItemAt(1,1)->label==UiId::FastLoadChatTime,"experiment entry immediately precedes timer");
    MenuProcessKeyImpl(9);require(g_menuPage==130 && g_menuSub==0 && saves==8,"white entry opens experiment page without saving");
    bool* choices[]={&g_optFastLoadAsyncAnims,&g_optFastLoadLessModelData,&g_optFastLoadLessTouch,&g_optFastLoadLessPreload,&g_optFastLoadLocalTransition,&g_optFastLoadMapPacks,&g_optFastLoadFileFilter,&g_optFastLoadLessTextureWarmup};
    for(int i=0;i<8;++i){MenuProcessKeyImpl(i+1);require(*choices[i] && saves==unsigned(9+i),"experimental row toggles and saves independently");}
    MenuProcessKeyImpl(9);require(saves==16,"empty experimental digit does nothing");
    MenuProcessKeyImpl(0);require(g_menuPage==124 && g_menuSub==0,"experiment back returns to its parent entry page");
    g_menuSub=1;MenuProcessKeyImpl(1);MenuProcessKeyImpl(2);MenuProcessKeyImpl(3);MenuProcessKeyImpl(4);
    require(g_optFastLoadChatTime && g_optFastLoadDiag && g_fastLoadReport && g_fastLoadReset && saves==18,"parent timer/diagnostic/actions retain behavior");
    g_menuSub=0;MenuProcessKeyImpl(1);require(mode()==0 && saves==19,"master off suppresses all selected experiments");
    FILE* f=nullptr;require(tmpfile_s(&f)==0 && f,"serialization fixture file");Write(f);rewind(f);
    for(auto* v:page1)*v=false;for(auto* v:choices)*v=false;
    g_optFastLoadWait=g_optFastLoadEarlyView=g_optFastLoadChatTime=g_optFastLoadLocalReady=g_optFastLoadLocalTransition=g_optFastLoadDiag=false;
    char row[256];unsigned count=0;while(std::fgets(row,sizeof(row),f)){require(Parse(row),"every emitted key reads back");++count;}std::fclose(f);
    require(count==18 && !g_optFastLoad && mode()==0,"master and 18 settings round trip");
    for(unsigned i=1;i<8;++i)require(*page1[i],"main choices retained with master off");
    for(auto* v:choices)require(*v,"experimental choices round trip independently");
    Parse("fast_load_async=0");require(g_optFastLoadAsyncMesh && g_optFastLoadAsyncCollide && g_optFastLoadAsyncAnims,"split keys retain their published meaning");
    g_fastLoadAsyncSplitSeen=false;Parse("fast_load_async=0");require(!g_optFastLoadAsyncMesh && !g_optFastLoadAsyncCollide && !g_optFastLoadAsyncAnims,"published combined key remains supported");
    MenuProcessKeyImpl(0);require(g_menuPage==0 && g_menuSub==2,"parent return remains unchanged");
    g_menuPage=124;g_menuVisible=false;MenuProcessKeyImpl(1);require(!g_optFastLoad && saves==19,"hidden menu ignores numeric input");
    std::puts("PASS loading menu: main/experimental navigation, controls, master, settings round trip");
}
'''

if __name__ == "__main__":
    native.main()
