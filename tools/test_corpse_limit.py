"""Compile the shared global corpse-limit owner against mutable CVar fixtures."""
from pathlib import Path
import os
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[1]
    source = (root / 'src/skeeto/core/skeeto_proxy.cpp').read_text(encoding='utf-8-sig')
    helper = source.split('// Shared global corpse limit ownership begin', 1)[1].split('// Shared global corpse limit ownership end', 1)[0]
    fixture = r'''#include <Windows.h>
#include <cstdio>
#include <cstdlib>
static bool g_optClientNoGroundCorpses=false,g_optClientNoCorpseSi=false,g_optClientNoCorpseCi=false;
static bool features=true;
static int value=24,writes=0,finds=0;
static struct { bool haveAll=false; int all=0; } g_ragdollSnap;
static constexpr int kFCvarCheat=1,kFCvarDevOnly=2,kFCvarHidden=4,kFCvarClientCmdCanExecute=8;
static bool SkeetoFeaturesOn(){return features;}
static void* CvarFind(const char*){++finds;return &value;}
static bool CvarReadPair(void*,int* n,float* f){*n=value;*f=float(value);return true;}
static void ClientUxCvarWriteInt(const char*,int n){value=n;++writes;}
static void UnlockConCommand(const char*,int,int){}
static void require(bool ok,const char* message){if(!ok){puts(message);exit(1);}}
// PRODUCTION
int main(){
 ClientUxApplyCorpseLimit();require(finds==0&&writes==0,"closed path does not query or write");
 g_optClientNoGroundCorpses=true;ClientUxApplyCorpseLimit();require(value==0&&g_ragdollSnap.all==24,"captures exact previous limit");
 g_optClientNoCorpseSi=true;ClientUxApplyCorpseLimit();
 g_optClientNoGroundCorpses=false;ClientUxApplyCorpseLimit();require(value==0&&g_ragdollSnap.all==24,"other owner keeps zero and original snapshot");
 g_optClientNoCorpseSi=false;ClientUxApplyCorpseLimit();require(value==24&&!g_ragdollSnap.haveAll,"last owner restores original value");
 value=75;g_optClientNoGroundCorpses=true;ClientUxApplyCorpseLimit();
 features=false;ClientUxApplyCorpseLimit();require(value==75&&!g_ragdollSnap.haveAll,"master disabled restores current session baseline");
 puts("PASS shared corpse limit: off state, exact restore, overlapping options and master disable");
}'''.replace('// PRODUCTION', helper)
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.check_output([str(vswhere), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-corpse-limit-') as tmp:
        folder = Path(tmp)
        (folder / 'fixture.cpp').write_text(fixture, encoding='utf-8')
        (folder / 'build.bat').write_text('@echo off\ncall "'+vs+'\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\ncl /nologo /O2 /MT /EHsc /std:c++17 /utf-8 fixture.cpp /Fe:fixture.exe\nif errorlevel 1 exit /b 1\nfixture.exe\n', encoding='utf-8')
        subprocess.run(['cmd', '/c', 'build.bat'], cwd=folder, check=True)


if __name__ == '__main__':
    main()
