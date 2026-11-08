"""Run the production addon filter/detour with native x86 fixtures."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

HARNESS = r'''
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <initializer_list>
#include <cstring>
#include <cstdio>
#include "skeeto_memread.h"
#include "skeeto_ui_text.h"
static bool g_optClientHideBackItems=false, features=true;
static std::atomic_bool g_run{true};
static DWORD g_paintTid=GetCurrentThreadId();
static int g_offTeam=16;
static constexpr int kClientAddonBitsOffset=0x1880;
static constexpr int kClientAddonWeaponsOffset=0x1884;
static uint32_t g_clientAddonCalls=0,g_clientAddonFiltered=0;
static std::atomic_uint32_t g_clientAddonWrongThread{0};
static bool g_clientAddonFailed=false;
static int g_offClientAddonBits=kClientAddonBitsOffset,g_offClientAddonWeapons=kClientAddonWeaponsOffset;
static uint8_t* g_clientAddonTarget=nullptr;
static uint8_t g_clientAddonPatch[6]{};
static bool g_clientAddonHooked=false;
static uint8_t* fixtureTarget=nullptr;static int scans=0,matches=1;
static int CountPat(const char*,const char*,uintptr_t* hits,int){++scans;hits[0]=(uintptr_t)fixtureTarget;return matches;}
static void Log(const char*,...){}
static bool SkeetoFeaturesOn(){return features;}
static unsigned seen=0,calls=0;static bool faultOriginal=false;
static void __fastcall Original(void* self,void*){
 ++calls;seen=*(unsigned*)((uint8_t*)self+kClientAddonBitsOffset);
 if(faultOriginal)RaiseException(0xe0000001,0,0,nullptr);
}
using ClientAddonUpdateFn=void(__thiscall*)(void*);
static ClientAddonUpdateFn g_origClientAddonUpdate=(ClientAddonUpdateFn)Original;
'''

FIXTURE = r'''
int main(){
 alignas(4) uint8_t player[0x1b00]{};
 *(int*)(player+g_offTeam)=2;
 auto& bits=*(unsigned*)(player+kClientAddonBitsOffset);
 auto* ids=(int*)(player+kClientAddonWeaponsOffset);
 for(int i=0;i<3;++i)g_optClientBackItemHidden[i]=true;
 // Place each supported item in every script-driven slot. Unknown IDs,
 // guns, melee, tongue, carried props and unrelated bits must survive.
 for(int id:{12,13,14,15,23,24,25})for(int slot=0;slot<5;++slot){
  for(int i=0;i<5;++i)ids[i]=1;ids[slot]=id;
  if(ClientUxFilterAddonBits(0x1ff,ids)!=(0x1ffu&~((1u<<slot)|(1u<<6))))return 1;
 }
 for(int id:{0,1,19,20,26,32,55,64+19}){
  for(int i=0;i<5;++i)ids[i]=id;
  if(ClientUxFilterAddonBits(0x1ff,ids)!=(0x1ffu&~(1u<<6)))return 2;
 }
 ids[0]=1;ids[1]=19;ids[2]=13;ids[3]=12;ids[4]=15;
 bits=0xff;Hooked_ClientUxUpdateAddons(player,nullptr);
 if(seen!=0xff||bits!=0xff)return 3;
 g_optClientHideBackItems=true;
 Hooked_ClientUxUpdateAddons(player,nullptr);
 if(seen!=0xa3||bits!=0xff)return 4;
 g_optClientHideBackItems=false;Hooked_ClientUxUpdateAddons(player,nullptr);
 if(seen!=0xff||bits!=0xff)return 5;
 g_optClientHideBackItems=true;features=false;Hooked_ClientUxUpdateAddons(player,nullptr);
 if(seen!=0xff||bits!=0xff)return 6;
 features=true;*(int*)(player+g_offTeam)=3;Hooked_ClientUxUpdateAddons(player,nullptr);
 if(seen!=0xff)return 7;
 *(int*)(player+g_offTeam)=2;g_paintTid=0;Hooked_ClientUxUpdateAddons(player,nullptr);
 if(seen!=0xff||bits!=0xff)return 8;
 g_paintTid=GetCurrentThreadId();*(int*)(player+g_offTeam)=3;
 g_optClientBackItemHidden[(int)ClientBackItemCategory::Tongue]=true;
 Hooked_ClientUxUpdateAddons(player,nullptr);if(seen!=0xdf||bits!=0xff)return 23;
 g_optClientBackItemHidden[(int)ClientBackItemCategory::Tongue]=false;
 *(int*)(player+g_offTeam)=2;
 g_paintTid=GetCurrentThreadId();faultOriginal=true;
 __try{Hooked_ClientUxUpdateAddons(player,nullptr);return 9;}
 __except(GetExceptionCode()==0xe0000001?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){}
 if(bits!=0xff)return 10;
 faultOriginal=false;g_run=false;Hooked_ClientUxUpdateAddons(player,nullptr);
 if(seen!=0xff)return 11;
 // Exercise actual production installation/removal against a private fixture
 // page. No game process or installed game bytes are touched by this test.
 g_run=true;g_optClientHideBackItems=false;
 ClientUxBackItemsPump();if(scans)return 12;
 fixtureTarget=(uint8_t*)VirtualAlloc(nullptr,0x1000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
 const uint8_t head[]={0x55,0x8b,0xec,0x83,0xec,0x10};
 const uint8_t mask[]={0x8b,0x97,0x80,0x18,0,0},idsRead[]={0x8b,0x94,0x8f,0x84,0x18,0,0};
 memcpy(fixtureTarget,head,6);memcpy(fixtureTarget+0xfb,mask,6);memcpy(fixtureTarget+0x180,idsRead,7);
 g_optClientHideBackItems=true;ClientUxBackItemsPump();
 if(!g_clientAddonHooked||scans!=1||memcmp(fixtureTarget,g_clientAddonPatch,6))return 13;
 auto* trampoline=(uint8_t*)g_origClientAddonUpdate;
 if(memcmp(trampoline,head,6)||trampoline[6]!=0xe9)return 14;
 if((uintptr_t)(trampoline+11)+*(uint32_t*)(trampoline+7)!=(uintptr_t)(fixtureTarget+6))return 15;
 for(int i=0;i<10;++i)ClientUxBackItemsPump();if(scans!=1)return 16;
 g_optClientHideBackItems=false;ClientUxBackItemsPump();
 if(g_clientAddonHooked||memcmp(fixtureTarget,head,6))return 17;
 g_optClientHideBackItems=true;ClientUxBackItemsPump();
 if(!g_clientAddonHooked||scans!=1||(uint8_t*)g_origClientAddonUpdate!=trampoline)return 18;
 features=false;ClientUxBackItemsPump();
 if(g_clientAddonHooked||memcmp(fixtureTarget,head,6))return 19;
 features=true;ClientUxBackItemsPump();
 fixtureTarget[0]=0xcc;g_optClientHideBackItems=false;ClientUxBackItemsPump();
 if(!g_clientAddonFailed||fixtureTarget[0]!=0xcc)return 20;
 // An incompatible layout fails without patching or allocating another stub.
 g_clientAddonFailed=false;g_clientAddonHooked=false;g_clientAddonTarget=nullptr;
 memcpy(fixtureTarget,head,6);g_offClientAddonWeapons=-123;g_optClientHideBackItems=true;
 ClientUxBackItemsPump();if(memcmp(fixtureTarget,head,6))return 21;
 g_offClientAddonWeapons=0x1890;ClientUxBackItemsPump();
 if(!g_clientAddonFailed||memcmp(fixtureTarget,head,6))return 22;
 return 0;
}
'''

MENU_HARNESS = r'''
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "skeeto_ui_text.h"
static constexpr int kMenuItemsPerPage=9,kMenuPageSkeetoScript=31;
static constexpr int kMenuPageHoldSemiSetup=101,kMenuPageShotgunReload=102;
static constexpr int kMenuPageDissolveSetup=103,kMenuPageAcidOutlineSetup=104;
static constexpr int kMenuPageTongueCutHud=105,kMenuPageLastMusic=106;
static constexpr int kMenuPageTpsAim=107,kMenuPageRouteProgressHud=108;
static constexpr int kMenuPageBackItems=109;
static constexpr int kMenuPageViewRollSetup=139,kMenuPageParryFlash=126;
static constexpr int kMenuPageSupplyScan=140;
static int g_menuPage=31,g_menuSub=0,g_lastMusicAdjustSel=0,g_viewRollAdjSel=0;
static bool g_optClientLowHealthCue=false,g_optClientHealFade=false;
static bool g_optClientEmptyMagSound=false,g_optClientLowAmmoSound=false;
static bool g_optClientShellImpact=false,g_optClientClankReload=false;
static bool g_optLocalViewRoll=false,g_optClientHideBackItems=false;
static bool g_optIncapRoll=false,g_optLadderStep=false,g_optJockeyLeap=false;
static bool g_optScorch=false,g_optGunReverb=false,g_optClientDspLock=false;
static int g_optLocalViewRollAngle=0,saves=0,sounds=0;
static void SaveSettings(){++saves;}static void EngClientCmd(const char*){++sounds;}
static void Log(const char*,...){}static void MenuMarkDirty(){}
static void ClientUxClearScreenFadeKind(int){}static void ClientUxResetHealthCueArms(){}
static void ClientUxClearDelayedSfxKind(int){}static void ClientUxApplyViewRoll(bool){}
static void ClientUxApplyForceCvars(bool){}static void ParrySoundRefreshList(){}
'''

MENU_FIXTURE = r'''
int main(){
 // The parent entry must open settings without toggling or saving.
 g_menuSub=1;TestScriptDigit(7);
 if(g_menuPage!=kMenuPageBackItems||g_menuSub!=0||g_optClientHideBackItems||saves)return 1;
 TestBackItemDigit(1);if(!g_optClientHideBackItems||saves!=1)return 2;
 TestBackItemDigit(1);if(g_optClientHideBackItems||saves!=2)return 3;
 for(int digit=2;digit<=5;++digit){
  bool before=g_optClientBackItemHidden[digit-2];int written=saves;
  TestBackItemDigit(digit);
  if(g_optClientBackItemHidden[digit-2]==before||saves!=written+1)return 7;
  TestBackItemDigit(digit);if(g_optClientBackItemHidden[digit-2]!=before)return 8;
 }
 int written=saves;TestBackItemDigit(10);if(saves!=written)return 9;
 written=saves;for(int d=6;d<=9;++d)TestBackItemDigit(d);if(saves!=written)return 12;
 TestBackItemDigit(0);if(g_menuPage!=kMenuPageSkeetoScript||g_menuSub!=1)return 10;
 for(int index=0;index<SkeetoScriptMenuItemCount();++index){
  const auto& item=kSkeetoScriptMenuItems[index];
  g_menuPage=kMenuPageSkeetoScript;g_menuSub=index/9;
  bool* setting=nullptr;
  switch(item.action){
   case SkeetoScriptMenuAction::ToggleLowHealthCue:setting=&g_optClientLowHealthCue;break;
   case SkeetoScriptMenuAction::ToggleHealFade:setting=&g_optClientHealFade;break;
   case SkeetoScriptMenuAction::ToggleEmptyMagSound:setting=&g_optClientEmptyMagSound;break;
   case SkeetoScriptMenuAction::ToggleLowAmmoSound:setting=&g_optClientLowAmmoSound;break;
   case SkeetoScriptMenuAction::ToggleShellImpact:setting=&g_optClientShellImpact;break;
   case SkeetoScriptMenuAction::ToggleClankReload:setting=&g_optClientClankReload;break;
   case SkeetoScriptMenuAction::ToggleIncapRoll:setting=&g_optIncapRoll;break;
   case SkeetoScriptMenuAction::ToggleLadderStep:setting=&g_optLadderStep;break;
   case SkeetoScriptMenuAction::ToggleJockeyLeap:setting=&g_optJockeyLeap;break;
   case SkeetoScriptMenuAction::ToggleScorch:setting=&g_optScorch;break;
   case SkeetoScriptMenuAction::ToggleGunReverb:setting=&g_optGunReverb;break;
  }
  if(setting)*setting=false;
  const int before=saves;TestScriptDigit(index%9+1);
  if(setting){if(!*setting||saves!=before+1)return 3;}
  else if(g_menuPage!=item.targetPage)return 4;
 }
 g_menuPage=kMenuPageSkeetoScript;g_menuSub=2;
 int before=saves;TestScriptDigit(8);TestScriptDigit(9);
 if(saves!=before||g_menuPage!=kMenuPageSkeetoScript)return 5;
 TestScriptDigit(0);if(g_menuPage!=0||g_menuSub!=0)return 6;
 puts("Skeetoscript: all 23 actions executed; single-page addon settings, toggles and return verified");
 return 0;
}
'''

CATEGORY_FIXTURE = r'''
int main(){
 using C=ClientBackItemCategory;
 if(g_optClientHideBackItems)return 1;
 for(int i=0;i<(int)C::Count;++i)if(g_optClientBackItemHidden[i])return 2;
 const int ids[][20]={
  {15,23}, {12,24}, {13,14,25}, {}
 };
 for(int c=0;c<(int)C::Count;++c){
  for(int i=0;i<(int)C::Count;++i)g_optClientBackItemHidden[i]=false;
  g_optClientBackItemHidden[c]=true;
  for(int other=0;other<(int)C::Count;++other)for(int id:ids[other]){
   if(!id)continue;
   int weapons[]={id,id,id,id,id};
   unsigned expected=0x1ff;
   if(other==c)expected&=~0x1fu;
   if(c==(int)C::Medical)expected&=~(1u<<6);
   if(c==(int)C::Tongue)expected&=~(1u<<5);
   if(ClientUxFilterAddonBits(0x1ff,weapons)!=expected)return 3;
  }
 }
 for(int i=0;i<(int)C::Count;++i)g_optClientBackItemHidden[i]=true;
 for(int id:{0,1,19,20,22,30,31,26,32,37,16,27,64,83,55,128,255,-1}){
  int weapons[]={id,id,id,id,id};
  if(ClientUxFilterAddonBits(0x1ff,weapons)!=0x19fu)return 4;
 }
 if(!ClientUxLoadBackItemCategory("client_hide_back_medicine=0")||g_optClientBackItemHidden[(int)C::Medicine])return 5;
 if(!ClientUxLoadBackItemCategory("client_hide_back_medical=0")||g_optClientBackItemHidden[(int)C::Medical])return 6;
 if(!ClientUxLoadBackItemCategory("client_hide_back_medicine=1")||!g_optClientBackItemHidden[(int)C::Medicine])return 7;
 if(ClientUxLoadBackItemCategory("client_hide_back_medicine_extra=0"))return 8;
 if(ClientUxLoadBackItemCategory("client_hide_back_medicine=bad"))return 9;
 if(ClientUxLoadBackItemCategory("client_hide_back_primary=1"))return 10;
 return 0;
}
'''


class SurvivorAddons(unittest.TestCase):
    def settings_code(self, source):
        code = ''
        for pattern in (r'^enum class ClientBackItemCategory', r'^struct ClientBackItemMenuItem',
                r'^static constexpr ClientBackItemMenuItem kClientBackItemMenuItems',
                r'^static bool g_optClientBackItemHidden'):
            code += block(source, pattern) + ';\n'
        for pattern in (r'^static constexpr int ClientBackItemMenuItemCount',
                r'^static bool& ClientUxBackItemSetting', r'^static bool ClientUxLoadBackItemCategory'):
            code += block(source, pattern) + '\n'
        return code

    def test_independent_categories_and_settings(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        code = MENU_HARNESS + self.settings_code(source)
        code += block(source, r'^static int ClientUxBackItemCategory') + '\n'
        code += block(source, r'^static unsigned ClientUxFilterAddonBits') + '\n'
        self.run_fixture(code + CATEGORY_FIXTURE)

    def test_both_script_pages_execute_their_actions(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        code = MENU_HARNESS + self.settings_code(source)
        for pattern in (r'^enum class SkeetoScriptMenuAction', r'^struct SkeetoScriptMenuItem',
                r'^static constexpr SkeetoScriptMenuItem kSkeetoScriptMenuItems'):
            code += block(source, pattern) + ';\n'
        for pattern in (r'^static constexpr int SkeetoScriptMenuItemCount',
                r'^static const SkeetoScriptMenuItem\* SkeetoScriptMenuItemAt'):
            code += block(source, pattern) + '\n'
        # Execute the actual dispatch block, including every early return.
        code += 'static void TestScriptDigit(int digit){\n'
        code += block(source, r'^\tif \(g_menuPage == kMenuPageSkeetoScript\) \{') + '\n}\n'
        code += 'static void TestBackItemDigit(int digit){\n'
        code += block(source, r'^\tif \(g_menuPage == kMenuPageBackItems\) \{') + '\n}\n'
        self.run_fixture(code + MENU_FIXTURE)

    def test_filter_and_native_call_restoration(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        functions = self.settings_code(source)
        functions += block(source, r'^static int ClientUxBackItemCategory') + '\n'
        functions += block(source, r'^static unsigned ClientUxFilterAddonBits\(')
        functions += block(source, r'^static void __fastcall Hooked_ClientUxUpdateAddons\(')
        functions += block(source, r'^static void ClientUxBackItemsPump\(\) \{')
        self.run_fixture(HARNESS + functions + FIXTURE)

    def run_fixture(self, code):
        with tempfile.TemporaryDirectory(prefix='skeeto-addons-') as directory:
            folder = Path(directory)
            (folder / 'test.cpp').write_text(code, encoding='utf-8')
            vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
            include = ROOT / 'src/skeeto/core'
            (folder / 'run.bat').write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                f'cl /nologo /EHsc /std:c++17 /utf-8 /I"{include}" test.cpp /Fe:test.exe\n'
                'if errorlevel 1 exit /b 1\ntest.exe\nexit /b %errorlevel%\n', encoding='utf-8')
            result = subprocess.run(['cmd', '/c', str(folder / 'run.bat')], cwd=folder,
                capture_output=True, text=True, errors='replace')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
