#pragma once
#include <Windows.h>
#include <cstdint>
#include "skeeto_plugin_api.h"

namespace skplugins {
constexpr unsigned kMaxPlugins=32, kMaxItems=64, kMaxDepth=8;
enum class State { LoadError,Loaded,Active,EntryMissing,ApiMismatch,DuplicateId,InvalidDescriptor,InitError,PageError,ActionError };
struct Plugin {
    wchar_t file[MAX_PATH]{},nameZh[96]{},nameEn[96]{},author[96]{},version[48]{};
    char id[48]{};
    State state=State::LoadError;
    uint32_t error=0;
    bool hasMenu=false;
};
struct Item {
    uint32_t id=0,type=0,flags=0,targetPage=0,choice_count=0;
    wchar_t label[96]{},text[128]{},choices[256]{};
    double value=0,min=0,max=0,step=0;
};
struct Page {
    wchar_t title[96]{};
    Item items[kMaxItems]{};
    uint32_t plugin=0,page_id=0,count=0;
    bool valid=false;
};
struct Counters { uint32_t loads=0,inits=0,builds=0,actions=0; };

// Start is worker-only, once, after normal core initialization. Disabled returns
// before path/filesystem work. Published records are then owned by Paint.
void Start(const wchar_t* root,bool enabled);
bool Started();
bool Loading();
bool Limited();
uint32_t ScanError();
unsigned Count();
const Plugin* At(unsigned index);
bool NeedsPaint();
void PaintPump(bool english); // <= one init OR one action+page; never a render callback.
bool RequestPage(unsigned plugin,uint32_t page);
bool RequestAction(uint32_t item,double value);
bool Pending();
const Page& CurrentPage(); // Paint-only, host-owned snapshot.
Counters Stats(); // Paint/engine command thread after startup publication.
}
