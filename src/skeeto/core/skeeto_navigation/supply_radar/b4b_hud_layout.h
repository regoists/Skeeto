#pragma once
#include "items.h"
#include "layout.h"
namespace b4b_hud {
// Keep the provided VPK's groups and bottom-to-top weapon order. Reserve six
// HUD units around each plate for the single rotating arrow. Missing weapons
// use the existing icon system and continue after the original weapon rows.
constexpr int SmallWidth=44,WeaponWidth=84,PlateHeight=16,Margin=6;
constexpr int SmallColumns=4,SpecialIconLane=17;
constexpr int SmallPitch=SmallWidth+2*Margin,WeaponPitch=WeaponWidth+2*Margin;
constexpr int RowPitch=PlateHeight+2*Margin,BottomPadding=99;
constexpr float ArrowHalf=1.8f,ArrowGap=.4f;
static constexpr unsigned weaponOrder[]={18,17,14,13,12,11,19,29,26,27,15,16,28,20,21,24,25};
struct Grid {int columns=2,rows=8;};
inline Grid Weapons(){
    Grid grid;grid.columns=Limit(options::values[options::Columns],1,3);
    grid.rows=(15+grid.columns-1)/grid.columns;
    int limit=Limit(options::values[options::Rows],3,16);
    if(grid.rows>limit){grid.columns=Limit((15+limit-1)/limit,1,3);grid.rows=(15+grid.columns-1)/grid.columns;}
    return grid;
}
inline float Scale(int width,int height){
    // Honor the menu percent directly. The old grid-fit clamp made Scale>100%
    // look identical to 100% on typical resolutions; DrawCards still pins each
    // card to the screen edges if the layout grows past the viewport.
    (void)width;
    float scale=float(options::values[options::Scale])/100.f*float(height)/480.f;
    if(scale<.35f)scale=.35f;
    if(scale>6.f)scale=6.f;
    return scale;
}
inline bool GridPosition(unsigned id,int& group,int& column,int& row){
    auto grid=Weapons();column=row=group=0;
    if(id<=4)row=int(id)-1;
    else if(id<=8){column=1;row=int(id)-5;}
    else if(items::IsSpecial(id)){column=SmallColumns-1;row=int(id-items::SpecialFirst);}
    else if(id==9||id>=32){column=2;row=id==9?0:int(id)-31;}
    else{
        group=1;bool found=false;
        for(unsigned i=0;i<17;++i){
            if(weaponOrder[i]!=id)continue;
            // Keep all previous weapon cells. Add pump/chrome to free cells or
            // an extra row rather than shifting the existing fifteen weapons.
            int capacity=grid.rows*grid.columns;
            if(int(i)<capacity){column=int(i)/grid.rows;row=int(i)%grid.rows;}
            else{column=(int(i)-capacity)%grid.columns;row=grid.rows+(int(i)-capacity)/grid.columns;}
            found=true;break;
        }
        if(!found)return false;
    }
    return true;
}
inline layout::Box Cell(unsigned id,int screenHeight,float scale){
    int group=0,column=0,row=0;if(!GridPosition(id,group,column,row))return {};
    bool weapon=group==1;
    int wide=weapon?WeaponPitch:SmallPitch;
    // Round the pitch once. Rounding each absolute origin could leave a
    // neighbour one pixel inside a minimum-height card at small resolutions.
    int pitchX=int(wide*scale+.5f),pitchY=int(RowPitch*scale+.5f);
    int x=int(6.f*scale+.5f)+column*pitchX;
    int anchor=int(float(BottomPadding+(weapon?4*RowPitch:0))*scale+.5f);
    int bottom=anchor+(weapon?row+1:4-row)*pitchY;
    return {x,screenHeight-bottom,pitchX,pitchY};
}
}
