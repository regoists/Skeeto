#pragma once
#include "nav.h"
#include "../skeeto_ui_text.h"
namespace items {
enum class Kind {None,Medical,Defibrillator,Pills,Adrenaline,Molotov,PipeBomb,Bile,Ammo,Laser,Weapon};
constexpr int Capacity=64;
constexpr unsigned AllMask=(1u<<10)-1;
constexpr unsigned Mask(Kind kind){return kind>Kind::None&&kind<=Kind::Weapon?1u<<(unsigned(kind)-1):0;}
constexpr bool Enabled(Kind kind,unsigned mask){return (Mask(kind)&mask)!=0;}
inline Kind FromWeaponID(int id){
    // Verified against the local L4D2 weapon_spawn ID enumeration.
    switch(id){
    case 12:return Kind::Medical;case 24:return Kind::Defibrillator;
    case 15:return Kind::Pills;case 23:return Kind::Adrenaline;
    case 13:return Kind::Molotov;case 14:return Kind::PipeBomb;case 25:return Kind::Bile;
    case 22:case 54:return Kind::Ammo;
    default:break;
    }
    if((id>=1&&id<=11)||id==19||id==20||id==21||id==26||(id>=32&&id<=37))return Kind::Weapon;
    return Kind::None;
}
inline Kind FromClass(const char* name){
    if(Same(name,"CFirstAidKit"))return Kind::Medical;
    if(Same(name,"CItemDefibrillator"))return Kind::Defibrillator;
    if(Same(name,"CPainPills"))return Kind::Pills;
    if(Same(name,"CItem_Adrenaline"))return Kind::Adrenaline;
    if(Same(name,"CMolotov"))return Kind::Molotov;
    if(Same(name,"CPipeBomb"))return Kind::PipeBomb;
    if(Same(name,"CItem_VomitJar"))return Kind::Bile;
    if(Same(name,"CWeaponAmmoSpawn")||Same(name,"CItemAmmoPack"))return Kind::Ammo;
    const char* guns[]={"CAssaultRifle","CAutoShotgun","CChainsaw","CGrenadeLauncher","CMagnumPistol","CPistol","CPumpShotgun","CRifle_AK47","CRifle_Desert","CRifle_M60","CRifle_SG552","CShotgun_Chrome","CShotgun_SPAS","CSMG_MP5","CSMG_Silenced","CSniper_AWP","CSniper_Military","CSniper_Scout","CSniperRifle","CSubMachinegun","CTerrorMeleeWeapon"};
    for(auto gun:guns)if(Same(name,gun))return Kind::Weapon;return Kind::None;
}
// Text IDs also select independent weapon filters. The category switch remains a master.
enum class Label {None,Medical,Defibrillator,Pills,Adrenaline,Molotov,PipeBomb,Bile,Ammo,Laser,Weapon,AK,M4,Military,Hunting,MAC,UZI,AWP,Scout,Scar,Grenade,M60,Pistol,Magnum,Pump,Chrome,Auto,Spas,MP5,SG552,Melee,Chainsaw,Count};
inline Label DefaultLabel(Kind k){return Label(unsigned(k));}
constexpr unsigned WeaponCount=unsigned(Label::Count)-unsigned(Label::Weapon);
constexpr unsigned AllWeapons=(1u<<WeaponCount)-1;
inline bool WeaponEnabled(Label label,unsigned mask){unsigned index=unsigned(label)-unsigned(Label::Weapon);return index<WeaponCount&&(mask&(1u<<index));}
struct WeaponDefinition {int id;const char* classname;Label label;};
static const WeaponDefinition weapons[]={
    {26,"CRifle_AK47",Label::AK},
    {5,"CAssaultRifle",Label::M4},
    {10,"CSniper_Military",Label::Military},
    {6,"CSniperRifle",Label::Hunting},
    {7,"CSMG_Silenced",Label::MAC},
    {2,"CSubMachinegun",Label::UZI},
    {35,"CSniper_AWP",Label::AWP},
    {36,"CSniper_Scout",Label::Scout},
    {9,"CRifle_Desert",Label::Scar},
    {21,"CGrenadeLauncher",Label::Grenade},
    {37,"CRifle_M60",Label::M60},
    {1,"CPistol",Label::Pistol},
    {32,"CMagnumPistol",Label::Magnum},
    {3,"CPumpShotgun",Label::Pump},
    {8,"CShotgun_Chrome",Label::Chrome},
    {4,"CAutoShotgun",Label::Auto},
    {11,"CShotgun_SPAS",Label::Spas},
    {33,"CSMG_MP5",Label::MP5},
    {34,"CRifle_SG552",Label::SG552},
    {19,"CTerrorMeleeWeapon",Label::Melee},
    {20,"CChainsaw",Label::Chainsaw},
};
inline Label WeaponLabel(int id){for(const auto& w:weapons)if(w.id==id)return w.label;return DefaultLabel(FromWeaponID(id));}
inline Label ClassLabel(const char* name){for(const auto& w:weapons)if(Same(name,w.classname))return w.label;return DefaultLabel(FromClass(name));}
inline const WCHAR* Text(Label label){
    static const UiTextEntry names[]={
        {UiId::NavItem0, L""},
        {UiId::NavItem1, L"包"},
        {UiId::NavItem2, L"电"},
        {UiId::NavItem3, L"药"},
        {UiId::NavItem4, L"针"},
        {UiId::NavItem5, L"火"},
        {UiId::NavItem6, L"雷"},
        {UiId::NavItem7, L"胆汁"},
        {UiId::NavItem8, L"子弹"},
        {UiId::NavItem9, L"激光"},
        {UiId::NavItem10, L"未知"},
        {UiId::NavItem11, L"ak"},
        {UiId::NavItem12, L"m4"},
        {UiId::NavItem13, L"30"},
        {UiId::NavItem14, L"15"},
        {UiId::NavItem15, L"mac"},
        {UiId::NavItem16, L"uzi"},
        {UiId::NavItem17, L"awp"},
        {UiId::NavItem18, L"scout"},
        {UiId::NavItem19, L"scar"},
        {UiId::NavItem20, L"榴弹"},
        {UiId::NavItem21, L"m60"},
        {UiId::NavItem22, L"手枪"},
        {UiId::NavItem23, L"沙鹰"},
        {UiId::NavItem24, L"木喷"},
        {UiId::NavItem25, L"铁喷"},
        {UiId::NavItem26, L"连喷"},
        {UiId::NavItem27, L"spas"},
        {UiId::NavItem28, L"mp5"},
        {UiId::NavItem29, L"sg552"},
        {UiId::NavItem30, L"近战"},
        {UiId::NavItem31, L"电锯"},
    };
    static_assert(sizeof(names)/sizeof(names[0])==unsigned(Label::Count),"item labels");
    const unsigned index=unsigned(label)<unsigned(Label::Count)?unsigned(label):unsigned(Label::Weapon);
    return UiText(names[index].id,names[index].chinese);
}
inline bool LaserModel(const char* name){
    const char* expected="models/w_models/weapons/w_laser_sights.mdl";
    for(unsigned i=0;;++i){char c=name[i];if(c>='A'&&c<='Z')c+=32;if(c=='\\')c='/';if(c!=expected[i])return false;if(!c)return true;}
}
// Keep negative custom-model indices visible to the optional model interface.
inline bool ValidModel(int index){return index>=-32768&&index<=32767&&index!=0&&index!=-1;}
inline bool Available(unsigned owner,unsigned weaponOwner,int effects,int model){return (owner==0||owner==nav::Invalid)&&(weaponOwner==0||weaponOwner==nav::Invalid)&&!(effects&32)&&ValidModel(model);}
struct Marker {Vec position;Kind kind;int entity;float distance;Label label=Label::None;};
struct Snapshot {
    Marker markers[Capacity]{};int count=0;bool available=true;
    void Add(Vec p,Kind kind,int entity,Vec player,float range,float layerZ,float band,bool layering,unsigned mask,Label label=Label::None,unsigned weapons=AllWeapons){
        if(label==Label::None)label=DefaultLabel(kind);
        if(kind==Kind::Weapon&&!WeaponEnabled(label,weapons))return;
        if(!Enabled(kind,mask)||nav::Abs(p.z-layerZ)>(layering?band:1000000.f))return;
        Vec delta=p-player;float distance=delta.Dot(delta);if(distance>range*range)return;
        int at=count;while(at>0&&markers[at-1].distance>distance)--at;if(at>=Capacity)return;
        if(count<Capacity)++count;for(int i=count-1;i>at;--i)markers[i]=markers[i-1];markers[at]={p,kind,entity,distance,label==Label::None?DefaultLabel(kind):label};
    }
    bool Filter(unsigned mask,unsigned weapons=AllWeapons){
        int kept=0;for(int i=0;i<count;++i)if(Enabled(markers[i].kind,mask)&&(markers[i].kind!=Kind::Weapon||WeaponEnabled(markers[i].label,weapons)))markers[kept++]=markers[i];
        bool changed=kept!=count;count=kept;return changed;
    }
    bool Matches(const Snapshot& other)const{
        if(count!=other.count||available!=other.available)return false;
        for(int i=0;i<count;++i){const auto& a=markers[i];const auto& b=other.markers[i];Vec d=a.position-b.position;
            if(a.kind!=b.kind||a.label!=b.label||a.entity!=b.entity||d.Dot(d)>1)return false;
        }return true;
    }
};
}
