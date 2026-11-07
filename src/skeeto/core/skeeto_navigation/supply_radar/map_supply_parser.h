#pragma once
#include "map_supply.h"
#include "budget_clock.h"

namespace map_supply {
// All state owns copied data or references the immutable BSP byte buffer.
// No engine object crosses a callback. Long quoted strings and pair matching
// can yield independently, so a giant script output cannot monopolize a tick.
struct Parser {
    Map* map=nullptr;
    Reader reader{nullptr,0};
    Entity entity{};
    Token key{},value{};
    unsigned phase=0,syntax=0,start=0,pairs=0,i=0,j=0;
    unsigned reducedSize=0;unsigned char reduced[256]{};
    bool escaped=false,referenced=false,button=false,failed=false,suppliesEnabled=true;

    void Cancel(){map=nullptr;reader={nullptr,0};phase=syntax=0;}
    bool Active()const{return phase!=0;}
    bool Begin(Map& output,const unsigned char* data,unsigned size,bool supplies=true){
        Cancel();output.Clear();failed=false;
        if(!data||!size||size>MaxBytes){failed=true;return false;}
        suppliesEnabled=supplies;map=&output;reader={data,size};phase=1;syntax=0;return true;
    }
    void Fail(){if(map)map->Clear();failed=true;phase=0;}

    bool Pair(){
        char name[64]{};
        if(key.size>=sizeof name)return true;
        if(!reader.Copy(key,name,sizeof name))return false;
        if(Equal(name,"OnPressed")){unsigned label=reducedSize<=sizeof reduced?GiveOutput({reduced,reducedSize}):0;if(label)entity.giveMask|=1u<<label;return true;}
        unsigned bit=0,cap=0;char* out=nullptr;
        if(Equal(name,"map")){bit=8192;out=entity.nextMap;cap=sizeof entity.nextMap;}
        else if(Equal(name,"landmark")){bit=16384;out=entity.landmark;cap=sizeof entity.landmark;}
        else if(Equal(name,"classname")){bit=1;out=entity.classname;cap=sizeof entity.classname;}
        else if(Equal(name,"weapon_selection")){bit=2;out=entity.selection;cap=sizeof entity.selection;}
        else if(Equal(name,"parentname")){bit=4;out=entity.parent;cap=sizeof entity.parent;}
        else if(Equal(name,"targetname")){bit=8;out=entity.name;cap=sizeof entity.name;}
        else if(Equal(name,"model")){bit=16;out=entity.model;cap=sizeof entity.model;}
        else if(Equal(name,"origin"))bit=32;
        else if(Equal(name,"angles"))bit=64;
        else if(Equal(name,"spawnflags"))bit=128;
        else if(Equal(name,"laser_sight"))bit=256;
        else if(Equal(name,"upgradepack_incendiary"))bit=512;
        else if(Equal(name,"upgradepack_explosive"))bit=1024;
        else if(Equal(name,"glow")){bit=2048;out=entity.glow;cap=sizeof entity.glow;}
        else if(Equal(name,"wait"))bit=4096;
        if(!bit)return true;
        if(entity.seen&bit)return false;
        entity.seen|=bit;
        if(out)return reader.Copy(value,out,cap);
        char text[96]{};if(!reader.Copy(value,text,sizeof text))return false;
        if(bit==32){entity.hasOrigin=Vector(text,entity.origin);return entity.hasOrigin;}
        if(bit==64){entity.hasAngles=Vector(text,entity.angles);return entity.hasAngles;}
        if(bit==4096){const char* at=text;return Number(at,entity.wait);}
        unsigned* destination=bit==128?&entity.flags:bit==256?&entity.laser:bit==512?&entity.incendiary:&entity.explosive;
        return Integer(text,*destination);
    }
    // One bounded tokenizer action. 1=entity completed, 2=end, 0=continue.
    unsigned TokenStep(){
        if(reader.at>=reader.size){if(syntax!=0&&syntax!=5){Fail();return 0;}return 2;}
        unsigned char c=reader.data[reader.at];
        if(syntax==0||syntax==1||syntax==3){
            if(White(c)){++reader.at;return 0;}
            if(syntax==0){
                if(c==0){++reader.at;syntax=5;return 0;}
                if(c!='{'){Fail();return 0;}
                ++reader.at;entity={};pairs=0;syntax=1;return 0;
            }
            if(syntax==1&&c=='}'){++reader.at;syntax=0;return 1;}
            if(c!='"'||(syntax==1&&++pairs>4096)){Fail();return 0;}
            ++reader.at;start=reader.at;escaped=false;syntax=syntax==1?2:4;reducedSize=0;return 0;
        }
        if(syntax==5){if(!White(c)){Fail();return 0;}++reader.at;return 0;}
        ++reader.at;
        if(!c){Fail();return 0;}
        if(syntax==4&&(c!='"'||escaped)&&!White(c)){if(reducedSize<sizeof reduced)reduced[reducedSize]=c;if(reducedSize<=sizeof reduced)++reducedSize;}
        if(escaped){escaped=false;return 0;}
        if(c=='\\'){escaped=true;return 0;}
        if(c!='"')return 0;
        Token token{reader.data+start,reader.at-start-1};
        if(syntax==2){key=token;syntax=3;}
        else{value=token;if(!Pair()){Fail();return 0;}syntax=1;}
        return 0;
    }
    void Collected(){
        if(++map->entities>MaxEntities){Fail();return;}
        map->checkpoint.Add(entity.classname,entity.name,entity.origin,entity.hasOrigin,entity.parent,entity.nextMap,entity.landmark,entity.flags);
        if(!suppliesEnabled)return;
        unsigned label=Label(entity);if(!label||!entity.hasOrigin)return;
        if(map->supplies>=MaxSupplies){Fail();return;}
        auto& s=map->records[map->supplies++];s={};
        s.label=label;s.flags=entity.flags;s.position=entity.origin;s.usable=!entity.parent[0];
        CopyText(s.parent,sizeof s.parent,entity.parent);CopyText(s.name,sizeof s.name,entity.name);
        s.displayItem=Equal(entity.classname,"prop_dynamic")||Equal(entity.classname,"prop_dynamic_override");
    }
    void Action(){
        if(phase==1||phase==2){
            unsigned result=TokenStep();if(!phase)return;
            if(result==1){
                if(phase==1)Collected();
                else{button=Equal(entity.classname,"func_button")&&entity.giveMask&&entity.hasOrigin&&(entity.flags&1024)&&entity.wait>=0&&!entity.parent[0]&&entity.glow[0];i=0;referenced=false;phase=3;}
            }else if(result==2){
                if(phase==1){if(!map->entities){Fail();return;}if(!suppliesEnabled){map->loaded=true;phase=0;return;}reader.at=0;syntax=0;phase=2;}
                else{i=0;j=1;phase=5;}
            }
            return;
        }
        if(phase==3){
            if(i<map->supplies){auto& s=map->records[i++];
                if(button){Vec d=s.position-entity.origin;if(s.displayItem&&(entity.giveMask&(1u<<s.label))&&Equal(s.name,entity.glow)&&d.Dot(d)<=96.f*96.f){s.verifiedButton=true;s.flags|=8;}}
                if(entity.name[0]&&s.parent[0]&&Equal(s.parent,entity.name))referenced=true;
            }else{j=0;phase=referenced?4:2;}
            return;
        }
        if(phase==4){
            if(j<map->parents&&!Equal(map->parentRecords[j].name,entity.name)){++j;return;}
            if(j==map->parents){
                if(map->parents>=MaxParents){Fail();return;}
                auto& p=map->parentRecords[map->parents++];p={};
                CopyText(p.name,sizeof p.name,entity.name);CopyText(p.model,sizeof p.model,entity.model);p.origin=entity.origin;
                p.usable=entity.hasOrigin&&Brush(entity.model)&&!entity.parent[0]&&Abs(entity.angles.x)<.001f&&Abs(entity.angles.y)<.001f&&Abs(entity.angles.z)<.001f;
            }
            ++map->parentRecords[j].matches;phase=2;return;
        }
        if(phase==5){
            if(i>=map->parents){i=j=0;phase=6;return;}
            if(j>=map->parents){j=++i+1;return;}
            auto& a=map->parentRecords[i];auto& b=map->parentRecords[j++];
            if(Brush(a.model)&&Equal(a.model,b.model))a.usable=b.usable=false;
            return;
        }
        if(phase==6){
            if(i>=map->supplies){i=0;j=1;phase=7;return;}
            auto& s=map->records[i];
            if(!s.parent[0]||j>=map->parents){if(s.displayItem&&!s.verifiedButton)s.usable=false;++i;j=0;return;}
            const auto& p=map->parentRecords[j++];
            if(Equal(s.parent,p.name)&&p.usable&&p.matches==1){s.position=s.position-p.origin;s.usable=Position(s.position);CopyText(s.parentModel,sizeof s.parentModel,p.model);j=map->parents;}
            return;
        }
        if(phase==7){
            if(i>=map->supplies){i=0;phase=8;return;}
            if(j>=map->supplies){j=++i+1;return;}
            auto& a=map->records[i];auto& b=map->records[j++];
            if(a.displayItem&&b.displayItem&&Equal(a.name,b.name))a.usable=b.usable=false;
            return;
        }
        if(phase==8){
            if(i<map->supplies){const auto& s=map->records[i++];if(s.displayItem&&s.usable)++map->buttonDisplays;}
            else{map->loaded=true;phase=0;}
        }
    }
    // Soft deadline plus a strict action cap. Individual API-free field
    // validation may exceed the deadline; no unbounded loop runs between yields.
    bool Step(unsigned budgetUs=250,unsigned maximumActions=8192){
        long long started=budget_clock::Now();
        for(unsigned n=0;Active()&&n<maximumActions;++n){Action();if((n&31u)==31u&&budget_clock::Microseconds(budget_clock::Now()-started)>=budgetUs)break;}
        return !Active();
    }
};
}
