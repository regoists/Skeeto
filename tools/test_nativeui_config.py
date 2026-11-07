"""Exercise native-UI settings without loading game interfaces."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from test_nativeui_chat_binding import definition

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "src/skeeto/core/skeeto_nativeui_config.h"


def main():
    assert HEADER.is_file(), "native UI configuration is missing"
    compiler = shutil.which("g++") or shutil.which("clang++")
    assert compiler, "C++ compiler required"
    menu = (HEADER.parent / "skeeto_nativeui_menu.h").read_text(encoding="utf-8")
    rows = re.findall(r"sknativeui::(\w+),(\d+)\}", menu)
    assert rows, "native UI menu registry missing"
    coverage = ""
    for target in (0, 1):
        visible = [(field, int(mask)) for field, mask in rows if int(mask) & (1 << target)]
        assert visible[0][0] == "Enabled", "master toggle must be first"
        if target == 0:
            assert visible[1][0] == "SayTime", "chat duration must be visible on the first page"
        assert visible[-1][0] == "Count", "reset action must be last"
        assert len({field for field, _ in visible}) == len(visible), "duplicate menu field"
        assert [field for field, _ in visible].count("AlphaStep") == 1, "one opacity step per target"
        assert [field for field, _ in visible].index("AlphaStep") < [field for field, _ in visible].index("BgAlpha")
        coverage += "{ int seen[Count]{};\n"
        for field, _ in visible:
            if field != "Count":
                coverage += f"assert(Applies({field},{target}));\n"
                coverage += f"++seen[{field}];\n"
        if target == 0:
            assert any(field == "X" for field, _ in visible), "chat position editor entry missing"
            assert not any(field == "Y" for field, _ in visible), "Y belongs to the arrow-key editor"
            coverage += "++seen[Y]; // Y is edited with X on the position subpage.\n"
        coverage += f"for(int f=0;f<Count;++f)assert(seen[f] == (Applies((Field)f,{target})?1:0));\n}}\n"
        coverage += f"assert({len(visible)} <= 27);\n"
    step = definition(menu, "NativeUiMenuStep")
    source = r'''
#include "skeeto_nativeui_config.h"
#include <cassert>
#include <atomic>
struct SRWLOCK {};
#define SRWLOCK_INIT {}
static void AcquireSRWLockShared(SRWLOCK*){}
static void ReleaseSRWLockShared(SRWLOCK*){}
static void AcquireSRWLockExclusive(SRWLOCK*){}
static void ReleaseSRWLockExclusive(SRWLOCK*){}
#include "skeeto_nativeui_settings.h"
struct NativeUiMenuItem {sknativeui::Field field;};
static int g_nativeUiMenuSelection[2]{},g_menuPage=0,g_menuSub=2;
static constexpr int kMenuPageChatUiPosition=133;
static sknativeui::Field testField=sknativeui::X;
static const NativeUiMenuItem* NativeUiMenuItemAt(int,int){static NativeUiMenuItem item;item.field=testField;return &item;}
static void MenuMarkDirty(){}
static void NativeUiChanged(int target,const sknativeui::Config& c){NativeUiStore(target,c);}
#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif
MENU_STEP
int main() {
    using namespace sknativeui;
    const Field alphaFields[]={BgAlpha,InputBgAlpha};
    for(int target=0;target<2;++target){
        for(Field alpha:alphaFields){
            NativeUiStore(target,Defaults(target));testField=alpha;
            const int before=NativeUiConfig(target).v[alpha];
            NativeUiMenuStep(target,0,1,false);
            assert(NativeUiConfig(target).v[alpha]==before+5);
            NativeUiMenuStep(target,0,-1,false);
            assert(NativeUiConfig(target).v[alpha]==before);
        }
    }
    NativeUiLoadDefaults();testField=X;
    assert(NativeUiConfig(0).v[AlphaStep]==5 && NativeUiConfig(1).v[AlphaStep]==5);
    Config custom=NativeUiConfig(0);custom.v[AlphaStep]=10;NativeUiStore(0,custom);
    assert(NativeUiConfig(1).v[AlphaStep]==5);
    for(int target=0;target<2;++target){
        const int step=target?5:10;
        for(Field alpha:alphaFields){
            testField=alpha;const int before=NativeUiConfig(target).v[alpha];
            NativeUiMenuStep(target,0,1,true);assert(NativeUiConfig(target).v[alpha]==before);
            NativeUiMenuStep(target,0,1,false);assert(NativeUiConfig(target).v[alpha]==before+step);
            NativeUiMenuStep(target,0,-1,false);assert(NativeUiConfig(target).v[alpha]==before);
            auto bounded=NativeUiConfig(target);bounded.v[alpha]=99;NativeUiStore(target,bounded);
            NativeUiMenuStep(target,0,1,false);assert(NativeUiConfig(target).v[alpha]==100);
            bounded.v[alpha]=1;NativeUiStore(target,bounded);
            NativeUiMenuStep(target,0,-1,false);assert(NativeUiConfig(target).v[alpha]==0);
        }
        testField=FontSize;const int font=NativeUiConfig(target).v[FontSize];
        NativeUiMenuStep(target,0,1,false);assert(NativeUiConfig(target).v[FontSize]==font+1);
        testField=AlphaStep;NativeUiMenuStep(target,0,1,false);assert(NativeUiConfig(target).v[AlphaStep]==step+1);
        testField=Count;NativeUiMenuStep(target,0,1,true);
        assert(NativeUiConfig(target).v[AlphaStep]==5 && NativeUiConfig(target).v[BgAlpha]==70);
    }
    NativeUiLoadDefaults();testField=X;
    const auto beforeEdit=NativeUiConfig(0);
    NativeUiMenuStep(0,17,1,true);
    assert(g_menuPage==kMenuPageChatUiPosition && g_menuSub==0);
    NativeUiMenuStep(0,17,1,false);
    assert(NativeUiConfig(0).v[X]==beforeEdit.v[X]);
    auto revision0=g_nativeUiRevision[0].load(),revision1=g_nativeUiRevision[1].load();
    Config onlyConsole=NativeUiConfig(1);onlyConsole.v[Enabled]=1;NativeUiStore(1,onlyConsole);
    assert(g_nativeUiRevision[0].load()==revision0 && g_nativeUiRevision[1].load()==revision1+1);
    assert(g_nativeUiWanted.load()==2 && !NativeUiConfig(0).v[Enabled]);
    NativeUiLoadDefaults();assert(g_nativeUiWanted.load()==0);
    Config chat = Defaults(0), console = Defaults(1);
    assert(ReadLine(chat,0,"chat_ui_alpha_step=8") && chat.v[AlphaStep]==8);
    assert(ReadLine(console,1,"console_ui_alpha_step=12") && console.v[AlphaStep]==12 && chat.v[AlphaStep]==8);
    assert(!ReadLine(console,1,"chat_ui_alpha_step=20"));
    assert(ReadLine(chat,0,"chat_ui_alpha_step=0") && chat.v[AlphaStep]==1);
    assert(ReadLine(chat,0,"chat_ui_alpha_step=999") && chat.v[AlphaStep]==100);
    assert(!ReadLine(chat,0,"chat_ui_alpha_step=5oops") && chat.v[AlphaStep]==100);
    assert(ReadLine(chat,0,"chat_ui_alpha_step=8"));
    assert(chat.v[Enabled] == 0 && console.v[Enabled] == 0);
    assert(chat.v[FontOverride] == 0 && console.v[FontOverride] == 1);
    assert(ReadLine(chat, 0, "chat_ui_say_time=0"));
    assert(chat.v[SayTime] == 0 && kFields[SayTime].lo == 0);
    assert(ReadLine(chat, 0, "chat_ui_say_time=-1"));
    assert(chat.v[SayTime] == 0);
    assert(ReadLine(chat, 0, "chat_ui_font_size=32\n"));
    assert(chat.v[FontSize] == 32);
    assert(!ReadLine(console, 1, "chat_ui_font_size=12"));
    assert(!ReadLine(console, 1, "console_ui_survivor_rgb=1"));
    assert(!ReadLine(chat, 0, "chat_ui_developer_rgb=1"));
    const Field geometryFields[]={Geometry,X,Y,Width,Height};
    for (Field f : geometryFields) assert(!Applies(f,1));
    assert(!ReadLine(console,1,"console_ui_x=20"));
    assert(!ReadLine(console,1,"console_ui_geometry=1"));
    Config moved=Defaults(0);
    assert(Nudge(moved,1,-1,false) && moved.v[X]==3 && moved.v[Y]==59 && moved.v[Geometry]);
    assert(Nudge(moved,1,1,true) && moved.v[X]==7 && moved.v[Y]==63);
    moved.v[X]=99;moved.v[Y]=99;
    assert(Nudge(moved,-1,-1,false));
    assert(moved.v[X]==100-moved.v[Width]-1 && moved.v[Y]==100-moved.v[Height]-1);
    moved.v[X]=0;moved.v[Y]=0;assert(!Nudge(moved,-1,-1,false));
    assert(!ReadLine(chat, 0, "chat_ui_font_size=12oops"));
    assert(chat.v[FontSize] == 32);
    assert(ReadLine(chat, 0, "chat_ui_bg_alpha=0"));
    assert(chat.v[BgAlpha] == 0);
    assert(ReadLine(chat, 0, "chat_ui_bg_alpha=999999"));
    assert(chat.v[BgAlpha] == 100);
    assert(!ReadLine(chat, 0, "chat_ui_enabled=99999999999999999999999"));
    FILE* f = tmpfile(); assert(f);
    Save(f, chat, 0); rewind(f);
    Config restored = Defaults(0); char line[128];
    while (fgets(line, sizeof(line), f)) assert(ReadLine(restored, 0, line));
    fclose(f);
    for (int i=0;i<Count;++i) assert(chat.v[i] == restored.v[i]);
    NativeUiStore(0,chat);NativeUiStore(1,console);
    f=tmpfile();assert(f);NativeUiSave(f);rewind(f);NativeUiLoadDefaults();
    while(fgets(line,sizeof(line),f))assert(NativeUiLoadLine(line));
    fclose(f);
    assert(NativeUiConfig(0).v[AlphaStep]==8 && NativeUiConfig(1).v[AlphaStep]==12);
    chat.v[X]=99; chat.v[Y]=100; chat.v[Width]=100; chat.v[Height]=100;
    Rect r = Bounds(chat, 1920, 1080);
    assert(r.x>=0 && r.y>=0 && r.x+r.w<=1920 && r.y+r.h<=1080);
    chat.v[X]=0; chat.v[Y]=0; r=Bounds(chat,1280,720);
    assert(r.x==0 && r.y==0 && r.w==1280 && r.h==720);
    assert(Pack(0x112233, 100) == 0xff332211u);
    assert(Pack(0x112233, 0) == 0x00332211u);
}
'''
    source=source.replace("MENU_STEP",step)
    with tempfile.TemporaryDirectory(prefix="skeeto-nativeui-") as directory:
        cpp = Path(directory) / "config.cpp"
        exe = Path(directory) / "config.exe"
        source = source.replace("    using namespace sknativeui;", "    using namespace sknativeui;\n" + coverage)
        cpp.write_text(source, encoding="utf-8")
        subprocess.run([compiler, "-std=c++17", "-I", str(HEADER.parent), str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("native UI config: defaults, isolation, validation, round trip, bounds and alpha passed")


if __name__ == "__main__":
    main()
