#pragma once
#include "radar_abi.h"
namespace rounded_panel {
// Mask the cached original panel once. 4x4 samples soften only the corner arc.
inline void Apply(unsigned char* pixels,int w,int h){
    if(!pixels||w<2||h<2)return;float radius=float(h)*.25f;
    if(radius>float(w)*.25f)radius=float(w)*.25f;
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){
        if((x>=radius&&x+1<=w-radius)||(y>=radius&&y+1<=h-radius))continue;
        unsigned hits=0;float cx=x<w/2?radius:w-radius,cy=y<h/2?radius:h-radius;
        for(unsigned sy=0;sy<4;++sy)for(unsigned sx=0;sx<4;++sx){float dx=x+(sx+.5f)*.25f-cx,dy=y+(sy+.5f)*.25f-cy;if(dx*dx+dy*dy<=radius*radius)++hits;}
        auto p=pixels+(y*w+x)*4;p[3]=(unsigned char)((unsigned(p[3])*hits+8)/16);
    }
}
}
