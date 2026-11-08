#pragma once
#include "items.h"
#include "safehouse_target.h"

// Read-only hints from the mounted BSP's original entity lump. These are initial
// map flags, not a claim that a server plugin cannot later change pickup rules.
// Runtime callers must restrict hints to actual CWeaponSpawn entities, with the
// already verified weapon ID, and must pass the network (parent-local) origin.
namespace map_supply {
enum Hint {Unknown,Finite,Unlimited};
struct RawHint {unsigned label;Vec networkOrigin;const char* parentModel;};
constexpr unsigned MaxSupplies=512,MaxParents=1024,MaxEntities=16384,MaxBytes=8*1024*1024;
inline bool White(unsigned char c){return c==' '||c=='\t'||c=='\n'||c=='\r';}
inline char Lower(char c){return c>='A'&&c<='Z'?char(c+32):c;}
inline bool Equal(const char* a,const char* b){if(!a||!b)return a==b;while(*a&&Lower(*a)==Lower(*b)){++a;++b;}return *a==*b;}
inline float Abs(float x){return x<0?-x:x;}
inline bool Position(Vec p){return ::Finite(p.x)&&::Finite(p.y)&&::Finite(p.z)&&Abs(p.x)<1000000&&Abs(p.y)<1000000&&Abs(p.z)<1000000;}
inline bool Near(Vec a,Vec b){Vec d=a-b;return d.Dot(d)<=1.f;}
inline bool Integer(const char* s,unsigned& value){
    if(!s||!*s)return false;unsigned n=0;
    for(;*s;++s){if(*s<'0'||*s>'9')return false;unsigned digit=unsigned(*s-'0');if(n>(0xffffffffu-digit)/10)return false;n=n*10+digit;}
    value=n;return true;
}
inline bool Number(const char*& s,float& value){
    while(White((unsigned char)*s))++s;bool negative=false;if(*s=='+'||*s=='-'){negative=*s=='-';++s;}
    float v=0;unsigned digits=0;while(*s>='0'&&*s<='9'){if(++digits>9)return false;v=v*10+float(*s++-'0');}
    if(*s=='.'){++s;float scale=.1f;while(*s>='0'&&*s<='9'){if(++digits>12)return false;v+=float(*s++-'0')*scale;scale*=.1f;}}
    if(!digits)return false;if(*s=='e'||*s=='E'){++s;bool down=false;if(*s=='+'||*s=='-'){down=*s=='-';++s;}unsigned power=0,exponentDigits=0;
        while(*s>='0'&&*s<='9'){if(++exponentDigits>8)return false;power=power*10+unsigned(*s++-'0');}if(!exponentDigits||power>38)return false;
        for(unsigned i=0;i<power;++i)v=down?v*.1f:v*10.f;}
    if(*s&&!White((unsigned char)*s))return false;value=negative?-v:v;return ::Finite(value)&&Abs(value)<1000000;
}
inline bool Vector(const char* text,Vec& value){if(!text)return false;const char* at=text;Vec v{};if(!Number(at,v.x)||!Number(at,v.y)||!Number(at,v.z))return false;while(White((unsigned char)*at))++at;if(*at||!Position(v))return false;value=v;return true;}
inline bool Brush(const char* text){unsigned n;return text&&*text=='*'&&Integer(text+1,n)&&n>0;}

struct Token {const unsigned char* data;unsigned size;};
struct Reader {
    const unsigned char* data;unsigned size,at=0;bool ok=true;
    void Space(){while(at<size&&White(data[at]))++at;}
    bool Quote(Token& out){Space();if(at>=size||data[at++]!='"'){ok=false;return false;}unsigned start=at;
        while(at<size){unsigned char c=data[at++];if(c=='"'){out={data+start,at-start-1};return true;}if(!c){ok=false;return false;}if(c=='\\'){if(at>=size||!data[at]){ok=false;return false;}++at;}}
        ok=false;return false;
    }
    bool Copy(Token token,char* out,unsigned cap){unsigned used=0;for(unsigned i=0;i<token.size;++i){unsigned char c=token.data[i];if(c=='\\'){
            if(++i>=token.size){ok=false;return false;}c=token.data[i];if(c!='\\'&&c!='"'){ok=false;return false;}}
            if(c<32||used+1>=cap){ok=false;return false;}out[used++]=char(c);}
        out[used]=0;return true;
    }
    bool End(){Space();if(at<size&&data[at]==0)++at;Space();return ok&&at==size;}
};
struct Entity {
    char classname[64]{},selection[64]{},parent[128]{},name[128]{},model[384]{},glow[128]{},nextMap[240]{},landmark[128]{};
    Vec origin{},angles{};unsigned flags=0,laser=0,incendiary=0,explosive=0,giveMask=0;float wait=-1;bool hasOrigin=false,hasAngles=false;unsigned seen=0;
};
// Read this one known output as data. Never execute or infer arbitrary scripts.
inline bool MatchOutput(Token token,const char* expected){
    unsigned at=0;for(unsigned i=0;i<token.size;++i){unsigned char c=token.data[i];if(White(c))continue;if(!expected[at]||c!=(unsigned char)expected[at++])return false;}return !expected[at];
}
inline unsigned GiveOutput(Token token){
    if(MatchOutput(token,"!activator\x1bRunScriptCode\x1bGiveUpgrade(self,4)\x1b" "0\x1b-1"))return 9;
    if(MatchOutput(token,"!activator\x1bRunScriptCode\x1bself.GiveItem(WeaponNames.m60)\x1b" "0.01\x1b-1"))return 21;
    if(MatchOutput(token,"!activator\x1bRunScriptCode\x1bself.GiveItem(WeaponNames.grenadelauncher)\x1b" "0.01\x1b-1"))return 20;
    return 0;
}
// Unknown key/value strings are scanned without copying, so long script outputs
// do not overflow a token buffer. Relevant duplicate/malformed keys fail closed.
inline bool ReadEntity(Reader& r,Entity& e){
    r.Space();if(r.at>=r.size||r.data[r.at++]!='{'){r.ok=false;return false;}unsigned pairs=0;
    for(;;){r.Space();if(r.at>=r.size){r.ok=false;return false;}if(r.data[r.at]=='}'){++r.at;return true;}if(++pairs>4096){r.ok=false;return false;}
        Token key{},value{};if(!r.Quote(key)||!r.Quote(value))return false;
        char k[64]{};if(key.size>=sizeof k)continue;if(!r.Copy(key,k,sizeof k))return false;
        if(Equal(k,"OnPressed")){unsigned label=GiveOutput(value);if(label)e.giveMask|=1u<<label;continue;}
        unsigned bit=0;char* out=nullptr;unsigned cap=0;
        if(Equal(k,"map")){bit=8192;out=e.nextMap;cap=sizeof e.nextMap;}
        else if(Equal(k,"landmark")){bit=16384;out=e.landmark;cap=sizeof e.landmark;}
        else if(Equal(k,"classname")){bit=1;out=e.classname;cap=sizeof e.classname;}
        else if(Equal(k,"weapon_selection")){bit=2;out=e.selection;cap=sizeof e.selection;}
        else if(Equal(k,"parentname")){bit=4;out=e.parent;cap=sizeof e.parent;}
        else if(Equal(k,"targetname")){bit=8;out=e.name;cap=sizeof e.name;}
        else if(Equal(k,"model")){bit=16;out=e.model;cap=sizeof e.model;}
        else if(Equal(k,"origin"))bit=32;
        else if(Equal(k,"angles"))bit=64;
        else if(Equal(k,"spawnflags"))bit=128;
        else if(Equal(k,"laser_sight"))bit=256;
        else if(Equal(k,"upgradepack_incendiary"))bit=512;
        else if(Equal(k,"upgradepack_explosive"))bit=1024;
        else if(Equal(k,"glow")){bit=2048;out=e.glow;cap=sizeof e.glow;}
        else if(Equal(k,"wait"))bit=4096;
        if(!bit)continue;if(e.seen&bit){r.ok=false;return false;}e.seen|=bit;
        if(out){if(!r.Copy(value,out,cap))return false;}
        else {char text[96]{};if(!r.Copy(value,text,sizeof text))return false;
            if(bit==32){if(!Vector(text,e.origin)){r.ok=false;return false;}e.hasOrigin=true;}
            else if(bit==64){if(!Vector(text,e.angles)){r.ok=false;return false;}e.hasAngles=true;}
            else if(bit==4096){const char* at=text;if(!Number(at,e.wait)){r.ok=false;return false;}}
            else {unsigned* destination=bit==128?&e.flags:bit==256?&e.laser:bit==512?&e.incendiary:&e.explosive;if(!Integer(text,*destination)){r.ok=false;return false;}}}
    }
}
inline unsigned Label(const char* name){
    struct Definition {const char* name;unsigned label;};
    static const Definition definitions[]={
        {"weapon_first_aid_kit_spawn",1},{"weapon_defibrillator_spawn",2},{"weapon_pain_pills_spawn",3},{"weapon_adrenaline_spawn",4},
        {"weapon_molotov_spawn",5},{"weapon_pipe_bomb_spawn",6},{"weapon_vomitjar_spawn",7},
        {"weapon_rifle_ak47_spawn",11},{"weapon_rifle_spawn",12},{"weapon_sniper_military_spawn",13},{"weapon_hunting_rifle_spawn",14},
        {"weapon_smg_silenced_spawn",15},{"weapon_smg_spawn",16},{"weapon_sniper_awp_spawn",17},{"weapon_sniper_scout_spawn",18},
        {"weapon_rifle_desert_spawn",19},{"weapon_grenade_launcher_spawn",20},{"weapon_rifle_m60_spawn",21},
        {"weapon_pumpshotgun_spawn",24},{"weapon_shotgun_chrome_spawn",25},{"weapon_autoshotgun_spawn",26},{"weapon_shotgun_spas_spawn",27},
        {"weapon_smg_mp5_spawn",28},{"weapon_rifle_sg552_spawn",29}};
    for(const auto& d:definitions)if(Equal(name,d.name))return d.label;return 0;
}
inline unsigned Label(const Entity& e){unsigned label=Label(e.classname);if(label)return label;
    if(Equal(e.classname,"upgrade_laser_sight"))return 9;
    // Mixed director pools provide no evidence of the runtime upgrade type.
    if(Equal(e.classname,"upgrade_spawn")&&(e.seen&1792)==1792&&e.laser==1&&!e.incendiary&&!e.explosive)return 9;
    if((Equal(e.classname,"prop_dynamic")||Equal(e.classname,"prop_dynamic_override"))&&e.name[0]){
        if(items::LaserModel(e.model))return 9;
        if(Equal(e.model,"models/w_models/weapons/w_m60.mdl"))return 21;
        if(Equal(e.model,"models/w_models/weapons/w_grenade_launcher.mdl"))return 20;
    }
    if(Equal(e.classname,"weapon_spawn")&&e.selection[0]){char name[80]{};unsigned n=TextLength(e.selection);if(n+7>=sizeof name)return 0;
        CopyText(name,sizeof name,e.selection);CopyText(name+n,sizeof name-n,"_spawn");return Label(name);}return 0;
}
struct Map {
    struct Supply {unsigned label,flags;Vec position;char parent[128],parentModel[32],name[128];bool usable,displayItem,verifiedButton;};
    struct Parent {char name[128],model[32];Vec origin;unsigned matches;bool usable;};
    Supply records[MaxSupplies]{};Parent parentRecords[MaxParents]{};safehouse_target::Map checkpoint;
    unsigned supplies=0,parents=0,entities=0,buttonDisplays=0;bool loaded=false;
    void Clear(){supplies=parents=entities=buttonDisplays=0;loaded=false;checkpoint.Clear();}
    bool Fail(){Clear();return false;}
    bool Parse(const unsigned char* data,unsigned size){
        Clear();if(!data||!size||size>MaxBytes)return false;Reader r{data,size};
        while(true){r.Space();if(r.at>=r.size||r.data[r.at]==0)break;Entity e{};if(++entities>MaxEntities||!ReadEntity(r,e))return Fail();
            checkpoint.Add(e.classname,e.name,e.origin,e.hasOrigin,e.parent,e.nextMap,e.landmark,e.flags);
            unsigned label=Label(e);if(!label||!e.hasOrigin)continue;if(supplies>=MaxSupplies)return Fail();auto& s=records[supplies++];s={};
            s.label=label;s.flags=e.flags;s.position=e.origin;s.usable=!e.parent[0];CopyText(s.parent,sizeof s.parent,e.parent);CopyText(s.name,sizeof s.name,e.name);
            s.displayItem=Equal(e.classname,"prop_dynamic")||Equal(e.classname,"prop_dynamic_override");}
        if(!r.End()||!entities)return Fail();
        // Resolve only names referenced by supply records, counting all matches.
        r={data,size};while(true){r.Space();if(r.at>=r.size||r.data[r.at]==0)break;Entity e{};if(!ReadEntity(r,e))return Fail();
            if(Equal(e.classname,"func_button")&&e.giveMask&&e.hasOrigin&&(e.flags&1024)&&e.wait>=0&&!e.parent[0]&&e.glow[0]){
                for(unsigned i=0;i<supplies;++i){auto& s=records[i];Vec d=s.position-e.origin;if(s.displayItem&&(e.giveMask&(1u<<s.label))&&Equal(s.name,e.glow)&&d.Dot(d)<=96.f*96.f){s.verifiedButton=true;s.flags|=8;}}
            }
            if(!e.name[0])continue;
            bool referenced=false;for(unsigned i=0;i<supplies;++i)if(records[i].parent[0]&&Equal(records[i].parent,e.name)){referenced=true;break;}if(!referenced)continue;
            unsigned index=0;while(index<parents&&!Equal(parentRecords[index].name,e.name))++index;
            if(index==parents){if(parents>=MaxParents)return Fail();auto& p=parentRecords[parents++];p={};CopyText(p.name,sizeof p.name,e.name);CopyText(p.model,sizeof p.model,e.model);p.origin=e.origin;
                p.usable=e.hasOrigin&&Brush(e.model)&&!e.parent[0]&&Abs(e.angles.x)<.001f&&Abs(e.angles.y)<.001f&&Abs(e.angles.z)<.001f;}
            ++parentRecords[index].matches;
        }
        if(!r.End())return Fail();
        // A brush model used by multiple named parents cannot identify which
        // parent the network handle refers to; even matching local coordinates
        // must stay unknown in that case.
        for(unsigned i=0;i<parents;++i)for(unsigned j=i+1;j<parents;++j)if(Brush(parentRecords[i].model)&&Equal(parentRecords[i].model,parentRecords[j].model)){parentRecords[i].usable=false;parentRecords[j].usable=false;}
        for(unsigned i=0;i<supplies;++i){auto& s=records[i];if(!s.parent[0])continue;
            for(unsigned j=0;j<parents;++j){const auto& p=parentRecords[j];if(Equal(s.parent,p.name)&&p.usable&&p.matches==1){s.position=s.position-p.origin;s.usable=Position(s.position);CopyText(s.parentModel,sizeof s.parentModel,p.model);break;}}}
        for(unsigned i=0;i<supplies;++i)if(records[i].displayItem&&!records[i].verifiedButton)records[i].usable=false;
        // Duplicate display names make the button target ambiguous.
        for(unsigned i=0;i<supplies;++i)for(unsigned j=i+1;j<supplies;++j)if(records[i].displayItem&&records[j].displayItem&&Equal(records[i].name,records[j].name)){records[i].usable=records[j].usable=false;}
        for(unsigned i=0;i<supplies;++i)if(records[i].displayItem&&records[i].usable)++buttonDisplays;
        loaded=true;return true;
    }
    Hint Find(const RawHint& h,bool displayOnly=false)const {
        if(!loaded||!h.label||!Position(h.networkOrigin))return Unknown;const char* model=h.parentModel?h.parentModel:"";
        if(*model&&!Brush(model))return Unknown;Hint found=Unknown;bool matched=false;
        for(unsigned i=0;i<supplies;++i){const auto& s=records[i];if(!s.usable||(displayOnly&&!s.displayItem)||s.label!=h.label||!Equal(s.parentModel,model)||!Near(s.position,h.networkOrigin))continue;
            Hint current=(s.flags&8)?Unlimited:Finite;if(matched&&found!=current)return Unknown;found=current;matched=true;}
        return found;
    }
    bool FixedLaser(Vec world)const {
        if(!loaded||!Position(world))return false;
        for(unsigned i=0;i<supplies;++i){const auto& s=records[i];if(s.label!=9||s.displayItem||!s.usable||s.parent[0])continue;
            Vec d=world-s.position;if(d.x*d.x+d.y*d.y<=1.f&&Abs(d.z)<=8.f)return true;}
        return false;
    }
    unsigned DisplayLabel(const RawHint& h)const {
        if(!loaded||!Position(h.networkOrigin))return 0;const char* model=h.parentModel?h.parentModel:"";unsigned found=0;
        for(unsigned i=0;i<supplies;++i){const auto& s=records[i];if(!s.usable||!s.displayItem||!Equal(s.parentModel,model)||!Near(s.position,h.networkOrigin))continue;if(found&&found!=s.label)return 0;found=s.label;}
        return found;
    }
};
}
