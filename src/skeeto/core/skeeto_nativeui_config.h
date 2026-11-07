#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdint>

// Independent from VGUI: serialized values are validated before reaching Paint.
namespace sknativeui {
enum Field {
    Enabled, FontOverride, FontFace, FontSize, Bold, Shadow, Outline, AutoScale,
    Text, Input, Background, InputBackground, BgAlpha, InputBgAlpha,
    Geometry, X, Y, Width, Height, SayTime, Survivor, Infected,
    Developer, TitleText, TitleBackground, Border, AlphaStep, Count
};
struct Config { int v[Count]{}; };
struct FieldInfo { const char* key; int lo, hi; int targets=3; };
inline constexpr FieldInfo kFields[] = {
    {"enabled",0,1}, {"font_override",0,1}, {"font_face",0,3}, {"font_size",10,48},
    {"bold",0,1}, {"shadow",0,1}, {"outline",0,1}, {"auto_scale",0,1},
    {"text_rgb",0,0xffffff}, {"input_rgb",0,0xffffff}, {"bg_rgb",0,0xffffff},
    {"input_bg_rgb",0,0xffffff}, {"bg_alpha",0,100}, {"input_bg_alpha",0,100},
    {"geometry",0,1,1}, {"x",0,100,1}, {"y",0,100,1}, {"width",10,100,1}, {"height",5,100,1},
    {"say_time",0,60,1}, {"survivor_rgb",0,0xffffff,1}, {"infected_rgb",0,0xffffff,1},
    {"developer_rgb",0,0xffffff,2}, {"title_text_rgb",0,0xffffff,2}, {"title_bg_rgb",0,0xffffff,2},
    {"border",0,1},
    {"alpha_step",1,100},
};
static_assert(sizeof(kFields)/sizeof(kFields[0]) == Count, "native UI field registry mismatch");
inline int Clamp(Field f, int value) {
    return value < kFields[f].lo ? kFields[f].lo : value > kFields[f].hi ? kFields[f].hi : value;
}
inline bool Color(Field f) {
    return (f >= Text && f <= InputBackground) || (f >= Survivor && f <= TitleBackground);
}
inline bool Toggle(Field f) { return kFields[f].lo == 0 && kFields[f].hi == 1; }
inline Config Defaults(int target) {
    Config c{};
    c.v[FontOverride]=target?1:0; c.v[FontSize]=20; c.v[Shadow]=1; c.v[AutoScale]=1;
    c.v[Text]=0xe0eaf4; c.v[Input]=0xe0eaf4; c.v[Background]=0x101820;
    c.v[InputBackground]=0x182430; c.v[BgAlpha]=70; c.v[InputBgAlpha]=85;
    c.v[AlphaStep]=5;
    if(!target){c.v[X]=2;c.v[Y]=60;c.v[Width]=42;c.v[Height]=26;}
    c.v[SayTime]=12; c.v[Survivor]=0x99ccff; c.v[Infected]=0xff4040;
    c.v[Developer]=0xd0b078; c.v[TitleText]=0xe0eaf4; c.v[TitleBackground]=0x182430;
    return c;
}
inline const char* Prefix(int target) { return target ? "console_ui_" : "chat_ui_"; }
inline bool Applies(Field f,int target) {return (kFields[f].targets&(1<<target))!=0;}
inline bool ReadLine(Config& c, int target, const char* line) {
    const char* prefix=Prefix(target); const size_t n=strlen(prefix);
    if (strncmp(line,prefix,n)) return false;
    for (int i=0;i<Count;++i) {
        if(!Applies((Field)i,target))continue;
        const size_t len=strlen(kFields[i].key);
        if (strncmp(line+n,kFields[i].key,len) || line[n+len]!='=') continue;
        errno=0; char* end=nullptr; const char* start=line+n+len+1;
        const long value=strtol(start,&end,10);
        if (errno || end==start || value < (-2147483647L-1) || value>2147483647L) return false;
        while (*end==' ' || *end=='\t' || *end=='\r' || *end=='\n') ++end;
        if (*end) return false;
        c.v[i]=Clamp((Field)i,(int)value); return true;
    }
    return false;
}
inline void Save(FILE* f, const Config& c, int target) {
    for (int i=0;i<Count;++i) if(Applies((Field)i,target))fprintf(f,"%s%s=%d\n",Prefix(target),kFields[i].key,c.v[i]);
}
inline uint32_t Pack(int rgb, int alpha) {
    return ((uint32_t)(alpha*255/100)<<24) | ((rgb&255)<<16) | (rgb&0xff00) | ((rgb>>16)&255);
}
struct Rect { int x,y,w,h; };
inline Rect Bounds(const Config& c, int sw, int sh) {
    Rect r{sw*c.v[X]/100,sh*c.v[Y]/100,sw*c.v[Width]/100,sh*c.v[Height]/100};
    if (r.x+r.w>sw) r.x=sw-r.w;
    if (r.y+r.h>sh) r.y=sh-r.h;
    return r;
}
inline bool Nudge(Config& c,int dx,int dy,bool fast) {
    const int step=fast?4:1;
    const int maxX=100-c.v[Width],maxY=100-c.v[Height];
    int x=(c.v[X]>maxX?maxX:c.v[X])+dx*step;
    int y=(c.v[Y]>maxY?maxY:c.v[Y])+dy*step;
    x=x<0?0:x>maxX?maxX:x;y=y<0?0:y>maxY?maxY:y;
    if(c.v[X]==x && c.v[Y]==y && c.v[Geometry])return false;
    c.v[X]=x;c.v[Y]=y;c.v[Geometry]=1;return true;
}
}
