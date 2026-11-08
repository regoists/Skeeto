#pragma once
#include "nav.h"
#include "guidance.h"
#include "items.h"
namespace minimap {
constexpr int Pixels=320;
struct Point {float x,y;};
inline void SinCos(float degrees,float& s,float& c){
    // Bounded polynomial; avoids an external CRT dependency.
    float x=degrees*0.0174532925199433f;
    while(x>3.141592654f)x-=6.283185307f;while(x< -3.141592654f)x+=6.283185307f;
    float x2=x*x;
    s=x*(1+x2*(-1.f/6+x2*(1.f/120+x2*(-1.f/5040+x2*(1.f/362880+x2*(-1.f/39916800))))));
    c=1+x2*(-1.f/2+x2*(1.f/24+x2*(-1.f/720+x2*(1.f/40320+x2*(-1.f/3628800+x2/479001600)))));
}
struct View {
    Vec player{};float yaw=0,range=1200,ground=0,band=96;int layer=0,style=0,shape=0;bool rotate=true,layering=true;
    float s=0,c=1,scale=1,markerScale=1;
    void Prepare(){SinCos(yaw,s,c);scale=(Pixels-16)/(2*range);}
    Point Project(float x,float y)const{float dx=x-player.x,dy=y-player.y;
        return rotate?Point{Pixels*.5f+(dx*s-dy*c)*scale,Pixels*.5f-(dx*c+dy*s)*scale}:Point{Pixels*.5f+dx*scale,Pixels*.5f-dy*scale};}
    Vec World(float x,float y)const{float dx=(x-Pixels*.5f)/scale,dy=-(y-Pixels*.5f)/scale;
        return rotate?Vec{player.x+dx*s+dy*c,player.y-dx*c+dy*s,0}:Vec{player.x+dx,player.y+dy,0};}
    float LayerZ()const{return ground+layer*192.f;}
};
inline unsigned Color(unsigned r,unsigned g,unsigned b,unsigned a=255){return r|(g<<8)|(b<<16)|(a<<24);}
struct Scene {
    View view;Vec teammates[64]{};int count=0;unsigned meshSerial=0,overlaySerial=0,routeSerial=0;
    static bool Moved(Vec a,Vec b,float tolerance){return nav::Abs(a.x-b.x)>tolerance||nav::Abs(a.y-b.y)>tolerance||nav::Abs(a.z-b.z)>2;}
    bool TerrainMatches(const Scene& other)const {
        const auto& a=view;const auto& b=other.view;
        float delta=nav::Abs(a.yaw-b.yaw);if(delta>180)delta=nav::Abs(delta-360);
        return meshSerial==other.meshSerial&&a.style==b.style&&a.shape==b.shape&&a.rotate==b.rotate&&(!a.rotate||delta<=.25f)&&
            a.markerScale==b.markerScale&&a.range==b.range&&a.band==b.band&&a.layer==b.layer&&a.layering==b.layering&&
            nav::Abs(a.ground-b.ground)<=2&&!Moved(a.player,b.player,.35f/a.scale);
    }
    bool Matches(const Scene& other)const {
        const auto& a=view;const auto& b=other.view;
        float delta=nav::Abs(a.yaw-b.yaw);if(delta>180)delta=nav::Abs(delta-360);
        if(meshSerial!=other.meshSerial||overlaySerial!=other.overlaySerial||routeSerial!=other.routeSerial||count!=other.count||a.style!=b.style||a.shape!=b.shape||delta>.25f||
           a.markerScale!=b.markerScale||a.range!=b.range||a.band!=b.band||a.layer!=b.layer||a.rotate!=b.rotate||a.layering!=b.layering||
           nav::Abs(a.ground-b.ground)>2||Moved(a.player,b.player,.35f/a.scale))return false;
        for(int i=0;i<count;++i)if(Moved(teammates[i],other.teammates[i],.35f/a.scale))return false;
        return true;
    }
};
// The panel is the map disc only; no text rows sit above or below it.
struct Layout {
    int x,y,size,font,margin;
    static Layout Make(int width,int height,int requested,int posX,int posY){
        Layout l{};l.font=Limit(height*14/1080,12,26);l.margin=Limit(20*height/1080,12,64);
        int maximum=int(nav::Min(float(width-2*l.margin),float(height-2*l.margin)));
        l.size=Limit(requested*height/1080,160,Limit(maximum,160,16000));
        int travelX=width-2*l.margin-l.size,travelY=height-2*l.margin-l.size;
        if(travelX<0)travelX=0;if(travelY<0)travelY=0;
        l.x=l.margin+(Limit(posX,0,10000)*travelX+5000)/10000;
        l.y=l.margin+(Limit(posY,0,10000)*travelY+5000)/10000;return l;
    }
};
// Disc geometry in texture pixels around the texture centre.
constexpr float Center=Pixels*.5f,Rim=Pixels*.5f-1.5f,RingInner=Rim-5,TickOuter=RingInner-1;
constexpr float TargetRadius=TickOuter-22,TeamRadius=TickOuter-16;
inline float Root(float x){
    if(x<=0)return 0;union{float f;unsigned u;}bits{x};bits.u=(bits.u>>1)+0x1fbd1df5u;float r=bits.f;
    for(int i=0;i<3;++i)r=.5f*(r+x/r);return r;
}
// Shape 0 is the disc, shape 1 a rounded square with the same outer extent.
constexpr float SquareCorner=22;
// Clamp a texture point into the shape; `limit` is the disc radius or square half extent.
inline Point ClampShape(Point p,int shape,float limit,bool* clamped=nullptr){
    float dx=p.x-Center,dy=p.y-Center;
    if(!shape){float d=Root(dx*dx+dy*dy);bool outside=d>limit;
        if(clamped)*clamped=outside;return outside?Point{Center+dx*limit/d,Center+dy*limit/d}:p;}
    bool outside=false;
    if(dx>limit){dx=limit;outside=true;}else if(dx<-limit){dx=-limit;outside=true;}
    if(dy>limit){dy=limit;outside=true;}else if(dy<-limit){dy=-limit;outside=true;}
    const float corner=nav::Max(SquareCorner-(Rim-limit),6.f),inner=limit-corner;
    float ax=nav::Abs(dx)-inner,ay=nav::Abs(dy)-inner;
    if(ax>0&&ay>0){float d=Root(ax*ax+ay*ay);if(d>corner){float k=corner/d;ax*=k;ay*=k;dx=(dx<0?-1.f:1.f)*(inner+ax);dy=(dy<0?-1.f:1.f)*(inner+ay);outside=true;}}
    if(clamped)*clamped=outside;return outside?Point{Center+dx,Center+dy}:p;
}
inline Point TargetPoint(const View& v,const guidance::Navigator& n,bool* clamped=nullptr){
    Vec target=n.goals[n.selected].position;return ClampShape(v.Project(target.x,target.y),v.shape,TargetRadius,clamped);
}
inline Point TeamPoint(const View& v,Vec p,bool* clamped=nullptr){return ClampShape(v.Project(p.x,p.y),v.shape,TeamRadius,clamped);}
inline float NorthBadge(float markerScale){return float(Limit(int(7*markerScale+.5f),5,13));}
inline Point NorthPoint(const View& v){
    Point north=v.Project(v.player.x,v.player.y+v.range);float dx=north.x-Center,dy=north.y-Center,d=Root(dx*dx+dy*dy);
    float r=TickOuter-NorthBadge(v.markerScale)-3;if(d<=.001f)return {Center,Center-r};
    if(!v.shape)return {Center+dx*r/d,Center+dy*r/d};
    float m=nav::Max(nav::Abs(dx),nav::Abs(dy));return ClampShape({Center+dx*r/m,Center+dy*r/m},1,r);
}
// Outer coverage and the bright rim both span several texture pixels. A one-pixel
// colour step stays stair-stepped after the 320 map is scaled up.
constexpr float EdgeAa=5.f,RimFade=8.f;
// Built once per shape: coverage, soft rim weights and inner shading for the 320x320 texture.
struct DiscMask {
    enum : unsigned char {Outside,Map,Casing,Accent};
    unsigned char alpha[Pixels*Pixels],shade[Pixels*Pixels],frame[Pixels*Pixels],accent[Pixels*Pixels],kind[Pixels*Pixels];
    explicit DiscMask(int shape){
        auto depthAt=[&](float px,float py){float dx=px-Center,dy=py-Center;
            if(!shape)return Rim-Root(dx*dx+dy*dy);
            float ax=nav::Abs(dx)-(Rim-SquareCorner),ay=nav::Abs(dy)-(Rim-SquareCorner);
            float sdf=Root(nav::Max(ax,0.f)*nav::Max(ax,0.f)+nav::Max(ay,0.f)*nav::Max(ay,0.f))+nav::Min(nav::Max(ax,ay),0.f)-SquareCorner;
            return -sdf;};
        for(int y=0;y<Pixels;++y)for(int x=0;x<Pixels;++x){int at=y*Pixels+x;
            float centerDepth=depthAt(x+.5f,y+.5f),fade;
            if(!shape){float d=Rim-centerDepth;fade=(d-RingInner*.78f)/(RingInner*.22f);}
            else fade=(26-centerDepth)/26;
            float cover=0,rim=0,band=0;
            for(int sy=0;sy<2;++sy)for(int sx=0;sx<2;++sx){float depth=depthAt(x+.25f+sx*.5f,y+.25f+sy*.5f);
                float edge=(depth+EdgeAa*.5f)/EdgeAa;if(edge<0)edge=0;else if(edge>1)edge=1;cover+=edge;
                float t=1.f-depth/RimFade;if(t<0)t=0;else if(t>1)t=1;rim+=t*t*(3.f-2.f*t);
                float bump=1.f-nav::Abs(depth-3.f)/4.f;if(bump<0)bump=0;band+=bump*bump;}
            cover*=.25f;rim*=.25f;band*=.25f;
            alpha[at]=(unsigned char)(cover<=0?0:cover>=1?255:int(cover*255.f+.5f));
            frame[at]=(unsigned char)(rim<=0?0:rim>=1?255:int(rim*255.f+.5f));
            accent[at]=(unsigned char)(band<=0?0:int(band*200.f+.5f));
            kind[at]=!alpha[at]?Outside:!frame[at]&&!accent[at]?Map:Casing;
            shade[at]=(unsigned char)(fade<=0?255:255-int(nav::Min(fade,1.f)*58));
        }
    }
};
inline const DiscMask& Mask(int shape){static const DiscMask disc(0);static const DiscMask square(1);return shape?square:disc;}
inline const DiscMask& Disc(){return Mask(0);}
struct DiscPalette {unsigned casing,accent,major,minor,letter,badge;};
inline DiscPalette Palette(int style){
    return style?DiscPalette{Color(6,14,9),Color(224,196,131),Color(240,214,154),Color(140,122,86),Color(240,214,154),Color(10,20,13)}
                :DiscPalette{Color(9,16,21),Color(154,196,206),Color(186,224,232),Color(98,118,130),Color(226,240,238),Color(13,22,30)};
}
struct Raster {
    unsigned rgba[Pixels*Pixels]{};
    unsigned terrain[Pixels*Pixels]{};unsigned terrainBuilds=0;
    unsigned char coverage[Pixels*Pixels]{};
    unsigned char heightColor[Pixels*Pixels]{};
    // Render-only topology: no coverage, height, or NAV connection is changed.
    unsigned char detail[Pixels*Pixels]{};
    unsigned flood[Pixels*Pixels]{};
    unsigned softenedHoles=0,softenedIslands=0;
    unsigned activeAreas=0,pixelWork=0;bool budgetExceeded=false;
    void Dot(int x,int y,unsigned color,int radius=1){for(int j=-radius;j<=radius;++j)for(int i=-radius;i<=radius;++i)if(i*i+j*j<=radius*radius+1){int xx=x+i,yy=y+j;if(xx>=3&&yy>=3&&xx<Pixels-3&&yy<Pixels-3)rgba[yy*Pixels+xx]=color;}}
    // Clip before stepping so remote teammates and route points cannot cause huge loops.
    bool Clip(Point& a,Point& b){
        float dx=b.x-a.x,dy=b.y-a.y,t0=0,t1=1;
        const float p[]={-dx,dx,-dy,dy},q[]={a.x-3,Pixels-4-a.x,a.y-3,Pixels-4-a.y};
        for(int i=0;i<4;++i){if(nav::Abs(p[i])<.00001f){if(q[i]<0)return false;continue;}float t=q[i]/p[i];if(p[i]<0){if(t>t1)return false;t0=nav::Max(t0,t);}else{if(t<t0)return false;t1=nav::Min(t1,t);}}
        Point origin=a;a={origin.x+dx*t0,origin.y+dy*t0};b={origin.x+dx*t1,origin.y+dy*t1};return true;
    }
    void Line(Point a,Point b,unsigned color,int radius=0){if(!Clip(a,b))return;float dx=b.x-a.x,dy=b.y-a.y;int steps=int(nav::Max(nav::Abs(dx),nav::Abs(dy)))+1;for(int i=0;i<=steps;++i){float t=float(i)/steps;Dot(int(a.x+dx*t+.5f),int(a.y+dy*t+.5f),color,radius);}}
    void Blend(int x,int y,unsigned color,unsigned alpha){
        if(x<3||y<3||x>=Pixels-3||y>=Pixels-3)return;auto& dest=rgba[y*Pixels+x];unsigned inv=255-alpha;
        dest=Color(((dest&255)*inv+(color&255)*alpha)/255,(((dest>>8)&255)*inv+((color>>8)&255)*alpha)/255,(((dest>>16)&255)*inv+((color>>16)&255)*alpha)/255);
    }
    static float Side(Point a,Point b,Point p){return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);}
    void Triangle(Point a,Point b,Point c,unsigned color,unsigned alpha=255){
        int x0=Limit(int(nav::Min(a.x,nav::Min(b.x,c.x)))-1,3,Pixels-4),x1=Limit(int(nav::Max(a.x,nav::Max(b.x,c.x)))+1,3,Pixels-4);
        int y0=Limit(int(nav::Min(a.y,nav::Min(b.y,c.y)))-1,3,Pixels-4),y1=Limit(int(nav::Max(a.y,nav::Max(b.y,c.y)))+1,3,Pixels-4);
        float sign=Side(a,b,c)>=0?1.f:-1.f;
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){unsigned covered=0;for(int j=0;j<2;++j)for(int i=0;i<2;++i){Point p{float(x)+.25f+i*.5f,float(y)+.25f+j*.5f};if(Side(a,b,p)*sign>=0&&Side(b,c,p)*sign>=0&&Side(c,a,p)*sign>=0)++covered;}if(covered)Blend(x,y,color,alpha*covered/4);}
    }
    void Cover(int at,unsigned char level,unsigned char tint){
        if(level>coverage[at]||(level==coverage[at]&&nav::Abs(float(tint)-128)<nav::Abs(float(heightColor[at])-128))){coverage[at]=level;heightColor[at]=tint;}
    }
    void Area(const nav::Area& a,const View& v){
        if(budgetExceeded)return;
        float reach=v.range*1.5f;
        if(a.x2<v.player.x-reach||a.x1>v.player.x+reach||a.y2<v.player.y-reach||a.y1>v.player.y+reach)return;
        float lo=nav::Min(nav::Min(a.z1,a.z2),nav::Min(a.zNE,a.zSW)),hi=nav::Max(nav::Max(a.z1,a.z2),nav::Max(a.zNE,a.zSW));
        if(v.layering&&(hi<v.LayerZ()-v.band-500||lo>v.LayerZ()+v.band+500))return;
        if(a.x2-a.x1<=.01f||a.y2-a.y1<=.01f)return;
        const bool flat=hi-lo<.01f;float dz=a.z1-v.LayerZ();
        unsigned char flatLevel=(!v.layering||nav::Abs(dz)<=v.band)?2:1;
        unsigned char flatTint=(unsigned char)Limit(int(128+dz*.18f),1,255);
        const float invX=1/(a.x2-a.x1),invY=1/(a.y2-a.y1);
        const float du=(v.rotate?v.s:1)*invX/v.scale,dv=(v.rotate?-v.c:0)*invY/v.scale;
        Point p[]={v.Project(a.x1,a.y1),v.Project(a.x2,a.y1),v.Project(a.x2,a.y2),v.Project(a.x1,a.y2)};
        float lowY=p[0].y,highY=lowY;for(int i=1;i<4;++i){lowY=nav::Min(lowY,p[i].y);highY=nav::Max(highY,p[i].y);}
        int first=Limit(int(lowY),2,Pixels-3),last=Limit(int(highY)+1,2,Pixels-3);bool counted=false;
        for(int y=first;y<=last;++y){float scan=y+.5f,left=1e20f,right=-1e20f;int hits=0;
            for(int edge=0;edge<4;++edge){Point u=p[edge],w=p[(edge+1)%4];if((u.y<=scan&&w.y>scan)||(w.y<=scan&&u.y>scan)){float x=u.x+(scan-u.y)*(w.x-u.x)/(w.y-u.y);left=nav::Min(left,x);right=nav::Max(right,x);++hits;}}
            if(hits<2||right<2||left>=Pixels-2)continue;
            int x0=Limit(int(left+.5f),2,Pixels-3),x1=Limit(int(right),2,Pixels-3);
            pixelWork+=unsigned(nav::Max(0.f,float(x1-x0+1)));
            if(pixelWork>2000000){budgetExceeded=true;return;}
            if(flat){for(int x=x0;x<=x1;++x)Cover(y*Pixels+x,flatLevel,flatTint);if(flatLevel==2&&x1>=x0&&!counted){counted=true;++activeAreas;}continue;}
            Vec world=v.World(x0+.5f,y+.5f);float u=(world.x-a.x1)*invX,w=(world.y-a.y1)*invY;
            for(int x=x0;x<=x1;++x,u+=du,w+=dv){float uu=Limit(u,0.f,1.f),ww=Limit(w,0.f,1.f);
                float z=(a.z1+(a.zNE-a.z1)*uu)*(1-ww)+(a.zSW+(a.z2-a.zSW)*uu)*ww,dz=z-v.LayerZ();
                unsigned char level=(!v.layering||nav::Abs(dz)<=v.band)?2:1;
                Cover(y*Pixels+x,level,(unsigned char)Limit(int(128+dz*.18f),1,255));
                if(level==2&&!counted){counted=true;++activeAreas;}
            }
        }
    }
    static bool ClipHeight(Vec& a,Vec& b,float z,float band){
        float dz=b.z-a.z;if(nav::Abs(dz)<.001f)return nav::Abs(a.z-z)<=band;
        float lo=(z-band-a.z)/dz,hi=(z+band-a.z)/dz;if(lo>hi){float swap=lo;lo=hi;hi=swap;}
        lo=nav::Max(lo,0.f);hi=nav::Min(hi,1.f);if(lo>hi)return false;
        Vec start=a,delta=b-a;a=start+delta*lo;b=start+delta*hi;return true;
    }
    void ClassifyDetail(const View& v){
        softenedHoles=softenedIslands=0;for(auto& d:detail)d=0;
        const int holeSpan=Limit(int(20*v.markerScale+.5f),12,40),islandSpan=Limit(int(10*v.markerScale+.5f),6,20);
        const unsigned holeArea=unsigned(220*v.markerScale*v.markerScale),islandArea=unsigned(36*v.markerScale*v.markerScale);
        for(int y=2;y<Pixels-2;++y)for(int x=2;x<Pixels-2;++x){unsigned start=y*Pixels+x;if(detail[start])continue;
            bool floor=coverage[start]==2,border=false,player=false;unsigned head=0,tail=1;flood[0]=start;detail[start]=1;
            int minX=x,maxX=x,minY=y,maxY=y;
            while(head<tail){unsigned at=flood[head++];int xx=int(at)%Pixels,yy=int(at)/Pixels;
                if(xx<minX)minX=xx;if(xx>maxX)maxX=xx;if(yy<minY)minY=yy;if(yy>maxY)maxY=yy;
                if(xx==2||yy==2||xx==Pixels-3||yy==Pixels-3)border=true;
                if(xx>=Pixels/2-16&&xx<=Pixels/2+16&&yy>=Pixels/2-16&&yy<=Pixels/2+16)player=true;
                auto add=[&](int nx,int ny){if(nx<2||ny<2||nx>=Pixels-2||ny>=Pixels-2)return;unsigned next=ny*Pixels+nx;
                    if(!detail[next]&&(coverage[next]==2)==floor){detail[next]=1;flood[tail++]=next;}};
                add(xx-1,yy);add(xx+1,yy);add(xx,yy-1);add(xx,yy+1);
            }
            unsigned char mode=1;
            // Closed small voids remain obstacles, but lose their bright cutout border.
            // Open seams, long walls, cliffs, and large courtyards keep full contrast.
            if(!floor&&!border&&tail<=holeArea&&maxX-minX<holeSpan&&maxY-minY<holeSpan){mode=2;++softenedHoles;}
            if(floor&&!border&&!player&&tail<=islandArea&&maxX-minX<islandSpan&&maxY-minY<islandSpan){mode=3;++softenedIslands;}
            if(mode!=1)for(unsigned i=0;i<tail;++i)detail[flood[i]]=mode;
        }
    }
    void BuildTerrain(const nav::Mesh& mesh,const View& v){
        ++terrainBuilds;
        const bool tactical=v.style!=0;
        activeAreas=pixelWork=0;budgetExceeded=false;for(int i=0;i<Pixels*Pixels;++i){coverage[i]=0;heightColor[i]=0;rgba[i]=tactical?Color(10,16,18):Color(17,26,32);}
        for(unsigned i=0;i<mesh.count&&!budgetExceeded;++i)Area(mesh.areas[i],v);
        ClassifyDetail(v);
        for(int y=2;y<Pixels-2;++y)for(int x=2;x<Pixels-2;++x){int at=y*Pixels+x;unsigned char level=coverage[at];
            if(level==2){bool edge=coverage[at-1]!=2||coverage[at+1]!=2||coverage[at-Pixels]!=2||coverage[at+Pixels]!=2;
                bool major=(coverage[at-1]!=2&&detail[at-1]!=2)||(coverage[at+1]!=2&&detail[at+1]!=2)||(coverage[at-Pixels]!=2&&detail[at-Pixels]!=2)||(coverage[at+Pixels]!=2&&detail[at+Pixels]!=2);
                unsigned h=heightColor[at];
                rgba[at]=tactical
                    ? detail[at]==3?(edge?Color(83,119,91):Color(38,64,48)):edge?(major?Color(184,204,149):Color(90,132,94)):Color(29+h/85,57+h/95,49+h/100)
                    : detail[at]==3?(edge?Color(65,84,97):Color(53,71,84)):edge?(major?Color(142,165,177):Color(88,108,121)):Color(74+h/43,94+h/43,108+h/43);}
            else if(level==1){bool edge=!coverage[at-1]||!coverage[at+1]||!coverage[at-Pixels]||!coverage[at+Pixels];rgba[at]=tactical?(edge?Color(57,77,58):Color(27,42,31)):(edge?Color(52,69,81):Color(32,45,56));}
            else if(detail[at]==2)rgba[at]=tactical?Color(43,58,42):Color(49,65,75);
            else if(coverage[at-1]==2||coverage[at+1]==2||coverage[at-Pixels]==2||coverage[at+Pixels]==2)rgba[at]=tactical?Color(5,12,14):Color(8,17,22);
        }
        for(int i=0;i<Pixels*Pixels;++i)terrain[i]=rgba[i];
    }
    void Compose(const nav::Mesh& mesh,const View& v,const Vec* teammates=nullptr,int count=0,const guidance::Navigator* navigator=nullptr){
        for(int i=0;i<Pixels*Pixels;++i)rgba[i]=terrain[i];
        if(navigator)Navigation(mesh,v,*navigator);
        for(int i=0;i<count;++i)Teammate(v,teammates[i]);
        // Heading marker is always drawn, even when NAV is missing.
        float kMarker=v.markerScale;int casing=Limit(int(2*kMarker+.5f),1,4);
        float dx=v.rotate?0:v.c,dy=v.rotate?-1:-v.s;Point center{Pixels*.5f,Pixels*.5f};
        Point tip{center.x+dx*12*kMarker,center.y+dy*12*kMarker},left{center.x-dx*7*kMarker-dy*7*kMarker,center.y-dy*7*kMarker+dx*7*kMarker},right{center.x-dx*7*kMarker+dy*7*kMarker,center.y-dy*7*kMarker-dx*7*kMarker};
        Triangle(center,{center.x+dx*39*kMarker-dy*21*kMarker,center.y+dy*39*kMarker+dx*21*kMarker},{center.x+dx*39*kMarker+dy*21*kMarker,center.y+dy*39*kMarker-dx*21*kMarker},Color(244,242,220),18);
        Line(tip,left,Color(8,17,22),casing);Line(left,right,Color(8,17,22),casing);Line(right,tip,Color(8,17,22),casing);
        Triangle(tip,left,right,Color(244,242,220));Triangle(center,left,right,Color(203,207,192));
        // One pass over the existing texture: disc coverage, rim and inner shading.
        const DiscMask& mask=Mask(v.shape);const DiscPalette palette=Palette(v.style);
        auto mix=[&](unsigned dst,unsigned src,unsigned w){unsigned inv=255-w;
            return Color(((dst&255)*inv+(src&255)*w)/255,(((dst>>8)&255)*inv+((src>>8)&255)*w)/255,(((dst>>16)&255)*inv+((src>>16)&255)*w)/255);};
        for(int i=0;i<Pixels*Pixels;++i){if(!mask.alpha[i]){rgba[i]=0;continue;}
            unsigned c=rgba[i],s=mask.shade[i];c=Color((c&255)*s/255,((c>>8)&255)*s/255,((c>>16)&255)*s/255);
            if(mask.frame[i])c=mix(c,palette.casing,mask.frame[i]);
            if(mask.accent[i])c=mix(c,palette.accent,mask.accent[i]);
            rgba[i]=(c&0xffffffu)|(unsigned(mask.alpha[i])<<24);}
        Point north=NorthPoint(v);
        if(!v.shape){
            // Twelve bearings; cardinals are longer and north carries its own badge.
            float nx=(north.x-Center)/(TickOuter-NorthBadge(kMarker)-3),ny=(north.y-Center)/(TickOuter-NorthBadge(kMarker)-3);
            for(int i=1;i<12;++i){float s,c;SinCos(i*30.f,s,c);float tx=nx*c-ny*s,ty=nx*s+ny*c;
                float length=i%3?5.f:9.f;Line({Center+tx*(TickOuter-length),Center+ty*(TickOuter-length)},{Center+tx*TickOuter,Center+ty*TickOuter},i%3?palette.minor:palette.major);}
            if(v.style)for(int side=-1;side<=1;side+=2){float s,c;SinCos(side*9.f,s,c);float tx=nx*c-ny*s,ty=nx*s+ny*c;
                Line({Center+tx*(TickOuter-4),Center+ty*(TickOuter-4)},{Center+tx*TickOuter,Center+ty*TickOuter},palette.major);}
        }else{
            // Square: corner brackets frame the map; the tactical style adds edge-centre ticks.
            const float corner=TickOuter-9,arm=18;
            for(int sx=-1;sx<=1;sx+=2)for(int sy=-1;sy<=1;sy+=2){Point c{Center+sx*corner,Center+sy*corner};
                Line(c,{c.x-sx*arm,c.y},palette.major);Line(c,{c.x,c.y-sy*arm},palette.major);}
            if(v.style)for(int side=-1;side<=1;side+=2){
                Line({Center+side*TickOuter,Center-5},{Center+side*(TickOuter-6),Center-5},palette.minor);Line({Center+side*TickOuter,Center+5},{Center+side*(TickOuter-6),Center+5},palette.minor);
                Line({Center-5,Center+side*TickOuter},{Center-5,Center+side*(TickOuter-6)},palette.minor);Line({Center+5,Center+side*TickOuter},{Center+5,Center+side*(TickOuter-6)},palette.minor);}
        }
        int badge=int(NorthBadge(kMarker));Dot(int(north.x),int(north.y),palette.accent,badge+1);Dot(int(north.x),int(north.y),palette.badge,badge);
        float n=kMarker*.8f;
        Line({north.x-3*n,north.y+4*n},{north.x-3*n,north.y-4*n},palette.letter);Line({north.x-3*n,north.y-4*n},{north.x+3*n,north.y+4*n},palette.letter);Line({north.x+3*n,north.y+4*n},{north.x+3*n,north.y-4*n},palette.letter);
        // Ticks and the heading marker write solid pixels. Put the silhouette alpha back.
        for(int i=0;i<Pixels*Pixels;++i){unsigned a=mask.alpha[i];rgba[i]=a?(rgba[i]&0xffffffu)|(a<<24):0;}
    }
    void Build(const nav::Mesh& mesh,const View& v,const Vec* teammates=nullptr,int count=0,const guidance::Navigator* navigator=nullptr){
        BuildTerrain(mesh,v);Compose(mesh,v,teammates,count,navigator);
    }
    void RouteSegment(const View& v,Vec from,Vec to,unsigned color=Color(101,220,209),bool dashed=false){
        if(v.layering&&!ClipHeight(from,to,v.LayerZ(),v.band))return;
        Point a=v.Project(from.x,from.y),b=v.Project(to.x,to.y);if(!Clip(a,b))return;
        int stroke=Limit(int(v.markerScale+.5f),1,2);
        Line(a,b,Color(8,17,22),stroke+1);float dx=b.x-a.x,dy=b.y-a.y;int steps=int(nav::Max(nav::Abs(dx),nav::Abs(dy)))+1;
        for(int i=0;i<=steps;++i)if(!dashed||i%Limit(int(10*v.markerScale),6,20)<Limit(int(6*v.markerScale),4,12)){float t=float(i)/steps;Dot(int(a.x+dx*t+.5f),int(a.y+dy*t+.5f),color,stroke);}
    }
    void LadderMarker(const View& v,Vec at){
        if(v.layering&&nav::Abs(at.z-v.LayerZ())>v.band)return;Point p=v.Project(at.x,at.y);if(p.x<9||p.x>Pixels-10||p.y<9||p.y>Pixels-10)return;
        unsigned c=Color(130,201,255);Dot(int(p.x),int(p.y),Color(7,22,24),7);Line({p.x-3,p.y-5},{p.x-3,p.y+5},c);Line({p.x+3,p.y-5},{p.x+3,p.y+5},c);
        for(int y=-3;y<=3;y+=3)Line({p.x-3,p.y+y},{p.x+3,p.y+y},c);
    }
    void Navigation(const nav::Mesh& mesh,const View& v,const guidance::Navigator& n){
        if(n.selected<0||unsigned(n.selected)>=n.goalCount)return;
        for(unsigned i=1;(n.state==guidance::State::Route||n.state==guidance::State::AtGoal)&&i<n.routeCount;++i){unsigned from=n.route[i-1],to=n.route[i];if(from>=mesh.count||to>=mesh.count)continue;
            const auto& a=mesh.areas[from];const auto& b=mesh.areas[to];unsigned edge=n.nodes[from].edge;
            if(edge<n.linkCount){const auto& link=n.links[edge];
                if(link.ladder<mesh.ladderCount){const auto& ladder=mesh.ladders[link.ladder];Vec entry=link.down?ladder.top:ladder.bottom,exit=link.down?ladder.bottom:ladder.top;
                    RouteSegment(v,guidance::Center(a),entry);RouteSegment(v,entry,exit,Color(130,201,255));RouteSegment(v,exit,guidance::Center(b));LadderMarker(v,entry);LadderMarker(v,exit);continue;}
                if(link.unusual){RouteSegment(v,guidance::Center(a),guidance::Center(b),Color(242,188,112),true);continue;}}
            Vec portal=guidance::Portal(a,b);
            RouteSegment(v,guidance::Center(a),portal);portal.z=nav::Height(b,portal.x,portal.y);RouteSegment(v,portal,guidance::Center(b));
        }
        bool edge=false;Point at=TargetPoint(v,n,&edge);
        float dx=at.x-Center,dy=at.y-Center;
        float k=v.markerScale;auto point=[&](float x,float y){return Point{at.x+x*k,at.y+y*k};};
        unsigned color=(n.state==guidance::State::Route||n.state==guidance::State::AtGoal)?Color(101,220,209):n.state==guidance::State::Searching?Color(172,189,197):Color(242,188,112);
        Dot(int(at.x),int(at.y),Color(8,17,22),Limit(int(9*k),6,18));
        Line(point(-6,-1),point(0,-7),color,1);Line(point(0,-7),point(6,-1),color,1);
        Line(point(-4,0),point(-4,6),color);Line(point(-4,6),point(4,6),color);Line(point(4,6),point(4,0),color);
        if(edge){float scale=Root(dx*dx+dy*dy);dx/=scale;dy/=scale;Point tip=point(dx*13,dy*13);
            Line(tip,{tip.x+(-dx*4-dy*3)*k,tip.y+(-dy*4+dx*3)*k},color);Line(tip,{tip.x+(-dx*4+dy*3)*k,tip.y+(-dy*4-dx*3)*k},color);}
    }
    void Teammate(const View& v,Vec p){bool edge=false;Point at=TeamPoint(v,p,&edge);float k=v.markerScale;
        if(edge){float dx=at.x-Center,dy=at.y-Center,len=Root(dx*dx+dy*dy);dx/=len;dy/=len;
            Point tip{at.x+dx*5*k,at.y+dy*5*k},left{at.x+(-dx*4-dy*4)*k,at.y+(-dy*4+dx*4)*k},right{at.x+(-dx*4+dy*4)*k,at.y+(-dy*4-dx*4)*k};
            Line(tip,left,Color(8,17,22),2);Line(tip,right,Color(8,17,22),2);Triangle(tip,left,right,Color(137,183,255));return;}
        Dot(int(at.x),int(at.y),Color(8,17,22),Limit(int(5*k+.5f),3,10));Dot(int(at.x),int(at.y),Color(137,183,255),Limit(int(3*k+.5f),2,6));
        float dz=p.z-v.player.z;if(nav::Abs(dz)>100){float dir=dz>0?-1.f:1.f;Point a{at.x-3*k,at.y+dir*9*k},b{at.x,at.y+dir*12*k},c{at.x+3*k,at.y+dir*9*k};Line(a,b,Color(137,183,255));Line(b,c,Color(137,183,255));}
    }
};
}
