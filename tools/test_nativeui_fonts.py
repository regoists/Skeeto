"""Regression: reusing an HFont must still invalidate native text layout."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "src/skeeto/core"


def function(source, name):
    start = source.index("static " + name)
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    runtime = (CORE / "skeeto_nativeui_runtime.h").read_text(encoding="utf-8")
    code = r'''
#include "skeeto_nativeui_config.h"
#include <cassert>
#include <cstring>
#undef __thiscall
#define __thiscall
using namespace sknativeui;
enum Role {Root,History,Entry,Prompt,ConsolePage,ChatLine};
struct Target {
 unsigned long font=0; int fontPx=0,fontFlags=-1,fontWeight=0,fontFace=-1;
 bool fontReady=false; void* richFont=nullptr;void* entryFont=nullptr;
};
struct Panel {void* object=nullptr; Role role=History; uint32_t lastFont=0;};
static int allocations=0,glyphChanges=0,layoutChanges=0;
static void* g_surf=(void*)1;static bool g_sktUiFontsHold=false;
static constexpr int kFontFlagAntiAlias=16,kFontFlagDropShadow=128,kFontFlagOutline=512;
static constexpr int kVmtSurfCreateFont=1,kVmtSurfSetFontGlyphSet=2;
static const char* kFaces[]={"Microsoft YaHei","SimHei","Consolas","Tahoma"};
static unsigned long Create(void*){++allocations;return 42;}
static bool Glyph(void*,unsigned long,const char*,int,int,int,int,int,int,int){++glyphChanges;return true;}
static void* VGet(void*,int slot){return slot==1?(void*)&Create:(void*)&Glyph;}
static bool IsExec(void* p){return p!=nullptr;}
namespace skmem {static bool Copy(void* d,const void* s,size_t n){memcpy(d,s,n);return true;}}
static void SetFont(void* object,uint32_t font){++layoutChanges;memcpy((char*)object+0x1c0,&font,4);}
'''
    code += function(runtime, "bool Font(") + "\n" + function(runtime, "void ApplyFont(")
    code += r'''
int main(){
 Target t; t.richFont=(void*)&SetFont; Config c=Defaults(0);
 unsigned char object[1024]{};Panel p;p.object=object;bool changed=false;
 assert(Font(t,c,1080,changed));assert(!changed && allocations==0);
 c.v[FontOverride]=1;
 assert(Font(t,c,1080,changed));assert(changed);
 ApplyFont(t,p,t.font,changed);assert(layoutChanges==1);
 assert(Font(t,c,1080,changed));assert(!changed);
 ApplyFont(t,p,t.font,changed);assert(layoutChanges==1);
 ++c.v[FontSize];assert(Font(t,c,1080,changed));assert(changed);
 ApplyFont(t,p,t.font,changed);assert(layoutChanges==2);
 assert(Font(t,c,1440,changed));assert(changed);
 ApplyFont(t,p,t.font,changed);assert(layoutChanges==3);
 ApplyFont(t,p,7,false);assert(layoutChanges==4);
 assert(allocations==1);assert(glyphChanges==3);
}
'''
    compiler = shutil.which("g++") or shutil.which("clang++")
    assert compiler
    with tempfile.TemporaryDirectory(prefix="skeeto-nativeui-font-") as directory:
        cpp = Path(directory) / "fonts.cpp"
        exe = Path(directory) / "fonts.exe"
        cpp.write_text(code, encoding="utf-8")
        subprocess.run([compiler,"-std=c++17","-I",str(CORE),str(cpp),"-o",str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("native UI fonts: one handle, no redundant updates, size/resolution layout invalidation and restore passed")


if __name__ == "__main__":
    main()
