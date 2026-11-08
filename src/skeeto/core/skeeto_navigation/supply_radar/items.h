#pragma once
#include "radar_abi.h"
namespace items {
constexpr unsigned kInvalid=0xffffffffu;
enum class Kind {None,Medical,Defibrillator,Pills,Adrenaline,Molotov,PipeBomb,Bile,Ammo,Laser,Weapon};
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
enum class Label {None,Medical,Defibrillator,Pills,Adrenaline,Molotov,PipeBomb,Bile,Ammo,Laser,Weapon,AK,M4,Military,Hunting,MAC,UZI,AWP,Scout,Scar,Grenade,M60,Pistol,Magnum,Pump,Chrome,Auto,Spas,MP5,SG552,Melee,Chainsaw,GasCan,Fireworks,Safehouse,Count};
inline Label DefaultLabel(Kind k){return Label(unsigned(k));}
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
    static const WCHAR* names[]={L"",L"包",L"电",L"药",L"针",L"火",L"雷",L"胆汁",L"子弹",L"激光",L"未知",L"ak",L"m4",L"30",L"15",L"mac",L"uzi",L"awp",L"scout",L"scar",L"榴弹",L"m60",L"手枪",L"沙鹰",L"木喷",L"铁喷",L"连喷",L"spas",L"mp5",L"sg552",L"近战",L"电锯",L"油桶",L"烟花盒",L"终点安全屋"};
    return unsigned(label)<unsigned(Label::Count)?names[unsigned(label)]:L"未知";
}
// Exact model matching: avoid treating unrelated physics props as supplies.
inline bool ModelEquals(const char* model,const char* expected){
    if(!model)return false;
    for(unsigned i=0;;++i){char a=model[i],b=expected[i];
        if(a>='A'&&a<='Z')a+=32;if(a=='\\')a='/';
        if(a!=b)return false;if(!a)return true;
    }
}
inline Label PhysicsSupply(const char* model){
    if(ModelEquals(model,"models/props_junk/gascan001a.mdl")||
       ModelEquals(model,"models/props_industrial/barrel_fuel.mdl")||
       ModelEquals(model,"models/props_c17/oildrum001_explosive.mdl"))return Label::GasCan;
    if(ModelEquals(model,"models/props_junk/explosive_box001.mdl"))return Label::Fireworks;
    return Label::None;
}
inline bool PhysicsClass(const char* name){
    return Same(name,"CPhysicsProp")||Same(name,"CPhysicsPropMultiplayer")||
           Same(name,"CPropPhysics")||Same(name,"CPropPhysicsMultiplayer")||
           Same(name,"CGasCan")||Same(name,"CWeaponGascan")||
           Same(name,"CFireworkCrate");
}
inline bool LaserModel(const char* name){
    const char* expected="models/w_models/weapons/w_laser_sights.mdl";
    for(unsigned i=0;;++i){char c=name[i];if(c>='A'&&c<='Z')c+=32;if(c=='\\')c='/';if(c!=expected[i])return false;if(!c)return true;}
}
// Stock networked model indices only; Skeeto does not provide L4N's custom-model lookup.
inline bool ValidModel(int index){return index>0&&index<=32767;}
inline bool Available(unsigned owner,unsigned weaponOwner,int effects,int model){return (owner==0||owner==kInvalid)&&(weaponOwner==0||weaponOwner==kInvalid)&&!(effects&32)&&ValidModel(model);}
}
