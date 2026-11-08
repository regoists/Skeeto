#pragma once
#include "direction_math.h"

namespace rectangular_hud {
// Reserve this much on every side of the complete card, including its quantity
// and height text. A rotated square has extent <=sqrt(2)*half. Its center can
// extend by one such extent beyond the card, and its vertices by another.
// Result vectors from State::Update are unit length (within floating error).
inline int Margin(float half,float gap){
    if(!Finite(half)||!Finite(gap)||half<=0||gap<0)return 0;
    float needed=gap+2.82857f*half;
    if(needed>1000000)return 0;
    int whole=int(needed);return whole+(needed>float(whole));
}
// Put the arrow on the ray from the center of the complete card. Intersect the
// ray with a rectangle expanded by gap + the arrow's rotated square extent.
// Therefore the entire texture quad remains outside the card at every bearing.
// No angle/trigonometry, texture rewrite, world query, or screen clamping here.
// The caller places a complete cell with Margin padding so arrows stay on-screen.
template<class BoxT>
inline bool ArrowQuad(const BoxT& card,float gap,float half,const direction_math::Result& direction,SurfaceVertex (&vertices)[4]){
    float x=float(card.x),y=float(card.y),wide=float(card.w),tall=float(card.h);
    if(!direction.horizontal||!Finite(direction.x)||!Finite(direction.y)||
       !Finite(x)||!Finite(y)||!Finite(wide)||!Finite(tall)||wide<=0||tall<=0||
       !Finite(gap)||!Finite(half)||gap<0||half<=0)return false;
    float length=direction.x*direction.x+direction.y*direction.y;
    if(!Finite(length)||radar_nav::Abs(length-1.f)>.0001f)return false;
    float dx=radar_nav::Abs(direction.x),dy=radar_nav::Abs(direction.y);
    float extent=half*(dx+dy);
    float tx=dx>.00001f?(wide*.5f+gap+extent)/dx:1.e30f;
    float ty=dy>.00001f?(tall*.5f+gap+extent)/dy:1.e30f;
    float distance=tx<ty?tx:ty;
    return direction_math::Quad(x+wide*.5f+distance*direction.x,
                                y+tall*.5f+distance*direction.y,half,direction,vertices);
}
}
