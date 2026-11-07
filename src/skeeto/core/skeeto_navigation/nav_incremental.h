#pragma once
#include "nav.h"

// The source parser consumed the complete NAV in one timer callback. Skeeto
// keeps its validation and graph format while spreading each phase across
// Paint calls. The buffer stays owned by the caller until Ready/Failed.
namespace nav {
struct ParseJob {
    enum class Result {Idle,Working,Ready,Failed};
    Mesh* mesh=nullptr;Reader r{nullptr,0};unsigned total=0,areaAt=0,ladderAt=0,edgeAt=0,refAt=0;
    unsigned* areasByID=nullptr;unsigned areaCap=0,*laddersByID=nullptr,ladderCap=0;
    // Resume variable-length area records exactly where a per-Paint budget ends.
    unsigned areaStage=0,areaDirection=0,areaRemaining=0,encounterRemaining=0,refDirection=0;
    int phase=0;
    void Cancel(){
        if(areasByID)HeapFree(GetProcessHeap(),0,areasByID);
        if(laddersByID)HeapFree(GetProcessHeap(),0,laddersByID);
        areasByID=laddersByID=nullptr;areaCap=ladderCap=0;mesh=nullptr;phase=0;
        areaStage=areaDirection=areaRemaining=encounterRemaining=refDirection=0;
    }
    Result Fail(const char* reason){Mesh* m=mesh;Cancel();if(m)m->Fail(reason);return Result::Failed;}
    Result Begin(Mesh& target,const unsigned char* bytes,unsigned size){
        Cancel();target.Clear();target.error="Invalid NAV";mesh=&target;r={bytes,size};
        if(!bytes||size<32||size>MaxBytes)return Fail("Invalid NAV size");
        if(r.U32()!=0xfeedface)return Fail("Invalid NAV magic");
        target.version=r.U32();target.subversion=r.U32();target.bspSize=r.U32();
        if((target.version!=15&&target.version!=16)||target.subversion<10||target.subversion>14)
            return Fail("Unsupported NAV version (requires v15/v16/sub10..14)");
        r.U8();r.Strings();r.U8();if(target.subversion>=12)r.CString();r.Strings();total=r.U32();
        if(!r.ok||!total||total>MaxAreas||total>(size-r.pos)/90)return Fail("Invalid NAV area count");
        target.areas=(Area*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,total*sizeof(Area));
        if(!target.areas)return Fail("NAV allocation failed");
        phase=1;areaAt=ladderAt=edgeAt=refAt=0;areaStage=areaDirection=areaRemaining=encounterRemaining=refDirection=0;return Result::Working;
    }
    Result Step(unsigned areaBudget=256,unsigned edgeBudget=8192){
        if(!mesh||!phase)return Result::Idle;
        Mesh& m=*mesh;
        unsigned workLeft=edgeBudget?edgeBudget:1;
        if(phase==1){
            unsigned done=0;
            while(areaAt<total&&done<areaBudget&&workLeft&&r.ok){
                Area& a=m.areas[areaAt];
                if(areaStage==0){
                    a.id=r.U32();a.flags=r.U32();
                    a.x1=r.F32();a.y1=r.F32();a.z1=r.F32();a.x2=r.F32();a.y2=r.F32();a.z2=r.F32();a.zNE=r.F32();a.zSW=r.F32();
                    const float coords[]={a.x1,a.y1,a.z1,a.x2,a.y2,a.z2,a.zNE,a.zSW};
                    for(float v:coords)if(!Finite(v)||Abs(v)>1000000)return Fail("Invalid NAV coordinates");
                    if(a.x2<a.x1||a.y2<a.y1)return Fail("Reversed NAV area bounds");
                    a.firstEdge=m.edgeCount;areaDirection=0;areaRemaining=0;areaStage=1;--workLeft;continue;
                }
                if(areaStage==1){
                    if(areaRemaining){m.edges[m.edgeCount++]=r.U32();--areaRemaining;--workLeft;continue;}
                    if(areaDirection<4){
                        unsigned n=r.U32();
                        if(!r.ok||n>(r.size-r.pos)/4||n>MaxEdges-m.edgeCount||!m.ReserveEdges(m.edgeCount+n))return Fail("Invalid/excessive NAV connections");
                        areaRemaining=n;++areaDirection;--workLeft;continue;
                    }
                    a.edgeCount=m.edgeCount-a.firstEdge;areaStage=2;continue;
                }
                if(areaStage==2){unsigned hiding=r.U8();r.Array(hiding,17);areaStage=3;--workLeft;continue;}
                if(areaStage==3){
                    encounterRemaining=r.U32();
                    if(encounterRemaining>(r.size-r.pos)/11)return Fail("Invalid NAV encounters");
                    areaStage=4;--workLeft;continue;
                }
                if(areaStage==4){
                    if(encounterRemaining){r.Skip(10);unsigned n=r.U8();r.Array(n,5);--encounterRemaining;--workLeft;continue;}
                    areaStage=5;continue;
                }
                if(areaStage==5){r.Skip(2);refDirection=0;areaRemaining=0;areaStage=6;--workLeft;continue;}
                if(areaStage==6){
                    if(areaRemaining){m.ladderRefs[m.ladderRefCount++]={areaAt,r.U32(),refDirection-1};--areaRemaining;--workLeft;continue;}
                    if(refDirection<2){
                        unsigned n=r.U32();
                        if(!r.ok||n>(r.size-r.pos)/4||n>MaxEdges/3-m.ladderRefCount||!m.ReserveLadderRefs(m.ladderRefCount+n))return Fail("Invalid/excessive NAV ladder refs");
                        areaRemaining=n;++refDirection;--workLeft;continue;
                    }
                    areaStage=7;continue;
                }
                if(areaStage==7){
                    r.Skip(24);a.spawn=r.U32();if(m.subversion<13)r.Skip(4);r.Skip(2);
                    unsigned visible=r.U32();r.Array(visible,5);if(m.subversion>10)r.Skip(4);
                    areaStage=0;++areaAt;++done;--workLeft;continue;
                }
                return Fail("Invalid NAV parser state");
            }
            if(!r.ok)return Fail("Truncated NAV area data");
            if(areaAt<total)return Result::Working;
            if(!workLeft)return Result::Working;
            m.ladderCount=r.U32();
            --workLeft;
            if(!r.ok||m.ladderCount>MaxAreas||m.ladderCount>(r.size-r.pos)/60)return Fail("Truncated/excessive NAV ladder data");
            if(m.ladderCount){m.ladders=(Ladder*)HeapAlloc(GetProcessHeap(),0,m.ladderCount*sizeof(Ladder));if(!m.ladders)return Fail("NAV ladder allocation failed");}
            phase=2;
        }
        if(phase==2){
            unsigned done=0;
            while(ladderAt<m.ladderCount&&done<areaBudget&&workLeft){++done;--workLeft;
                auto& l=m.ladders[ladderAt++];l.id=r.U32();l.width=r.F32();l.top={r.F32(),r.F32(),r.F32()};l.bottom={r.F32(),r.F32(),r.F32()};l.length=r.F32();l.direction=r.U32();for(auto& id:l.upper)id=r.U32();l.lower=r.U32();
                const float values[]={l.width,l.top.x,l.top.y,l.top.z,l.bottom.x,l.bottom.y,l.bottom.z,l.length};
                for(float v:values)if(!Finite(v)||Abs(v)>1000000)return Fail("Invalid NAV ladder geometry");
                if(l.width<0||l.length<0||l.direction>3)return Fail("Invalid NAV ladder fields");
            }
            if(!r.ok)return Fail("Truncated NAV ladder data");
            if(ladderAt<m.ladderCount)return Result::Working;
            m.parsedBytes=r.pos;
            areaCap=8;while(areaCap<total*2)areaCap*=2;
            areasByID=(unsigned*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,areaCap*4);
            if(!areasByID)return Fail("NAV graph allocation failed");
            phase=3;areaAt=0;
        }
        if(phase==3){
            while(areaAt<total&&workLeft){
                --workLeft;unsigned i=areaAt++;unsigned at=(m.areas[i].id*2654435761u)&(areaCap-1);unsigned probes=0;
                while(areasByID[at]){if(m.areas[areasByID[at]-1].id==m.areas[i].id)return Fail("Duplicate NAV area IDs");
                    if(++probes>256)return Fail("Excessive NAV area ID collisions");at=(at+1)&(areaCap-1);}
                areasByID[at]=i+1;
            }
            if(areaAt<total)return Result::Working;phase=4;
        }
        if(phase==4){
            while(edgeAt<m.edgeCount&&workLeft){
                --workLeft;unsigned id=m.edges[edgeAt],at=(id*2654435761u)&(areaCap-1);unsigned probes=0;
                while(areasByID[at]&&m.areas[areasByID[at]-1].id!=id){if(++probes>256)return Fail("Excessive NAV edge lookup collisions");at=(at+1)&(areaCap-1);}
                m.edges[edgeAt++]=areasByID[at]?areasByID[at]-1:Invalid;
            }
            if(edgeAt<m.edgeCount)return Result::Working;
            if(!m.ladderCount||!m.ladderRefCount){m.count=total;m.error=nullptr;Cancel();return Result::Ready;}
            if(m.ladderCount&&m.ladderRefCount){
                ladderCap=8;while(ladderCap<m.ladderCount*2)ladderCap*=2;
                laddersByID=(unsigned*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,ladderCap*4);
                m.ladderLinks=(LadderLink*)HeapAlloc(GetProcessHeap(),0,m.ladderRefCount*3*sizeof(LadderLink));
                if(!laddersByID||!m.ladderLinks)return Fail("NAV ladder graph allocation failed");
            }
            phase=5;ladderAt=0;
        }
        if(phase==5){
            while(ladderAt<m.ladderCount&&workLeft&&laddersByID){
                --workLeft;unsigned i=ladderAt++;unsigned at=(m.ladders[i].id*2654435761u)&(ladderCap-1);unsigned probes=0;
                while(laddersByID[at]){if(m.ladders[laddersByID[at]-1].id==m.ladders[i].id)return Fail("Duplicate NAV ladder IDs");
                    if(++probes>256)return Fail("Excessive NAV ladder ID collisions");at=(at+1)&(ladderCap-1);}
                laddersByID[at]=i+1;
            }
            if(ladderAt<m.ladderCount)return Result::Working;phase=6;
        }
        if(phase==6){
            while(refAt<m.ladderRefCount&&workLeft&&laddersByID){
                --workLeft;const auto& ref=m.ladderRefs[refAt++];unsigned at=(ref.id*2654435761u)&(ladderCap-1);unsigned probes=0;
                while(laddersByID[at]&&m.ladders[laddersByID[at]-1].id!=ref.id){if(++probes>256)return Fail("Excessive NAV ladder lookup collisions");at=(at+1)&(ladderCap-1);}
                if(!laddersByID[at])continue;unsigned l=laddersByID[at]-1;
                for(unsigned j=0;j<(ref.down?1u:3u);++j){unsigned id=ref.down?m.ladders[l].lower:m.ladders[l].upper[j];if(!id)continue;
                    unsigned to=Mesh::Lookup(areasByID,areaCap,m.areas,id);if(to<total&&to!=ref.area)m.ladderLinks[m.ladderLinkCount++]={ref.area,to,l,ref.down};}
            }
            if(refAt<m.ladderRefCount)return Result::Working;
            m.count=total;m.error=nullptr;Cancel();return Result::Ready;
        }
        return Result::Working;
    }
};
}
