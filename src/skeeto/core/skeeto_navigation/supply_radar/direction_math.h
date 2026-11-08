#pragma once
#include "radar_math.h"
#include "direction_surface.h"

namespace direction_math {
constexpr float UnitsPerMeter=39.3701f;
struct Result {float x=0,y=-1;bool horizontal=false;int height=0;};
// Camera heading is shared by every HUD row. Evaluate once per frame, rather
// than querying a camera interface or evaluating trigonometry for every item.
struct Basis {
    Vec forward{},right{};bool valid=false,hasHeading=false;
    void Reset(){*this={};}
    template<class MatrixT> bool Update(const MatrixT& view){
        valid=false;
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)if(!Finite(view.m[r][c]))return false;
        for(unsigned r=0;r<3;++r){float length=0;for(unsigned c=0;c<3;++c)length+=view.m[r][c]*view.m[r][c];if(radar_nav::Abs(length-1.f)>.01f)return false;
            for(unsigned s=r+1;s<3;++s){float dot=0;for(unsigned c=0;c<3;++c)dot+=view.m[r][c]*view.m[s][c];if(radar_nav::Abs(dot)>.01f)return false;}}
        if(radar_nav::Abs(view.m[3][0])>.0001f||radar_nav::Abs(view.m[3][1])>.0001f||radar_nav::Abs(view.m[3][2])>.0001f||radar_nav::Abs(view.m[3][3]-1.f)>.0001f)return false;
        // Rigid WorldToViewMatrix rows are right, up, and negative forward.
        Vec f{-view.m[2][0],-view.m[2][1],0};float flat=f.Dot(f);
        if(flat>=.0025f){
            f=f*(1.f/radar_nav::Sqrt(flat));
            Vec r{f.y,-f.x,0};
            // Project camera right onto the perpendicular horizontal axis.
            // This also retains the correct left/right sign for rolled views.
            float alignment=r.x*view.m[0][0]+r.y*view.m[0][1];
            if(radar_nav::Abs(alignment)<.001f)return false;
            if(alignment<0)r=r*-1.f;
            forward=f;right=r;hasHeading=true;
        }else if(!hasHeading)return false;
        // Near vertical pitch has no stable new horizontal forward. Retain the
        // last valid heading; an unavailable/invalid matrix never reaches here.
        valid=true;return true;
    }
};
struct State {
    unsigned key=0;bool keyed=false;int height=0;
    void Reset(){*this={};}
    Result Update(const Basis& basis,Vec player,Vec target,unsigned targetKey){
        if(!keyed||key!=targetKey){key=targetKey;keyed=true;height=0;}
        Result result;
        if(!radar_nav::Position(player)||!radar_nav::Position(target)){height=0;return result;}
        Vec delta=target-player;float dz=delta.z/UnitsPerMeter;
        if(dz>2.f)height=1;else if(dz<-2.f)height=-1;
        else if((height>0&&dz<=1.5f)||(height<0&&dz>=-1.5f))height=0;
        result.height=height;
        // Height is world Z difference and remains valid when the optional
        // horizontal bearing is disabled or the camera matrix is unavailable.
        if(!basis.valid)return result;
        float flat=delta.x*delta.x+delta.y*delta.y;
        float closeRange=.25f*UnitsPerMeter;
        if(flat<.0001f||(height&&flat<closeRange*closeRange))return result;
        float inverse=1.f/radar_nav::Sqrt(flat);
        result.x=(delta.x*basis.right.x+delta.y*basis.right.y)*inverse;
        result.y=-(delta.x*basis.forward.x+delta.y*basis.forward.y)*inverse;
        result.horizontal=Finite(result.x)&&Finite(result.y);return result;
    }
};
// Rotate an upward-facing square texture using its direction vector as the
// sine/cosine pair. UVs and winding stay attached to the source texture.
inline bool Quad(float centerX,float centerY,float halfSize,const Result& direction,SurfaceVertex (&vertices)[4]){
    if(!direction.horizontal||!Finite(direction.x)||!Finite(direction.y)||!Finite(centerX)||!Finite(centerY)||!Finite(halfSize)||halfSize<=0)return false;
    const float x[4]={-halfSize,halfSize,halfSize,-halfSize},y[4]={-halfSize,-halfSize,halfSize,halfSize};
    const float u[4]={0,1,1,0},v[4]={0,0,1,1};
    float cosine=-direction.y,sine=direction.x;
    for(unsigned i=0;i<4;++i)vertices[i]={centerX+cosine*x[i]-sine*y[i],centerY+sine*x[i]+cosine*y[i],u[i],v[i]};
    return true;
}
}
