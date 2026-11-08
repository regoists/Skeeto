"""Check production network lookup never guesses unrelated engine ABI slots."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
def function(s,signature):
    at=s.index(signature);start=s.index('{',at);end=start+1;depth=1
    while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
    return s[at:end]
def main():
    source=(ROOT/'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8')
    fixture=r'''
#include <windows.h>
#include <cassert>
#include <cstdio>
static int object=0;static void* g_engine=&object;static void* result=nullptr;
static bool connected=true;static int calls=0,slots=0,g_idxNetChan=-1;
static constexpr int kEngineNetChanInfoSlot=74;
static bool IfaceAlive(void* p){return p!=nullptr;}
static bool EngConnected(){return connected;}
static bool IsExec(void* p){return p!=nullptr;}
static bool ListenPtrLooksObject(void* p){return p==&object;}
static void* __fastcall getter(void*,void*){++calls;return result;}
static void* VGet(void*,int index){++slots;assert(index==74);return (void*)&getter;}
static const char* ListenChanAddr(void*){return "loopback";}
static bool ListenChanCallIsLoopback(void*){return true;}
static float ListenChanLatency(void*){return 0;}
static void Log(const char*,...){}
static void* EngNetChanFromServer(){assert(false);return nullptr;}
'''
    fixture+=function(source,'static void* EngNetChanFromClient() {')+'\n'
    fixture+=function(source,'static void* EngNetChan() {')+'\n'
    fixture+=r'''
int main(){
 // playdemo: connected client, no real channel. Must not call SaveAllocMemory,
 // DebugDrawPhysCollide, or guessed server interfaces as a fallback.
 assert(EngNetChan()==nullptr&&calls==1&&slots==1);
 result=(void*)0x1c;assert(EngNetChan()==nullptr&&calls==2&&slots==2);
 result=&object;assert(EngNetChan()==&object&&calls==3&&slots==3);
 connected=false;assert(EngNetChan()==nullptr&&calls==3);
 connected=true;g_engine=nullptr;assert(EngNetChan()==nullptr&&calls==3);
 puts("PASS channel lookup: demo/no channel, junk result, live channel, disconnect; exact slot 74 only");
}
'''
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-nc-') as tmp:
        folder=Path(tmp);(folder/'fixture.cpp').write_text(fixture,encoding='utf-8')
        (folder/'build.bat').write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n',encoding='utf-8')
        subprocess.run(['cmd','/c','build.bat'],cwd=folder,check=True)
if __name__=='__main__':main()
