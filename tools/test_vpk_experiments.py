"""x86 fixture for real retail-style VPK directory records and experimental policy."""
import test_fast_load as native

native.FIXTURE=r'''
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include "skeeto_vpk_experiments.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
static void require(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
struct Dir {Dir* next;const char* name;};
struct Ext {Ext* next;const char* name;Dir* dirs[43];};
struct Pack {
    alignas(4) unsigned char store[0x280]{};
    Ext ext[4]{};Dir dir[4]{};char records[4][1024]{};
    Pack(const char* path){std::strcpy((char*)store,path);}
    void Add(unsigned i,const char* extension,const char* directory,const char* file){
        char* p=records[i];std::strcpy(p,directory);p+=std::strlen(p)+1;
        std::strcpy(p,file);p+=std::strlen(p)+1;p+=4;
        uint16_t preload=2,archive=0x7fff,end=0xffff;std::memcpy(p,&preload,2);p+=2;
        std::memcpy(p,&archive,2);p+=10;std::memcpy(p,&end,2);p+=2;*p++='x';*p++='y';*p=0;
        dir[i].name=records[i];ext[i].name=extension;ext[i].dirs[0]=&dir[i];
        ext[i].next=i?&ext[i-1]:nullptr;*reinterpret_cast<Ext**>(store+0x22c)=&ext[i];
    }
};
static int missionReads=0;
static int Mission(const char* file,const char* map,skvpkexp::MapSet& campaign){
    ++missionReads;
    if(!std::strcmp(file,"broken"))return -1;
    const bool related=!std::strcmp(file,"campaign_a") && (!std::strcmp(map,"a1")||!std::strcmp(map,"a2"));
    if(related){campaign.Add(skvpkexp::Hash("a1"));campaign.Add(skvpkexp::Hash("a2"));}return related;
}
static skvpkexp::Controller c;
static unsigned nativeFinds=0;
static void* __fastcall Find(void* store,void*,const char* directory,const char* filename,const char* extension,void*,void*){
    ++nativeFinds;
    for(Ext* x=*reinterpret_cast<Ext**>((char*)store+0x22c);x;x=x->next){
        if(std::strcmp(x->name,extension))continue;
        for(Dir* head:x->dirs)for(Dir* d=head;d;d=d->next){
            if(std::strcmp(d->name,directory))continue;
            const char* p=d->name+std::strlen(d->name)+1;
            while(*p){if(!std::strcmp(p,filename))return (void*)(p+std::strlen(p)+1);
                p+=std::strlen(p)+5;uint16_t preload=0;std::memcpy(&preload,p,2);p+=2;
                uint16_t archive;for(;;){std::memcpy(&archive,p,2);if(archive==0xffff){p+=2;break;}p+=10;}p+=preload;
            }
        }
    }
    return nullptr;
}
int main(){
    Pack current("D:/game/left4dead2/addons/a.vpk"),other("D:/game/left4dead2/addons/b.vpk"),
        resource("D:/game/left4dead2/addons/texture.vpk"),chapter("D:/game/left4dead2/addons/a_part2.vpk"),
        official("D:/game/left4dead2/pak01"),broken("D:/game/left4dead2/addons/broken.vpk");
    current.Add(0,"bsp","maps","a1");current.Add(1,"txt","missions","campaign_a");
    other.Add(0,"bsp","maps","b1");other.Add(1,"txt","missions","campaign_b");
    resource.Add(0,"vmt","materials/shared","wall");resource.Add(1,"txt","missions","unrelated_metadata");
    chapter.Add(0,"bsp","maps","a2");chapter.Add(1,"txt","missions","campaign_a");
    official.Add(0,"bsp","maps","stock");broken.Add(0,"nav","maps","broken");broken.Add(1,"txt","missions","broken");
    void* stores[]{other.store,chapter.store,current.store,resource.store,official.store,broken.store};
    c.mission=Mission;c.enabled=1;c.SetMap("maps/A1.bsp");
    require(c.Next(stores,6,0,other.store,1,"maps","a1","bsp")==1,"skip unrelated map but retain another chapter of current campaign");
    require(c.Next(stores,6,2,current.store,1,"maps","a1","bsp")==2,"current map remains eligible");
    require(c.Next(stores,6,3,resource.store,1,"materials/shared","wall","vmt")==3,"pure resource pack stays even with mission metadata");
    require(c.Next(stores,6,4,official.store,1,"maps","stock","bsp")==4,"base/DLC archives are never map-isolated");
    require(c.Next(stores,6,5,broken.store,1,"maps","broken","nav")==5,"unreadable campaign association stays native");
    const auto reads=missionReads;c.Next(stores,6,1,chapter.store,1,"x","x","x");require(missionReads==reads,"mission metadata is cached, never read per request");
    Pack split("D:/game/left4dead2/addons/a_split.vpk"),looseOther("D:/game/left4dead2/addons/b_split.vpk");
    split.Add(0,"bsp","maps","a2");looseOther.Add(0,"bsp","maps","b2");
    void* splitStores[]{split.store,looseOther.store,current.store};
    require(c.Next(splitStores,3,0,split.store,8,"materials","shared","vmt")==0,"mission in later main pack protects chapter without its own mission");
    require(c.Next(splitStores,3,1,looseOther.store,8,"materials","shared","vmt")==2,"complete current campaign excludes a foreign chapter without mission");
    c.SetMap("unknown");require(c.Next(splitStores,3,0,split.store,9,"materials","shared","vmt")==0,"unattributed split packs remain native when current campaign is unknown");
    c.SetMap("b1");require(c.Next(stores,6,0,other.store,1,"maps","b1","bsp")==0,"next map invalidates prior exclusions");
    c.SetMap("stock");void* large[301]{};for(unsigned i=0;i<300;++i)large[i]=other.store;large[300]=resource.store;
    for(unsigned i=0;i<300;++i)c.Next(large,301,i,other.store,11,"materials","wall","vmt");
    require(c.Next(large,301,0,other.store,11,"materials","wall","vmt")==300,"cached map exclusion works above 256 and stops at pure resources");
    void* changed[]{other.store,current.store,resource.store};
    c.SetMap("a1");require(c.Next(changed,3,0,other.store,12,"maps","a1","bsp")==1,"list identity and mount epoch rebuild decisions");
    c.BeginLoad();require(c.Next(changed,3,0,other.store,13,"x","x","x")==0,"missing target cannot discard maps");
    c.ObserveBsp("maps","a1","bsp");require(c.Next(changed,3,0,other.store,13,"maps","a1","bsp")==1,"first BSP request can supply early target");
    const char missionText[]="\"mission\" { \"modes\" { \"coop\" { \"1\" { \"Map\" \"a1\" } } \"versus\" { \"1\" { \"Map\" \"maps/sub/a2.bsp\" } } } }";
    skvpkexp::MapSet maps;
    require(skvpkexp::MissionContains(missionText,sizeof(missionText)-1,"a1",maps)==1
        && skvpkexp::MissionContains(missionText,sizeof(missionText)-1,"sub/a2",maps)==1
        && skvpkexp::MissionContains(missionText,sizeof(missionText)-1,"b1",maps)==0,"campaign association reads complete Map tokens across modes");
    const char displayMap[]="mission { modes { coop { 1 { DisplayName map Map a1 } 2 { Map a2 } } } }";
    require(skvpkexp::MissionContains(displayMap,sizeof(displayMap)-1,"a1",maps)==1,"a field value equal to map does not consume a Map key");
    require(skvpkexp::MissionContains("mission {",9,"a1",maps)==-1,"invalid mission is unknown, not an exclusion");
    c.enabled=2;c.Reset();c.find=reinterpret_cast<skvpkexp::FindEntryFn>(&Find);
    require(c.Next(stores,6,0,other.store,2,"maps","a1","bsp",1)==2,"file filter independently rejects absent basename");
    require(c.Next(stores,6,2,current.store,2,"maps","a1","bsp",1)==2,"file supplier and original priority remain");
    const auto nativeBefore=nativeFinds;
    require(c.Next(stores,6,0,other.store,2,"maps","a1","bsp",1)==2 && nativeFinds==nativeBefore,"same request reuses confirmed misses and positives without new native prechecks");
    require(c.Next(stores,6,0,other.store,2,"maps","b1","bsp",2)==0,"a new path token never reuses another filename's exclusion");
    require(c.Next(stores,6,0,other.store,3,"maps","a1","bsp",2)==2 && nativeFinds>nativeBefore,"mount generation invalidates exact-file decisions");
    Pack heavy("D:/game/left4dead2/addons/heavy.vpk");std::vector<char> big(120000);char* cursor=big.data();
    std::strcpy(cursor,"maps");cursor+=5;
    for(unsigned i=0;i<6000;++i){char name[20];std::snprintf(name,sizeof(name),"file%u",i);std::strcpy(cursor,name);cursor+=std::strlen(name)+1;cursor+=6;uint16_t end=0xffff;std::memcpy(cursor,&end,2);cursor+=2;}*cursor=0;
    heavy.dir[0].name=big.data();heavy.ext[0].name="bsp";heavy.ext[0].dirs[0]=&heavy.dir[0];*reinterpret_cast<Ext**>(heavy.store+0x22c)=&heavy.ext[0];
    void* heavyStores[]{heavy.store};const auto beforeHeavy=nativeFinds;
    require(c.Next(heavyStores,1,0,heavy.store,4,"maps","missing","bsp",3)==1,"a pack above the old 4096-name limit remains filterable");
    for(unsigned i=0;i<100;++i)require(c.Next(heavyStores,1,0,heavy.store,4,"maps","missing","bsp",3)==1,"large-pack exact miss remains reusable");
    require(nativeFinds==beforeHeavy+1,"large pack gets one original metadata lookup, no pre-index scan or repeated native precheck");
    c.enabled=0;c.Reset();const auto builds=c.stats.builds;
    require(c.Next(stores,6,0,other.store,3,"maps","a1","bsp")==0 && c.stats.builds==builds,"disabled does no indexing");
    std::puts("PASS VPK experiments: current/other/campaign/resource/base, file names, epoch/budgets/off");
}
'''
if __name__=='__main__':native.main()
