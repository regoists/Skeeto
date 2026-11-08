#pragma once
#include "render.h"
#include "../skeeto_ui_text.h"
namespace minimap {
constexpr unsigned MainText=0xe5ecee,Secondary=0xacbdc5,RouteColor=0x65dcd1,Amber=0xf2bc70,Failure=0xf1847c;
struct NavPresentation {const WCHAR* text;unsigned color;};
inline NavPresentation NavigationStatus(const guidance::Navigator& n,bool compact=false){
    using S=guidance::State;
    // A usable route may be published before the reverse search finishes.
    if(n.state==S::Route)return {n.routeUnusual?(compact?UiText(UiId::NavHud0, L"参考路线 注意橙段"):UiText(UiId::NavHud1, L"参考路线 留意橙色路段")):n.routeLadders?UiText(UiId::NavHud2, L"参考路线 途经梯子"):UiText(UiId::NavHud3, L"参考路线"),RouteColor};
    if(n.state==S::AtGoal)return {UiText(UiId::NavHud4, L"已到候选区域"),RouteColor};
    if(n.state==S::Searching)return {compact?UiText(UiId::NavHud5, L"计算中 示方向"):UiText(UiId::NavHud6, L"计算中 仅示方向"),Secondary};
    if(n.state==S::DirectionOnly){
        if(n.source==nav::Invalid||n.source>=n.capacity)return {compact?UiText(UiId::NavHud7, L"未定位 示方向"):UiText(UiId::NavHud8, L"未定位 仅示方向"),Amber};
        if(n.building)return {compact?UiText(UiId::NavHud5, L"计算中 示方向"):UiText(UiId::NavHud6, L"计算中 仅示方向"),Secondary};
        if(n.nodes&&n.nodes[n.source].visit!=n.generation)return {compact?UiText(UiId::NavHud9, L"路线不通 仅示方向"):UiText(UiId::NavHud9, L"路线不通 仅示方向"),Failure};
        return {compact?UiText(UiId::NavHud10, L"路线未完整 示方向"):UiText(UiId::NavHud11, L"路线未完整 仅示方向"),Amber};
    }
    if(n.state==S::NoGoals)return {UiText(UiId::NavHud12, L"未找到安全屋标记"),Amber};
    if(n.state==S::Ambiguous)return {UiText(UiId::NavHud13, L"请选择安全屋候选"),Amber};
    if(n.state==S::InvalidGoal)return {UiText(UiId::NavHud14, L"该候选不存在"),Amber};
    if(n.state==S::Unavailable)return {UiText(UiId::NavHud15, L"导航暂不可用"),Failure};
    if(n.state==S::NoNav)return {UiText(UiId::NavHud16, L"无 NAV 地图"),Amber};
    return {UiText(UiId::NavHud17, L"准备导航"),Secondary};
}
// Bounded, allocation-free text fitting with ordinary glyphs only.
inline int TextAdvance(WCHAR c,int pixels){return c>=128?pixels+1:c==L' '?pixels/2:(pixels*2+2)/3;}
inline unsigned FitText(WCHAR* out,unsigned capacity,const WCHAR* text,int width,int pixels){
    if(!capacity)return 0;unsigned n=0;int used=0;
    while(text[n]&&n+1<capacity){int advance=TextAdvance(text[n],pixels);if(used+advance>width)break;out[n]=text[n];used+=advance;++n;}
    out[n]=0;return n;
}
inline int SupplyFont(int base){return Limit((base*8+3)/7,14,30);}
inline int LabelWidth(items::Label label,int font){
    // Conservative advances for the finite alias alphabet; m/w are wider.
    int width=8;for(auto t=items::Text(label);*t;++t)width+=*t>=128?font+1:(*t==L'm'||*t==L'w')?font:(font*2+2)/3;return width;
}
inline unsigned ItemColor(items::Kind k){
    using K=items::Kind;switch(k){case K::Ammo:return 0xdacca6;case K::Bile:return 0xb6c989;
    case K::Molotov:case K::PipeBomb:return 0xe9bb85;case K::Laser:return 0xcbbbe7;case K::Weapon:return 0xc4cdd3;default:return 0xd7b4c5;}
}
struct LabelStyle {int width,height;unsigned color;};
struct LabelMetrics {
    LabelStyle entries[unsigned(items::Label::Count)]{};int font=0;unsigned builds=0;
    void Prepare(int nextFont){if(font==nextFont)return;font=nextFont;++builds;
        for(unsigned i=0;i<unsigned(items::Label::Count);++i){auto label=items::Label(i);bool weapon=label>=items::Label::Weapon;
            int pixels=weapon?font:SupplyFont(font);entries[i]={LabelWidth(label,pixels)+2,pixels+5,ItemColor(weapon?items::Kind::Weapon:items::Kind(i))};}}
    const LabelStyle& Get(items::Label label)const{return entries[unsigned(label)<unsigned(items::Label::Count)?unsigned(label):unsigned(items::Label::Weapon)];}
};
struct Box {int x,y,w,h;bool Overlaps(const Box& b,int gap)const{return x<b.x+b.w+gap&&x+w+gap>b.x&&y<b.y+b.h+gap&&y+h+gap>b.y;}};
// All four corners of a screen-space box lie inside the map shape (`limit` in texture pixels).
inline bool InsideShape(const Box& b,int size,int shape,float limit){
    float c=size*.5f,k=float(Pixels)/size;
    for(int i=0;i<4;++i){bool out=false;ClampShape({Center+((i&1?b.x+b.w:b.x)-c)*k,Center+((i&2?b.y+b.h:b.y)-c)*k},shape,limit,&out);if(out)return false;}
    return true;
}
struct LabelPlacement {Box box;items::Label label;items::Kind kind;int entity,slot,anchorX,anchorY;bool occluded=false;};
struct LabelLayout {
    LabelMetrics metrics;
    Box leaders[items::Capacity*3]{};int leaderCount=0;
    LabelPlacement labels[items::Capacity]{},previous[items::Capacity]{},tracked[items::Capacity]{};int trackedCount=0,count=0,hidden=0,crowded=0,outside=0;
    int previousSize=0,previousFont=0;unsigned previousMesh=~0u;
    Box protectedBoxes[68]{};int protectedCount=0;
    void Protect(Point p,float factor,int radius){if(protectedCount<68)protectedBoxes[protectedCount++]={int(p.x*factor+.5f)-radius,int(p.y*factor+.5f)-radius,2*radius+1,2*radius+1};}
    bool Protected(const Box& box,int gap=1)const{for(int i=0;i<protectedCount;++i)if(box.Overlaps(protectedBoxes[i],gap))return true;return false;}
    void Clear(){trackedCount=count=hidden=crowded=outside=leaderCount=0;previousSize=0;}
    void AddLeader(const Box& b){if(b.w<=0||b.h<=0||Protected(b))return;
        for(int i=0;i<leaderCount;++i){const auto& p=leaders[i];if(p.x==b.x&&p.y==b.y&&p.w==b.w&&p.h==b.h)return;}
        if(leaderCount<items::Capacity*3)leaders[leaderCount++]=b;
    }
    void BuildLeaders(){leaderCount=0;
        for(int i=0;i<count;++i){const auto& l=labels[i];const auto& b=l.box;
            int ax=l.anchorX,ay=l.anchorY,ex=Limit(ax,b.x,b.x+b.w-1),ey=Limit(ay,b.y,b.y+b.h-1);
            if(ex==ax&&ey==ay)continue;
            int left=ex<ax?ex:ax,top=ey<ay?ey:ay,w=ex<ax?ax-ex:ex-ax,h=ey<ay?ay-ey:ey-ay;
            Box horizontal{left,ay,w+1,1},vertical{ex,top,1,h+1};
            if(Protected(horizontal)||Protected(vertical)){horizontal.y=ey;vertical.x=ax;}
            // Avoid disconnected line fragments across important markers.
            if(!Protected(horizontal)&&!Protected(vertical)){if(w)AddLeader(horizontal);if(h)AddLeader(vertical);}
            AddLeader({ax-1,ay-1,3,3});
        }
    }
    void Build(const Scene& scene,const items::Snapshot& snapshot,const guidance::Navigator* n,int size,int font){
        metrics.Prepare(font);
        int oldCount=size==previousSize&&font==previousFont&&scene.meshSerial==previousMesh?trackedCount:0;
        for(int i=0;i<oldCount;++i)previous[i]=tracked[i];
        previousSize=size;previousFont=font;previousMesh=scene.meshSerial;
        trackedCount=count=hidden=crowded=outside=protectedCount=0;float factor=float(size)/Pixels;const auto& v=scene.view;float unit=factor*v.markerScale;
        Protect({Pixels*.5f,Pixels*.5f},factor,int(15*unit+2.5f));
        for(int i=0;i<scene.count;++i)Protect(TeamPoint(v,scene.teammates[i]),factor,int(13*unit+2.5f));
        if(n&&n->selected>=0&&unsigned(n->selected)<n->goalCount)Protect(TargetPoint(v,*n),factor,int(17*unit+2.5f));
        Protect(NorthPoint(v),factor,int((NorthBadge(v.markerScale)+2)*factor+2.5f));
        const float labelLimit=TickOuter-2;const int shape=v.shape;
        // Keep slots for both visible and temporarily occluded entities ahead
        // of new arrivals; moving markers must not trigger a new layout search.
        int order[items::Capacity],oldSlot[items::Capacity],ordered=0;bool queued[items::Capacity]{},wasOccluded[items::Capacity]{};
        for(int p=0;p<oldCount;++p)for(int i=0;i<snapshot.count;++i){const auto& m=snapshot.markers[i];const auto& old=previous[p];
            if(!queued[i]&&m.entity==old.entity&&m.label==old.label&&m.kind==old.kind){order[ordered]=i;wasOccluded[ordered]=old.occluded;oldSlot[ordered++]=old.slot;queued[i]=true;break;}}
        for(int i=0;i<snapshot.count;++i)if(!queued[i]){order[ordered]=i;oldSlot[ordered++]=-1;}
        const int directions[8][2]={{0,-1},{1,0},{0,1},{-1,0},{1,-1},{1,1},{-1,1},{-1,-1}};
        for(int index=0;index<ordered;++index){const auto& m=snapshot.markers[order[index]];Point at=v.Project(m.position.x,m.position.y);
            int ax=int(at.x*factor+.5f),ay=int(at.y*factor+.5f);
            const auto& style=metrics.Get(m.label);int w=style.width,h=style.height;
            bool placed=false;int retained=oldSlot[index];
            bool offscreen=!InsideShape({ax,ay,1,1},size,shape,labelLimit);
            if(offscreen&&retained<0){++outside;++hidden;continue;}
            // Search 17 slots only on first placement. Existing slots are
            // pinned; there is no heap allocation or moving-label carousel.
            for(int attempt=retained<0?0:-1;attempt<17;++attempt){int slot=attempt<0?retained:attempt;if(attempt>=0&&slot==retained)continue;
                int dx=0,dy=0;if(slot){int ring=(slot-1)/8,dir=(slot-1)%8;dx=directions[dir][0]*(w/2+10+ring*18);dy=directions[dir][1]*(h/2+10+ring*18);}
                Box b{Limit(ax+dx-w/2,5,size-5-w),Limit(ay+dy-h/2,5,size-5-h),w,h};
                if(retained>=0){
                    // An existing label never changes its slot due to contact.
                    // Remember it through occlusion, including the screen edge.
                    bool blocked=offscreen||!InsideShape(b,size,shape,labelLimit)||Protected(b,wasOccluded[index]?7:3);
                    for(int j=0;!blocked&&j<count;++j)blocked=b.Overlaps(labels[j].box,wasOccluded[index]?8:4);
                    LabelPlacement fixed{b,m.label,m.kind,m.entity,slot,ax,ay,blocked};tracked[trackedCount++]=fixed;
                    if(blocked){++hidden;if(offscreen)++outside;else ++crowded;}
                    else labels[count++]=fixed;
                    placed=true;break;
                }
                bool reject=!InsideShape(b,size,shape,labelLimit)||Protected(b,retained>=0?3:6);
                // Reserve temporarily covered slots so new arrivals do not take them.
                for(int j=0;!reject&&j<trackedCount;++j)reject=b.Overlaps(tracked[j].box,8);
                if(!reject){LabelPlacement fixed{b,m.label,m.kind,m.entity,slot,ax,ay};labels[count++]=fixed;tracked[trackedCount++]=fixed;placed=true;break;}
            }
            if(!placed){++crowded;++hidden;}
        }
        BuildLeaders();
    }
};
}

