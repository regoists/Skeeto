#pragma once
#include "skeeto_memread.h"
// Existing mission parser bounds token copies before its CRT string operations.
#pragma warning(push)
#pragma warning(disable:4996)
#include "skeeto_mission.h"
#pragma warning(pop)
#include <cstdint>
#include <cstdio>
#include <cstring>

// Loading/owner thread only, from the already admitted native VPK loop.
// Metadata only: no saved engine nodes, handles, resource contents or workers.
namespace skvpkexp {
constexpr unsigned kMaxStores=2048,kMapBudget=16384,kDirectoryBudget=65536;
inline uint32_t Hash(const char* text,uint32_t h=2166136261u){for(;*text;++text)h=(h^uint8_t(*text))*16777619u;return h;}
using FindEntryFn=void*(__thiscall*)(void*,const char*,const char*,const char*,void*,void*);
inline bool NormalizeMap(const char* input,char (&out)[240]){
    char value[260]{};if(!input||!skmem::CopyCStr(value,sizeof(value),input))return false;
    for(char* p=value;*p;++p){if(*p=='\\')*p='/';if(*p>='A'&&*p<='Z')*p+=32;}
    const char* start=!std::strncmp(value,"maps/",5)?value+5:value;
    size_t n=std::strlen(start);if(n>4&&!std::strcmp(start+n-4,".bsp"))n-=4;
    if(!n||n>=sizeof(out)||std::strchr(start,':')||std::strstr(start,".."))return false;
    std::memcpy(out,start,n);out[n]=0;return true;
}
struct MapSet {
    uint32_t hashes[256]{};unsigned count=0;
    bool Add(uint32_t hash){for(unsigned i=0;i<count;++i)if(hashes[i]==hash)return true;
        if(count==256)return false;hashes[count++]=hash;return true;}
};
// Read key/value pairs, not arbitrary tokens: a DisplayName value may be "map".
// Retain all modes of a mission containing the target, including split packs
// whose own archive has no missions/*.txt. Hash collisions only retain packs.
inline int MissionContains(const char* text,size_t size,const char* target,MapSet& campaign){
    skmission::Reader reader(text,size);char key[260]{},value[260]{};int depth=0;
    bool match=false;MapSet maps;
    for(;;){int k=reader.Next(key);if(!k)break;
        if(k==3){if(!depth)return -1;--depth;continue;}if(k!=1)return -1;
        const int v=reader.Next(value);if(v==2){if(++depth>16)return -1;continue;}
        if(v!=1)return -1;
        if(skmission::Equal(key,"map")){
            char map[240]{};if(!NormalizeMap(value,map)||!maps.Add(Hash(map)))return -1;
            if(!std::strcmp(map,target))match=true;
        }
    }
    if(depth)return -1;
    if(match)for(unsigned i=0;i<maps.count;++i)if(!campaign.Add(maps.hashes[i]))return -1;
    return match?1:0;
}
struct Counters {uint64_t builds=0,names=0,mapSkipped=0,fileRejected=0,fileQueries=0,fileChecks=0,fileHits=0,unknown=0,missionReads=0,failures=0;};
struct Entry {
    void* store=nullptr;unsigned serial=0,mapNext=0;
    unsigned fileRequest=0,fileNext=0;uint8_t fileKnown=0;
    bool mapReady=false,hasMap=false,current=false,related=false,hasMission=false;
    uint32_t mapBits[32]{};
};
struct Controller {
    // Bits: 1 isolate other map addons, 2 refine exact-file candidates.
    unsigned enabled=0,requested=0,serial=1,mapBudget=0,directoryBudget=0;
    char map[240]{};bool inside=false,sawBsp=false,prepared=false,campaignComplete=true;
    MapSet campaign;
    int (*mission)(const char*,const char*,MapSet&)=nullptr;
    FindEntryFn find=nullptr;
    Entry entries[kMaxStores];
    struct Mission {char name[80]{};int result=0;} missions[128];
    unsigned missionCount=0;
    void** stores=nullptr;unsigned count=0,epoch=0;
    Counters stats;
    void Reset(){++serial;mapBudget=directoryBudget=missionCount=0;stores=nullptr;count=epoch=0;
        prepared=false;campaignComplete=true;campaign.count=0;}
    void SetMap(const char* name){char target[240]{};if(!NormalizeMap(name,target))return;
        if(std::strcmp(map,target)){std::memcpy(map,target,std::strlen(target)+1);Reset();}}
    void BeginLoad(){map[0]=0;sawBsp=false;Reset();}
    void ObserveBsp(const char* dir,const char* file,const char* ext){
        if(!(enabled&1)||inside||sawBsp||std::strcmp(ext,"bsp")||(std::strcmp(dir,"maps")&&std::strncmp(dir,"maps/",5)))return;
        char target[260]{};const int n=std::snprintf(target,sizeof(target),"%s/%s",dir,file);
        if(n>0&&n<int(sizeof(target))){sawBsp=true;SetMap(target);}
    }
    bool Configure(unsigned want){
        if(want==requested)return enabled==want;
        requested=want;enabled=0;Reset();if(!want)return true;
        // Exact retail 2d390 record walker; CRC/preload/range layout is not SDK VPK v1.
        uint8_t expected[]={0x8b,0x06,0x57,0x50,0x83,0xcf,0xff,0xe8,0x14,0x6e,0xff,0xff,0x83,0xc0,0x05,0x01,0x06,0x8b,0x06,0x0f,0xb7,0x10,0x83,0xc0,0x02,0xb9,0xff,0xff,0x00,0x00,0x83,0xc4,0x04,0x89,0x06,0x66,0x39,0x08,0x74,0x2b,0xeb,0x06,0x8d,0x9b,0x00,0x00,0x00,0x00,0x8b,0x0e,0x0f,0xb7,0x01,0x3d,0xff,0x7f,0x00,0x00,0x74,0x06,0x3b,0xf8,0x7f,0x02,0x8b,0xf8,0x83,0xc1,0x0a,0x89,0x0e,0x8b,0xc1,0xb9,0xff,0xff,0x00,0x00,0x66,0x39,0x08,0x75,0xdd,0x83,0xc2,0x02,0x01,0x16,0x8b,0xc7,0x5f,0xc3};
        const auto* module=reinterpret_cast<const uint8_t*>(GetModuleHandleA("filesystem_stdio.dll"));uint8_t actual[sizeof(expected)]{};
        if(!module||!skmem::Copy(actual,module+0x2d390,sizeof(actual))||std::memcmp(actual,expected,sizeof(actual))){++stats.failures;return false;}
        enabled=want;return true;
    }
    int MissionResult(const char* filename){
        for(unsigned i=0;i<missionCount;++i)if(!std::strcmp(missions[i].name,filename))return missions[i].result;
        if(!mission||missionCount==128){campaignComplete=false;return -1;}
        ++stats.missionReads;const int result=mission(filename,map,campaign);
        if(result<0)campaignComplete=false;
        auto& cached=missions[missionCount++];std::memcpy(cached.name,filename,std::strlen(filename)+1);cached.result=result;return result;
    }
    bool Walk(Entry& e,const char* cursor,const char* dir,bool mapFiles,bool missionFiles,
              char (&names)[8][80],unsigned& missionNames){
        char name[260]{};unsigned walked=0;
        for(;;){
            if(++walked>4096)return false;
            if(!skmem::CopyCStr(name,sizeof(name),cursor))return false;
            if(!name[0])return true;
            if(mapBudget==kMapBudget)return false;++mapBudget;++stats.names;
            if(mapFiles){
                e.hasMap=true;char path[260]{},target[240]{};
                const int n=std::snprintf(path,sizeof(path),"%s/%s",dir,name);
                if(n<0||n>=int(sizeof(path)))return false;
                if(!NormalizeMap(path,target))return false;
                const uint32_t h=Hash(target);e.mapBits[(h&1023u)>>5]|=1u<<(h&31);
                e.mapBits[((h>>16)&1023u)>>5]|=1u<<((h>>16)&31);
                if(!std::strcmp(target,map))e.current=true;
            }
            if(missionFiles){
                if(missionNames==8||std::strlen(name)>=80)return false;
                std::memcpy(names[missionNames++],name,std::strlen(name)+1);
            }
            cursor+=std::strlen(name)+5;uint16_t preload=0;
            if(!skmem::Copy(&preload,cursor,2))return false;cursor+=2;bool end=false;
            for(unsigned i=0;i<16;++i){uint16_t archive=0;if(!skmem::Copy(&archive,cursor,2))return false;
                if(archive==0xffff){cursor+=2;end=true;break;}cursor+=10;}
            if(!end)return false;cursor+=preload;
        }
    }
    void Build(Entry& e){
        ++stats.builds;char path[260]{};
        if(!skmem::CopyCStr(path,sizeof(path),(const char*)e.store)){campaignComplete=false;++stats.unknown;return;}
        for(char* p=path;*p;++p){if(*p=='\\')*p='/';if(*p>='A'&&*p<='Z')*p+=32;}
        e.mapReady=(enabled&1)&&map[0]&&std::strstr(path,"/addons/");
        if(!e.mapReady)return;
        uintptr_t roots[15]{};
        struct Extension{uintptr_t next;const char* name;uintptr_t dirs[43];};struct Directory{uintptr_t next;const char* name;};
        char ext[260]{},dir[260]{},missionNames[8][80]{};unsigned extensions=0,names=0,nodes=0;
        bool good=skmem::Copy(roots,(uint8_t*)e.store+0x22c,sizeof(roots));
        for(uintptr_t root:roots)for(uintptr_t p=root;good&&p;){
            Extension x{};if(++extensions>128||!skmem::Copy(&x,(void*)p,sizeof(x))||!skmem::CopyCStr(ext,sizeof(ext),x.name)){good=false;break;}
            const bool mapExt=e.mapReady&&(!std::strcmp(ext,"bsp")||!std::strcmp(ext,"nav"));
            const bool missionExt=e.mapReady&&!std::strcmp(ext,"txt");
            if(mapExt||missionExt)for(uintptr_t head:x.dirs)for(uintptr_t d=head;good&&d;){
                Directory item{};
                if(++nodes>4096||directoryBudget==kDirectoryBudget||!skmem::Copy(&item,(void*)d,sizeof(item))||!skmem::CopyCStr(dir,sizeof(dir),item.name)){good=false;break;}
                ++directoryBudget;
                const bool maps=mapExt&&(!std::strcmp(dir,"maps")||!std::strncmp(dir,"maps/",5));
                const bool missionDir=missionExt&&!std::strcmp(dir,"missions");
                if((maps||missionDir)&&!Walk(e,item.name+std::strlen(dir)+1,dir,maps,missionDir,missionNames,names)){good=false;break;}
                d=item.next;
            }
            p=x.next;
        }
        if(!good){e.mapReady=false;campaignComplete=false;++stats.unknown;return;}
        e.hasMission=names!=0;
        if(e.mapReady)for(unsigned i=0;i<names;++i){
            const int related=MissionResult(missionNames[i]);
            if(related<0){e.mapReady=false;++stats.unknown;break;}
            if(related)e.related=true;
        }
    }
    void Prepare(void** list,unsigned n){
        // One bounded metadata pass before making any map exclusion. Mission
        // metadata can live in a later archive than the chapter it describes.
        inside=true;
        for(unsigned i=0;i<n;++i){void* candidate=nullptr;auto& e=entries[i];e={};e.serial=serial;e.mapNext=i+1;
            if(!skmem::Copy(&candidate,list+i,sizeof(candidate))){campaignComplete=false;++stats.failures;continue;}
            e.store=candidate;Build(e);
        }
        inside=false;
        for(unsigned i=0;i<n;++i){auto& e=entries[i];if(!e.mapReady||!e.hasMap||e.current||e.related)continue;
            for(unsigned j=0;j<campaign.count;++j){const auto h=campaign.hashes[j];
                if((e.mapBits[(h&1023u)>>5]&(1u<<(h&31)))&&(e.mapBits[((h>>16)&1023u)>>5]&(1u<<((h>>16)&31)))){e.related=true;break;}}
            if(!e.hasMission&&(!campaignComplete||!campaign.count)){e.mapReady=false;++stats.unknown;}
        }
        prepared=true;
    }
    unsigned Next(void** list,unsigned n,unsigned at,void* store,unsigned generation,const char* dir,const char* file,const char* ext,unsigned request=0){
        if(!enabled||inside||n>kMaxStores||at>=n)return at;
        if(stores!=list||count!=n||epoch!=generation){Reset();stores=list;count=n;epoch=generation;}
        if((enabled&1)&&map[0]&&!prepared)Prepare(list,n);
        unsigned next=at;
        // At most 64 consecutive records per native-loop entry, independent of
        // the stable directory list's 256-store capacity. Retain unknown packs.
        for(unsigned steps=0;next<n&&steps<64;++steps){
            void* candidate=store;if(next!=at&&!skmem::Copy(&candidate,list+next,sizeof(candidate))){++stats.failures;return at;}
            auto& e=entries[next];
            if(e.serial!=serial||e.store!=candidate){e={};e.store=candidate;e.serial=serial;e.mapNext=next+1;
                if(enabled&1){inside=true;Build(e);inside=false;}}
            if((enabled&1)&&e.mapReady&&e.hasMap&&!e.current&&!e.related){
                const unsigned child=e.mapNext;
                if(child<n){const auto& c=entries[child];
                    if(c.serial==serial&&c.mapReady&&c.hasMap&&!c.current&&!c.related)e.mapNext=c.mapNext;}
                stats.mapSkipped+=e.mapNext-next;next=e.mapNext;continue;
            }
            if(enabled&2){
                ++stats.fileQueries;
                if(e.fileRequest!=request||!e.fileKnown){
                    if(!find){++stats.failures;return at;}
                    ++stats.fileChecks;
                    e.fileKnown=find(candidate,dir,file,ext,nullptr,nullptr)?1:2;
                    e.fileRequest=request;e.fileNext=next+1;
                }else ++stats.fileHits;
                if(e.fileKnown==2){
                    const unsigned child=e.fileNext;
                    if(child<n){const auto& c=entries[child];
                        if(c.serial==serial&&c.fileRequest==request&&c.fileKnown==2)e.fileNext=c.fileNext;}
                    stats.fileRejected+=e.fileNext-next;next=e.fileNext;continue;
                }
            }
            break;
        }
        return next;
    }
};
inline Controller g_controller;
}
