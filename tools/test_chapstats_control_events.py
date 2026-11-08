"""Execute production chapter control-event dispatch and score-toast updates."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from test_chapstats_pages import ROOT, block

HARNESS = r'''
static bool g_optChapStats=true,toastWanted=true;
static ChapStatsRec records[33]{};
struct Event { int userid,victim,rescuer; };
static int EvInt(void* event,const char* name,int fallback){
 auto& e=*(Event*)event;
 if(!strcmp(name,"userid"))return e.userid;
 if(!strcmp(name,"victim"))return e.victim;
 if(!strcmp(name,"rescuer"))return e.rescuer;
 return fallback;
}
static ChapStatsRec* ChapStatsRecFor(int uid){return uid>0&&uid<33?&records[uid]:nullptr;}
static bool ChapStatsScoreHudCaptureWanted(int){return toastWanted;}
static int ChapStatsScoreTotal(const ChapStatsRec& r){return -100*r.st[kCsPinned]+50*r.st[kCsRescue];}
static int toastCount=0,lastKind=-1,lastDetail=-1,lastBefore=0,lastAfter=0;
static void ChapStatsScoreHudPush(int,int before,int after,int kind,int,int detail){
 ++toastCount;lastKind=kind;lastDetail=detail;lastBefore=before;lastAfter=after;
}
'''

FIXTURE = r'''
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 Event e{10,1,2};
 // Collision/carry may end with a self-kill or a teammate kill: no pin debit.
 ControlEvent("charger_impact",&e);ControlEvent("charger_carry_start",&e);
 CHECK(records[1].st[kCsPinned]==0&&toastCount==0);
 ControlEvent("charger_carry_end",&e);
 CHECK(records[1].st[kCsPinned]==0&&toastCount==0);
 // A real carry -> pummel episode charges only at the pummel event.
 ControlEvent("charger_carry_start",&e);ControlEvent("charger_pummel_start",&e);
 CHECK(records[1].st[kCsPinned]==1&&toastCount==1);
 CHECK(lastKind==kCsToastPenalty&&lastDetail==kCsDetailChargerPummel);
 CHECK(lastBefore==0&&lastAfter==-100);
 ControlEvent("charger_pummel_end",&e);
 CHECK(records[1].st[kCsPinned]==1&&records[2].st[kCsRescue]==1);
 // Another real pummel by the same Charger is a new pin, with no time window.
 ControlEvent("charger_pummel_start",&e);
 CHECK(records[1].st[kCsPinned]==2&&lastBefore==-100&&lastAfter==-200);
 // A pummel with no observed carry (e.g. stationary grab) still counts.
 e.victim=3;ControlEvent("charger_pummel_start",&e);
 CHECK(records[3].st[kCsPinned]==1);
 // No rescuer / self-rescue must not grant teammate support credit.
 e.rescuer=3;ControlEvent("charger_pummel_end",&e);
 CHECK(records[3].st[kCsRescue]==0);
 e.rescuer=0;ControlEvent("charger_pummel_end",&e);
 CHECK(records[3].st[kCsRescue]==0);
 // Statistics work without score popups, and stay inactive while disabled.
 toastWanted=false;e.victim=4;int before=toastCount;
 ControlEvent("charger_pummel_start",&e);
 CHECK(records[4].st[kCsPinned]==1&&toastCount==before);
 g_optChapStats=false;ControlEvent("charger_pummel_start",&e);
 CHECK(records[4].st[kCsPinned]==1);
 // Keep the existing Smoker criterion and penalty detail.
 g_optChapStats=true;toastWanted=true;e.victim=5;
 ControlEvent("choke_start",&e);
 CHECK(records[5].st[kCsPinned]==1&&lastDetail==kCsDetailSmoker);
 return 0;
}
'''


class ChapterControlEvents(unittest.TestCase):
    def test_carry_pummel_release_and_score_toasts(self):
        source = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
        code = '#include <windows.h>\n#include <cstdint>\n#include <cstdio>\n#include <cstring>\n'
        for pattern in (r'^enum \{\s*\n\s*kCsCi', r'^struct ChapStatsRec \{',
                r'^enum ChapStatsScoreToastKind', r'^enum ChapStatsScoreDetail'):
            code += block(source, pattern) + ';\n'
        code += HARNESS + block(source, r'^static void ChapStatsBump\(')
        dispatch = block(source, r'^static void ChapStatsOnEvent\(const char\* n, void\* ev\) \{')
        prefix = dispatch[:dispatch.index("\tcase 'i':")].replace('ChapStatsOnEvent(', 'ControlEvent(')
        charger = dispatch[dispatch.index("\tcase 'c':"):dispatch.index("\tcase 'w':")]
        code += prefix + charger + '\n}\n}\n' + FIXTURE
        with tempfile.TemporaryDirectory(prefix='skeeto-control-score-') as directory:
            folder = Path(directory)
            (folder / 'test.cpp').write_text(code, encoding='utf-8')
            vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
            vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
            (folder / 'run.bat').write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                'cl /nologo /EHsc /std:c++17 /utf-8 test.cpp /Fe:test.exe\n'
                'if errorlevel 1 exit /b 1\ntest.exe\nexit /b %errorlevel%\n', encoding='utf-8')
            result = subprocess.run(['cmd', '/c', str(folder / 'run.bat')], cwd=folder,
                capture_output=True, text=True, errors='replace')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
