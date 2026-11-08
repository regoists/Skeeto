#pragma once
#include "sdk.h"
extern bool SkeetoNavTraceLine(const float* from,const float* to,unsigned mask,void* skip,void* out,unsigned outBytes);
namespace worldnav {
// L4D2 client ABI shared with Skeeto's existing TraceRay path.
// TraceRay is slot 5 here, not slot 4 from the generic Source SDK interface.
inline bool InterfaceProfile(void* engine,void* trace,void* surface){
    // Skeeto acquires the public interfaces directly. In particular, no
    // left4neko.dll address or L4N menu profile is part of this check.
    return MethodOK(engine,37)&&MethodOK(trace,5)&&MethodOK(surface,15);
}
struct TraceContext {void* engineTrace=nullptr;void* local=nullptr;};
inline bool CastScene(void* context,Vec from,Vec to,Trace& result){
    auto& ctx=*(TraceContext*)context;if(!ctx.engineTrace)return false;
    const float a[]={from.x,from.y,from.z},b[]={to.x,to.y,to.z};
    // Reuse Skeeto's already guarded TraceRay path and its 512-byte engine
    // output buffer. The mask excludes monster/hitbox occlusion.
    if(!SkeetoNavTraceLine(a,b,0x408b,ctx.local,&result,sizeof result))return false;
    return Finite(result.fraction)&&result.fraction>=0&&result.fraction<=1;
}
}
