#pragma once
// Shared helpers for the ported SupplyRadar 1.3.6 headers. Everything here is
// built on the navigation sdk.h guarded reads (skmem); no VirtualQuery.
#include "../sdk.h"

inline unsigned TextLength(const char* s){unsigned n=0;if(s)while(s[n])++n;return n;}

namespace radar_abi {
// Byte comparison of fixed, immutable module code. Cold: used by Configure paths only.
inline bool EqualBytes(const unsigned char* p,const unsigned char* expected,unsigned size){
    if(!Readable(p,size))return false;
    for(unsigned i=0;i<size;++i)if(p[i]!=expected[i])return false;
    return true;
}
// engine.dll of the single supported L4D2 build (timestamp 6A440ED6, image size 73F000).
inline bool EngineBuild(unsigned char* base){
    if(!Readable(base,0x1000)||*(unsigned short*)base!=0x5a4d)return false;
    unsigned nt=*(unsigned*)(base+0x3c);if(nt>0xe00)return false;
    return *(unsigned*)(base+nt)==0x4550&&*(unsigned short*)(base+nt+4)==0x14c&&
           *(unsigned*)(base+nt+8)==0x6a440ed6&&*(unsigned*)(base+nt+0x50)==0x73f000;
}
}
