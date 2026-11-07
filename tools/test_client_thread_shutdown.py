"""Run production UI/input thread functions without loading or changing L4D2.

Real Win32 threads, queues, windows and stop events; GDI/input effects are counted
stubs. This tests ownership and stop races, not game/driver shutdown acceptance.
"""
from pathlib import Path
import os, subprocess, tempfile, sys

ROOT = Path(__file__).resolve().parents[1]

def function(source, signature):
    at = source.index(signature); start = source.index('{', at); end = start + 1; depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[at:end]

def main():
    source = (ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8')
    fixture = r'''
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static std::atomic_bool g_run{true};
static HANDLE g_clientUiStopEvent=nullptr;
static std::atomic<unsigned> g_clientUiWaitFaults{0};
static std::atomic_bool g_xhairUiRun{true},g_xhairWantShown{true},g_hudLayerWantShown{true},g_hudLayerCanDraw{false};
static HANDLE g_xhairUiTh=nullptr,g_xhairUiReady=nullptr,gate=nullptr;
static DWORD g_xhairUiTid=0;static std::atomic<DWORD> g_xhairUiShutdownWait{WAIT_OBJECT_0};
static HWND g_xhairHwnd=nullptr;
static int g_xhairHudMode=1;static bool g_xhairParked=false,g_xhairOsShown=false,g_xhairParkedOffscreen=false;
static UINT_PTR g_hudLayerRenderRetryTimerId=0;
static std::atomic_bool g_gdiplusOk{false};static ULONG_PTR g_gdiplusToken=1;
static SRWLOCK g_xhairGdiLock=SRWLOCK_INIT;
static constexpr UINT WM_XHAIR_SYNC=WM_APP+21;
static volatile LONG g_hudLayerSyncPending=0;
static std::atomic<unsigned> shutdowns{0},wrongOwner{0},installs{0},unhooks{0},releases{0};
static std::atomic<DWORD> owner{0};
static void require(bool yes,const char* why){if(!yes){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static void Log(const char*,...){}
static void XhairUiDisableIme(){}
namespace Gdiplus {
 static constexpr int Ok=0;struct GdiplusStartupInput {};
 static int GdiplusStartup(ULONG_PTR*,GdiplusStartupInput*,void*){owner=GetCurrentThreadId();return Ok;}
 static void GdiplusShutdown(ULONG_PTR){++shutdowns;if(GetCurrentThreadId()!=owner)++wrongOwner;}
}
static void XhairUiApply(){if(!g_run||!g_xhairUiRun)return;g_xhairHwnd=CreateWindowExW(0,L"STATIC",L"",WS_POPUP,0,0,8,8,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);if(gate)WaitForSingleObject(gate,INFINITE);}
static void XhairUiDestroyWindow(const char*){if(GetCurrentThreadId()!=owner)++wrongOwner;DestroyWindow(g_xhairHwnd);g_xhairHwnd=nullptr;}
static void HudLayerUiDestroyAll(){if(owner&&GetCurrentThreadId()!=owner)++wrongOwner;}
static void HudLayerReleaseFont(){if(owner&&GetCurrentThreadId()!=owner)++wrongOwner;}
static std::atomic_bool g_holdSemiHookRun{false},g_holdSemiHookAlive{false},g_holdSemiPhysDown{false};
static std::atomic<unsigned> g_holdSemiHookEvents{0};
static std::atomic<DWORD> g_holdSemiHookTid{0};
static std::atomic_bool g_holdSemiInjectedDown{false},g_holdSemiReloadDown{false};
static HHOOK g_holdSemiMouseHook=nullptr;static HANDLE g_holdSemiHookThread=nullptr;
static DWORD g_holdSemiShutdownWait=WAIT_OBJECT_0;
static SRWLOCK g_holdSemiInputLock=SRWLOCK_INIT;
static INPUT g_holdSemiReloadInput{};
static HANDLE inputEntered=nullptr,inputGate=nullptr;
static bool HoldSemiResolveReloadInput(INPUT* in){*in={};in->type=INPUT_KEYBOARD;in->ki.wVk='R';return true;}
static UINT FakeInput(UINT,const INPUT* in,int){
 bool up=in->type==INPUT_KEYBOARD?(in->ki.dwFlags&KEYEVENTF_KEYUP)!=0:(in->mi.dwFlags&MOUSEEVENTF_LEFTUP)!=0;
 if(up)++releases;
 else if(inputGate){SetEvent(inputEntered);WaitForSingleObject(inputGate,INFINITE);}
 return 1;
}
#define SendInput FakeInput
static LRESULT CALLBACK HoldSemiMouseProc(int,WPARAM,LPARAM){return 0;}
static HHOOK FakeHook(int,HOOKPROC,HINSTANCE,DWORD){++installs;return reinterpret_cast<HHOOK>(1);}
static BOOL FakeUnhook(HHOOK){++unhooks;return TRUE;}
#define SetWindowsHookExW FakeHook
#define UnhookWindowsHookEx FakeUnhook
'''
    for signature in ('static bool ClientUiNextMessage(', 'static void HoldSemiRequestStop('):
        if signature in source: fixture += function(source,signature)+'\n'
    for signature in ('static bool HoldSemiMakeRelease(', 'static bool HoldSemiInjectMouse(', 'static bool HoldSemiInjectReloadKey(', 'static void HoldSemiReleaseOwnedInput('):
        fixture += function(source,signature)+'\n'
    fixture += function(source,'struct XhairGdiBorrow {')+';\n'
    for signature in ('static void XhairGdiplusInit() {','static void XhairGdiplusShutdown() {'):
        fixture += function(source,signature)+'\n'
    for signature in ('static DWORD WINAPI XhairUiThread(', 'static void XhairShutdown() {',
                      'static DWORD WINAPI HoldSemiHookThreadProc(', 'static void HoldSemiEnsureHookThread() {',
                      'static void HoldSemiShutdownHookThread() {'):
        fixture += function(source,signature)+'\n'
    fixture += r'''
int main(int argc,char** argv){
 require(argc==2,"case argument");g_clientUiStopEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
 const char* mode=argv[1];
 if(!std::strcmp(mode,"reload-only")){
  HoldSemiInjectReloadKey(true);g_run=false;SetEvent(g_clientUiStopEvent);
  HoldSemiReleaseOwnedInput();HoldSemiReleaseOwnedInput();
  require(releases==1&&!g_holdSemiReloadDown,"reload-only stops without a mouse-hook thread and is idempotent");
 }else if(!std::strcmp(mode,"physical-disable")){
  HoldSemiEnsureHookThread();HANDLE observer=nullptr;
  require(DuplicateHandle(GetCurrentProcess(),g_holdSemiHookThread,GetCurrentProcess(),&observer,SYNCHRONIZE,FALSE,0)!=FALSE,"observe held-button stop");
  for(int i=0;i<1000&&!g_holdSemiHookAlive;++i)Sleep(1);
  require(g_holdSemiHookAlive,"held-button owner ready");g_holdSemiPhysDown=true;HoldSemiInjectMouse(true);
  AcquireSRWLockExclusive(&g_holdSemiInputLock);HoldSemiShutdownHookThread();ReleaseSRWLockExclusive(&g_holdSemiInputLock);
  require(WaitForSingleObject(observer,1000)==WAIT_OBJECT_0,"held-button owner stops");CloseHandle(observer);
  require(releases==0,"normal disable preserves physically-held left button");
  require(!g_holdSemiInjectedDown&&!g_holdSemiPhysDown,"physical hold takes ownership before state clears");
  HoldSemiReleaseOwnedInput();require(releases==0,"later worker cleanup does not release handed-back input");
  HoldSemiEnsureHookThread();observer=nullptr;
  require(DuplicateHandle(GetCurrentProcess(),g_holdSemiHookThread,GetCurrentProcess(),&observer,SYNCHRONIZE,FALSE,0)!=FALSE,"observe restart");
  for(int i=0;i<1000&&!g_holdSemiHookAlive;++i)Sleep(1);
  require(g_holdSemiHookAlive&&installs==2,"feature restarts after held-button disable");HoldSemiInjectMouse(true);
  HoldSemiShutdownHookThread();require(WaitForSingleObject(observer,1000)==WAIT_OBJECT_0,"restarted owner stops");CloseHandle(observer);
  require(releases==1&&!g_holdSemiInjectedDown,"unheld synthetic input still releases on next cycle");
 }else if(!std::strcmp(mode,"input-cycle")){
  DWORD steady=0;
  for(int cycle=0;cycle<32;++cycle){
   HoldSemiEnsureHookThread();HANDLE observer=nullptr;
   require(DuplicateHandle(GetCurrentProcess(),g_holdSemiHookThread,GetCurrentProcess(),&observer,SYNCHRONIZE,FALSE,0)!=FALSE,"observe hook thread");
   for(int i=0;i<1000&&!g_holdSemiHookAlive;++i)Sleep(1);
   require(g_holdSemiHookAlive,"cycled thread ready");HoldSemiShutdownHookThread();
   require(WaitForSingleObject(observer,1000)==WAIT_OBJECT_0,"cycled thread stops");
   HoldSemiShutdownHookThread();CloseHandle(observer);require(!g_holdSemiHookThread,"finished handle reaped");
   DWORD count=0;GetProcessHandleCount(GetCurrentProcess(),&count);
   if(cycle==0)steady=count;else require(count<=steady+1,"toggle cycles do not accumulate thread handles");
  }
 }else if(std::strncmp(mode,"input",5)==0){
  g_holdSemiHookRun=true;
  if(!std::strcmp(mode,"input-late")){
   g_holdSemiHookThread=CreateThread(nullptr,0,HoldSemiHookThreadProc,nullptr,CREATE_SUSPENDED,nullptr);
   HANDLE thread=g_holdSemiHookThread;HoldSemiShutdownHookThread();ResumeThread(thread);
   require(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0,"stop before queue publication is not lost");
   require(installs==0,"cancelled input startup never installs a hook");
  }else{
   HoldSemiEnsureHookThread();HANDLE thread=g_holdSemiHookThread;
   for(int i=0;i<1000&&!g_holdSemiHookAlive;++i)Sleep(1);
   require(g_holdSemiHookAlive,"input thread ready");
   HANDLE sender=nullptr;
   if(!std::strcmp(mode,"input-race")){
    inputEntered=CreateEventW(nullptr,TRUE,FALSE,nullptr);inputGate=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    sender=CreateThread(nullptr,0,[](void*)->DWORD{HoldSemiInjectMouse(true);return 0;},nullptr,0,nullptr);
    require(WaitForSingleObject(inputEntered,1000)==WAIT_OBJECT_0,"synthetic press is in flight");
   }else{HoldSemiInjectMouse(true);HoldSemiInjectReloadKey(true);}
   g_run=false;SetEvent(g_clientUiStopEvent);
   if(sender){SetEvent(inputGate);require(WaitForSingleObject(sender,1000)==WAIT_OBJECT_0,"sender completes");}
   require(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0,"input exits without Paint/WM_QUIT");
   require(unhooks==1&&releases==unsigned(sender?1:2)&&!g_holdSemiInjectedDown&&!g_holdSemiReloadDown,"owner unhooks and releases outstanding synthetic input");
   require(!HoldSemiInjectMouse(true)&&!HoldSemiInjectReloadKey(true),"no synthetic press after shutdown");
  }
 }else{
  g_xhairUiReady=CreateEventW(nullptr,TRUE,FALSE,nullptr);
  if(!std::strcmp(mode,"timeout"))gate=CreateEventW(nullptr,TRUE,FALSE,nullptr);
  if(!std::strcmp(mode,"late")){g_run=false;SetEvent(g_clientUiStopEvent);}
  g_xhairUiTh=CreateThread(nullptr,0,XhairUiThread,nullptr,0,&g_xhairUiTid);HANDLE thread=g_xhairUiTh;
  require(WaitForSingleObject(g_xhairUiReady,1000)==WAIT_OBJECT_0,"UI ready");
  HANDLE borrower=nullptr;
  if(!std::strcmp(mode,"bake")){
   inputEntered=CreateEventW(nullptr,TRUE,FALSE,nullptr);inputGate=CreateEventW(nullptr,TRUE,FALSE,nullptr);
   borrower=CreateThread(nullptr,0,[](void*)->DWORD{XhairGdiBorrow borrow;require(borrow.live,"live HUD bake borrows session");SetEvent(inputEntered);WaitForSingleObject(inputGate,INFINITE);return 0;},nullptr,0,nullptr);
   require(WaitForSingleObject(inputEntered,1000)==WAIT_OBJECT_0,"HUD bake is in flight");
   g_run=false;SetEvent(g_clientUiStopEvent);XhairShutdown();
   require(shutdowns==0&&g_xhairUiTh==thread,"GDI waits for outstanding bake without freeing its resources");
   SetEvent(inputGate);WaitForSingleObject(borrower,1000);require(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0,"owner completes after bake releases");
  }else if(!std::strcmp(mode,"event")||!std::strcmp(mode,"late")){
   g_run=false;SetEvent(g_clientUiStopEvent);require(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0,"UI stop wakes idle GetMessage without Paint");
  }else if(!std::strcmp(mode,"timeout")){
   Sleep(20);XhairShutdown();require(shutdowns==0&&g_xhairUiTh==thread,"timeout keeps live thread resources owned");
   SetEvent(gate);require(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0,"blocked owner finishes after release");
  }else XhairShutdown();
  require(WaitForSingleObject(thread,1000)==WAIT_OBJECT_0||g_xhairUiTh==nullptr,"UI exits");
  require(shutdowns==unsigned(!std::strcmp(mode,"late")?0:1)&&wrongOwner==0&&g_xhairHwnd==nullptr,"windows/fonts/GDI released safely exactly once");
 }
 std::printf("PASS %s\n",mode);return 0;
}
'''
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-thread-stop-') as tmp:
        folder=Path(tmp);(folder/'fixture.cpp').write_text(fixture,encoding='utf-8')
        (folder/'build.bat').write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 /W3 fixture.cpp user32.lib /Fe:fixture.exe\n',encoding='utf-8')
        subprocess.run(['cmd','/c','build.bat'],cwd=folder,check=True)
        for case in sys.argv[1:] or ['owner','event','late','timeout','bake','input-late','input-event','input-race','input-cycle','reload-only','physical-disable']:
            subprocess.run([str(folder/'fixture.exe'),case],check=True,timeout=8)

if __name__=='__main__':main()
