"""Compile the production mission parser with real KeyValues-shaped fixtures."""
from pathlib import Path
import os
import subprocess
import tempfile

def main():
    root=Path(__file__).resolve().parents[1]
    fixture=r'''#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <Windows.h>
#include <cwchar>
#include "skeeto_mission.h"
using namespace skmission;
void require(bool b,const char* text){if(!b){puts(text);exit(1);}}
struct DlcGameGlobScan { size_t at=0; };
std::vector<std::string> files;
int readCalls=0, beginCalls=0, closeCalls=0;
DWORD tick=1;
int localizeCalls=0;
wchar_t* __fastcall FakeFind(void*,void*,const char* token){++localizeCalls;if(!strcmp(token,"#L4D360UI_LevelName_COOP_C1M1"))return const_cast<wchar_t*>(L"旅馆");return !strcmp(token,"#CampaignTitle")?const_cast<wchar_t*>(L"本地化战役"):nullptr;}
void* fakeVtable[3]={nullptr,nullptr,(void*)&FakeFind};
void** fakeLocalize=fakeVtable;
void* GetIface(const char*,const char*){return &fakeLocalize;}
bool IfaceAlive(void* p){return p!=nullptr;}
char g_elimLastMap[80]{},g_mapHudLayoutStem[80]{};
void MapHudStem(const char* s,char* out,int){char stem[80]{};skmission::Stem(s,stem);strcpy(out,stem);}
void MapHudParse(const char* stem,int* camp,int* chapter){*camp=!strcmp(stem,"c1m1_hotel")?1:0;*chapter=*camp?1:0;}
int MapHudCampaignChapters(int){return 0;}
DlcGameGlobScan* DlcGameGlobScanBegin(const char*){++beginCalls;return new DlcGameGlobScan;}
void DlcGameGlobScanCancel(DlcGameGlobScan* s){++closeCalls;delete s;}
int DlcGameGlobScanPump(DlcGameGlobScan* s,int budget,void(*fn)(const char*,void*),void*){
 require(budget==1,"one file per Pump");char name[32]{};sprintf_s(name,"%zu.txt",s->at);fn(name,nullptr);++s->at;return s->at==files.size()?1:0;
}
bool DlcReadGameBinaryLimited(const char*,const char* path,void** out,int* size,unsigned limit){
 require(limit==65536,"file size bound");++readCalls;size_t i=std::atoi(strrchr(path,'/')+1);require(i<files.size(),"valid fake file");
 *size=(int)files[i].size();*out=malloc(*size);memcpy(*out,files[i].data(),*size);return true;
}
#define GetTickCount() tick
// PRODUCTION_PUMP
#undef GetTickCount
void TestDriver(){
 files={"mission {modes {coop {1 {Map first} 2 {Map second}}}}","mission {modes {coop {1 {Map first} 2 {Map second} 3 {Map third}}}}"};
 strcpy(g_elimLastMap,"first");MapHudMissionPump("coop");require(readCalls==1&&!g_mapMissionDone,"partial inventory stays pending");
 MapHudMissionPump("coop");require(readCalls==1,"100ms rate bound");
 strcpy(g_elimLastMap,"second");tick+=200;MapHudMissionPump("coop");require(readCalls==2&&!g_mapMissionDone,"mid-scan chapter change must not publish candidate cache");
 tick+=200;MapHudMissionPump("coop");require(g_mapMissionDone&&g_mapMissionConflict&&g_mapMissionChap==0,"conflicting mission totals remain unknown");
 int before=readCalls;tick+=200;MapHudMissionPump("coop");require(readCalls==before,"completed scan does not read again");
 MapHudMissionPump(nullptr);require(!g_mapMissionScan,"disabled closes iterator");
 files.resize(1);strcpy(g_elimLastMap,"first");tick+=200;MapHudMissionPump("coop");require(g_mapMissionDone&&g_mapMissionChap==1,"complete mission published");
 int chapter=0,total=0;require(MapHudChapterCounts("first",&chapter,&total)&&chapter==1&&total==2,"TAB reads shared completed mission");
 require(!MapHudChapterCounts("different",&chapter,&total),"TAB cannot use another map cache");
 before=readCalls;strcpy(g_elimLastMap,"second");tick+=200;MapHudMissionPump("coop");require(g_mapMissionChap==2&&readCalls==before+1,"new chapter rescans its own mission definitions");
 MapHudMissionPump(nullptr);require(!g_mapMissionScan&&!g_mapMissionChapters.count,"disable clears campaign cache");
 files={"mission {modes {coop {1 {Map first} 2 {Map second}}}}","mission {modes {coop {1 {Map second} 2 {Map third}}}}"};
 strcpy(g_elimLastMap,"first");tick+=200;MapHudMissionPump("coop");tick+=200;MapHudMissionPump("coop");
 require(g_mapMissionDone&&g_mapMissionChap==1&&!g_mapMissionConflict,"first belongs to only one definition");
 strcpy(g_elimLastMap,"second");tick+=200;MapHudMissionPump("coop");tick+=200;MapHudMissionPump("coop");
 require(g_mapMissionDone&&g_mapMissionConflict&&!g_mapMissionChap,"overlap appearing only in the next chapter must be rechecked");
 MapHudMissionPump(nullptr);
 files.assign(300,files[0]);strcpy(g_elimLastMap,"first");
 for(int i=0;i<256;++i){tick+=200;MapHudMissionPump("coop");}
 require(g_mapMissionDone&&!g_mapMissionScan&&!g_mapMissionChap,"inventory bound cannot publish incomplete total");
 MapHudMissionPump(nullptr);
 files={u8"mission {DisplayTitle \"作者战役名\" modes {coop {1 {Map first DisplayName \"第一章\"}}}}"};
 strcpy(g_elimLastMap,"first");tick+=200;MapHudMissionPump("coop");
 require(!wcscmp(MapHudMissionName("first",false),L"作者战役名"),"HUD and TAB receive UTF8 campaign title");
 require(!wcscmp(MapHudMissionName("first",true),L"第一章"),"TAB receives chapter display name");
 require(!MapHudMissionName("different",false),"title cannot leak to another map");
 before=readCalls;int finds=localizeCalls;tick+=200;MapHudMissionPump("coop");
 require(before==readCalls&&finds==localizeCalls,"completed titles do not repeat IO or localization");
 MapHudMissionPump(nullptr);require(!MapHudMissionName("first",false),"disable clears title");
 files={"mission {DisplayTitle #CampaignTitle modes {coop {1 {Map first DisplayName #Missing}}}}"};
 tick+=200;MapHudMissionPump("coop");
 require(!wcscmp(MapHudMissionName("first",false),L"本地化战役"),"game localization tokens resolve once");
 require(!MapHudMissionName("first",true),"missing token is not displayed literally");
 MapHudMissionPump(nullptr);
 files={"mission {modes {coop {1 {Map first DisplayName Station}}}}"};
 tick+=200;MapHudMissionPump("coop");require(!MapHudMissionName("first",false),"missing campaign title must not borrow the chapter label");
 require(!wcscmp(MapHudMissionName("first",true),L"Station"),"chapter label remains available when campaign title is missing");
 MapHudMissionPump(nullptr);
 files={"mission {DisplayTitle Alpha modes {coop {1 {Map first}}}}","mission {DisplayTitle Beta modes {coop {1 {Map first}}}}"};
 tick+=200;MapHudMissionPump("coop");require(!MapHudMissionName("first",false),"partial inventory cannot publish title");
 tick+=200;MapHudMissionPump("coop");require(!MapHudMissionName("first",false),"ambiguous titles stay unknown");
 require(MapHudChapterCounts("first",&chapter,&total)&&total==1,"title conflict does not discard agreed chapter total");
 MapHudMissionPump(nullptr);
 files={"mission {DisplayTitle \"  Station\nNorth  \" modes {coop {1 {Map first}}}}"};
 tick+=200;MapHudMissionPump("coop");require(!wcscmp(MapHudMissionName("first",false),L"Station North"),"metadata cannot introduce extra HUD lines");
 MapHudMissionPump(nullptr);
 files={"mission {DisplayTitle \""+std::string(200,'x')+"\" modes {coop {1 {Map first}}}}"};
 tick+=200;MapHudMissionPump("coop");
 wchar_t line[80]{};require(swprintf_s(line,L"%s  Chapter 32/32",MapHudMissionName("first",false))>0,"long author names leave room for English chapter count");
 MapHudMissionPump("survival");require(!MapHudMissionName("first",false),"mode change cannot reuse old title");
 MapHudMissionPump(nullptr);
 before=readCalls;strcpy(g_elimLastMap,"c1m1_hotel");tick+=200;MapHudMissionPump("coop");
 require(readCalls==before,"official chapter names keep the no-mission-IO fast path");
 require(!wcscmp(MapHudMissionName("c1m1_hotel",true),L"旅馆"),"official chapter name comes from game localization");
 MapHudMissionPump(nullptr);
}
int main(){
 Chapters c{};
 const char* mission=R"KV("mission" { // ordinary workshop mission
 "displaytitle" "Not a map list" "modes" { "coop" {
 "1" {"Map" "maps/start_here.bsp"} "2" {"Map" "SECOND"} "3" {"Map" "no_m_number"}
 "4" {"Map" "fourth"} "5" {"Map" "finale"} }
 "versus" {"1" {"Map" "SECOND"} "2" {"Map" "finale"}} } })KV";
 require(Parse(mission,strlen(mission),"coop",c),"five-map mission parse");
 require(c.count==5&&Index(c,"second")==2&&Index(c,"finale")==5&&Index(c,"start_here")==1,"irregular map names and total/index");
 require(!strcmp(c.title,"Not a map list"),"campaign title is retained");
 const char* named=u8"mission {poster {DisplayTitle Wrong} modes {coop {2 {DisplayName \"车站\" Map second} 1 {Map first DisplayName Start}} versus {1 {Map second DisplayName Arena}}} DisplayTitle \"作者战役名\"}";
 require(Parse(named,strlen(named),"coop",c)&&!strcmp(c.title,u8"作者战役名")&&!strcmp(c.names[1],u8"车站"),"root title and chapter names survive out-of-order fields and UTF8");
 require(Parse(named,strlen(named),"versus",c)&&!strcmp(c.names[0],"Arena"),"chapter labels belong to selected mode");
 require(Parse(named,strlen(named),"realism",c)&&!strcmp(c.names[1],u8"车站"),"realism inherits coop chapter labels");
 require(Parse(mission,strlen(mission),"coop",c),"restore original fixture");
 require(Parse(mission,strlen(mission),"versus",c)&&c.count==2&&Index(c,"second")==1,"use current mode, not coop list");
 require(!Parse(mission,strlen(mission),"survival",c),"missing survival never borrows coop count");
 require(Parse(mission,strlen(mission),"realism",c)&&c.count==5,"realism may inherit coop");
 require(!Parse(mission,strlen(mission),"mutation12",c),"unknown mutation must not borrow coop total");
 std::string bom="\xef\xbb\xbf";bom+=mission;
 require(Parse(bom.data(),bom.size(),"coop",c),"UTF8 BOM");
 const char* gap="mission {modes {coop {1 {Map a} 3 {Map c}}}}";
 require(!Parse(gap,strlen(gap),"coop",c),"gap cannot invent total");
 const char* repeated="mission {modes {coop {1 {Map a} 2 {Map A}}}}";
 require(!Parse(repeated,strlen(repeated),"coop",c),"duplicate chapter identity rejected");
 require(!Parse(mission,strlen(mission)-1,"coop",c),"truncated braces rejected");
 std::string description="mission { Description \""+std::string(1000,'x')+"\" modes {coop {1 {Map single}}}}";
 require(Parse(description.data(),description.size(),"coop",c)&&c.count==1,"long unrelated metadata is skipped without allocation");
 std::string longNames="mission {DisplayTitle \""+std::string(500,'x')+"\" modes {coop {1 {Map single DisplayName \""+std::string(500,'y')+"\"}}}}";
 require(Parse(longNames.data(),longNames.size(),"coop",c)&&c.count==1&&!c.title[0]&&!c.names[0][0],"oversized optional labels do not discard valid chapter counts");
 std::string huge(65537,' ');require(!Parse(huge.data(),huge.size(),"coop",c),"size bound");
 std::string deep="x {";for(int i=0;i<16;++i)deep+="x {";
 require(!Parse(deep.data(),deep.size(),"coop",c),"nesting bound");
 const char* bad="mission {modes {coop {33 {Map outside}}}}";
 require(!Parse(bad,strlen(bad),"coop",c),"32 chapter bound");
 TestDriver();
 puts("PASS mission chapters/names: modes, UTF8, localization, conflicts, cleanup and bounds");
}'''
    fixture=fixture.replace('"skeeto_mission.h"', '"'+(root/'src/skeeto/core/skeeto_mission.h').as_posix()+'"')
    source=(root/'src/skeeto/core/skeeto_hud.cpp').read_text(encoding='utf-8-sig')
    pump=source.split('// Paint-owned, one mission',1)[1].split('void MapHudPaintHud()',1)[0]
    pump='// Paint-owned, one mission'+pump
    fixture=fixture.replace('// PRODUCTION_PUMP',pump)
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-campaign-outline-') as tmp:
        folder = Path(tmp)
        (folder/'fixture.cpp').write_text(fixture, encoding='utf-8')
        (folder/'build.bat').write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n', encoding='utf-8')
        subprocess.run(['cmd','/c','build.bat'],cwd=folder,check=True)


if __name__ == '__main__':
    main()
