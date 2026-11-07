#pragma once
// Extracted from the unchanged user VPK's resource/ui/hud/itempickup.res.
// CItemPickupPanel parent: xpos=0,ypos=0,wide=f0,tall=f0 (full screen).
// All configured radar X values are absolute; Y values are rN (bottom minus N).
// Values are raw Source HUD units. scaleImage=1 stretches the entire original VTF
// frame to wide/tall. Draw cropped texture at cropX*wide/textureWidth etc., retaining
// its original full-frame position and scaling independently in X and Y.
// hasPanel=0 means NO original radar layout; do not infer a original panel there.
namespace original_hud {
struct Meta {
  unsigned char hasPanel;
  unsigned short x,bottom,wide,tall,textureWidth,textureHeight,cropX,cropY,cropWidth,cropHeight;
};
static constexpr Meta meta[32]={
{0,0,0,0,0,0,0,0,0,0,0},
{1,0,171,36,18,128,64,14,13,47,39},
{1,0,153,36,18,128,64,21,8,31,48},
{1,0,135,36,18,128,64,22,14,27,38},
{1,0,117,36,18,128,64,15,9,48,46},
{1,36,171,36,18,128,64,11,9,48,47},
{1,36,153,36,18,128,64,14,9,48,47},
{1,36,135,36,18,128,64,12,8,51,47},
{1,36,117,36,18,128,64,10,11,51,43},
{1,72,171,36,18,128,64,11,7,52,51},
{1,0,261,72,18,256,64,22,10,146,43},
{1,0,279,72,18,256,64,24,10,146,44},
{1,0,261,72,18,256,64,22,10,146,43},
{1,0,243,72,18,256,64,23,11,144,40},
{1,0,225,72,18,256,64,22,10,147,40},
{0,0,0,0,0,256,128,5,84,200,38},
{0,0,0,0,0,256,128,7,84,78,38},
{1,0,207,72,18,256,64,19,11,148,39},
{1,0,189,72,18,256,64,22,13,148,33},
{1,0,297,72,18,256,64,40,6,120,48},
{0,0,0,0,0,256,128,5,84,101,38},
{0,0,0,0,0,256,128,5,68,146,38},
{0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,256,128,5,84,198,38},
{0,0,0,0,0,256,128,5,84,203,38},
{1,0,333,72,18,256,64,21,14,147,31},
{1,0,351,72,18,256,64,70,15,90,29},
{0,0,0,0,0,256,128,5,84,173,38},
{1,0,315,72,18,256,64,43,9,124,44},
{0,0,0,0,0,256,64,35,13,125,39},
{0,0,0,0,0,0,0,0,0,0,0}
};
}
