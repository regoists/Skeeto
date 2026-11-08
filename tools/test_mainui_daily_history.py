"""Exercise real worker history persistence and HTML rendering in temporary roots."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / 'src/skeeto/core'
FIXTURE = r'''
#include <ctime>
static time_t testNow=0;
static time_t TestNow(time_t* output){if(output)*output=testNow;return testNow;}
// Function-like macro replaces the worker clock, never a record's .time field.
#define time(...) TestNow(__VA_ARGS__)
#include "skeeto_mainui_services.cpp"
#undef time
#include <cassert>
static time_t Stamp(int year,int month,int day,int hour=12){tm t{};t.tm_year=year-1900;t.tm_mon=month-1;t.tm_mday=day;t.tm_hour=hour;t.tm_isdst=-1;return mktime(&t);}
static void Chapter(time_t at,int kills,int special=0,int result=0){testNow=at;MainUiBattleRecord b{};b.stats[0]=kills;b.stats[1]=special;b.result=result;assert(MainUiServicesBattle(b));MainUiServicesWorkerPump();}
static void Export(time_t at){testNow=at;wchar_t path[MAX_PATH]{};MainUiCareerReport career{};assert(Report(path,career));}
// Wire layouts captured from the existing v1/v2 formats, independent of v3.
struct LegacyPersonal {uint64_t time=0;char map[96]{},server[96]{},player[192]{},version[32]{};int result=0,seconds=0,score=0,scoreEnabled=0,pageMask=0,statCount=25,stats[32]{},headshots[9]{},weights[32]{};};
struct LegacySquad {uint64_t time=0;char map[96]{},server[96]{},player[192]{},version[32]{};int result=0,seconds=0,score=0,scoreEnabled=0,pageMask=0,statCount=25,stats[32]{},headshots[9]{},weights[32]{};int mateCount=0;MainUiBattleMate mates[8]{};};
template<class Row>struct LegacyFile {unsigned magic=0x534B4831,version=2;int recent=0,battles=2;MainUiServerRecord servers[10]{};Row reports[10]{};unsigned checksum=0;};
template<class Row>static void OldHistory(const char* rootPath,int version,bool corrupt=false){
 LegacyFile<Row> old{};old.version=version;old.reports[0].time=old.reports[1].time=Stamp(2026,10,27);
 old.reports[0].stats[0]=7;old.reports[1].stats[0]=11;old.reports[0].stats[1]=2;old.reports[1].stats[1]=3;
 old.reports[1].result=version==1?1:0;old.checksum=Hash(&old,offsetof(LegacyFile<Row>,checksum));if(corrupt)old.checksum^=1;
 char path[MAX_PATH]{};sprintf_s(path,"%s/left4dead2/skeeto/mainmenu/history.bin",rootPath);FILE* f=fopen(path,"wb");assert(f);assert(fwrite(&old,1,sizeof(old),f)==sizeof(old));fclose(f);
}
static void Calendar(){
 int keys[7]{};assert(MainUiBattleWindow(Stamp(2027,1,4),keys)&&keys[0]==20261229&&keys[6]==20270104);
 assert(MainUiBattleWindow(Stamp(2028,3,1),keys)&&keys[0]==20280224&&keys[5]==20280229&&keys[6]==20280301);
 _putenv_s("TZ","PST8PDT");_tzset();
 assert(MainUiBattleWindow(Stamp(2026,3,8),keys)&&keys[0]==20260302&&keys[6]==20260308);
 MainUiBattleDays days{};MainUiBattleRecord row{};row.time=Stamp(2026,3,7);row.stats[0]=5;MainUiBattleDaysAdd(days,row,Stamp(2026,3,8));
 row.time=Stamp(2026,3,8,0);row.stats[0]=6;MainUiBattleDaysAdd(days,row,Stamp(2026,3,8));
 int sum=0;for(const auto& day:days.rows){if(day.date==20260307)assert(day.common==5);if(day.date==20260308)assert(day.common==6);sum+=(int)day.common;}assert(sum==11);
 days.rows[0].date=20260230;assert(!MainUiBattleDaysValid(days));
}
int main(int argc,char** argv){assert(argc==3);testNow=Stamp(2026,10,27);
 if(!strcmp(argv[2],"calendar")){Calendar();return 0;}
 const bool legacy1=!strcmp(argv[2],"legacy1"),legacy2=!strcmp(argv[2],"legacy2"),corrupt=!strcmp(argv[2],"corrupt"),locked=!strcmp(argv[2],"locked");
 if(legacy1)OldHistory<LegacyPersonal>(argv[1],1);if(legacy2)OldHistory<LegacySquad>(argv[1],2);if(corrupt||locked)OldHistory<MainUiBattleRecord>(argv[1],2,corrupt);
 MainUiServicesInit(argv[1]);HANDLE denied=INVALID_HANDLE_VALUE;
 if(locked){wchar_t path[MAX_PATH]{};assert(DataPath(L"history.bin",path));denied=CreateFileW(path,GENERIC_READ,FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);assert(denied!=INVALID_HANDLE_VALUE);}
 MainUiServicesSetWanted(true,false);MainUiServicesWorkerPump();
 if(locked){assert(!historyWritable);Chapter(Stamp(2026,10,27),3,1);CloseHandle(denied);return 0;}
 if(!strcmp(argv[2],"write")){
  Chapter(Stamp(2026,10,24),333,7);
  for(int i=0;i<10;++i)Chapter(Stamp(2026,10,i<5?26:27),10,1);
  Chapter(Stamp(2026,10,27),999,999,1); // Failed chapters remain excluded.
 }else if(!strcmp(argv[2],"roll"))Chapter(Stamp(2026,10,31),20,2);
 else if(legacy1||legacy2||corrupt)Chapter(Stamp(2026,10,27),3,1);
 if(corrupt){assert(!historyWritable);wchar_t path[MAX_PATH]{};assert(DataPath(L"history.bin",path));FILE* f=_wfopen(path,L"rb");assert(f);LegacyFile<MainUiBattleRecord> old{};assert(fread(&old,1,sizeof(old),f)==sizeof(old));fclose(f);assert(old.version==2&&old.reports[0].stats[0]==7);}
 if(!strcmp(argv[2],"write")||!strcmp(argv[2],"read")||!strcmp(argv[2],"roll"))assert(data.battleCount==10);
 Export(!strcmp(argv[2],"roll")?Stamp(2026,10,31):Stamp(2026,10,27));
 MainUiServicesShutdown();puts("PASS: real service history/export fixture");
}
'''


def chart(path, number=0):
    html = path.read_text(encoding='utf-8')
    svg = re.findall(r'<svg\b.*?</svg>', html, re.S)[number]
    return {date: int(value) for value, date in re.findall(
        r'text-anchor=middle>(\d+)</text><text class=axis[^>]*>(\d\d-\d\d)</text>', svg)}


class DailyHistory(unittest.TestCase):
    def test_retention_restart_and_calendar_window(self):
        with tempfile.TemporaryDirectory(prefix='skeeto-daily-history-') as tmp:
            folder = Path(tmp)
            (folder/'fixture.cpp').write_text(FIXTURE, encoding='utf-8')
            vswhere = Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
            (folder/'compile.bat').write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                f'cl /nologo /EHsc /std:c++17 /utf-8 /W3 /D_CRT_SECURE_NO_WARNINGS /I"{CORE}" fixture.cpp /Fe:fixture.exe '
                '/link ws2_32.lib user32.lib shell32.lib ole32.lib comdlg32.lib\nexit /b %errorlevel%\n', encoding='utf-8')
            compile_result = subprocess.run(['cmd','/c','compile.bat'], cwd=folder, capture_output=True, text=True, errors='replace')
            self.assertEqual(compile_result.returncode, 0, compile_result.stdout+compile_result.stderr)
            game = folder/'game'; (game/'left4dead2').mkdir(parents=True)
            report = game/'left4dead2/skeeto/mainmenu/battle-report.html'
            def run(mode, target=game):
                result = subprocess.run([str(folder/'fixture.exe'),str(target),mode], cwd=folder,
                    capture_output=True,text=True,errors='replace',timeout=20)
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            run('write')
            self.assertEqual(chart(report)['10-24'],333,'old day must survive ten newer chapter details')
            self.assertEqual(chart(report)['10-26'],50)
            self.assertEqual(chart(report)['10-27'],50)
            self.assertEqual(chart(report,1)['10-24'],7)
            self.assertEqual(chart(report,1)['10-27'],5)
            run('read') # A new process must load totals exactly once.
            self.assertEqual(chart(report)['10-24'],333)
            self.assertEqual(chart(report)['10-27'],50)
            run('roll') # Oct 24 is now outside the seven-calendar-day window.
            self.assertNotIn('10-24',chart(report))
            self.assertEqual(chart(report)['10-26'],50)
            self.assertEqual(chart(report)['10-31'],20)
            for mode, expected in [('legacy1',10),('legacy2',21)]:
                target=folder/mode;(target/'left4dead2/skeeto/mainmenu').mkdir(parents=True)
                legacy_report=target/'left4dead2/skeeto/mainmenu/battle-report.html'
                run(mode,target)
                self.assertEqual(chart(legacy_report)['10-27'],expected)
                history=target/'left4dead2/skeeto/mainmenu/history.bin'
                self.assertEqual(int.from_bytes(history.read_bytes()[4:8],'little'),4)
                run('read-any',target)
                self.assertEqual(chart(legacy_report)['10-27'],expected,'upgrade/restart must not count detail rows twice')
            target=folder/'corrupt';(target/'left4dead2/skeeto/mainmenu').mkdir(parents=True)
            run('corrupt',target)
            target=folder/'locked';(target/'left4dead2/skeeto/mainmenu').mkdir(parents=True)
            run('locked',target)
            self.assertEqual(int.from_bytes((target/'left4dead2/skeeto/mainmenu/history.bin').read_bytes()[4:8],'little'),2)
            run('calendar')
            print('PASS: >10 clears, personal CI/SI, failure exclusion, restart, v1/v2 upgrade without double count, corrupt-file preservation, month/year/leap/DST boundaries')


if __name__ == '__main__':
    unittest.main()
