#pragma once
#include "skeeto_escui_layout.h"
// Pure editor geometry. <=128 selected/visible items, no engine calls or IO.
namespace skescgeo {
inline int Count(const skesc::Layout& l,const bool* selected){int n=0;for(int i=0;i<l.count;++i)n+=selected[i]?1:0;return n;}
inline void Move(skesc::Layout& l,const bool* selected,int dx,int dy){
    int left=-1920,right=1920,top=-1080,bottom=1080;
    for(int i=0;i<l.count;++i)if(selected[i]){const auto& b=l.items[i];
        left=std::max(left,(b.parent<0?0:skesc::kFlyoutLeft)-b.x);right=std::min(right,(b.parent<0?1920:skesc::kFlyoutRight)-b.x-b.w);
        top=std::max(top,(b.parent<0?0:skesc::kFlyoutTop)-b.y);bottom=std::min(bottom,(b.parent<0?1080:skesc::kFlyoutBottom)-b.y-b.h);}
    dx=std::max(left,std::min(right,dx));dy=std::max(top,std::min(bottom,dy));
    for(int i=0;i<l.count;++i)if(selected[i]){l.items[i].x+=dx;l.items[i].y+=dy;}
}
inline bool Arrange(skesc::Layout& l,const bool* selected,int primary,int operation){
    int ids[skesc::kMaxItems]{},n=0;for(int i=0;i<l.count;++i)if(selected[i])ids[n++]=i;
    if(n<(operation>=9?3:2))return false;
    const auto anchor=l.items[primary];static skesc::Layout work;work=l;
    for(int i=0;i<n;++i)if(l.items[ids[i]].parent!=anchor.parent)return false;
    if(operation>=9){bool horizontal=operation==9;
        std::sort(ids,ids+n,[&](int a,int b){int x=horizontal?l.items[a].x:l.items[a].y,y=horizontal?l.items[b].x:l.items[b].y;return x!=y?x<y:a<b;});
        const auto& first=l.items[ids[0]];const auto& last=l.items[ids[n-1]];
        int start=horizontal?first.x:first.y,finish=horizontal?last.x+last.w:last.y+last.h,total=0;
        for(int i=0;i<n;++i)total+=horizontal?l.items[ids[i]].w:l.items[ids[i]].h;
        if(finish-start<total)return false;
        double gap=double(finish-start-total)/(n-1),at=start;
        for(int i=0;i<n;++i){auto& b=work.items[ids[i]];if(horizontal)b.x=int(at+.5);else b.y=int(at+.5);at+=(horizontal?b.w:b.h)+gap;}
    }else{
        int width=anchor.w,height=anchor.h;
        for(int i=0;i<n;++i){width=std::max(width,skesc::MinimumWidth(l.items[ids[i]]));height=std::max(height,skesc::MinimumHeight(l,l.items[ids[i]]));}
        for(int i=0;i<n;++i){auto& b=work.items[ids[i]];
            switch(operation){case 0:b.x=anchor.x;break;case 1:b.x=anchor.x+(anchor.w-b.w)/2;break;case 2:b.x=anchor.x+anchor.w-b.w;break;
            case 3:b.y=anchor.y;break;case 4:b.y=anchor.y+(anchor.h-b.h)/2;break;case 5:b.y=anchor.y+anchor.h-b.h;break;
            case 6:b.w=width;break;case 7:b.h=height;break;case 8:b.w=width;b.h=height;break;}
        }
    }
    if(!skesc::Valid(work))return false;l=work;return true;
}
struct Snap {int dx=0,dy=0,x=-1,y=-1;};
inline Snap Align(const skesc::Layout& l,int primary,const bool* selected,const int* view,int count,bool resize){
    const auto& b=l.items[primary];Snap s{};int bestX=9,bestY=9;
    auto consider=[](int value,int target,int& best,int& delta,int& guide){int d=target-value;
        if(std::abs(d)<best){best=std::abs(d);delta=d;guide=target;}};
    const int bx[]={b.x,b.x+b.w/2,b.x+b.w},by[]={b.y,b.y+b.h/2,b.y+b.h};
    for(int n=-1;n<count;++n){int tx[3]{},ty[3]{};
        if(n<0){int left=b.parent<0?0:skesc::kFlyoutLeft,right=b.parent<0?1920:skesc::kFlyoutRight,top=b.parent<0?0:skesc::kFlyoutTop,bottom=b.parent<0?1080:skesc::kFlyoutBottom;
            tx[0]=left;tx[1]=(left+right)/2;tx[2]=right;ty[0]=top;ty[1]=(top+bottom)/2;ty[2]=bottom;
        }else{int i=view[n];const auto& other=l.items[i];if(selected[i]||other.parent!=b.parent)continue;
            tx[0]=other.x;tx[1]=other.x+other.w/2;tx[2]=other.x+other.w;ty[0]=other.y;ty[1]=other.y+other.h/2;ty[2]=other.y+other.h;}
        for(int a=resize?2:0;a<3;++a)for(int c=0;c<3;++c){consider(bx[a],tx[c],bestX,s.dx,s.x);consider(by[a],ty[c],bestY,s.dy,s.y);}
    }
    if(s.x<0)s.dx=((resize?b.w:b.x)+4)/8*8-(resize?b.w:b.x);
    if(s.y<0)s.dy=((resize?b.h:b.y)+4)/8*8-(resize?b.h:b.y);
    return s;
}
}
