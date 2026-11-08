#pragma once
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

// Shared pure parameter rules. Called on document/edit changes, never engine IO.
namespace skmenu {
struct CvarSpec {char name[64]{};float minimum=0,maximum=0,step=0;};
inline bool CvarParse(const char* text,CvarSpec* result){
    if(!text)return false;CvarSpec value{};int consumed=0;
    if(std::sscanf(text,"%63s %f %f %f %n",value.name,&value.minimum,&value.maximum,&value.step,&consumed)!=4||!consumed||text[consumed])return false;
    for(const char* c=value.name;*c;++c)if(!std::isalnum((unsigned char)*c)&&*c!='_')return false;
    if(!std::isfinite(value.minimum)||!std::isfinite(value.maximum)||!std::isfinite(value.step)||value.minimum<-1e9f||value.maximum>1e9f||value.maximum<=value.minimum||value.step<=0.f||value.step>value.maximum-value.minimum||double(value.maximum-value.minimum)/value.step>1e6)return false;
    if(value.minimum+value.step==value.minimum||value.maximum-value.step==value.maximum)return false;
    if(result)*result=value;return true;
}
}
