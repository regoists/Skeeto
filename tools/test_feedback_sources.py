"""Run real JSON parsing, resolution and feedback entry points without Source IO."""
from pathlib import Path
import os
import subprocess
import tempfile
from test_chapstats_pages import ROOT, block

PREFIX = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include "skeeto_style.h"
static DlcCatalog g_cat{};
static int g_styleLimit=64;
'''

PROVIDERS = r'''
static bool g_optSiVisual=true,g_optSiSound=true,g_optSound=true,g_optIcon=true,g_optFf=true;
static bool hitEnabled=true;
static DWORD fixtureNow=1000,g_lastHitFeedbackAt=0,g_siStreakLastKillAt=0,g_ciStreakLastKillAt=0,g_suppressCommonHitUntil=0;
#define GetTickCount() fixtureNow
static int g_siStreak=0,g_ciStreak=0,g_feedbackStreakWindowSec=3,g_feedbackLogLeft=0;
static void* g_engine=(void*)1;
static bool g_haveImpactPos=false;
static float g_lastImpactPos[3]{};
static constexpr DWORD kOverlayClearMsHit=200,kOverlayClearMs=500;
static bool EngInGame(){return true;}
static bool HitModeSiAllowed(){return hitEnabled;}
static bool HitModeCiAllowed(){return hitEnabled;}
static void Log(const char*,...){}
static bool EvBool(void*,const char*){return false;}
static int EvInt(void*,const char*,int fallback=0){return fallback;}
static float EvFloat(void*,const char*){return 0;}
static bool EventWeaponIsMelee(void*){return false;}
static int applied=0,splats=0;
static DlcFx last{};
static void BloodSplatNoteHit(){++splats;}
static void ApplyDlcFx(const DlcFx* fx,bool,float,float,float,DWORD=kOverlayClearMs,bool=false){++applied;last=*fx;}
static void FeedbackAdvanceStreak(const char*,int*,DWORD*);
'''

CASES = r'''
static bool Check(bool value,const char* message){if(!value)puts(message);return value;}
static void Load(const char* channel,const char* events){
 std::string json=std::string("{\"id\":\"")+channel+"\",\"name\":\"fixture\",\"channel\":\""+channel+"\","+events+"}";
 AddStyleFromJson(json.c_str());DlcSetSelected(channel,channel);
}
static const char* CiMedia="\"hit\":{\"overlay\":\"ci_icon\",\"particle\":\"ci_particle\",\"sound\":\"ci_sound\",\"priority\":9},\"kill\":{\"overlay\":\"ci_icon\",\"sound\":\"ci_sound\",\"priority\":9}";
static const char* SiMedia="\"hit\":{\"overlay\":\"si_icon\",\"particle\":\"si_particle\",\"sound\":\"si_sound\",\"priority\":17},\"kill\":{\"overlay\":\"si_icon\",\"sound\":\"si_sound\",\"priority\":17}";
static void ClearSink(){applied=0;splats=0;last={};}
static void Fire(bool kill){if(kill)FeedbackSiKill((void*)1);else FeedbackSiHit(0,0,0);}
int main(int argc,char** argv){
 if(argc!=2)return 2;const int test=atoi(argv[1]);bool ok=true;
 if(test==0){
  Load("ci",CiMedia);Load("si",SiMedia);DlcSetSelected("si","off");
  for(int visual=0;visual<2;++visual)for(int sound=0;sound<2;++sound)for(int kill=0;kill<2;++kill){
   g_optSiVisual=visual!=0;g_optSiSound=sound!=0;ClearSink();Fire(kill!=0);
   ok &= Check(applied==0 && splats==0 && g_siStreak==0,"SI off must not borrow CI media or advance SI feedback work");
  }
 }else if(test==1 || test==2 || test==18){
  Load("ci",CiMedia);Load("si","\"hit\":{},\"kill\":{}");
  if(test==18)DlcSetSelected("si","not-mounted");
  Fire(test==2);ok &= Check(applied==0,"missing selected SI event/pack must not borrow CI");
 }else if(test==3 || test==4 || test==20){
  Load("ci",CiMedia);Load("si",SiMedia);
  if(test==3){Load("si","\"hit\":{} ,\"kill\":{}");g_optSiSound=false;}
  else {DlcSetSelected("ci","off");g_optSiVisual=false;}
  if(test==20)g_optSiSound=false;
  for(int kill=0;kill<2;++kill){
   ClearSink();Fire(kill!=0);
   if(test==20)ok &= Check(!applied,"explicit CI sources do not fall back to available SI media");
   else ok &= Check(applied==1 && !last.overlay[0] && !last.particle[0] && !last.overlaySequenceCount && !last.particleExtraCount
    && !strcmp(last.sound,test==3?"ci_sound":"si_sound") && last.priority==(test==3?9:17),"missing visual source stays empty while explicit audio retains its priority");
  }
 }else if(test==5 || test==6){
  Load("ci",CiMedia);Load("si",SiMedia);
  const int variants=test==5?1:2;
  for(int n=0;n<variants;++n){
   g_optSiVisual=test==6 && n==0;g_optSiSound=test==6 && n==1;
   for(int kill=0;kill<2;++kill){
    ClearSink();Fire(kill!=0);
    ok &= Check(applied==1 && !strcmp(last.overlay,g_optSiVisual?"si_icon":"ci_icon")
     && !strcmp(last.sound,g_optSiSound?"si_sound":"ci_sound") && last.priority==(g_optSiVisual?17:9),"explicit CI and mixed SI/CI sources remain independent");
   }
  }
 }else if(test==7 || test==8){
  Load("ci",test==8?"\"hit\":{\"overlay\":\"ci_icon\"}":CiMedia);
  Load("si",test==7?"\"hit\":{\"overlay\":\"si_icon\"}":SiMedia);
  g_optSiSound=test==7;FeedbackSiHit(0,0,0);
  ok &= Check(applied==1 && !last.sound[0] && !last.soundExtraCount,"empty audio in the chosen source stays silent");
 }else if(test>=9 && test<=14){
  const char* names[]={"hit","kill","headshot","melee","streak_1","streak_default"};
  const char* event=names[test-9];
  for(const char* channel:{"ci","si","ff"}){
   std::string events=test>=13?"\"streak\":{\"enabled\":true,\"wrap\":0},":"";
   if(test>=11)events+="\"kill\":{\"sound\":\"base_sound\",\"priority\":1},";
   events+=std::string("\"")+event+"\":{\"sounds\":[\"pool_a\",\"pool_b\"],\"priority\":30}";
   Load(channel,events.c_str());DlcFx fx{};
   bool resolved=DlcResolve(channel,test==9?"hit":"kill",test==11,test==12,test==13?1:test==14?8:0,&fx);
   ok &= Check(resolved && !fx.sound[0] && fx.soundExtraCount==2 && fx.priority==30,"real parser/resolver must retain pool-only events in CI/SI/FF");
   char sound[96]{};ok &= Check(DlcPickSound(&fx,sound,sizeof(sound)) && (!strcmp(sound,"pool_a") || !strcmp(sound,"pool_b")),"pool-only events must yield their own sound");
  }
 }else if(test==15 || test==16){
  Load("ci","\"streak\":{\"enabled\":true,\"wrap\":2},\"kill\":{\"sound\":\"base\",\"priority\":1},\"streak_1\":{\"sound\":\"one\",\"priority\":20},\"streak_default\":{\"sound\":\"overflow\",\"priority\":10},\"headshot\":{\"sound\":\"hs\",\"priority\":30},\"melee\":{\"sound\":\"knife\",\"priority\":20}");
  DlcFx fx{};
  if(test==15){
   DlcResolve("ci","kill",false,false,1,&fx);ok &= Check(!strcmp(fx.sound,"one"),"exact streak slot retained");
   DlcResolve("ci","kill",false,false,2,&fx);ok &= Check(!strcmp(fx.sound,"overflow"),"same-pack streak default retained");
   DlcResolve("ci","kill",false,false,3,&fx);ok &= Check(!strcmp(fx.sound,"one"),"streak wrap retained");
  }else {
   DlcResolve("ci","kill",true,false,1,&fx);ok &= Check(!strcmp(fx.sound,"hs"),"higher-priority headshot overrides streak");
   DlcResolve("ci","kill",false,true,1,&fx);ok &= Check(!strcmp(fx.sound,"one"),"equal-priority melee must not override streak");
  }
 }else if(test==17){
  Load("fx","\"skeet\":{\"particle\":\"\",\"sound\":\"\"}");DlcFx fx{};
  ok &= Check(DlcResolveNamed("fx","skeet",&fx) && fx.used && !fx.particle[0],"explicit empty named particle remains an intentional override");
 }else if(test==21){
  for(const char* channel:{"ci","si","ff"}){
   Load(channel,"\"hit\":{\"sounds\":[\"pool_a\",\"pool_b\"]}");ClearSink();
   if(!strcmp(channel,"ci"))FeedbackCi("hit",true,false,false,0,0,0);
   else if(!strcmp(channel,"si"))FeedbackSiHit(0,0,0);
   else FeedbackFf(1,0,0,0);
   ok &= Check(applied==1 && !last.sound[0] && last.soundExtraCount==2,"pool-only media reaches the real CI/SI/FF entry points");
  }
 }else if(test==22){
  Load("si",SiMedia);DlcSetSelected("ci","off");DlcSetSelected("ff","off");
  FeedbackCi("hit",true,false,false,0,0,0);FeedbackFf(1,0,0,0);
  ok &= Check(!applied && !splats,"disabled CI and FF channels do not borrow SI media");
 }else if(test==23){
  Load("si","\"streak\":{\"enabled\":true,\"wrap\":0},\"streak_1\":{\"overlay\":\"first_kill\",\"priority\":10},\"streak_2\":{\"sound\":\"silent_selection\",\"priority\":30}");
  DlcSetSelected("ci","off");g_optSiSound=false;
  FeedbackSiKill((void*)1);FeedbackSiKill((void*)1);
  ok &= Check(applied==1 && !strcmp(last.overlay,"first_kill") && last.priority==10 && g_siStreak==2,"empty composed kills must not be published while their streak still advances");
 }else if(test==24){
  Load("si","\"streak\":{\"enabled\":true,\"wrap\":0},\"streak_1\":{\"overlay\":\"first_kill\",\"priority\":10},\"streak_2\":{\"sounds\":[\"si_unused\"],\"priority\":30}");
  Load("ci","\"kill\":{\"sound\":\"ci_played\",\"priority\":9}");g_optSiSound=false;
  FeedbackSiKill((void*)1);ok &= Check(applied==1 && last.priority==10,"real visual media retains its event priority");
  FeedbackSiKill((void*)1);
  ok &= Check(applied==2 && !last.overlay[0] && !last.particle[0] && !strcmp(last.sound,"ci_played") && last.priority==9,"sound-only composition uses actual audio priority rather than discarded visual-event audio");
 }else if(test==25){
  Load("ci","\"hit\":{\"sounds\":[\"ci_unused\"],\"priority\":30}");
  Load("si","\"hit\":{\"sound\":\"si_played\",\"priority\":9}");g_optSiVisual=false;
  FeedbackSiHit(0,0,0);
  ok &= Check(applied==1 && !last.overlay[0] && !last.particle[0] && !strcmp(last.sound,"si_played") && last.priority==9,"reversed explicit sources also use actual audio priority when no visual remains");
 }else if(test==19){
  Load("ci",CiMedia);Load("si",SiMedia);hitEnabled=false;FeedbackSiHit(0,0,0);
  ok &= Check(!applied && !splats,"hit mode still gates SI hits");
 }else return 2;
 return ok?0:1;
}
'''


def main():
    core = ROOT / 'src/skeeto/core'
    style = (core / 'skeeto_style.cpp').read_text(encoding='utf-8-sig')
    proxy = (core / 'skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    parser = style[style.index('static const char* SkipWs('):style.index('static IFileSystem* FsGet()')]
    style_functions = '\n'.join(block(style, p) for p in [
        r'^static void AddStyleFromJson\(const char\* json\) \{', r'^const DlcStyle\* DlcFindStyle\(',
        r'^void DlcSetSelected\(', r'^const char\* DlcGetSelected\(', r'^static void Consider\(',
        r'^bool DlcResolve\(', r'^bool DlcResolveNamed\(', r'^const char\* DlcPickSound\(',
    ])
    runtime = '\n'.join(block(proxy, r'^static (?:void|bool|int) '+name+r'\([^;]*?\) \{') for name in [
        'CiChannelOn', 'SiChannelOn', 'AssembleSiFx', 'FeedbackCi', 'FeedbackFf',
        'FeedbackStreakMax', 'FeedbackAdvanceStreak', 'FeedbackSiHit', 'FeedbackSiKill',
    ])
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-feedback-sources-') as directory:
        work = Path(directory)
        (work / 'fixture.cpp').write_text(PREFIX + parser + style_functions + PROVIDERS + runtime + CASES, encoding='utf-8')
        (work / 'build.bat').write_text(f'@echo off\ncall "{vs}/VC/Auxiliary/Build/vcvars32.bat" >nul\n'
            f'cl /nologo /MT /EHsc /std:c++17 /utf-8 /W3 /WX /I"{core}" fixture.cpp /Fefixture.exe\nexit /b %errorlevel%\n', encoding='utf-8')
        built = subprocess.run(['cmd', '/c', 'build.bat'], cwd=work, capture_output=True)
        assert built.returncode == 0, (built.stdout + built.stderr).decode(errors='replace')
        failed = []
        for case in range(26):
            result = subprocess.run([str(work / 'fixture.exe'), str(case)], capture_output=True, text=True, timeout=10)
            print(f'case {case}: {"PASS" if result.returncode == 0 else "FAIL"}', flush=True)
            if result.returncode:
                failed.append(case)
                print(result.stdout + result.stderr, flush=True)
        assert not failed, f'failed cases: {failed}'
    print('PASS: real parser/resolver, disabled SI, strict sources, pool-only media and intentional same-pack priorities')


if __name__ == '__main__':
    main()
