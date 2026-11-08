"""Execute production native-handedness ownership and command publication, x86."""
from pathlib import Path
import os
import subprocess
import tempfile
from test_chapstats_pages import block
ROOT=Path(__file__).resolve().parents[1]
STUBS=r'''
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <initializer_list>
namespace skviewmodel { struct Vec {float x,y,z;}; struct Mat {float m[3][3];}; struct Pose {float scale=1.f;bool mirror;}; struct Record {Pose pose;}; struct Store {int count;Record records[4];}; static Mat AngleMatrix(Vec); static float ClampScale(float v){return v;} }
static bool g_vmMirrorEnabled=true,g_vmMirrorPerModel=false,g_vmMirrorActive=false,g_vmMirrorFailed=false,g_vmMirrorConflict=false;
static uint32_t g_vmMirrorChanges=0,g_vmMirrorRestores=0;
static std::atomic<int>g_vmMirrorCommand{-1};
static skviewmodel::Store g_vmStore{};
static bool g_vmReady=true,g_vmEnabled=true,g_vmScaleUseBones=false,g_vmScaleRewriteLogged=false;
static float g_vmScaleLastWrite[2]={1.f,1.f};
static uint32_t g_vmScaleFieldWrites=0,g_vmScaleRewrites=0,g_vmScaleBoneBuilds=0;
static int g_offModelScale=-1;
static std::atomic_bool g_run{true};
static bool SkeetoFeaturesOn(){return true;}
static unsigned char vm[2][0xc00],weapon[2][0xa00],info[3][0x900],fallback[0x900];
static void* currentInfo[2]={info[0],info[1]};
struct Binding {void*entity;int profile;};static Binding g_vmBindings[2]={{vm[0],0},{vm[1],1}};
static uint32_t g_vmMirrorBuilds=0,g_vmMirrorMatrices=0;
static unsigned g_paintTid=7,threadId=7;
static unsigned GetCurrentThreadId(){return threadId;}
namespace skmem { static bool Copy(void*d,const void*s,size_t n){memcpy(d,s,n);return true;} }
static bool writeOk=true;
static int g_handleLookupState=1;
template<class T>static bool EntRead(const void*p,int off,T*out){memcpy(out,(const char*)p+off,sizeof(T));return true;}
template<class T>static bool EntWrite(void*p,int off,const T&value){if(!writeOk)return false;memcpy((char*)p+off,&value,sizeof(T));return true;}
template<class T>static bool EntReadBlock(const void*p,int off,T*out,size_t n){memcpy(out,(const char*)p+off,n);return true;}
static void*EntFromHandle(int h){return h==10?weapon[0]:h==11?weapon[1]:nullptr;}
static void Log(const char*,...){}
static const char*argument=nullptr;static int errors=0;
static int CmdArgC(int*){return argument?2:1;}
static const char*CmdArg(int*,int){return argument;}
static void ConEcho(const char*){++errors;}
'''
TEST=r'''
static void* __cdecl Getter(int h){return h>=1&&h<=2?currentInfo[h-1]:fallback;}
static bool ViewModelMirrorResolve(){assert(false);return false;}
static bool Cull(unsigned char*p){return p[0x859]==0;}
static void Reset(){
 g_vmMirrorEnabled=true;g_vmMirrorPerModel=false;g_vmMirrorActive=g_vmMirrorFailed=g_vmMirrorConflict=false;writeOk=true;g_vmStore={};g_vmReady=true;
 g_vmMirrorChanges=g_vmMirrorRestores=0;g_vmMirrorOwners[0]={};g_vmMirrorOwners[1]={};
 g_vmMirrorInfo=&Getter;g_vmMirrorDefaultInfo=fallback;currentInfo[0]=info[0];currentInfo[1]=info[1];
 info[0][0x859]=1;info[1][0x859]=0;info[2][0x859]=1;
 int h0=10,h1=11;uint16_t script0=1,script1=2;
 memcpy(vm[0]+0xbc0,&h0,4);memcpy(vm[1]+0xbc0,&h1,4);
 memcpy(weapon[0]+0x9da,&script0,2);memcpy(weapon[1]+0x9da,&script1,2);
}
static unsigned char buildHeader[64],rawHeader[256],viewBytes[100];
static float buildBones[3][3][4];static unsigned buildFlags[3]={0x100,0x100,0x100};static int upstreamBuilds=0;
static const uint8_t* __fastcall GetView(void*,void*,int){return viewBytes;}
static void __fastcall Build(void*,void*,void*,void*,void*,const float*,int,void*){
 ++upstreamBuilds;assert(info[0][0x859]==1);
 for(int i=0;i<3;++i){memset(buildBones[i],0,sizeof(buildBones[i]));for(int j=0;j<3;++j)buildBones[i][j][j]=1;buildBones[i][1][3]=float(i+4);}
}
static void TestBuild(){
 Reset();ViewModelMirrorPump(true);g_vmMirrorBuildOriginal=(ViewModelBuildFn)&Build;g_vmMirrorGetView=(ViewModelViewFn)&GetView;
 void* renderer=viewBytes;g_vmMirrorView=&renderer;void* raw=rawHeader;void* flags=buildFlags;void* matrices=buildBones;int count=3;
 memcpy(buildHeader,&raw,4);memcpy(buildHeader+0x2c,&flags,4);memcpy(rawHeader+0x9c,&count,4);memcpy(vm[0]+0x6c0,&matrices,4);
 skviewmodel::Vec eye={0,2,0};memset(viewBytes,0,sizeof(viewBytes));memcpy(viewBytes+0x3c,&eye,sizeof(eye));
 for(int n=0;n<2;++n){Hooked_ViewModelBuild(vm[0],nullptr,buildHeader,nullptr,nullptr,nullptr,0x100,nullptr);
  assert(info[0][0x859]==0&&buildBones[0][1][3]==0&&buildBones[1][1][3]==-1&&buildBones[2][1][3]==-2);
  assert(buildBones[0][1][1]==-1&&buildBones[1][1][1]==-1&&buildBones[2][1][1]==-1);}
 assert(upstreamBuilds==2&&g_vmMirrorBuilds==2&&g_vmMirrorMatrices==6);
 g_vmMirrorActive=false;ViewModelMirrorPump(false);Hooked_ViewModelBuild(vm[0],nullptr,buildHeader,nullptr,nullptr,nullptr,0x100,nullptr);
 assert(buildBones[0][1][3]==4&&buildBones[0][1][1]==1);
 puts("PASS production build hook restores upstream handedness, reflects all bones once per rebuild and disables cleanly");
}
static void TestSharedOwnerConflict(){
 Reset();upstreamBuilds=0;g_vmMirrorBuilds=g_vmMirrorMatrices=0;g_vmMirrorPerModel=true;g_vmMirrorEnabled=false;g_vmStore.count=2;
 g_vmStore.records[0].pose.mirror=true;g_vmStore.records[1].pose.mirror=false;
 int sharedWeapon=10;uint16_t sharedScript=1;
 memcpy(vm[0]+0xbc0,&sharedWeapon,4);memcpy(vm[1]+0xbc0,&sharedWeapon,4);
 memcpy(weapon[0]+0x9da,&sharedScript,2);memcpy(weapon[1]+0x9da,&sharedScript,2);
 ViewModelMirrorPump(true);
 assert(!g_vmMirrorActive&&g_vmMirrorConflict&&g_vmMirrorChanges==0);
 assert(info[0][0x859]==1&&!Cull(info[0]));
 g_vmMirrorBuildOriginal=(ViewModelBuildFn)&Build;g_vmMirrorGetView=(ViewModelViewFn)&GetView;
 void* renderer=viewBytes;g_vmMirrorView=&renderer;void* raw=rawHeader;void* flags=buildFlags;void* matrices=buildBones;int count=3;
 memcpy(buildHeader,&raw,4);memcpy(buildHeader+0x2c,&flags,4);memcpy(rawHeader+0x9c,&count,4);
 memcpy(vm[0]+0x6c0,&matrices,4);memcpy(vm[1]+0x6c0,&matrices,4);
 skviewmodel::Vec eye={0,2,0};memset(viewBytes,0,sizeof(viewBytes));memcpy(viewBytes+0x3c,&eye,sizeof(eye));
 Hooked_ViewModelBuild(vm[0],nullptr,buildHeader,nullptr,nullptr,nullptr,0x100,nullptr);
 assert(!Cull(info[0])&&buildBones[0][1][3]==4&&buildBones[0][1][1]==1);
 Hooked_ViewModelBuild(vm[1],nullptr,buildHeader,nullptr,nullptr,nullptr,0x100,nullptr);
 assert(!Cull(info[0])&&buildBones[0][1][3]==4&&buildBones[0][1][1]==1);
 assert(upstreamBuilds==2&&g_vmMirrorBuilds==0);
 g_vmStore.records[1].pose.mirror=true;ViewModelMirrorPump(true);
 assert(g_vmMirrorActive&&!g_vmMirrorConflict&&g_vmMirrorChanges==1&&Cull(info[0]));
 Hooked_ViewModelBuild(vm[0],nullptr,buildHeader,nullptr,nullptr,nullptr,0x100,nullptr);
 assert(Cull(info[0])&&buildBones[0][1][3]==0&&buildBones[0][1][1]==-1);
 Hooked_ViewModelBuild(vm[1],nullptr,buildHeader,nullptr,nullptr,nullptr,0x100,nullptr);
 assert(Cull(info[0])&&buildBones[0][1][3]==0&&buildBones[0][1][1]==-1);
 assert(upstreamBuilds==4&&g_vmMirrorBuilds==2);
 g_vmStore.records[1].pose.mirror=false;ViewModelMirrorPump(true);
 assert(!g_vmMirrorActive&&g_vmMirrorConflict&&info[0][0x859]==1&&!Cull(info[0]));
 Hooked_ViewModelBuild(vm[1],nullptr,buildHeader,nullptr,nullptr,nullptr,0x100,nullptr);
 assert(!Cull(info[0])&&buildBones[0][1][3]==4&&buildBones[0][1][1]==1);
 g_vmStore.records[1].pose.mirror=true;ViewModelMirrorPump(true);assert(g_vmMirrorActive&&Cull(info[0]));
 g_handleLookupState=0;g_vmStore.records[1].pose.mirror=false;ViewModelMirrorPump(true);
 assert(!g_vmMirrorActive&&g_vmMirrorConflict&&info[0][0x859]==1&&!Cull(info[0]));
 g_handleLookupState=1;
 puts("PASS shared owner conflicts keep culling and both skeletons aligned; matching flags mirror both");
}
static void TestCompleteSkeleton(){
 float bones[3][3][4]={{{1,0,0,10},{0,1,0,4},{0,0,1,3}},{{1,0,0,11},{0,1,0,7},{0,0,1,4}},{{1,0,0,12},{0,1,0,-2},{0,0,1,5}}};
 float original[3][3][4];memcpy(original,bones,sizeof(bones));
 const unsigned flags[3]={0x100,0x100,0x200};
 ViewModelMirrorBones(bones,flags,3,0x100,{0,2,0},{0,1,0});
 assert(bones[0][1][3]==0&&bones[1][1][3]==-3&&bones[0][1][1]==-1&&bones[1][1][1]==-1);
 assert(!memcmp(bones[2],original[2],sizeof(bones[2])));
 ViewModelMirrorBones(bones,flags,3,0x100,{0,2,0},{0,1,0});
 assert(!memcmp(bones,original,sizeof(bones)));
 const float r=std::sqrt(.5f);ViewModelMirrorBones(bones,flags,3,0x100,{0,0,0},{r,r,0});
 assert(std::fabs(bones[1][0][3]+7)<.0001f&&std::fabs(bones[1][1][3]+11)<.0001f);
 puts("PASS full parent/child skeleton reflection, camera plane, mask and involution");
}
int main(){TestCompleteSkeleton();TestBuild();TestSharedOwnerConflict();
 static_assert(sizeof(void*)==4,"x86 ABI required");Reset();ViewModelMirrorPump(true);
 assert(g_vmMirrorActive&&g_vmMirrorChanges==2&&Cull(info[0])&&!Cull(info[1]));
 for(int i=0;i<100;++i)ViewModelMirrorPump(true);assert(g_vmMirrorChanges==2);
 ViewModelMirrorPump(false);assert(!g_vmMirrorActive&&info[0][0x859]==1&&info[1][0x859]==0&&g_vmMirrorRestores==2);
 puts("PASS native culling handedness ownership; original left/right restore");
 Reset();uint16_t one=1;memcpy(weapon[1]+0x9da,&one,2);ViewModelMirrorPump(true);assert(g_vmMirrorChanges==1);
 puts("PASS duplicate owners never double-flip");
 Reset();g_vmMirrorPerModel=true;g_vmMirrorEnabled=false;g_vmStore.count=2;g_vmStore.records[0].pose.mirror=true;
 ViewModelMirrorPump(true);assert(g_vmMirrorActive&&g_vmMirrorChanges==1&&info[0][0x859]==0&&info[1][0x859]==0);
 assert(ViewModelMirrorSlotEnabled(0)&&!ViewModelMirrorSlotEnabled(1));
 g_vmStore.records[0].pose.mirror=false;g_vmStore.records[1].pose.mirror=true;ViewModelMirrorPump(true);
 assert(g_vmMirrorChanges==2&&g_vmMirrorRestores==1&&info[0][0x859]==1&&info[1][0x859]==1);
 assert(!ViewModelMirrorSlotEnabled(0)&&ViewModelMirrorSlotEnabled(1));
 g_vmStore.records[1].pose.mirror=false;ViewModelMirrorPump(true);
 assert(!g_vmMirrorActive&&g_vmMirrorRestores==2&&info[1][0x859]==0);
 puts("PASS per-model flags independently select one owner and restore stale models");
 Reset();ViewModelMirrorPump(true);int invalid=-1;memcpy(vm[0]+0xbc0,&invalid,4);ViewModelMirrorPump(true);
 assert(info[0][0x859]==1&&g_vmMirrorRestores==1);ViewModelMirrorPump(false);assert(info[1][0x859]==0);
 puts("PASS weapon switch restores previous metadata");
 Reset();ViewModelMirrorPump(true);currentInfo[0]=info[2];ViewModelMirrorPump(false);
 assert(info[2][0x859]==1&&info[0][0x859]==0);puts("PASS replaced script object is never overwritten");
 Reset();ViewModelMirrorPump(true);info[0][0x859]=1;ViewModelMirrorPump(false);assert(info[0][0x859]==1);
 Reset();writeOk=false;ViewModelMirrorPump(true);assert(g_vmMirrorFailed&&!g_vmMirrorActive&&info[0][0x859]==1&&info[1][0x859]==0);
 puts("PASS unexpected changes and failed writes preserve native data");
 for(const char*a:{"0","1","toggle"}){argument=a;g_vmMirrorCommand=-1;SkCmd_ViewModelMirror(nullptr);assert(g_vmMirrorCommand==(a[0]=='0'?0:a[0]=='1'?1:2));}
 argument=nullptr;SkCmd_ViewModelMirror(nullptr);assert(g_vmMirrorCommand==2);
 argument="bad";g_vmMirrorCommand=-1;SkCmd_ViewModelMirror(nullptr);assert(g_vmMirrorCommand==-1&&errors==1);
 puts("PASS console 0/1/toggle publishes requests and rejects invalid input");return 0;
}
'''
def main():
    s=(ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    begin=s.index('using ViewModelInfoFn =');end=s.index('using ViewModelBuildFn =',begin)
    chunks=[STUBS,'namespace skviewmodel { '+block(s,r'^static Mat AngleMatrix\(Vec a\)\s*\{')+' }',s[begin:end],s[end:s.index('// One camera plane',end)],'static bool ViewModelMirrorResolve(); static bool ViewModelMirrorInstallBuild(){return true;} static void ViewModelMirrorRemoveBuild(){}']
    for name in ['ViewModelMirrorBones','ViewModelScaleBones','ViewModelScaleFieldPump','Hooked_ViewModelBuild','ViewModelMirrorRestore','ViewModelMirrorPump','SkCmd_ViewModelMirror']:
        chunks.append(block(s,rf'^static [^;\n]*\b{name}\([^;]*?\)\s*\{{'))
    chunks.append(TEST)
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-native-mirror-') as temp:
        out=Path(temp);(out/'fixture.cpp').write_text('\n'.join(chunks),encoding='utf-8')
        (out/'run.bat').write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /EHsc /O2 /std:c++17 /utf-8 /W3 fixture.cpp /Fe:fixture.exe\nexit /b %errorlevel%\n',encoding='utf-8')
        subprocess.run(['cmd','/c',str(out/'run.bat')],cwd=out,check=True)
        subprocess.run([str(out/'fixture.exe')],check=True)
if __name__=='__main__':main()
