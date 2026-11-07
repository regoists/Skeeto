"""Retail Panel and Label color ABI regression fixtures (no game required)."""
from pathlib import Path
import shutil
import subprocess
import tempfile
from test_nativeui_chat_binding import definition

ROOT = Path(__file__).resolve().parents[1]

def main():
    runtime = (ROOT / "src/skeeto/core/skeeto_nativeui_runtime.h").read_text(encoding="utf-8")
    actual = definition(runtime, "ColorAbi")
    source = r'''
#include <cassert>
#include <cstring>
#include <cstdint>
enum Role {Root,History,Entry,Prompt,ConsolePage,ChatLine};
static void* methods[80]{};
static void* VGet(void*,int i){return methods[i];}
static bool IsExec(void* p){return p!=nullptr;}
namespace skmem {static bool Copy(void* d,const void* s,size_t n){memcpy(d,s,n);return true;}}
ACTUAL
int main(){
 // Captured retail client.dll Panel getters, border/background setters.
 unsigned char bg[]={0x55,0x8b,0xec,0x8b,0x89,0x90,0,0,0,0x8b,0x45,8,0x89,8,0x5d,0xc2,4,0};
 unsigned char fg[]={0x55,0x8b,0xec,0x8b,0x89,0x8c,0,0,0,0x8b,0x45,8,0x89,8,0x5d,0xc2,4,0};
 unsigned char border[]={0x55,0x8b,0xec,0x80,0x7d,8,0,0x74,9,0x66,0x83,0x49,0x60,4,0x5d,0xc2,4,0};
 unsigned char type[]={0x55,0x8b,0xec,0x8b,0x45,8,0x83,0xf8,2,0x7e,0x0f,0xb8,2,0,0,0,0x89,0x81,0x20,1,0,0,0x5d,0xc2};
 methods[4]=methods[58]=methods[59]=bg;methods[60]=bg;methods[61]=fg;methods[70]=border;methods[74]=type;
 assert(ColorAbi((void*)1,History));
 // ChatInputPrompt is a Label: its Getter61 delegates, rather than matching
 // Panel bytes at its entry point. Both use the same hidden result ABI.
 unsigned char label[96]{};
 unsigned char wrapper[]={0x55,0x8b,0xec,0x56,0x8b,0x75,8,0x56,0xe8,0,0,0,0,0x8b,0xc6,0x5e,0x5d,0xc2,4,0};
 memcpy(label,wrapper,sizeof(wrapper));memcpy(label+32,fg,sizeof(fg));
 int displacement=32-13;memcpy(label+9,&displacement,4);methods[61]=label;
 assert(ColorAbi((void*)1,Prompt));assert(!ColorAbi((void*)1,History));
 label[13]=0;assert(!ColorAbi((void*)1,Prompt));
}
'''.replace("ACTUAL",actual)
    compiler=shutil.which("g++") or shutil.which("clang++")
    assert compiler
    with tempfile.TemporaryDirectory(prefix="skeeto-nativeui-abi-") as directory:
        cpp=Path(directory)/"abi.cpp";exe=Path(directory)/"abi.exe"
        cpp.write_text(source,encoding="utf-8")
        subprocess.run([compiler,"-std=c++17",str(cpp),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print("native UI ABI: retail Panel and delegated Label getters passed")

if __name__=="__main__":
    main()
