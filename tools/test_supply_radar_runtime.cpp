// Compile the real adapter and exercise the radar lifecycle, live layout editing and artwork decode without a game.
#include "../src/skeeto/core/skeeto_navigation.cpp"
#include <cstdio>
static int calls=0,checks=0,failures=0;
void* g_engine=nullptr;void* g_entlist=nullptr;
void Log(const char*,...){}
int SurfTextWFont(unsigned long,const wchar_t*){return 0;}
void* EntGet(int){++calls;return nullptr;}
void* EntNetworkable(int){++calls;return nullptr;}
void* EntFromHandle(int){++calls;return nullptr;}
int EntHighestIndex(){++calls;return 0;}
int EngLocal(){++calls;return 1;}
bool EngInGame(){++calls;return false;}
void SurfColor(int,int,int,int){}
void SurfFill(int,int,int,int){}
void SurfTextAt(unsigned long,int,int,COLORREF,const wchar_t*){}
void SurfDrawTextured(int,int,int,int,int){}
bool SkeetoNavUploadRgba(int,const unsigned char*,int,int){return false;}
bool SkeetoNavTraceLine(const float*,const float*,unsigned,void*,void*,unsigned){return false;}
static void Check(bool value,const char* name){++checks;if(!value){++failures;std::printf("FAIL %s\n",name);}}
using namespace supplyradar;
static void DefaultsAndIsolation(){
    skeeto_nav::RadarResetDefaults();calls=0;
    Tick(100);
    Check(!skeeto_nav::RadarEnabled()&&calls==0,"disabled tick touches no engine/entity producer");
    Check(options::values[options::Enable]==0&&!options::values[options::ItemBase+32]&&options::values[options::ItemBase+34],"defaults: master off, new world supplies off");
    skeeto_nav::RadarSet(0,1);
    Check(skeeto_nav::RadarEnabled()&&mapPending,"enabling requests a map start on the next tick");
    publishedCount=1;phase=1;live=true;
    skeeto_nav::RadarSet(0,-1); // normalized to 0: must clean up exactly like an explicit off
    Check(!skeeto_nav::RadarEnabled()&&!live&&!phase&&!publishedCount&&!mapPending,"normalized close clears the display, scan and pending start");
    Check(!skeeto_nav::RadarItemSupported(10)&&skeeto_nav::RadarItemSupported(32)&&skeeto_nav::RadarItemSupported(34),"supported labels include the new world supplies");
    Check(std::wcslen(skeeto_nav::RadarItemName(34))>0&&std::wcslen(skeeto_nav::RadarItemName(1))>0,"every label has a name");
    Check(!ScanSetting(options::Opacity)&&!ScanSetting(options::Direction)&&!ScanSetting(options::LayoutBase)&&ScanSetting(options::Range)&&ScanSetting(options::ItemBase+3)&&ScanSetting(options::MapMetadata),"only entity-selection settings reset the scan");
}
static void SetBoxes(unsigned id,layout::Box box,layout::Box plate){layout::boxes[id]=layout::automaticBoxes[id]=box;layout::panelBoxes[id]=plate;layout::visualBoxes[id]=box;}
static void LiveEditing(){
    skeeto_nav::RadarResetDefaults();skeeto_nav::RadarSet(0,1);mapPending=false;
    layout::width=1280;layout::height=720;SyncLayoutFromSettings();settingsChanged=false;
    // Selected card 1 starts at x=100, plate 104..196; card 2's plate left edge sits 5 px to the right of it.
    layout::Begin(1); // Begin clears the plates; the painted frame then fills them in
    SetBoxes(1,{100,100,100,40},{104,104,92,16});SetBoxes(2,{300,300,100,40},{109,300,92,16});
    Check(layout::active&&layout::selected==1,"edit starts on the requested item");
    ApplyEditKey(VK_RIGHT);
    Check(layout::items[1].custom==1&&layout::boxes[1].x==105,"arrow key snaps the plate edge to the peer within tolerance");
    Check(layout::GuideVisible(layout::guideX,true),"a snapped axis leaves an alignment guide");
    Check(options::values[options::LayoutBase+4]==1&&options::values[options::LayoutBase+5]==layout::items[1].x&&settingsChanged,"every applied key writes the setting at once and requests a save");
    int stored=options::values[options::LayoutBase+5];
    settingsChanged=false;ApplyEditKey(VK_OEM_PLUS);
    Check(layout::items[1].scale==105&&options::values[options::LayoutBase+7]==105&&settingsChanged,"resize applies live too");
    Check(layout::items[1].x!=0&&options::values[options::LayoutBase+5]==layout::items[1].x&&stored>0,"position survives a resize");
    // Ctrl (free move) bypasses snapping: one pixel only.
    SetBoxes(1,{100,100,100,40},{104,104,92,16});layout::items[1]={};layout::ResetCursor();
    layout::Key(VK_RIGHT,false,true);
    Check(layout::boxes[1].x==101,"free move ignores alignment");
    // Enter/Esc only leave editing; nothing is rolled back.
    int before=options::values[options::LayoutBase+5];
    ApplyEditKey(VK_RETURN);
    Check(!layout::active&&options::values[options::LayoutBase+5]==before,"Enter leaves edit mode without reverting");
    layout::Begin(1);ApplyEditKey(VK_ESCAPE);
    Check(!layout::active&&options::values[options::LayoutBase+5]==before,"Esc leaves edit mode without reverting");
    // Reset key restores the item and writes defaults.
    layout::Begin(1);ApplyEditKey('R');
    Check(layout::items[1].custom==0&&options::values[options::LayoutBase+4]==0&&options::values[options::LayoutBase+7]==100,"R resets the item and its stored fields");
    EndEdit();
    Check(skeeto_nav::ConsumeSettingsChanged()&&!skeeto_nav::ConsumeSettingsChanged(),"save notification is consumed once");
    layout::Begin(1);ClearMap();
    Check(!layout::active&&!live,"map cleanup cancels editing");
    skeeto_nav::RadarResetDefaults();
}
static void MenuDrivenEditStart(){
    // The menu click closes the menu and requests the edit in the same poll, before Tick can set live again.
    skeeto_nav::RadarResetDefaults();skeeto_nav::RadarSet(0,1);mapPending=false;
    layout::width=1280;layout::height=720;
    inSession=true;radarReady=true;live=false;playerValid=true;menuShown=true;
    skeeto_nav::RadarBeginEdit(1);
    Check(layout::active&&layout::selected==1,"edit starts while live is still false from the open menu");
    Check(skeeto_nav::RadarEditing(),"adapter reports editing");
    EndEdit();radarReady=false;
    skeeto_nav::RadarBeginEdit(1);
    Check(!layout::active,"edit is refused when the radar is not configured");
    radarReady=true;configured=true;playerValid=false;
    Check(std::wcscmp(skeeto_nav::RadarStatus(),UiText(UiId::RadarStatusWaiting,L"等待有效玩家"))==0,"status waits for a valid player");
    playerValid=true;live=false;menuShown=true;
    Check(std::wcscmp(skeeto_nav::RadarStatus(),UiText(UiId::RadarStatusWaiting,L"等待有效玩家"))!=0,"an open menu alone does not report a missing player");
    menuShown=false;inSession=false;radarReady=false;live=false;playerValid=false;skeeto_nav::RadarResetDefaults();
}
static void ArtworkAndFonts(){
    unsigned ticks=0,largest=0;
    while(!pixels&&!artFailed&&++ticks<200){unsigned before=decodeOutput;ArtworkStep();if(decodeOutput-before>largest)largest=decodeOutput-before;}
    Check(pixels&&!artFailed&&decodeOutput==IconCount*classic_hud_artwork::Stride,"all embedded HUD artwork decodes without external assets");
    Check(ticks>2&&largest<=64*1024+4*65535,"artwork decoding is chunked per tick");
    Check(hud_digit_font::Load()&&hud_numeric_font::Load()&&hud_heart_font::Load()&&hud_cat_font::Load(),"private HUD fonts register");
    fontBank[14]=123;uiFontBank[14]=321;radarFontBank[1][14]=456;supplyradar::font=123;
    skeeto_nav::InvalidateFonts();
    Check(!fontBank[14]&&!uiFontBank[14]&&!radarFontBank[1][14]&&!supplyradar::font&&supplyradar::textureDirty,"font invalidation drops handles and rebuilds textures lazily");
    ReleaseArtwork();
    Check(!pixels&&!decodeOutput&&!decodeInput,"closing releases artwork and partial decoding");
    UnloadFonts();
}
static void StatsLine(){
    char text[400]{};unsigned n=skeeto_nav::RadarStats(text,sizeof text);
    Check(n>20&&n<sizeof text&&std::strstr(text,"radar ready=")&&std::strstr(text,"work avg/peak="),"diagnostic line reports state and cost");
}
int main(){DefaultsAndIsolation();LiveEditing();MenuDrivenEditStart();ArtworkAndFonts();StatsLine();std::printf("%d runtime checks, %d failures\n",checks,failures);return failures?1:0;}
