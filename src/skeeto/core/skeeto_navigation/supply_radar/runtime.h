#pragma once
// SupplyRadar 1.3.6 / Build 132 adaptation (RagDoll, authorized). Skeeto owns the
// host: this header runs on Skeeto's game/Paint thread only (Pump -> Tick at most
// every 25 ms, Paint -> Draw once per frame). There is no NAV, Trace or route work:
// distance is the straight line plus a height hint, as in the upstream design.
#include "offline_counters.h"
#include "settings.h"
#include "radar_abi.h"
#include "../performance.h"
namespace supplyradar {
#include "radar_math.h"
#include "items.h"
namespace options {
inline bool Enabled(items::Label label){return EnabledId(unsigned(label));}
// The upstream font name is a user setting; Skeeto always uses the packaged font.
constexpr const char* fontName="20HUD_Torus_SemiBold";
}
static_assert(options::LabelCount==unsigned(items::Label::Count),"Radar label count mismatch");
static_assert(options::LabelCount==unsigned(skeeto_nav::kRadarLabelCount)&&options::ItemBase==skeeto_nav::kRadarItemBase&&
              options::LayoutBase==skeeto_nav::kRadarLayoutBase&&options::Count==skeeto_nav::kRadarSettingCount,"skeeto_navigation.h radar constants out of sync");
#include "layout.h"
#include "budget_clock.h"
#include "world_origin.h"
#include "camera_origin.h"
#include "surface_abi.h"
#include "entity_identity_abi.h"
#include "direction_surface.h"
#include "direction_math.h"
#include "rectangular_hud.h"
#include "b4b_hud_layout.h"
#include "classic_hud_style.h"
#include "classic_hud_artwork.h"
#include "special_supply_artwork.h"
#include "direction_arrow_artwork.h"
#include "classic_text_alignment.h"
#include "hud_numeric_alignment.h"
#include "raster_filter.h"
#include "rounded_panel.h"
#include "distance_presentation.h"
#include "map_supply.h"
#include "map_supply_parser.h"
#include "hud_digit_font.h"
#include "hud_numeric_font.h"
#include "hud_heart_font.h"
#include "hud_cat_font.h"

using radar_nav::Distance;
// ---- host state -----------------------------------------------------------
static bool radarReady,configured,entitySerialReady,parentHandleReady,modelLookupReady,directionSurfaceReady;
static char radarMap[240];
static map_supply::Map supplyHints;static Blob supplyBytes;static void* supplyFile;static unsigned supplyRead;
// live: the HUD may be drawn this frame (false while any menu/console is open); playerValid: the last evaluation found a
// valid survivor/spectator source and is kept while Tick pauses for a menu (used for the status line and edit start).
static bool live,playerValid,mapPending;static unsigned lastTick;
static unsigned font,fontBank[64],uiFontBank[64],radarFontBank[3][64];
static constexpr unsigned IconCount=unsigned(items::Label::Count),ArrowTexture=IconCount*2,TextureCount=ArrowTexture+1;
static unsigned char *pixels,*decoding;static unsigned decodeInput,decodeOutput;static bool artFailed;
static int textures[TextureCount],textureWidths[TextureCount],textureHeights[TextureCount];
// Scan-HUD icons must not share textures[] with the radar. Different pixel sizes
// were Destroy+CreateNewTextureID every frame and overflowed the surface RBTree.
static int externalTextures[IconCount],externalWidths[IconCount],externalHeights[IconCount];
static unsigned char resizedPixels[512*256*4];static unsigned uploadsLeft;
// The panel artwork is identical for all items. Bound the shared size cache
// by the number of item kinds, so every visible custom size can remain warm.
struct PanelTexture {int texture=0,width=0,height=0;unsigned used=0,checked=0;};
static PanelTexture panelTextures[IconCount];static unsigned renderFrame;
static int frameArrowTexture;static bool frameArrowPrepared,renderStateActive,textureDirty=true;
static int lastScreenW,lastScreenH;
// Counters: plain game-thread increments, reported by Stats().
L4N_COUNTERS(static unsigned textureUploads,fontCreates,fontResets,rowTextBuilds;)
static performance::Metric workCost,drawCost;
static Vec viewOrigin;
static direction_math::Basis directionBasis;
static direction_math::State directionStates[IconCount];
static void ResetDirection(){directionBasis.Reset();for(auto& state:directionStates)state.Reset();}
static distance_presentation::State displayedDistances[IconCount];

// Protected string reads: one guarded copy, no page query.
static bool SafeSame(const char* a,const char* trusted){char copy[160];return skmem::CopyCStr(copy,sizeof copy,a)&&Same(copy,trusted);}
static bool CopySafe(char* to,unsigned cap,const char* from){return skmem::CopyCStr(to,cap,from);}

// ---- RecvTable offsets (cold: cached per table/name) -------------------------
struct Property {RecvTable* table;const char* name;int offset;};static Property properties[1024];static unsigned propertyCount;
static int FindRecvOffset(RecvTable* table,const char* name,unsigned depth=0){
    if(depth>16||!Readable(table,sizeof *table)||table->count<1||table->count>4096||!Readable(table->props,table->count*sizeof(RecvProp)))return -1;
    for(int i=0;i<table->count;++i){auto& p=table->props[i];if(p.name&&SafeSame(p.name,name))return p.offset;}
    for(int i=0;i<table->count;++i){auto& p=table->props[i];if(p.type==6&&p.table){int o=FindRecvOffset(p.table,name,depth+1);if(o>=0&&p.offset>=0&&p.offset<65536-o)return o+p.offset;}}
    return -1;
}
static int TableOffset(RecvTable* table,const char* name){
    for(unsigned i=0;i<propertyCount;++i)if(properties[i].table==table&&Same(properties[i].name,name))return properties[i].offset;
    int off=FindRecvOffset(table,name);if(propertyCount<1024)properties[propertyCount++]={table,name,off};return off;
}
static int ClassOffset(ClientClass* cls,const char* name){return Readable(cls,sizeof *cls)?TableOffset(cls->table,name):-1;}
template<class T>static bool Read(void* entity,ClientClass* cls,const char* name,T& value){
    int off=ClassOffset(cls,name);if(!entity||off<0||unsigned(off)>65536-sizeof(T))return false;return EntRead(entity,off,&value);
}
static void* Entity(int index){return VCall<void*(__thiscall*)(void*,int)>(entities,3)(entities,index);}
static ClientClass* Class(int index,bool& dormant){
    void* net=VCall<void*(__thiscall*)(void*,int)>(entities,0)(entities,index);
    if(!Readable(net,sizeof(void*)))return nullptr;
    auto methods=*(void***)net;
    // One checked span covers both slots; never cache a network object's vtable.
    if(!Readable(methods,8*sizeof(void*)))return nullptr;
    auto classMethod=methods[1],dormantMethod=methods[7];
    if(!Executable(classMethod)||!Executable(dormantMethod))return nullptr;
    dormant=((bool(__thiscall*)(void*))dormantMethod)(net);if(dormant)return nullptr;
    auto cls=((ClientClass*(__thiscall*)(void*))classMethod)(net);
    return Readable(cls,sizeof *cls)?cls:nullptr;
}

// ---- radar source (local survivor, observed survivor or free camera) -------------
struct RadarSource {Vec position{};void* object=nullptr;int index=0;bool spectating=false,roaming=false;};
static RadarSource activeSource;static unsigned activeSourceAt;
static bool ValidSurvivor(int index,RadarSource& result,bool spectating){
    if(index<1||index>64)return false;
    bool dormant=false;auto cls=Class(index,dormant);void* entity=Entity(index);
    int team=0;unsigned char life=1;Vec p{};
    if(!cls||dormant||!entity||!Read(entity,cls,"m_iTeamNum",team)||team!=2||
       !Read(entity,cls,"m_lifeState",life)||life||
       !WorldPosition(entity,p)||!radar_nav::Position(p))return false;
    result={p,entity,index,spectating};return true;
}
static bool Player(RadarSource& result){
    int index=VCall<int(__thiscall*)(void*)>(engine,12)(engine);
    if(index<1||index>64)return false;
    if(ValidSurvivor(index,result,false))return true;
    if(!options::values[options::Spectator])return false;
    bool dormant=false;auto cls=Class(index,dormant);void* local=Entity(index);
    if(!cls||dormant||!local)return false;
    int mode=0;unsigned target=~0u;
    if(!Read(local,cls,"m_iObserverMode",mode))return false;
    if(mode==6){int team=0;unsigned char life=0;Vec camera{};
        if(!Read(local,cls,"m_iTeamNum",team)||!Read(local,cls,"m_lifeState",life)||
           !(team==1||(team==2&&life!=0))||!camera_origin::Read(engine,camera))return false;
        result={camera,local,index,true,true};return true;
    }
    // Source OBS_MODE_IN_EYE (4), OBS_MODE_CHASE (5). Both have a
    // survivor origin. Free roaming (6) uses the view above.
    if(!parentHandleReady||(mode!=4&&mode!=5)||
       !Read(local,cls,"m_hObserverTarget",target)||target==~0u)return false;
    int targetIndex=int(target&2047u);
    if(targetIndex<1||targetIndex>64)return false;
    void* resolved=VCall<void*(__thiscall*)(void*,unsigned)>(entities,4)(entities,target);
    if(!resolved||Entity(targetIndex)!=resolved)return false; // serial/identity
    return ValidSurvivor(targetIndex,result,true)&&result.object==resolved;
}

// ---- item identity ------------------------------------------------------------------
static const void* LookupModel(short index){return modelLookupReady&&items::ValidModel(index)?VCall<const void*(__thiscall*)(void*,int)>(modelInfo,1)(modelInfo,index):nullptr;}
static bool LaserModelName(short index,char* copy,unsigned cap){auto model=LookupModel(index);return model&&CopySafe(copy,cap,VCall<const char*(__thiscall*)(void*,const void*)>(modelInfo,3)(modelInfo,model));}
static bool IsLaser(short index){char copy[384];return LaserModelName(index,copy,sizeof copy)&&items::LaserModel(copy);}
static bool MapIdentity(void* e,ClientClass* cls,unsigned parent,Vec& networkOrigin,char* parentModel){
    if(!options::values[options::MapMetadata]||!supplyHints.loaded||!Read(e,cls,"m_vecOrigin",networkOrigin))return false;parentModel[0]=0;
    if(parent&&parent!=~0u){
        unsigned char attachment=255;if(!parentHandleReady||!modelLookupReady||!Read(e,cls,"m_iParentAttachment",attachment)||attachment)return false;
        void* object=VCall<void*(__thiscall*)(void*,unsigned)>(entities,4)(entities,parent);unsigned index=parent&4095u;if(!object||!index||index>=2048||Entity(int(index))!=object)return false;
        bool dormant=false;auto pc=Class(int(index),dormant);unsigned grandparent=~0u;short model=0;
        if(!pc||dormant||!Read(object,pc,"moveparent",grandparent)||(grandparent&&grandparent!=~0u)||!Read(object,pc,"m_nModelIndex",model)||!items::ValidModel(model))return false;
        auto m=LookupModel(model);if(!m)return false;auto name=VCall<const char*(__thiscall*)(void*,const void*)>(modelInfo,3)(modelInfo,m);if(!CopySafe(parentModel,32,name)||!map_supply::Brush(parentModel))return false;
    }
    return true;
}
static map_supply::Hint MapHint(void* e,ClientClass* cls,items::Label label,unsigned parent,bool displayOnly=false){Vec p{};char model[32];return MapIdentity(e,cls,parent,p,model)?supplyHints.Find({unsigned(label),p,model},displayOnly):map_supply::Unknown;}
static items::Label MapDisplayLabel(void* e,ClientClass* cls,unsigned parent){Vec p{};char model[32];return MapIdentity(e,cls,parent,p,model)?items::Label(supplyHints.DisplayLabel({0,p,model})):items::Label::None;}
static bool MapUnlimited(void* e,ClientClass* cls,items::Label label,bool spawn,unsigned parent){return spawn&&MapHint(e,cls,label,parent)==map_supply::Unlimited;}
struct Candidate {int index;void* object;ClientClass* cls;items::Label label;Vec position;float direct;bool unlimited,valid;unsigned serialKey;};
#include "scan_inspect.h"
#include "lite_scan.h"

// ---- drawing primitives: direct Surface031 calls (Skeeto brackets Paint with fill flush/colour invalidate) ----
static void Color(int r,int g,int b,int a=255){VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,11)(surface,r,g,b,a);}
static void Rect(int x,int y,int w,int h,int r,int g,int b,int a=220){Color(r,g,b,a);VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,12)(surface,x,y,x+w,y+h);}
static void Text(int x,int y,const WCHAR* text,int r=235,int g=237,int b=240){
    if(!font)return;
    VCall<void(__thiscall*)(void*,unsigned)>(surface,17)(surface,font);VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,19)(surface,r,g,b,255);
    VCall<void(__thiscall*)(void*,int,int)>(surface,20)(surface,x,y);int len=0;while(text[len])++len;
    VCall<void(__thiscall*)(void*,const WCHAR*,int,int)>(surface,22)(surface,text,len,0);
}
static bool TextureLive(int id){return id&&VCall<bool(__thiscall*)(void*,int)>(surface,32)(surface,id);}
static void ReleaseTexture(int& id){
    if(!id)return;
    // Always destroy when we still hold an id. Skipping DestroyTextureID because
    // IsTextureIDValid flickered left orphaned nodes in the surface CUtlRBTree.
    if(MethodOK(surface,33))VCall<bool(__thiscall*)(void*,int)>(surface,33)(surface,id);
    id=0;
}
static int CachedTexture(unsigned label,const unsigned char* source,int sw,int sh,int w,int h,bool panel=false){
    if(!source||label>=TextureCount||w<1||h<1)return 0;
    int& t=textures[label];
    bool valid=t!=0&&TextureLive(t);
    if(!valid){t=0;textureWidths[label]=textureHeights[label]=0;}
    // First upload wins. Re-uploading on HUD scale change crashes Surface/tier0
    // (same class as supply-scan '='). DrawTexturedRect stretches to the box.
    if(valid)return t;
    if(!uploadsLeft)return 0;
    int tw=Limit(w,1,512),th=Limit(h,1,256);
    t=VCall<int(__thiscall*)(void*,bool)>(surface,34)(surface,true);
    if(!t)return 0;
    if(!raster_filter::Resize(source,sw,sh,resizedPixels,tw,th)){ReleaseTexture(t);return 0;}
    if(panel)rounded_panel::Apply(resizedPixels,tw,th);
    if(!SkeetoNavUploadRgba(t,resizedPixels,tw,th)){ReleaseTexture(t);return 0;}
    --uploadsLeft;L4N_OBSERVE(++textureUploads);textureWidths[label]=tw;textureHeights[label]=th;
    return t;
}
static int CachedPanelTexture(int w,int h){
    if(!pixels||w<1||h<1)return 0;
    // Prefer any already-uploaded panel texture and stretch. Size-keyed reupload
    // on radar scale edits hit the same Surface path as icon reupload crashes.
    PanelTexture* chosen=nullptr;
    for(auto& entry:panelTextures){
        if(entry.texture&&TextureLive(entry.texture)&&entry.width>0&&entry.height>0){
            if(!chosen||entry.used>chosen->used)chosen=&entry;
        }
    }
    if(chosen){chosen->used=renderFrame;chosen->checked=renderFrame;return chosen->texture;}
    if(!uploadsLeft)return 0;
    for(auto& entry:panelTextures)if(!chosen||!entry.texture||entry.used<chosen->used){chosen=&entry;if(!entry.texture)break;}
    if(!chosen)return 0;
    auto& entry=*chosen;entry.used=renderFrame;entry.checked=renderFrame;
    if(entry.texture&&!TextureLive(entry.texture)){entry.texture=0;entry.width=entry.height=0;}
    if(!entry.texture){
        entry.texture=VCall<int(__thiscall*)(void*,bool)>(surface,34)(surface,true);if(!entry.texture)return 0;
    }
    int tw=Limit(w,1,512),th=Limit(h,1,256);
    auto source=classic_hud_artwork::meta[0];
    if(!raster_filter::Resize(pixels,source.sourceWidth,source.sourceHeight,resizedPixels,tw,th))return 0;
    rounded_panel::Apply(resizedPixels,tw,th);
    if(!SkeetoNavUploadRgba(entry.texture,resizedPixels,tw,th)){ReleaseTexture(entry.texture);entry.width=entry.height=0;return 0;}
    --uploadsLeft;L4N_OBSERVE(++textureUploads);entry.width=tw;entry.height=th;
    return entry.texture;
}
static classic_hud_artwork::Meta ArtworkMeta(unsigned id){
    return items::IsSpecial(id)?special_supply_artwork::meta[id-items::SpecialFirst]:classic_hud_artwork::meta[id];
}
static const unsigned char* ArtworkPixels(unsigned id){
    return items::IsSpecial(id)?special_supply_artwork::rgba[id-items::SpecialFirst]:pixels+id*classic_hud_artwork::Stride;
}
static void Icon(unsigned label,int x,int y,int w,int h,int red=255,int green=255,int blue=255,int alpha=255){
    if(label>=ArrowTexture||w<1||h<1)return;
    bool panel=label>=IconCount;unsigned art=panel?0:label;
    if((panel||!items::IsSpecial(art))&&!pixels)return;
    auto m=ArtworkMeta(art);
    const unsigned char* source=panel?pixels:ArtworkPixels(art);
    if(!source)return;
    int texture=panel?CachedPanelTexture(w,h):CachedTexture(label,source,m.sourceWidth,m.sourceHeight,w,h);if(!texture)return;
    Color(red,green,blue,alpha);VCall<void(__thiscall*)(void*,int)>(surface,29)(surface,texture);VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,31)(surface,x,y,x+w,y+h);
}
static_assert(classic_hud_artwork::Count==items::SpecialFirst&&special_supply_artwork::Count==items::SpecialCount,"HUD artwork mapping mismatch");
static void DirectionArrow(layout::Box card,float unit,const direction_math::Result& result,classic_hud_artwork::Meta color){
    if(!directionSurfaceReady||!result.horizontal)return;
    SurfaceVertex vertices[4];if(!rectangular_hud::ArrowQuad(card,b4b_hud::ArrowGap*unit,classic_hud::ArrowHalf*unit,result,vertices))return;
    if(!frameArrowPrepared){frameArrowTexture=CachedTexture(ArrowTexture,direction_arrow_artwork::rgba,32,32,32,32);frameArrowPrepared=true;}
    int texture=frameArrowTexture;if(!texture)return;
    Color(color.red,color.green,color.blue);VCall<void(__thiscall*)(void*,int)>(surface,29)(surface,texture);DrawDirectionQuad(surface,vertices);
}

// ---- text ---------------------------------------------------------------------------------------
static const WCHAR* ItemName(unsigned id){
    static const UiTextList<39> names{{
        {UiId::RadarItem10,L"未知武器"}, // index 0 is never shown
        {UiId::RadarItem1,L"医疗包"},
        {UiId::RadarItem2,L"电击器"},
        {UiId::RadarItem3,L"止痛药"},
        {UiId::RadarItem4,L"肾上腺素"},
        {UiId::RadarItem5,L"燃烧瓶"},
        {UiId::RadarItem6,L"土制炸弹"},
        {UiId::RadarItem7,L"胆汁"},
        {UiId::RadarItem8,L"弹药堆"},
        {UiId::RadarItem9,L"镭射"},
        {UiId::RadarItem10,L"未知武器"},
        {UiId::RadarItem11,L"AK47"},
        {UiId::RadarItem12,L"M16"},
        {UiId::RadarItem13,L"军用狙击枪"},
        {UiId::RadarItem14,L"猎枪"},
        {UiId::RadarItem15,L"消音冲锋枪"},
        {UiId::RadarItem16,L"UZI"},
        {UiId::RadarItem17,L"AWP"},
        {UiId::RadarItem18,L"Scout"},
        {UiId::RadarItem19,L"SCAR"},
        {UiId::RadarItem20,L"榴弹发射器"},
        {UiId::RadarItem21,L"M60"},
        {UiId::RadarItem22,L"手枪"},
        {UiId::RadarItem23,L"沙鹰"},
        {UiId::RadarItem24,L"木喷"},
        {UiId::RadarItem25,L"铁喷"},
        {UiId::RadarItem26,L"连喷"},
        {UiId::RadarItem27,L"SPAS"},
        {UiId::RadarItem28,L"MP5"},
        {UiId::RadarItem29,L"SG552"},
        {UiId::RadarItem30,L"近战"},
        {UiId::RadarItem31,L"电锯"},
        {UiId::RadarItem32,L"油桶"},
        {UiId::RadarItem33,L"烟花盒"},
        {UiId::RadarItem34,L"终点安全屋"},
        {UiId::RadarItem35,L"可乐"},
        {UiId::RadarItem36,L"侏儒"},
        {UiId::RadarItem37,L"高爆弹"},
        {UiId::RadarItem38,L"燃烧弹"},
    }};return id<39?names[id]:L"";
}
static const char* DigitFamily(){static const char* names[]={"L4N_Radar_Digits","20HUD_Kudo","20HUD_MeowMoe"};return names[options::DigitStyle()];}
static void UseFont(int pixels,bool ui=false,bool radarNumber=false){
    pixels=Limit(pixels,7,48);auto& handle=ui?uiFontBank[pixels]:radarNumber?radarFontBank[options::DigitStyle()][pixels]:fontBank[pixels];
    if(!handle){
        L4N_OBSERVE(++fontCreates);handle=VCall<unsigned(__thiscall*)(void*)>(surface,63)(surface);
        if(handle){
            auto set=VCall<bool(__thiscall*)(void*,unsigned,const char*,int,int,int,int,int,int,int)>(surface,64);
            const char* name=ui?"Microsoft YaHei":radarNumber?DigitFamily():options::fontName;
            int weight=ui?700:Same(name,"20HUD_Torus_SemiBold")?600:400;int flags=ui?0x10:0x210;
            if(!set(surface,handle,name,pixels,weight,0,0,flags,0,0xffff))set(surface,handle,ui?"Microsoft YaHei":"Arial",pixels,ui?700:400,0,0,flags,0,0xffff);
        }
    }
    font=handle;
}
static const unsigned hudOrder[]={26,27,15,16,28,11,12,19,29,14,13,17,18,20,21,24,25,1,2,3,4,5,6,7,8,9,32,33,34,35,36,37,38};
struct TextExtent {int w=0,h=0;};
struct TextMetric {unsigned font=0,length=0;WCHAR text[16]{};TextExtent size;};
static TextMetric textMetrics[128];
struct FittedText {WCHAR text[16]{};TextExtent size{};unsigned handle=0;int pixels=0,inkLeft=0,inkTop=0,inkWidth=0,inkHeight=0;};
struct RowTextCache {
    bool valid=false,unlimited=false,showCount=false,showHeight=false;
    int meters=0,height=0,high=0,width=0,tall=0,countWidth=0,heightWidth=0,distancePixels=0,sidePixels=0;
    unsigned count=0;int drawCountWidth=0,drawHeightWidth=0,cardWidth=0,cardHeight=0;FittedText distance,quantity,elevation;
};
static RowTextCache rowTextCaches[3][IconCount];
static TextExtent MeasureText(const WCHAR* text){
    TextExtent size{};if(!font)return size;
    unsigned length=0,hash=font*2654435761u;
    for(;length<15&&text[length];++length)hash=(hash^unsigned(text[length]))*16777619u;
    bool cacheable=!text[length];auto& cached=textMetrics[hash&127u];
    if(cacheable&&cached.font==font&&cached.length==length){unsigned i=0;for(;i<length&&cached.text[i]==text[i];++i){}if(i==length)return cached.size;}
    VCall<void(__thiscall*)(void*,unsigned,const WCHAR*,int&,int&)>(surface,72)(surface,font,text,size.w,size.h);
    size.w=Limit(size.w,0,4096);size.h=Limit(size.h,0,128);
    if(cacheable){cached.font=font;cached.length=length;cached.size=size;for(unsigned i=0;i<=length;++i)cached.text[i]=text[i];}
    return size;
}
static void FitTextLine(FittedText& text,int pixels,int width,bool radarNumber=false){
    pixels=radarNumber?hud_numeric_text::PreferredPixels(options::DigitStyle(),text.text,pixels):Limit(pixels,7,48);UseFont(pixels,false,radarNumber);auto size=MeasureText(text.text);
    while(size.w>width&&pixels>7){UseFont(--pixels,false,radarNumber);size=MeasureText(text.text);}
    text.size=size;text.handle=font;text.pixels=pixels;
    text.inkLeft=text.inkTop=0;text.inkWidth=size.w;text.inkHeight=size.h;
    if(!radarNumber&&Same(options::fontName,"20HUD_Torus_SemiBold"))classic_text::Bounds(text.text,pixels,size.w,size.h,text.inkLeft,text.inkTop,text.inkWidth,text.inkHeight);
    else if(radarNumber)hud_numeric_text::Bounds(options::DigitStyle(),text.text,pixels,size.w,size.h,text.inkLeft,text.inkTop,text.inkWidth,text.inkHeight);
    // The one-pixel native outline is symmetric around the visible fill.
    --text.inkLeft;--text.inkTop;text.inkWidth+=2;text.inkHeight+=2;
}
static RowTextCache& PrepareRowText(unsigned id,int meters,unsigned count,bool unlimited,int height,int high,int wide,int tall,int countWidth,int heightWidth,int distancePixels,int sidePixels){
    auto& cache=rowTextCaches[options::DigitStyle()][id];bool showCount=id!=34&&options::values[options::ShowCount]!=0,showHeight=options::values[options::Height]!=0&&height!=0;
    if(!showCount){count=0;unlimited=false;}if(!showHeight){height=0;high=0;}
    if(cache.valid&&cache.meters==meters&&cache.count==count&&cache.unlimited==unlimited&&cache.height==height&&cache.high==high&&cache.width==wide&&cache.tall==tall&&cache.countWidth==countWidth&&cache.heightWidth==heightWidth&&cache.distancePixels==distancePixels&&cache.sidePixels==sidePixels&&cache.showCount==showCount&&cache.showHeight==showHeight)return cache;
    L4N_OBSERVE(++rowTextBuilds);cache={};cache.valid=true;cache.meters=meters;cache.count=count;cache.unlimited=unlimited;cache.height=height;cache.high=high;cache.width=wide;cache.tall=tall;cache.countWidth=countWidth;cache.heightWidth=heightWidth;cache.distancePixels=distancePixels;cache.sidePixels=sidePixels;cache.showCount=showCount;cache.showHeight=showHeight;
    wsprintfW(cache.distance.text,L"%d",meters);FitTextLine(cache.distance,distancePixels,wide-countWidth-heightWidth,true);
    if(showCount){if(unlimited)WideCopy(cache.quantity.text,16,L"INF");else wsprintfW(cache.quantity.text,L"%ux",count);FitTextLine(cache.quantity,sidePixels,countWidth-2);}
    if(showHeight){wsprintfW(cache.elevation.text,height>0?L"+%dm":L"-%dm",high);FitTextLine(cache.elevation,sidePixels,heightWidth-2);}
    // Preserve normal positions. Only a side string that cannot fit even at
    // seven pixels receives more room; never truncate counts or signed height.
    cache.drawCountWidth=countWidth;cache.drawHeightWidth=heightWidth;
    if(showCount&&cache.quantity.size.w+2>countWidth)cache.drawCountWidth=cache.quantity.size.w+2;
    if(showHeight&&cache.elevation.size.w+2>heightWidth)cache.drawHeightWidth=cache.elevation.size.w+2;
    // The icon and distance share the card centre even when the two side
    // strings differ in length or only one side is visible.
    int side=cache.drawCountWidth>cache.drawHeightWidth?cache.drawCountWidth:cache.drawHeightWidth;
    cache.drawCountWidth=cache.drawHeightWidth=side;
    int center=wide-cache.drawCountWidth-cache.drawHeightWidth;
    if(cache.distance.size.w>center)FitTextLine(cache.distance,distancePixels,center,true);
    int minimum=cache.drawCountWidth+cache.drawHeightWidth+cache.distance.size.w;
    cache.cardWidth=wide>minimum?wide:minimum;
    // Only visible glyph ink can overflow the card. Decorative fonts may have
    // generous blank ascender space that must not enlarge the panel or grid.
    int textHeight=cache.distance.inkHeight;
    if(showCount&&cache.quantity.inkHeight>textHeight)textHeight=cache.quantity.inkHeight;
    if(showHeight&&cache.elevation.inkHeight>textHeight)textHeight=cache.elevation.inkHeight;
    cache.cardHeight=tall>textHeight?tall:textHeight;
    cache.valid=cache.distance.handle&&(!showCount||cache.quantity.handle)&&(!showHeight||cache.elevation.handle);
    return cache;
}
static void DrawTextLine(int x,int y,const FittedText& text,int r=255,int g=255,int b=255){font=text.handle;Text(x,y,text.text,r,g,b);}
static void DrawCenteredText(layout::Box area,const FittedText& text,int r,int g,int b,int adjustX=0,int adjustY=0){
    DrawTextLine(area.x+(area.w-text.inkWidth)/2-text.inkLeft+adjustX,area.y+(area.h-text.inkHeight)/2-text.inkTop+adjustY,text,r,g,b);
}
static void ResetFonts(){
    L4N_OBSERVE(++fontResets);font=0;memset(fontBank,0,sizeof fontBank);memset(radarFontBank,0,sizeof radarFontBank);memset(uiFontBank,0,sizeof uiFontBank);
    memset(textMetrics,0,sizeof textMetrics);memset(rowTextCaches,0,sizeof rowTextCaches);
}
static void ResetRenderPresentation(){
    if(!renderStateActive)return;
    for(auto& distance:displayedDistances)distance={};ResetDirection();
    memset(layout::visualBoxes,0,sizeof layout::visualBoxes);renderStateActive=false;
}

// ---- layout editor overlay -------------------------------------------------------------------------------
// Editor instructions use screen scale, independently of individual radar sizes.
static void EditorPanel(int x,int y,int width,int height,int radius){
    radius=Limit(radius,1,height/2);
    Rect(x,y+radius,width,height-2*radius,19,31,36,248);
    for(int row=0;row<radius;++row){
        int inset=0,vertical=radius-row-1;
        while(inset<radius&&(radius-inset)*(radius-inset)+vertical*vertical>radius*radius)++inset;
        Rect(x+inset,y+row,width-2*inset,1,19,31,36,248);
        Rect(x+inset,y+height-row-1,width-2*inset,1,19,31,36,248);
    }
}
static void EditorFooter(int width,int height){
    float scale=float(height)/720.f,fit=float(width)/1280.f;if(scale>fit)scale=fit;
    if(scale<.8f)scale=.8f;if(scale>2.f)scale=2.f;
    int panelWidth=Limit(int(640*scale+.5f),400,width-32),pad=int(18*scale+.5f),gap=int(10*scale+.5f);
    int textWidth=panelWidth-2*pad,headingPixels=Limit(int(24*scale+.5f),20,48),bodyPixels=Limit(int(18*scale+.5f),16,36);
    WCHAR title[80];swprintf_s(title,UiText(UiId::RadarEditTitle,L"位置调节   %s   %d%%"),ItemName(layout::selected),layout::items[layout::selected].scale);
    UseFont(headingPixels,true);auto heading=MeasureText(title);
    while(heading.w>textWidth&&headingPixels>18){UseFont(--headingPixels,true);heading=MeasureText(title);}
    const WCHAR* lines[]={UiText(UiId::RadarEditHint1,L"方向键微调并吸附  Shift加速  Ctrl不吸附  +/- 大小"),UiText(UiId::RadarEditHint2,L"即时生效并自动保存  回车/Esc退出  R复位  Tab下一个")};
    int bodyHeight=0;
    for(;;){UseFont(bodyPixels,true);bool fits=true;bodyHeight=0;for(auto line:lines){auto size=MeasureText(line);if(size.w>textWidth)fits=false;if(size.h>bodyHeight)bodyHeight=size.h;}if(fits||bodyPixels<=14)break;--bodyPixels;}
    int lineHeight=bodyHeight+int(10*scale+.5f),panelHeight=2*pad+heading.h+gap+2*lineHeight;
    int x=(width-panelWidth)/2,y=height-panelHeight-int(16*scale+.5f);
    auto selected=layout::visualBoxes[layout::selected];if(selected.w&&selected.h&&x<selected.x+selected.w+2&&x+panelWidth>selected.x-2&&y<selected.y+selected.h+2&&y+panelHeight>selected.y-2)y=int(16*scale+.5f);
    EditorPanel(x,y,panelWidth,panelHeight,Limit(int(10*scale+.5f),8,20));
    UseFont(headingPixels,true);Text(x+pad,y+pad,title,169,244,214);
    UseFont(bodyPixels,true);for(unsigned i=0;i<2;++i)Text(x+pad,y+pad+heading.h+gap+int(i)*lineHeight,lines[i],i==1?174:239,i==1?235:243,i==1?211:245);
}
static void DrawAdjustmentFrame(layout::Box box){
    Rect(box.x,box.y,box.w,1,79,219,166,255);Rect(box.x,box.y+box.h-1,box.w,1,79,219,166,255);
    Rect(box.x,box.y,1,box.h,79,219,166,255);Rect(box.x+box.w-1,box.y,1,box.h,79,219,166,255);
}
// Thin alignment lines between the selected card plate and the peer it snapped to.
static int Smaller(int a,int b){return a<b?a:b;}
static int Larger(int a,int b){return a>b?a:b;}
static void EditorGuides(int w,int h){
    const auto selected=layout::panelBoxes[layout::selected];
    if(layout::GuideVisible(layout::guideX,true)){
        const auto peer=layout::panelBoxes[layout::guideX.peer];
        int top=Limit(Smaller(selected.y,peer.y)-6,0,h-1),bottom=Limit(Larger(selected.y+selected.h,peer.y+peer.h)+6,top+1,h);
        Rect(Limit(layout::guideX.position,0,w-1),top,1,bottom-top,100,210,240,210);
    }
    if(layout::GuideVisible(layout::guideY,false)){
        const auto peer=layout::panelBoxes[layout::guideY.peer];
        int left=Limit(Smaller(selected.x,peer.x)-6,0,w-1),right=Limit(Larger(selected.x+selected.w,peer.x+peer.w)+6,left+1,w);
        Rect(left,Limit(layout::guideY.position,0,h-1),right-left,1,100,210,240,210);
    }
}

// ---- display values -----------------------------------------------------------------------------------
static unsigned DisplayCount(const Row& r){return layout::active?1:r.unlimited?99:r.count;}
static unsigned DisplayKey(unsigned id){
    auto& row=rows[id];unsigned key=id;
    if(row.nearest&&row.nearest<=publishedCount){const auto& c=published[row.nearest-1];key=c.serialKey?c.serialKey:unsigned(c.object)^(unsigned(c.index)<<2);}
    return key;
}
static int DisplayDistance(unsigned id,Vec& point,unsigned now){
    point=rows[id].point;return displayedDistances[id].Update(Distance(viewOrigin,point)/39.3701f,DisplayKey(id),now);
}
// One Paint-thread frame: bounded by the fixed item labels; all expensive text
// work is cached in rowTextCaches and rebuilt only when a value changes.
static void DrawCards(){
    int w=0,h=0;VCall<void(__thiscall*)(void*,int&,int&)>(engine,5)(engine,w,h);if(w<640||h<400||w>16384||h>16384)return;
    layout::width=w;layout::height=h;float s=b4b_hud::Scale(w,h);
    // A mode switch clears the engine's glyph mapping: GetFontTall (slot 66) then reads 0.
    if((lastScreenW&&(w!=lastScreenW||h!=lastScreenH))||(font&&VCall<int(__thiscall*)(void*,unsigned)>(surface,66)(surface,font)==0))ResetFonts();
    lastScreenW=w;lastScreenH=h;
    float alpha=VCall<float(__thiscall*)(void*)>(surface,106)(surface);VCall<void(__thiscall*)(void*,float)>(surface,105)(surface,float(options::values[options::Opacity])/100.f);
    if(textureDirty){
        memset(textureWidths,0,sizeof textureWidths);memset(textureHeights,0,sizeof textureHeights);
        memset(externalWidths,0,sizeof externalWidths);memset(externalHeights,0,sizeof externalHeights);
        for(auto& entry:panelTextures){ReleaseTexture(entry.texture);entry={};}
        textureDirty=false;
    }
    if(++renderFrame==0){renderFrame=1;for(auto& entry:panelTextures)entry.used=entry.checked=0;}
    frameArrowPrepared=false;frameArrowTexture=0;renderStateActive=true;uploadsLeft=3;
    unsigned now=GetTickCount();
    struct PreparedRow {bool visible=false;layout::Box box;int mx=0,my=0,iconLane=0,iconWidth=0,iconHeight=0,group=0,column=0,row=0;float sy=0,unit=0;direction_math::Result direction{};};
    PreparedRow prepared[IconCount]{};
    int columnExtra[2][b4b_hud::SmallColumns]{},rowExtra[2][18]{};
    for(unsigned id:hudOrder){
        if(!DisplayItem(id)){displayedDistances[id]={};directionStates[id].Reset();layout::visualBoxes[id]={};layout::panelBoxes[id]={};continue;}
        auto automatic=b4b_hud::Cell(id,h,s);if(!automatic.w||!automatic.h)continue;
        auto box=layout::Place(id,automatic);bool weapon=id>=11&&id<32;
        float sx=float(box.w)/(weapon?b4b_hud::WeaponPitch:b4b_hud::SmallPitch),sy=float(box.h)/b4b_hud::RowPitch;
        float unit=sx<sy?sx:sy;int mx=int(b4b_hud::Margin*sx+.5f),my=int(b4b_hud::Margin*sy+.5f);
        int arrowMargin=rectangular_hud::Margin(classic_hud::ArrowHalf*unit,b4b_hud::ArrowGap*unit);
        if(mx<arrowMargin)mx=arrowMargin;if(my<arrowMargin)my=arrowMargin;
        layout::Box card{box.x+mx,box.y+my,int(classic_hud::PanelWidth(id)*sx+.5f),int(15*sy+.5f)};
        int countWidth=Limit(int((weapon?12.f:8.f)*sx+.5f),1,card.w/3),heightWidth=Limit(int(12.f*sx+.5f),1,card.w/3);
        if(id==34)countWidth=heightWidth=Limit(int(14.f*sx+.5f),1,card.w/2-1);
        int baseIconWidth=0,baseIconHeight=0;classic_hud::IconSize(id,sx,sy,baseIconWidth,baseIconHeight);
        // Resolve the final aspect fit before centring, avoiding two separate
        // integer roundings that moved small icons up and left by one pixel.
        if(id!=9){auto art=ArtworkMeta(id);if(unsigned(art.sourceWidth)*unsigned(baseIconHeight)>unsigned(art.sourceHeight)*unsigned(baseIconWidth))baseIconHeight=Limit(int(unsigned(baseIconWidth)*art.sourceHeight/art.sourceWidth),1,baseIconHeight);else baseIconWidth=Limit(int(unsigned(baseIconHeight)*art.sourceWidth/art.sourceHeight),1,baseIconWidth);}
        int iconLane=0;
        if(items::IsSpecial(id)){
            iconLane=int(b4b_hud::SpecialIconLane*sx+.5f);
            if(iconLane<baseIconWidth+2)iconLane=baseIconWidth+2;
        }
        Vec point{};if(!displayedDistances[id].valid)directionStates[id].Reset();
        int meters=layout::active?0:DisplayDistance(id,point,now);
        auto result=layout::active?direction_math::Result{0,-1,true,1}:directionStates[id].Update(directionBasis,viewOrigin,point,DisplayKey(id));
        int high=layout::active?0:int(radar_nav::Abs(point.z-viewOrigin.z)/39.3701f+.5f);
        auto& words=PrepareRowText(id,meters,DisplayCount(rows[id]),!layout::active&&rows[id].unlimited,result.height,high,card.w,card.h,countWidth,heightWidth,Limit(int(classic_hud::DistanceFont*sy+.5f),7,42),Limit(int(4.5f*sy+.5f),7,18));
        // Extremely small custom cells have a readable minimum. Expand the
        // complete cell (including arrow padding), then keep it on screen.
        int panelWidth=words.cardWidth+iconLane;
        box.w+=panelWidth-card.w;box.h+=words.cardHeight-card.h;
        if(box.w<panelWidth+2*mx)box.w=panelWidth+2*mx;
        if(box.h<words.cardHeight+2*my)box.h=words.cardHeight+2*my;
        auto& item=prepared[id];item.visible=true;item.box=box;item.mx=mx;item.my=my;item.iconLane=iconLane;item.iconWidth=baseIconWidth;item.iconHeight=baseIconHeight;item.sy=sy;item.unit=unit;item.direction=result;
        b4b_hud::GridPosition(id,item.group,item.column,item.row);
        if(!layout::items[id].custom){
            int extra=box.w-automatic.w;if(extra>columnExtra[item.group][item.column])columnExtra[item.group][item.column]=extra;
            extra=box.h-automatic.h;if(extra>rowExtra[item.group][item.row])rowExtra[item.group][item.row]=extra;
        }
    }
    // Only enlarged automatic cells affect their grid neighbours. Custom
    // coordinates remain controlled by the user. Normal values keep the
    // original B4B positions, including the space reserved for arrows.
    for(unsigned id:hudOrder){
        const auto& item=prepared[id];if(!item.visible)continue;
        auto box=item.box;auto& words=rowTextCaches[options::DigitStyle()][id];
        if(!layout::items[id].custom){
            for(int c=0;c<item.column;++c)box.x+=columnExtra[item.group][c];
            if(item.group==1){for(int r=0;r<=item.row;++r)box.y-=rowExtra[1][r];}
            else{for(int r=0;r<item.row;++r)box.y+=rowExtra[0][r];}
        }
        box.x=Limit(box.x,4,w-box.w-4);box.y=Limit(box.y,2,h-box.h-4);
        layout::Box card{box.x+item.mx,box.y+item.my,words.cardWidth+item.iconLane,words.cardHeight};layout::boxes[id]=box;
        auto palette=ArtworkMeta(id);
        Icon(id+IconCount,card.x,card.y,card.w,card.h);
        // The icon keeps its size as text values change. A wider exceptional
        // string reallocates text lanes without re-uploading the icon.
        int iconWidth=item.iconLane?item.iconLane:card.w;
        Icon(id,card.x+(iconWidth-item.iconWidth)/2,card.y+(card.h-item.iconHeight)/2,item.iconWidth,item.iconHeight);
        int textOrigin=card.x+item.iconLane;
        int textX=textOrigin+words.drawCountWidth,textWidth=words.cardWidth-words.drawCountWidth-words.drawHeightWidth;
        DrawCenteredText({textX,card.y,textWidth,card.h},words.distance,classic_hud::DistanceRed,classic_hud::DistanceGreen,classic_hud::DistanceBlue,(id==1||id==3)?1:0,id==1?1:id==34?int(3.2f*item.unit+.5f):0);
        if(words.showCount)DrawCenteredText({textOrigin,card.y,words.drawCountWidth,card.h},words.quantity,classic_hud::SideRed,classic_hud::SideGreen,classic_hud::SideBlue);
        if(words.showHeight)DrawCenteredText({card.x+card.w-words.drawHeightWidth,card.y,words.drawHeightWidth,card.h},words.elevation,classic_hud::SideRed,classic_hud::SideGreen,classic_hud::SideBlue);
        if(options::values[options::Direction])DirectionArrow(card,item.unit,item.direction,palette);
        layout::visualBoxes[id]=box;
        if(layout::active){layout::panelBoxes[id]=card;if(layout::selected==id)DrawAdjustmentFrame(card);}
    }
    if(layout::active){EditorGuides(w,h);EditorFooter(w,h);}
    VCall<void(__thiscall*)(void*)>(surface,24)(surface);VCall<void(__thiscall*)(void*,float)>(surface,105)(surface,alpha);
}
// Paint-thread entry (once per frame). Every skip is cheap: flags and one entity-identity check.
static void Draw(){
    if(!radarReady||!pixels)return;
    performance::Measure timing(drawCost);
    unsigned now=GetTickCount();
    if(layout::active){DrawCards();return;}
    if(!options::values[options::Enable]||!options::AnyEnabled())return;
    if(!totalVisible||!options::values[options::Opacity]){ResetRenderPresentation();return;}
    if(!activeSource.object||now-activeSourceAt>120||Entity(activeSource.index)!=activeSource.object)return;
    camera_origin::Matrix view{};directionBasis.valid=false;
    bool needView=activeSource.roaming||options::values[options::Direction]!=0;bool haveView=needView&&camera_origin::ReadView(engine,view);
    if(haveView&&options::values[options::Direction])directionBasis.Update(view);
    Vec p{};bool position=activeSource.roaming?(haveView&&camera_origin::Position(view,p)):WorldPosition(activeSource.object,p);
    if(!position||!radar_nav::Position(p))return;
    viewOrigin=p;DrawCards();
}

// ---- artwork (incremental) and resource lifetime -----------------------------------------------------------
static void ReleaseArtwork(){
    if(pixels)HeapFree(GetProcessHeap(),0,pixels);if(decoding)HeapFree(GetProcessHeap(),0,decoding);
    pixels=decoding=nullptr;decodeInput=decodeOutput=0;artFailed=false;
    if(surface&&MethodOK(surface,33)){
        for(auto& id:textures)ReleaseTexture(id);
        for(auto& id:externalTextures)ReleaseTexture(id);
        for(auto& entry:panelTextures){ReleaseTexture(entry.texture);entry={};}
    }
    memset(textures,0,sizeof textures);memset(textureWidths,0,sizeof textureWidths);memset(textureHeights,0,sizeof textureHeights);
    memset(externalTextures,0,sizeof externalTextures);memset(externalWidths,0,sizeof externalWidths);memset(externalHeights,0,sizeof externalHeights);
    frameArrowTexture=0;frameArrowPrepared=false;
}
static void UnloadFonts(){hud_digit_font::Unload();hud_numeric_font::Unload();hud_heart_font::Unload();hud_cat_font::Unload();}
// Decodes about 64 KiB of RGBA per call (one run may exceed that by its own length),
// so artwork never costs a frame. The image is published only when complete.
static void ArtworkStep(){
    if(pixels||artFailed)return;constexpr unsigned size=classic_hud_artwork::Count*classic_hud_artwork::Stride;
    if(!decoding){
        if(sizeof classic_hud_artwork::packed%6){artFailed=true;return;}
        decoding=(unsigned char*)HeapAlloc(GetProcessHeap(),0,size);if(!decoding){artFailed=true;return;}
        decodeInput=decodeOutput=0;hud_digit_font::Load();hud_numeric_font::Load();hud_heart_font::Load();hud_cat_font::Load();
    }
    unsigned end=decodeOutput+64*1024;if(end>size)end=size;
    while(decodeOutput<end&&decodeInput<sizeof classic_hud_artwork::packed){
        auto run=classic_hud_artwork::packed+decodeInput;unsigned n=run[0]|(unsigned(run[1])<<8);
        if(!n||n>(size-decodeOutput)/4){ReleaseArtwork();artFailed=true;return;}
        for(unsigned j=0;j<n;++j)for(unsigned c=0;c<4;++c)decoding[decodeOutput+j*4+c]=run[2+c];
        decodeOutput+=n*4;decodeInput+=6;
    }
    if(decodeInput>=sizeof classic_hud_artwork::packed){
        if(decodeOutput!=size){ReleaseArtwork();artFailed=true;return;}
        pixels=decoding;decoding=nullptr;
    }
}
// Fixed engine/client profile checks, once per map start (cold).
static bool ModuleProfile(const WCHAR* name,unsigned stamp,unsigned size){
    auto p=(unsigned char*)GetModuleHandleW(name);if(!Readable(p,4096)||*(unsigned short*)p!=0x5a4d)return false;
    unsigned n=*(unsigned*)(p+60);return n<0xe00&&*(unsigned*)(p+n)==0x4550&&*(unsigned*)(p+n+8)==stamp&&*(unsigned*)(p+n+80)==size;
}
// Why the last Configure rejected the game (0 = accepted); shown by Stats and logged once per map start.
static int configureFail;
static bool Configure(){
    configured=true;radarReady=false;configureFail=0;entitySerialReady=parentHandleReady=modelLookupReady=directionSurfaceReady=false;
    if(!engine||!entities||!surface){configureFail=1;return false;}
    if(!ModuleProfile(L"engine.dll",0x6a440ed6,0x73f000)||!ModuleProfile(L"client.dll",0x6a440ecf,0x963000)||!ModuleProfile(L"vguimatsurface.dll",0x6a440da4,0x172000)){configureFail=2;return false;}
    const int slots[]={8,9,11,12,17,19,20,22,24,28,29,31,32,34,33,63,64,66,72,105,106};
    for(int slot:slots)if(!MethodOK(surface,slot)){configureFail=100+slot;return false;}
    if(!MethodOK(engine,5)||!MethodOK(engine,12)||!MethodOK(entities,0)||!MethodOK(entities,3)){configureFail=3;return false;}
    if(!ConfigureWorldOrigin()){configureFail=4;return false;}
    if(!EntityScanABI(entities)){configureFail=5;return false;}
    if(!TextureUploadABI(surface)){configureFail=6;return false;}
    if(!TextureReleaseABI(surface)){configureFail=7;return false;}
    if(!FontMetricsABI(surface)){configureFail=8;return false;}
    if(!TextSizeABI(surface)){configureFail=9;return false;}
    parentHandleReady=EntityHandleABI(entities);entitySerialReady=EntityIdentityABI(entities);
    // Retail model lookup (slot 1) and name (slot 3) only; any other layout disables model-based labels.
    auto base=(unsigned char*)GetModuleHandleW(L"engine.dll");
    if(base&&MethodOK(modelInfo,1)&&MethodOK(modelInfo,3))modelLookupReady=(*(void***)modelInfo)[3]==base+0x1afba0&&(*(void***)modelInfo)[1]==base+0x1b0420;
    camera_origin::Configure(engine);directionSurfaceReady=DirectionPolygonABI(surface);
    radarReady=true;return true;
}
static void StartMapNamed(const char* name){mapPending=false;Configure();Log("Supply radar configure: %s (code %d)",radarReady?"ready":"rejected",configureFail);StartMap(name);}
// Video mode / device reset: font handles and uploaded textures are rebuilt lazily by Draw.
static void InvalidateFonts(){ResetFonts();textureDirty=true;}

// ---- settings <-> layout ----------------------------------------------------------------------------------------------
static void SyncLayoutFromSettings(){for(unsigned id=0;id<layout::Count;++id){auto& item=layout::items[id];int at=options::LayoutBase+int(id)*4;item={options::values[at],options::values[at+1],options::values[at+2],options::values[at+3]};}}
static void EndEdit(){if(layout::active)layout::End();}
// Edits are live: each applied key writes the selected item straight into the
// settings array; the existing Paint settings pump debounces the disk write.
static void ApplyEditKey(unsigned key){
    const int action=layout::Key(key,(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0,(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0);
    if(action==2){EndEdit();return;}
    if(action!=3)return;
    const auto& item=layout::items[layout::selected];int at=options::LayoutBase+int(layout::selected)*4;
    options::values[at]=item.custom;options::values[at+1]=item.x;options::values[at+2]=item.y;options::values[at+3]=item.scale;
    settingsChanged=true;
}
static unsigned char editDown[256];static unsigned editRepeatAt;
static void EditKey(unsigned key,bool down){
    if(key>=256)return;bool edge=down&&!editDown[key];editDown[key]=down?1:0;if(!layout::active||!edge)return;
    editRepeatAt=GetTickCount()+300; // hold-to-repeat starts after the first step
    if(key==VK_TAB){layout::Select(layout::Next(layout::selected,(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0));return;}
    ApplyEditKey(key);
}
static void BeginEdit(unsigned label){
    SyncLayoutFromSettings();layout::Begin(label);
    for(unsigned key=0;key<256;++key)editDown[key]=(GetAsyncKeyState(int(key))&0x8000)?1:0;editRepeatAt=GetTickCount()+300;
}
// Settings that change which entities qualify or which source is followed (as upstream ScanSetting).
// Pure presentation settings (count, scale, opacity, direction, height, font, grid, layout) do not rescan.
static bool ScanSetting(int index){
    if(index>=options::ItemBase&&index<options::LayoutBase)return true;
    return index==options::Range||index==options::Medical||index==options::Throwables||index==options::Weapons||
           index==options::Spectator||index==options::MapMetadata;
}

// ---- Pump-side tick (game thread, every frame, work gated to 25 ms) ----------------------------------------------------
static bool GameForeground(){DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();}
static void Tick(unsigned now){
    if(!options::values[options::Enable])return;
    if(mapPending)StartMapNamed(::mapName);
    if(layout::active){
        // Edit keys are polled globally (GetAsyncKeyState), so only act while the game has focus and no UI is open.
        if(GameTextInputVisible()||!GameForeground()){EndEdit();}
        else if(int(now-editRepeatAt)>=0){
            static const unsigned keys[]={VK_LEFT,VK_UP,VK_RIGHT,VK_DOWN,VK_OEM_PLUS,VK_OEM_MINUS,VK_ADD,VK_SUBTRACT};
            editRepeatAt=now+50;for(unsigned key:keys)if(editDown[key])ApplyEditKey(key);
        }
    }
    if(lastTick&&now-lastTick<25)return;lastTick=now;
    if(!radarReady){live=playerValid=false;return;}
    // Only GameUI/console pause the radar. Skeeto menu must keep live=true so
    // scale/layout pages can preview icons (opening the menu previously wiped them).
    if(GameUiOrConsoleVisible()){live=false;return;}
    ArtworkStep();
    if(layout::active){live=true;return;}
    if(!options::AnyEnabled()){activeSource={};if(live||publishedCount||phase)ResetDisplay();live=playerValid=false;return;}
    SyncMetadata();ReadSupplyMap();
    RadarSource current;
    if(!Player(current)){activeSource={};ResetDisplay();live=playerValid=false;return;}
    live=playerValid=true;SetActiveSource(current,now);
    performance::Measure timing(workCost);
    Work(now,current.position,current.object);
}

static bool ExternalIconSize(unsigned label,int screenW,int screenH,float hudScalePercent,int& outW,int& outH){
    if(label==0||label>=IconCount||screenW<640||screenH<400)return false;
    float unit=b4b_hud::Scale(screenW,screenH);
    if(hudScalePercent>0.f&&hudScalePercent!=100.f)unit*=hudScalePercent/100.f;
    if(unit<.35f)unit=.35f;
    if(unit>4.f)unit=4.f;
    int wide=0,tall=0;
    classic_hud::IconSize(label,unit,unit,wide,tall);
    if(wide<1||tall<1)return false;
    // Same aspect resolve the radar applies before Icon().
    if(label!=9){
        auto art=ArtworkMeta(label);
        if(art.sourceWidth>0&&art.sourceHeight>0){
            if(unsigned(art.sourceWidth)*unsigned(tall)>unsigned(art.sourceHeight)*unsigned(wide))
                tall=Limit(int(unsigned(wide)*art.sourceHeight/art.sourceWidth),1,tall);
            else
                wide=Limit(int(unsigned(tall)*art.sourceWidth/art.sourceHeight),1,wide);
        }
    }
    outW=wide;outH=tall;return true;
}
static int CachedExternalTexture(unsigned label,int w,int h){
    if(label==0||label>=IconCount||w<1||h<1)return 0;
    if(!items::IsSpecial(label)&&!pixels)return 0;
    auto m=ArtworkMeta(label);
    const unsigned char* source=ArtworkPixels(label);if(!source)return 0;
    int& t=externalTextures[label];
    bool valid=t!=0&&TextureLive(t);
    if(!valid){t=0;externalWidths[label]=externalHeights[label]=0;}
    if(valid)return t;
    if(!uploadsLeft)return 0;
    int tw=Limit(w,1,512),th=Limit(h,1,256);
    t=VCall<int(__thiscall*)(void*,bool)>(surface,34)(surface,true);
    if(!t)return 0;
    if(!raster_filter::Resize(source,m.sourceWidth,m.sourceHeight,resizedPixels,tw,th)){ReleaseTexture(t);return 0;}
    if(!SkeetoNavUploadRgba(t,resizedPixels,tw,th)){ReleaseTexture(t);return 0;}
    --uploadsLeft;L4N_OBSERVE(++textureUploads);externalWidths[label]=tw;externalHeights[label]=th;
    return t;
}
static bool DrawExternalIcon(unsigned label,int x,int y,int w,int h){
    if(!surface||label==0||label>=IconCount||w<1||h<1)return false;
    if(!items::IsSpecial(label)){
        for(int step=0;step<40&&!pixels&&!artFailed;++step)ArtworkStep();
        if(!pixels)return false;
    }
    if(!uploadsLeft)uploadsLeft=4;
    // First upload size is sticky; draw stretches. Scale edits must not reupload.
    int texture=CachedExternalTexture(label,w,h);
    if(!texture)return false;
    Color(255,255,255,255);
    VCall<void(__thiscall*)(void*,int)>(surface,29)(surface,texture);
    VCall<void(__thiscall*)(void*,int,int,int,int)>(surface,31)(surface,x,y,x+w,y+h);
    return true;
}
static unsigned Stats(char* out,unsigned cap){
    return unsigned(snprintf(out,cap,"radar ready=%d cfg=%d live=%d tracked=%u visible=%u work avg/peak=%u/%uus draw avg/peak=%u/%uus discovery=%u refresh=%u/%u uploads=%u fonts=%u/%u rowtext=%u scan hit/miss=%u/%u art=%d",
        radarReady?1:0,configureFail,live?1:0,publishedCount,totalVisible,workCost.Average(),workCost.peak,drawCost.Average(),drawCost.peak,
        discoveryPasses,refreshPasses,refreshChecks,textureUploads,fontCreates,fontResets,rowTextBuilds,scan_profile::hits,scan_profile::misses,pixels?1:artFailed?-1:0));
}
} // supplyradar
