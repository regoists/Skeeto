#pragma once
#include "radar_abi.h"
namespace raster_filter {
// Exact pixel-area averaging in premultiplied alpha. Evaluate only when output
// size changes, then upload at 1:1: no point-sampled shrink or Lanczos ringing.
inline bool Resize(const unsigned char* source,int sw,int sh,unsigned char* target,int dw,int dh){
    if(!source||!target||sw<1||sh<1||sw>256||sh>64||dw<1||dh<1||dw>512||dh>256)return false;
    for(int y=0;y<dh;++y){int y0=y*sh,y1=(y+1)*sh;
        for(int x=0;x<dw;++x){int x0=x*sw,x1=(x+1)*sw;unsigned weight=0,alpha=0,r=0,g=0,b=0;
            for(int sy=y0/dh;sy<(y1+dh-1)/dh;++sy){unsigned wy=unsigned(Limit((sy+1)*dh,y0,y1)-Limit(sy*dh,y0,y1));
                for(int sx=x0/dw;sx<(x1+dw-1)/dw;++sx){unsigned wx=unsigned(Limit((sx+1)*dw,x0,x1)-Limit(sx*dw,x0,x1));unsigned area=wx*wy;auto p=source+(sy*sw+sx)*4;unsigned wa=area*p[3];weight+=area;alpha+=wa;r+=wa*p[0];g+=wa*p[1];b+=wa*p[2];}}
            auto out=target+(y*dw+x)*4;out[3]=(unsigned char)((alpha+weight/2)/weight);
            if(alpha){out[0]=(unsigned char)((r+alpha/2)/alpha);out[1]=(unsigned char)((g+alpha/2)/alpha);out[2]=(unsigned char)((b+alpha/2)/alpha);}else out[0]=out[1]=out[2]=0;
        }
    }return true;
}
}
