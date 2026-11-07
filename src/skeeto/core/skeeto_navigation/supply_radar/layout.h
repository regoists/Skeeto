#pragma once
#include "settings.h"

// Per-item card placement and keyboard editing. Placement follows SupplyRadar
// 1.3.6 (coordinates relative to the nominal cell, so text growth never moves
// the saved origin). Editing is live: every key press is applied at once, and
// matching edges/centres of other enabled cards snap within 4 px (Ctrl disables).
namespace layout {
struct Item {int custom=0,x=0,y=0,scale=100;};
struct Box {int x=0,y=0,w=0,h=0;};
constexpr unsigned Count=options::LabelCount;
static Item items[Count];
// boxes: painted cell; automaticBoxes: nominal cell used as the coordinate base;
// panelBoxes: stable card plates used only for snapping while editing.
static Box boxes[Count],automaticBoxes[Count],visualBoxes[Count],panelBoxes[Count];
static bool active=false;static unsigned selected=1;static int width=1280,height=720;
// Keep requested pixels separate from snapped pixels so holding a direction
// can leave an alignment line. This state is used only by the editor.
static unsigned cursorItem;
static int cursorX,cursorY,cursorW,cursorH,cursorScreenW,cursorScreenH;
struct Guide {unsigned peer=0;int anchor=0,position=0;};
static Guide guideX,guideY;
inline void ClearGuides(){guideX={};guideY={};}
inline void ResetCursor(){cursorItem=0;ClearGuides();}
inline unsigned Next(unsigned from,bool backwards){
    for(unsigned k=1;k<Count;++k){unsigned i=backwards?(from+Count-k)%Count:(from+k)%Count;if(options::EnabledId(i))return i;}
    return from;
}
inline void Select(unsigned id){if(selected!=id){selected=id;ResetCursor();}}
inline void Begin(unsigned id=0){
    if(id&&!options::EnabledId(id))return;
    if(id)Select(id);
    if(!options::EnabledId(selected))Select(Next(0,false));
    if(!options::EnabledId(selected))return;
    if(!active){ResetCursor();for(auto& panel:panelBoxes)panel={};}
    active=true;
}
inline void End(){active=false;ResetCursor();}
inline Box Sized(Box b,int scale){float factor=float(scale)/100.f;b.w=Limit(int(float(b.w)*factor+.5f),16,width-8);b.h=Limit(int(float(b.h)*factor+.5f),8,height-8);return b;}
// b carries the coordinate-size box; only its position is produced.
inline void Decode(unsigned id,Box& b){auto& item=items[id];b.x=4+((width-b.w-8)*item.x+5000)/10000;b.y=4+((height-b.h-8)*item.y+5000)/10000;}
inline Box Place(unsigned id,Box automatic){
    if(id>=Count)return automatic;
    automaticBoxes[id]=automatic;
    auto& item=items[id];Box b=Sized(automatic,item.scale);
    if(item.custom)Decode(id,b);
    b.x=Limit(b.x,4,width-b.w-4);b.y=Limit(b.y,item.custom?4:2,height-b.h-4);boxes[id]=b;return b;
}
inline void Pin(unsigned id,int x,int y,int w,int h){
    auto& item=items[id];item.custom=1;auto coordinate=Sized(automaticBoxes[id],item.scale);
    int spanX=width-coordinate.w-8,spanY=height-coordinate.h-8;x=Limit(x,4,width-w-4);y=Limit(y,4,height-h-4);
    item.x=spanX>0?Limit(((x-4)*10000+spanX/2)/spanX,0,10000):0;item.y=spanY>0?Limit(((y-4)*10000+spanY/2)/spanY,0,10000):0;
}
inline void Include(Box& bounds,Box drawn){if(drawn.w<=0||drawn.h<=0)return;int right=bounds.x+bounds.w,bottom=bounds.y+bounds.h;if(drawn.x+drawn.w>right)right=drawn.x+drawn.w;if(drawn.y+drawn.h>bottom)bottom=drawn.y+drawn.h;if(drawn.x<bounds.x)bounds.x=drawn.x;if(drawn.y<bounds.y)bounds.y=drawn.y;bounds.w=right-bounds.x;bounds.h=bottom-bounds.y;}
inline int Anchor(Box b,bool horizontal,int anchor){int start=horizontal?b.x:b.y,size=horizontal?b.w:b.h;return start+(anchor==0?0:anchor==1?size/2:size);}
inline int Snap(int requested,Box b,bool horizontal,Guide& guide){
    const Box panel=panelBoxes[selected];if(panel.w<=0||panel.h<=0)return requested;
    int result=requested,best=5,base=horizontal?b.x:b.y,maxPosition=horizontal?width-b.w-4:height-b.h-4;
    // Stable plate bounds, not distance/count text whose width changes.
    // Compare matching edges and centres only, with a four-pixel tolerance.
    for(unsigned id=1;id<Count;++id){
        if(id==selected||!options::EnabledId(id)||panelBoxes[id].w<=0||panelBoxes[id].h<=0)continue;
        for(int anchor=0;anchor<3;++anchor){
            int line=Anchor(panelBoxes[id],horizontal,anchor),target=base+line-Anchor(panel,horizontal,anchor);
            if(target<4||target>maxPosition)continue;
            int distance=target-requested;if(distance<0)distance=-distance;
            if(distance<best){best=distance;result=target;guide={id,anchor,line};}
        }
    }
    return result;
}
inline bool GuideVisible(const Guide& guide,bool horizontal){
    return guide.peer&&options::EnabledId(guide.peer)&&panelBoxes[selected].w>0&&panelBoxes[guide.peer].w>0
        &&Anchor(panelBoxes[selected],horizontal,guide.anchor)==guide.position
        &&Anchor(panelBoxes[guide.peer],horizontal,guide.anchor)==guide.position;
}
// Return 1: handled, 2: close, 3: changed (already applied to items[]).
inline int Key(unsigned key,bool shift,bool freeMove=false){
    if(!active)return 0;
    if(!options::EnabledId(selected)){Select(Next(0,false));if(!options::EnabledId(selected)){End();return 0;}}
    if(key==13||key==27)return 2;
    if(key=='R'){Item old=items[selected];items[selected]={};ResetCursor();panelBoxes[selected]={};return old.custom||old.x||old.y||old.scale!=100?3:1;}
    auto& item=items[selected];Box b=boxes[selected];if(!b.w||!b.h)return 1;
    if(key>=37&&key<=40){
        if(cursorItem!=selected||cursorW!=b.w||cursorH!=b.h||cursorScreenW!=width||cursorScreenH!=height){
            cursorItem=selected;cursorX=b.x;cursorY=b.y;cursorW=b.w;cursorH=b.h;cursorScreenW=width;cursorScreenH=height;
        }
        ClearGuides();int step=shift?10:1,x=b.x,y=b.y;bool horizontal=key==37||key==39;
        if(freeMove){cursorX=b.x;cursorY=b.y;}
        if(horizontal){cursorX=Limit(cursorX+(key==37?-step:step),4,width-b.w-4);x=freeMove?cursorX:Snap(cursorX,b,true,guideX);}
        else {cursorY=Limit(cursorY+(key==38?-step:step),4,height-b.h-4);y=freeMove?cursorY:Snap(cursorY,b,false,guideY);}
        Item old=item;Pin(selected,x,y,b.w,b.h);
        // An axis that was already custom keeps its stored value: re-encoding the
        // other axis from the painted box would only add rounding drift.
        if(old.custom){if(horizontal)item.y=old.y;else item.x=old.x;}
        Box placed=Sized(automaticBoxes[selected],item.scale);Decode(selected,placed);
        int dx=placed.x-b.x,dy=placed.y-b.y;
        boxes[selected].x=placed.x;boxes[selected].y=placed.y;
        panelBoxes[selected].x+=dx;panelBoxes[selected].y+=dy;visualBoxes[selected].x+=dx;visualBoxes[selected].y+=dy;
        return old.custom!=item.custom||old.x!=item.x||old.y!=item.y?3:1;
    }
    if(key==187||key==107||key==189||key==109){
        int next=Limit(item.scale+((key==187||key==107)?5:-5),50,200);if(next==item.scale)return 1;
        item.scale=next;Box placed=Sized(automaticBoxes[selected],next);Pin(selected,b.x,b.y,placed.w,placed.h);Decode(selected,placed);boxes[selected]=placed;
        ResetCursor();panelBoxes[selected]={};return 3;
    }
    return 0;
}
}
