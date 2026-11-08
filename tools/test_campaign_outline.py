"""Compile the production outline ownership code against a serial-aware entity fixture."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


def main():
    source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8')
    assert '// Campaign outline ownership begin' in source, 'campaign outline ownership is not implemented'
    assert 'g_campaignWitchPublished' not in source, 'Witch no longer needs an Event queue'
    assert 'g_campaignWitchStopped' not in source
    assert 'static bool g_optCampaignTankPropOutline = false;' in source
    assert 'm_hasTankGlow' in source
    assert 'g_optCampaignTankPropOutline && tankPresent' in source
    assert 'CampaignAlarmModelPath' not in source
    assert 'g_campaignAlarmModels' not in source
    assert 'static bool g_optCampaignWitchOutline = false;' in source
    assert 'static bool g_optCampaignTankBodyOutline = false;' in source
    assert 'refresh(g_campaignTanks, campaign && g_optCampaignTankBodyOutline' in source
    assert 'static bool g_optLaserSightOutline = false;' in source
    assert 'static bool g_optInteractScan = false;' in source
    assert 'kCampaignLaserCap = 8' in source
    assert 'kCampaignLaserRange = 2000' in source
    assert 'kInteractScanCap = 96' in source
    assert 'skeeto_laser_outline' in source
    assert 'skeeto_interact_scan' in source
    assert 'InteractScanPump' in source
    assert 'CDynamicProp' in source
    assert 'ToggleLaserOutline' not in source.split('kCampaignOutlineMenuItems[]', 1)[1].split('};', 1)[0]
    assert 'g_optButtonOutline' not in source
    assert 'CampaignOutlineButton' not in source
    assert 'CampaignOutlineRestore(slot); else slot = {}; }' in source
    assert 'g_campaignTankBodyColor' in source
    assert 'if (!HandleLookupUsable()) return;' in source
    assert 'CampaignOutlineClear(!loading)' in source
    assert 'InteractScanClear(restore)' in source
    assert 'kCampaignCarCap = 24, kCampaignWitchCap = 8, kCampaignTankCap = 8' in source
    assert 'n < 128 && hi >= 33' in source
    assert source.count('kCampaignOutlineMenuItems[') >= 2
    assert 'g_campaignMapEpoch.fetch_add' in source
    ownership = source.split('// Campaign outline ownership begin', 1)[1].split('// Campaign outline ownership end', 1)[0]
    fixture = r'''
#include <Windows.h>
#include <cstdint>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <cstdlib>
struct Fake { int handle; int glow[4]; int flashing; float cached[3]; unsigned char life, tankGlow; float rage, wander; short laserModel; int laserEffects; unsigned laserOwner; } entities[2048]{};
struct RecvProxyDataPod { const void* recvProp; int valueInt; int pad0[2]; int valueType,element,objectId; };
using RecvVarProxyFn=void(*)(const void*,void*,void*);
static void SetColor(const void* data,void* p,void* out) {
 int color=((const RecvProxyDataPod*)data)->valueInt; *(int*)out=color;
 float* rgb=(float*)((char*)p+24);for(int i=0;i<3;++i)rgb[i]=(float)((color>>(8*i))&255)/255.f;
}
static int setterCalls=0;
static void* EntFromHandle(int h) { int i=h&4095; return i>0&&i<2048&&entities[i].handle==h ? &entities[i] : nullptr; }
template<class T> bool EntRead(void* e,int off,T* out) { if(!e)return false; memcpy(out,(char*)e+off,sizeof(T)); return true; }
static bool EntReadBlock(void* e,int off,void* out,size_t size) { if(!e)return false; memcpy(out,(char*)e+off,size); return true; }
template<class T> bool EntWrite(void* e,int off,const T& value) { memcpy((char*)e+off,&value,sizeof(T)); return true; }
static void __thiscall SetType(void* p,int v) { ++setterCalls; *(int*)((char*)p+4)=v; }
static int g_campaignWitchRagdollOffset=offsetof(Fake,life);
static int g_campaignTankGlowOffset=offsetof(Fake,tankGlow);
static constexpr int kEfNoDraw=32;
static int g_campaignLaserModelKey=-1;
static int g_offModelIndex=offsetof(Fake,laserModel), g_offEffects=offsetof(Fake,laserEffects), g_offOwnerEntity=offsetof(Fake,laserOwner);
static void require(bool b,const char* msg) { if(!b) {puts(msg);exit(1);} }
'''
    # MSVC free functions cannot use __thiscall: production ABI is retained as a pointer,
    # while this fixture supplies the native setter via a tiny x86 thunk below.
    fixture = fixture.replace('static void __thiscall SetType(void* p,int v) { ++setterCalls; *(int*)((char*)p+4)=v; }',
        'static __declspec(naked) void SetType() { __asm { inc setterCalls\n mov eax, [esp+4]\n mov [ecx+4], eax\n ret 4 } }')
    alive = function(source, 'static bool CampaignOutlineWitchAlive(void* entity) {')
    props = function(source, 'static bool CampaignOutlineTankProp(void* entity) {')
    laser = function(source, 'static bool CampaignOutlineLaserSight(void* entity) {')
    mode_gate = next(line for line in function(source, 'static void CampaignOutlinePump() {').splitlines()
                     if 'const bool campaign =' in line)
    fixture += 'static bool CampaignModeAllowed(const char* mode) {\n' + mode_gate + '\nreturn campaign;\n}\n'
    fixture += ownership + alive + props + laser + r'''
int main() {
 require(CampaignModeAllowed("realism")&&CampaignModeAllowed("REALISM"),"Realism allows all three campaign outlines");
 require(CampaignModeAllowed("coop")&&CampaignModeAllowed("COOP"),"coop campaign remains allowed");
 const char* blockedModes[]={nullptr,"","versus","mutation12","survival","scavenge","mutation1"};
 for(const char* mode : blockedModes)
  require(!CampaignModeAllowed(mode),"other or unknown modes remain excluded");
 g_campaignGlowTypeOffset=offsetof(Fake,glow); g_campaignGlowOffset=g_campaignGlowTypeOffset-4;
 g_campaignSetGlowType=(CampaignSetGlowTypeFn)&SetType;
 g_campaignGlowColorProxy=&SetColor;
 Fake& e=entities[50]; e.handle=0x1032; e.glow[0]=0; e.glow[1]=77; e.glow[2]=9; e.glow[3]=123;
 CampaignOutlineSlot slot{};
 require(CampaignOutlineAcquire(slot,&e,e.handle,1,0xabcdef,1500),"acquire");
 require(e.cached[0]>0.93f&&e.cached[1]>0.80f,"native cached color is updated, not just the network integer");
 require(e.glow[0]==3 && e.glow[1]==1500 && e.glow[2]==0 && e.glow[3]==0xabcdef,"native persistent warning values");
 CampaignOutlineRestore(slot);
 require(e.glow[0]==0&&e.glow[1]==77&&e.glow[2]==9&&e.glow[3]==123&&!slot.handle,"exact restore");
 require(CampaignOutlineAcquire(slot,&e,e.handle,1,234,1500),"reacquire");
 e.glow[3]=777; e.glow[1]=900; // received newer server values
 CampaignOutlineRestore(slot);
 require(e.glow[3]==777&&e.glow[1]==900&&e.glow[0]==0,"preserve external updates per field");
 e.glow[0]=3;
 require(!CampaignOutlineAcquire(slot,&e,e.handle,1,234,1500),"do not replace existing server glow");
 e.glow[0]=0;
 require(CampaignOutlineAcquire(slot,&e,e.handle,2,234,1500),"witch independent owner");
 e.handle += 0x1000; const int calls=setterCalls;
 CampaignOutlineRestore(slot);
 require(setterCalls==calls&&e.glow[0]==3&&!slot.handle,"serial reuse never restores into another entity");
 e.life=0;e.rage=0;e.wander=0;
 require(CampaignOutlineWitchAlive(&e),"calm Witch eligible");
 e.rage=1;e.wander=1;
 require(CampaignOutlineWitchAlive(&e),"startled Witch remains outlined");
 e.life=1;
 require(!CampaignOutlineWitchAlive(&e),"ragdoll is not outlined");
 e.tankGlow=1;
 require(CampaignOutlineTankProp(&e),"any model with the Tank punch flag is eligible");
 e.tankGlow=0;
 require(!CampaignOutlineTankProp(&e),"same prop without the Tank punch flag is not eligible");
 g_campaignLaserModelKey=42; e.laserModel=42; e.laserEffects=0; e.laserOwner=0;
 require(CampaignOutlineLaserSight(&e),"stock unowned laser sight");
 e.laserEffects=kEfNoDraw;
 require(!CampaignOutlineLaserSight(&e),"hidden laser sight");
 e.laserEffects=0; e.laserOwner=5;
 require(!CampaignOutlineLaserSight(&e),"held laser sight");
 e.laserOwner=~0u;
 require(CampaignOutlineLaserSight(&e),"empty owner handle still counts as a world pickup");
 e.laserModel=7;
 require(!CampaignOutlineLaserSight(&e),"other upgrade model");
 g_campaignLaserModelKey=-1;
 require(!CampaignOutlineLaserSight(&e),"model index is not queried yet");
 require(g_campaignOwnedCount==0,"ownership count returns to zero");
 puts("PASS outline ownership: native registration, exact restore, foreign writes, existing glow, calm/startled/dead Witch, Tank hittable flag, laser pickup and serial reuse");
}
'''
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-campaign-outline-') as tmp:
        folder = Path(tmp)
        (folder/'fixture.cpp').write_text(fixture, encoding='utf-8')
        (folder/'build.bat').write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n', encoding='utf-8')
        subprocess.run(['cmd','/c','build.bat'],cwd=folder,check=True)


if __name__ == '__main__':
    main()
