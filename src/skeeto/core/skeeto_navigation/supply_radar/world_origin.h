#pragma once
#include "radar_abi.h"

// Verified in the installed client 6A440ECF / 963000 image. EntityList slot 3
// returns the main IClientEntity base. Its slot 11 is GetAbsOrigin for every
// supported item and CTerrorPlayer. A C++ Vector reference is returned in EAX.
using WorldOriginFn=const Vec*(__thiscall*)(void*);
static WorldOriginFn worldOriginGetter;
inline bool ConfigureWorldOrigin(){
    auto base=(unsigned char*)GetModuleHandleW(L"client.dll");
    const unsigned char getter[]={0x56,0x8b,0xf1,0xe8,0xb8,0xe2,0xff,0xff,0x8d,0x86,0x94,0,0,0,0x5e,0xc3};
    if(!base||!Readable(base+0x48a70,sizeof getter)||!Executable(base+0x48a70))return false;
    for(unsigned i=0;i<sizeof getter;++i)if(base[0x48a70+i]!=getter[i])return false;
    // CalcAbsolutePosition handles the parent's rotation/attachments, unlike
    // reading m_vecOrigin or merely adding the parent's translation.
    worldOriginGetter=(WorldOriginFn)(base+0x48a70);return true;
}
inline bool WorldPosition(void* entity,Vec& position){
    // Configure verified this fixed executable getter. Entity and vtable
    // lifetime are still checked for every call, as is the returned position.
    if(!worldOriginGetter||!Readable(entity,4))return false;auto methods=*(void***)entity;
    if(!Readable(methods,12*4)||methods[11]!=(void*)worldOriginGetter)return false;
    const Vec* value=worldOriginGetter(entity);if(!Readable(value,sizeof(Vec)))return false;
    position=*value;return Finite(position.x)&&Finite(position.y)&&Finite(position.z);
}
