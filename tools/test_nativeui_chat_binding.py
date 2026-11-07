"""Run the real chat discovery against a fake HUD object with multiple bases.

This covers the bounded routing/identity contract, not the retail ABI itself.
Retail offsets and consumers are separately documented from binary evidence.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def definition(source, name):
    start = re.search(r"static (?:bool|void|int) " + name + r"\(", source).start()
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    runtime = (ROOT / "src/skeeto/core/skeeto_nativeui_runtime.h").read_text(encoding="utf-8")
    actual = "\n".join(definition(runtime, n) for n in ("RootMatch", "Discover", "DiscoverChat"))
    source = r'''
#include <cassert>
#include <cstring>
#include <cstdint>
#ifdef __cdecl
#undef __cdecl
#endif
#ifdef __thiscall
#undef __thiscall
#endif
#define __cdecl
#define __thiscall
enum Role {Root,History,Entry,Prompt,ConsolePage,ChatLine};
struct Target {int count=0; bool abiUnsupported=false; unsigned roles=0;};
alignas(void*) static uint8_t chat[256]{};
static int finderCalls=0,lookups=0;
static bool hasChat=true,validChat=true,samePanel=true;
static unsigned getPanel(void* p){assert(p==chat+0x38);return 11;}
static void* findChat(){++finderCalls;return hasChat?chat:nullptr;}
static void* LoadChatFinder(){return (void*)&findChat;}
static bool LoadChatObject(void* p,void** out){assert(p==chat);*out=nullptr;return validChat;}
static const uint8_t getter[]={0x8b,0x41,0x48,0xc3};
static void* VGet(void* p,int slot){assert(p==chat+0x38 && slot==0);return (void*)&getPanel;}
namespace skmem {static bool Copy(void* d,const void*,size_t n){memcpy(d,getter,n);return true;}}
static void* Object(unsigned id,int target){assert(id==11 && target==0);return samePanel?chat+0x38:nullptr;}
static const char* HudVguiName(unsigned id){
    ++lookups;
    switch(id){case 11:return "HudChat";case 12:return "HudChatHistory";
    case 13:return "ChatInputLine";case 14:return "ChatInput";case 15:return "ChatInputPrompt";
    case 16:return "ChatLine1";default:return "Other";}
}
static const char* HudVguiClass(unsigned id){return id==11?"CHudChat":"Panel";}
static int HudVguiChildCount(unsigned id){return id==11?3:id==13?2:0;}
static unsigned HudVguiChild(unsigned id,int i){return id==11?(i==0?12:i==1?13:16):(i==0?14:15);}
static void Add(Target& s,int,unsigned,Role r){++s.count;s.roles|=1u<<r;}
ACTUAL
int main(){
 Target s;DiscoverChat(s);
 assert(finderCalls==1 && s.count==5 && !s.abiUnsupported);
 assert(s.roles==((1u<<Root)|(1u<<History)|(1u<<Entry)|(1u<<Prompt)|(1u<<ChatLine)));
 assert(lookups<30); // Only the known chat subtree; no global viewport scan.
 hasChat=false;s={};DiscoverChat(s);assert(!s.count && !s.abiUnsupported);
 hasChat=true;validChat=false;s={};DiscoverChat(s);assert(!s.count && s.abiUnsupported);
 validChat=true;samePanel=false;s={};DiscoverChat(s);assert(!s.count && s.abiUnsupported);
}
'''.replace("ACTUAL", actual)
    compiler = shutil.which("g++") or shutil.which("clang++")
    assert compiler
    with tempfile.TemporaryDirectory(prefix="skeeto-chat-binding-") as directory:
        cpp = Path(directory) / "binding.cpp"
        exe = Path(directory) / "binding.exe"
        cpp.write_text(source, encoding="utf-8")
        subprocess.run([compiler, "-std=c++17", str(cpp), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print("native chat binding: HUD/Panel base, subtree roles, bounded search and stale identity passed")


if __name__ == "__main__":
    main()
