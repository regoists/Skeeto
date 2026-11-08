#pragma once
#include "offline_counters.h"
// Metadata only: current entity and class are resolved by Inspect every time.
// RecvTables are immutable for the profiled client build and reset on map change;
// a changed class, table, or name pointer creates a fresh descriptor. No cached
// live entity or virtual method is dereferenced across callbacks.
namespace scan_profile {
enum Field : unsigned { Owner, WeaponOwner, Effects, Model, Parent, WeaponId, Count };
enum Flag : unsigned { Spawn=1,Upgrade=2,Display=4,Physics=8 };
struct Entry {
    ClientClass* cls=nullptr;RecvTable* table=nullptr;const char* namePointer=nullptr;
    items::Label label=items::Label::None;unsigned flags=0,ready=0;
    int offsets[Count]{};
};
static Entry entries[512];
L4N_COUNTERS(static unsigned hits,misses;)
inline void Reset(){memset(entries,0,sizeof entries);}
inline Entry* Find(ClientClass* cls){
    if(!cls)return nullptr;
    // Class() already checked this descriptor in the current Inspect call.
    auto table=cls->table;const char* name=cls->name;
    unsigned hash=(unsigned(cls)>>4)^(unsigned(table)>>4)^(unsigned(name)>>3);
    auto& e=entries[hash&511u];
    if(e.cls==cls&&e.table==table&&e.namePointer==name){L4N_OBSERVE(++hits);return &e;}
    L4N_OBSERVE(++misses);char copy[80];if(!CopySafe(copy,sizeof copy,name))return nullptr;
    e={};e.cls=cls;e.table=table;e.namePointer=name;e.label=items::ClassLabel(copy);
    e.flags=(Same(copy,"CWeaponSpawn")?Spawn:0u)|(Same(copy,"CBaseUpgradeItem")?Upgrade:0u)|
            (Same(copy,"CDynamicProp")?Display:0u)|(items::PhysicsClass(copy)?Physics:0u);
    return &e;
}
inline int Offset(Entry& e,Field field,const char* property){
    unsigned bit=1u<<unsigned(field);if(!(e.ready&bit)){e.offsets[field]=TableOffset(e.table,property);e.ready|=bit;}return e.offsets[field];
}
// Each field is one protected copy (skmem::EntRead): no page query, and a fault
// reads as failure. `entity` comes from the entity list in the current call.
struct Reader {
    void* entity;
    template<class T>bool Read(Entry& profile,Field field,const char* property,T& value){
        int off=Offset(profile,field,property);if(!entity||off<0||unsigned(off)>65536u-sizeof(T))return false;
        return EntRead(entity,off,&value);
    }
};
}
