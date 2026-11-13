// Authorized adaptation of L4N_Minimap 0.1.19 by RagDoll for Skeeto.
// Original gameplay algorithms live under skeeto_navigation/; this host uses
// Skeeto's Paint/menu/settings lifecycle and has no L4N platform dependency.
#include "skeeto_navigation.h"
#include "skeeto_host.h"
#include "skeeto_navigation/controls.h"
#include "skeeto_navigation/minimap_ui.h"
#include "skeeto_navigation/nav_incremental.h"
#include "skeeto_navigation/performance.h"
#include "skeeto_navigation/map_editor.h"
#include "skeeto_navigation/world_navigation.h"
#include "skeeto_navigation/world_abi.h"
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <atomic>
extern bool SkeetoNavUploadRgba(int id,const unsigned char* data,int width,int height);
struct Blob {unsigned char* data=nullptr;unsigned size=0;void Clear(){if(data)HeapFree(GetProcessHeap(),0,data);data=nullptr;size=0;}bool Allocate(unsigned n){Clear();if(!n||n>nav::MaxBytes)return false;data=(unsigned char*)HeapAlloc(GetProcessHeap(),0,n);size=data?n:0;return data!=nullptr;}};
static controls::Settings settings;static nav::Mesh mesh;static nav::ParseJob parser;static Blob loading;
static minimap::Raster raster;static minimap::View view;
static minimap::MapEditor mapEditor;
static bool worldHudEditing;
static void EndPositionEdit(){mapEditor.End();worldHudEditing=false;}
static minimap::LabelLayout labelLayout;static unsigned labelRaster=~0u,labelOverlay=~0u;static int labelSize,labelFont;
static unsigned supplyFont;static int supplyFontSize;static void* modelInfo;
struct ModelCheck {int index;bool laser;};static ModelCheck modelChecks[64];static unsigned modelCheckCount;
static minimap::Scene lastScene,terrainScene;static nav::FloorTracker floorTracker;static bool sceneValid,terrainValid;
static unsigned meshSerial,rasterBuilds,textureUploads,framesReused;
static guidance::Navigator navigator;static items::Snapshot itemSnapshot,itemScratch;
// Same game/Paint owner as Navigator. Events publish only the round boundary.
static bool progressEnabled=false,progressPlayable=false,progressRoundEnded=false;
static std::atomic_int progressBoundary{0}; // 1: start/reset, 2: ended/reset
struct ProgressState {
    float baseline=0;unsigned generation=0,reached=0,samples=0,until=0,sampledAt=0;
    int percent=0,notice=0;
    bool soundPending=false;
};
static ProgressState progress;
static void ResetProgress(){progress={};progressPlayable=false;}
static void UpdateProgress(unsigned now){
    if(!progressEnabled||progressRoundEnded)return;
    if(progress.samples&&now-progress.sampledAt<200)return;
    progress.sampledAt=now;
    ++progress.samples;
    if(navigator.state!=guidance::State::Route&&navigator.state!=guidance::State::AtGoal)return;
    if(progress.generation!=navigator.generation){
        progress.generation=navigator.generation;progress.baseline=0;progress.notice=0;
    }
    if(progress.baseline<=0){
        if(navigator.routeLength>0){progress.baseline=navigator.routeLength;progress.percent=0;}
        return;
    }
    progress.percent=int(Limit(1.f-navigator.routeLength/progress.baseline,0.f,1.f)*100.f+.01f);
    const int thresholds[]={25,50,75,90};int highest=0;
    for(unsigned i=0;i<4;++i)if(progress.percent>=thresholds[i]&&!(progress.reached&(1u<<i))){
        progress.reached|=1u<<i;highest=thresholds[i];
    }
    if(highest){progress.notice=highest;progress.until=now+5000u;progress.soundPending=true;}
}
static worldnav::Overlay worldOverlay;static void* worldTrace;static bool worldReady;
static performance::Metric worldCost;static unsigned worldPaints;
static unsigned overlaySerial,lastItemPoll,lastRoutePoll;static bool itemScanAvailable,anchorValid;static Vec sessionAnchor;
static int itemCursor,itemHighest;static minimap::View itemScanView;static unsigned scanMask,scanWeapons;static int scanRange;
static unsigned itemSweeps,itemChanges;
static performance::Metric frameCost,itemCost,paintCost,uploadCost;
static void *engine,*engineVGui,*entities,*surface,*filesystem;
static char mapName[240],navPath[256];static void* navFile;static unsigned readAt,lastMapCheck,lastFrame,lastRead;
static bool inSession,liveFrame,textureDirty,firstFrameReported;static int texture;static unsigned font,lastDependencyAttempt;static int fontSize,lastScreenW,lastScreenH;
static const WCHAR* navStatus=L"等待地图";
static const WCHAR* LocalizedLoadStatus(){
    if(!std::wcscmp(navStatus,L"等待地图"))return UiText(UiId::NavLoad0,L"等待地图");
    if(!std::wcscmp(navStatus,L"地图名不可用"))return UiText(UiId::NavLoad1,L"地图名不可用");
    if(!std::wcscmp(navStatus,L"无导航接口"))return UiText(UiId::NavLoad2,L"无导航接口");
    if(!std::wcscmp(navStatus,L"无 NAV 地图"))return UiText(UiId::NavHud16,L"无 NAV 地图");
    if(!std::wcscmp(navStatus,L"导航过大"))return UiText(UiId::NavLoad3,L"导航过大");
    if(!std::wcscmp(navStatus,L"正在读取地图"))return UiText(UiId::NavLoad4,L"正在读取地图");
    if(!std::wcscmp(navStatus,L"读取失败"))return UiText(UiId::NavLoad5,L"读取失败");
    if(!std::wcscmp(navStatus,L"导航不兼容"))return UiText(UiId::NavLoad6,L"导航不兼容");
    if(!std::wcscmp(navStatus,L"正在解析地图"))return UiText(UiId::NavLoad7,L"正在解析地图");
    return UiText(UiId::NavLoad8,L"通路");
}
static bool menuShown,uiShown,ready,settingsChanged;
static bool ConsoleVisible(){return MethodOK(engine,11)&&VCall<bool(__thiscall*)(void*)>(engine,11)(engine);}
// Chat / GameUI / console: hide or pause presentation. Skeeto menu is NOT included —
// radar settings need the icons to stay live while the menu is open.
static bool GameTextInputVisible(){return menuShown||uiShown||ConsoleVisible();}
static bool GameUiOrConsoleVisible(){return uiShown||ConsoleVisible();}
struct Property {RecvTable* table;const char* name;int offset;};static Property properties[512];static int propertyCount;
static bool SafeSame(const char* a,const char* b){for(unsigned i=0;i<160;++i){if(!Readable(a+i,1)||a[i]!=b[i])return false;if(!b[i])return true;}return false;}
static int FindOffset(RecvTable* table,const char* name,int depth=0){if(!Readable(table,sizeof *table)||depth>16||table->count<=0||table->count>4096||!Readable(table->props,table->count*sizeof(RecvProp)))return -1;
    for(int i=0;i<table->count;++i){const auto& p=table->props[i];if(p.name&&SafeSame(p.name,name))return p.offset;}
    for(int i=0;i<table->count;++i){const auto& p=table->props[i];if(p.type==6&&p.table){int off=FindOffset(p.table,name,depth+1);if(off>=0&&p.offset>=0&&p.offset<65536-off)return p.offset+off;}}return -1;}
static int Offset(ClientClass* cls,const char* name){if(!Readable(cls,sizeof *cls))return -1;for(int i=0;i<propertyCount;++i)if(properties[i].table==cls->table&&Same(properties[i].name,name))return properties[i].offset;int off=FindOffset(cls->table,name);if(propertyCount<512)properties[propertyCount++]={cls->table,name,off};return off;}
template<class T>static bool ReadProperty(void* entity,ClientClass* cls,const char* name,T& value){int off=Offset(cls,name);if(!entity||off<0||unsigned(off)>65536-sizeof(T)||!Readable((char*)entity+off,sizeof(T)))return false;value=*(T*)((char*)entity+off);return true;}
static bool Player(int index,Vec& position,int& team,bool local){
    if(index<1||index>64)return false;auto network=VCall<void*(__thiscall*)(void*,int)>(entities,0)(entities,index);if(!MethodOK(network,1))return false;
    // IClientNetworkable::IsDormant is slot 7 in the current L4D2 ABI.
    if(!local&&MethodOK(network,7)&&VCall<bool(__thiscall*)(void*)>(network,7)(network))return false;
    auto cls=VCall<ClientClass*(__thiscall*)(void*)>(network,1)(network);auto player=VCall<void*(__thiscall*)(void*,int)>(entities,3)(entities,index);unsigned char life=1;
    return ReadProperty(player,cls,"m_iTeamNum",team)&&team==2&&ReadProperty(player,cls,"m_lifeState",life)&&life==0&&ReadProperty(player,cls,"m_vecOrigin",position)&&Finite(position.x)&&Finite(position.y)&&Finite(position.z)&&nav::Abs(position.x)<1000000&&nav::Abs(position.y)<1000000&&nav::Abs(position.z)<1000000;
}
#include "skeeto_navigation/bug_scan.h"
static void CancelRead(){if(navFile&&filesystem)VCall<void(__thiscall*)(void*,void*)>(filesystem,3)(filesystem,navFile);navFile=nullptr;parser.Cancel();loading.Clear();readAt=0;}
struct ItemClass {ClientClass* cls;items::Kind kind;bool spawn;bool upgrade;items::Label label;int offsets[6];unsigned span;};static ItemClass itemClasses[256];static unsigned itemClassCount;
static ItemClass ClassifyItem(ClientClass* cls){
    for(unsigned i=0;i<itemClassCount;++i)if(itemClasses[i].cls==cls)return itemClasses[i];
    ItemClass result{cls,items::Kind::None,false,false,items::Label::None,{-1,-1,-1,-1,-1,-1},0};char name[80]{};
    if(Readable(cls,sizeof *cls)&&cls->name){unsigned i=0;for(;i<79&&Readable(cls->name+i,1)&&cls->name[i];++i)name[i]=cls->name[i];
        if(i<79){result.spawn=Same(name,"CWeaponSpawn");result.kind=items::FromClass(name);result.label=items::ClassLabel(name);result.upgrade=Same(name,"CBaseUpgradeItem");}}
    if(result.spawn||result.upgrade||result.kind!=items::Kind::None){
        const char* names[]={"m_hOwnerEntity","m_fEffects","m_nModelIndex","m_vecOrigin","m_hOwner","m_weaponID"};
        for(int i=0;i<6;++i){int offset=Offset(cls,names[i]),width=i==3?12:i==2?2:4;
            if(offset>=0&&offset<=65536-width){result.offsets[i]=offset;unsigned end=unsigned(offset+width);if(end>result.span)result.span=end;}}
        if(result.offsets[0]<0||result.offsets[1]<0||result.offsets[2]<0||result.offsets[3]<0||(result.spawn&&result.offsets[5]<0))result.span=0;
    }
    if(itemClassCount<256)itemClasses[itemClassCount++]=result;return result;
}
// VModelInfoClient004 slots 1/3 were verified in the supported engine binary.
// Upgrade ammo and laser stations share CBaseUpgradeItem; the class alone is unsafe.
static bool LaserModelIndex(int index){
    if(!modelInfo||!items::ValidModel(index))return false;
    for(unsigned i=0;i<modelCheckCount;++i)if(modelChecks[i].index==index)return modelChecks[i].laser;
    const void* model=nullptr;const char* name=nullptr;
    __try {
        model=VCall<const void*(__thiscall*)(void*,int)>(modelInfo,1)(modelInfo,index);
        if(model)name=VCall<const char*(__thiscall*)(void*,const void*)>(modelInfo,3)(modelInfo,model);
    } __except(EXCEPTION_EXECUTE_HANDLER) {modelInfo=nullptr;return false;}
    if(!name)return false;
    char copy[128]{};unsigned i=0;for(;i<127&&Readable(name+i,1)&&name[i];++i)copy[i]=name[i];
    bool laser=i<127&&Readable(name+i,1)&&!name[i]&&items::LaserModel(copy);
    if(modelCheckCount<64)modelChecks[modelCheckCount++]={index,laser};return laser;
}
static unsigned ItemMask(){
    unsigned result=0;
    if(settings.itemAmmo)result|=items::Mask(items::Kind::Ammo);
    if(settings.itemBile)result|=items::Mask(items::Kind::Bile);
    if(settings.itemMolotov)result|=items::Mask(items::Kind::Molotov);
    if(settings.itemPills)result|=items::Mask(items::Kind::Pills);
    if(settings.itemMedkit)result|=items::Mask(items::Kind::Medical);
    if(settings.itemPipeBomb)result|=items::Mask(items::Kind::PipeBomb);
    if(settings.itemAdrenaline)result|=items::Mask(items::Kind::Adrenaline);
    if(settings.itemDefib)result|=items::Mask(items::Kind::Defibrillator);
    if(settings.itemLaser)result|=items::Mask(items::Kind::Laser);
    if(settings.itemWeapons)result|=items::Mask(items::Kind::Weapon);
    return result;
}
static unsigned WeaponMask(){unsigned mask=0;for(unsigned i=0;i<items::WeaponCount;++i)if(settings.weapons[i])mask|=1u<<i;return mask;}
static void ScanItems(const minimap::View& v,unsigned budget=256){
    performance::Measure timing(itemCost);
    if(!itemCursor){itemScratch.count=0;itemScratch.available=itemScanAvailable;itemScanView=v;scanMask=ItemMask();scanWeapons=WeaponMask();scanRange=settings.itemRange;
        itemHighest=itemScanAvailable?Limit(EntHighestIndex(),0,2047):0;itemCursor=1;}
    // Every object's readable span is rechecked. Only immutable method tables
    // share validation within this synchronous chunk, never across frames.
    void** verifiedTables[32];unsigned verifiedCount=0,processed=0;
    if(itemScanAvailable){
        while(itemCursor<=itemHighest&&processed<budget){
            if(processed&&performance::Microseconds(performance::Now()-timing.start)>=750)break;
            int i=itemCursor++;++processed;auto network=VCall<void*(__thiscall*)(void*,int)>(entities,0)(entities,i);
            if(!Readable(network,4))continue;auto methods=*(void***)network;bool verified=false;
            for(unsigned j=0;j<verifiedCount;++j)if(methods==verifiedTables[j]){verified=true;break;}
            if(!verified){if(!Readable(methods,8*4)||!Executable(methods[1])||!Executable(methods[7]))continue;if(verifiedCount<32)verifiedTables[verifiedCount++]=methods;}
            if(VCall<bool(__thiscall*)(void*)>(network,7)(network))continue;
            auto cls=VCall<ClientClass*(__thiscall*)(void*)>(network,1)(network);auto info=ClassifyItem(cls);
            if(!info.span)continue;
            auto entity=(char*)VCall<void*(__thiscall*)(void*,int)>(entities,3)(entities,i);if(!Readable(entity,info.span))continue;
            // L4D2's networked model index is a short; the following bytes are
            // render FX/mode and must not be included in the index.
            int model=*(short*)(entity+info.offsets[2]);
            if(info.spawn){int id=*(int*)(entity+info.offsets[5]);info.kind=items::FromWeaponID(id);info.label=items::WeaponLabel(id);}
            if(info.upgrade&&settings.itemLaser&&LaserModelIndex(model)){info.kind=items::Kind::Laser;info.label=items::Label::Laser;}
            if(!items::Enabled(info.kind,scanMask))continue;
            unsigned owner=*(unsigned*)(entity+info.offsets[0]),weaponOwner=info.offsets[4]>=0?*(unsigned*)(entity+info.offsets[4]):nav::Invalid;
            int effects=*(int*)(entity+info.offsets[1]);Vec p=*(Vec*)(entity+info.offsets[3]);
            if(!items::Available(owner,weaponOwner,effects,model)||!Finite(p.x)||!Finite(p.y)||!Finite(p.z)||nav::Abs(p.x)>1000000||nav::Abs(p.y)>1000000||nav::Abs(p.z)>1000000)continue;
            itemScratch.Add(p,info.kind,i,itemScanView.player,float(scanRange),itemScanView.LayerZ(),itemScanView.band,itemScanView.layering,scanMask,info.label,scanWeapons);
        }
    }else itemCursor=itemHighest+1;
    // Publish only a complete snapshot; partial scans must not flash icons.
    if(itemCursor>itemHighest){itemCursor=0;++itemSweeps;if(!itemSnapshot.Matches(itemScratch)){itemSnapshot=itemScratch;++overlaySerial;++itemChanges;}}
}
static void UpdateExtras(const minimap::View& v,unsigned now){
    if(!anchorValid){sessionAnchor=v.player;anchorValid=true;}
    if(settings.navigation||progressEnabled){
        if(!navigator.prepared&&!navigator.preparePhase)navigator.BeginPrepare(mesh,sessionAnchor);
        if(navigator.preparePhase)navigator.PrepareStep(mesh);
        if(navigator.prepared){
            navigator.Select(settings.goal);
            if(navigator.source==nav::Invalid||now-lastRoutePoll>=200){lastRoutePoll=now;navigator.Begin(mesh,v.player);}
            // Route expansion keeps the source plugin's 400 us game-thread budget.
            auto searchStart=performance::Now();unsigned batches=0;
            do{navigator.Step(mesh,512);}while(navigator.building&&++batches<16&&performance::Microseconds(performance::Now()-searchStart)<400);
        }
    }else if(navigator.prepared||navigator.preparePhase)navigator.Clear();
    UpdateProgress(now);
    if(settings.enabled&&settings.items){
        unsigned mask=ItemMask();
        if(itemSnapshot.Filter(mask,WeaponMask()))++overlaySerial;
        if(!mask){itemCursor=0;lastItemPoll=0;itemSnapshot.available=itemScanAvailable;return;}
        if(scanMask!=mask||scanWeapons!=WeaponMask()||scanRange!=settings.itemRange||
            (itemCursor&&(itemScanView.layer!=v.layer||itemScanView.layering!=v.layering||itemScanView.band!=v.band||nav::Abs(itemScanView.ground-v.ground)>96))){itemCursor=0;lastItemPoll=0;}
        if(itemCursor||!lastItemPoll||now-lastItemPoll>=200){if(!itemCursor)lastItemPoll=now;ScanItems(v);}}
    else{itemCursor=0;if(itemSnapshot.count){itemSnapshot.count=0;++overlaySerial;}}
}

// Minimap chip: only abnormal states; routine route and count readouts stay off the disc.
static unsigned noticeColor=minimap::Amber;
static const WCHAR* MapNotice(){
    noticeColor=minimap::Amber;
    if(!mesh.count)return LocalizedLoadStatus();
    if(raster.budgetExceeded)return UiText(UiId::NavHud39, L"底图已简化");
    if(settings.navigation){auto status=minimap::NavigationStatus(navigator);
        if(status.color==minimap::Amber||status.color==minimap::Failure){noticeColor=status.color;return status.text;}}
    if(settings.items&&!itemSnapshot.available)return UiText(UiId::NavHud41, L"物品接口暂不可用");
    return nullptr;
}

#include "skeeto_navigation/supply_radar/runtime.h"

static bool MapPath(const char* name){unsigned n=0;for(;n<sizeof(mapName)-1;++n){if(!Readable(name+n,1))return false;char c=name[n];if(!c)break;if(c=='\\')c='/';mapName[n]=c;}if(n>=sizeof(mapName)-1)return false;mapName[n]=0;if(n<1)return false;
    if(n>=4&&Same(mapName+n-4,".bsp")){n-=4;mapName[n]=0;}
    for(unsigned i=0;i<n;++i)if(mapName[i]==':'||(mapName[i]=='.'&&mapName[i+1]=='.'))return false;
    bool prefix=n>5&&mapName[0]=='m'&&mapName[1]=='a'&&mapName[2]=='p'&&mapName[3]=='s'&&mapName[4]=='/';
    unsigned at=0;if(!prefix){const char* p="maps/";while(*p)navPath[at++]=*p++;}for(unsigned i=0;i<n;++i)navPath[at++]=mapName[i];const char* ext=".nav";while(*ext)navPath[at++]=*ext++;navPath[at]=0;return true;}
// The radar needs no NAV mesh (straight-line distance); NAV is only read for the minimap / world route.
static bool NavWanted(){return settings.enabled!=0||(settings.navigation!=0&&settings.worldNavigation!=0)||progressEnabled||g_bug.enabled;}
static bool navDeferred;
static void StartMap(const char* name){navDeferred=false;BugOnMapCleared();supplyradar::ClearMap();EndPositionEdit();CancelRead();navigator.Clear();worldOverlay.Clear();itemCursor=0;modelCheckCount=0;itemClassCount=propertyCount=0;itemSnapshot.count=0;lastItemPoll=lastRoutePoll=0;anchorValid=false;++overlaySerial;mesh.Clear();floorTracker.Reset();sceneValid=terrainValid=false;++meshSerial;liveFrame=false;firstFrameReported=false;settings.goal=0;mapName[0]=0;navPath[0]=0;
    ResetProgress();progressRoundEnded=false;navigator.measureLength=progressEnabled;
    if(!name||!MapPath(name)){navStatus=L"地图名不可用";Log("Map path unavailable.");return;}
    if(supplyradar::options::values[supplyradar::options::Enable])supplyradar::StartMapNamed(mapName);
    if(!NavWanted()){navStatus=L"等待地图";navDeferred=true;return;} // radar only: no NAV read or parse
    Log("Loading map NAV: %s",navPath);
    if(!filesystem){navStatus=L"无导航接口";return;}
    navFile=VCall<void*(__thiscall*)(void*,const char*,const char*,const char*)>(filesystem,2)(filesystem,navPath,"rb","GAME");
    if(!navFile){navStatus=L"无 NAV 地图";Log("NAV absent in mounted GAME search paths.");return;}
    unsigned n=VCall<unsigned(__thiscall*)(void*,void*)>(filesystem,7)(filesystem,navFile);
    if(!loading.Allocate(n)){navStatus=L"导航过大";CancelRead();Log("NAV size/allocation rejected.");return;}navStatus=L"正在读取地图";
}
static void ReadStep(){if(!navFile)return;unsigned amount=loading.size-readAt;if(amount>1024*1024)amount=1024*1024;
    int got=VCall<int(__thiscall*)(void*,void*,int,void*)>(filesystem,0)(filesystem,loading.data+readAt,int(amount),navFile);
    if(got<=0||unsigned(got)>amount){CancelRead();navStatus=L"读取失败";Log("NAV read failed.");return;}readAt+=got;
    if(readAt==loading.size){VCall<void(__thiscall*)(void*,void*)>(filesystem,3)(filesystem,navFile);navFile=nullptr;
        auto result=parser.Begin(mesh,loading.data,loading.size);
        if(result==nav::ParseJob::Result::Failed){navStatus=L"导航不兼容";loading.Clear();}
        else navStatus=L"正在解析地图";
    }
}
static void UpdateFrame(){performance::Measure timing(frameCost);int index=VCall<int(__thiscall*)(void*)>(engine,12)(engine);Vec position,angles{};int team=0;if(!Player(index,position,team,true)){liveFrame=false;progressPlayable=false;return;}
    progressPlayable=true;
    // Progress alone needs position and one shared route update, no camera,
    // terrain composition, teammates, supplies, or texture upload.
    if(!settings.enabled&&!(settings.navigation&&settings.worldNavigation)){
        minimap::View progressView{};progressView.player=position;
        UpdateExtras(progressView,GetTickCount());liveFrame=false;return;
    }
    VCall<void(__thiscall*)(void*,Vec&)>(engine,19)(engine,angles);if(!Finite(angles.y)||nav::Abs(angles.y)>36000){liveFrame=false;return;}
    if(!settings.enabled){
        minimap::View routeView{};routeView.player=position;routeView.yaw=angles.y;routeView.range=float(settings.range);routeView.ground=position.z;routeView.band=96;routeView.layer=0;routeView.layering=true;
        UpdateExtras(routeView,GetTickCount());view=routeView;liveFrame=true;return;
    }
    minimap::Scene next;auto& v=next.view;
    v.player=position;v.yaw=angles.y;v.range=float(settings.range);v.ground=floorTracker.Update(mesh,position);v.band=96;v.layer=0;v.style=settings.style;v.shape=settings.shape;v.rotate=settings.rotate!=0;v.layering=true;v.Prepare();
    next.meshSerial=meshSerial;
    UpdateExtras(v,GetTickCount());next.overlaySerial=0;next.routeSerial=settings.navigation?navigator.serial+1:0;
    int screenW=0,screenH=0;VCall<void(__thiscall*)(void*,int&,int&)>(engine,5)(engine,screenW,screenH);
    auto screenLayout=minimap::Layout::Make(screenW,screenH,settings.size,settings.posX,settings.posY);
    v.markerScale=Limit(float(minimap::Pixels)/screenLayout.size,2.f/3,2.f);
    view=v;
    if(settings.teammates){int maxClients=Limit(VCall<int(__thiscall*)(void*)>(engine,21)(engine),1,64);for(int i=1;i<=maxClients;++i)if(i!=index){Vec p;int otherTeam=0;if(Player(i,p,otherTeam,false)&&otherTeam==team)next.teammates[next.count++]=p;}}
    if(!sceneValid||!liveFrame||!lastScene.Matches(next)){
        view=v;
        if(!terrainValid||!terrainScene.TerrainMatches(next)){raster.BuildTerrain(mesh,v);terrainScene=next;terrainValid=true;}
        raster.Compose(mesh,v,next.teammates,next.count,settings.navigation?&navigator:nullptr);
        lastScene=next;sceneValid=true;textureDirty=true;++rasterBuilds;
    }else ++framesReused;
    liveFrame=true;
}
static void Text(int x,int y,const WCHAR* text,unsigned color=0xe5ecee,unsigned chosenFont=0){if(!chosenFont)chosenFont=font;if(!chosenFont)return;int n=0;while(text[n]&&n<250)++n;VCall<void(__thiscall*)(void*,unsigned)>(surface,17)(surface,chosenFont);VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,19)(surface,(color>>16)&255,(color>>8)&255,color&255,255);VCall<void(__thiscall*)(void*,int,int)>(surface,20)(surface,x,y);VCall<void(__thiscall*)(void*,const WCHAR*,int,int)>(surface,22)(surface,text,n,0);}
static void TextFit(int x,int y,const WCHAR* text,int width,int pixels,unsigned color){
    WCHAR clipped[64];minimap::FitText(clipped,64,text,width,pixels);Text(x,y,clipped,color);
}
static void Rect(int x,int y,int w,int h,int r,int g,int b,int a){VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,11)(surface,r,g,b,a);VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,12)(surface,x,y,x+w,y+h);}
#include "skeeto_navigation/world_draw.h"
static void Draw(bool showMap,bool showWorld){performance::Measure timing(paintCost);int width=0,height=0;VCall<void(__thiscall*)(void*,int&,int&)>(engine,5)(engine,width,height);if(width<640||height<400||width>16384||height>16384)return;
    if(width!=lastScreenW||height!=lastScreenH){lastScreenW=width;lastScreenH=height;fontSize=supplyFontSize=worldFontSize=0;textureDirty=true;sceneValid=terrainValid=false;}
    const bool worldEditPreview=showWorld&&worldHudEditing;
    if(showWorld&&!worldEditPreview)DrawWorldNavigation(width,height);
    if(!showMap||!settings.enabled){if(worldEditPreview)DrawWorldNavigation(width,height);if(showWorld)VCall<void(__thiscall*)(void*)>(surface,24)(surface);return;}
    // Skeeto already owns the EngineVGui Paint drawing scope.
    auto layout=minimap::Layout::Make(width,height,settings.size,settings.posX,settings.posY);
    int size=layout.size,x=layout.x,y=layout.y;
    int targetFont=layout.font;if(!font)font=VCall<unsigned(__thiscall*)(void*)>(surface,63)(surface);
    if(font&&fontSize!=targetFont){if(VCall<bool(__thiscall*)(void*,unsigned,const char*,int,int,int,int,int,int,int)>(surface,64)(surface,font,"Microsoft YaHei",targetFont,500,0,0,0x90,0,0xffff))fontSize=targetFont;}
    // The disc texture carries its own rim; opacity is applied exactly once.
    if(!texture){const int id=VCall<int(__thiscall*)(void*,bool)>(surface,34)(surface,true);texture=id>0?id:0;textureDirty=true;}
    if(texture){if(textureDirty){performance::Measure uploadTiming(uploadCost);if(SkeetoNavUploadRgba(texture,(const unsigned char*)raster.rgba,minimap::Pixels,minimap::Pixels)){textureDirty=false;++textureUploads;}else texture=0;}
        if(texture){VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,11)(surface,255,255,255,settings.opacity);VCall<void(__thiscall*)(void*,int)>(surface,29)(surface,texture);VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,31)(surface,x,y,x+size,y+size);}}
    if(!supplyFont)supplyFont=VCall<unsigned(__thiscall*)(void*)>(surface,63)(surface);
    int supplyPixels=minimap::SupplyFont(targetFont);
    if(supplyFont&&supplyFontSize!=supplyPixels){if(VCall<bool(__thiscall*)(void*,unsigned,const char*,int,int,int,int,int,int,int)>(surface,64)(surface,supplyFont,"Microsoft YaHei",supplyPixels,600,0,0,0x90,0,0xffff))supplyFontSize=supplyPixels;}
    static bool labelEnglish=false;
    const bool english=g_uiEnglish.load(std::memory_order_relaxed);
    if(labelRaster!=rasterBuilds||labelOverlay!=overlaySerial||labelSize!=size||labelFont!=targetFont||labelEnglish!=english){
        labelLayout.Build(lastScene,itemSnapshot,settings.navigation?&navigator:nullptr,size,targetFont);
        labelRaster=rasterBuilds;labelOverlay=overlaySerial;labelSize=size;labelFont=targetFont;labelEnglish=english;
    }
    if(settings.items)for(int i=0;i<labelLayout.leaderCount;++i){const auto& b=labelLayout.leaders[i];
        Rect(x+b.x,y+b.y,b.w,b.h,145,167,179,b.w==3&&b.h==3?205:135);
    }
    if(settings.items)for(int i=0;i<labelLayout.count;++i){const auto& l=labelLayout.labels[i];const auto& b=l.box;
        unsigned color=labelLayout.metrics.Get(l.label).color;
        // Flat tag: rounded by one pixel, type colour as a slim leading bar.
        Rect(x+b.x+1,y+b.y,b.w-2,b.h,8,15,20,232);
        Rect(x+b.x,y+b.y+1,1,b.h-2,8,15,20,232);
        Rect(x+b.x+b.w-1,y+b.y+1,1,b.h-2,8,15,20,232);
        Rect(x+b.x+1,y+b.y+3,2,b.h-6,(color>>16)&255,(color>>8)&255,color&255,215);
        Text(x+b.x+5,y+b.y+1,items::Text(l.label),color,l.kind==items::Kind::Weapon?font:supplyFont);
    }
    if(const WCHAR* notice=MapNotice()){
        // Only states the disc cannot show itself; one line, measured on change.
        static const WCHAR* noticeSource=nullptr;static int noticeFont=0,noticeLimit=0,noticeWidth=0;static WCHAR noticeText[64]{};
        const int limit=size*(settings.shape?4:7)/(settings.shape?5:10);
        if(noticeSource!=notice||noticeFont!=targetFont||noticeLimit!=limit){noticeSource=notice;noticeFont=targetFont;noticeLimit=limit;
            minimap::FitText(noticeText,64,notice,limit,targetFont);noticeWidth=SurfTextWFont(font,noticeText);}
        const int chipH=targetFont+8,chipW=noticeWidth+26,chipX=x+(size-chipW)/2,chipY=y+size-chipH-size/(settings.shape?24:9);
        Rect(chipX+1,chipY,chipW-2,chipH,8,15,20,238);Rect(chipX,chipY+1,chipW,chipH-2,8,15,20,238);
        Rect(chipX+9,chipY+chipH/2-2,4,4,(noticeColor>>16)&255,(noticeColor>>8)&255,noticeColor&255,255);
        Text(chipX+18,chipY+4,noticeText,noticeColor);
    }
    if(mapEditor.active&&!worldHudEditing){
        int top=y,bottom=y+size;
        Rect(x-2,top-2,size+4,2,242,188,112,255);Rect(x-2,bottom,size+4,2,242,188,112,255);
        Rect(x-2,top,2,bottom-top,242,188,112,255);Rect(x+size,top,2,bottom-top,242,188,112,255);
        for(int side=0;side<2;++side)for(int vertical=0;vertical<2;++vertical){int cx=side?x+size-6:x-2,cy=vertical?bottom-6:top-2;Rect(cx,cy,8,8,242,188,112,255);}
        int hintWidth=Limit(width/3,300,420),hintHeight=2*(targetFont+6)+16;
        int hintX=(width-hintWidth)/2,hintY=layout.margin;
        Rect(hintX+2,hintY+2,hintWidth,hintHeight,0,0,0,settings.opacity/3);
        Rect(hintX,hintY,hintWidth,hintHeight,17,26,32,settings.opacity);
        TextFit(hintX+12,hintY+8,UiText(UiId::NavHud47, L"方向键微调（Shift大步） B放大 S缩小"),hintWidth-24,targetFont,minimap::Amber);
        TextFit(hintX+12,hintY+14+targetFont,UiText(UiId::NavHud48, L"再按2 回车或Esc保存退出"),hintWidth-24,targetFont,0xdce7e8);
    }
    if(worldEditPreview)DrawWorldNavigation(width,height);
    VCall<void(__thiscall*)(void*)>(surface,24)(surface);
    if(!firstFrameReported){firstFrameReported=true;Log("First live minimap frame painted.");}
}
static bool Dependencies(){
    engine=g_engine;entities=g_entlist;
    engineVGui=GetInterface(L"engine.dll","VEngineVGui001");
    surface=GetInterface(L"vguimatsurface.dll","VGUI_Surface031");
    if(!MethodOK(engine,5)||!MethodOK(engine,12)||!MethodOK(engine,19)||!MethodOK(engine,21)||!MethodOK(engine,52)
        ||!MethodOK(entities,0)||!MethodOK(entities,3)||!MethodOK(surface,11)||!MethodOK(surface,12)
        ||!MethodOK(surface,17)||!MethodOK(surface,19)||!MethodOK(surface,20)||!MethodOK(surface,22)
        ||!MethodOK(surface,24)||!MethodOK(surface,28)||!MethodOK(surface,29)||!MethodOK(surface,31)
        ||!MethodOK(surface,32)||!MethodOK(surface,34)||!MethodOK(surface,63)||!MethodOK(surface,64)
        ||!MethodOK(engineVGui,2))return false;
    worldTrace=GetInterface(L"engine.dll","EngineTraceClient003");
    worldReady=worldnav::InterfaceProfile(engine,worldTrace,surface);
    itemScanAvailable=true; // Highest index comes from Skeeto's existing safe wrapper.
    modelInfo=GetInterface(L"engine.dll","VModelInfoClient004");
    if(!MethodOK(modelInfo,1)||!MethodOK(modelInfo,3))modelInfo=nullptr;
    filesystem=GetInterface(L"filesystem_stdio.dll","VBaseFileSystem012");
    if(!MethodOK(filesystem,0)||!MethodOK(filesystem,2)||!MethodOK(filesystem,3)||!MethodOK(filesystem,7)){
        filesystem=nullptr;navStatus=L"无导航接口";return supplyradar::options::values[0]!=0;
    }
    return true;
}
static void ResetSession(){
    ResetProgress();BugOnMapCleared();
    supplyradar::ClearMap();CancelRead();navigator.Clear();worldOverlay.Clear();EndPositionEdit();
    if(texture&&MethodOK(surface,33))VCall<void(__thiscall*)(void*,int)>(surface,33)(surface,texture);
    texture=0;textureDirty=true;labelRaster=labelOverlay=~0u;
    itemCursor=0;modelCheckCount=0;itemClassCount=propertyCount=0;itemSnapshot.count=0;lastItemPoll=lastRoutePoll=0;
    anchorValid=false;++overlaySerial;mesh.Clear();floorTracker.Reset();
    sceneValid=terrainValid=false;liveFrame=false;inSession=false;
    mapName[0]=navPath[0]=0;firstFrameReported=false;navStatus=L"等待地图";++meshSerial;
}
static void ResetMinimapPresentation(){
    if(!worldHudEditing)mapEditor.End();
    if(texture&&MethodOK(surface,33))VCall<void(__thiscall*)(void*,int)>(surface,33)(surface,texture);
    texture=0;textureDirty=true;labelRaster=labelOverlay=~0u;
    itemCursor=0;itemSnapshot.count=0;lastItemPoll=0;
    sceneValid=terrainValid=false;liveFrame=false;lastFrame=0;
}
namespace skeeto_nav {
bool PresentationActive(){return settings.enabled!=0||(settings.navigation!=0&&settings.worldNavigation!=0)||RadarEnabled();}
bool Active(){return PresentationActive()||progressEnabled||g_bug.enabled;}
bool Enabled(){return settings.enabled!=0;}
void Reload();
void SetBugScan(bool on, bool boxes){
    g_bug.boxes=boxes;
    if(g_bug.enabled==on)return;
    g_bug.enabled=on;BugOnMapCleared();
    if(on&&navDeferred&&inSession)Reload();
    else if(!on&&!Active())ResetSession();
}
bool BugBoxesWanted(){return g_bug.enabled&&g_bug.boxes;}
const wchar_t* BugStatus(){return !g_bug.enabled?L"":(g_bug.line[0]?g_bug.line:UiText(UiId::LocalBugStatusScan,L"正在查找"));}
bool BugNearestPoint(float* x,float* y,float* z){return ::BugNearestPoint(x,y,z);}
void PaintBug(){BugPaint();}
void SetProgressEnabled(bool enabled){
    if(progressEnabled==enabled)return;
    progressEnabled=enabled;navigator.measureLength=enabled&&!progressRoundEnded;ResetProgress();
    // Enabling on an already cached route must initialize its length once.
    navigator.routeSource=nav::Invalid;lastFrame=0;
    if(enabled&&navDeferred&&inSession)Reload();
    else if(!enabled&&!Active())ResetSession();
}
void ProgressRoundBoundary(bool ended){progressBoundary.store(ended?2:1,std::memory_order_release);}
int ProgressNotice(){
    return progressEnabled&&progressPlayable&&!progressRoundEnded&&progress.notice
        &&(navigator.state==guidance::State::Route||navigator.state==guidance::State::AtGoal)
        &&(int)(GetTickCount()-progress.until)<0?progress.notice:0;
}
int ProgressPercent(){
    return progressEnabled&&progressPlayable&&!progressRoundEnded&&progress.baseline>0
        &&progress.generation==navigator.generation
        &&(navigator.state==guidance::State::Route||navigator.state==guidance::State::AtGoal)
        ?progress.percent:-1;
}
bool TakeProgressSound(){
    if(!progress.soundPending)return false;
    progress.soundPending=false;
    return ProgressNotice()!=0;
}
const wchar_t* ProgressStatus(){
    if(!progressEnabled)return UiText(UiId::NavDisabled,L"已关闭");
    if(!mesh.count)return LocalizedLoadStatus();
    if(!progressPlayable||progressRoundEnded)return UiText(UiId::ProgressWaitingPlayer,L"等待存活的本地生还者");
    if(navigator.state!=guidance::State::Route&&navigator.state!=guidance::State::AtGoal)
        return minimap::NavigationStatus(navigator).text;
    return progress.baseline>0?UiText(UiId::ProgressReady,L"路程估算已就绪")
        :UiText(UiId::ProgressWaitingRoute,L"等待有效的初始路程");
}
unsigned ProgressStats(char* out,unsigned capacity){
    if(!out||!capacity)return 0;
    int n=std::snprintf(out,capacity,"progress enabled=%d playable=%d state=%d goal=%d source=%u baseline=%.1f remaining=%.1f pct=%d mask=%u samples=%u length_edges=%u location_queries=%u route_searches=%u nav_frame_calls=%u nav_frame_avg_us=%u nav_frame_peak_us=%u",
        progressEnabled?1:0,progressPlayable?1:0,int(navigator.state),navigator.selected,navigator.source,
        progress.baseline,navigator.routeLength,progress.percent,progress.reached,progress.samples,
        navigator.lengthVisits,navigator.locationQueries,navigator.searches,frameCost.count,frameCost.Average(),frameCost.peak);
    return n>0?unsigned(Limit(n,0,int(capacity)-1)):0;
}

bool RadarEnabled(){return supplyradar::options::values[0]!=0;}
int RadarGet(int index){return index>=0&&index<kRadarSettingCount?supplyradar::options::values[index]:0;}
const char* RadarKey(int index){return supplyradar::options::Key(index);}
int RadarMinimum(int index){return supplyradar::options::Minimum(index);}
int RadarMaximum(int index){return supplyradar::options::Maximum(index);}
void RadarSet(int index,int value){
    if(index<0||index>=kRadarSettingCount)return;int old=RadarGet(index);supplyradar::options::Set(index,value);if(old==RadarGet(index))return;
    if(index==supplyradar::options::Enable){
        if(!RadarEnabled()){supplyradar::ClearMap();supplyradar::ReleaseArtwork();supplyradar::mapPending=false;if(!Active())ResetSession();}
        else supplyradar::mapPending=true;
    }
    else if(index>=supplyradar::options::LayoutBase){supplyradar::EndEdit();supplyradar::SyncLayoutFromSettings();}
    else if(supplyradar::ScanSetting(index))supplyradar::ResetDisplay();
}
void RadarResetDefaults(){RadarSet(0,0);supplyradar::options::Reset();supplyradar::SyncLayoutFromSettings();supplyradar::ResetDisplay();}
bool RadarItemSupported(unsigned label){return supplyradar::options::Supported(label);}
const wchar_t* RadarItemName(unsigned label){return supplyradar::ItemName(label);}
void RadarBeginEdit(unsigned label){ // one user request per menu click; the refusal reason is logged so a silent no-op is visible
    // Not gated on `live`: it is false whenever the menu that issued this request is still open, and the
    // request is consumed in the same poll that closes the menu. The edit shows placeholder cards, so only
    // a configured radar in a running session is required.
    if(!RadarEnabled()||!inSession||!supplyradar::radarReady||!supplyradar::options::EnabledId(label)){
        Log("Supply radar edit refused: label=%u enabled=%d session=%d ready=%d cfg=%d live=%d",label,RadarEnabled()?1:0,inSession?1:0,supplyradar::radarReady?1:0,supplyradar::configureFail,supplyradar::live?1:0);return;}
    EndPositionEdit();supplyradar::BeginEdit(label);
    Log("Supply radar edit %s: label=%u",supplyradar::layout::active?"started":"not started",label);
}
bool RadarEditing(){return supplyradar::layout::active;}
void RadarEndEdit(){supplyradar::EndEdit();}
void RadarEditKey(unsigned key,bool down){supplyradar::EditKey(key,down);}
const wchar_t* RadarStatus(){
    if(!RadarEnabled())return UiText(UiId::RadarStatusOff,L"已关闭");
    if(!supplyradar::configured)return UiText(UiId::RadarStatusWaiting,L"等待有效玩家");
    if(!supplyradar::radarReady)return UiText(UiId::RadarStatusUnsupported,L"当前游戏版本不支持");
    if(!supplyradar::playerValid)return UiText(UiId::RadarStatusWaiting,L"等待有效玩家");
    if(supplyradar::phase)return UiText(UiId::RadarStatusScanning,L"扫描物资中");
    if(!supplyradar::totalVisible)return UiText(UiId::RadarStatusNoItems,L"附近无物资");
    return UiText(UiId::RadarStatusReady,L"已就绪");
}
unsigned RadarStats(char* out,unsigned capacity){return supplyradar::Stats(out,capacity);}

int Get(int index){return index>=0&&index<kSettingCount?((int*)&settings)[index]:0;}
const char* Key(int index){return index>=0&&index<kSettingCount?controls::specs[index].name:nullptr;}
int Minimum(int index){return index>=0&&index<kSettingCount?controls::specs[index].minimum:0;}
int Maximum(int index){return index>=0&&index<kSettingCount?controls::specs[index].maximum:0;}
void ResetDefaults(){settings=controls::Settings{};}
void Set(int index,int value){if(index<0||index>=kSettingCount)return;value=Limit(value,Minimum(index),Maximum(index));if(index==controls::RefreshIndex)value=10+5*((value-10+2)/5);int& slot=((int*)&settings)[index];if(slot==value)return;slot=value;
    // The map was loaded for the radar only; the minimap / world route now needs its NAV.
    if(navDeferred&&value&&inSession&&NavWanted()){Reload();return;}
    if(index==0&&!value){if(Active())ResetMinimapPresentation();else ResetSession();return;}
    if(index==8&&!value){if(!progressEnabled)navigator.Clear();worldOverlay.Clear();}
    if((index==8||index==controls::WorldIndex)&&!Active()){ResetSession();return;}
    if(index==9){if(navigator.prepared)navigator.Select(value);}
    else if(index!=controls::WorldHudIndex&&(index<controls::WorldArrowColorIndex||index>controls::WorldHudYIndex)){sceneValid=terrainValid=false;itemCursor=0;lastItemPoll=lastRoutePoll=lastFrame=0;}}
void Reload(){if(!Active())return;char name[240]{};CopyText(name,sizeof name,mapName);StartMap(name);}
void InvalidateFonts(){fontSize=supplyFontSize=worldFontSize=0;supplyradar::InvalidateFonts();}
void Shutdown(){ResetSession();supplyradar::ReleaseArtwork();supplyradar::UnloadFonts();supplyradar::options::values[0]=0;settings.enabled=0;ready=false;}
unsigned GoalCount(){return navigator.goalCount;}
const wchar_t* Status(){return !Active()?UiText(UiId::NavDisabled,L"已关闭"):!mesh.count?LocalizedLoadStatus():settings.navigation?minimap::NavigationStatus(navigator).text:UiText(UiId::NavReady,L"地图已就绪");}
bool Editing(){return mapEditor.active;}
bool MinimapEditing(){return mapEditor.active&&!worldHudEditing;}
bool WorldHudEditing(){return worldHudEditing;}
void BeginEdit(){if(settings.enabled&&inSession){mapEditor.Begin();}}
void BeginWorldHudEdit(){if(settings.navigation&&settings.worldNavigation&&settings.worldHud&&inSession){mapEditor.Begin();worldHudEditing=true;}}
void EndEdit(){EndPositionEdit();}
void EditKey(unsigned key,bool down){mapEditor.Key(key,down);}
bool ConsumeSettingsChanged(){bool changed=settingsChanged;settingsChanged=false;return changed;}
static bool pausedByUser=false;
void SetPaused(bool paused){pausedByUser=paused;}
void Pump(bool inGame,bool gameUiVisible,bool menuVisible){
    const int boundary=progressBoundary.exchange(0,std::memory_order_acq_rel);
    if(boundary){ResetProgress();progressRoundEnded=boundary==2;
        navigator.measureLength=progressEnabled&&!progressRoundEnded;navigator.routeSource=nav::Invalid;lastFrame=0;}
    if(!Active()){if(inSession||navFile||mesh.count)ResetSession();return;}
    menuShown=menuVisible;uiShown=gameUiVisible;
    if(!inGame){if(inSession)ResetSession();return;}
    if(pausedByUser){liveFrame=false;progressPlayable=false;return;}
    unsigned now=GetTickCount();
    if(!ready){
        if(lastDependencyAttempt&&now-lastDependencyAttempt<1000)return;
        lastDependencyAttempt=now;ready=Dependencies();if(!ready)return;
    }
    if(!inSession||now-lastMapCheck>=500){lastMapCheck=now;const char* name=VCall<const char*(__thiscall*)(void*)>(engine,52)(engine);char current[240]{};if(name){unsigned i=0;for(;i<sizeof current-1&&Readable(name+i,1)&&name[i];++i)current[i]=name[i]=='\\'?'/':name[i];if(i>=4&&Same(current+i-4,".bsp"))current[i-4]=0;}if(!inSession||!Same(current,mapName)){inSession=true;StartMap(current);}}
    if(navFile&&now-lastRead>=50){lastRead=now;ReadStep();}
    if(parser.phase){
        auto result=parser.Step();
        if(result==nav::ParseJob::Result::Ready||result==nav::ParseJob::Result::Failed){
            navStatus=result==nav::ParseJob::Result::Ready?L"通路":L"导航不兼容";
            loading.Clear();++meshSerial;navigator.Clear();floorTracker.Reset();
            sceneValid=terrainValid=false;lastFrame=0;
        }
    }
    BugStep();
    if((settings.navigation||progressEnabled)&&navigator.preparePhase){
        auto began=performance::Now();unsigned batches=0;
        do{navigator.PrepareStep(mesh,512);}while(navigator.preparePhase&&++batches<8&&performance::Microseconds(performance::Now()-began)<400);
    }
    if(RadarEnabled())supplyradar::Tick(now);
    const bool consoleVisible=MethodOK(engine,11)&&VCall<bool(__thiscall*)(void*)>(engine,11)(engine);
    if(gameUiVisible||consoleVisible){if(mapEditor.active)EndPositionEdit();liveFrame=false;progressPlayable=false;return;}
    if(menuShown&&mapEditor.active)EndPositionEdit();
    if(mapEditor.active&&(settings.enabled||worldHudEditing)){int w=0,h=0;VCall<void(__thiscall*)(void*,int&,int&)>(engine,5)(engine,w,h);const bool fastStep=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0;bool changed=false;
        if(w>=640&&h>=400&&w<=16384&&h<=16384){if(worldHudEditing){const int pixels=Limit(h/80,13,20);const WCHAR* preview=UiText(UiId::NavHud26,L"沿地面箭头前进");int chars=0;while(preview[chars]&&chars<30)++chars;const int boxW=Limit((chars+3)*pixels+20,210,w/2),boxH=pixels+20;changed=mapEditor.StepPosition(now,w,h,boxW,boxH,settings.worldHudX,settings.worldHudY,fastStep);}
            else changed=mapEditor.Step(now,w,h,settings.size,settings.posX,settings.posY,fastStep);}
        if(changed){if(!worldHudEditing)sceneValid=terrainValid=false;settingsChanged=true;}}
    const bool presentation=settings.enabled||(settings.navigation&&settings.worldNavigation);
    if((presentation||(progressEnabled&&!progressRoundEnded))&&now-lastFrame>=unsigned(presentation?(1000+settings.refreshFps-1)/settings.refreshFps:500)){lastFrame=now;UpdateFrame();}
}
void Paint(bool showMap,bool showWorld,bool showRadar){showMap=showMap&&settings.enabled;showWorld=(showWorld||worldHudEditing)&&settings.navigation&&settings.worldNavigation;if(Active()&&liveFrame&&ready&&!uiShown&&(showMap||showWorld))Draw(showMap,showWorld);if(showRadar&&RadarEnabled()&&supplyradar::live&&ready&&!uiShown)supplyradar::Draw();}
bool DrawSupplyIcon(unsigned label,int x,int y,int w,int h){
    if(!surface){
        surface=GetInterface(L"vguimatsurface.dll","VGUI_Surface031");
        if(!MethodOK(surface,11)||!MethodOK(surface,29)||!MethodOK(surface,31)||!MethodOK(surface,34)){surface=nullptr;return false;}
    }
    return supplyradar::DrawExternalIcon(label,x,y,w,h);
}
bool SupplyIconSize(unsigned label,int screenW,int screenH,float hudScalePercent,int* outW,int* outH){
    if(!outW||!outH)return false;
    int w=0,h=0;
    if(!supplyradar::ExternalIconSize(label,screenW,screenH,hudScalePercent,w,h))return false;
    *outW=w;*outH=h;return true;
}
}

