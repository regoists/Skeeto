"""Compile actual profile IO with MSVC x86; run bounded real Win32 file tests."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def implementation(name, fragment):
    marker = f"namespace {name} {{"
    for path in (ROOT / "src/skeeto/core/skeeto_proxy.cpp",):
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8-sig")
        start = text.find(marker)
        if start < 0:
            continue
        # Match braces while skipping literals and comments.
        import re
        tokens = re.finditer(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', text[start:])
        depth = 0
        for token in tokens:
            if token.group() == "{":
                depth += 1
            elif token.group() == "}":
                depth -= 1
                if depth == 0:
                    return text[start:start + token.end()]
    raise RuntimeError(f"Missing {name} implementation")


TEST = r'''
static skviewmodel::Store store{}, loaded{};
static void Raw(const wchar_t* path,const char* bytes,DWORD count) {
    HANDLE f=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);
    assert(f!=INVALID_HANDLE_VALUE);DWORD n=0;assert(WriteFile(f,bytes,count,&n,nullptr)&&n==count);CloseHandle(f);
}
int wmain(int argc,wchar_t** argv) {
    assert(argc==2); std::wstring root=argv[1];
    std::wstring base=root+L"\\left4dead2";assert(CreateDirectoryW(base.c_str(),nullptr));
    std::wstring path=base+L"\\skeeto\\viewmodel\\profiles.ini";
    assert(skviewmodelio::Load(path.c_str(),&loaded)==2);
    assert(GetFileAttributesW((base+L"\\skeeto").c_str())==INVALID_FILE_ATTRIBUTES);
    int i=store.Ensure("models/weapons/v_rifle.mdl",0x1234);assert(i==0);
    store.records[0].pose.position={1.25f,-2.5f,3};store.records[0].pose.rotation={10,20,30};store.records[0].pose.Rebuild();
    assert(skviewmodelio::Save(path.c_str(),store));
    assert(skviewmodelio::Load(path.c_str(),&loaded)==1&&loaded.count==1&&loaded.records[0].pose.position.x==1.25f);
    puts("PASS missing and roundtrip");
    Raw(path.c_str(),"broken",6);auto before=loaded;
    assert(skviewmodelio::Load(path.c_str(),&loaded)==3&&std::memcmp(&loaded,&before,sizeof(loaded))==0);
    puts("PASS bad load preserves store");
    static char oversized[256*1024+1]{};Raw(path.c_str(),oversized,sizeof(oversized));
    assert(skviewmodelio::Load(path.c_str(),&loaded)==3&&std::memcmp(&loaded,&before,sizeof(loaded))==0);
    puts("PASS overcapacity");
    assert(skviewmodelio::Save(path.c_str(),store));
    HANDLE held=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(held!=INVALID_HANDLE_VALUE);
    store.records[0].pose.position.x=99;assert(!skviewmodelio::Save(path.c_str(),store));CloseHandle(held);
    assert(skviewmodelio::Load(path.c_str(),&loaded)==1&&loaded.records[0].pose.position.x==1.25f);
    assert(GetFileAttributesW((path+L".tmp").c_str())==INVALID_FILE_ATTRIBUTES);
    puts("PASS atomic replacement failure preserves old file");
    std::wstring unicode=base+L"\\skeeto\\viewmodel\\持枪配置.ini";
    assert(skviewmodelio::Save(unicode.c_str(),store));assert(skviewmodelio::Load(unicode.c_str(),&loaded)==1&&loaded.records[0].pose.position.x==99);
    puts("PASS Unicode path");return 0;
}
'''


def main():
    vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
    vs = subprocess.check_output([str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], text=True).strip()
    if not vs:
        raise RuntimeError("MSVC x86 toolset missing")
    with tempfile.TemporaryDirectory(prefix="skeeto-viewmodel-io-") as tmp:
        folder = Path(tmp)
        source = "#define _CRT_SECURE_NO_WARNINGS\n#define WIN32_LEAN_AND_MEAN\n#include <Windows.h>\n#include <cmath>\n#include <cstring>\n#include <cstdint>\n#include <cstdlib>\n#include <cstdio>\n#include <cwchar>\n#include <string>\n#include <cassert>\n"
        source += implementation("skviewmodel", "tools/local/analysis/viewmodel/pose_core.inc")
        source += implementation("skviewmodelio", "tools/local/analysis/viewmodel/io_core.inc") + TEST
        (folder / "fixture.cpp").write_text(source, encoding="utf-8")
        (folder / "run.bat").write_text(f'@echo off\ncall "{vs}\\VC\\Auxiliary\\Build\\vcvarsall.bat" x86 >nul\nif errorlevel 1 exit /b 1\ncl /nologo /EHsc /std:c++17 /utf-8 /W3 fixture.cpp /Fe:fixture.exe\nexit /b %errorlevel%\n', encoding="utf-8")
        subprocess.run(["cmd", "/c", str(folder / "run.bat")], cwd=folder, check=True)
        subprocess.run([str(folder / "fixture.exe"), str(folder)], check=True)


if __name__ == "__main__":
    main()
