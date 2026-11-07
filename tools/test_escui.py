"""Exercise the production ESC context gate on MSVC x86; no game process needed."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, marker):
    assert marker in source, "ESC context gate has not been implemented"
    start = source.index(marker)
    opening = source.index("{", start)
    while ";" in source[start:opening]:
        start = source.index(marker, start + len(marker))
        opening = source.index("{", start)
    depth, at = 1, opening + 1
    while depth:
        depth += (source[at] == "{") - (source[at] == "}")
        at += 1
    return source[start:at]


def main():
    proxy = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    gate = function(proxy, "static bool EscUiContext(")
    layout = ROOT / "src/skeeto/core/skeeto_escui_layout.h"
    assert layout.exists(), "ESC editable layout model has not been implemented"
    source = '#define NOMINMAX\n#include <Windows.h>\n#include <cassert>\n#include <cstring>\n#include "' + layout.as_posix() + '"\n#include "' + (layout.parent / 'skeeto_escui_io.h').as_posix() + '"\n' + gate + r'''
int main() {
    // Every ownership-domain combination: off/exit/policy/disconnect/loading releases ownership.
    for (int bits=0;bits<64;++bits) {
        bool enabled=bits&1, running=bits&2, allowed=bits&4, connected=bits&8;
        bool inGame=bits&16, loading=bits&32;
        bool expected=(bits&31)==31 && !(bits&32);
        assert(EscUiContext(enabled,running,allowed,connected,inGame,loading)==expected);
    }
    static skesc::Layout model{};model=skesc::Defaults(); assert(skesc::Valid(model));
    int first=0;while(model.items[first].action!=8)++first;
    assert(skesc::Hit(model,150,210,false)==first);
    model.items[first].visible=false; assert(skesc::Valid(model));
    assert(skesc::Hit(model,150,210,false)==-1);
    model=skesc::Defaults(); assert(skesc::Erase(model,0));
    assert(skesc::Add(model,1)); assert(skesc::Erase(model,0));
    assert(skesc::Valid(model));
    while(model.count<skesc::kMaxItems)assert(skesc::Add(model,2));
    assert(!skesc::Add(model,2));
    model=skesc::Defaults();
    std::strcpy(model.items[0].zh,u8"音量调节"); std::strcpy(model.items[0].en,"Audio & music");
    model.items[0].color=4; model.items[3].visible=false;
    static char text[skesc::kFileBytes]{}; auto size=skesc::Format(model,text,sizeof(text)); assert(size>0);
    static skesc::Layout loaded{};assert(skesc::Parse(text,size,loaded));
    assert(loaded.count==model.count && loaded.items[0].color==4 && !loaded.items[3].visible);
    assert(std::strcmp(loaded.items[0].zh,u8"音量调节")==0);
    assert(std::strcmp(loaded.items[0].en,"Audio & music")==0);
    static skesc::Layout original{};original=loaded;
    assert(!skesc::Parse(text,size-4,loaded)); assert(std::memcmp(&loaded,&original,sizeof(loaded))==0);
    assert(!skesc::Parse("SKEETO_ESC_UI 9\n",16,loaded));
    model.items[0].x=-1;assert(!skesc::Valid(model));
    skesc::ClampItem(model,0);assert(skesc::Valid(model));
    model.items[0].zh[0]=-64;model.items[0].zh[1]=-128;model.items[0].zh[2]=0;
    assert(!skesc::Valid(model));
    model=skesc::Defaults();model.items[3].visible=false;
    assert(skesc::NextAction(model,2,1)==4);
    int lastRoot=0;for(int i=0;i<model.count;++i)if(model.items[i].parent<0&&model.items[i].visible&&model.items[i].action)lastRoot=i;
    assert(skesc::NextAction(model,0,-1)==lastRoot);
    assert(skesc::Move(model,model.count-1,1)==model.count-1); assert(skesc::Move(model,7,-1)==6);
    int integers[2]{}; assert(!skesc::Numbers("count 4294967297","count",integers,1));
    assert(!skesc::Numbers("count 1 garbage","count",integers,1));
    static skesc::Layout geo{};geo=skesc::Defaults();geo.count=3;
    for(int i=0;i<3;++i){auto& b=geo.items[i];skesc::SetAction(b,2);b.x=32+i*200;b.y=200+i*12;b.w=100;b.h=56;b.parent=-1;}
    bool chosen[skesc::kMaxItems]{};chosen[0]=chosen[1]=chosen[2]=true;
    assert(skesc::Valid(geo));assert(skescgeo::Arrange(geo,chosen,1,0));
    assert(geo.items[0].x==232&&geo.items[2].x==232);
    geo.items[0].y=100;geo.items[1].y=190;geo.items[2].y=400;
    assert(skescgeo::Arrange(geo,chosen,1,10));assert(geo.items[1].y==250&&geo.items[2].y==400);
    geo.items[1].y=120;geo.items[2].y=140;static skesc::Layout unchanged{};unchanged=geo;
    assert(!skescgeo::Arrange(geo,chosen,1,10));assert(!memcmp(&geo,&unchanged,sizeof(geo)));
    geo=unchanged;skescgeo::Move(geo,chosen,-1000,0);assert(geo.items[0].x==0&&geo.items[1].x==0);
    geo.items[0].x=35;geo.items[1].x=200;geo.items[1].y=200;geo.items[2].x=400;
    bool single[skesc::kMaxItems]{};single[0]=true;int visible[]={0,1,2};
    geo.items[0].x=203;auto snap=skescgeo::Align(geo,0,single,visible,3,false);
    assert(snap.x==200&&snap.dx==-3);
    chosen[1]=chosen[2]=false;unchanged=geo;assert(!skescgeo::Arrange(geo,chosen,0,0));assert(!memcmp(&geo,&unchanged,sizeof(geo)));
    wchar_t folder[1024]{},path[1024]{};assert(GetCurrentDirectoryW(1024,folder));
    swprintf_s(path,L"%s\\left4dead2\\skeeto\\escmenu\\current.hud",folder);
    loaded=original;assert(skescio::Load(path,loaded)==0);
    assert(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES);
    model=skesc::Defaults();assert(skescio::Save(path,model));
    assert(skescio::Load(path,loaded)==1);
    HANDLE held=CreateFileW(path,GENERIC_READ,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);assert(held!=INVALID_HANDLE_VALUE);
    model.backdrop=80;assert(!skescio::Save(path,model));CloseHandle(held);
    assert(skescio::Load(path,loaded)==1 && loaded.backdrop==0);
    HANDLE corrupt=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    DWORD written=0;assert(WriteFile(corrupt,"bad",3,&written,nullptr));CloseHandle(corrupt);
    original=loaded;assert(skescio::Load(path,loaded)==-1 && std::memcmp(&loaded,&original,sizeof(loaded))==0);
}
'''
    source = source.replace("int main() {", '#include "' + (layout.parent / "skeeto_escui_geometry.h").as_posix() + '"\nint main() {')
    editor = (layout.parent / "skeeto_escui_editor.h").read_text(encoding="utf-8")
    harness = '#include <atomic>\n#include "' + (layout.parent / 'skeeto_ui_text.h').as_posix() + '"\n'
    harness += function(proxy, "enum class EscUiStatus") + ';\n'
    harness += function(editor, "enum class EscUiEditAction") + ';\n'
    harness += r'''
static skesc::Layout g_escUiLayout{},g_escUiDraft{},g_escUiIoLayout{};
static bool g_escUiIoBusy=false,g_escUiWritable=true,g_escUiEditing=false,g_escUiEditRequested=false;
static bool g_escUiTextDirty=false,g_escUiSeed=false,g_escUiDraftAvailable=false;
static bool g_escUiConfirmCurrent=false,g_escUiConfirmProfile=false;
static bool g_escUiOwned=false,g_escUiDraw=false;
static unsigned int g_escUiHome=0;
static void EscUiNativeRelease(){}
static int g_escUiTextIndex=-1,g_escUiTextLanguage=0,g_escUiDrag=-1,g_escUiPressed=-1;
static int g_escUiSliderDrag=-1,g_escUiPick=0,g_escUiPickPage=0;
static int g_escUiFlyout=-1,g_escUiFlyoutPage=0,g_escUiFocus=0;
static wchar_t g_escUiCvarFields[4][64]{};
static int g_escUiSelected=0,g_escUiIoKind=0,g_escUiIoProfile=1,g_escUiProfile=1;
static std::atomic_int g_escUiIoJob{0},g_escUiIoResult{0};
static EscUiStatus g_escUiStatus=EscUiStatus::Ready;
static unsigned g_escUiTextRequest=0;
static void MainUiServicesCancelText(unsigned){}
static unsigned MainUiServicesText(const wchar_t*,const char*){return 1;}
static int openedFolders=0;
static bool MainUiServicesOpen(int,const char*){++openedFolders;return true;}
static void MenuMarkDirty(){}
static bool g_escUiMulti[skesc::kMaxItems]{};
static bool g_escUiMapInfoWanted=false;
static bool g_optPauseSampling=false,g_optHideAllCustomHud=false,g_optHideAllWorldHud=false,g_optHideAllXhair=false;
static int extrasClosed=0;
static void EscUiExtrasClose(){++extrasClosed;}

'''
    harness += function(editor, "static int EscUiButtonState(") + "\n"
    harness += function(editor, "static void EscUiSelectOne(") + "\n"
    harness += function(proxy, "static void EscUiRefreshMapWanted(") + "\n"
    for marker in ["static bool EscUiPostIo(", "static void EscUiConfigPump(",
                   "static void EscUiCancelText(", "static void EscUiAbortEditor(",
                   "static void EscUiLayoutChanged(", "static void EscUiBeginEditor(",
                   "static void EscUiSuspendInput(", "static void EscUiRestore("]:
        harness += function(proxy, marker) + '\n'
    harness += function(editor, "static void EscUiRevealSelected(")
    harness += function(editor, "static void EscUiOpenFlyout(")
    for marker in ["static COLORREF EscUiColor(", "static COLORREF EscUiTextColor(", "static COLORREF EscUiItemColor("]:
        harness += function(editor, marker)
    harness += function(editor, "static void EscUiTextStart(")
    harness += function(editor, "static void EscUiPickOpen(")
    harness += function(editor, "static void EscUiEditorAction(")
    harness += r'''
static void check_editor(){
    skesc::Item toggle{};toggle.action=skesc::kCommand;strcpy_s(toggle.parameter,"skeeto_hud_toggle");
    assert(EscUiButtonState(toggle)==0);g_optPauseSampling=true;assert(EscUiButtonState(toggle)==4);
    g_optHideAllCustomHud=true;assert(EscUiButtonState(toggle)==5);g_optPauseSampling=false;assert(EscUiButtonState(toggle)==1);
    g_optHideAllCustomHud=false;
    g_escUiLayout=skesc::Defaults();EscUiBeginEditor();g_escUiDraft.items[0].x=500;
    EscUiEditorAction(EscUiEditAction::Cancel);assert(g_escUiLayout.items[0].x==32&&!g_escUiDraftAvailable);
    EscUiBeginEditor();g_escUiDraft.items[0].x=600;EscUiAbortEditor();EscUiBeginEditor();
    assert(extrasClosed>0);
    assert(g_escUiDraft.items[0].x==600); // Leaving GameUI keeps this session's draft.
    g_escUiDrag=2;g_escUiPressed=2;g_escUiOwned=true;g_escUiDraw=true;
    static skesc::Layout suspended{};suspended=g_escUiDraft;EscUiRestore();
    assert(g_escUiDrag==-1&&g_escUiPressed==-1&&g_escUiSeed&&!g_escUiOwned&&!g_escUiDraw);
    assert(g_escUiEditing&&std::memcmp(&suspended,&g_escUiDraft,sizeof(suspended))==0);
    g_escUiSelected=0;EscUiEditorAction(EscUiEditAction::Visible);
    assert(!g_escUiDraft.items[0].visible); // All default buttons can be hidden.
    EscUiEditorAction(EscUiEditAction::Action);assert(g_escUiPick==2&&g_escUiDraft.items[0].action==skesc::kMapInfo);
    g_escUiWritable=false;EscUiEditorAction(EscUiEditAction::Apply);
    assert(g_escUiEditing&&g_escUiConfirmCurrent&&g_escUiLayout.items[0].x==32);
    EscUiEditorAction(EscUiEditAction::Apply);
    assert(!g_escUiEditing&&g_escUiWritable&&g_escUiLayout.items[0].x==600);
    assert(!g_escUiMapInfoWanted);
    assert(g_escUiIoBusy&&g_escUiIoKind==1&&g_escUiIoLayout.items[0].x==600);
    static skesc::Layout snapshot{};snapshot=g_escUiIoLayout;assert(!EscUiPostIo(2));
    assert(std::memcmp(&snapshot,&g_escUiIoLayout,sizeof(snapshot))==0); // Busy job is immutable.
    g_escUiIoResult=1;EscUiConfigPump();assert(!g_escUiIoBusy);
    EscUiBeginEditor();g_escUiDraft.items[0].x=700;EscUiEditorAction(EscUiEditAction::SaveProfile);
    assert(!g_escUiIoBusy&&g_escUiConfirmProfile);
    EscUiEditorAction(EscUiEditAction::SaveProfile);assert(g_escUiIoKind==2&&g_escUiIoLayout.items[0].x==700);
    g_escUiIoResult=1;EscUiConfigPump();EscUiEditorAction(EscUiEditAction::LoadProfile);
    static skesc::Layout before{};before=g_escUiDraft;EscUiEditorAction(EscUiEditAction::Delete);
    assert(std::memcmp(&before,&g_escUiDraft,sizeof(before))==0); // Pending load blocks edits.
    g_escUiMulti[7]=true;
    g_escUiIoLayout=skesc::Defaults();g_escUiIoLayout.items[0].x=800;g_escUiIoResult=1;EscUiConfigPump();
    assert(g_escUiMulti[0]&&!g_escUiMulti[7]);
    assert(g_escUiDraft.items[0].x==800&&g_escUiLayout.items[0].x==600); // Import is a draft, not live.
    EscUiEditorAction(EscUiEditAction::LoadProfile);EscUiAbortEditor(true);g_escUiIoLayout.items[0].x=900;
    g_escUiIoResult=1;EscUiConfigPump();assert(!g_escUiEditing&&g_escUiLayout.items[0].x==600);
    EscUiBeginEditor();EscUiEditorAction(EscUiEditAction::OpenProfiles);
    assert(g_escUiIoBusy&&g_escUiIoKind==4&&openedFolders==0);
    g_escUiIoResult=1;EscUiConfigPump();assert(openedFolders==1&&!g_escUiIoBusy);
}
'''
    source = source.replace("int main() {", "void check_editor();\nint main() {\ncheck_editor();")
    harness = harness.replace("static void check_editor(){", "void check_editor(){")
    source += '\n' + harness
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
                                  "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property",
                                  "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-escui-") as temp:
        folder = Path(temp)
        (folder / "fixture.cpp").write_text(source, encoding="utf-8")
        (folder / "run.bat").write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvars32.bat" >nul\n'
            'if errorlevel 1 exit /b 1\n'
            'cl /nologo /O2 /EHsc /std:c++17 /utf-8 /W4 /WX /D_CRT_SECURE_NO_WARNINGS fixture.cpp /Fe:fixture.exe /link shell32.lib\n'
            'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)
    print("ESC context / layout / UTF-8 / parser / selection checks passed")


if __name__ == "__main__":
    main()
