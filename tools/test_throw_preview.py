"""MSVC x86 fixtures for production throwable gates, cache and drawing bounds.

Extract the actual functions; engine/input/draw boundaries are counted stubs.
This checks work and lifecycle, not native TraceRay cost or in-game FPS.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PROXY = "src/skeeto/core/skeeto_proxy.cpp"


def function(source: str, name: str) -> str:
    # These private functions contain no braces in string literals.
    match = re.search(r"^static [^\n]+\b" + name + r"\([^;]*?\)\s*\{", source, re.M)
    if not match:
        raise AssertionError(f"missing production function: {name}")
    depth = 1
    cursor = source.index("{", match.start()) + 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor] + "\n"


PRELUDE = r'''
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <string>
using DWORD = uint32_t;
using LONG = int32_t;
constexpr int VK_LBUTTON = 1;
enum { kThrowStyleClassic, kThrowStyleBeam, kThrowStyleHelix, kThrowStyleTunnel, kThrowStyleCount };
constexpr int kThrowPathMax = 128;
constexpr DWORD kThrowLingerMs = 4000;
constexpr int kHazMolotov = 1, kHazBileGround = 2;
static bool g_optClientThrowLand = true, g_optHideAllWorldHud = false, g_optPauseSampling = false;
static int g_throwLandHz = 0;
static bool g_throwLandOnAttack = true;
static bool g_menuVisible = false, g_menuParked = false;
static std::atomic_bool g_run{true}, g_paintInGame{true}, g_throwLandReset{true};
static std::atomic<DWORD> g_spawnQuietUntil{0}, g_paintLastDtMs{8};
static std::atomic<int> g_dbgLifeState{0};
static void* g_trace = (void*)1;
static void* g_entlist = (void*)1;
static DWORD tick = 10000;
static unsigned epoch = 1;
static bool pressed = false, classReady = true, failPrediction = false, eyeReady = true;
static bool hazardWanted = false, quiet = false, feature = true;
static float angle = 0.f, nativeGravity = 800.f;
struct Counts { unsigned polls=0, views=0, inputs=0, classes=0, sims=0, projects=0, lines=0, fills=0, hazards=0; };
static Counts calls;
struct Pawn { int weapon; float origin[3], view[3], velocity[3]; };
static Pawn pawn{3, {0.f,0.f,0.f}, {0.f,0.f,64.f}, {0.f,0.f,0.f}};
struct Weapon { float throwTime; };
static Weapon weapons[16]{};
static bool weaponMissing = false;
static int g_offThrowTime = offsetof(Weapon,throwTime);
static int g_handleLookupState = 1;
static int g_offActiveWeapon = offsetof(Pawn,weapon), g_offOrigin = offsetof(Pawn,origin);
static int g_offViewOffset = offsetof(Pawn,view), g_offAbsVelocity = offsetof(Pawn,velocity);
static unsigned GetTickCount() { return tick; }
static short GetAsyncKeyState(int) { ++calls.polls; return pressed ? (short)0x8000 : 0; }
static bool IdleSwapHudQuietActive() { return quiet; }
static bool SkeetoFeaturesOn() { return feature; }
static unsigned SiDamageEpoch() { return epoch; }
static int EngLocal() { return 1; }
static void* EntGet(int index) { return index == 1 ? (void*)&pawn : (index > 1 && index < 16 ? (void*)&weapons[index] : nullptr); }
static void* EntFromHandle(int handle) { return weaponMissing ? nullptr : EntGet(handle); }
static bool EntReadable(void* p) { return p != nullptr; }
template<class T> static bool EntRead(const void* p,int off,T* out) {
    if (!p || off < 0) return false;
    std::memcpy(out,(const char*)p+off,sizeof(T));return true;
}
static bool EntReadBlock(const void* p,int off,void* out,size_t n) {
    ++calls.inputs;
    if (!p || off < 0 || !eyeReady) return false;
    std::memcpy(out,(const char*)p+off,n);return true;
}
static int EntIndexFromHandle(int handle) { return handle & 0xFFF; }
static const char* EntNetClassName(int index) {
    ++calls.classes;
    if (!classReady) return nullptr;
    switch(index) { case 3:return "CMolotov";case 4:return "CVomitJar";case 5:return "CPipeBomb";default:return "CRifle"; }
}
static void EngGetViewAngles(float* a) { ++calls.views; a[0]=0;a[1]=angle;a[2]=0; }
static bool LocalEyePos(float* eye) { ++calls.inputs;std::memcpy(eye,pawn.view,12);return eyeReady; }
static bool HazHudThrowNotifyWanted() { return hazardWanted; }
static void HazHudOnThrow(int,const float*,float) { ++calls.hazards; }
static bool MenuEnsureSurf() { return true; }
static bool SurfGetScreenSize(int* w,int* h) { *w=1280;*h=1080;return true; }
static void Log(const char*,...) {}
static bool LocalPlayWorldToScreen(const float* p,float* x,float* y,int,int) {
    ++calls.projects;*x=p[0];*y=p[1];return p[2]>=0;
}
static void SurfColor(int,int,int,int) {}
static void SurfLine(int,int,int,int) { ++calls.lines; }
static void SurfFill(int,int,int,int) { ++calls.fills; }
static bool ClientUxPredictThrowLand(int,void*,const float* eye,const float*,const float*,
    float* land,float path[][3],int* n,float* flight) {
    ++calls.sims;*n=0;*flight=0;
    if (failPrediction) return false;
    std::memcpy(path[0],eye,12);
    path[1][0]=150;path[1][1]=100;path[1][2]=1;
    std::memcpy(land,path[1],12);*n=2;*flight=0.6f;return true;
}
static void* CvarFind(const char*) { return (void*)1; }
static bool CvarReadPair(void*,int* n,float* f) { *f=nativeGravity;*n=(int)nativeGravity;return true; }
'''

CASES = r'''
static unsigned failures = 0;
static void require(bool ok,const char* message) {
    if (!ok) { ++failures;std::fprintf(stderr,"FAIL: %s\n",message); }
}
static void reset() {
    ++epoch;g_throwLandReset=true;tick=10000;pressed=false;classReady=true;eyeReady=true;
    pawn.weapon=3;angle=0;failPrediction=false;hazardWanted=false;quiet=false;feature=true;
    g_optClientThrowLand=true;g_throwLandHz=0;g_optHideAllWorldHud=false;g_optPauseSampling=false;
    g_throwLandOnAttack=false;weaponMissing=false;g_offThrowTime=offsetof(Weapon,throwTime);g_handleLookupState=1;
    for(auto& weapon:weapons)weapon={};
    g_menuVisible=false;g_menuParked=false;g_run=true;g_paintInGame=true;g_dbgLifeState=0;
    g_spawnQuietUntil=0;g_paintLastDtMs=8;calls={};
}
static void frame(unsigned advance=10) { tick+=advance;FrameDriver(); }
static void releaseThrow() { pressed=false;weapons[pawn.weapon].throwTime=10.2f;frame(); }
int main(int argc,char** argv) {
    const std::string selected=argc>1?argv[1]:"all";
    auto run=[&](const char* name,auto body) {
        if (selected!="all" && selected!=name) return;
        reset();const unsigned before=failures;body();
        std::printf("%s %s\n",failures==before?"PASS":"FAIL",name);
    };
    run("off",[] {
        g_optClientThrowLand=false;for(int i=0;i<100;++i)frame();
        require(!calls.classes&&!calls.views&&!calls.inputs&&!calls.sims&&!calls.projects,"off path does no module work");
    });
    run("nonthrow",[] {
        pawn.weapon=6;frame();calls={};for(int i=0;i<600;++i)frame();
        require(!calls.classes&&!calls.polls&&!calls.views&&!calls.inputs&&!calls.sims&&!calls.projects,"gun/empty path stays on cached weapon gate");
    });
    run("holster",[] {
        pressed=true;frame();pawn.weapon=6;frame();calls={};for(int i=0;i<100;++i)frame();
        require(!calls.polls&&!calls.views&&!calls.inputs&&!calls.sims&&!calls.projects,"held LMB followed by gun swap stops all launch sampling");
    });
    run("cancel-swap",[] {
        hazardWanted=true;pressed=true;frame();calls={};pawn.weapon=6;pressed=false;frame();
        require(!calls.hazards&&!calls.projects,"releasing LMB during a cancelled weapon swap never commits a hazard or linger");
    });
    run("unaccepted-release",[] {
        hazardWanted=true;pressed=true;frame();calls={};pressed=false;frame();
        require(!calls.hazards&&!calls.projects,"mouse release alone is not an accepted throw");
        pawn.weapon=6;frame();require(!calls.hazards,"later holster cancels the pending release");
    });
    run("accepted-release",[] {
        hazardWanted=true;pressed=true;frame();pressed=false;frame();calls={};
        weapons[3].throwTime=10.2f;frame();
        require(calls.hazards==1&&!calls.sims&&!calls.views&&!calls.inputs,"engine acceptance on the following frame commits the cached throw once");
        frame();require(calls.hazards==1,"accepted state cannot notify twice");
    });
    run("accepted-swap",[] {
        hazardWanted=true;pressed=true;frame();calls={};weapons[3].throwTime=10.2f;pawn.weapon=6;pressed=false;frame();
        require(calls.hazards==1&&!calls.sims&&!calls.inputs,"accepted old handle survives an automatic switch without repredicting");
    });
    run("missing-throw-state",[] {
        hazardWanted=true;pressed=true;frame();calls={};weaponMissing=true;pressed=false;frame();
        require(!calls.hazards,"a destroyed/reused handle cannot fall back to mouse release");
    });
    run("unverified-throw-state",[] {
        for(bool missingOffset:{false,true}) {
            reset();hazardWanted=true;pressed=true;frame();calls={};weapons[3].throwTime=10.2f;
            if(missingOffset)g_offThrowTime=-1;else g_handleLookupState=0;
            pressed=false;frame();require(!calls.hazards,"missing netvar or unverified handle serial cannot prove a throw");
        }
    });
    run("attack-display",[] {
        g_throwLandOnAttack=true;hazardWanted=true;frame();
        require(!calls.views&&!calls.inputs&&!calls.sims&&!calls.projects,"attack-only mode skips launch sampling and prediction while merely holding");
        pressed=true;calls={};frame();require(calls.sims==1&&calls.projects>0,"holding LMB shows the live preview");
        calls={};releaseThrow();require(calls.hazards==1&&!calls.projects&&!calls.sims,"release hides the trajectory immediately while accepted throw still notifies hazards");
        pawn.weapon=6;calls={};frame(100);require(!calls.projects,"attack-only mode also hides the post-throw linger");
    });
    run("stable",[] {
        pressed=true;frame();calls={};for(int i=0;i<600;++i)frame();
        require(calls.sims<=41,"stable aim is at most one simulation per 150 ms");
        require(!calls.classes,"held preview reuses cached weapon classification");
    });
    run("moving",[] {
        frame();calls={};for(int i=0;i<600;++i){angle+=0.5f;frame();}
        require(calls.sims<=120,"moving input is at most 20 Hz");
    });
    run("rates",[] {
        for(int hz:{10,20,25,30,60}) for(bool moving:{false,true}) {
            reset();g_throwLandHz=hz;frame(1);calls={};
            for(int i=0;i<1000;++i){if(moving)angle+=0.5f;frame(1);}
            const unsigned gap=(1000u+hz-1u)/hz;
            require(calls.sims==1000u/gap,"selected rate governs steady and moving aim, rounded up without overshooting");
            require(!calls.classes,"higher rate still reuses weapon classification");
            pawn.weapon=6;frame();calls={};frame(1000);
            require(!calls.sims&&!calls.views&&!calls.inputs,"selected rate cannot bypass the weapon gate");
        }
    });
    run("rate-change",[] {
        frame();calls={};g_throwLandHz=60;frame(17);
        require(calls.sims==1,"higher rate takes effect immediately without resetting the preview");
        calls={};g_throwLandHz=10;frame(99);require(!calls.sims,"lower rate is respected immediately");
        frame(1);require(calls.sims==1,"lower rate resumes at its interval");
        g_throwLandHz=0;calls={};frame(149);require(!calls.sims,"returning to default restores stable-aim throttling");
        frame(1);require(calls.sims==1,"default stable aim refreshes at 150 ms");
    });
    run("failure",[] {
        failPrediction=true;frame();calls={};for(int i=0;i<600;++i)frame();
        require(calls.sims<=41&&!calls.projects,"failed prediction stays throttled and has no stale marker");
    });
    run("release",[] {
        hazardWanted=true;pressed=true;frame();calls={};releaseThrow();
        require(!calls.sims&&!calls.views&&!calls.inputs,"release uses a snapshot without launch sampling or simulation");
        require(calls.hazards==1&&calls.fills==5,"release retains hazard notification and landing marker");
    });
    run("linger",[] {
        pressed=true;frame();releaseThrow();pawn.weapon=6;frame();calls={};
        frame(100);require(!calls.sims&&!calls.views&&!calls.inputs&&calls.projects>0,"linger only projects cached geometry");
        calls={};frame(4100);require(!calls.projects&&!calls.sims&&!calls.views&&!calls.inputs,"expired linger does no drawing or sampling");
    });
    run("reselect",[] {
        pressed=true;frame();releaseThrow();pawn.weapon=4;calls={};frame(50);
        require(calls.sims==1,"a new throwable immediately replaces the old linger with a live preview");
    });
    run("lateclass",[] {
        classReady=false;frame();require(!calls.sims,"unresolved weapon has no preview");
        classReady=true;calls={};frame(50);require(calls.sims==1,"same handle retries an initially unavailable class");
    });
    run("dead",[] {
        g_dbgLifeState=1;frame();require(!calls.sims&&!calls.views&&!calls.inputs,"dead pawn with retained weapon does not predict");
    });
    run("wrap",[] {
        tick=0xFFFFFF00u;pressed=true;frame(1);weapons[3].throwTime=10.2f;pressed=false;frame(1);pawn.weapon=6;calls={};frame(1);
        require(calls.projects>0,"linger survives tick-count wrap");calls={};frame(4001);
        require(!calls.projects&&!calls.sims,"wrapped linger expires exactly once");
    });
    run("visibility",[] {
        g_optHideAllWorldHud=true;frame();require(!calls.sims&&!calls.inputs,"hidden world HUD without a consumer does no sampling");
        hazardWanted=true;pressed=true;calls={};frame(100);
        require(calls.sims==1&&!calls.projects,"hazard consumer keeps data but hidden HUD does not draw");
        releaseThrow();require(calls.hazards==1,"hidden HUD still notifies enabled hazard consumer");
        g_optPauseSampling=true;calls={};frame(100);require(!calls.sims&&!calls.views&&!calls.inputs&&!calls.projects,"pause stops producer and drawing");
    });
    run("lifecycle",[] {
        pressed=true;frame();releaseThrow();++epoch;pressed=false;calls={};frame();
        require(calls.sims==1&&!calls.hazards,"map boundary clears lock and held input");
        pressed=true;frame();g_optClientThrowLand=false;g_throwLandReset=true;frame();
        g_optClientThrowLand=true;pressed=false;calls={};frame();require(!calls.hazards,"off/on reset cannot commit a stale release");
    });
    run("inputs",[] {
        frame();eyeReady=false;calls={};frame(100);require(!calls.sims&&!calls.projects,"unavailable launch input invalidates the marker");
    });
    run("hitch",[] {
        frame();g_paintLastDtMs=60;calls={};frame(100);require(!calls.sims,"existing hitch gate skips simulation");
        calls={};frame(301);require(!calls.projects,"hitch-stale marker expires instead of persisting");
    });
    run("drawing",[] {
        float path[kThrowPathMax][3]{};
        for(int i=0;i<kThrowPathMax;++i){path[i][0]=i*4.f;path[i][1]=100.f;path[i][2]=1.f;}
        const unsigned bounds[]={127,635,329,349};
        for(int style=0;style<kThrowStyleCount;++style){
            g_throwLandStyle=style;calls={};DrawFixture(path,kThrowPathMax,false);
            require(calls.projects==128&&calls.lines<=bounds[style]&&!calls.fills,"style has bounded projections/lines and partial path has no marker");
            std::printf("  style=%d projections=%u lines=%u bound=%u\n",style,calls.projects,calls.lines,bounds[style]);
            for(auto& p:path){p[2]=-1.f;}calls={};DrawFixture(path,kThrowPathMax,false);
            require(!calls.lines&&!calls.fills,"failed projection does not create screen-corner geometry");
            for(auto& p:path){p[2]=1.f;}
        }
    });
    run("gravity",[] {
        for(float gravity:{0.f,0.5f,-200.f,800.f}) {
            nativeGravity=gravity;require(ClientUxSvGravity()==gravity,"valid zero/small/reversed gravity is not replaced by 800");
        }
    });
    std::printf("throw preview failures=%u\n",failures);
    return failures?1:0;
}
'''


def run() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-ref", help="read an immutable Git revision for a regression red run")
    parser.add_argument("--case", default="all")
    args = parser.parse_args()
    source = subprocess.check_output(["git", "show", f"{args.source_ref}:{PROXY}"], cwd=ROOT).decode("utf-8") if args.source_ref else (ROOT / PROXY).read_text(encoding="utf-8-sig")
    code = PRELUDE
    style = re.search(r"^static int g_throwLandStyle = [^;]+;", source, re.M)
    if not args.source_ref:
        assert style and style.group() == "static int g_throwLandStyle = kThrowStyleBeam;", "initial style must be beam"
        assert "g_throwLandStyle = v >= 0 && v < kThrowStyleCount ? v : kThrowStyleBeam;" in source, "valid saved styles must be retained"
        assert "g_throwLandStyle = kThrowStyleBeam;" in function(source, "LoadSettings"), "missing style setting must default to beam"
        assert "static bool g_optClientThrowLand = false;" in source, "feature stays off by default"
        assert "static int g_throwLandHz = 0;" in source, "refresh rate must default to the original adaptive cadence"
        assert "g_throwLandHz = 0;" in function(source, "LoadSettings"), "missing refresh setting must restore adaptive cadence"
        assert "static bool g_throwLandOnAttack = true;" in source, "attack-only display must be the initial default"
        assert "g_throwLandOnAttack = true;" in function(source, "LoadSettings"), "existing ini without the new key uses attack-only display"
        assert 'fprintf(f, "client_throw_land_on_attack=%d\\n", g_throwLandOnAttack ? 1 : 0);' in source, "display mode must persist"
        assert 'sscanf(line, "client_throw_land_on_attack=%d", &v)' in function(source, "LoadSettingsClientUxLine"), "saved display mode must load"
        print("PASS defaults: beam, saved selection preserved, feature disabled", flush=True)
    code += style.group() + "\n" if style else "static int g_throwLandStyle = kThrowStyleClassic;\n"
    for name in ("ClientUxLocalThrowableKind", "ClientUxThrowAccepted", "ClientUxSvGravity", "HazHudThrowableKind", "ClientUxDrawStyledThrowPath", "ClientUxDrawThrowOverlay", "ClientUxPaintThrowLand"):
        if re.search(r"^static [^\n]+\b" + name + r"\(", source, re.M):
            code += function(source, name)
    caller = re.search(r"if \((g_optClientThrowLand[^\n]+)\)\s*ClientUxPaintThrowLand\(\);", source)
    assert caller, "production Paint must gate throwable work explicitly"
    code += "static void FrameDriver() { const bool worldHudVisible=!g_optHideAllWorldHud&&!g_optPauseSampling; " + caller.group() + " }\n"
    overlay = function(source, "ClientUxDrawThrowOverlay")
    has_now = "DWORD now" in overlay.split("{", 1)[0]
    code += "static void DrawFixture(const float path[][3],int n,bool landed) { float land[3]={100,100,1};ClientUxDrawThrowOverlay(land,path,n,false,1280,1080," + ("tick," if has_now else "") + "landed); }\n"
    code += CASES
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    with tempfile.TemporaryDirectory(prefix="skeeto-throw-preview-") as temp:
        folder = Path(temp)
        (folder / "fixture.cpp").write_text(code, encoding="utf-8")
        (folder / "run.bat").write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 /W3 /wd4505 fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe {args.case}\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", "run.bat"], cwd=folder, check=True)


if __name__ == "__main__":
    run()
