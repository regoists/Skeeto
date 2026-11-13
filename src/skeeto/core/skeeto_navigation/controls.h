#pragma once
#include "sdk.h"
namespace controls {
struct Settings {
    // `enabled` keeps the existing skeeto_nav_enable config key, but now gates
    // only minimap presentation and minimap-specific scans/composition.
    int enabled=0,size=300,range=1200,rotate=1,teammates=1,posX=0,posY=0,opacity=235,navigation=0,goal=0,items=0,itemRange=1200;
    int itemAmmo=1,itemBile=1,itemMolotov=1,itemPills=1,itemMedkit=1,itemPipeBomb=1,itemAdrenaline=1,itemDefib=1,itemLaser=1,itemWeapons=1;
    int weapons[22]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
    int worldNavigation=1,refreshFps=10,style=0,worldArrowColor=0,worldHudX=5000,worldHudY=1400,shape=0,worldHud=1,edit=0,reload=0;
};
struct Spec {const char* name;const WCHAR* key;int minimum,maximum;};
static constexpr Spec specs[]={
    {"skeeto_nav_enable",L"Enabled",0,1},{"skeeto_nav_size",L"Size",180,480},
    {"skeeto_nav_range",L"Range",600,4200},{"skeeto_nav_rotate",L"Rotate",0,1},
    {"skeeto_nav_teammates",L"Teammates",0,1},{"skeeto_nav_x",L"X",0,10000},
    {"skeeto_nav_y",L"Y",0,10000},
    {"skeeto_nav_opacity",L"Opacity",80,255},
    {"skeeto_nav_navigation",L"Navigation",0,1},{"skeeto_nav_goal",L"Goal",0,16},
    {"skeeto_nav_items",L"Items",0,1},{"skeeto_nav_item_range",L"ItemRange",300,2400},
    {"skeeto_nav_item_ammo",L"ItemAmmo",0,1},{"skeeto_nav_item_bile",L"ItemBile",0,1},
    {"skeeto_nav_item_molotov",L"ItemMolotov",0,1},{"skeeto_nav_item_pills",L"ItemPills",0,1},
    {"skeeto_nav_item_medkit",L"ItemMedkit",0,1},{"skeeto_nav_item_pipebomb",L"ItemPipeBomb",0,1},
    {"skeeto_nav_item_adrenaline",L"ItemAdrenaline",0,1},{"skeeto_nav_item_defib",L"ItemDefib",0,1},
    {"skeeto_nav_item_laser",L"ItemLaser",0,1},{"skeeto_nav_item_weapons",L"ItemWeapons",0,1},
    {"skeeto_nav_weapon_unknown",L"WeaponUnknown",0,1},
    {"skeeto_nav_weapon_ak",L"WeaponAK",0,1},
    {"skeeto_nav_weapon_m4",L"WeaponM4",0,1},
    {"skeeto_nav_weapon_30",L"WeaponMilitary",0,1},
    {"skeeto_nav_weapon_15",L"WeaponHunting",0,1},
    {"skeeto_nav_weapon_mac",L"WeaponMAC",0,1},
    {"skeeto_nav_weapon_uzi",L"WeaponUZI",0,1},
    {"skeeto_nav_weapon_awp",L"WeaponAWP",0,1},
    {"skeeto_nav_weapon_scout",L"WeaponScout",0,1},
    {"skeeto_nav_weapon_scar",L"WeaponScar",0,1},
    {"skeeto_nav_weapon_grenade",L"WeaponGrenade",0,1},
    {"skeeto_nav_weapon_m60",L"WeaponM60",0,1},
    {"skeeto_nav_weapon_pistol",L"WeaponPistol",0,1},
    {"skeeto_nav_weapon_magnum",L"WeaponMagnum",0,1},
    {"skeeto_nav_weapon_pump",L"WeaponPump",0,1},
    {"skeeto_nav_weapon_chrome",L"WeaponChrome",0,1},
    {"skeeto_nav_weapon_auto",L"WeaponAuto",0,1},
    {"skeeto_nav_weapon_spas",L"WeaponSpas",0,1},
    {"skeeto_nav_weapon_mp5",L"WeaponMP5",0,1},
    {"skeeto_nav_weapon_sg552",L"WeaponSG552",0,1},
    {"skeeto_nav_weapon_melee",L"WeaponMelee",0,1},
    {"skeeto_nav_weapon_chainsaw",L"WeaponChainsaw",0,1},
    {"skeeto_nav_world_navigation",L"WorldNavigation",0,1},
    {"skeeto_nav_refresh_fps",L"RefreshFps",10,120},
    {"skeeto_nav_style",L"Style",0,1},
    {"skeeto_nav_world_arrow_color",L"WorldArrowColor",0,6},
    {"skeeto_nav_world_hud_x",L"WorldHudX",0,10000},
    {"skeeto_nav_world_hud_y",L"WorldHudY",0,10000},
    {"skeeto_nav_shape",L"Shape",0,1},
    {"skeeto_nav_world_hud",L"WorldHud",0,1},
    {"skeeto_nav_edit",L"Edit",0,1},
    {"skeeto_nav_reload",L"Reload",0,1}
};
constexpr int Count=sizeof(specs)/sizeof(specs[0]);
constexpr int SizeIndex=1,XIndex=5,YIndex=6,GoalIndex=9,ItemFirstIndex=12,ItemTypeCount=10,WeaponFirstIndex=22,WeaponTypeCount=22,WorldIndex=44,RefreshIndex=45,StyleIndex=46,WorldArrowColorIndex=47,WorldHudXIndex=48,WorldHudYIndex=49,ShapeIndex=50,WorldHudIndex=51,EditIndex=52,ReloadIndex=53;
constexpr int PersistCount=EditIndex;
static_assert(sizeof(Settings)==Count*sizeof(int),"Settings layout");
// Seed granular defaults from the old coarse filter. Individual saved values
// are read afterwards and therefore always override this migration.
inline void ApplyLegacyItemFilter(Settings& settings,int filter){
    bool medicalOnly=filter==2;
    settings.itemAmmo=settings.itemBile=settings.itemMolotov=settings.itemPipeBomb=settings.itemLaser=medicalOnly?0:1;
    settings.itemMedkit=settings.itemDefib=settings.itemPills=settings.itemAdrenaline=1;
    settings.itemWeapons=filter==0?1:0;
}

}


