"""Execute production viewmodel callback ABI, slot cleanup and IO transport."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
from test_viewmodel_io import implementation

ROOT = Path(__file__).resolve().parents[1]


def function(text, signature):
    start = text.index(signature)
    depth = 0
    for token in re.finditer(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', text[start:]):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return text[start:start + token.end()] + "\n"
    raise RuntimeError("Unclosed " + signature)


PRELUDE = r'''
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cassert>
constexpr unsigned SKT_PATH_CAP=1024;
enum class UiId { VmStatusInitial,VmStatusLoading,VmStatusReady,VmStatusLoadError,
    VmStatusSaveError,VmStatusSaved,VmStatusSaving,VmStatusHookError,VmStatusEdited };
'''

STUBS = r'''
static DWORD g_paintTid;
static std::atomic<bool> g_run{true},g_paintInGame{false};
static std::atomic<int> g_dbgLocalEnt{1};
static bool feature=true,g_menuVisible=false,g_menuParked=false;
static DWORD fakeNow=1000;
static DWORD VmTestTick(){return fakeNow;}
#define GetTickCount VmTestTick
static int g_menuPage=0,kMenuPageViewModel=201,g_offModelIndex=0x80,g_offViewModel=0x120;
static int origCalls=0,originWrites=0,angleWrites=0,reads=0,slotWrites=0,menuMarks=0;
static bool readOk=true,slotWriteOk=true;
static bool SkeetoFeaturesOn(){return feature;}
static void MenuMarkDirty(){++menuMarks;}
template<class T> static bool EntRead(void* e,int off,T* out) {
    ++reads;if(!readOk)return false;std::memcpy(out,(char*)e+off,sizeof(T));return true;
}
static bool EntReadBlock(void* e,int off,void* out,size_t n) {
    ++reads;if(!readOk)return false;std::memcpy(out,(char*)e+off,n);return true;
}
namespace skmem {static bool Copy(void* out,const void* src,size_t n) {
    if(!readOk)return false;std::memcpy(out,src,n);return true;
}}
static bool VtProtectWrite(void** slot,void* value,void*) {
    ++slotWrites;if(!slotWriteOk)return false;*slot=value;return true;
}
static void* EntGet(int){return nullptr;}
static void* EntFromHandle(int){return nullptr;}
static void ViewModelStepsChanged(){}
static void ViewModelMirrorPump(bool){}
static void ViewModelScaleFieldPump(bool){}
static void ViewModelBind(int,void*){assert(false);}
static bool ViewModelInstall(void*){assert(false);return false;}
static unsigned char entity[0xa00],other[0xa00];
static int playerStorage,rawHeaderStorage;
static void* headerStorage=&rawHeaderStorage;
static void __fastcall Upstream(void* self,void*,void* player,
        const skviewmodel::Vec& eyeOrigin,const skviewmodel::Vec& eyeAngles) {
    assert(self==entity&&player!=nullptr&&eyeOrigin.x==500&&eyeAngles.x==0);
    ++origCalls;const skviewmodel::Vec o{10,20,30},a{0,0,0};
    std::memcpy((char*)self+0xa0,&o,sizeof(o));std::memcpy((char*)self+0xc4,&a,sizeof(a));
}
static void __fastcall SetOrigin(void* self,void*,const skviewmodel::Vec& value) {
    assert(origCalls>0);++originWrites;std::memcpy((char*)self+0xa0,&value,sizeof(value));
}
static void __fastcall SetAngles(void* self,void*,const skviewmodel::Vec& value) {
    assert(origCalls>0);++angleWrites;std::memcpy((char*)self+0xc4,&value,sizeof(value));
}
'''

TEST = r'''
static skviewmodel::Vec Origin(){skviewmodel::Vec r;std::memcpy(&r,entity+0xa0,sizeof(r));return r;}
static void Run(void* player=&playerStorage){
    const skviewmodel::Vec origin{500,600,700},angles{0,0,0};
    Hooked_ViewModelCalc(entity,nullptr,player,origin,angles);
}
static void Reset(){
    g_paintTid=GetCurrentThreadId();g_vmEnabled=true;g_run=true;feature=true;g_vmReady=true;g_vmHookFailed=false;
    g_vmLocal=&playerStorage;g_vmStore={};g_vmStore.Ensure("models/weapons/v_test.mdl",123);
    g_vmStore.records[0].pose.position={1,2,3};g_vmStore.records[0].pose.Rebuild();
    g_vmBindings[0]={};g_vmBindings[0].entity=entity;g_vmBindings[0].modelIndex=7;
    g_vmBindings[0].header=&headerStorage;g_vmBindings[0].profile=0;g_vmBindings[1]={};
    headerStorage=&rawHeaderStorage;g_vmBindings[0].rawHeader=&rawHeaderStorage;
    short idx=7;void* hdr=&headerStorage;std::memcpy(entity+g_offModelIndex,&idx,sizeof(idx));std::memcpy(entity+0x934,&hdr,sizeof(hdr));
    origCalls=originWrites=angleWrites=reads=slotWrites=0;readOk=slotWriteOk=true;
    g_vmOriginal=(ViewModelCalcFn)&Upstream;g_vmSetOrigin=(ViewModelSetFn)&SetOrigin;g_vmSetAngles=(ViewModelSetFn)&SetAngles;
}
int main(){
    static_assert(sizeof(void*)==4,"Tests require x86 ABI");
    Reset();Run();auto o=Origin();assert(origCalls==1&&originWrites==1&&angleWrites==0&&o.x==12&&o.y==19&&o.z==33);
    for(int i=0;i<100;++i)Run();o=Origin();assert(o.x==12&&o.y==19&&o.z==33&&origCalls==101);
    puts("PASS upstream first, additive result and no accumulation");
    Reset();g_vmEnabled=false;Run();assert(origCalls==1&&originWrites==0&&reads==0);
    Reset();g_paintTid=GetCurrentThreadId()+1;Run();assert(origCalls==1&&originWrites==0&&reads==0);
    Reset();Run(other);assert(origCalls==1&&originWrites==0&&reads==0);
    Reset();g_run=false;Run();assert(originWrites==0);Reset();feature=false;Run();assert(originWrites==0);
    puts("PASS disabled, wrong thread/local and global gates");
    Reset();short idx=8;std::memcpy(entity+g_offModelIndex,&idx,sizeof(idx));Run();assert(originWrites==0);
    Reset();void* hdr=other;std::memcpy(entity+0x934,&hdr,sizeof(hdr));Run();assert(originWrites==0);
    Reset();headerStorage=other;Run();assert(originWrites==0);
    Reset();g_vmBindings[0].entity=other;Run();assert(originWrites==0);
    Reset();readOk=false;Run();assert(originWrites==0);
    puts("PASS stale model/header/entity and read failure");
    Reset();g_vmStore.records[0].pose={};g_vmStore.records[0].pose.Rebuild();Run();assert(originWrites==0&&angleWrites==0&&reads==0);
    Reset();g_vmStore.records[0].pose.rotation={0,90,0};g_vmStore.records[0].pose.Rebuild();Run();
    assert(originWrites==1&&angleWrites==1);skviewmodel::Vec angle;std::memcpy(&angle,entity+0xc4,sizeof(angle));assert(std::fabs(angle.y-90)<.01f);
    puts("PASS neutral does not write and rotation uses both native setters");
    Reset();void* slot=(void*)&Hooked_ViewModelCalc;g_vmHookSlot=&slot;g_vmHooked=true;ViewModelRemove();
    assert(slot==(void*)g_vmOriginal&&!g_vmHooked&&slotWrites==1);
    g_vmHooked=true;slot=(void*)&SetOrigin;slotWrites=0;ViewModelRemove();assert(slot==(void*)&SetOrigin&&!g_vmHooked&&slotWrites==0);
    g_vmHooked=true;slot=(void*)&Hooked_ViewModelCalc;slotWriteOk=false;g_vmHookFailed=false;ViewModelRemove();assert(g_vmHooked&&g_vmHookFailed&&slot==(void*)&Hooked_ViewModelCalc);
    puts("PASS remove restores only owned slot and reports restore failure");
    Reset();g_vmHooked=false;g_vmIoBusy=false;g_vmDirty=false;g_vmReady=false;g_vmEnabled=true;g_vmJob=0;g_vmResult=0;
    ViewModelGamePump();assert(g_vmIoBusy&&g_vmJob==1&&g_vmStatus==UiId::VmStatusLoading);
    g_vmIoStore={};g_vmIoStore.Ensure("models/weapons/v_loaded.mdl",55);g_vmResult=1;g_vmJob=0;
    ViewModelGamePump();assert(g_vmReady&&g_vmWritable&&!g_vmIoBusy&&g_vmStore.records[0].checksum==55);
    g_vmResult=3;g_vmIoBusy=true;ViewModelGamePump();assert(g_vmReady&&!g_vmWritable&&g_vmStore.records[0].checksum==55);
    g_vmWritable=true;g_vmDirty=true;ViewModelRequestSave();assert(g_vmIoBusy&&!g_vmDirty&&g_vmJob==2&&g_vmIoStore.records[0].checksum==55);
    g_vmResult=5;ViewModelGamePump();assert(!g_vmIoBusy&&g_vmSaveFailed&&g_vmDirty&&g_vmStatus==UiId::VmStatusSaveError);
    ViewModelGamePump();assert(!g_vmIoBusy); // failed save is not silently retried every Paint
    ViewModelRequestSave();assert(g_vmIoBusy&&!g_vmDirty&&!g_vmSaveFailed);
    g_vmResult=4;ViewModelGamePump();assert(!g_vmSaveFailed&&g_vmStatus==UiId::VmStatusSaved);
    puts("PASS load/save transport ownership, error preservation and no retry loop");
    g_vmMirrorPerModel=true;g_vmMirrorEnabled=false;g_vmEnabled=false;
    ViewModelMirrorSetGlobal(2);assert(!g_vmMirrorPerModel&&g_vmMirrorEnabled&&g_vmEnabled);
    ViewModelMirrorSetGlobal(2);assert(!g_vmMirrorPerModel&&!g_vmMirrorEnabled);
    ViewModelMirrorSetGlobal(1);assert(!g_vmMirrorPerModel&&g_vmMirrorEnabled);
    ViewModelMirrorSetGlobal(0);assert(!g_vmMirrorPerModel&&!g_vmMirrorEnabled);
    puts("PASS global command selects global mode; toggle uses effective mode state");
    // An edit while the worker owns the first snapshot remains dirty and is
    // queued as a second snapshot only at its own deadline (also across wrap).
    g_vmDirty=true;g_vmSaveAt=2000;g_vmStore.records[0].pose.position.x=1;
    ViewModelRequestSave();g_vmStore.records[0].pose.position.x=2;g_vmDirty=true;
    g_vmResult=4;ViewModelGamePump();
    assert(g_vmDirty&&!g_vmIoBusy&&g_vmStatus==UiId::VmStatusEdited&&g_vmIoStore.records[0].pose.position.x==1);
    fakeNow=2000;ViewModelGamePump();assert(g_vmIoBusy&&!g_vmDirty&&g_vmIoStore.records[0].pose.position.x==2);
    g_vmResult=4;ViewModelGamePump();
    fakeNow=0xffffff00u;g_vmSaveAt=0x100u;g_vmDirty=true;
    ViewModelGamePump();assert(!g_vmIoBusy&&g_vmDirty);
    fakeNow=0x100u;ViewModelGamePump();assert(g_vmIoBusy&&!g_vmDirty);
    puts("PASS pending edits survive prior save and deadline handles DWORD wrap");
    return 0;
}
'''


def main():
    text = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    source = PRELUDE + implementation("skviewmodel", "tools/local/analysis/viewmodel/pose_core.inc")
    begin = text.index("static bool g_vmEnabled =")
    end = text.index("static void ViewModelGamePump();", begin)
    source += text[begin:end] + STUBS
    for signature in ("static void ViewModelRequestSave() {", "static void __fastcall Hooked_ViewModelCalc(", "static void ViewModelRemove() {", "static bool ViewModelGlobalMirrorActive() {", "static void ViewModelMirrorSetGlobal(", "static void ViewModelGamePump() {"):
        source += function(text, signature)
    source += TEST
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    if not vs:
        raise RuntimeError("MSVC x86 toolset missing")
    with tempfile.TemporaryDirectory(prefix="skeeto-viewmodel-hook-") as tmp:
        folder = Path(tmp)
        (folder / "fixture.cpp").write_text(source, encoding="utf-8")
        (folder / "run.bat").write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\nif errorlevel 1 exit /b 1\ncl /nologo /EHsc /std:c++17 /utf-8 /W3 fixture.cpp /Fe:fixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", str(folder / "run.bat")], cwd=folder, check=True)
        subprocess.run([str(folder / "fixture.exe")], check=True)


if __name__ == "__main__":
    main()
