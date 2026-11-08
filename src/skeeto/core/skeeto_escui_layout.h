#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include "skeeto_menu_params.h"

// Independent bounded document. Children use the flyout content rectangle;
// parent indices are remapped by every operation that changes array order.
namespace skesc {
constexpr int kMaxItems=128,kLabelBytes=128,kParamBytes=192,kActionCount=33,kFileBytes=131072,kPageItems=8,kDepth=8;
constexpr int kFlyoutLeft=416,kFlyoutTop=160,kFlyoutRight=1888,kFlyoutBottom=936;
constexpr int kCommand=14,kSlider=15,kFlyout=16,kPinnedSlider=17,kVote=18,kQuit=19,kDifficulty=20,kScenario=21,kRestart=22,kLobby=23,kAllTalk=24,kNativeVote=25,kServerInfo=26,kCvarPresets=27,kBattleReport=28;
constexpr int kMapInfo=29,kClock=30,kRoundTimer=31,kNativeOptions=32;
inline const char* ComponentOptions(int action){
    switch(action){case kServerInfo:return "all server map ping players address";case kMapInfo:return "all campaign chapter code";case kClock:return "time date full";case kRoundTimer:return "hms seconds";default:return nullptr;}
}
inline bool ChoiceValid(const char* choices,const char* value){
    if(!value[0])return true;size_t n=std::strlen(value);
    for(const char* p=choices;*p;){const char* end=std::strchr(p,' ');size_t len=end?size_t(end-p):std::strlen(p);
        if(n==len&&!std::strncmp(p,value,n))return true;if(!end)break;p=end+1;}return false;
}
// Background: -1 uses the document color, 0 transparent, 1 follows text,
// 2 white, 3..8 palette colors. Item opacity -1 inherits the shared opacity.
struct Item {int action=0,x=0,y=0,w=0,h=0,font=0,color=0,parent=-1,background=-1,opacity=-1,textRgb=-1;bool visible=false;char zh[kLabelBytes]{},en[kLabelBytes]{},parameter[kParamBytes]{};};
struct Layout {int count=0,accent=6,backdrop=0,titlePx=0,bodyPx=0,background=1,opacity=5,transparentBorder=1;Item items[kMaxItems]{};};
// RGB text input: exactly three decimal channels in 0..255, packed RRGGBB.
inline bool RgbParse(const char* text,int& rgb){
    int value=0;const char* p=text;
    for(int i=0;i<3;++i){while(*p==' ')++p;char* end=nullptr;errno=0;
        long channel=std::strtol(p,&end,10);if(end==p||errno==ERANGE||channel<0||channel>255)return false;
        value=(value<<8)|(int)channel;p=end;if(i<2&&*p!=' ')return false;
    }
    while(*p==' ')++p;if(*p)return false;rgb=value;return true;
}
inline bool IsMenu(int action){return action==kFlyout||action==kVote;}
inline bool IsSlider(int action){return action==kSlider||action==kPinnedSlider;}
inline bool LabelValid(const char* text,size_t cap=kLabelBytes){
    const auto* p=(const unsigned char*)text;size_t n=0;
    while(n<cap&&p[n]){
        unsigned c=p[n++];if(c<32||c==127)return false;if(c<128)continue;
        unsigned value=0,min=0;int extra=0;
        if(c>=0xc2&&c<=0xdf){value=c&31;extra=1;min=0x80;}
        else if(c>=0xe0&&c<=0xef){value=c&15;extra=2;min=0x800;}
        else if(c>=0xf0&&c<=0xf4){value=c&7;extra=3;min=0x10000;}
        else return false;
        for(int j=0;j<extra;++j){if(n>=cap||p[n]<0x80||p[n]>0xbf)return false;value=(value<<6)|(p[n++]&63);}
        if(value<min||value>0x10ffff||(value>=0xd800&&value<=0xdfff))return false;
    }return n<cap;
}
inline bool ParameterValid(const Item& b){
    if(!LabelValid(b.parameter,kParamBytes))return false;
    if(IsSlider(b.action))return skmenu::CvarParse(b.parameter,nullptr);
    if(b.action==kCommand)return b.parameter[0]&&std::strspn(b.parameter," ")!=std::strlen(b.parameter);
    if(const char* choices=ComponentOptions(b.action))return ChoiceValid(choices,b.parameter);
    return !b.parameter[0];
}
inline void SetAction(Item& b,int action){
    b.action=action;b.parameter[0]=0;
    if(IsSlider(action))std::strcpy(b.parameter,"volume 0 1 0.01");
    if(action==kCommand)std::strcpy(b.parameter,"echo Skeeto");
}
inline int MinimumHeight(const Layout& l,const Item& b){return (b.font?l.titlePx:l.bodyPx)+(IsSlider(b.action)?32:12);}
inline int MinimumWidth(const Item& b){return IsSlider(b.action)?240:40;}
inline void ClampItem(Layout& l,int index){
    auto& b=l.items[index];const int x=b.parent<0?0:kFlyoutLeft,y=b.parent<0?0:kFlyoutTop;
    const int right=b.parent<0?1920:kFlyoutRight,bottom=b.parent<0?1080:kFlyoutBottom;
    b.w=(std::max)(MinimumWidth(b),(std::min)(right-x,b.w));
    b.h=(std::max)(MinimumHeight(l,b),(std::min)(bottom-y,b.h));
    b.x=(std::max)(x,(std::min)(right-b.w,b.x));b.y=(std::max)(y,(std::min)(bottom-b.h,b.y));
}
inline bool Valid(const Layout& l){
    if(l.count<1||l.count>kMaxItems||l.accent<0||l.accent>6||l.backdrop<0||l.backdrop>100||l.titlePx<12||l.titlePx>96||l.bodyPx<12||l.bodyPx>64||l.background<0||l.background>8||l.opacity<0||l.opacity>100||l.transparentBorder<0||l.transparentBorder>1)return false;
    for(int i=0;i<l.count;++i){const auto& b=l.items[i];
        if(b.action<0||b.action>=kActionCount||b.font<0||b.font>1||b.color<0||b.color>6||b.parent<-1||b.parent>=l.count
            ||b.x<0||b.y<0||b.w<MinimumWidth(b)||b.w>1920||b.h<MinimumHeight(l,b)||b.h>1080||b.x>1920-b.w||b.y>1080-b.h
            ||!LabelValid(b.zh)||!LabelValid(b.en)||!ParameterValid(b))return false;
        if(b.background<-1||b.background>8||b.opacity<-1||b.opacity>100||b.textRgb<-1||b.textRgb>0xffffff)return false;
        if(b.parent>=0&&(b.x<kFlyoutLeft||b.y<kFlyoutTop||b.x>kFlyoutRight-b.w||b.y>kFlyoutBottom-b.h))return false;
        int at=b.parent,depth=0;
        while(at>=0){if(at>=l.count||at==i||++depth>kDepth||!IsMenu(l.items[at].action))return false;at=l.items[at].parent;}
    }return true;
}
inline int Children(const Layout& l,int parent,bool editing){int n=0;for(int i=0;i<l.count;++i)if(l.items[i].parent==parent&&(editing||l.items[i].visible))++n;return n;}
inline int PageFor(const Layout& l,int index,bool editing){int n=0;for(int i=0;i<index;++i)if(l.items[i].parent==l.items[index].parent&&(editing||l.items[i].visible))++n;return n/kPageItems;}
inline int View(const Layout& l,int parent,int page,bool editing,int* out){
    int n=0,skip=parent<0?0:page*kPageItems;
    for(int i=0;i<l.count;++i)if(l.items[i].parent==parent&&(editing||l.items[i].visible)){
        if(skip){--skip;continue;}out[n++]=i;if(parent>=0&&n==kPageItems)break;
    }return n;
}
inline int Hit(const Layout& l,int x,int y,bool editing){
    for(int i=l.count-1;i>=0;--i){const auto& b=l.items[i];if(b.parent>=0||(!editing&&(!b.visible||!b.action)))continue;
        if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h)return i;}return -1;
}
inline int NextAction(const Layout& l,int index,int delta){
    for(int step=0;step<l.count;++step){index=(index+delta+l.count)%l.count;if(l.items[index].parent<0&&l.items[index].visible&&l.items[index].action)return index;}return -1;
}
inline bool Add(Layout& l,int action,int parent=-1){
    if(l.count>=kMaxItems||action<0||action>=kActionCount)return false;
    Item b{};SetAction(b,action);b.parent=parent;b.x=1416;b.y=160;b.w=472;b.h=76;b.visible=true;l.items[l.count++]=b;ClampItem(l,l.count-1);return true;
}
inline bool SetParent(Layout& l,int index,int parent){
    Item old=l.items[index];l.items[index].parent=parent;ClampItem(l,index);
    if(Valid(l))return true;l.items[index]=old;return false;
}
inline bool Descendant(const Layout& l,int index,int ancestor){
    for(int depth=0,at=index;at>=0&&depth<=kDepth;++depth,at=l.items[at].parent)if(at==ancestor)return true;return false;
}
inline bool Erase(Layout& l,int index){
    if(index<0||index>=l.count)return false;
    bool remove[kMaxItems]{};int map[kMaxItems]{},count=0;
    for(int i=0;i<l.count;++i){remove[i]=Descendant(l,i,index);if(!remove[i]){map[i]=count++;}}
    if(!count)return false;
    for(int i=0;i<l.count;++i)if(!remove[i]){auto b=l.items[i];if(b.parent>=0)b.parent=map[b.parent];l.items[map[i]]=b;}
    for(int i=count;i<l.count;++i)l.items[i]=Item{};l.count=count;return true;
}
inline int Move(Layout& l,int index,int delta){
    int next=index;for(int at=index+delta;at>=0&&at<l.count;at+=delta)if(l.items[at].parent==l.items[index].parent){next=at;break;}
    std::swap(l.items[index],l.items[next]);
    for(int i=0;i<l.count;++i){auto& p=l.items[i].parent;if(p==index)p=next;else if(p==next)p=index;}return next;
}
inline int Duplicate(Layout& l,int index){
    const int before=l.count;int map[kMaxItems];std::fill(map,map+kMaxItems,-1);int needed=0;
    for(int i=0;i<before;++i)if(Descendant(l,i,index))map[i]=before+needed++;
    if(before+needed>kMaxItems)return -1;
    for(int i=0;i<before;++i)if(map[i]>=0){auto b=l.items[i];if(b.parent>=0&&map[b.parent]>=0)b.parent=map[b.parent];l.items[map[i]]=b;}
    l.count+=needed;return map[index];
}
inline Layout Defaults(){
    Layout l{};l.backdrop=0;l.titlePx=42;l.bodyPx=26;
    auto add=[&](int action,int x,int y,int w,int h,const char* zh=nullptr,const char* en=nullptr,const char* parameter=nullptr,int font=0,int parent=-1){
        auto& b=l.items[l.count++];SetAction(b,action);b.x=x;b.y=y;b.w=w;b.h=h;b.font=font;b.parent=parent;b.visible=true;
        if(zh)std::strcpy(b.zh,zh);if(en)std::strcpy(b.en,en);if(parameter)std::strcpy(b.parameter,parameter);return l.count-1;
    };
    auto child=[&](int parent,int action,const char* zh,const char* en,const char* parameter=nullptr){int n=Children(l,parent,true)%kPageItems;
        int i=add(action,1416,160+n*96,472,76,zh,en,parameter,0,parent);l.items[i].color=l.items[parent].color;return i;};
    // Grouped rails: team green, information purple, session orange;
    // audio cyan, view/input amber, tools silver. No decorative headings.
    int map=add(kMapInfo,32,24,1072,48);l.items[map].color=6;
    int clock=add(kClock,1120,24,184,48);l.items[clock].color=2;
    int timer=add(kRoundTimer,1320,24,212,48);l.items[timer].color=3;
    const char* fields[]={"server","ping","players","address"};const int fx[]={32,568,804,1120},fw[]={520,220,300,768};
    const char* fieldZh[]={u8"服务器名称",u8"服务器延迟",u8"服务器人数",u8"连接地址"};
    const char* fieldEn[]={"Server name","Server ping","Player count","Connection address"};
    for(int i=0;i<4;++i){int at=add(kServerInfo,fx[i],88,fw[i],56,fieldZh[i],fieldEn[i],fields[i]);l.items[at].color=6;}
    const int team[]={8,9,kNativeVote,10,kScenario,kDifficulty};for(int i=0;i<6;++i){int at=add(team[i],32,184+i*68,340,56);l.items[at].color=3;if(team[i]==kNativeVote){std::strcpy(l.items[at].zh,u8"发起投票");std::strcpy(l.items[at].en,"Call vote");}}
    const int info[]={12,kBattleReport,13};for(int i=0;i<3;++i){int at=add(info[i],32,628+i*58,340,48);l.items[at].color=4;}
    int options=add(kNativeOptions,32,802,340,48);l.items[options].color=4;
    int disconnect=add(6,32,868,340,56);l.items[disconnect].color=5;
    int quit=add(kQuit,32,936,340,56);l.items[quit].color=5;
    int volume=add(kSlider,1548,164,340,68,u8"游戏音量","Game volume","volume 0 1 0.01");l.items[volume].color=2;
    int music=add(kSlider,1548,244,340,68,u8"音乐音量","Music volume","snd_musicvolume 0 1 0.01");l.items[music].color=2;
    int audio=add(kFlyout,1548,324,340,52,u8"分类音量调节","Category volume mixer");l.items[audio].color=2;
    int audioSettings=add(2,1548,388,340,48);l.items[audioSettings].color=2;
    int sensitivity=add(kSlider,1548,492,340,68,u8"鼠标灵敏度","Mouse sensitivity","sensitivity 0.1 10 0.1");l.items[sensitivity].color=1;
    const int input[]={4,3};for(int i=0;i<2;++i){int at=add(input[i],1548,580+i*58,340,48);l.items[at].color=1;}
    int common=add(kFlyout,1548,724,340,52,u8"常用 ConVar 修改","Common ConVar controls");l.items[common].color=6;
    int presets=add(kCvarPresets,1548,788,340,48);l.items[presets].color=6;
    int hud=add(kFlyout,1548,846,340,48,u8"HUD 与采集控制","HUD and sampling");l.items[hud].color=6;
    int native=add(7,1548,904,340,48,u8"原生 ESC 菜单","Native ESC menu");l.items[native].color=6;
    child(hud,kCommand,u8"自定义 HUD","Custom HUD","skeeto_hud_toggle");
    child(hud,kCommand,u8"世界 HUD","World HUD","skeeto_world_hud_toggle");
    child(hud,kCommand,u8"准星","Crosshairs","skeeto_xhair_toggle");
    child(hud,kCommand,u8"数据采集","Data sampling","skeeto_hud_pause");
    child(audio,kSlider,u8"分类音量开关","Category mixer enabled","skeeto_audio_mixer_enabled 0 1 1");
    const char* names[]={"gunfire","weapon_handling","melee","chainsaw","footstep","survivor_voice","special_infected","common_infected","tank","witch","explosion","throwable","ambient","ui","vehicle","item","script_cue","hit_sfx"};
    const char* zh[]={u8"枪声",u8"武器操作",u8"近战",u8"电锯",u8"脚步",u8"生还者语音",u8"特殊感染者",u8"普通感染者",u8"Tank",u8"Witch",u8"爆炸",u8"投掷物",u8"环境音",u8"界面交互",u8"载具与救援",u8"物品",u8"弹药提示音",u8"命中反馈"};
    const char* en[]={"Gunfire","Weapon handling","Melee","Chainsaw","Footsteps","Survivor voice","Special infected","Common infected","Tank","Witch","Explosions","Throwables","Ambient","Interface","Vehicles / rescue","Items","Ammo cues","Hit feedback"};
    for(int i=0;i<18;++i){char param[kParamBytes]{};std::snprintf(param,sizeof(param),"skeeto_audio_%s_volume 0 100 1",names[i]);child(audio,kSlider,zh[i],en[i],param);}
    child(audio,kSlider,u8"安全屋结算","Safe-room results","skeeto_safe_music_volume 0 100 1");
    child(audio,kSlider,u8"雨爱","Rainie Love","skeeto_last_music_volume 0 100 1");
    child(common,kPinnedSlider,"sv_cheats","sv_cheats","sv_cheats 0 1 1");
    child(common,kPinnedSlider,u8"开枪后座抖动","Vertical gun punch","z_gun_vertical_punch 0 1 0.05");
    child(common,kPinnedSlider,u8"第一人称视场角","View FOV","fov_desired 75 120 1");
    child(common,kPinnedSlider,u8"手臂视场角","Viewmodel FOV","cl_viewmodelfovsurvivor 30 120 1");
    child(common,kPinnedSlider,u8"显示第一人称模型","Draw viewmodel","r_drawviewmodel 0 1 1");
    child(common,kPinnedSlider,u8"显示 FPS","FPS display","cl_showfps 0 2 1");
    child(common,kPinnedSlider,u8"手电射角","Beam FOV","r_flashlightfov 30 160 1");
    child(common,kPinnedSlider,u8"手电射程","Beam range","r_flashlightfar 100 6000 50");
    child(common,kPinnedSlider,u8"手电恒定光照","Constant beam light","r_flashlightconstant 0 2 0.05");
    child(common,kPinnedSlider,u8"手电亮度","Beam brightness","r_flashlightbrightness 0 2 0.05");
    child(common,kPinnedSlider,u8"手电跟随手模","Attach beam to viewmodel","r_flashlight_attach_to_viewmodel 0 1 1");
    child(common,kPinnedSlider,u8"手电动态阴影距离","Dynamic shadow range","cl_max_shadow_renderable_dist 0 3000 50");
    child(common,kPinnedSlider,u8"手电枪口闪光","Flashlight muzzle flash","r_flashlightmuzzleflash 0 1 1");
    child(common,kPinnedSlider,u8"雾效果","Fog enabled","fog_enable 0 1 1");
    child(common,kPinnedSlider,u8"覆盖地图雾设置","Fog override","fog_override -1 1 1");
    child(common,kPinnedSlider,u8"雾起始距离","Fog start distance","fog_start 0 5000 50");
    child(common,kPinnedSlider,u8"雾结束距离","Fog end distance","fog_end 0 10000 50");
    child(common,kPinnedSlider,u8"雾最大密度","Maximum fog density","fog_maxdensity 0 1 0.05");
    child(common,kPinnedSlider,u8"雨粒子密度","Rain particle density","r_RainParticleDensity 0 1 0.05");
    child(common,kPinnedSlider,u8"雪粒子数量","Snow particle count","r_SnowParticles 0 500 25");
    child(common,kPinnedSlider,u8"显示粒子","Draw particles","r_drawparticles 0 1 1");
    child(common,kPinnedSlider,u8"渲染距离（-1 跟随地图）","Far clip (-1: map default)","r_farz -1 20000 1");
    child(common,kPinnedSlider,u8"颜色校正","Color correction","mat_colorcorrection 0 1 1");
    child(common,kPinnedSlider,u8"画面亮度","Monitor gamma","mat_monitorgamma 1.8 2.6 0.05");
    child(common,kPinnedSlider,u8"全亮模式","Fullbright","mat_fullbright 0 2 1");
    child(common,kPinnedSlider,u8"网络图表","Network graph","net_graph 0 4 1");
    return l;
}
inline char HexDigit(unsigned value){return "0123456789abcdef"[value&15];}
inline void Hex(const char* source,char* out){size_t n=0;for(;source[n];++n){unsigned c=(unsigned char)source[n];out[n*2]=HexDigit(c>>4);out[n*2+1]=HexDigit(c);}if(!n){out[0]='-';out[1]=0;}else out[n*2]=0;}
inline bool Unhex(const char* text,char* out,size_t cap=kLabelBytes){
    if(!std::strcmp(text,"-")){out[0]=0;return true;}
    size_t n=std::strlen(text);if((n&1)||n>=cap*2)return false;
    auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(size_t i=0;i<n;i+=2){int a=digit(text[i]),b=digit(text[i+1]);if(a<0||b<0||!(a*16+b))return false;out[i/2]=(char)(a*16+b);}
    out[n/2]=0;return LabelValid(out,cap);
}
inline size_t Format(const Layout& l,char* out,size_t cap){
    if(!Valid(l)||cap<128)return 0;
    int head=std::snprintf(out,cap,"SKEETO_ESC_UI 8\nstyle %d %d %d %d %d %d %d\ncount %d\n",l.accent,l.backdrop,l.titlePx,l.bodyPx,l.background,l.opacity,l.transparentBorder,l.count);
    if(head<0||(size_t)head>=cap)return 0;size_t used=(size_t)head;
    for(int i=0;i<l.count;++i){const auto& b=l.items[i];char zh[kLabelBytes*2]{},en[kLabelBytes*2]{},param[kParamBytes*2]{};Hex(b.zh,zh);Hex(b.en,en);Hex(b.parameter,param);
        int n=std::snprintf(out+used,cap-used,"item %d %d %d %d %d %d %d %d %d %d %d %d %d\nzh %s\nen %s\nparam %s\n",b.action,b.visible?1:0,b.font,b.color,b.x,b.y,b.w,b.h,b.parent,b.background,b.opacity,b.textRgb,i,zh,en,param);
        if(n<0||(size_t)n>=cap-used)return 0;used+=(size_t)n;
    }
    return used;
}
inline bool Numbers(const char* line,const char* name,int* values,int count){
    const size_t n=std::strlen(name);if(std::strncmp(line,name,n)||line[n]!=' ')return false;
    const char* p=line+n;
    for(int i=0;i<count;++i){if(*p!=' ')return false;while(*p==' ')++p;char* end=nullptr;errno=0;
        long v=std::strtol(p,&end,10);if(end==p||errno==ERANGE||v<INT_MIN||v>INT_MAX)return false;values[i]=(int)v;p=end;}
    while(*p==' ')++p;return !*p;
}
inline bool Parse(const char* source,size_t bytes,Layout& out){
    if(!source||!bytes||bytes>=kFileBytes||std::memchr(source,0,bytes))return false;
    Layout l{};size_t at=0;char line[600]{};
    auto read=[&](){size_t start=at;while(at<bytes&&source[at]!='\n')++at;
        size_t n=at-start;if(at==bytes||n>=sizeof(line))return false;++at;
        if(n&&source[start+n-1]=='\r')--n;std::memcpy(line,source+start,n);line[n]=0;return true;};
    if(!read()||std::strcmp(line,"SKEETO_ESC_UI 8"))return false;
    int v[13]{};if(!read()||!Numbers(line,"style",v,7))return false;
    l.accent=v[0];l.backdrop=v[1];l.titlePx=v[2];l.bodyPx=v[3];l.background=v[4];l.opacity=v[5];l.transparentBorder=v[6];
    if(!read()||!Numbers(line,"count",v,1)||v[0]<1||v[0]>kMaxItems)return false;l.count=v[0];
    for(int i=0;i<l.count;++i){auto& b=l.items[i];
        if(!read()||!Numbers(line,"item",v,13)||v[12]!=i||(v[1]!=0&&v[1]!=1))return false;
        b.action=v[0];b.visible=v[1]!=0;b.font=v[2];b.color=v[3];b.x=v[4];b.y=v[5];b.w=v[6];b.h=v[7];b.parent=v[8];b.background=v[9];b.opacity=v[10];b.textRgb=v[11];
        if(!read()||std::strncmp(line,"zh ",3)||!Unhex(line+3,b.zh))return false;
        if(!read()||std::strncmp(line,"en ",3)||!Unhex(line+3,b.en))return false;
        if(!read()||std::strncmp(line,"param ",6)||!Unhex(line+6,b.parameter,kParamBytes))return false;
    }
    if(at!=bytes||!Valid(l))return false;out=l;return true;
}
}
