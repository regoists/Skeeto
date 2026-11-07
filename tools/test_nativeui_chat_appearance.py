"""Run the actual appearance functions against native fade/input state fixtures."""
from pathlib import Path
import shutil
import subprocess
import tempfile
from test_nativeui_chat_binding import definition

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "src/skeeto/core"


def main():
    runtime = (CORE / "skeeto_nativeui_runtime.h").read_text(encoding="utf-8")
    source = r'''
#include "skeeto_nativeui_config.h"
#include <cassert>
#include <cstring>
#undef __thiscall
#define __thiscall
using namespace sknativeui;
enum Role {Root,History,Entry,Prompt,ConsolePage,ChatLine};
struct Target {bool fontReady=false;uint32_t font=0;void* setAlpha=nullptr;};
struct Panel {
 unsigned id=0;void* object=nullptr;Role role=Root;
 uint32_t bg=0,fg=0,font=0,lastBg=0,lastFg=0,lastFont=0;
 Rect bounds{},lastBounds{};bool geometry=false;
 int paintType=0;bool border=false,lastBorder=false;
 uint32_t extra[6]{},lastExtra[6]{};bool special=false;
 float alpha=255;bool alphaOwned=false;
};
static const int kFrameFields[]={0x196,0x19a,0x186,0x18a,0x18e,0x192};
namespace skmem {
static bool Copy(void*d,const void*s,size_t n){memcpy(d,s,n);return true;}
static bool Write(void*d,const void*s,size_t n){memcpy(d,s,n);return true;}
}
static int backgroundWrites=0,fontWrites=0,alphaWrites=0,geometryWrites=0,borderWrites=0,typeWrites=0;
static uint32_t GetColor(void*p,bool fg){uint32_t v;memcpy(&v,(char*)p+(fg?0x8c:0x90),4);return v;}
static void SetColor(void*p,bool fg,uint32_t v){if(!fg)++backgroundWrites;memcpy((char*)p+(fg?0x8c:0x90),&v,4);}
static void SetAlpha(void*p,int value){++alphaWrites;float v=(float)value;memcpy((char*)p+0x118,&v,4);}
static void BorderCall(void*,bool){++borderWrites;}
static void Type(void*,int){++typeWrites;}
static void* VGet(void*,int slot){return slot==70?(void*)&BorderCall:(void*)&Type;}
static void ApplyFont(Target&,Panel&p,uint32_t value,bool=false){++fontWrites;p.lastFont=value;}
static bool Alive(const Panel&,int){return true;}
static void HudVguiGetPos(unsigned,int*x,int*y){*x=*y=0;}
static void HudVguiGetSize(unsigned,int*w,int*h){*w=*h=0;}
static void HudVguiSetPos(unsigned,int,int){++geometryWrites;}
static void HudVguiSetSize(unsigned,int,int){++geometryWrites;}
ACTUAL
int main(){
 alignas(void*) unsigned char memory[1024]{};
 Target t;t.setAlpha=(void*)&SetAlpha;
 Config c=Defaults(0);Panel p;p.object=memory;p.role=History;
 // The native fade owns history/line backgrounds. Neither phase may be
 // overwritten by a periodic fixed-opacity style update.
 uint32_t native=Pack(0x090909,40);memcpy(memory+0x90,&native,4);
 Apply(t,p,c,0,1920,1080,false);assert(backgroundWrites==0 && GetColor(memory,false)==native);
 native=0;memcpy(memory+0x90,&native,4);Apply(t,p,c,0,1920,1080,false);
 assert(backgroundWrites==0 && GetColor(memory,false)==native);
 p.role=ChatLine;Apply(t,p,c,0,1920,1080,false);assert(backgroundWrites==0);
 // Disabling must also leave those native-owned fade/theme properties alone.
 // A native state change after discovery is not a style write to restore.
 p.paintType=2;
 const Role nativeFadeRoles[]={History,ChatLine,Prompt};
 for(Role role:nativeFadeRoles){
  p.role=role;Restore(t,p,0);
  assert(backgroundWrites==0 && borderWrites==0 && typeWrites==0);
 }
 p.role=Root;p.alpha=180.5f;memcpy(memory+0x118,&p.alpha,4);
 c.v[SayTime]=0;Apply(t,p,c,0,1920,1080,false);
 float alpha;memcpy(&alpha,memory+0x118,4);assert(alpha==0 && p.alphaOwned);
 Apply(t,p,c,0,1920,1080,false);assert(alphaWrites==1);
 // An explicitly opened input must remain usable even with duration zero.
 int mode=1;memcpy(memory+0x1c0,&mode,4);Apply(t,p,c,0,1920,1080,false);
 memcpy(&alpha,memory+0x118,4);assert(alpha==180.5f && !p.alphaOwned);
 assert(GetColor(memory,false)==Pack(c.v[Background],c.v[BgAlpha]));
 mode=0;memcpy(memory+0x1c0,&mode,4);Apply(t,p,c,0,1920,1080,false);
 assert(GetColor(memory,false)==p.bg);
 // Returning to positive duration restores only an alpha value still owned.
 c.v[SayTime]=3;Apply(t,p,c,0,1920,1080,false);
 memcpy(&alpha,memory+0x118,4);assert(alpha==180.5f && !p.alphaOwned);
 c.v[SayTime]=0;Apply(t,p,c,0,1920,1080,false);Restore(t,p,0);
 memcpy(&alpha,memory+0x118,4);assert(alpha==180.5f && !p.alphaOwned);
 // A newer external alpha is preserved when disabling the zero-duration mode.
 Apply(t,p,c,0,1920,1080,false);alpha=128;memcpy(memory+0x118,&alpha,4);
 c.v[SayTime]=3;Apply(t,p,c,0,1920,1080,false);
 memcpy(&alpha,memory+0x118,4);assert(alpha==128);
 // Console background behavior is independent of the chat policy.
 p.role=History;int before=backgroundWrites;Apply(t,p,c,1,1920,1080,false);
 assert(backgroundWrites>before);
 p.role=Root;c.v[Geometry]=1;Apply(t,p,c,1,1920,1080,false);
 assert(geometryWrites==0); // Console owns its native drag/resize behavior.
}
'''.replace("ACTUAL", definition(runtime, "Apply") + "\n" + definition(runtime, "Restore"))
    compiler = shutil.which("g++") or shutil.which("clang++")
    assert compiler
    with tempfile.TemporaryDirectory(prefix="skeeto-chat-appearance-") as directory:
        cpp = Path(directory) / "appearance.cpp"
        exe = Path(directory) / "appearance.exe"
        cpp.write_text(source, encoding="utf-8")
        subprocess.run([compiler, "-std=c++17", "-I", str(CORE), str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("native chat appearance: native fade ownership, input background, zero duration and restore passed")


if __name__ == "__main__":
    main()
