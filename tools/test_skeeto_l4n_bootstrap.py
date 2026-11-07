"""Exercise root loading, duplicate rejection and Necola startup policy without L4D2."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CPP = r"""
#include "skeeto_bootstrap_common.h"
#include <tlhelp32.h>
static unsigned CoreCount() {
    unsigned count=0;
    HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
    MODULEENTRY32W e{}; e.dwSize=sizeof(e);
    if(Module32FirstW(snap,&e)) do {
        if(GetProcAddress(e.hModule,"SkeetoBootstrapQuery")) ++count;
    } while(Module32NextW(snap,&e));
    CloseHandle(snap); return count;
}
static bool ProbeContains(const wchar_t* root,const char* expected) {
    wchar_t path[SKT_PATH_CAP]{};SktJoinPath(root,L"skeeto_bootstrap_probe.log",path);
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    if(f==INVALID_HANDLE_VALUE)return false;
    char data[4096]{};DWORD n=0;bool ok=ReadFile(f,data,sizeof(data)-1,&n,nullptr)!=FALSE;
    CloseHandle(f);return ok&&strstr(data,expected)!=nullptr;
}
int wmain(int argc,wchar_t** argv) {
    if(argc<3)return 1;
    const wchar_t* root=argv[1];
    const wchar_t* mode=argv[2];
    wchar_t rootDll[SKT_PATH_CAP]{},pluginDll[SKT_PATH_CAP]{},proxy[SKT_PATH_CAP]{};
    if(!SktJoinPath(root,L"skeeto.dll",rootDll)||!SktJoinPath(root,L"bin\\neko\\plugins\\skeeto.dll",pluginDll)||!SktJoinPath(root,L"bin\\wininet.dll",proxy))return 1;
    if(!wcscmp(mode,L"reuse-orig")) {
        wchar_t original[SKT_PATH_CAP]{};
        if(!SktJoinPath(root,L"necola_orig.dll",original)||!LoadLibraryW(original))return 14;
    }
    if(!LoadLibraryW(proxy))return 11;
    HMODULE core=nullptr;
    DWORD until=GetTickCount()+5000;
    do {core=GetModuleHandleW(L"skeeto.dll");if(core)break;Sleep(10);}while((LONG)(GetTickCount()-until)<0);
    if(!core)return 3;
    wchar_t loaded[SKT_PATH_CAP]{};
    if(!GetModuleFileNameW(core,loaded,SKT_PATH_CAP)||_wcsicmp(loaded,rootDll))return 12;
    auto id=SktIdentity(core);
    until=GetTickCount()+5000;
    while(id&&(id->state==SKT_BOOT_LOADING||id->state==SKT_BOOT_WORKER)&&(LONG)(GetTickCount()-until)<0)Sleep(10);
    if(!id||id->state!=SKT_BOOT_HIDDEN||CoreCount()!=1)return 4;
    until=GetTickCount()+5000;
    while(!ProbeContains(root,"entry=wininet event=core-loaded")&&(LONG)(GetTickCount()-until)<0)Sleep(10);
    if(!ProbeContains(root,"entry=wininet event=core-loaded"))return 13;
    HMODULE native=GetModuleHandleW(L"necola.dll"),original=GetModuleHandleW(L"necola_orig.dll");
    const bool wantNone=!wcscmp(mode,L"empty")||!wcscmp(mode,L"disabled");
    const bool wantOriginal=!wcscmp(mode,L"orig")||!wcscmp(mode,L"reuse-orig")||!wcscmp(mode,L"skip-alias");
    if(wantNone&&(native||original))return 15;
    if(wantOriginal&&(!original||native))return 16;
    if(!wantNone&&!wantOriginal&&(!native||original))return 17;
    HMODULE expected=wantOriginal?original:native;
    if(expected&&!GetProcAddress(expected,"?necola_export@@YAXXZ"))return 18;
    if(!wcscmp(mode,L"disabled")&&!ProbeContains(root,"native-necola-autoload-disabled"))return 19;
    if(LoadLibraryW(pluginDll))return 9;
    if(CoreCount()!=1)return 10;
    printf("PASS: root core only; duplicate rejected; Necola case %ls\n",mode);
    return 0;
}
"""


def main():
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = Path(subprocess.check_output([
        str(vswhere), "-latest", "-products", "*", "-requires",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath",
    ], text=True).strip())
    with tempfile.TemporaryDirectory(prefix="skeeto-bootstrap-load-") as temporary:
        work = Path(temporary)
        (work / "host.cpp").write_text(CPP, encoding="utf-8")
        # A no-op fixture carries the native decorated marker, without game hooks.
        (work / "native.cpp").write_text('__declspec(dllexport) void necola_export() {}\n', encoding="utf-8")
        build = (
            '@echo off\ncall "' + str(vs / "VC/Auxiliary/Build/vcvarsall.bat") + '" x86 >nul\n'
            f'cl /nologo /MT /O2 /EHsc /std:c++17 /utf-8 /I "{ROOT / "src/skeeto/core"}" /Fe:host.exe host.cpp\n'
            "if errorlevel 1 exit /b 1\n"
            "cl /nologo /MT /O2 /LD /Fe:native.dll native.cpp\n"
            "if errorlevel 1 exit /b 1\n"
        )
        (work / "build.bat").write_text(build, encoding="utf-8")
        subprocess.run(["cmd", "/d", "/c", str(work / "build.bat")], cwd=work, check=True)
        for case, layout, settings in (
            ("empty", "empty", None),
            ("default", "both", None),
            ("enabled", "both", "necola_autoload=1\n"),
            ("disabled", "both", "necola_autoload=0\n"),
            ("orig", "orig", "necola_autoload=1\n"),
            ("reuse-orig", "both", "necola_autoload=0\n"),
            ("skip-alias", "alias", "necola_autoload=1\n"),
        ):
            root = work / case
            (root / "bin/neko/plugins").mkdir(parents=True)
            shutil.copyfile(ROOT / "runtime/dll/skeeto.dll", root / "skeeto.dll")
            shutil.copyfile(ROOT / "runtime/dll/skeeto.dll", root / "bin/neko/plugins/skeeto.dll")
            shutil.copyfile(ROOT / "runtime/bootstrap/wininet.dll", root / "bin/wininet.dll")
            if layout in ("both", "alias"):
                source = ROOT / "runtime/dll/skeeto.dll" if layout == "alias" else work / "native.dll"
                shutil.copyfile(source, root / "necola.dll")
            if layout in ("both", "orig", "alias"):
                shutil.copyfile(work / "native.dll", root / "necola_orig.dll")
            if settings is not None:
                (root / "skeeto_setting.ini").write_text(settings, encoding="utf-8")
            result = subprocess.run(
                [str(work / "host.exe"), str(root), case, "-hide_skeeto", "-skeeto_boot_probe"],
                cwd=root, capture_output=True, timeout=15)
            assert result.returncode == 0, (case, result.returncode, result.stdout, result.stderr)
            print(result.stdout.decode("utf-8", "replace").strip())


if __name__ == "__main__":
    main()
