#pragma once
#include "radar_abi.h"
namespace radar_nav {
inline float Abs(float x){return x<0?-x:x;}
inline float Sqrt(float x){float result;__asm {
    movss xmm0,x
    sqrtss xmm0,xmm0
    movss result,xmm0
}return result;}
inline float Distance(Vec a,Vec b){Vec d=a-b;return Sqrt(d.Dot(d));}
inline bool Position(Vec p){return Finite(p.x)&&Finite(p.y)&&Finite(p.z)&&Abs(p.x)<1000000&&Abs(p.y)<1000000&&Abs(p.z)<1000000;}
}
