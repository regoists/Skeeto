#pragma once
#include "nav.h"
namespace guidance {
constexpr unsigned Checkpoint=2048,Rescue=32768,PlayerStart=128;
constexpr unsigned Excluded=65536|131072|262144|134217728|268435456;
constexpr unsigned MaxGoals=16,MaxRoute=2048;
inline Vec Center(const nav::Area& a){float x=(a.x1+a.x2)*.5f,y=(a.y1+a.y2)*.5f;return{x,y,nav::Height(a,x,y)};}
inline float DistanceSq(const nav::Area& a,Vec p){Vec q{Limit(p.x,a.x1,a.x2),Limit(p.y,a.y1,a.y2),0};q.z=nav::Height(a,q.x,q.y);Vec d=q-p;return d.x*d.x+d.y*d.y+d.z*d.z*4;}
inline bool Walkable(const nav::Area& a){return !(a.flags&Excluded);}
inline Vec Portal(const nav::Area& a,const nav::Area& b){
    float x=(nav::Max(a.x1,b.x1)+nav::Min(a.x2,b.x2))*.5f,y=(nav::Max(a.y1,b.y1)+nav::Min(a.y2,b.y2))*.5f;
    return{x,y,nav::Height(a,x,y)};
}
inline bool Passable(const nav::Area& a,const nav::Area& b){
    if(!Walkable(b))return false;Vec p=Portal(a,b);
    float dx=nav::Max(0.f,nav::Max(a.x1-b.x2,b.x1-a.x2)),dy=nav::Max(0.f,nav::Max(a.y1-b.y2,b.y1-a.y2));
    float dz=nav::Height(b,p.x,p.y)-p.z;
    // Static NAV cannot decide when elevators or scripted doors are ready.
    return dx*dx+dy*dy<=80*80&&dz<=64&&dz>=-180;
}
enum class State {Off,NoNav,NoGoals,Ambiguous,InvalidGoal,Searching,Route,DirectionOnly,AtGoal,Unavailable};
enum class SearchPass : unsigned char {Ground,Unusual,Ladders};
struct Goal {unsigned root=0,area=0,areas=0;Vec position{};bool start=false,rescue=false;};
struct Node {unsigned parent,visit;int group;unsigned edge;};
struct BackLink {unsigned from,next,ladder=nav::Invalid,down=0;bool unusual=false;};
struct Navigator {
    Node* nodes=nullptr;unsigned* queue=nullptr;unsigned capacity=0;
    Goal goals[MaxGoals]{};unsigned goalCount=0;bool overflow=false,prepared=false;
    unsigned route[MaxRoute]{},routeCount=0,serial=0,generation=0,head=0,tail=0,edgeCursor=0,source=nav::Invalid;
    unsigned* incoming=nullptr;BackLink* links=nullptr;unsigned linkCount=0,routeSource=nav::Invalid,searches=0;bool building=false;
    SearchPass searchPass=SearchPass::Ground;unsigned routeLadders=0,routeUnusual=0;
    // Optional progress consumer: length is accumulated in the existing bounded
    // route walk, never by another graph search or per-area distance array.
    bool measureLength=false;float routeLength=0;unsigned lengthVisits=0,locationQueries=0;
    int selected=-1;State state=State::Off;
    unsigned preparePhase=0,prepareAt=0,prepareEdgeAt=0;Vec prepareAnchor{};float goalBest[MaxGoals]{};
    void Clear(){if(nodes)HeapFree(GetProcessHeap(),0,nodes);if(queue)HeapFree(GetProcessHeap(),0,queue);if(incoming)HeapFree(GetProcessHeap(),0,incoming);if(links)HeapFree(GetProcessHeap(),0,links);
        nodes=nullptr;queue=incoming=nullptr;links=nullptr;capacity=goalCount=routeCount=head=tail=edgeCursor=generation=linkCount=searches=routeLadders=routeUnusual=lengthVisits=locationQueries=0;routeLength=0;selected=-1;state=State::Off;prepared=overflow=building=false;searchPass=SearchPass::Ground;preparePhase=prepareAt=prepareEdgeAt=0;source=routeSource=nav::Invalid;++serial;}
    unsigned Root(unsigned index){while(nodes[index].parent!=index){nodes[index].parent=nodes[nodes[index].parent].parent;index=nodes[index].parent;}return index;}
    bool BeginPrepare(const nav::Mesh& mesh,Vec anchor){
        Clear();prepareAnchor=anchor;
        if(!mesh.count){prepared=true;state=State::NoNav;return false;}
        nodes=(Node*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,mesh.count*sizeof(Node));
        queue=(unsigned*)HeapAlloc(GetProcessHeap(),0,mesh.count*4);
        unsigned totalLinks=mesh.edgeCount+mesh.ladderLinkCount;
        incoming=(unsigned*)HeapAlloc(GetProcessHeap(),0,mesh.count*4);
        if(totalLinks)links=(BackLink*)HeapAlloc(GetProcessHeap(),0,totalLinks*sizeof(BackLink));
        if(!nodes||!queue||!incoming||(totalLinks&&!links)){Clear();prepared=true;state=State::Unavailable;return false;}
        capacity=mesh.count;preparePhase=1;prepareAt=prepareEdgeAt=0;state=State::Searching;return true;
    }
    bool PrepareStep(const nav::Mesh& mesh,unsigned budget=512){
        if(!preparePhase)return prepared;
        if(capacity!=mesh.count){Clear();prepared=true;state=State::Unavailable;return false;}
        unsigned done=0;
        while(preparePhase&&done<budget){
            if(preparePhase==1){
                while(prepareAt<capacity&&done++<budget){nodes[prepareAt].parent=prepareAt;nodes[prepareAt].group=-1;incoming[prepareAt]=nav::Invalid;++prepareAt;}
                if(prepareAt<capacity)break;preparePhase=2;prepareAt=0;continue;
            }
            if(preparePhase==2){
                while(prepareAt<capacity&&done<budget){
                    unsigned i=prepareAt;const auto& a=mesh.areas[i];
                    if(!Walkable(a)||prepareEdgeAt>=a.edgeCount){++prepareAt;prepareEdgeAt=0;++done;continue;}
                    unsigned to=mesh.edges[a.firstEdge+prepareEdgeAt++];++done;
                    if(to>=capacity||!Walkable(mesh.areas[to]))continue;
                    links[linkCount]={i,incoming[to],nav::Invalid,0,!Passable(a,mesh.areas[to])};incoming[to]=linkCount++;
                }
                if(prepareAt<capacity)break;preparePhase=3;prepareAt=prepareEdgeAt=0;continue;
            }
            if(preparePhase==3){
                while(prepareAt<mesh.ladderLinkCount&&done++<budget){const auto& link=mesh.ladderLinks[prepareAt++];if(link.from>=capacity||link.to>=capacity||!Walkable(mesh.areas[link.from])||!Walkable(mesh.areas[link.to]))continue;
                    links[linkCount]={link.from,incoming[link.to],link.ladder,link.down,false};incoming[link.to]=linkCount++;}
                if(prepareAt<mesh.ladderLinkCount)break;preparePhase=4;prepareAt=0;continue;
            }
            if(preparePhase==4){
                while(prepareAt<capacity&&done<budget){
                    unsigned i=prepareAt;const auto& a=mesh.areas[i];
                    if(!(a.spawn&(Checkpoint|Rescue))||prepareEdgeAt>=a.edgeCount){++prepareAt;prepareEdgeAt=0;++done;continue;}
                    unsigned to=mesh.edges[a.firstEdge+prepareEdgeAt++];++done;
                    if(to>=capacity)continue;
                    const auto& b=mesh.areas[to];if((a.spawn&(Checkpoint|Rescue))==(b.spawn&(Checkpoint|Rescue)))nodes[Root(to)].parent=Root(i);
                }
                if(prepareAt<capacity)break;preparePhase=5;prepareAt=prepareEdgeAt=0;continue;
            }
            if(preparePhase==5){
                while(prepareAt<capacity&&done++<budget){unsigned i=prepareAt++;const auto& a=mesh.areas[i];if(!(a.spawn&(Checkpoint|Rescue)))continue;
                    unsigned root=Root(i),g=0;for(;g<goalCount&&goals[g].root!=root;++g){}
                    if(g==goalCount){if(goalCount==MaxGoals){overflow=true;continue;}goals[goalCount++]={root,i,0,{},false,(a.spawn&Rescue)!=0};}
                    auto& goal=goals[g];nodes[i].group=int(g);goal.position=goal.position+Center(a);++goal.areas;
                    if((a.spawn&PlayerStart)||(!goal.rescue&&DistanceSq(a,prepareAnchor)<512*512))goal.start=true;}
                if(prepareAt<capacity)break;
                for(unsigned g=0;g<goalCount;++g){goals[g].position=goals[g].position*(1.f/goals[g].areas);goalBest[g]=1e30f;}
                preparePhase=6;prepareAt=0;continue;
            }
            if(preparePhase==6){
                while(prepareAt<capacity&&done++<budget){unsigned i=prepareAt++;int g=nodes[i].group;if(g<0||unsigned(g)>=goalCount)continue;
                    Vec d=Center(mesh.areas[i])-goals[g].position;float score=d.Dot(d);if(score<goalBest[g]){goalBest[g]=score;goals[g].area=i;}}
                if(prepareAt<capacity)break;
                for(unsigned g=0;g<goalCount;++g)goals[g].position=Center(mesh.areas[goals[g].area]);
                prepared=true;preparePhase=0;state=goalCount?State::Ambiguous:State::NoGoals;return true;
            }
        }
        return prepared;
    }
    void Select(int requested){
        if(state==State::NoNav||state==State::Unavailable)return;
        int choice=-1;if(requested){if(unsigned(requested)<=goalCount)choice=requested-1;}
        else if(!overflow){
            // NAVs commonly contain several checkpoint roots plus a rescue area.
            // Auto used to give up whenever more than one non-start root existed,
            // leaving the user with a "choose a safehouse" prompt. Prefer the
            // nearest checkpoint to the map-entry anchor; only fall back to a
            // rescue root when no non-start checkpoint is present. The anchor is
            // fixed for this map, so the choice cannot oscillate as the player moves.
            bool haveCheckpoint=false;
            for(unsigned g=0;g<goalCount;++g)
                if(!goals[g].start&&!goals[g].rescue){haveCheckpoint=true;break;}
            float best=1e30f;
            for(unsigned g=0;g<goalCount;++g){
                const Goal& goal=goals[g];
                if(goal.start||(haveCheckpoint&&goal.rescue))continue;
                Vec d=goal.position-prepareAnchor;float score=d.Dot(d);
                if(score<best){best=score;choice=int(g);}
            }
        }
        if(choice!=selected){selected=choice;source=routeSource=nav::Invalid;routeCount=head=tail=routeLadders=routeUnusual=0;edgeCursor=nav::Invalid;building=false;searchPass=SearchPass::Ground;state=State::Ambiguous;++serial;
            if(selected>=0){if(++generation==0){for(unsigned i=0;i<capacity;++i)nodes[i].visit=0;generation=1;}
                for(unsigned i=0;i<capacity;++i)if(nodes[i].group==selected){nodes[i].visit=generation;nodes[i].parent=nodes[i].edge=nav::Invalid;queue[tail++]=i;}
                building=tail!=0;state=building?State::Searching:State::DirectionOnly;++searches;}}
        if(selected<0){State next=!goalCount?State::NoGoals:requested?State::InvalidGoal:State::Ambiguous;if(state!=next){state=next;++serial;}}
    }
    unsigned Nearest(const nav::Mesh& mesh,Vec p)const{unsigned result=nav::Invalid;float best=256*256;
        for(unsigned i=0;i<mesh.count;++i){const auto& a=mesh.areas[i];if(!Walkable(a))continue;float z=nav::Height(a,p.x,p.y);if(z>p.z+32||z<p.z-180)continue;
            float score=DistanceSq(a,p);if(score<best){best=score;result=i;}}
        // Mid-ladder positions may be far above either floor. Keep navigation
        // attached to an explicitly linked, goal-reachable ladder endpoint.
        if(result==nav::Invalid)for(unsigned i=0;i<mesh.ladderLinkCount;++i){const auto& link=mesh.ladderLinks[i];
            if(link.from>=capacity||link.to>=capacity||link.ladder>=mesh.ladderCount||nodes[link.from].visit!=generation||!Walkable(mesh.areas[link.from])||!Walkable(mesh.areas[link.to]))continue;
            const auto& l=mesh.ladders[link.ladder];Vec d=l.top-l.bottom;float length=d.Dot(d);if(length<1)continue;float t=Limit((p-l.bottom).Dot(d)/length,0.f,1.f);Vec diff=p-(l.bottom+d*t);float score=diff.Dot(diff);
            if(score<=64*64&&score<best){best=score;result=link.from;}}
        return result;
    }
    void Begin(const nav::Mesh& mesh,Vec p){
        if(selected<0||capacity!=mesh.count)return;++locationQueries;source=Nearest(mesh,p);
        // The ground field may have finished while the player was elsewhere.
        // Resume its same queue only when this new location needs a ladder.
        if(!building&&searchPass==SearchPass::Unusual&&source<capacity&&nodes[source].visit!=generation){
            searchPass=SearchPass::Ladders;head=0;edgeCursor=nav::Invalid;building=tail!=0;}
        Refresh(mesh);
    }
    void Refresh(const nav::Mesh& mesh){
        State next=State::DirectionOnly;
        if(source<capacity&&nodes[source].visit==generation){
            if(routeSource==source&&(state==State::Route||state==State::AtGoal))return;
            routeSource=source;routeCount=routeLadders=routeUnusual=0;routeLength=0;unsigned cursor=source;
            while(cursor!=nav::Invalid&&cursor<capacity&&routeCount<MaxRoute){route[routeCount++]=cursor;unsigned edge=nodes[cursor].edge;
                if(edge<linkCount){if(links[edge].ladder!=nav::Invalid)++routeLadders;if(links[edge].unusual)++routeUnusual;}
                unsigned parent=nodes[cursor].parent;
                if(measureLength&&parent<capacity){
                    Vec a=Center(mesh.areas[cursor]),b=Center(mesh.areas[parent]);
                    if(edge<linkCount&&links[edge].ladder!=nav::Invalid){
                        const auto& link=links[edge];const auto& ladder=mesh.ladders[link.ladder];
                        Vec entry=link.down?ladder.top:ladder.bottom,exit=link.down?ladder.bottom:ladder.top;
                        Vec first=a-entry,last=b-exit;
                        routeLength+=std::sqrt(first.Dot(first))+ladder.length+std::sqrt(last.Dot(last));
                    }else{Vec delta=b-a;routeLength+=std::sqrt(delta.Dot(delta));}
                    ++lengthVisits;
                }
                cursor=parent;}
            if(cursor!=nav::Invalid){routeCount=0;next=State::DirectionOnly;}
            else next=routeCount==1?State::AtGoal:State::Route;
            state=next;++serial;return;
        }
        if(source<capacity&&building)next=State::Searching;
        if(state!=next||routeCount){routeCount=routeLadders=routeUnusual=0;routeSource=nav::Invalid;state=next;++serial;}
    }
    void Step(const nav::Mesh& mesh,unsigned budget=4096){
        if(!building||capacity!=mesh.count)return;
        while(building&&budget){
            // Prefer non-ladder ground paths, then their unusual NAV edges.
            // Only an unreached current source can enable the final ladder pass;
            // retain parents already found so climbing shortcuts cannot replace them.
            if(head==tail){
                if(searchPass==SearchPass::Ground)searchPass=SearchPass::Unusual;
                else if(searchPass==SearchPass::Unusual&&source<capacity&&nodes[source].visit!=generation)searchPass=SearchPass::Ladders;
                else{building=false;break;}
                head=0;edgeCursor=nav::Invalid;
            }
            --budget;unsigned at=queue[head];if(edgeCursor==nav::Invalid)edgeCursor=incoming[at];
            if(edgeCursor==nav::Invalid){++head;continue;}
            unsigned edge=edgeCursor;const auto& link=links[edge];unsigned from=link.from;edgeCursor=link.next;
            if(edgeCursor==nav::Invalid)++head;
            if(nodes[from].visit==generation||(searchPass==SearchPass::Ground&&link.unusual)
                ||(searchPass!=SearchPass::Ladders&&link.ladder!=nav::Invalid))continue;
            nodes[from].visit=generation;nodes[from].parent=at;nodes[from].edge=edge;queue[tail++]=from;
        }
        Refresh(mesh);
    }
    const WCHAR* Label()const{
        switch(state){
        case State::NoNav:return L"导航：无 NAV";case State::NoGoals:return L"未找到安全屋标记";
        case State::Ambiguous:return L"安全屋：请切换候选";case State::InvalidGoal:return L"该候选不存在";
        case State::Searching:return L"正在计算参考路线";case State::AtGoal:return L"已到候选区域";
        case State::Route:if(routeUnusual)return L"参考路线 留意橙色路段";if(routeLadders)return L"参考路线 途经梯子";return selected>=0&&goals[selected].rescue?L"撤离点 参考路线":L"安全屋 参考路线";
        case State::DirectionOnly:return source==nav::Invalid?L"候选方向 未定位地面":L"候选方向 无连通路径";case State::Unavailable:return L"导航暂不可用";
        default:return L"";
        }
    }
};
}

