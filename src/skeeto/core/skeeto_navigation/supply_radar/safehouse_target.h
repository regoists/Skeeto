#pragma once
#include "radar_abi.h"

// BSP evidence only: a checkpoint door must be uniquely associated with a
// touch-enabled forward chapter transition. Never choose the nearest door to
// the player, or infer direction from entity names / door spawnflags.
namespace safehouse_target {
inline bool Equal(const char* a,const char* b){if(!a||!b)return a==b;while(*a&&*b){char x=*a++,y=*b++;if(x>='A'&&x<='Z')x+=32;if(y>='A'&&y<='Z')y+=32;if(x=='\\')x='/';if(y=='\\')y='/';if(x!=y)return false;}return *a==*b;}
inline bool SameMap(const char* a,const char* b){
    if(!a||!b)return false;
    auto leaf=[](const char* s){const char* p=s;for(;*s;++s)if(*s=='/'||*s=='\\')p=s+1;return p;};
    char x[240]{},y[240]{};CopyText(x,sizeof x,leaf(a));CopyText(y,sizeof y,leaf(b));
    unsigned n=TextLength(x);if(n>=4&&Equal(x+n-4,".bsp"))x[n-4]=0;
    n=TextLength(y);if(n>=4&&Equal(y+n-4,".bsp"))y[n-4]=0;
    return Equal(x,y);
}
struct Map {
    struct Door {Vec point;};
    struct Landmark {char name[128];Vec point;};
    struct Transition {char landmark[128],map[240];unsigned flags;};
    Door doors[32]{};Landmark landmarks[64]{};Transition transitions[32]{};
    unsigned doorCount=0,landmarkCount=0,transitionCount=0;
    bool overflow=false,valid=false;Vec point{};
    void Clear(){doorCount=landmarkCount=transitionCount=0;overflow=valid=false;point={};}
    void Add(const char* cls,const char* name,Vec origin,bool hasOrigin,const char* parent,const char* map,const char* landmark,unsigned flags){
        if(parent[0])return;
        if(Equal(cls,"prop_door_rotating_checkpoint")&&hasOrigin){if(doorCount==32){overflow=true;return;}doors[doorCount++]={origin};}
        else if(Equal(cls,"info_landmark")&&name[0]&&hasOrigin){if(landmarkCount==64){overflow=true;return;}auto& p=landmarks[landmarkCount++];CopyText(p.name,sizeof p.name,name);p.point=origin;}
        else if((Equal(cls,"info_changelevel")||Equal(cls,"trigger_changelevel"))&&landmark[0]){if(transitionCount==32){overflow=true;return;}auto& t=transitions[transitionCount++];CopyText(t.landmark,sizeof t.landmark,landmark);CopyText(t.map,sizeof t.map,map);t.flags=flags;}
    }
    const Landmark* Find(const char* name)const{const Landmark* p=nullptr;for(unsigned i=0;i<landmarkCount;++i)if(Equal(name,landmarks[i].name)){if(p)return nullptr;p=&landmarks[i];}return p;}
    bool Forward(const Transition& t,const char* current)const{return !(t.flags&6)&&t.map[0]&&!SameMap(t.map,current);}
    bool Resolve(const char* current){
        valid=false;point={};if(overflow||!current||!current[0])return false;
        unsigned chosen=32;
        // Bounded once per map (32 transitions, 64 landmarks, 32 doors).
        for(unsigned t=0;t<transitionCount;++t){const auto& transition=transitions[t];if(!Forward(transition,current))continue;
            const auto* landmark=Find(transition.landmark);if(!landmark)continue;
            unsigned nearest=32;float best=1024.f*1024.f,second=1e20f;
            for(unsigned d=0;d<doorCount;++d){Vec delta=doors[d].point-landmark->point;float distance=delta.Dot(delta);if(distance<best){second=best;best=distance;nearest=d;}else if(distance<second)second=distance;}
            if(nearest==32)continue;
            // Two neighbouring doors or competing forward exits are ambiguous.
            if(second-best<64.f*64.f)return false;
            bool start=false;
            for(unsigned b=0;b<transitionCount;++b){if(Forward(transitions[b],current))continue;const auto* back=Find(transitions[b].landmark);if(!back)continue;Vec delta=doors[nearest].point-back->point;if(delta.Dot(delta)<=best){start=true;break;}}
            if(start)continue;
            if(chosen!=32&&chosen!=nearest)return false;chosen=nearest;
        }
        if(chosen==32)return false;point=doors[chosen].point;valid=true;return true;
    }
};
}
