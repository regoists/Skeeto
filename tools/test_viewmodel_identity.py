"""Exercise production model binding and profile identity without starting L4D2."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

from test_chapstats_pages import block
from test_viewmodel_hook import function
from test_viewmodel_io import implementation

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = r'''
static skviewmodel::Store g_vmStore{};
static ViewModelBinding g_vmBindings[2]{};
static unsigned g_vmBindingChanges=0;
static int g_offModelIndex=0x80, nameCalls=0, provider=0;
static bool failName=false, failId=false;
static const char* modelName="models/v_models/v_smg_ncl1.mdl";
static const void* __fastcall GetModel(void*,void*,int){return &provider;}
static const char* __fastcall GetName(void*,void*,const void*){++nameCalls;return failName?nullptr:modelName;}
static void* GetIface(const char*,const char*){return &provider;}
static void* VGet(void*,int slot){return slot==1?(void*)&GetModel:(void*)&GetName;}
static bool IsExec(void* p){return p!=nullptr;}
template<class T> static bool EntRead(void* e,int off,T* out){std::memcpy(out,(char*)e+off,sizeof(T));return true;}
static bool EntReadBlock(void* e,int off,void* out,size_t n){if(failId)return false;std::memcpy(out,(char*)e+off,n);return true;}
namespace skmem {static bool CopyCStr(char* out,size_t cap,const char* in){if(!in||strlen(in)>=cap)return false;strcpy(out,in);return true;}}
'''
CASES = r'''
#define CHECK(x) do {if(!(x)){printf("line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static unsigned char entity[0xa00]{};
static struct Header {uint32_t id,version,checksum;} hdr={0x54534449,49,0xc84d2604};
static Header* hdrPtr=&hdr;
static skviewmodel::Store store{},parsed{};
static char text[262145];
int main(int argc,char**argv){
 CHECK(argc==2);int test=atoi(argv[1]);short idx=7;void* p=&hdrPtr;
 memcpy(entity+g_offModelIndex,&idx,sizeof idx);memcpy(entity+0x934,&p,sizeof p);
 for(auto& b:g_vmBindings)b.profile=-1;
 if(test==0){
  g_vmStore.Ensure("models/v_models/v_smg.mdl",hdr.checksum);
  failName=true;ViewModelBind(0,entity);CHECK(!g_vmBindings[0].model[0]);
  failName=false;ViewModelBind(0,entity);
  CHECK(g_vmBindings[0].model[0]&&g_vmBindings[0].profile==0&&nameCalls==2);
  ViewModelBind(0,entity);CHECK(nameCalls==2);
 }else if(test==1){
  g_vmStore.Ensure("models/v_models/v_smg.mdl",hdr.checksum);
  failId=true;ViewModelBind(0,entity);failId=false;ViewModelBind(0,entity);
  CHECK(g_vmBindings[0].model[0]&&g_vmBindings[0].profile==0&&nameCalls==1);
 }else if(test==2){
  const char* old="# Skeeto viewmodel profiles v2\n[models/v_models/v_smg_ncl1.mdl|c84d2604]\nposition=2.2 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=1\n";
  CHECK(skviewmodel::Parse(old,strlen(old),&store));
  CHECK(store.Find("models/v_models/v_smg_ncl2.mdl",0xc84d2604)==0);
  CHECK(store.Find("models/v_models/v_smg.mdl",0xc84d2604)==0);
  CHECK(store.Find("models/v_models/v_smg_ncl2.mdl",0x12345678)==-1);
  CHECK(store.Find("models/v_models/v_rifle_ncl1.mdl",0xc84d2604)==-1);
  int second=store.Ensure("models/v_models/v_smg_ncl9.mdl",0x12345678);
  CHECK(second==1&&store.count==2&&store.records[0].pose.mirror);
  CHECK(!strcmp(store.records[second].model,"models/v_models/v_smg.mdl"));
 }else if(test==3){
  const char* old="# Skeeto viewmodel profiles v1\n[models/v_models/v_smg_ncl1.mdl|c84d2604]\nposition=2.20000005 0 0\nrotation=0 0 0\npivot=0 0 0\n";
  store.Ensure("models/v_models/v_smg.mdl",1);store.records[0].pose.position.x=3;
  const auto before=store;
  CHECK(!skviewmodel::Parse(old,strlen(old),&store)&&!memcmp(&store,&before,sizeof store));
 }else if(test==4){
  const char* old="# Skeeto viewmodel profiles v2\n[models/v_models/v_smg_ncl1.mdl|c84d2604]\nposition=1 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n[models/v_models/v_smg_ncl2.mdl|c84d2604]\nposition=2 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=1\n";
  store.Ensure("models/v_models/v_smg.mdl",1);store.records[0].pose.position.x=3;
  CHECK(skviewmodel::Parse(old,strlen(old),&store)&&store.count==1);
  CHECK(store.Find("models/v_models/v_smg_ncl7.mdl",0xc84d2604)==0);
  CHECK(store.records[0].pose.position.x==2&&store.records[0].pose.mirror);
 }else if(test==5){
  int first=store.Ensure("models/v_models/v_smg_ncl10.mdl",1);
  CHECK(first==0&&store.Find("models/v_models/v_smg_ncl99.mdl",1)==first);
  const char* distinct[]={"models/v_models/v_smg_ncl0.mdl","models/v_models/v_smg_ncl01.mdl","models/v_models/v_smg_nclx.mdl","models/v_models/v_smg_ncl1_extra.mdl","models/other/v_smg_ncl2.mdl"};
  for(const char* name:distinct)CHECK(store.Find(name,1)==-1);
  int nested=store.Ensure("models/v_models/v_rifle_ncl1_ncl2.mdl",2);
  CHECK(!strcmp(store.records[nested].model,"models/v_models/v_rifle.mdl"));
  int literal=store.Ensure("models/v_models/_ncl1.mdl",3);
  CHECK(!strcmp(store.records[literal].model,"models/v_models/_ncl1.mdl"));
  CHECK(skviewmodel::Format(store,text,sizeof text)>0);
 }else if(test==6){
  const char* old="# Skeeto viewmodel profiles v2\n[models/v_models/v_smg_ncl1.mdl|c84d2604]\nposition=1 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n[models/v_models/v_smg_ncl2.mdl|c84d2604]\nposition=1 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n";
  CHECK(skviewmodel::Parse(old,strlen(old),&store)&&store.count==1);
  char again[2048];size_t n=skviewmodel::Format(store,again,sizeof again);
  CHECK(n&&skviewmodel::Parse(again,n,&parsed)&&parsed.count==1);
 }else if(test==7){
  g_vmStore.Ensure("models/v_models/v_smg_ncl1.mdl",hdr.checksum);
  g_vmStore.records[0].pose.position.x=3;
  ViewModelBind(0,entity);CHECK(g_vmBindings[0].profile==0&&nameCalls==1);
  modelName="models/v_models/v_smg_ncl2.mdl";idx=8;memcpy(entity+g_offModelIndex,&idx,sizeof idx);
  ViewModelBind(0,entity);CHECK(g_vmBindings[0].profile==0&&nameCalls==2);
  hdr.checksum=0x12345678;idx=9;memcpy(entity+g_offModelIndex,&idx,sizeof idx);
  ViewModelBind(0,entity);CHECK(g_vmBindings[0].profile==-1&&nameCalls==3);
  CHECK(g_vmStore.records[0].pose.position.x==3);
 }else if(test==8){
  const char* old="# Skeeto viewmodel profiles v2\n[models/v_models/v_sniper_military_ncl1.mdl|0970ac3a]\nposition=1 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n[models/v_models/v_sniper_military_ncl2.mdl|fdf36965]\nposition=2 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=1\n";
  CHECK(skviewmodel::Parse(old,strlen(old),&store));
  CHECK(store.Find("models/v_models/v_sniper_military_ncl2.mdl",0x0970ac3a)==0);
  CHECK(store.Find("models/v_models/v_sniper_military_ncl1.mdl",0xfdf36965)==1);
  CHECK(store.Ensure("models/v_models/v_sniper_military_ncl2.mdl",0x0970ac3a)==0&&store.count==2);
  size_t n=skviewmodel::Format(store,text,sizeof text);
  CHECK(n&&skviewmodel::Parse(text,n,&parsed)&&parsed.count==2);
  CHECK(parsed.Find("models/v_models/v_sniper_military_ncl9.mdl",0x0970ac3a)==0);
  CHECK(parsed.Find("models/v_models/v_sniper_military_ncl8.mdl",0xfdf36965)==1);
  CHECK(parsed.records[0].pose.position.x==1&&parsed.records[1].pose.position.x==2&&parsed.records[1].pose.mirror);
 }else if(test==9){
  for(int i=0;i<200;++i){
   char path[260];sprintf(path,"models/v_models/v_weapon_%d_ncl%d.mdl",i%8,i%99+1);
   int at=store.Ensure(path,0x10000u+(unsigned)i);CHECK(at==i);
   auto& p=store.records[at].pose;p.position={float(i%31)-15,float(i%23),float(i%17)-8};
   p.rotation={float(i%121)-60,float(i%91)-45,float(i%71)-35};
   p.pivot={float(i%13),float(i%11)-5,float(i%7)};p.mirror=i%3==0;p.Rebuild();
  }
  for(int round=0;round<8;++round){
   size_t n=skviewmodel::Format(store,text,sizeof text);
   CHECK(n&&skviewmodel::Parse(text,n,&parsed)&&parsed.count==200);
   for(int i=199;i>=0;--i){
    char path[260];sprintf(path,"models/v_models/v_weapon_%d_ncl%d.mdl",i%8,(i*37+round*13)%99+1);
    int at=parsed.Find(path,0x10000u+(unsigned)i);CHECK(at==i);
    const auto& a=store.records[i].pose;const auto& b=parsed.records[at].pose;
    CHECK(!memcmp(&a.position,&b.position,sizeof(a.position))&&!memcmp(&a.rotation,&b.rotation,sizeof(a.rotation)));
    CHECK(!memcmp(&a.pivot,&b.pivot,sizeof(a.pivot))&&a.mirror==b.mirror);
    CHECK(parsed.Ensure(path,0x10000u+(unsigned)i)==i&&parsed.count==200);
   }
   store=parsed;
  }
 }else return 2;
 return 0;
}
'''


class ViewmodelIdentity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        src = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        cls.tmp = tempfile.TemporaryDirectory(prefix="skeeto-vm-identity-")
        folder = Path(cls.tmp.name)
        code = "#define _CRT_SECURE_NO_WARNINGS\n#define WIN32_LEAN_AND_MEAN\n#include <Windows.h>\n#include <cmath>\n#include <cstring>\n#include <cstdint>\n#include <cstdlib>\n#include <cstdio>\n"
        code += implementation("skviewmodel", "")
        code += block(src, r"^struct ViewModelBinding\s*\{") + ";\n"
        code += FIXTURE + function(src, "static void ViewModelBind(int slot, void* entity) {") + CASES
        (folder / "fixture.cpp").write_text(code, encoding="utf-8")
        vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
        vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
        bat = folder / "compile.bat"
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /W4 fixture.cpp /Fe:fixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        result = subprocess.run(["cmd", "/c", str(bat)], cwd=folder, capture_output=True, text=True, errors="replace")
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        cls.exe = folder / "fixture.exe"

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_case(self, case):
        result = subprocess.run([str(self.exe), str(case)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_failed_name_read_can_rebind_same_model(self): self.run_case(0)
    def test_failed_header_read_can_rebind_same_model(self): self.run_case(1)
    def test_generated_number_is_not_persistent_identity(self): self.run_case(2)
    def test_unsupported_v1_leaves_current_store_unchanged(self): self.run_case(3)
    def test_old_numbered_duplicates_load_latest_instead_of_blocking_file(self): self.run_case(4)
    def test_weapon_paths_and_non_generated_suffixes_stay_separate(self): self.run_case(5)
    def test_identical_old_numbered_duplicates_roundtrip_as_one_profile(self): self.run_case(6)
    def test_actual_bind_uses_stable_identity_and_keeps_other_mods_separate(self): self.run_case(7)
    def test_two_real_cache_models_keep_values_after_numbers_swap_and_reload(self): self.run_case(8)
    def test_new_profiles_survive_repeated_number_permutations_and_roundtrips(self): self.run_case(9)


if __name__ == "__main__":
    unittest.main()
