"""Compile and execute the real viewmodel descriptor table and digit lookup."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest
from test_chapstats_pages import block

ROOT = Path(__file__).resolve().parents[1]
STUBS = r'''
#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#endif
using DWORD = unsigned long;
static constexpr int VK_SHIFT=16;
static bool fine=false;static int marks=0,sounds=0;
static DWORD testNow=1000;
static DWORD GetTickCount(){return testNow;}
static short GetAsyncKeyState(int){return fine ? (short)0x8000 : 0;}
static void MenuMarkDirty(){++marks;}
static void EngClientCmd(const char*){++sounds;}
static bool g_vmMirrorEnabled=false,g_vmMirrorPerModel=false,g_vmEnabled=false,g_vmReady=true,g_vmWritable=true,g_vmIoBusy=false,g_vmDirty=false,g_vmSaveFailed=false,g_settingsDirty=false,g_vmHooked=false;
static DWORD g_vmSaveAt=0,g_settingsSaveAt=0;
static float g_vmMoveStep=.1f,g_vmAngleStep=1;
static int g_vmEditSlot=0,g_menuSub=0,g_menuPage=123;
static constexpr int kMenuPageViewModel=123;
static int g_menuPagePrev=-1,g_navMenuSel=0,g_radarMenuSel=0;
static int MenuPageSubCount(int page){return page==kMenuPageViewModel?3:1;}
static int NavMenuPageFirstSetting(int,int){return 0;}
static int RadarMenuSettingIndex(int,int,int){return 0;}
static void Log(const char*,...){}
static UiId g_vmStatus=UiId::VmStatusInitial;
static skviewmodel::Store g_vmStore{},g_vmIoStore{};
static std::atomic<int> g_vmJob{0};
static uint32_t g_vmSaveRequests=0;
'''
FIXTURES = r'''
static bool Near(float a,float b){return std::fabs(a-b)<.00001f;}
static bool Zero(skviewmodel::Vec v){return v.x==0&&v.y==0&&v.z==0;}
static void Setup(){
 g_vmStore={};g_vmIoStore={};g_vmBindings[0]={};g_vmBindings[1]={};
 std::strcpy(g_vmBindings[0].model,"models/a.mdl");std::strcpy(g_vmBindings[1].model,"models/a.mdl");
 g_vmBindings[0].checksum=1;g_vmBindings[1].checksum=2;
 g_vmBindings[0].profile=g_vmBindings[1].profile=-1;
 g_vmReady=g_vmWritable=true;g_vmDirty=g_vmIoBusy=g_vmSaveFailed=g_vmEnabled=g_vmMirrorEnabled=g_vmMirrorPerModel=g_settingsDirty=g_vmHooked=fine=false;
 g_vmEditSlot=g_menuSub=0;g_menuPage=123;g_vmMoveStep=.1f;g_vmAngleStep=1;g_vmSelected=ViewModelMenuAction::PosX;
 g_vmJob=0;marks=sounds=0;g_vmStatus=UiId::VmStatusInitial;
}
int main(int argc,char**argv) {
 if(argc!=2)return 90;const int test=atoi(argv[1]);Setup();
 if(test==0){
 const ViewModelMenuAction expected[]={ViewModelMenuAction::Enabled,ViewModelMenuAction::Mirror,ViewModelMenuAction::MirrorModel,ViewModelMenuAction::PosX,ViewModelMenuAction::PosY,ViewModelMenuAction::PosZ,ViewModelMenuAction::Pitch,ViewModelMenuAction::Yaw,ViewModelMenuAction::Roll,ViewModelMenuAction::MoveStep,ViewModelMenuAction::AngleStep,ViewModelMenuAction::PivotX,ViewModelMenuAction::PivotY,ViewModelMenuAction::PivotZ,ViewModelMenuAction::Scale,ViewModelMenuAction::Slot,ViewModelMenuAction::ResetPosition,ViewModelMenuAction::ResetRotation,ViewModelMenuAction::ResetAll,ViewModelMenuAction::Reload};
 if(kViewModelMenuPageCount!=3||sizeof(kViewModelMenuItems)/sizeof(kViewModelMenuItems[0])!=20)return 1;
 for(int page=0;page<2;++page)for(int digit=1;digit<=9;++digit){
  const auto* item=ViewModelMenuItemAt(page,digit);
  if(!item||item->action!=expected[page*9+digit-1]||!item->fallback||!*item->fallback)return 2;
  if(item->label>=UiId::Count)return 3;
 }

 if(std::wcscmp(kViewModelMenuItems[0].fallback,L"持枪视角调整")!=0)return 4;
 if(ViewModelMenuItemAt(2,1)->action!=ViewModelMenuAction::ResetAll)return 5;
 if(ViewModelMenuItemAt(2,2)->action!=ViewModelMenuAction::Reload)return 5;
 for(int digit=3;digit<=9;++digit)if(ViewModelMenuItemAt(2,digit))return 6;
 const int badPages[]={-1,3,99};for(int page:badPages)if(ViewModelMenuItemAt(page,1))return 7;
 const int badDigits[]={-1,0,10,99};for(int digit:badDigits)for(int page=0;page<3;++page)if(ViewModelMenuItemAt(page,digit))return 8;
 } else if(test==1){
 // Every selectable digit executes the descriptor's action on both pages.
 const int digits[]={4,5,6,7,8,9};
 for(int digit:digits){g_menuSub=0;g_vmSelected=ViewModelMenuAction::Enabled;ViewModelMenuKey(digit);if(g_vmSelected!=ViewModelMenuItemAt(0,digit)->action)return 10;}
 for(int digit=1;digit<=3;++digit){g_menuSub=1;ViewModelMenuKey(digit);if(g_vmSelected!=ViewModelMenuItemAt(1,digit)->action)return 11;}
 g_menuSub=0;ViewModelMenuKey(1);if(!g_vmEnabled||!g_settingsDirty)return 12;
 ViewModelMenuKey(1);if(g_vmEnabled)return 13;
 // Page 0 [9] really selects roll, editing while disabled still persists a profile.
 ViewModelMenuKey(9);ViewModelMenuAdjust(1);
 if(!Near(ViewModelMenuPose()->rotation.z,1)||!g_vmDirty||g_vmHooked||g_vmEnabled)return 14;
 if(sounds!=12)return 15;
 } else if(test==2){
 auto*p=ViewModelEditablePose();p->position={1,2,3};p->rotation={4,5,6};p->pivot={7,8,9};p->scale=1.5f;p->Rebuild();g_menuSub=1;
 ViewModelMenuKey(8);if(!Zero(p->position)||!Near(p->rotation.x,4)||!Near(p->pivot.x,7)||!Near(p->scale,1.5f))return 20;
 p->position={1,2,3};ViewModelMenuKey(9);if(!Zero(p->rotation)||!Near(p->position.x,1)||!Near(p->pivot.x,7)||!Near(p->scale,1.5f))return 21;
 p->rotation={4,5,6};g_menuSub=2;ViewModelMenuKey(1);if(!Zero(p->position)||!Zero(p->rotation)||!Zero(p->pivot)||!Near(p->scale,1.f)||p->active||!g_vmDirty)return 22;
 ViewModelRequestSave();if(!g_vmIoBusy||g_vmDirty||g_vmJob.load()!=2||g_vmIoStore.count!=1)return 23;
 g_vmIoBusy=false;g_menuSub=2;ViewModelMenuKey(2);if(g_vmReady)return 24;
 } else if(test==3){
 ViewModelMenuKey(4);ViewModelMenuAdjust(1);g_menuSub=1;ViewModelMenuKey(7);
 if(g_vmEditSlot!=1)return 30;
 g_menuSub=0;ViewModelMenuKey(9);ViewModelMenuAdjust(1);
 if(g_vmStore.count!=2||!Near(g_vmStore.records[0].pose.position.x,.1f)||!Near(g_vmStore.records[1].pose.rotation.z,1))return 31;
 g_menuSub=1;ViewModelMenuKey(7);if(g_vmEditSlot||!Near(ViewModelMenuPose()->position.x,.1f)||!Zero(ViewModelMenuPose()->rotation))return 32;
 g_vmBindings[1].checksum=1;g_vmBindings[1].profile=-1;g_vmEditSlot=1;ViewModelEditablePose();
 if(g_vmStore.count!=2||g_vmBindings[0].profile!=g_vmBindings[1].profile)return 33;
 } else if(test==4){
 // Unreadable profile file must remain unwritten; reload can recover after fixes.
 g_vmWritable=false;g_vmStatus=UiId::VmStatusLoadError;ViewModelMenuKey(4);ViewModelMenuAdjust(1);
 if(!g_vmDirty||g_vmStatus!=UiId::VmStatusLoadError)return 40;
 g_menuSub=1;ViewModelRequestSave();if(g_vmIoBusy||g_vmJob.load()!=0||!g_vmDirty)return 41;
 g_menuSub=2;ViewModelMenuKey(2);if(g_vmReady)return 42;
 Setup();ViewModelMenuKey(4);ViewModelMenuAdjust(1);g_menuSub=2;ViewModelMenuKey(2);
 if(!g_vmReady||g_vmStatus!=UiId::VmStatusBusy)return 43;
 g_vmDirty=false;g_vmIoBusy=true;ViewModelMenuKey(2);if(!g_vmReady)return 44;
 } else if(test==5){
 g_menuSub=1;ViewModelMenuKey(1);if(!Near(g_vmMoveStep,.1f))return 50;
 ViewModelMenuKey(1);if(!Near(g_vmMoveStep,.5f))return 51;
 g_vmMoveStep=5;ViewModelMenuKey(1);if(!Near(g_vmMoveStep,.001f))return 52;
 fine=true;ViewModelMenuAdjust(1);if(!Near(g_vmMoveStep,.002f))return 53;
 ViewModelMenuAdjust(-10000);if(!Near(g_vmMoveStep,.001f))return 54;
 g_vmSelected=ViewModelMenuAction::AngleStep;g_vmAngleStep=15;ViewModelMenuKey(2);if(!Near(g_vmAngleStep,.01f))return 55;
 ViewModelMenuAdjust(1);if(!Near(g_vmAngleStep,.02f))return 56;
 ViewModelMenuAdjust(100000);if(!Near(g_vmAngleStep,15))return 57;
 g_vmSelected=ViewModelMenuAction::Roll;g_vmAngleStep=1;auto*p=ViewModelEditablePose();p->rotation.z=179.95f;ViewModelMenuAdjust(1);if(!Near(p->rotation.z,180))return 58;
 p->rotation.z=-179.95f;ViewModelMenuAdjust(-1);if(!Near(p->rotation.z,-180))return 59;
 g_vmSelected=ViewModelMenuAction::PivotY;p->pivot.y=99.99f;g_vmMoveStep=1;ViewModelMenuAdjust(1);if(!Near(p->pivot.y,100))return 60;
 } else if(test==6){
 ViewModelMenuKey(4);ViewModelMenuAdjust(1);ViewModelMenuKey(0);
 if(g_menuPage||g_menuSub||!g_vmIoBusy||g_vmDirty||g_vmJob.load()!=2)return 61;
 Setup();const auto before=g_vmSelected;ViewModelMenuKey(10);ViewModelMenuKey(-1);
 if(g_vmSelected!=before||sounds||marks)return 62;
 g_vmReady=false;ViewModelMenuAdjust(1);if(g_vmStore.count)return 63;
 } else if(test==7){
 g_vmSelected=ViewModelMenuAction::Pitch;MenuSubTurn(1);
 if(g_menuSub!=1||g_vmSelected!=ViewModelMenuAction::PivotX)return 70;
 ViewModelMenuAdjust(1);if(!Near(ViewModelMenuPose()->pivot.x,.1f)||!Zero(ViewModelMenuPose()->rotation))return 71;
 MenuSubTurn(-1);if(g_menuSub||g_vmSelected!=ViewModelMenuAction::PosX)return 72;
 g_vmSelected=ViewModelMenuAction::AngleStep;g_menuPagePrev=-1;MenuSubSync();
 if(g_menuSub||g_vmSelected!=ViewModelMenuAction::PosX)return 73;
 } else if(test==8){
 testNow=1000;ViewModelMenuKey(4);ViewModelMenuAdjust(1);if(g_vmSaveAt!=1750)return 80;
 testNow=1500;ViewModelMenuAdjust(1);if(g_vmSaveAt!=1750)return 81;
 ViewModelRequestSave();testNow=1600;ViewModelMenuAdjust(1);if(g_vmSaveAt!=2350||!g_vmDirty)return 82;
 } else if(test==9){
 g_menuSub=0;ViewModelMenuKey(2);
 if(!g_vmMirrorEnabled||g_vmMirrorPerModel||!g_vmEnabled||!g_settingsDirty)return 90;
 ViewModelMenuKey(2);if(g_vmMirrorEnabled||g_vmMirrorPerModel)return 91;
 } else if(test==10){
 g_menuSub=0;ViewModelMenuKey(3);auto* pose=ViewModelMenuPose();
 if(!g_vmMirrorPerModel||!pose||!pose->mirror||!g_vmEnabled||!g_vmDirty||!g_settingsDirty)return 100;
 const bool global=g_vmMirrorEnabled;ViewModelMenuKey(3);
 if(!g_vmMirrorPerModel||pose->mirror||g_vmMirrorEnabled!=global)return 101;
 ViewModelMenuKey(2);if(g_vmMirrorPerModel||!g_vmMirrorEnabled)return 102;
 } else if(test==11){
 g_vmReady=false;g_menuSub=0;ViewModelMenuKey(3);
 if(g_vmMirrorPerModel||g_vmDirty||g_settingsDirty)return 110;
 g_vmReady=true;g_menuSub=2;ViewModelMenuKey(2);
 if(g_vmReady||g_vmMirrorPerModel)return 111;
 g_vmSelected=ViewModelMenuAction::Reload;MenuSubTurn(-1);
 if(g_menuSub!=1||g_vmSelected!=ViewModelMenuAction::PivotX)return 112;
 } else if(test==12){
 g_vmMirrorEnabled=true;g_vmMirrorPerModel=false;
 if(!ViewModelGlobalMirrorActive())return 120;
 g_vmMirrorPerModel=true;
 if(ViewModelGlobalMirrorActive())return 121;
 g_vmMirrorEnabled=false;g_vmMirrorPerModel=false;
 if(ViewModelGlobalMirrorActive())return 122;
 } else if(test==13){
 std::strcpy(g_vmBindings[0].model,"models/v_models/v_smg_ncl1.mdl");
 std::strcpy(g_vmBindings[1].model,"models/v_models/v_smg_ncl2.mdl");g_vmBindings[1].checksum=1;
 auto* p=ViewModelEditablePose();if(!p||g_vmBindings[0].profile!=g_vmBindings[1].profile)return 130;
 p->position.x=3;g_vmEditSlot=1;
 if(ViewModelMenuPose()->position.x!=3||g_vmStore.count!=1)return 131;
 } else return 91;
 return 0;
}
'''

class ViewmodelMenu(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        chunks = ['#include <cmath>\n#include <cstring>\n#include <cwchar>\n#include <cstdio>\n#include <cstdlib>\n#include <cstdint>\n#include <atomic>\n#include "skeeto_ui_text.h"']
        chunks.append(block(source, r"^namespace skviewmodel\s*\{"))
        chunks.append(STUBS)
        chunks.append(block(source, r"^struct ViewModelBinding\s*\{")+';\nstatic ViewModelBinding g_vmBindings[2]{};')
        chunks.append(block(source, r"^enum class ViewModelMenuAction\s*\{") + ';')
        chunks.append(block(source, r"^struct ViewModelMenuItem\s*\{") + ';')
        chunks.append(block(source, r"^static constexpr ViewModelMenuItem kViewModelMenuItems\[\]\s*=\s*\{") + ';')
        chunks.append(re.search(r"static constexpr int kViewModelMenuPageCount = [^;]+;", source).group())
        chunks.append(block(source, r"^static const ViewModelMenuItem\* ViewModelMenuItemAt\(int sub, int digit\)\s*\{"))
        chunks.append('static ViewModelMenuAction g_vmSelected=ViewModelMenuAction::PosX;')
        for name in ['ViewModelRequestSave','ViewModelMenuPose','ViewModelEditablePose','ViewModelAxis','ViewModelEdited','ViewModelStepsChanged','ViewModelGlobalMirrorActive','ViewModelMirrorSetGlobal','ViewModelMenuAdjust','ViewModelMenuKey','ViewModelMenuPageSelection','MenuSubSync','MenuSubTurn']:
            chunks.append(block(source,rf"^static [^;\n]*\b{name}\([^;]*?\)\s*\{{"))
        chunks.append(FIXTURES)
        cls.temp=tempfile.TemporaryDirectory(prefix="skeeto-viewmodel-menu-")
        folder=cls.temp.name
        if folder:
            out=Path(folder);cpp=out/"test.cpp";cpp.write_text('\n'.join(chunks),encoding="utf-8")
            vswhere=Path(os.environ["ProgramFiles(x86)"])/"Microsoft Visual Studio/Installer/vswhere.exe"
            vs=subprocess.check_output([str(vswhere),"-latest","-products","*","-requires","Microsoft.VisualStudio.Component.VC.Tools.x86.x64","-property","installationPath"],text=True).strip()
            exe=out/"test.exe";bat=out/"compile.bat"
            bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /W4 /I"{ROOT / "src/skeeto/core"}" /Fe"{exe}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n',encoding="utf-8")
            result=subprocess.run(["cmd","/c",str(bat)],capture_output=True,text=True,errors="replace")
            if result.returncode:raise RuntimeError(result.stdout+result.stderr)
            cls.exe=exe

    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()

    def run_case(self,case):
        result=subprocess.run([str(self.exe),str(case)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_real_descriptor_pages_and_input_bounds(self):self.run_case(0)
    def test_every_numeric_selection_and_disabled_roll_edit(self):self.run_case(1)
    def test_page_two_resets_save_and_reload_execute(self):self.run_case(2)
    def test_model_slots_keep_profiles_separate(self):self.run_case(3)
    def test_readonly_error_can_reload_without_writing(self):self.run_case(4)
    def test_steps_cycles_fine_edit_and_limits(self):self.run_case(5)
    def test_return_saves_and_invalid_input_is_inert(self):self.run_case(6)
    def test_page_turn_selects_a_visible_axis(self):self.run_case(7)
    def test_continuous_edits_do_not_postpone_auto_save(self):self.run_case(8)
    def test_global_mirror_toggle_selects_global_mode(self):self.run_case(9)
    def test_model_mirror_toggle_selects_per_model_mode(self):self.run_case(10)
    def test_mirror_modes_require_loaded_current_profile_and_menu_page_selects_action(self):self.run_case(11)
    def test_global_mirror_status_ignores_remembered_value_in_per_model_mode(self):self.run_case(12)
    def test_generated_names_share_current_model_edit(self):self.run_case(13)

if __name__ == "__main__":unittest.main()
