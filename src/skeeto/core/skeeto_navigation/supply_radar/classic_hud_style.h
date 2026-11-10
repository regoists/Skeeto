#pragma once
#include "items.h"
#include "original_hud_layout.h"
namespace classic_hud {
constexpr float DistanceFont=9.f,ArrowHalf=2.1f;
constexpr int DistanceRed=225,DistanceGreen=233,DistanceBlue=237;
constexpr int SideRed=190,SideGreen=205,SideBlue=212;
// 1.2.3's visible panel was [-5,33] for supplies and [-5,62] for
// weapons, with 15 HUD units of height. Keep the current grouped cells
// and arrow margin; restore the visible plate and icon sizes inside them.
inline float PanelWidth(unsigned id){return id>=11&&id<32?67.f:38.f;}
inline original_hud::Meta IconPreset(unsigned id){
    // Gas cans, fireworks and specials use the medical item size, not the weapon size.
    if(id>=32)return original_hud::meta[1];
    auto m=original_hud::meta[id];
    if(!m.hasPanel){m.wide=72;m.tall=18;}
    return m;
}
inline void IconSize(unsigned id,float sx,float sy,int& wide,int& tall){
    if(id==34){wide=Limit(int(28*sx+.5f),1,512);tall=Limit(int(14*sy+.5f),1,256);return;}
    if(items::IsSpecial(id)){wide=Limit(int(16*sx+.5f),1,512);tall=Limit(int(13*sy+.5f),1,256);return;}
    // Match the AK/M16 size envelope using the cropped, visible artwork.
    // Compact SMGs retain their naturally shorter silhouette at the same height.
    // All sniper/hunting rifle sizes and existing supply sizes remain unchanged.
    if(id==15||id==16||id==20||id==21||id==24||id==25||id==26||id==27||id==28){
        wide=Limit(int(41*sx+.5f),1,512);tall=Limit(int(12*sy+.5f),1,256);return;
    }
    auto m=IconPreset(id);wide=tall=0;
    if(!m.textureWidth||!m.textureHeight)return;
    wide=Limit(int(float(m.cropWidth)*m.wide*sx/m.textureWidth+.5f),1,512);
    tall=Limit(int(float(m.cropHeight)*m.tall*sy/m.textureHeight+.5f),1,256);
}
}
