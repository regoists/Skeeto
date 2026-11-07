#pragma once
#include "sdk.h"
// Read-only L4D2 NAV v15/v16, subversions 10..14. No game file is modified.
// This bounded parser retains geometry and directed connections, not bot
// visibility or encounter data.
namespace nav {
constexpr unsigned MaxBytes=64u*1024*1024, MaxAreas=200000;
inline float Abs(float x){return x<0?-x:x;}
inline float Min(float a,float b){return a<b?a:b;}
inline float Max(float a,float b){return a>b?a:b;}
constexpr unsigned Invalid=0xffffffffu,MaxEdges=2000000;
struct Area {unsigned id,flags,spawn;float x1,y1,z1,x2,y2,z2,zNE,zSW;unsigned firstEdge=0,edgeCount=0;};
struct Ladder {unsigned id;float width;Vec top,bottom;float length;unsigned direction,upper[4],lower;};
struct LadderRef {unsigned area,id,down;};
struct LadderLink {unsigned from,to,ladder,down;};
static_assert(sizeof(Ladder)==60,"NAV ladder record layout");
struct Reader {
    const unsigned char* data;unsigned size,pos=0;bool ok=true;
    bool Skip(unsigned n){if(!ok||n>size-pos){ok=false;return false;}pos+=n;return true;}
    unsigned U8(){if(!ok||pos>=size){ok=false;return 0;}return data[pos++];}
    unsigned U16(){unsigned a=U8(),b=U8();return a|(b<<8);}
    unsigned U32(){unsigned a=U16(),b=U16();return a|(b<<16);}
    float F32(){union {unsigned u;float f;} v;v.u=U32();return v.f;}
    bool Array(unsigned n,unsigned width){if(!ok||!width||n>(size-pos)/width){ok=false;return false;}return Skip(n*width);}
    bool CString(){for(unsigned n=0;n<1024&&ok;++n)if(!U8())return ok;ok=false;return false;}
    bool Strings(){unsigned n=U16();for(unsigned i=0;i<n&&ok;++i){unsigned len=U16();if(len>4096){ok=false;break;}Skip(len);}return ok;}
};
struct Mesh {
    Area* areas=nullptr;unsigned count=0,version=0,subversion=0,bspSize=0,parsedBytes=0;
    unsigned* edges=nullptr;unsigned edgeCount=0,edgeCapacity=0;
    Ladder* ladders=nullptr;unsigned ladderCount=0;
    LadderRef* ladderRefs=nullptr;unsigned ladderRefCount=0,ladderRefCapacity=0;
    LadderLink* ladderLinks=nullptr;unsigned ladderLinkCount=0;
    const char* error="No NAV loaded";
    void Clear(){if(areas)HeapFree(GetProcessHeap(),0,areas);if(edges)HeapFree(GetProcessHeap(),0,edges);if(ladders)HeapFree(GetProcessHeap(),0,ladders);if(ladderRefs)HeapFree(GetProcessHeap(),0,ladderRefs);if(ladderLinks)HeapFree(GetProcessHeap(),0,ladderLinks);
        areas=nullptr;edges=nullptr;ladders=nullptr;ladderRefs=nullptr;ladderLinks=nullptr;count=edgeCount=edgeCapacity=ladderCount=ladderRefCount=ladderRefCapacity=ladderLinkCount=0;parsedBytes=0;}
    bool ReserveLadderRefs(unsigned needed){
        if(needed<=ladderRefCapacity)return true;if(needed>MaxEdges/3)return false;unsigned cap=ladderRefCapacity?ladderRefCapacity:64;
        while(cap<needed)cap=cap>MaxEdges/6?MaxEdges/3:cap*2;
        auto next=(LadderRef*)(ladderRefs?HeapReAlloc(GetProcessHeap(),0,ladderRefs,cap*sizeof(LadderRef)):HeapAlloc(GetProcessHeap(),0,cap*sizeof(LadderRef)));
        if(!next)return false;ladderRefs=next;ladderRefCapacity=cap;return true;
    }
    static unsigned Lookup(const unsigned* slots,unsigned cap,const Area* list,unsigned id){unsigned at=(id*2654435761u)&(cap-1);for(unsigned probes=0;probes<256&&slots[at];++probes){if(list[slots[at]-1].id==id)return slots[at]-1;at=(at+1)&(cap-1);}return Invalid;}
    bool ReserveEdges(unsigned needed){
        if(needed<=edgeCapacity)return true;if(needed>MaxEdges)return false;unsigned capacity=edgeCapacity?edgeCapacity:1024;
        while(capacity<needed)capacity=capacity>MaxEdges/2?MaxEdges:capacity*2;
        auto next=(unsigned*)(edges?HeapReAlloc(GetProcessHeap(),0,edges,capacity*4):HeapAlloc(GetProcessHeap(),0,capacity*4));
        if(!next)return false;edges=next;edgeCapacity=capacity;return true;
    }
    bool Fail(const char* why){Clear();error=why;return false;}

};
inline float Height(const Area& a,float x,float y){
    float u=a.x2-a.x1>.01f?Limit((x-a.x1)/(a.x2-a.x1),0.f,1.f):0;
    float v=a.y2-a.y1>.01f?Limit((y-a.y1)/(a.y2-a.y1),0.f,1.f):0;
    return (a.z1+(a.zNE-a.z1)*u)*(1-v)+(a.zSW+(a.z2-a.zSW)*u)*v;
}
inline bool FindGround(const Mesh& mesh,Vec p,float& answer){
    float best=1e20f;bool found=false;answer=p.z;
    for(unsigned i=0;i<mesh.count;++i){const Area& a=mesh.areas[i];float x=Limit(p.x,a.x1,a.x2),y=Limit(p.y,a.y1,a.y2),z=Height(a,x,y);
        // Prefer underfoot areas; a nearby upper storey must not steal the layer.
        if(z>p.z+20||z<p.z-180)continue;
        float dx=x-p.x,dy=y-p.y,dz=z-p.z;float score=dx*dx+dy*dy+dz*dz*2;
        // Underfoot geometry outranks a nearby landing on another storey.
        if(dx*dx+dy*dy>96*96)continue;
        if(dx*dx+dy*dy>4)score+=100000;
        if(score<best){best=score;answer=z;found=true;}
    }return found;
}
inline float Ground(const Mesh& mesh,Vec p){float z;FindGround(mesh,p,z);return z;}
struct FloorTracker {
    bool valid=false;float z=0;Vec previous{};
    void Reset(){valid=false;}
    float Update(const Mesh& mesh,Vec p){
        Vec d=p-previous;float candidate;
        if(FindGround(mesh,p,candidate))z=candidate;
        else if(!valid||d.Dot(d)>600*600||p.z<z-48||p.z>z+180)z=p.z;
        previous=p;valid=true;return z;
    }
};
}

