"""Run the production TPS geometry and command shaping in an MSVC x86 harness.

The fixtures check actual aim/movement results, including near walls and yaw wrap.
They do not substitute for a running L4D2 camera or remote server acceptance.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import re
import check_ui_text

ROOT = Path(__file__).resolve().parents[1]

HOOK_BOUNDARIES = r'''
enum class TpsAimState { Off, Waiting, Unsupported, Conflict, FirstPerson, Paused, Active, NoRay, Guarding, Recovering };
using DWORD=unsigned;
static TpsAimState g_tpsAimState=TpsAimState::Off;
static std::atomic<void*> g_tpsAimOriginal{nullptr};
static unsigned g_paintTid=7;
static void* g_tpsAimMode=(void*)1234;
static bool g_optClientTpsAim=true;
static bool g_optClientTpsAimAac=true;
static std::atomic<bool> g_run{true},g_tpsAimResetRequested{false};
struct TpsAimLastCommand {
    void* pointer=nullptr; TpsAimCommandPrefix raw{}, shaped{}; unsigned long at=0; bool valid=false;
    float eye[3]{},point[3]{};
};
static TpsAimLastCommand g_tpsAimLast;
static struct { float camera[3]{}; } g_tpsAimPose;
static TpsAimAacState g_tpsAimAac;
static unsigned g_tpsAimAacLimited=0,g_tpsAimAacBlocked=0;
static unsigned g_tpsAimCommands=0,g_tpsAimApplied=0;
static std::atomic<unsigned> g_tpsAimInputThread{0},g_tpsAimForeignCalls{0};
static bool scene=true,loading=false,gameUi=false,third=true,pawn=true,ray=true,policy=true,upstreamResult=true;
static bool g_hudPostMissionQuiet=false,g_idleViewArmed=false,g_idleViewCapturePending=false;
static std::atomic<bool> g_hudTransitionQuiet{false},g_mapEnterGameThreadResetPending{false},g_xhairHideScene{false};
static bool g_optHideAllXhair=false,g_optXhair=false,g_optXhairRing=false,g_optXhairTex=false,g_optXhairThirdPerson=false;
static unsigned thread=7,tick=12,upstreamCalls=0,traceCalls=0,viewCalls=0;
static float target[3]={100,-20,5},lastView[3]{};
static unsigned GetCurrentThreadId() { return thread; }
static unsigned GetTickCount() { return tick; }
static bool SkeetoFeaturesOn() { return policy; }
static bool TpsAimReadable(const void* p,size_t) { return p!=nullptr; }
static void TpsAimClear() {
    g_tpsAimLast=TpsAimLastCommand{};
    if (g_optClientTpsAimAac) {
        if (!g_tpsAimAac.interrupted) g_tpsAimAac.Suspend(GetTickCount());
    } else g_tpsAimAac=TpsAimAacState{};
}
static bool TpsAimNativeThirdPerson() { return third; }
static void* TpsAimLocalPawn(float* eye) { eye[0]=eye[1]=eye[2]=0;return pawn?(void*)5678:nullptr; }
static bool TpsAimCenterPoint(void*,const float*,float* point) { ++traceCalls;std::memcpy(point,target,sizeof(target));return ray; }
static void EngSetViewAngles(const float* a) { ++viewCalls;std::memcpy(lastView,a,sizeof(lastView)); }
static void* g_engine=(void*)9876;
static void* g_entlist=(void*)1;
static void* g_trace=(void*)1;
static bool EngInGame() { return scene; }
static bool EngDrawingLoading() { return loading; }
static bool EngGameUiVisible() { return gameUi; }
static void* VGet(void*,int) { return (void*)1; }
static bool IsExec(void*) { return true; }
static bool __fastcall upstream(void*,void*,float,void*) { ++upstreamCalls;return upstreamResult; }
'''

DRAW_BOUNDARIES = r'''
static int g_xhairDrawH=0,g_hudXhairSig=0,g_hudXhairDrawH=0,g_hudXhairWin=0,g_hudXhairId=99;
static bool g_hudXhairOk=false,g_hudXhairAttempted=false,bakeResult=false;
static float g_hudXhairUv=1;
static int drawHeight=720,style=3,windowSize=64,bakes=0,fallbacks=0,textures=0;
static bool SkeetoXhairSuppressed() { return false; }
static bool MenuEnsureSurf() { return true; }
static bool SurfGetScreenSize(int* w,int* h) { *w=1280;*h=drawHeight;return true; }
static int XhairWantWin() { return windowSize; }
static int XhairStyleSig() { return style; }
static bool XhairBakeHudTexture() { ++bakes;return bakeResult; }
static void SurfColor(int,int,int,int) {}
static void SurfDrawTexturedUv(int,int,int,int,int,float,float,float,float) { ++textures; }
static void XhairDrawHudFallback(int,int) { ++fallbacks; }
static int g_hudXhairIds[5]={-1,-1,-1,-1,-1},allocatedIds=0;
static bool g_hudXhairLoggedFail=false;
static void* g_surf=(void*)1;
static int createId(void*,bool) { return ++allocatedIds; }
static void Log(const char*,int) {}
'''

CASES = r'''
static bool near(float a, float b, float eps=0.001f) { return std::fabs(a-b)<eps; }
static int viewWrites=0;
static float keptView[3]{};
static void keepView(const float* a) { ++viewWrites;std::memcpy(keptView,a,sizeof(keptView)); }
int main() {
    float eye[3]={0,0,0}, angles[3]={0,0,0}, out[3]={99,99,99};
    float point[3]={100,-20,5};
    assert(TpsAimAnglesToPoint(eye,point,angles,out));
    // Hand-derived atan2(-20,100) and -atan2(5,sqrt(100^2+20^2)).
    assert(near(out[0],-2.806370f) && near(out[1],-11.309932f) && out[2]==0);
    point[0]=10; point[1]=-20; point[2]=0;
    assert(TpsAimAnglesToPoint(eye,point,angles,out));
    assert(near(out[1],-63.434949f)); // no arbitrary 10-degree cap near cover
    point[0]=-10; point[1]=point[2]=0;
    const float cameraForward[3]={1,0,0};
    assert(!TpsAimPointAheadOfEye(eye,point,cameraForward)); // cover behind the eyes
    assert(TpsAimAnglesToPoint(eye,point,angles,out)); // geometry itself has no arbitrary turn cap
    angles[1]=179;point[0]=100;
    assert(TpsAimPointAheadOfEye(eye,point,cameraForward));
    assert(TpsAimAnglesToPoint(eye,point,angles,out)&&near(out[1],0)); // native camera lag during a fast turn
    angles[1]=0;
    point[0]=point[1]=point[2]=0;
    assert(!TpsAimAnglesToPoint(eye,point,angles,out));
    point[0]=std::numeric_limits<float>::quiet_NaN();
    assert(!TpsAimAnglesToPoint(eye,point,angles,out));
    point[0]=std::numeric_limits<float>::infinity();
    assert(!TpsAimAnglesToPoint(eye,point,angles,out));
    angles[1]=179; point[0]=-100; point[1]=-1.745506f; point[2]=0;
    assert(TpsAimAnglesToPoint(eye,point,angles,out));
    assert(near(out[1],-179.f)); // normalize across the +/-180 seam

    const float moves[][2]={{450,0},{0,450},{450,450},{-225,120},{0,0}};
    const float turns[][2]={{0,90},{179,-179},{-90,90},{23,17},{0,-63.434949f}};
    for(auto& move:moves) for(auto& turn:turns) {
        float f=move[0],s=move[1];
        assert(TpsAimPreserveMovement(turn[0],turn[1],&f,&s));
        const double oldYaw=turn[0]*3.141592653589793/180.0;
        const double newYaw=turn[1]*3.141592653589793/180.0;
        // An independent world-space reconstruction, not the shaping formula.
        assert(std::fabs((move[0]*std::cos(oldYaw)+move[1]*std::sin(oldYaw))
            -(f*std::cos(newYaw)+s*std::sin(newYaw)))<0.0003);
        assert(std::fabs((move[0]*std::sin(oldYaw)-move[1]*std::cos(oldYaw))
            -(f*std::sin(newYaw)-s*std::cos(newYaw)))<0.0003);
    }
    float f=450,s=0;
    assert(TpsAimPreserveMovement(0,90,&f,&s));
    assert(near(f,0) && near(s,450));
    f=std::numeric_limits<float>::quiet_NaN();s=12;
    assert(!TpsAimPreserveMovement(0,90,&f,&s) && s==12);

    // Native shoulder projection: the three independent projection rows recover
    // the exact camera origin and forward even when FOV/aspect scaling changes.
    const float matrix[4][4]={{0,-1,0,-20},{0,0,1,-69},{0,0,0,0},{1,0,0,40}};
    float camera[3]{},direction[3]{};
    assert(TpsAimCameraFromMatrix(matrix,camera,direction));
    assert(near(camera[0],-40)&&near(camera[1],-20)&&near(camera[2],69));
    assert(near(direction[0],1)&&near(direction[1],0)&&near(direction[2],0));
    for(float scale:{0.3f,1.f,2.4f}) {
        float m[4][4];std::memcpy(m,matrix,sizeof(m));
        for(int j=0;j<4;++j){m[0][j]*=scale;m[1][j]*=scale*1.7f;}
        assert(TpsAimCameraFromMatrix(m,camera,direction));
        assert(near(camera[0],-40)&&near(camera[1],-20)&&near(camera[2],69));
    }
    float broken[4][4]{};
    assert(!TpsAimCameraFromMatrix(broken,camera,direction));
    broken[0][0]=std::numeric_limits<float>::infinity();
    assert(!TpsAimCameraFromMatrix(broken,camera,direction));
    TpsAimCommandPrefix cmd{};
    cmd.vtable=0x12345678;cmd.number=31;cmd.tick=99;cmd.buttons=0x8001;
    cmd.forward=450;cmd.side=100;cmd.up=12;cmd.angles[2]=0;
    TpsAimCommandPrefix saved=cmd;
    angles[0]=angles[1]=angles[2]=0;
    point[0]=100;point[1]=-20;point[2]=5;
    assert(TpsAimShapeCommand(&cmd,eye,point));
    assert(near(cmd.angles[1],-11.309932f));
    assert(cmd.number==saved.number&&cmd.tick==saved.tick&&cmd.buttons==saved.buttons
        &&cmd.vtable==saved.vtable&&cmd.up==saved.up);
    saved=cmd;cmd.number=0;saved.number=0;
    assert(!TpsAimShapeCommand(&cmd,eye,point));
    assert(std::memcmp(&cmd,&saved,sizeof(cmd))==0);
    cmd.number=32;cmd.side=std::numeric_limits<float>::infinity();saved=cmd;
    assert(!TpsAimShapeCommand(&cmd,eye,point));
    assert(std::memcmp(&cmd,&saved,sizeof(cmd))==0); // failure is atomic
    cmd=saved;cmd.number=33;cmd.side=100;cmd.angles[0]=cmd.angles[1]=0;
    saved=cmd;bool applied=false;
    assert(!TpsAimCommitCommand(&cmd,eye,point,true,keepView,&applied));
    assert(applied && viewWrites==1 && keptView[0]==saved.angles[0]
        && keptView[1]==saved.angles[1] && near(cmd.angles[1],-11.309932f));
    cmd=saved;applied=false;
    assert(!TpsAimCommitCommand(&cmd,eye,point,false,keepView,&applied));
    assert(applied && viewWrites==1); // upstream false already owns camera handling
    cmd=saved;cmd.number=0;saved=cmd;applied=false;
    assert(TpsAimCommitCommand(&cmd,eye,point,true,keepView,&applied));
    assert(!applied && viewWrites==1 && std::memcmp(&cmd,&saved,sizeof(cmd))==0);
    cmd.number=34;saved=cmd;applied=false;
    assert(TpsAimCommitCommand(&cmd,eye,point,true,nullptr,&applied));
    assert(!applied && std::memcmp(&cmd,&saved,sizeof(cmd))==0); // missing camera setter
    g_tpsAimOriginal.store((void*)upstream);
    alignas(4) unsigned char storage[0x58];
    std::memset(storage,0x5a,sizeof(storage));
    auto live=(TpsAimCommandPrefix*)storage;
    *live=TpsAimCommandPrefix{};live->number=77;live->tick=123;live->forward=450;
    live->side=120;live->up=12;live->buttons=0x8001;
    unsigned char before[0x58];std::memcpy(before,storage,sizeof(before));
    g_optClientTpsAim=false;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(upstreamCalls==1 && traceCalls==0 && viewCalls==0
        && !std::memcmp(before,storage,sizeof(before)));
    g_optClientTpsAim=true;live->number=0;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(upstreamCalls==2 && traceCalls==0); // extra mouse sample true branch remains native
    live->number=77;third=false;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(upstreamCalls==3 && traceCalls==0 && !std::memcmp(before,storage,sizeof(before)));
    third=true;thread=8;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    thread=7;
    assert(Hooked_TpsAimCreateMove((void*)1111,nullptr,0.03f,live)); // secondary split-screen
    pawn=false;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(traceCalls==0);pawn=true;tick+=TpsAimAacState::kRecoveryMs;
    assert(!Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(upstreamCalls==7 && traceCalls==1 && viewCalls==1
        && near(lastView[0],0) && near(lastView[1],0));
    assert(near(live->angles[0],-2.806370f) && near(live->angles[1],-11.309932f));
    assert(!std::memcmp(before+0x28,storage+0x28,0x30)); // full retail suffix unchanged
    const TpsAimCommandPrefix once=*live;
    assert(!Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(upstreamCalls==8 && traceCalls==1 && viewCalls==2
        && !std::memcmp(live,&once,sizeof(once)) && near(lastView[1],0));
    // A new native input starts from the uncorrected look; correction cannot accumulate.
    std::memcpy(storage,before,sizeof(storage));live->number=78;upstreamResult=false;
    assert(!Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(viewCalls==2 && traceCalls==2 && near(live->angles[1],-11.309932f));
    upstreamResult=true;std::memcpy(storage,before,sizeof(storage));live->number=79;
    target[0]=std::numeric_limits<float>::quiet_NaN();
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(viewCalls==2 && live->angles[1]==0 && live->forward==450 && live->side==120);
    target[0]=100;ray=false;live->number=80;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(viewCalls==2 && live->angles[1]==0);ray=true;
    const unsigned traces=traceCalls;
    g_tpsAimResetRequested.store(true);live->number=81;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(traceCalls==traces && !g_tpsAimLast.valid && live->angles[1]==0);
    g_tpsAimResetRequested.store(false);scene=false;
    assert(Hooked_TpsAimCreateMove(g_tpsAimMode,nullptr,0.03f,live));
    assert(traceCalls==traces);
    // The production scene gate must not depend on the old visual hide timer.
    scene=true;g_xhairHideScene.store(true);g_optHideAllXhair=true;
    assert(TpsAimSceneReady());
    g_hudTransitionQuiet.store(true);assert(!TpsAimSceneReady());
    g_hudTransitionQuiet.store(false);g_mapEnterGameThreadResetPending.store(true);
    assert(!TpsAimSceneReady());g_mapEnterGameThreadResetPending.store(false);
    assert(TpsAimSceneReady());
    assert(CrosshairAllOff()); // global visibility hides the reticle only
    g_optHideAllXhair=false;assert(!CrosshairAllOff()); // TPS alone owns the reticle
    g_optClientTpsAim=false;assert(CrosshairAllOff());g_optClientTpsAim=true;
    g_optXhair=true;g_xhairHideScene.store(false);
    for (int i=0;i<120;++i) SurfDrawHudXhair();
    assert(bakes==1 && fallbacks==120); // failed GDI bake is not retried every frame
    ++style;SurfDrawHudXhair();assert(bakes==2);
    ++drawHeight;SurfDrawHudXhair();assert(bakes==3);
    windowSize=128;bakeResult=true;SurfDrawHudXhair();assert(bakes==4 && textures==1);
    for (int i=0;i<120;++i) SurfDrawHudXhair();
    assert(bakes==4 && textures==121);
    for (int cycle=0;cycle<120;++cycle) for (int pot:{32,64,128,256,512}) {
        assert(SelectHudTexture(pot));
        assert(g_hudXhairId>=1 && g_hudXhairId<=5);
    }
    assert(allocatedIds==5); // actual production selection never loses an old-size ID
    std::puts("PASS: TPS close/far aim, invalid rays, yaw wrap, world movement and camera projection");
    std::puts("PASS: TPS-only/global-hide scene isolation and bounded reticle bake attempts");
}
'''

def run():
    proxy = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
    begin = "// TPS_AIM_GEOMETRY_BEGIN"
    end = "// TPS_AIM_GEOMETRY_END"
    if begin not in proxy or end not in proxy:
        raise AssertionError("TPS aim production geometry is not implemented yet")
    geometry = proxy.split(begin, 1)[1].split(end, 1)[0]
    aac_header = (ROOT / "src/skeeto/core/skeeto_tps_aim_aac.h").read_text(encoding="utf-8-sig").replace("#pragma once\n", "")
    aac_commit = proxy.split("// TPS_AIM_AAC_COMMIT_BEGIN", 1)[1].split("// TPS_AIM_AAC_COMMIT_END", 1)[0]
    hook = proxy.split("// TPS_AIM_HOOK_BEGIN", 1)[1].split("// TPS_AIM_HOOK_END", 1)[0]
    scene_gate = proxy.split("static bool TpsAimSceneReady() {", 1)[1].split("static bool TpsAimCenterPoint", 1)[0]
    scene_gate = "static bool TpsAimSceneReady() {" + scene_gate
    draw = proxy.split("static void SurfDrawHudXhair() {", 1)[1].split("static void XhairSyncEngineHud", 1)[0]
    draw = "static void SurfDrawHudXhair() {" + draw
    xhair_tick = proxy.split("static void XhairTick() {", 1)[1]
    all_off = re.search(r'if \((g_optHideAllXhair \|\| \([^\n]+\))\) \{', xhair_tick).group(1)
    source = "#include <cmath>\n#include <cstring>\n#include <cassert>\n#include <cstdio>\n#include <cstdint>\n#include <cstddef>\n#include <limits>\n#include <initializer_list>\n#include <atomic>\n"
    source += aac_header + geometry + HOOK_BOUNDARIES + scene_gate + aac_commit + hook + DRAW_BOUNDARIES + draw
    texture_selection = proxy.split("\tint textureSlot = 0;",1)[1].split("\tSurfSetTextureRGBA(g_hudXhairId",1)[0]
    source += "static bool SelectHudTexture(int pot) { int textureSlot = 0;" + texture_selection + "return true; }\n"
    source += "static bool CrosshairAllOff() { return " + all_off + "; }\n" + CASES
    # Validate the new UI against real calls, without inheriting unrelated catalog defects.
    catalog = dict(re.findall(r'SKEETO_UI\((TpsAim\w+),\s*('+check_ui_text.WIDE+r')\)',
        (ROOT / "src/skeeto/core/skeeto_ui_en.inc").read_text(encoding="utf-8-sig")))
    calls = '\n'.join(m.group() for m in re.finditer(
        r'UiText\(UiId::TpsAim\w+,\s*'+check_ui_text.WIDE+r'\)',proxy))
    errors, used = check_ui_text.validate_source(calls,catalog,{})
    assert not errors and set(catalog)==used, (errors,set(catalog)-used)
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-tps-aim-") as temp:
        folder = Path(temp)
        (folder / "fixture.cpp").write_text(source, encoding="utf-8")
        (folder / "run.bat").write_text(
            f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
            'if errorlevel 1 exit /b 1\n'
            'cl /nologo /EHsc /std:c++17 /utf-8 /W4 /WX fixture.cpp /Fe:fixture.exe\n'
            'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)

if __name__ == "__main__":
    run()
