"""Exercise actual playback/queue/presentation controls on MSVC x86.

This checks the clock and request contract, not in-game audiovisual acceptance.
"""
import os
from pathlib import Path
import subprocess
import tempfile
from test_chapstats_pages import ROOT, block


def run():
    cpp = (ROOT / 'src/skeeto/core/skeeto_background.cpp').read_text(encoding='utf-8-sig')
    editor = (ROOT / 'src/skeeto/core/skeeto_mainui_editor.h').read_text(encoding='utf-8-sig')
    proxy = (ROOT / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    source = r'''
#include <Windows.h>
#include <atomic>
#include <vector>
#include <utility>
#include <cassert>
#include <cstdio>
#include "skeeto_background.h"
struct DlcCacheExtract {};
static void DlcCacheExtractCancel(DlcCacheExtract*) {}
static void BgLog(const char*,...) {}
'''
    source += cpp[cpp.index('constexpr int kAssetMax'):cpp.index('static void BgLog(const char* fmt')]
    source += r'''
static LONGLONG clockNow=100000000;
static LONGLONG VideoClock100ns(){return clockNow;}
static int type=SKEETO_BG_VIDEO_MP4;
int SkeetoBackgroundCurrentType(){return type;}
int SkeetoBackgroundFitMode(){return 0;}
'''
    for pattern in [r'^static void ClearFrames\(', r'^static unsigned int PlaybackSeekRequest\(',
                    r'^static bool QueueVideoFrame\(', r'^SkeetoBackgroundPlayback SkeetoBackgroundGetPlayback\(',
                    r'^bool SkeetoBackgroundSetPaused\(', r'^bool SkeetoBackgroundSeek\(',
                    r'^bool SkeetoBackgroundAcquireFrame\(']:
        source += block(cpp, pattern)
    source += r'''
struct MainUiLayoutSlot{int id,action,parent,visible;};
static MainUiLayoutSlot g_mainUiLayout[]={{709,929,-1,1},{710,928,-1,1},{711,929,800,1}};
static int g_mainUiLayoutCount=3,g_mainUiPanel=0,g_mainUiFlyout=-1,g_mainUiCustomPage=0,g_mainUiCustomMenu=800;
static bool g_mainUiEditing=false;
static int MainUiCustomPageSize(){return 9;}
static MainUiLayoutSlot* MainUiLayoutFind(int id){for(auto& b:g_mainUiLayout)if(b.id==id)return &b;return nullptr;}
struct Button{int x0,y0,x1,y1,action;};static Button g_mainUiButtons[]={{0,0,240,46,709},{0,50,240,96,300}};
static int g_mainUiButtonCount=2,g_mainUiPlaybackDragId=-1;static double g_mainUiPlaybackFraction=0;
static std::atomic_int g_backgroundPauseRequest{-1};
static int echoes=0;
static void ConEcho(const char*){++echoes;}
static int CmdArgC(const int* cmd){return cmd[0];}
static bool SkCmdParseBoundedAt(const int* cmd,int,int lo,int hi,int* out){if(cmd[1]<lo||cmd[1]>hi)return false;*out=cmd[1];return true;}
'''
    for name in ['MainUiPlaybackSlider', 'MainUiPlaybackDrag', 'MainUiPlaybackStep']:
        source += block(editor, r'^static (?:bool|void) ' + name + r'\(')
    source += block(proxy, r'^static void SkCmd_BackgroundPause\(')
    source += r'''
static unsigned serial=0;
static void queue(LONGLONG media,LONGLONG src){std::vector<unsigned char> pixels(16,1);assert(QueueVideoFrame(pixels,2,2,media,src,g_seekEpoch,g_generation,&serial,true,12));}
static void acquire(){SkeetoBackgroundFrame frame{};assert(SkeetoBackgroundAcquireFrame(&frame));assert(frame.width==2&&frame.height==2);}
int main(){
 InitializeCriticalSection(&g_lock);g_lockReady=true;g_enabled=true;g_menuActive=true;
 g_wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);g_audioWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 g_playbackDuration=600000000;g_playbackSeekable=true;
 queue(0,0);acquire();queue(1000000,1000000);queue(2000000,2000000);
 const auto serialBefore=g_render.serial;assert(SkeetoBackgroundSetPaused(true));
 const auto base=g_videoClockBase;clockNow+=300000000;acquire();assert(g_render.serial==serialBefore);
 assert(SkeetoBackgroundGetPlayback().paused);assert(SkeetoBackgroundSetPaused(true));
 assert(SkeetoBackgroundSetPaused(false));assert(g_videoClockBase==base+300000000);
 acquire();assert(g_render.serial==serialBefore);clockNow+=1000000;acquire();assert(g_render.sourceTime==1000000);
 // Latest seek wins; queued and in-flight frames from the previous epoch cannot leak.
 assert(SkeetoBackgroundSetPaused(true));unsigned oldEpoch=g_seekEpoch;
 assert(SkeetoBackgroundSeek(200000000));assert(SkeetoBackgroundSeek(300000000));
 LONGLONG target=0;assert(PlaybackSeekRequest(&target)==g_seekEpoch&&target==300000000);
 assert(g_videoQueueCount==0&&g_videoClockBase==0);
 std::vector<unsigned char> stale(16,1);assert(!QueueVideoFrame(stale,2,2,0,0,oldEpoch,g_generation,&serial,true,12));
 queue(0,target);acquire();assert(g_render.sourceTime==target&&SkeetoBackgroundGetPlayback().paused);
 queue(1000000,target+1000000);clockNow+=100000000;acquire();assert(g_render.sourceTime==target);
 assert(SkeetoBackgroundSetPaused(false));clockNow+=1000000;acquire();assert(g_render.sourceTime==target+1000000);
 assert(SkeetoBackgroundSeek(-999));PlaybackSeekRequest(&target);assert(target==0);
 assert(SkeetoBackgroundSeek(999999999));PlaybackSeekRequest(&target);assert(target==599000000);
 // Non-MP4/inactive/disabled/unsupported controls are inert.
 type=SKEETO_BG_IMAGE;assert(!SkeetoBackgroundSetPaused(true)&&!SkeetoBackgroundSeek(1));type=SKEETO_BG_VIDEO_BIK;assert(!SkeetoBackgroundGetPlayback().available);type=SKEETO_BG_VIDEO_MP4;
 g_menuActive=false;assert(!SkeetoBackgroundSetPaused(true));g_menuActive=true;
 g_enabled=false;assert(!SkeetoBackgroundSetPaused(true));g_enabled=true;
 g_playbackSeekable=false;assert(!SkeetoBackgroundSeek(1));g_playbackSeekable=true;
 g_loadFailed=true;assert(!SkeetoBackgroundGetPlayback().available);g_loadFailed=false;
 const unsigned workerEpoch=g_seekEpoch;ClearFrames(false);assert(g_seekEpoch!=workerEpoch&&g_seekTarget==0&&g_playbackDuration==600000000);
 assert(SkeetoBackgroundSeek(400000000));assert(g_seekEpoch!=workerEpoch);PlaybackSeekRequest(&target);assert(target==400000000);
 // Production progress input: previews do not seek; a release submits once.
 assert(MainUiPlaybackSlider(709)&&!MainUiPlaybackSlider(710));const unsigned beforeDrag=g_seekEpoch;
 for(int x=0;x<300;x+=10)MainUiPlaybackDrag(709,x,false);
 assert(g_seekEpoch==beforeDrag&&g_mainUiPlaybackDragId==709&&g_mainUiPlaybackFraction==1);
 MainUiPlaybackDrag(709,120,true);assert(g_seekEpoch==beforeDrag+1&&g_mainUiPlaybackDragId==-1);PlaybackSeekRequest(&target);assert(target==300000000);
 g_render.sourceTime=100000000;MainUiPlaybackStep(709,1);PlaybackSeekRequest(&target);assert(target==150000000);
 MainUiPlaybackStep(709,-1);PlaybackSeekRequest(&target);assert(target==50000000);
 g_mainUiFlyout=12;assert(MainUiPlaybackSlider(300));g_mainUiEditing=true;assert(!MainUiPlaybackSlider(300));g_mainUiEditing=false;
 g_mainUiPanel=1;assert(MainUiPlaybackSlider(929)&&!MainUiPlaybackSlider(928)&&!MainUiPlaybackSlider(300));g_mainUiPanel=2;assert(!MainUiPlaybackSlider(929));g_mainUiPanel=0;
 g_mainUiLayout[2].visible=0;assert(!MainUiPlaybackSlider(300));g_mainUiLayout[2].visible=1;g_mainUiFlyout=-1;
 g_playbackSeekable=false;const unsigned unavailable=g_seekEpoch;MainUiPlaybackDrag(709,240,true);assert(g_seekEpoch==unavailable);g_playbackSeekable=true;
 // Console callback only publishes bounded intent, without touching playback.
 int command[]={1,0};SkCmd_BackgroundPause(command);assert(g_backgroundPauseRequest==2);
 command[0]=2;SkCmd_BackgroundPause(command);assert(g_backgroundPauseRequest==0);command[1]=1;SkCmd_BackgroundPause(command);assert(g_backgroundPauseRequest==1);
 command[1]=2;SkCmd_BackgroundPause(command);assert(g_backgroundPauseRequest==1&&echoes==1);command[0]=3;SkCmd_BackgroundPause(command);assert(echoes==2);
 ClearFrames();assert(!g_playbackPaused&&g_seekEpoch==0&&g_videoQueueCount==0&&g_playbackDuration==0);
 CloseHandle(g_wake);CloseHandle(g_audioWake);DeleteCriticalSection(&g_lock);
 puts("PASS: pause clock, idempotence, resume cadence, seek coalescing, stale-frame rejection, paused preview, bounds and lifecycle gates");
}
'''
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires',
                                  'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-playback-') as temp:
        folder = Path(temp)
        (folder / 'fixture.cpp').write_text(source, encoding='utf-8')
        include = ROOT / 'src/skeeto/core'
        (folder / 'run.bat').write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\n'
                                     f'cl /nologo /EHsc /std:c++17 /utf-8 /W4 /wd4505 /wd4189 /wd4101 /I"{include}" fixture.cpp /Fe:fixture.exe\n'
                                     'if errorlevel 1 exit /b 1\nfixture.exe\nexit /b %errorlevel%\n', encoding='utf-8')
        subprocess.run(['cmd', '/c', 'run.bat'], cwd=folder, check=True)


if __name__ == '__main__':
    run()
