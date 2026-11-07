#pragma once
#include "guidance.h"
#include "../skeeto_ui_text.h"
namespace worldnav {
constexpr unsigned MaxMarks=12;
constexpr float Reach=1600.f,Spacing=100.f;
inline float Sqrt(float x){return std::sqrt(x);}
inline float Length(Vec p){return Sqrt(p.Dot(p));}
inline Vec Cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline bool Valid(Vec p){return Finite(p.x)&&Finite(p.y)&&Finite(p.z)&&nav::Abs(p.x)<1000000&&nav::Abs(p.y)<1000000&&nav::Abs(p.z)<1000000;}
struct Point {float x=0,y=0,depth=0;};
struct Camera {
    Matrix matrix{};Vec eye{},forward{};int width=0,height=0;bool valid=false;
    bool Set(const Matrix& value,int w,int h){
        valid=false;if(w<640||h<400||w>16384||h>16384)return false;
        for(auto& row:value.m)for(float v:row)if(!Finite(v))return false;
        Vec a{value.m[0][0],value.m[0][1],value.m[0][2]},b{value.m[1][0],value.m[1][1],value.m[1][2]},c{value.m[3][0],value.m[3][1],value.m[3][2]};
        float det=a.Dot(Cross(b,c)),length=Length(c);if(nav::Abs(det)<.0001f||length<.01f)return false;
        eye=(Cross(b,c)*(-value.m[0][3])+Cross(c,a)*(-value.m[1][3])+Cross(a,b)*(-value.m[3][3]))*(1.f/det);
        if(!Valid(eye))return false;matrix=value;width=w;height=h;forward=c*(1.f/length);valid=true;return true;
    }
    bool Project(Vec p,Point& out)const{
        if(!valid||!Valid(p))return false;
        float w=matrix.m[3][0]*p.x+matrix.m[3][1]*p.y+matrix.m[3][2]*p.z+matrix.m[3][3];
        if(!Finite(w)||w<4.f)return false;
        out={(.5f+.5f*(matrix.m[0][0]*p.x+matrix.m[0][1]*p.y+matrix.m[0][2]*p.z+matrix.m[0][3])/w)*width,
             (.5f-.5f*(matrix.m[1][0]*p.x+matrix.m[1][1]*p.y+matrix.m[1][2]*p.z+matrix.m[1][3])/w)*height,w};
        return Finite(out.x)&&Finite(out.y)&&out.x>=3&&out.y>=3&&out.x<width-3&&out.y<height-3;
    }
    // Screen-right and camera-forward components, also valid behind the camera.
    void Bearing(Vec p,float& right,float& front)const{
        Vec d=p-eye;right=matrix.m[0][0]*d.x+matrix.m[0][1]*d.y+matrix.m[0][2]*d.z;front=forward.Dot(d);
    }
};
enum class Cue {Forward,Left,Right,Up,Down,LadderUp,LadderDown,Special,Arrived};
inline const WCHAR* CueText(Cue cue){switch(cue){
    case Cue::Left:return UiText(UiId::NavHud18, L"前方左转");case Cue::Right:return UiText(UiId::NavHud19, L"前方右转");
    case Cue::Up:return UiText(UiId::NavHud20, L"前方向上");case Cue::Down:return UiText(UiId::NavHud21, L"前方向下");
    case Cue::LadderUp:return UiText(UiId::NavHud22, L"前方上梯");case Cue::LadderDown:return UiText(UiId::NavHud23, L"前方下梯");
    case Cue::Special:return UiText(UiId::NavHud24, L"留意前方特殊路段");case Cue::Arrived:return UiText(UiId::NavHud25, L"已到目标区域");
    default:return UiText(UiId::NavHud26, L"沿地面箭头前进");}}
inline bool Routed(const guidance::Navigator& n){return n.state==guidance::State::Route||n.state==guidance::State::AtGoal;}
struct Mark {
    Vec anchor[3]{},points[3]{},eye{};unsigned checkedAt=0,groundAt=0;
    bool groundChecked=false,grounded=false,checked=false,visible=false;
};
inline bool SameMark(const Mark& a,const Mark& b){for(int i=0;i<3;++i){Vec d=a.anchor[i]-b.anchor[i];if(d.Dot(d)>.01f)return false;}return true;}
struct Path {
    Mark marks[MaxMarks]{};unsigned count=0;
    Vec target{},next{},cuePoint{};bool hasTarget=false,hasNext=false,hasCue=false;Cue cue=Cue::Forward;
    void Segment(const nav::Area& area,Vec from,Vec to,Vec player){
        Vec d=to-from;float len=Sqrt(d.x*d.x+d.y*d.y);if(!Finite(len)||len<24||count>=MaxMarks)return;
        Vec axis{d.x/len,d.y/len,0},side{-axis.y,axis.x,0};
        // Anchor samples to the end of this NAV segment, never to the camera.
        float first=len-38;if(first>Spacing)first-=int(first/Spacing)*Spacing;
        if(first<20)first+=Spacing;
        for(float t=first;t<len-14&&count<MaxMarks;t+=Spacing){
            Vec center=from+axis*t,distance=center-player;
            if(distance.Dot(distance)>Reach*Reach||distance.Dot(distance)<72*72)continue;
            Mark m;m.anchor[0]=center+axis*14;m.anchor[1]=center-axis*12+side*12;m.anchor[2]=center-axis*12-side*12;
            bool inside=true;for(Vec& p:m.anchor){if(p.x<area.x1+1||p.x>area.x2-1||p.y<area.y1+1||p.y>area.y2-1)inside=false;p.z=nav::Height(area,p.x,p.y);}
            if(!inside)continue;
            if(count){Vec gap=m.anchor[0]-marks[count-1].anchor[0];if(gap.Dot(gap)<54*54)continue;}
            marks[count++]=m;
        }
    }
    void Build(const nav::Mesh& mesh,const guidance::Navigator& n,Vec player){
        count=0;hasTarget=hasNext=hasCue=false;cue=Cue::Forward;
        if(n.selected<0||unsigned(n.selected)>=n.goalCount)return;
        target=n.goals[n.selected].position;hasTarget=Valid(target);
        if(!Routed(n)||!n.routeCount||!n.nodes)return;
        if(n.state==guidance::State::AtGoal){cue=Cue::Arrived;hasCue=true;cuePoint=target;return;}
        Vec entry{},previous{};float walked=0;bool entered=false,previousValid=false;
        for(unsigned i=0;i<n.routeCount&&i<guidance::MaxRoute&&walked<2200;++i){
            unsigned index=n.route[i];if(index>=mesh.count)break;const auto& area=mesh.areas[index];
            if(!entered)entry=guidance::Center(area);
            Vec exit=guidance::Center(area);bool stop=false;Cue special=Cue::Forward;
            if(i+1<n.routeCount){
                unsigned nextIndex=n.route[i+1];if(nextIndex>=mesh.count)break;
                unsigned edge=index<n.capacity?n.nodes[index].edge:nav::Invalid;
                const guidance::BackLink* link=edge<n.linkCount?&n.links[edge]:nullptr;
                if(link&&link->ladder<mesh.ladderCount){exit=link->down?mesh.ladders[link->ladder].top:mesh.ladders[link->ladder].bottom;special=link->down?Cue::LadderDown:Cue::LadderUp;stop=true;}
                else if(link&&link->unusual){special=Cue::Special;stop=true;}
                else {exit=guidance::Portal(area,mesh.areas[nextIndex]);exit.z=nav::Height(area,exit.x,exit.y);}
            }
            Vec d=exit-entry;float len=Length(d);if(!Finite(len)||len>100000)break;
            Vec horizontal{d.x,d.y,0};float flat=Length(horizontal);
            if(!hasNext){Vec delta=exit-player;if(delta.Dot(delta)>60*60){next=exit;hasNext=true;}}
            if(flat>24){
                Vec dir=horizontal*(1.f/flat);
                if(!hasCue&&walked>60&&walked<850&&previousValid&&previous.Dot(dir)<.64f){cue=Cross(previous,dir).z>0?Cue::Left:Cue::Right;cuePoint=entry;hasCue=true;}
                if(!hasCue&&walked<650&&nav::Abs(d.z)>48){cue=d.z>0?Cue::Up:Cue::Down;cuePoint=exit;hasCue=true;}
                previous=dir;previousValid=true;
            }
            Segment(area,entry,exit,player);walked+=len;
            if(stop){if(!hasCue&&walked<850){cue=special;cuePoint=exit;hasCue=true;}break;}
            if(i+1<n.routeCount){entry=exit;entry.z=nav::Height(mesh.areas[n.route[i+1]],entry.x,entry.y);entered=true;}
        }
    }
};
// A callback makes floor and visibility checks testable without a running game.
using Cast=bool(*)(void*,Vec,Vec,Trace&);
inline bool ClearSight(Cast cast,void* context,Vec eye,Vec end){Trace t{};return cast&&cast(context,eye,end,t)&&!t.startSolid&&!t.allSolid&&Finite(t.fraction)&&t.fraction>=.999f;}
inline bool Ground(Mark& m,Cast cast,void* context){
    for(int i=0;i<3;++i){Vec a=m.anchor[i]+Vec{0,0,32},b=m.anchor[i]-Vec{0,0,56};Trace t{};
        if(!cast||!cast(context,a,b,t)||t.startSolid||t.allSolid||!Finite(t.fraction)||t.fraction<0||t.fraction>=1||!Valid(t.end)||!Finite(t.normal.z)||t.normal.z<.55f)return false;
        m.points[i]=t.end+Vec{0,0,3};}
    // Reject arrows straddling a tall step or hanging off an edge.
    return nav::Abs(m.points[0].z-m.points[1].z)<20&&nav::Abs(m.points[0].z-m.points[2].z)<20;
}
struct Overlay {
    Path path;unsigned serial=~0u,meshSerial=~0u,lastBuild=0,cursor=0,floorCursor=0,traceCalls=0,visibleCount=0;Vec player{};
    Vec targetEye{};unsigned targetAt=0;bool targetChecked=false,targetVisible=false;
    void Clear(){path={};serial=meshSerial=~0u;lastBuild=cursor=floorCursor=visibleCount=0;targetChecked=targetVisible=false;}
    void Prepare(const nav::Mesh& mesh,const guidance::Navigator& n,Vec position,unsigned meshID,unsigned now){
        Vec moved=position-player;if(serial==n.serial&&meshSerial==meshID&&moved.Dot(moved)<48*48)return;
        Path fresh;fresh.Build(mesh,n,position);
        // Keep floor/visibility records of unchanged world-space anchors.
        if(meshSerial==meshID)for(unsigned i=0;i<fresh.count;++i)for(unsigned j=0;j<path.count;++j)if(SameMark(fresh.marks[i],path.marks[j])){fresh.marks[i]=path.marks[j];break;}
        Vec targetMoved=fresh.target-path.target;if(meshSerial!=meshID||targetMoved.Dot(targetMoved)>.01f||!Routed(n))targetChecked=targetVisible=false;
        path=fresh;player=position;serial=n.serial;meshSerial=meshID;lastBuild=now;
    }
    bool Fresh(const Mark& m,Vec eye,unsigned now)const{Vec d=eye-m.eye;return m.checked&&now-m.checkedAt<=240&&d.Dot(d)<64*64;}
    // Separate cursors reserve one floor and three visibility checks per frame.
    // Floor-cache refreshes cannot displace a complete visibility sweep.
    unsigned CheckGround(const Camera& camera,unsigned now,Cast cast,void* context){
        for(unsigned pass=0;pass<path.count;++pass){auto& m=path.marks[floorCursor++%path.count];Point p;
            if(!camera.Project(m.anchor[0],p))continue;
            if(!m.groundChecked||now-m.groundAt>1000){Vec old[3]={m.points[0],m.points[1],m.points[2]};bool was=m.grounded;
                m.grounded=Ground(m,cast,context);m.groundChecked=true;m.groundAt=now;
                if(!m.grounded||!was)m.checked=false;
                for(int i=0;i<3;++i){Vec d=old[i]-m.points[i];if(d.Dot(d)>1)m.checked=false;}
                traceCalls+=3;return 3;}
        }
        return 0;
    }
    unsigned CheckOne(const Camera& camera,unsigned now,Cast cast,void* context){
        for(unsigned pass=0;pass<path.count;++pass){auto& m=path.marks[cursor++%path.count];Point p;
            if(!camera.Project(m.anchor[0],p))continue;
            Vec moved=camera.eye-m.eye;
            // Refresh before the draw-time expiry, so a bounded round-robin
            // sweep cannot briefly hide stationary arrows at 30 FPS.
            if(!m.grounded||(m.checked&&now-m.checkedAt<55&&moved.Dot(moved)<8*8))continue;
            m.visible=true;for(int i=0;i<3;++i)if(!ClearSight(cast,context,camera.eye,m.points[i]))m.visible=false;
            traceCalls+=3;m.checked=true;m.checkedAt=now;m.eye=camera.eye;return 3;
        }
        return 0;
    }
};
}


