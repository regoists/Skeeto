#pragma once
#include "../sdk.h"
#include <cstdio>

// SupplyRadar 1.3.6 option set, stored in Skeeto's ini by the proxy. Index
// positions are mirrored by skeeto_navigation.h (kRadarItemBase/LayoutBase/SettingCount).
namespace supplyradar { namespace options {
constexpr unsigned LabelCount=35;
enum Index {Enable,Range,ShowCount,Scale,Opacity,Direction,Height,Spectator,MapMetadata,DigitFont,Rows,Columns,
    Medical,Throwables,Weapons,ItemBase,LayoutBase=ItemBase+LabelCount,Count=LayoutBase+LabelCount*4};
inline int values[Count]{};
// Labels with no confirmed pickup behaviour (generic weapon, pistol, magnum,
// melee, chainsaw) stay unsupported, as in the upstream option table.
inline bool Supported(unsigned id){return id>=1&&id<LabelCount&&id!=10&&id!=22&&id!=23&&id!=30&&id!=31;}
// 0 medical, 1 throwables, 2 weapons, 3 individually switched (ammo, laser, gas can, fireworks, safehouse).
inline int Group(unsigned id){if(!Supported(id))return -1;return id>=32?3:id<=4?0:id<=7?1:id>=11?2:3;}
inline bool EnabledId(unsigned id){
    int group=Group(id);if(group<0||!values[ItemBase+id])return false;
    return group==3||values[group==0?Medical:group==1?Throwables:Weapons]!=0;
}
inline bool AnyEnabled(){for(unsigned id=1;id<LabelCount;++id)if(EnabledId(id))return true;return false;}
inline int DigitStyle(){return Limit(values[DigitFont],0,2);}
inline int Minimum(int index){
    if(index==Range)return 5;if(index==Scale)return 70;if(index==Rows)return 3;if(index==Columns)return 1;
    if(index>=LayoutBase)return (index-LayoutBase)%4==3?50:0;return 0;
}
inline int Maximum(int index){
    if(index==Range)return 120;if(index==Scale)return 180;if(index==Opacity)return 100;if(index==DigitFont)return 2;
    if(index==Rows)return 16;if(index==Columns)return 3;
    if(index>=LayoutBase){int field=(index-LayoutBase)%4;return field==3?200:field==0?1:10000;}
    return 1;
}
inline int Step(int index){return index==Range||index==Scale||index==Opacity?5:1;}
inline void Set(int index,int value){
    if(index<0||index>=Count)return;
    if(index>=ItemBase&&index<LayoutBase&&!Supported(unsigned(index-ItemBase)))value=0;
    values[index]=Limit(value,Minimum(index),Maximum(index));
}
inline void Reset(){
    for(auto& v:values)v=0;
    values[Range]=40;values[ShowCount]=values[Direction]=values[Height]=values[Spectator]=1;
    values[Scale]=values[Opacity]=100;values[Rows]=8;values[Columns]=2;
    values[Medical]=values[Throwables]=values[Weapons]=1;
    for(unsigned id=0;id<LabelCount;++id){
        // Gas cans and fireworks are new, optional world supplies; everything else starts enabled.
        values[ItemBase+id]=Supported(id)&&id!=32&&id!=33?1:0;
        values[LayoutBase+id*4+3]=100;
    }
}
inline const char* Key(int index){
    if(index<0||index>=Count)return nullptr;
    static char keys[Count][48]{};
    if(!keys[0][0]){
        static const char* base[]={"enabled","range","count","scale","opacity","direction","height","spectator","map_metadata","digit_font","rows","columns","medical","throwables","weapons"};
        for(int i=0;i<ItemBase;++i)std::snprintf(keys[i],sizeof keys[i],"supply_radar_%s",base[i]);
        static const char* field[]={"custom","x","y","scale"};
        for(unsigned id=0;id<LabelCount;++id){
            std::snprintf(keys[ItemBase+id],sizeof keys[0],"supply_radar_item_%02u",id);
            for(int f=0;f<4;++f)std::snprintf(keys[LayoutBase+id*4+f],sizeof keys[0],"supply_radar_layout_%02u_%s",id,field[f]);
        }
    }
    return keys[index];
}
struct Defaults { Defaults(){Reset();} }; inline Defaults defaults;
} }
