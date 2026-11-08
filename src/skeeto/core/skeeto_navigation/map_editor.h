#pragma once
#include "render.h"

namespace minimap {
enum class MapEditAction { End, WaitForMenu, Run };

inline MapEditAction MapEditNext(bool requested,bool active,bool inGame,bool focused,bool menuVisible){
    if(!requested||!inGame||!focused)return MapEditAction::End;
    if(menuVisible)return active?MapEditAction::End:MapEditAction::WaitForMenu;
    return MapEditAction::Run;
}

// Entry may be triggered by Enter in the menu. Arm on release so that the
// activation key cannot also finish editing; later presses always exit.
struct MapEditExitKeys {
    bool enterHeld=false,escapeHeld=false,numberHeld=false,armed=false;
    void Begin(bool enterDown,bool escapeDown,bool numberDown=false){enterHeld=enterDown;escapeHeld=escapeDown;numberHeld=numberDown;armed=false;}
    bool Pressed(bool enterDown,bool escapeDown,bool numberDown=false){
        bool pressed=armed&&((enterDown&&!enterHeld)||(escapeDown&&!escapeHeld)||(numberDown&&!numberHeld));
        if(!enterDown&&!escapeDown&&!numberDown)armed=true;
        enterHeld=enterDown;escapeHeld=escapeDown;numberHeld=numberDown;return pressed;
    }
    bool Held(unsigned key)const{return !armed|| (key==13?enterHeld:key==27?escapeHeld:(key=='2'||key==98)?numberHeld:false);}
    void Release(unsigned key){if(key==13)enterHeld=false;else if(key==27)escapeHeld=false;else if(key=='2'||key==98)numberHeld=false;}
};

// Repeated movement has a fixed cadence independent of the OS keyboard-repeat setting.
struct MapEditRepeat {
    bool active=false;int dx=0,dy=0;unsigned start=0,last=0;
    bool Step(unsigned now,int x,int y){
        if(!x&&!y){active=false;return false;}
        if(!active||x!=dx||y!=dy){active=true;dx=x;dy=y;start=last=now;return true;}
        if(now-start<220||now-last<40)return false;
        last=now;return true;
    }
};

struct MapEditor {
    bool active=false,held[6]={};
    MapEditRepeat movement,zoom;
    static int KeyIndex(unsigned key){
        switch(key){case 37:return 0;case 39:return 1;case 38:return 2;
            case 40:return 3;case 'B':return 4;case 'S':return 5;default:return -1;}
    }
    void Begin(){*this={};active=true;}
    void End(){*this={};}
    bool Key(unsigned key,bool down){
        int i=KeyIndex(key);if(!active||i<0)return false;
        if(held[i]!=down){if(i<4)movement.active=false;else zoom.active=false;}
        held[i]=down;return true;
    }
    static bool Move(int width,int height,int size,int& posX,int& posY,int dx,int dy,int stepPx){
        Layout layout=Layout::Make(width,height,size,posX,posY);
        int travelX=width-2*layout.margin-layout.size;
        int travelY=height-2*layout.margin-layout.size;
        if(travelX<0)travelX=0;if(travelY<0)travelY=0;
        int x=layout.x-layout.margin,y=layout.y-layout.margin;
        int nextX=Limit(x+dx*stepPx,0,travelX),nextY=Limit(y+dy*stepPx,0,travelY);
        if(nextX==x&&nextY==y)return false;
        if(travelX&&nextX!=x)posX=(nextX*10000+travelX/2)/travelX;
        if(travelY&&nextY!=y)posY=(nextY*10000+travelY/2)/travelY;
        return true;
    }
    bool Step(unsigned now,int width,int height,int& size,int& posX,int& posY,bool fastStep){
        if(!active)return false;
        bool changed=false;
        int dx=int(held[1])-int(held[0]),dy=int(held[3])-int(held[2]);
        if(movement.Step(now,dx,dy))changed=Move(width,height,size,posX,posY,dx,dy,fastStep?8:2);
        int ds=int(held[4])-int(held[5]);
        if(zoom.Step(now,ds,0)){
            int next=Limit(size+ds*20,180,480);
            changed=changed||next!=size;size=next;
        }
        return changed;
    }
    bool StepPosition(unsigned now,int width,int height,int boxW,int boxH,int& posX,int& posY,bool fastStep){
        if(!active)return false;
        int dx=int(held[1])-int(held[0]),dy=int(held[3])-int(held[2]);
        if(!movement.Step(now,dx,dy))return false;
        constexpr int margin=8;
        int screenX=width-2*margin,travelY=height-2*margin-boxH;
        if(screenX<0)screenX=0;if(travelY<0)travelY=0;
        int center=margin+(screenX*posX+5000)/10000;
        int minCenter=margin+boxW/2,maxCenter=width-margin-boxW/2;
        if(maxCenter<minCenter)maxCenter=minCenter;
        center=Limit(center,minCenter,maxCenter);
        int x=center-boxW/2;
        int y=margin+(travelY*posY+5000)/10000;
        // Match the shared custom-HUD editor: 8 px normally, 32 px with Shift.
        const int stepPx=fastStep?32:8;
        int nextCenter=Limit(center+dx*stepPx,minCenter,maxCenter);
        int nextX=nextCenter-boxW/2;
        int nextY=Limit(y+dy*stepPx,margin,margin+travelY);
        if(nextX==x&&nextY==y)return false;
        if(screenX&&nextX!=x)posX=Limit(((nextCenter-margin)*10000+screenX/2)/screenX,0,10000);
        if(travelY&&nextY!=y)posY=Limit(((nextY-margin)*10000+travelY/2)/travelY,0,10000);
        return true;
    }
};
}
