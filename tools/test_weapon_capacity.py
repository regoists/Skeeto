"""Compile production capacity sampling against mutable weapon-data fixtures."""
from pathlib import Path
import os
import subprocess
import tempfile

def main():
    source=(Path(__file__).resolve().parents[1]/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    fixture=r'''#include <Windows.h>
#include <cstring>
#include <cstdint>
#include <atomic>
#include <cstdio>
#include <cstdlib>
struct Info{char pad[0x160]{};int capacity=0;};
struct Weapon{Info* info;uint8_t dual=0;};
static int g_offHasDualWeapons=offsetof(Weapon,dual);
static constexpr int TW_Pistol=1;
static std::atomic_bool g_listenHost{true};
static int g_lookupCalls=0;
static Info* g_info=nullptr;
static void* __cdecl NativeInfo(unsigned short){++g_lookupCalls;return g_info;}
using WeaponInfoFn=void*(__cdecl*)(unsigned short);
static WeaponInfoFn g_weaponInfoFn=&NativeInfo;
static void Log(const char*){}
template<class T>bool EntRead(void* p,int off,T* out){if(!p)return false;if(off==0x9da){*out=T{};return true;}memcpy(out,(char*)p+off,sizeof(T));return true;}
static int TeamHudGunMaxClipKind(int kind){return kind==TW_Pistol?15:50;}
static void require(bool b,const char* msg){if(!b){puts(msg);exit(1);}}
// PRODUCTION_COLOR
// PRODUCTION_SAMPLER
int main(){
 Info data{};Weapon weapon{&data};g_info=&data;data.capacity=80;
 require(WeaponClipCapacity(&weapon,2)==80,"script capacity overrides hardcoded rifle 50");
 data.capacity=37;require(WeaponClipCapacity(&weapon,2)==37,"capacity changes are read without stale cache");
 data.capacity=25;require(WeaponClipCapacity(&weapon,TW_Pistol)==25,"single pistol capacity over 16 is not mistaken for dual pistols");
 weapon.dual=1;require(WeaponClipCapacity(&weapon,TW_Pistol)==50,"dual pistols scale actual script capacity");
 weapon.dual=0;data.capacity=-1;require(WeaponClipCapacity(&weapon,TW_Pistol)==15,"invalid script capacity preserves baseline fallback");
 data.capacity=1000000;require(WeaponClipCapacity(&weapon,2)==50,"bounded external capacity");
 const COLORREF healthy=RGB(140,190,220);
 data.capacity=25;weapon.dual=0;
 require(AmmoHudClipColor(5,WeaponClipCapacity(&weapon,TW_Pistol),healthy)==RGB(255,196,72),"25-round pistol yellow at 5");
 require(AmmoHudClipColor(3,WeaponClipCapacity(&weapon,TW_Pistol),healthy)==RGB(255,72,72),"25-round pistol red at 3");
 weapon.dual=1;
 require(AmmoHudClipColor(10,WeaponClipCapacity(&weapon,TW_Pistol),healthy)==RGB(255,196,72),"50-round dual yellow at 10");
 require(AmmoHudClipColor(11,WeaponClipCapacity(&weapon,TW_Pistol),healthy)==healthy,"healthy secondary retains blue");
 require(AmmoHudClipColor(2,8,healthy)==RGB(255,196,72),"magnum uses capacity percentage");
 require(AmmoHudClipColor(0,8,healthy)==RGB(255,72,72),"empty secondary red");
 g_listenHost.store(false);const int calls=g_lookupCalls;
 data.capacity=80;weapon.dual=0;
 require(WeaponClipCapacity(&weapon,2)==50,"remote primary uses original hardcoded table");
 data.capacity=25;
 require(WeaponClipCapacity(&weapon,TW_Pistol)==15,"remote pistol ignores local script capacity");
 weapon.dual=1;
 require(WeaponClipCapacity(&weapon,TW_Pistol)==30,"remote dual pistols use original table times two");
 require(g_lookupCalls==calls,"remote path does not call native script lookup");
 g_listenHost.store(true);weapon.dual=0;
 require(WeaponClipCapacity(&weapon,TW_Pistol)==25,"return to local host uses script without stale capacity cache");
 g_weaponInfoFn=nullptr;require(WeaponClipCapacity(&weapon,2)==50,"missing ABI keeps baseline");
 puts("PASS weapon capacity: actual script values, live updates, single/dual pistols, local/remote selection and fallback");
}'''
    hud=(Path(__file__).resolve().parents[1]/'src/skeeto/core/skeeto_hud.cpp').read_text(encoding='utf-8-sig')
    color=hud.split('COLORREF AmmoHudClipColor(int clip, int maxClip, COLORREF healthy) {',1)[1].split('\nbool TeamHudIsSide',1)[0]
    fixture=fixture.replace('// PRODUCTION_COLOR','COLORREF AmmoHudClipColor(int clip, int maxClip, COLORREF healthy) {'+color)
    fixture=fixture.replace('// PRODUCTION_SAMPLER',source.split('// Weapon clip capacity sampling begin',1)[1].split('// Weapon clip capacity sampling end',1)[0])
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-campaign-outline-') as tmp:
        folder = Path(tmp)
        (folder/'fixture.cpp').write_text(fixture, encoding='utf-8')
        (folder/'build.bat').write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n', encoding='utf-8')
        subprocess.run(['cmd','/c','build.bat'],cwd=folder,check=True)


if __name__ == '__main__':
    main()
