"""Execute the production viewmodel pose math/store without engine or rendering APIs."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
from test_chapstats_pages import block

ROOT = Path(__file__).resolve().parents[1]
FIXTURES = r'''
using namespace skviewmodel;
static bool Near(float a,float b) { return std::fabs(a-b)<0.0001f; }
static bool NearVec(Vec a,Vec b) { return Near(a.x,b.x)&&Near(a.y,b.y)&&Near(a.z,b.z); }
static Vec Mul(Mat a,Vec v) { return {a.m[0][0]*v.x+a.m[0][1]*v.y+a.m[0][2]*v.z,a.m[1][0]*v.x+a.m[1][1]*v.y+a.m[1][2]*v.z,a.m[2][0]*v.x+a.m[2][1]*v.y+a.m[2][2]*v.z}; }
static Mat MM(Mat a,Mat b) { Mat r{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)for(int k=0;k<3;++k)r.m[i][j]+=a.m[i][k]*b.m[k][j];return r; }
static Mat T(Mat a) { Mat r{};for(int i=0;i<3;++i)for(int j=0;j<3;++j)r.m[i][j]=a.m[j][i];return r; }
static bool NearMat(Mat a,Mat b) { for(int i=0;i<3;++i)for(int j=0;j<3;++j)if(!Near(a.m[i][j],b.m[i][j]))return false;return true; }
static Vec Add(Vec a,Vec b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static bool Fail(const char* s) { printf("FAIL: %s\n",s);return false; }
static Store store, parsed, saved;
static char text[262145];
int main(int argc,char**argv) {
 if(argc!=2)return 2;int test=atoi(argv[1]);bool ok=true;
 if(test==0) {
  Pose p{};p.Rebuild();Vec o{-0.0f,1.234567f,-23.0f},a{0.0f,720.125f,-0.0f},oo{},aa{};
  Apply(p,{20,30,40},o,a,&oo,&aa);ok &= !memcmp(&o,&oo,sizeof o)&&!memcmp(&a,&aa,sizeof a);
  p.pivot={1,2,3};p.Rebuild();Apply(p,{20,30,40},o,a,&oo,&aa);ok &= !p.active&&!memcmp(&o,&oo,sizeof o)&&!memcmp(&a,&aa,sizeof a);
  p.position={2,3,4};p.Rebuild();Apply(p,{0,0,0},o,a,&oo,&aa);ok &= NearVec(oo,Add(o,{3,-2,4}))&&!memcmp(&a,&aa,sizeof a);
  Apply(p,{0,90,0},o,a,&oo,&aa);ok &= NearVec(oo,Add(o,{2,3,4}));
 } else if(test==1) {
  // Independent reference: local camera conjugation, including native animation rotation.
  const Vec cams[]={{0,0,0},{0,90,0},{45,73,21},{-65,-130,-37},{90,0,0},{-90,35,0}};
  for(Vec c:cams) {
   Pose p{};p.rotation={17,-26,33};p.Rebuild();Vec native{12,27,-9},o{1,2,3},oo{},aa{};
   Mat cam=AngleMatrix(c),d=MM(MM(cam,AngleMatrix(p.rotation)),T(cam));
   Apply(p,c,o,native,&oo,&aa);ok &= NearVec(o,oo)&&NearMat(AngleMatrix(aa),MM(d,AngleMatrix(native)));
  }
  for(Vec a:cams)ok &= NearMat(AngleMatrix(a),AngleMatrix(MatrixAngles(AngleMatrix(a))));
 } else if(test==2) {
  Pose p{};p.rotation={35,51,-17};p.pivot={3,2,-4};p.Rebuild();Vec camera{47,33,8},native{-20,17,29},o{6,-9,4},oo{},aa{};
  Vec local{p.pivot.y,-p.pivot.x,p.pivot.z};
  Vec before=Add(o,Mul(AngleMatrix(native),local));Apply(p,camera,o,native,&oo,&aa);
  ok &= NearVec(before,Add(oo,Mul(AngleMatrix(aa),local)));
  p.position={5,7,9};p.Rebuild();Apply(p,camera,o,native,&oo,&aa);
  ok &= NearVec(Add(before,Mul(AngleMatrix(camera),{7,-5,9})),Add(oo,Mul(AngleMatrix(aa),local)));
 } else if(test==3) {
  store={};int x=store.Ensure("models/weapons/v_rifle.mdl",1),y=store.Ensure("models/weapons/v_rifle.mdl",2);
  ok &= x==0&&y==1&&store.Find("models/weapons/v_rifle.mdl",1)==0&&store.Find("models/weapons/v_rifle.mdl",2)==1;
  char path[260]="models/";memset(path+7,'a',248);memcpy(path+255,".mdl",5);
  ok &= strlen(path)==259&&store.Ensure(path,3)==2;
  char normalized[260];ok &= NormalizeModel("MODELS\\Weapons\\V_RIFLE.MDL",normalized)&&!strcmp(normalized,"models/weapons/v_rifle.mdl");
  ok &= !NormalizeModel("models/../secret.mdl",normalized)&&!NormalizeModel("other/x.mdl",normalized)&&!NormalizeModel("models/a|b.mdl",normalized);
  char tooLong[261];memcpy(tooLong,path,255);tooLong[255]='a';memcpy(tooLong+256,".mdl",5);ok &= !NormalizeModel(tooLong,normalized);
 } else if(test==4) {
  store={};int i=store.Ensure("models/weapons/v_rifle.mdl",0xdeadbeef);store.records[i].pose.position={1.2345678f,-2,3};store.records[i].pose.rotation={17,-28,93};store.records[i].pose.pivot={4,5,-6};store.records[i].pose.mirror=true;store.records[i].pose.scale=1.25f;store.records[i].pose.Rebuild();
  size_t n=Format(store,text,sizeof text);ok &= n>0&&Parse(text,n,&parsed)&&parsed.count==1;
  ok &= !memcmp(&store.records[0].pose.position,&parsed.records[0].pose.position,sizeof(Vec))&&NearMat(store.records[0].pose.delta,parsed.records[0].pose.delta)&&parsed.records[0].pose.mirror&&Near(parsed.records[0].pose.scale,1.25f);
  const char* crlf="# comment\r\n# Skeeto viewmodel profiles v2\r\n\r\n[MODELS\\WEAPONS\\V_RIFLE.MDL|DEADBEEF]\r\nposition = 1 2 3\r\nrotation=4 5 6\r\npivot=7 8 9\r\nmirror=1\r\n";
  ok &= Parse(crlf,strlen(crlf),&parsed)&&parsed.Find("models/weapons/v_rifle.mdl",0xdeadbeef)==0&&Near(parsed.records[0].pose.scale,1.f);
  const char* withScale="# Skeeto viewmodel profiles v2\n[models/weapons/v_rifle.mdl|deadbeef]\nposition=0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\nscale=0.7\n";
  ok &= Parse(withScale,strlen(withScale),&parsed)&&Near(parsed.records[0].pose.scale,.7f);
 } else if(test==5) {
  store={};store.Ensure("models/a.mdl",42);saved=store;
  const char* bad[]={"[models/a.mdl|12345678]\nposition=0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|xyz]\n", "# Skeeto viewmodel profiles v2\n[models/../a.mdl|12345678]\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=nan 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=0 0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=101 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=0 0 0\nrotation=181 0 0\npivot=0 0 0\nmirror=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=0 0 0\nrotation=0 0 0\npivot=inf 0 0\nmirror=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\nunknown=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=0 0 0\nposition=0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n", "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=0 0 0\n" , "# Skeeto viewmodel profiles v2\n[models/a.mdl|12345678]\nposition=0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=2\n", "# Skeeto viewmodel profiles v1\n[models/a.mdl|12345678]\nposition=0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n"};
  for(const char* s:bad)ok &= !Parse(s,strlen(s),&store)&&!memcmp(&store,&saved,sizeof store);
  const char embedded[]="# Skeeto viewmodel profiles v2\n\0evil";ok &= !Parse(embedded,sizeof embedded-1,&store)&&!memcmp(&store,&saved,sizeof store);
  ok &= !Parse(text,262145,&store)&&!memcmp(&store,&saved,sizeof store);
 } else if(test==6) {
  store={};for(int i=0;i<256;++i)ok &= store.Ensure("models/a.mdl",(uint32_t)i)==i;
  ok &= store.Ensure("models/a.mdl",999)==-1&&store.count==256&&store.Ensure("models/a.mdl",255)==255;
  size_t n=Format(store,text,sizeof text);ok &= n&&Parse(text,n,&parsed)&&parsed.count==256;
  const char extra[]="[models/a.mdl|ffffffff]\nposition=0 0 0\nrotation=0 0 0\npivot=0 0 0\nmirror=0\n";
  memcpy(text+n,extra,sizeof extra);saved=parsed;ok &= !Parse(text,n+sizeof extra-1,&parsed)&&!memcmp(&parsed,&saved,sizeof parsed);
  char small[8]={'x'};ok &= Format(store,small,sizeof small)==0&&small[0]==0;
 } else return 2;
 return ok?0:(Fail("case failed")?0:1);
}
'''


class ViewmodelPose(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "src/skeeto/core/skeeto_proxy.cpp").read_text(encoding="utf-8-sig")
        production = block(source, r"^namespace skviewmodel\s*\{")
        cls.temp = tempfile.TemporaryDirectory(prefix="skeeto-viewmodel-")
        out = Path(cls.temp.name)
        cpp = out / "test.cpp"
        cpp.write_text("#include <cmath>\n#include <cstring>\n#include <cstdio>\n#include <cstdlib>\n#include <cstdint>\n#include <cstddef>\n" + production + FIXTURES, encoding="utf-8")
        vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
        vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
        cls.exe = out / "test.exe"
        bat = out / "compile.bat"
        bat.write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /std:c++17 /utf-8 /W4 /Fe"{cls.exe}" /Fo"{out / "test.obj"}" "{cpp}"\nexit /b %errorlevel%\n', encoding="utf-8")
        result = subprocess.run(["cmd", "/c", str(bat)], capture_output=True, text=True, errors="replace")
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)

    @classmethod
    def tearDownClass(cls): cls.temp.cleanup()

    def run_case(self, case):
        result = subprocess.run([str(self.exe), str(case)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_neutral_and_translation_preserve_native_bits(self): self.run_case(0)
    def test_rotation_conjugation_and_euler_singularities(self): self.run_case(1)
    def test_pivot_stays_fixed_with_native_pose(self): self.run_case(2)
    def test_model_identity_and_path_bounds(self): self.run_case(3)
    def test_store_roundtrip_and_crlf(self): self.run_case(4)
    def test_invalid_input_is_transactional(self): self.run_case(5)
    def test_capacity_and_small_format_buffer(self): self.run_case(6)


if __name__ == "__main__": unittest.main()
