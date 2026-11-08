#pragma once
#include "scan_profile.h"
// Included after Candidate and the existing Class/model/map helpers.
static bool Inspect(int index,Candidate& out,bool mapFlags=true){
    bool dormant=false;auto cls=Class(index,dormant);
    if(!cls||dormant){return false;}
    auto profile=scan_profile::Find(cls);if(!profile)return false;
    bool spawn=(profile->flags&scan_profile::Spawn)!=0,upgrade=(profile->flags&scan_profile::Upgrade)!=0;
    bool displayItem=options::values[options::MapMetadata]&&(profile->flags&scan_profile::Display)&&supplyHints.buttonDisplays;
    bool physics=(profile->flags&scan_profile::Physics)&&modelLookupReady&&(options::EnabledId(32)||options::EnabledId(33));
    items::Label label=profile->label;
    if(label==items::Label::None&&!spawn&&!upgrade&&!displayItem&&!physics)return false;
    void* e=Entity(index);scan_profile::Reader reader{e};
    unsigned owner=~0u,weaponOwner=~0u,parent=~0u;int effects=32;short model=0;Vec p{};bool modelRead=false,parentRead=false;
    if(spawn){int id=0;if(!reader.Read(*profile,scan_profile::WeaponId,"m_weaponID",id)){return false;}label=items::WeaponLabel(id);}
    if(upgrade)label=items::Label::Laser;
    if(physics){
        modelRead=reader.Read(*profile,scan_profile::Model,"m_nModelIndex",model);
        if(!modelRead||!items::ValidModel(model))return false;
        char modelName[384];if(!LaserModelName(model,modelName,sizeof modelName))return false;
        label=items::PhysicsSupply(modelName);if(label==items::Label::None)return false;
    }
    if(displayItem){parentRead=reader.Read(*profile,scan_profile::Parent,"moveparent",parent);if(!parentRead)return false;label=MapDisplayLabel(e,cls,parent);if(label==items::Label::None)return false;}
    if(!options::Enabled(label)){return false;}
    if(!reader.Read(*profile,scan_profile::Owner,"m_hOwnerEntity",owner)||!reader.Read(*profile,scan_profile::Effects,"m_fEffects",effects)||
       (!modelRead&&!reader.Read(*profile,scan_profile::Model,"m_nModelIndex",model))){return false;}
    reader.Read(*profile,scan_profile::WeaponOwner,"m_hOwner",weaponOwner);
    if(!items::Available(owner,weaponOwner,effects,model)){return false;}
    if(!parentRead)parentRead=reader.Read(*profile,scan_profile::Parent,"moveparent",parent);
    
    if(!WorldPosition(e,p)||!radar_nav::Position(p)){return false;}
    if(upgrade){bool stock=IsLaser(model),fixed=options::values[options::MapMetadata]&&supplyHints.FixedLaser(p);if(!stock&&!fixed)return false;}
    
    out={index,e,cls,label,p,0,displayItem||(mapFlags&&parentRead&&MapUnlimited(e,cls,label,spawn,parent)),true,
         entitySerialReady?EntitySerialKey(entities,index,e):0u};
    return true;
}
