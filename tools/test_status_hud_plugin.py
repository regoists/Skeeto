"""Exercise the real Status HUD SDK consumer, settings and runtime helpers."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
HOST=r'''
#include "skeeto_plugins.h"
#include <atomic>
#include <cassert>
#include <cwchar>
#include <cstdio>
std::atomic_bool g_run{true};
int wmain(int argc,wchar_t** argv){
 assert(argc==3);const int mode=_wtoi(argv[2]);
 skplugins::Start(argv[1],mode!=0);
 if(!mode){assert(!skplugins::Started() && !GetModuleHandleW(L"Skeeto_StatusHUD.dll"));return 0;}
 assert(skplugins::Count()==1 && skplugins::At(0)->state==skplugins::State::Loaded);
 skplugins::PaintPump(false);assert(skplugins::At(0)->state==skplugins::State::Active);
 assert(skplugins::RequestPage(0,0));skplugins::PaintPump(false);assert(skplugins::CurrentPage().count==7);
 assert(!strcmp(skplugins::At(0)->id,"skeeto.status_hud"));
 assert(skplugins::RequestPage(0,1));skplugins::PaintPump(false);
 const auto& page=skplugins::CurrentPage();assert(page.count==7);
 assert(page.items[0].type==SKT_MENU_TOGGLE && page.items[0].value==0);
 assert(page.items[4].value==(mode==2?23:18));
 if(mode==2){assert(!skplugins::NeedsPaint());return 0;}
 // Numeric select/adjust and actual save callbacks go through the real host.
 assert(skplugins::RequestAction(104,23));skplugins::PaintPump(false);assert(page.items[4].value==23);
 assert(skplugins::RequestAction(100,1));skplugins::PaintPump(false);
 assert(skplugins::At(0)->state==skplugins::State::Active && page.items[0].value==0); // no Source DLLs: rejected visibly, menu remains usable
 assert(skplugins::RequestPage(0,2));skplugins::PaintPump(true);assert(page.count==7 && !wcscmp(page.title,L"Memory HUD"));
 assert(skplugins::RequestPage(0,0));skplugins::PaintPump(true);
 assert(skplugins::RequestAction(11,0));skplugins::PaintPump(true);
 wchar_t path[2048]{};swprintf_s(path,L"%s\\left4dead2\\skeeto\\plugins\\skeeto.status_hud\\settings.ini",argv[1]);
 bool saved=false;for(int i=0;i<100;++i){FILE* f=nullptr;_wfopen_s(&f,path,L"rb");if(f){char text[512]{};fread(text,1,sizeof(text)-1,f);fclose(f);if(strstr(text,"net_font=23")){saved=true;break;}}Sleep(20);}
 assert(saved);return 0;
}
'''

def main():
    plugin=ROOT/'extras/Skeeto_StatusHUD'
    assert (plugin/'src/Skeeto_StatusHUD.cpp').is_file(), 'Skeeto Status HUD adapter missing'
    # The native fixture runner is kept with the independent plugin project.
    subprocess.run(['cmd','/c',str(plugin/'tests/run.bat')],cwd=ROOT,check=True)
    vswhere=Path(os.environ['ProgramFiles(x86)'])/'Microsoft Visual Studio/Installer/vswhere.exe'
    vs=subprocess.check_output([str(vswhere),'-latest','-products','*','-requires','Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],text=True).strip()
    with tempfile.TemporaryDirectory(prefix='skeeto-statushud-') as folder:
        temp=Path(folder)
        # Standalone source build outside repository; no core/L4N link dependency.
        standalone=temp/'independent'
        shutil.copytree(plugin,standalone,ignore=shutil.ignore_patterns('build','delivery'))
        subprocess.run(['cmd','/c',str(standalone/'build.bat')],check=True,capture_output=True)
        (temp/'host.cpp').write_text(HOST,encoding='utf-8')
        core=ROOT/'src/skeeto/core'
        sdk=ROOT/'src/skeeto/plugin_sdk/include'
        commands=[f'@call "{vs}/VC/Auxiliary/Build/vcvars32.bat" >nul','@if errorlevel 1 exit /b 1',
          f'cl /nologo /MT /EHsc /std:c++17 /utf-8 /W3 /WX /D_CRT_SECURE_NO_WARNINGS /I"{core}" /I"{sdk}" host.cpp "{core}/skeeto_plugins.cpp" /Fehost.exe',
          '@if errorlevel 1 exit /b 1']
        (temp/'build.cmd').write_text('\n'.join(commands),encoding='utf-8')
        result=subprocess.run(['cmd','/c','build.cmd'],cwd=temp,capture_output=True)
        if result.returncode:print(result.stdout.decode(errors='replace'));print(result.stderr.decode(errors='replace'))
        assert result.returncode==0
        root=temp/'game';destination=root/'bin/skeeto/plugins';destination.mkdir(parents=True)
        dll=standalone/'build/Skeeto_StatusHUD.dll'
        shutil.copyfile(dll,destination/dll.name)
        for mode in (0,1,2):subprocess.run([str(temp/'host.exe'),str(root),str(mode)],check=True,timeout=20)
        # Future host minor with unchanged DLL; public API tied to protocol, not product semver.
        future=temp/'future';future.mkdir()
        header=(sdk/'skeeto_plugin_api.h').read_text(encoding='utf-8')
        (future/'skeeto_plugin_api.h').write_text(header.replace('UINT32_C(0x00010000)','UINT32_C(0x00010001)').replace('SktOnAction on_action;','SktOnAction on_action;\n uint32_t optional_tail;'),encoding='utf-8')
        commands[2]=commands[2].replace(f'/I"{sdk}"',f'/I"{future}"').replace('/Fehost.exe','/Fehost_minor.exe')
        # core header includes SDK by name; current root has no private API header.
        (temp/'build.cmd').write_text('\n'.join(commands),encoding='utf-8')
        subprocess.run(['cmd','/c','build.cmd'],cwd=temp,check=True,capture_output=True)
        subprocess.run([str(temp/'host_minor.exe'),str(root),'2'],check=True,timeout=20)
    print('Skeeto Status HUD: SDK consumer, configuration, paint ownership and lifecycle fixtures passed')
    print('Real platform + independent DLL: default-off, both pages, actions, persistence, API-1.1 host passed')

if __name__=='__main__':main()
