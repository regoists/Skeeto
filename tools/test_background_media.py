"""Optional live MF/waveOut fixture, using a temporary silent FFmpeg MP4.

Runs the full production background module without Source/VGUI or game files.
Requires an existing ffmpeg on PATH; installs nothing.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from test_chapstats_pages import ROOT


def run():
    ffmpeg = shutil.which('ffmpeg')
    if not ffmpeg:
        raise RuntimeError('ffmpeg is required for this optional media test')
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                  'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    core = ROOT / 'src/skeeto/core'
    with tempfile.TemporaryDirectory(prefix='skeeto-media-') as temp:
        folder = Path(temp)
        media = folder / 'skeeto/backgrounds/test.mp4'
        media.parent.mkdir(parents=True)
        subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-f', 'lavfi', '-i',
                        'testsrc2=size=160x90:rate=30', '-f', 'lavfi', '-i', 'anullsrc=r=48000:cl=stereo',
                        '-t', '6', '-c:v', 'libx264', '-g', '30', '-pix_fmt', 'yuv420p', '-c:a', 'aac',
                        '-movflags', '+faststart', str(media)], check=True)
        source = r'''
#include <Windows.h>
#include <mmsystem.h>
#include <atomic>
#include <cassert>
#include <cstdio>
static std::atomic_int pauses{0},restarts{0},resets{0};
static MMRESULT TestPause(HWAVEOUT out){++pauses;return waveOutPause(out);}
static MMRESULT TestRestart(HWAVEOUT out){++restarts;return waveOutRestart(out);}
static MMRESULT TestReset(HWAVEOUT out){++resets;return waveOutReset(out);}
#define waveOutPause TestPause
#define waveOutRestart TestRestart
#define waveOutReset TestReset
#include "skeeto_background.cpp"
#undef waveOutPause
#undef waveOutRestart
#undef waveOutReset
// Only Source filesystem calls are stubbed. MF, QPC, events, workers and waveOut
// below are real Windows implementations; the video uses a temporary loose path.
bool DlcGameFileExists(const char*){return false;}
int DlcForEachGameGlob(const char*,void(*)(const char*,void*),void*){return 0;}
bool DlcReadGameBinaryLimited(const char*,const char*,void**,int*,unsigned){return false;}
DlcCacheExtract* DlcCacheExtractBegin(const char*,const wchar_t*,unsigned){return nullptr;}
int DlcCacheExtractPump(DlcCacheExtract*,unsigned,wchar_t*,size_t){return -1;}
void DlcCacheExtractCancel(DlcCacheExtract*){}
void DlcCacheExtractAbandon(DlcCacheExtract*){}
DlcGameGlobScan* DlcGameGlobScanBegin(const char*){return nullptr;}
int DlcGameGlobScanPump(DlcGameGlobScan*,int,void(*)(const char*,void*),void*){return -1;}
void DlcGameGlobScanCancel(DlcGameGlobScan*){}
void DlcGameGlobScanAbandon(DlcGameGlobScan*){}
static void log(const char* text){puts(text);}
static void paint(){SkeetoBackgroundFrame frame{};SkeetoBackgroundAcquireFrame(&frame);}
static bool waitPosition(LONGLONG min,LONGLONG max){for(int i=0;i<300;++i){paint();const auto s=SkeetoBackgroundGetPlayback();if(s.available&&SkeetoBackgroundHasFrame()&&s.position>=min&&s.position<=max)return true;Sleep(10);}return false;}
static void advance(int ms){for(int i=0;i<ms/10;++i){paint();Sleep(10);}}
int main(int argc,char** argv){
 assert(argc==2);SkeetoBackgroundInitialize(argv[1],log);SkeetoBackgroundSetVolume(0);
 SkeetoBackgroundSetEnabled(true);SkeetoBackgroundSetMenuActive(true);assert(SkeetoBackgroundScan());
 assert(waitPosition(1000000,20000000));auto s=SkeetoBackgroundGetPlayback();assert(s.seekable&&s.duration>=59000000&&s.duration<=61000000);
 assert(SkeetoBackgroundSetPaused(true));advance(100);auto pos=SkeetoBackgroundGetPlayback().position;const int restartBefore=restarts;
 advance(300);assert(SkeetoBackgroundGetPlayback().position==pos&&restarts==restartBefore);
 assert(SkeetoBackgroundSeek(40000000));assert(waitPosition(40000000,41000000));advance(200);assert(SkeetoBackgroundGetPlayback().position<41000000&&SkeetoBackgroundGetPlayback().paused);
 assert(SkeetoBackgroundSetPaused(false));assert(waitPosition(42000000,50000000));
 assert(SkeetoBackgroundSeek(10000000));assert(waitPosition(10000000,13000000));
 assert(SkeetoBackgroundSeek(20000000));assert(SkeetoBackgroundSeek(30000000));assert(SkeetoBackgroundSeek(50000000));assert(waitPosition(50000000,53000000));
 advance(1600);assert(SkeetoBackgroundGetPlayback().position<20000000); // loop source time
 assert(SkeetoBackgroundSetPaused(true));SkeetoBackgroundSetMenuActive(false);assert(!SkeetoBackgroundGetPlayback().available);
 Sleep(100);SkeetoBackgroundSetMenuActive(true);assert(waitPosition(0,10000000));assert(!SkeetoBackgroundGetPlayback().paused);
 // Keep both workers outside the frame lock across a rapid exit/reentry/seek.
 EnterCriticalSection(&g_lock);SkeetoBackgroundSetMenuActive(false);SkeetoBackgroundSetMenuActive(true);assert(SkeetoBackgroundSeek(30000000));LeaveCriticalSection(&g_lock);
 assert(waitPosition(30000000,33000000));
 assert(SkeetoBackgroundSetPaused(true));SkeetoBackgroundSelect("test.mp4");assert(waitPosition(0,10000000));assert(!SkeetoBackgroundGetPlayback().paused);
 assert(SkeetoBackgroundSetPaused(true));const DWORD at=GetTickCount();SkeetoBackgroundShutdown();assert(GetTickCount()-at<3000);
 printf("PASS: real MF MP4 metadata/pause/resume/seek/loop/menu reentry/asset reload/paused shutdown; waveOut pause=%d restart=%d reset=%d\n",pauses.load(),restarts.load(),resets.load());
}
'''
        (folder / 'fixture.cpp').write_text(source, encoding='utf-8')
        (folder / 'run.bat').write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                                     f'cl /nologo /EHsc /std:c++17 /utf-8 /D_CRT_SECURE_NO_WARNINGS /W3 /I"{core}" fixture.cpp /Fe:fixture.exe '
                                     '/link windowscodecs.lib mfplat.lib mfreadwrite.lib mfuuid.lib propsys.lib winmm.lib ole32.lib shell32.lib\n'
                                     'if errorlevel 1 exit /b 1\nfixture.exe "%CD%"\nexit /b %errorlevel%\n', encoding='utf-8')
        subprocess.run(['cmd', '/c', 'run.bat'], cwd=folder, check=True, timeout=90)


if __name__ == '__main__':
    run()
