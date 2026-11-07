#pragma once
#include "skeeto_escui_geometry.h"
static bool g_escUiMulti[skesc::kMaxItems]{},g_escUiSnap=true;
static int g_escUiGuideX=-1,g_escUiGuideY=-1;
static void EscUiSelectOne(int index){std::fill(g_escUiMulti,g_escUiMulti+skesc::kMaxItems,false);g_escUiMulti[index]=true;}
// Included after the ESC lifecycle/native bridge. Paint-owned UI; the existing
// text service supplies a short-lived IME dialog. No file IO or entity work here.
// Paint input edges only. Same UI samples and hover debounce as the homepage.
static void EscUiSound(bool select){
    static DWORD last=0;DWORD now=GetTickCount();if(!select&&last&&(DWORD)(now-last)<80u)return;
    last=now;EngClientCmd(select?"play UI/menu_accept.wav":"play UI/menu_focus.wav");
}
static COLORREF EscUiColor(int preset){
    static constexpr COLORREF colors[]={RGB(255,199,86),RGB(105,222,235),RGB(137,224,175),RGB(223,164,245),RGB(255,153,125),RGB(238,240,245)};
    return colors[preset];
}
static COLORREF EscUiAccentColor(int preset,COLORREF item=RGB(240,242,246)){return preset==6?item:preset==1?RGB(147,197,253):EscUiColor(preset);}
static COLORREF EscUiTextColor(int color){return color?EscUiColor(color-1):RGB(240,242,246);}
static COLORREF EscUiItemColor(const skesc::Item& b){
    return b.textRgb<0?EscUiTextColor(b.color):RGB((b.textRgb>>16)&255,(b.textRgb>>8)&255,b.textRgb&255);
}
static const wchar_t* EscUiColorLabel(int color){
    const UiTextEntry colors[]={{UiId::EscEdWhite,L"默认白色"},{UiId::EscEdAmber,L"琥珀"},{UiId::EscEdCyan,L"青色"},{UiId::EscEdGreen,L"绿色"},{UiId::EscEdPurple,L"紫色"},{UiId::EscEdOrange,L"橙色"},{UiId::EscEdSilver,L"银白"}};
    return UiText(colors[color].id,colors[color].chinese);
}
static const wchar_t* EscUiAccentLabel(int preset){
    if(preset==6)return UiText(UiId::EscAccentFollow,L"跟随当前按钮颜色");
    if(preset==1)return UiText(UiId::EscAccentBlue,L"淡蓝色");
    return EscUiColorLabel(preset+1);
}
static const wchar_t* EscUiBackgroundLabel(int color){
    if(color==-1)return UiText(UiId::EscEdBgGlobal,L"使用统一背景");
    if(!color)return UiText(UiId::EscEdBgClear,L"完全透明");
    if(color==1)return UiText(UiId::EscEdBgFollow,L"跟随文字颜色");
    return EscUiColorLabel(color-2);
}
static bool EscUiEnsureFonts(const skesc::Layout& l,float scale,float uiScale){
    if(g_sktUiFontsHold)return false;
    const int sizes[]={l.bodyPx,l.titlePx,22};bool ok=true;
    for(int i=0;i<3;++i){int px=std::max(12,std::min(192,(int)(sizes[i]*(i==2?uiScale:scale)+.5f)));px=(px+1)&~1;
        if(g_escUiFontEpoch!=g_sktFontEpoch||px!=g_escUiFontPx[i]){
            if(!g_escUiFontTried[i]){
                g_escUiFontTried[i]=true;g_escUiFonts[i]=SurfMakeGameFontRaw(px,i==1?700:500,kFontFlagAntiAlias|kFontFlagDropShadow);
                g_escUiFontValid[i]=g_escUiFonts[i]!=0;
            }else{
                using GlyphFn=bool(__thiscall*)(void*,unsigned long,const char*,int,int,int,int,int,int,int);
                auto glyph=(GlyphFn)VGet(g_surf,kVmtSurfSetFontGlyphSet);g_escUiFontValid[i]=false;
                if(g_escUiFonts[i]&&glyph&&IsExec((void*)glyph)){
                    __try{g_escUiFontValid[i]=SurfApplyGlyphFace(g_surf,glyph,g_escUiFonts[i],px,i==1?700:500,kFontFlagAntiAlias|kFontFlagDropShadow);}
                    __except(EXCEPTION_EXECUTE_HANDLER){g_escUiFontValid[i]=false;}
                }
            }
            g_escUiFontPx[i]=px;g_escUiTextDirty=true;
        }
        ok=ok&&g_escUiFontValid[i];
    }
    g_escUiFontEpoch=g_sktFontEpoch;return ok;
}
static const wchar_t* EscUiLabel(const skesc::Item& b,wchar_t (&out)[128]){
    const bool en=g_uiEnglish.load(std::memory_order_relaxed);
    const char* text=en&&b.en[0]?b.en:b.zh;
    if(text[0]){MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,out,128);return out;}
    if(b.action==skesc::kNativeOptions)return UiText(UiId::EscNativeOptions,L"选项");
    if(b.action==skesc::kMapInfo)return UiText(UiId::EscMapInfo,L"战役 / 关卡 / 建图代码");
    if(b.action==skesc::kClock)return UiText(UiId::EscClock,L"世界时间");
    if(b.action==skesc::kRoundTimer)return UiText(UiId::EscRoundTimer,L"本局计时");
    if(b.action==skesc::kServerInfo)return UiText(UiId::EscServerInfo,L"局内服务器信息条");
    if(b.action==skesc::kCvarPresets)return UiText(UiId::EscCvarPresets,L"ConVar 预设");
    if(b.action==skesc::kBattleReport)return UiText(UiId::EscBattleReport,L"近期战报");
    if(b.action==skesc::kNativeVote)return UiText(UiId::EscUiNativeVote,L"原生发起投票");
    if(b.action==skesc::kVote)return UiText(UiId::EscUiVote,L"发起投票");
    if(b.action==skesc::kQuit)return UiText(UiId::EscUiQuit,L"快速退出");
    if(skesc::IsMenu(b.action))return UiText(UiId::EscEdFlyout,L"飞出子菜单");
    return b.action==skesc::kCommand?UiText(UiId::EscEdCommand,L"控制台命令"):skesc::IsSlider(b.action)?UiText(UiId::MainUiCvarSlider,L"ConVar 滑块"):b.action?UiText(EscUiNativeItem(b.action).label,EscUiNativeItem(b.action).chinese):UiText(UiId::EscUiTitle,L"游戏菜单");
}
static int EscUiButtonState(const skesc::Item& b){
    if(b.action!=skesc::kCommand)return -1;
    if(!_stricmp(b.parameter,"skeeto_hud_toggle"))return g_optPauseSampling?(g_optHideAllCustomHud?5:4):g_optHideAllCustomHud?1:0;
    if(!_stricmp(b.parameter,"skeeto_world_hud_toggle"))return g_optPauseSampling?(g_optHideAllWorldHud?5:4):g_optHideAllWorldHud?1:0;
    if(!_stricmp(b.parameter,"skeeto_xhair_toggle"))return g_optHideAllXhair?1:0;
    if(!_stricmp(b.parameter,"skeeto_hud_pause"))return g_optPauseSampling?3:2;
    return -1;
}
static void EscUiText(int index,const skesc::Item& b,int x,int y,int width,COLORREF color,bool dirty){
    struct Cache{wchar_t text[128];int width,state;unsigned long font;};static Cache cache[skesc::kMaxItems]{};
    auto& c=cache[index];const unsigned long font=g_escUiFonts[b.font];int state=g_escUiEditing?-1:EscUiButtonState(b);
    if(dirty||c.width!=width||c.font!=font||c.state!=state){
        wchar_t converted[128]{},raw[128]{},suffix[32]{};wcsncpy_s(raw,EscUiLabel(b,converted),_TRUNCATE);
        if(state>=0){const UiTextEntry states[]={{UiId::EscStateShown,L"显示"},{UiId::EscStateHidden,L"隐藏"},{UiId::EscStateRunning,L"未暂停"},{UiId::EscStatePaused,L"已暂停"},{UiId::EscStateShownPaused,L"显示 / 已暂停"},{UiId::EscStateHiddenPaused,L"隐藏 / 已暂停"}};
            swprintf_s(suffix,L" [%s]",UiText(states[state].id,states[state].chinese));}
        int limit=std::min((int)wcslen(raw),124-(int)wcslen(suffix));swprintf_s(c.text,L"%.*s%s",limit,raw,suffix);
        if(SurfTextWFont(font,c.text)>width){
            int lo=0,hi=std::min(limit,120-(int)wcslen(suffix));
            while(lo<hi){int mid=(lo+hi+1)/2;swprintf_s(c.text,L"%.*s...%s",mid,raw,suffix);
                if(SurfTextWFont(font,c.text)<=width)lo=mid;else hi=mid-1;}
            swprintf_s(c.text,L"%.*s...%s",lo,raw,suffix);if(SurfTextWFont(font,c.text)>width)c.text[0]=0;
        }
        c.width=width;c.font=font;c.state=state;
    }
    SurfTextAt(font,x,y,color,c.text);
}
// Property strings have fixed cache slots. Measure only after text/font/width
// changes, including language changes; long English never escapes the panel.
static void EscUiUiText(int slot,int x,int y,int width,COLORREF color,const wchar_t* source,int fontIndex=2){
    struct Fit{wchar_t source[192],text[192];int width,px,epoch;unsigned long font;};static Fit cache[112+skesc::kMaxItems]{};
    auto& c=cache[slot];unsigned long font=g_escUiFonts[fontIndex];
    if(c.width!=width||c.font!=font||c.px!=g_escUiFontPx[fontIndex]||c.epoch!=g_sktFontEpoch||wcscmp(c.source,source)){
        wcsncpy_s(c.source,source,_TRUNCATE);wcscpy_s(c.text,c.source);
        if(SurfTextWFont(font,c.text)>width){
            int lo=0,hi=std::min(188,(int)wcslen(c.source));
            while(lo<hi){int mid=(lo+hi+1)/2;wmemcpy(c.text,c.source,mid);wcscpy_s(c.text+mid,192-mid,L"...");
                if(SurfTextWFont(font,c.text)<=width)lo=mid;else hi=mid-1;}
            wmemcpy(c.text,c.source,lo);wcscpy_s(c.text+lo,192-lo,L"...");if(SurfTextWFont(font,c.text)>width)c.text[0]=0;
        }
        c.width=width;c.font=font;c.px=g_escUiFontPx[fontIndex];c.epoch=g_sktFontEpoch;
    }
    SurfTextAt(font,x,y,color,c.text);
}

#include "skeeto_escui_extras.h"

// Only displayed live sliders sample Cvars, <=4Hz each. Parsing occurs after
// document changes. No Source pointers survive between samples or maps.
struct EscUiSlider {skmenu::CvarSpec spec{};DWORD readAt=0,sendAt=0;float value=0,lastSent=0;bool valid=false,available=false,writable=false,sent=false;wchar_t number[48]{};};
static EscUiSlider g_escUiSliders[skesc::kMaxItems]{};
static EscUiSlider& EscUiSliderRead(int index,const skesc::Item& b,bool dirty){
    auto& c=g_escUiSliders[index];
    if(dirty){c=EscUiSlider{};c.valid=skmenu::CvarParse(b.parameter,&c.spec);}
    DWORD now=GetTickCount();
    if(c.valid&&(!c.readAt||(DWORD)(now-c.readAt)>=250u)){
        c.readAt=now;void* var=CvarFind(c.spec.name);float value=0;
        const bool available=var&&CvarReadPair(var,nullptr,&value)&&_finite(value);
        const int flags=var?CvarFlagsOf(var):0;
        c.writable=available&&(b.action==skesc::kPinnedSlider||!(flags&(kFCvarCheat|kFCvarDevOnly|kFCvarHidden)));
        if(available!=c.available||(available&&value!=c.value))c.sent=false;
        c.available=available;
        if(c.available){c.value=value;swprintf_s(c.number,L"%.4g",double(value));}else wcscpy_s(c.number,L"--");
    }
    return c;
}
static void EscUiSliderSet(int index,float fraction,int direction=0,bool final=false){
    auto& c=g_escUiSliders[index];if(!c.writable)return;
    const auto& spec=c.spec;double raw=direction?double(c.value)+direction*spec.step:spec.minimum+double(fraction)*(spec.maximum-spec.minimum);
    float value=(float)(spec.minimum+floor((raw-spec.minimum)/spec.step+.5)*spec.step);
    value=std::max(spec.minimum,std::min(spec.maximum,value));
    if((c.sent&&c.lastSent==value)||(!c.sent&&c.value==value))return;
    DWORD now=GetTickCount();if(!final&&c.sendAt&&(DWORD)(now-c.sendAt)<50u)return;c.sendAt=now;
    if(g_escUiLayout.items[index].action==skesc::kPinnedSlider){
        int result=SktCvarMenuWrite(spec.name,value);
        if(result!=1){g_escUiStatus=result==2?EscUiStatus::PinDeferred:EscUiStatus::PinError;return;}
        if(g_escUiStatus==EscUiStatus::PinError||g_escUiStatus==EscUiStatus::PinDeferred)g_escUiStatus=EscUiStatus::Ready;
    }else{char command[128]{};snprintf(command,sizeof(command),"%s %.9g",spec.name,double(value));EngClientCmd(command);}
    c.sent=true;c.lastSent=c.value=value;c.sendAt=c.readAt=now;swprintf_s(c.number,L"%.4g",double(value));
}
static const wchar_t* EscUiActionLabel(int action){
    if(action==skesc::kNativeOptions)return UiText(UiId::EscNativeOptions,L"选项");
    if(action==skesc::kMapInfo)return UiText(UiId::EscMapInfo,L"战役 / 关卡 / 建图代码");
    if(action==skesc::kClock)return UiText(UiId::EscClock,L"世界时间");
    if(action==skesc::kRoundTimer)return UiText(UiId::EscRoundTimer,L"本局计时");
    if(action==skesc::kServerInfo)return UiText(UiId::EscServerInfo,L"局内服务器信息条");
    if(action==skesc::kCvarPresets)return UiText(UiId::EscCvarPresets,L"ConVar 预设");
    if(action==skesc::kBattleReport)return UiText(UiId::EscBattleReport,L"近期战报");
    if(action==skesc::kNativeVote)return UiText(UiId::EscUiNativeVote,L"原生发起投票");
    if(action==skesc::kVote)return UiText(UiId::EscUiVote,L"发起投票");
    if(action==skesc::kQuit)return UiText(UiId::EscUiQuit,L"快速退出");
    if(action==0)return UiText(UiId::EscEdDecoration,L"仅显示文字");
    if(action==skesc::kCommand)return UiText(UiId::EscEdCommand,L"控制台命令");
    if(action==skesc::kFlyout)return UiText(UiId::EscEdFlyout,L"飞出子菜单");
    if(action==skesc::kPinnedSlider)return UiText(UiId::EscEdPinnedSlider,L"ConVar 会话滑块 (skt_cvar)");
    if(action==skesc::kSlider)return UiText(UiId::MainUiCvarSlider,L"ConVar 滑块");
    return UiText(EscUiNativeItem(action).label,EscUiNativeItem(action).chinese);
}
enum class EscUiEditAction{Apply,Cancel,Next,TextZh,TextEn,Action,Parameter,Parent,Enter,Visible,Color,Background,Opacity,Accent,Font,Back,Front,Duplicate,Delete,AddButton,AddText,AddMenu,Defaults,Profile,SaveProfile,LoadProfile,OpenProfiles};
struct EscUiEditItem{UiId id;const wchar_t* chinese;EscUiEditAction action;};
static constexpr EscUiEditItem kEscUiEditItems[]={
    {UiId::EscEdApply,L"应用并保存",EscUiEditAction::Apply},
    {UiId::EscEdCancel,L"取消草稿",EscUiEditAction::Cancel},
    {UiId::EscEdNext,L"元素 %d / %d：打开元素列表",EscUiEditAction::Next},
    {UiId::EscEdZh,L"编辑中文文字",EscUiEditAction::TextZh},
    {UiId::EscEdEn,L"Edit English text",EscUiEditAction::TextEn},
    {UiId::EscEdAction,L"动作：%s",EscUiEditAction::Action},
    {UiId::EscEdParameter,L"编辑动作参数",EscUiEditAction::Parameter},
    {UiId::EscEdParent,L"所属菜单：%s",EscUiEditAction::Parent},
    {UiId::EscEdEnter,L"进入这个子菜单编辑",EscUiEditAction::Enter},
    {UiId::EscEdVisible,L"显示：%s",EscUiEditAction::Visible},
    {UiId::EscEdColor,L"文字颜色：%s",EscUiEditAction::Color},
    {UiId::EscEdBackground,L"按钮底色 / 统一配色 / 透明度",EscUiEditAction::Background},
    {UiId::EscEdItemOpacity,L"按钮背景透明度：%s",EscUiEditAction::Opacity},
    {UiId::EscEdAccent,L"悬停边框颜色：%s",EscUiEditAction::Accent},
    {UiId::EscEdFont,L"字号类型：%s",EscUiEditAction::Font},
    {UiId::EscEdBack,L"下移一层 / 导航前移",EscUiEditAction::Back},
    {UiId::EscEdFront,L"上移一层 / 导航后移",EscUiEditAction::Front},
    {UiId::EscEdDuplicate,L"复制元素及子项",EscUiEditAction::Duplicate},
    {UiId::EscEdDelete,L"删除元素及全部子项",EscUiEditAction::Delete},
    {UiId::EscEdAddButton,L"添加按钮",EscUiEditAction::AddButton},
    {UiId::EscEdAddText,L"添加文字",EscUiEditAction::AddText},
    {UiId::EscEdAddMenu,L"添加飞出子菜单",EscUiEditAction::AddMenu},
    {UiId::EscEdDefaults,L"草稿恢复默认布局",EscUiEditAction::Defaults},
    {UiId::EscEdProfile,L"方案槽位：%d / 8",EscUiEditAction::Profile},
    {UiId::EscEdSaveProfile,L"保存草稿到方案槽位",EscUiEditAction::SaveProfile},
    {UiId::EscEdLoadProfile,L"从方案槽位载入草稿",EscUiEditAction::LoadProfile},
    {UiId::EscEdOpenProfiles,L"打开方案文件夹",EscUiEditAction::OpenProfiles},
};
static void EscUiRevealSelected(){
    g_escUiFlyout=g_escUiDraft.items[g_escUiSelected].parent;
    g_escUiFlyoutPage=g_escUiFlyout<0?0:skesc::PageFor(g_escUiDraft,g_escUiSelected,true);
}
static void EscUiOpenFlyout(int index){
    g_escUiFlyout=index;g_escUiFlyoutPage=0;g_escUiPressed=g_escUiDrag=g_escUiSliderDrag=-1;g_escUiFocus=-1;g_escUiTextDirty=true;
}
static int EscUiPickCount(){
    switch(g_escUiPick){case 1:return g_escUiDraft.count;case 2:return skesc::kActionCount;case 3:return 8;case 4:return 2;case 5:return 8;case 6:return 5;case 8:return 5;case 9:return 10;case 10:return 9;case 11:return 0;case 12:return 11;default:break;}
    int count=1;for(int i=0;i<g_escUiDraft.count;++i)if(skesc::IsMenu(g_escUiDraft.items[i].action))++count;return count;
}
static int EscUiParentChoice(int nth){
    if(!nth)return -1;for(int i=0;i<g_escUiDraft.count;++i)if(skesc::IsMenu(g_escUiDraft.items[i].action)&&!--nth)return i;return -1;
}
static void EscUiTextStart(int field){
    if(g_escUiTextRequest){g_escUiStatus=EscUiStatus::Busy;return;}
    const auto& b=g_escUiDraft.items[g_escUiSelected];wchar_t seed[384]{};
    if(field==7){COLORREF rgb=EscUiItemColor(b);swprintf_s(seed,L"%d %d %d",GetRValue(rgb),GetGValue(rgb),GetBValue(rgb));}
    else if(field>=3)wcscpy_s(seed,g_escUiCvarFields[field-3]);
    else MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,field==0?b.zh:field==1?b.en:b.parameter,-1,seed,384);
    char caption[512]{};WideCharToMultiByte(CP_UTF8,0,(field==7?UiText(UiId::EscEdRgbInput,L"输入 RGB：R G B，每项 0 到 255"):UiText(UiId::EscEdTextCaption,L"Skeeto ESC 文字编辑")),-1,caption,512,nullptr,nullptr);
    if(field==2){if(const char* choices=skesc::ComponentOptions(b.action))snprintf(caption,sizeof(caption),"Skeeto: %s",choices);}
    g_escUiTextRequest=MainUiServicesText(seed,caption);
    if(g_escUiTextRequest){g_escUiTextIndex=g_escUiSelected;g_escUiTextLanguage=field;}
    else g_escUiStatus=EscUiStatus::Busy;
}
static void EscUiPickOpen(int kind){g_escUiPick=kind;g_escUiPickPage=kind==1?g_escUiSelected/16:0;g_escUiDrag=g_escUiSliderDrag=-1;}
static void EscUiEditorAction(EscUiEditAction action){
    if(action==EscUiEditAction::Cancel){EscUiAbortEditor(true);return;}
    if(g_escUiIoBusy){g_escUiStatus=EscUiStatus::Busy;return;}
    auto& l=g_escUiDraft;auto& b=l.items[g_escUiSelected];
    if(action!=EscUiEditAction::SaveProfile)g_escUiConfirmProfile=false;
    if(action!=EscUiEditAction::Apply)g_escUiConfirmCurrent=false;
    EscUiCancelText();EscUiSelectOne(g_escUiSelected);
    switch(action){
    case EscUiEditAction::Apply:
        if(!skesc::Valid(l)){g_escUiStatus=EscUiStatus::LayoutError;return;}
        if(!g_escUiWritable&&!g_escUiConfirmCurrent){g_escUiConfirmCurrent=true;g_escUiStatus=EscUiStatus::Confirm;return;}
        g_escUiWritable=true;g_escUiLayout=l;EscUiAbortEditor(true);EscUiLayoutChanged();return;
    case EscUiEditAction::Next:EscUiPickOpen(1);return;
    case EscUiEditAction::TextZh:EscUiTextStart(0);return;
    case EscUiEditAction::TextEn:EscUiTextStart(1);return;
    case EscUiEditAction::Action:EscUiPickOpen(2);return;
    case EscUiEditAction::Parameter:
        if(b.action==skesc::kCommand||skesc::ComponentOptions(b.action)){EscUiTextStart(2);return;}
        if(skesc::IsSlider(b.action)){skmenu::CvarSpec v{};skmenu::CvarParse(b.parameter,&v);
            MultiByteToWideChar(CP_UTF8,0,v.name,-1,g_escUiCvarFields[0],64);
            swprintf_s(g_escUiCvarFields[1],L"%.9g",double(v.minimum));swprintf_s(g_escUiCvarFields[2],L"%.9g",double(v.maximum));swprintf_s(g_escUiCvarFields[3],L"%.9g",double(v.step));EscUiPickOpen(6);return;}
        g_escUiStatus=EscUiStatus::ParameterError;return;
    case EscUiEditAction::Parent:EscUiPickOpen(7);return;
    case EscUiEditAction::Enter:if(skesc::IsMenu(b.action))EscUiOpenFlyout(g_escUiSelected);return;
    case EscUiEditAction::Visible:b.visible=!b.visible;break;
    case EscUiEditAction::Accent:l.accent=(l.accent+1)%7;break;
    case EscUiEditAction::Color:EscUiPickOpen(3);return;
    case EscUiEditAction::Background:EscUiPickOpen(8);return;
    case EscUiEditAction::Opacity:return; // Inline slider owns this row.
    case EscUiEditAction::Font:EscUiPickOpen(4);return;
    case EscUiEditAction::Back:g_escUiSelected=skesc::Move(l,g_escUiSelected,-1);break;
    case EscUiEditAction::Front:g_escUiSelected=skesc::Move(l,g_escUiSelected,1);break;
    case EscUiEditAction::Duplicate:{int copied=skesc::Duplicate(l,g_escUiSelected);if(copied>=0)g_escUiSelected=copied;else g_escUiStatus=EscUiStatus::Limit;break;}
    case EscUiEditAction::Delete:if(skesc::Erase(l,g_escUiSelected))g_escUiSelected=std::min(g_escUiSelected,l.count-1);else g_escUiStatus=EscUiStatus::NeedResume;break;
    case EscUiEditAction::AddButton:case EscUiEditAction::AddText:case EscUiEditAction::AddMenu:{
        int kind=action==EscUiEditAction::AddMenu?skesc::kFlyout:action==EscUiEditAction::AddButton?2:0;
        if(skesc::Add(l,kind,g_escUiFlyout)){
            g_escUiSelected=l.count-1;
            if(!skesc::Valid(l)){--l.count;g_escUiSelected=0;g_escUiStatus=EscUiStatus::LayoutError;}
            else if(action==EscUiEditAction::AddButton)EscUiPickOpen(2);
        }else g_escUiStatus=EscUiStatus::Limit;break;}
    case EscUiEditAction::Defaults:l=skesc::Defaults();g_escUiSelected=0;break;
    case EscUiEditAction::Profile:EscUiPickOpen(5);return;
    case EscUiEditAction::SaveProfile:
        if(!g_escUiConfirmProfile){g_escUiConfirmProfile=true;g_escUiStatus=EscUiStatus::Confirm;return;}
        g_escUiConfirmProfile=false;EscUiPostIo(2);return;
    case EscUiEditAction::LoadProfile:EscUiPostIo(3);return;
    case EscUiEditAction::OpenProfiles:EscUiPostIo(4);return;
    default:break;
    }
    EscUiRevealSelected();g_escUiTextDirty=true;
}
static void EscUiPickerChoose(int action){
    if(action==2000){EscUiCancelText();g_escUiPick=0;return;}
    if(g_escUiIoBusy)return;
    if(action==2001){g_escUiPickPage=std::max(0,g_escUiPickPage-1);return;}
    if(action==2002){g_escUiPickPage=std::min((EscUiPickCount()-1)/16,g_escUiPickPage+1);return;}
    if(action<1000||action>=1016)return;
    const int selected=action-1000+g_escUiPickPage*16;auto& b=g_escUiDraft.items[g_escUiSelected];
    EscUiCancelText();g_escUiConfirmCurrent=g_escUiConfirmProfile=false;
    if(g_escUiPick==12){
        g_escUiStatus=skescgeo::Arrange(g_escUiDraft,g_escUiMulti,g_escUiSelected,selected)?EscUiStatus::Ready:EscUiStatus::ArrangeError;
        g_escUiPick=0;g_escUiTextDirty=true;return;
    }
    if(g_escUiPick==1){g_escUiSelected=selected;EscUiSelectOne(selected);}
    else if(g_escUiPick==2){auto old=b;skesc::SetAction(b,selected);skesc::ClampItem(g_escUiDraft,g_escUiSelected);
        if(!skesc::Valid(g_escUiDraft)){b=old;g_escUiStatus=EscUiStatus::LayoutError;return;}}
    else if(g_escUiPick==3){if(selected==7){EscUiTextStart(7);return;}b.color=selected;b.textRgb=-1;}
    else if(g_escUiPick==4){b.font=selected;skesc::ClampItem(g_escUiDraft,g_escUiSelected);}
    else if(g_escUiPick==5)g_escUiProfile=selected+1;
    else if(g_escUiPick==8){
        if(selected<3){EscUiPickOpen(9+selected);return;}
        if(selected==4){g_escUiDraft.transparentBorder=!g_escUiDraft.transparentBorder;return;}
        for(int i=0;i<g_escUiDraft.count;++i){g_escUiDraft.items[i].background=-1;g_escUiDraft.items[i].opacity=-1;}
        return;
    }
    else if(g_escUiPick==9)b.background=selected-1;
    else if(g_escUiPick==10)g_escUiDraft.background=selected;
    else if(g_escUiPick==7){if(!skesc::SetParent(g_escUiDraft,g_escUiSelected,EscUiParentChoice(selected))){g_escUiStatus=EscUiStatus::LayoutError;return;}}
    else if(g_escUiPick==6){
        if(selected<4){EscUiTextStart(3+selected);return;}
        char fields[4][64]{},parameter[skesc::kParamBytes]{};
        for(int i=0;i<4;++i)if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,g_escUiCvarFields[i],-1,fields[i],64,nullptr,nullptr)){g_escUiStatus=EscUiStatus::ParameterError;return;}
        int n=snprintf(parameter,sizeof(parameter),"%s %s %s %s",fields[0],fields[1],fields[2],fields[3]);
        if(n<0||n>=(int)sizeof(parameter)||!skmenu::CvarParse(parameter,nullptr)){g_escUiStatus=EscUiStatus::ParameterError;return;}
        strcpy_s(b.parameter,parameter);
    }
    EscUiRevealSelected();g_escUiTextDirty=true;g_escUiPick=0;
}
static void EscUiTakeEditorText(){
    if(g_escUiTextIndex<0||!g_escUiTextRequest)return;wchar_t input[384]{};
    auto result=MainUiServicesTakeText(g_escUiTextRequest,input,384);if(result==MainUiTextResult::None)return;
    g_escUiTextRequest=0;
    if(result!=MainUiTextResult::Accepted){g_escUiTextIndex=-1;g_escUiSeed=true;if(result==MainUiTextResult::Failed)g_escUiStatus=EscUiStatus::TextError;return;}
    auto& b=g_escUiDraft.items[g_escUiTextIndex];int field=g_escUiTextLanguage;
    if(field==7){char rgb[64]{};int value=0;
        if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,input,-1,rgb,64,nullptr,nullptr)&&skesc::RgbParse(rgb,value)){
            b.textRgb=value;g_escUiTextDirty=true;g_escUiPick=0;
        }else g_escUiStatus=EscUiStatus::RgbError;
    }
    else if(field>=3){if(wcslen(input)<64)wcscpy_s(g_escUiCvarFields[field-3],input);else g_escUiStatus=EscUiStatus::ParameterError;}
    else{char utf8[skesc::kParamBytes]{};int cap=field==2?skesc::kParamBytes:skesc::kLabelBytes;
        if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,input,-1,utf8,cap,nullptr,nullptr)&&skesc::LabelValid(utf8,cap)){
            if(field==2){auto candidate=b;strcpy_s(candidate.parameter,utf8);if(skesc::ParameterValid(candidate))strcpy_s(b.parameter,utf8);else g_escUiStatus=EscUiStatus::ParameterError;}
            else strcpy_s(field?b.en:b.zh,skesc::kLabelBytes,utf8);
            g_escUiTextDirty=true;
        }else g_escUiStatus=EscUiStatus::TextError;
    }
    g_escUiTextIndex=-1;g_escUiSeed=true;
}
static void EscUiNavigate(const skesc::Layout& l,bool home){
    int old=g_escUiFlyout;if(old<0)return;
    int target=home?-1:l.items[old].parent;
    int selected=old;
    if(home)for(int depth=0;depth<skesc::kDepth&&l.items[selected].parent>=0;++depth)selected=l.items[selected].parent;
    g_escUiFlyout=target;g_escUiFlyoutPage=target<0?0:skesc::PageFor(l,selected,g_escUiEditing);
    if(g_escUiEditing)g_escUiSelected=selected;
    g_escUiPressed=g_escUiDrag=g_escUiSliderDrag=-1;g_escUiFocus=-1;g_escUiTextDirty=true;
}
static void EscUiAfterPaint(int mode){
    if(!g_escUiDraw||!(mode&kPaintUiPanels))return;
    // Native RunFrame/OnOpen can change windows inside the original Paint.
    // Decide ownership at the final consumer, immediately before input/drawing.
    if(!EscUiNativeHomeReady()){EscUiSuspendInput();return;}
    SurfDrawingLease drawing;if(!drawing.ready)return;
    auto& l=g_escUiEditing?g_escUiDraft:g_escUiLayout;
    const float scale=std::fmin(g_escUiWidth/1920.f,g_escUiHeight/1080.f);
    const float canvas=g_escUiEditing?scale*.64f:scale;
    auto U=[=](int x){return(int)(x*scale+.5f);};auto C=[=](int x){return(int)(x*canvas+.5f);};
    const int ox=(g_escUiWidth-U(1920))/2,oy=(g_escUiHeight-U(1080))/2;
    const int cx=ox,cy=oy+(U(1080)-C(1080))/2;
    if(!EscUiEnsureFonts(l,canvas,scale)){EscUiRestore();g_escUiGate="font-unavailable";return;}
    static bool english=false;bool language=g_uiEnglish.load(std::memory_order_relaxed);
    if(language!=english){english=language;g_escUiTextDirty=true;}
    if(g_escUiEditing)EscUiTakeEditorText();
    POINT pos{};RECT client{};int mx=-1,my=-1,ux=-1,uy=-1;
    if(GetCursorPos(&pos)&&ScreenToClient(g_gameHwnd,&pos)&&GetClientRect(g_gameHwnd,&client)&&client.right>0&&client.bottom>0){
        const int px=MulDiv(pos.x,g_escUiWidth,client.right),py=MulDiv(pos.y,g_escUiHeight,client.bottom);
        mx=(int)((px-cx)/canvas);my=(int)((py-cy)/canvas);ux=(int)((px-ox)/scale);uy=(int)((py-oy)/scale);
    }
    static bool mouseMode=true;static int lastMx=-1,lastMy=-1,lastHover=-1;
    if(g_escUiSeed){mouseMode=true;lastHover=-1;}
    if(mx!=lastMx||my!=lastMy)mouseMode=true;
    lastMx=mx;lastMy=my;
    int view[skesc::kMaxItems]{},count=skesc::View(l,g_escUiFlyout,g_escUiFlyoutPage,g_escUiEditing,view);
    if(g_escUiEditing){
        bool shown[skesc::kMaxItems]{};for(int n=0;n<count;++n)shown[view[n]]=true;
        if(count&&!shown[g_escUiSelected]){g_escUiSelected=view[0];EscUiSelectOne(g_escUiSelected);}
        if(g_escUiSeed||!g_escUiMulti[g_escUiSelected])EscUiSelectOne(g_escUiSelected);
        for(int i=0;i<l.count;++i)if(!shown[i])g_escUiMulti[i]=false;
    }
    const int childCount=count;
    if(!g_escUiEditing&&g_escUiFlyout>=0)for(int i=0;i<l.count;++i){const auto& b=l.items[i];
        if(b.parent>=0||!b.visible)continue;
        // Keep the other rail usable, but never draw or hit a root underneath
        // a child or its navigation. <=128 roots x 8 displayed children.
        bool overlap=b.x+b.w>1408&&b.x<1888&&b.y+b.h>88&&b.y<1012;
        for(int n=0;n<childCount&&!overlap;++n){const auto& c=l.items[view[n]];
            overlap=b.x<c.x+c.w&&b.x+b.w>c.x&&b.y<c.y+c.h&&b.y+b.h>c.y;
        }
        if(!overlap)view[count++]=i;
    }
    const int pages=g_escUiFlyout<0?1:std::max(1,(skesc::Children(l,g_escUiFlyout,g_escUiEditing)+skesc::kPageItems-1)/skesc::kPageItems);
    struct Hit{int x,y,w,h,id;};Hit controls[48]{};int controlCount=0;
    auto button=[&](int x,int y,int w,int h,int id){controls[controlCount++]={x,y,w,h,id};};
    auto canvasButton=[&](int x,int y,int w,int h,int id){const float ratio=canvas/scale;button((int)(x*ratio),(int)((cy-oy)/scale+y*ratio),(int)(w*ratio),(int)(h*ratio),id);};
    int hit=-1;
    for(int n=count-1;n>=0;--n){int i=view[n];const auto& b=l.items[i];if(!g_escUiEditing&&!b.action)continue;
        if(mx>=b.x&&mx<b.x+b.w&&my>=b.y&&my<b.y+b.h){hit=i;break;}}
    if(g_escUiEditing&&g_escUiPick){
        hit=-1;button(1030,156,170,44,2000);int choices=EscUiPickCount();
        if(g_escUiPick==11)button(396,352,680,52,912);
        for(int i=0;i<16&&g_escUiPickPage*16+i<choices;++i)button(160+(i/8)*520,234+(i%8)*76,500,62,1000+i);
        if(choices>16){button(160,880,240,48,2001);button(420,880,240,48,2002);}
    }else{
        if(g_escUiEditing){button(32,32,340,40,920);button(388,32,320,40,921);}
        if(g_escUiEditing){if(ux>=1280)hit=-1;for(int i=0;i<(int)_countof(kEscUiEditItems);++i){
            int y=118+i*32;
            if(kEscUiEditItems[i].action==EscUiEditAction::Opacity){button(1536,y,240,30,910);button(1790,y,110,30,911);}
            else button(1300,y,600,30,300+i);
        }}
        else button(1548,32,340,40,900);
        if(g_escUiFlyout>=0){
            canvasButton(1416,96,224,44,902);canvasButton(1664,96,224,44,903);
            if(pages>1){canvasButton(1416,960,176,44,904);canvasButton(1712,960,176,44,905);}
        }
    }
    if(!g_escUiEditing&&g_escUiExtraPanel){
        hit=-1;
        for(int i=0;i<8;++i)button(460+(i%2)*500,230+(i/2)*112,480,88,2100+i);
        button(460,730,480,58,2108);button(960,730,480,58,2109);
        button(460,808,480,58,2110);button(960,808,480,58,2111);
    }
    for(int i=controlCount-1;i>=0;--i){const auto& c=controls[i];if(ux>=c.x&&ux<c.x+c.w&&uy>=c.y&&uy<c.y+c.h){hit=c.id;break;}}
    const int keys[]={VK_LBUTTON,VK_UP,VK_DOWN,VK_TAB,VK_RETURN,VK_LEFT,VK_RIGHT,VK_BACK};bool down[8]{};
    for(int i=0;i<8;++i)down[i]=(GetAsyncKeyState(keys[i])&0x8000)!=0;
    bool focusInView=false;
    for(int n=0;n<count;++n)if(view[n]==g_escUiFocus&&l.items[view[n]].action)focusInView=true;
    if(!focusInView){g_escUiFocus=-1;for(int n=0;n<count;++n)if(l.items[view[n]].action){g_escUiFocus=view[n];break;}}
    if(!g_escUiEditing&&!g_escUiExtraPanel)for(int n=0;n<count;++n){int i=view[n];if(skesc::IsSlider(l.items[i].action))EscUiSliderRead(i,l.items[i],g_escUiTextDirty);}
    const int oldFocus=g_escUiFocus;
    int action=-1;
    if(!g_escUiSeed){
        if(down[0]&&!g_escUiKeys[0]){
            mouseMode=true;if(hit>=0)EscUiSound(true);
            g_escUiPressed=hit;
            if(g_escUiEditing&&!g_escUiPick&&hit>=0&&hit<l.count&&!(g_escUiIoBusy&&g_escUiIoKind==3)){
                EscUiCancelText();
                if(GetAsyncKeyState(VK_CONTROL)&0x8000){
                    if(g_escUiMulti[hit]&&skescgeo::Count(l,g_escUiMulti)>1){g_escUiMulti[hit]=false;if(g_escUiSelected==hit)for(int n=0;n<count;++n)if(g_escUiMulti[view[n]]){g_escUiSelected=view[n];break;}}
                    else{g_escUiMulti[hit]=true;g_escUiSelected=hit;}
                    g_escUiDrag=-1;
                }else{
                if(!g_escUiMulti[hit])EscUiSelectOne(hit);g_escUiSelected=hit;auto& b=l.items[hit];g_escUiDrag=hit;
                g_escUiResize=mx>=b.x+b.w-16&&my>=b.y+b.h-16;
                g_escUiDragX=g_escUiResize?b.w-mx+b.x:mx-b.x;g_escUiDragY=g_escUiResize?b.h-my+b.y:my-b.y;
                }
            }else if(g_escUiEditing&&!g_escUiIoBusy&&(hit==910||hit==912)){EscUiCancelText();g_escUiSliderDrag=hit;}
            else if(!g_escUiEditing&&hit>=0&&hit<l.count&&skesc::IsSlider(l.items[hit].action)&&g_escUiSliders[hit].writable){g_escUiSliderDrag=hit;g_escUiFocus=hit;}
        }
        g_escUiGuideX=g_escUiGuideY=-1;
        if(g_escUiEditing&&down[0]&&g_escUiDrag>=0){auto& b=l.items[g_escUiDrag];int oldX=b.x,oldY=b.y,oldW=b.w,oldH=b.h;
            if(g_escUiResize){b.w=mx-b.x+g_escUiDragX;b.h=my-b.y+g_escUiDragY;}else{b.x=mx-g_escUiDragX;b.y=my-g_escUiDragY;}
            skesc::ClampItem(l,g_escUiDrag);
            skescgeo::Snap snap{};
            if(g_escUiSnap&&!(GetAsyncKeyState(VK_MENU)&0x8000)&&(b.x!=oldX||b.y!=oldY||b.w!=oldW||b.h!=oldH)){
                snap=skescgeo::Align(l,g_escUiDrag,g_escUiMulti,view,count,g_escUiResize);
                if(g_escUiResize){b.w+=snap.dx;b.h+=snap.dy;}else{b.x+=snap.dx;b.y+=snap.dy;}
            }
            if(g_escUiResize)skesc::ClampItem(l,g_escUiDrag);
            else{int dx=b.x-oldX,dy=b.y-oldY;b.x=oldX;b.y=oldY;skescgeo::Move(l,g_escUiMulti,dx,dy);}
            if(snap.x>=0&&(b.x==snap.x||b.x+b.w/2==snap.x||b.x+b.w==snap.x))g_escUiGuideX=snap.x;
            if(snap.y>=0&&(b.y==snap.y||b.y+b.h/2==snap.y||b.y+b.h==snap.y))g_escUiGuideY=snap.y;
        }
        if(g_escUiEditing&&(g_escUiSliderDrag==910||g_escUiSliderDrag==912)){
            const bool global=g_escUiSliderDrag==912;
            int percent=std::max(0,std::min(100,(ux-(global?396:1536))*100/(global?680:240)));
            (global?l.opacity:l.items[g_escUiSelected].opacity)=100-percent;
            if(!down[0]){g_escUiSliderDrag=-1;g_escUiPressed=-1;}
        }
        if(!g_escUiEditing&&g_escUiSliderDrag>=0){const auto& b=l.items[g_escUiSliderDrag];
            EscUiSliderSet(g_escUiSliderDrag,float(mx-b.x-18)/std::max(1,b.w-36),0,!down[0]);
            if(!down[0]){g_escUiSliderDrag=-1;g_escUiPressed=-1;}
        }
        if(!down[0]&&g_escUiKeys[0]){if(hit>=0&&hit==g_escUiPressed)action=hit;g_escUiPressed=g_escUiDrag=-1;}
        if(down[7]&&!g_escUiKeys[7]&&g_escUiFlyout>=0&&!g_escUiPick)action=902;
        if(!g_escUiEditing&&!g_escUiExtraPanel){
            if(down[4]&&!g_escUiKeys[4]){
                if(mouseMode&&hit>=0&&hit<l.count&&l.items[hit].action)g_escUiFocus=hit;
                mouseMode=false;EscUiSound(true);
            }
            if(down[7]&&!g_escUiKeys[7])EscUiSound(true);
            if((down[5]&&!g_escUiKeys[5])||(down[6]&&!g_escUiKeys[6]))mouseMode=false;
            if((down[1]&&!g_escUiKeys[1])||(down[2]&&!g_escUiKeys[2])||(down[3]&&!g_escUiKeys[3])){
                mouseMode=false;
                int direction=down[1]||(down[3]&&(GetAsyncKeyState(VK_SHIFT)&0x8000))?-1:1,at=0;
                for(int n=0;n<count;++n)if(view[n]==g_escUiFocus)at=n;
                for(int n=0;n<count;++n){at=(at+direction+count)%count;if(l.items[view[at]].action){g_escUiFocus=view[at];break;}}
            }
            if(g_escUiFocus>=0&&skesc::IsSlider(l.items[g_escUiFocus].action)){
                if((down[5]&&!g_escUiKeys[5])||(down[6]&&!g_escUiKeys[6]))EscUiSliderSet(g_escUiFocus,0,down[5]?-1:1,true);
            }else if(down[4]&&!g_escUiKeys[4])action=g_escUiFocus;
        }else if(g_escUiEditing&&!g_escUiPick){
            static DWORD repeatAt=0;bool edge=(down[1]&&!g_escUiKeys[1])||(down[2]&&!g_escUiKeys[2])||(down[5]&&!g_escUiKeys[5])||(down[6]&&!g_escUiKeys[6]);DWORD now=GetTickCount();
            if((down[1]||down[2]||down[5]||down[6])&&!(g_escUiIoBusy&&g_escUiIoKind==3)&&(edge||(LONG)(now-repeatAt)>=0)){
                int step=(GetAsyncKeyState(VK_SHIFT)&0x8000)?10:1;skescgeo::Move(l,g_escUiMulti,(int(down[6])-int(down[5]))*step,(int(down[2])-int(down[1]))*step);repeatAt=now+80u;
            }
        }
    }
    if(!g_escUiSeed){
        if(mouseMode&&hit>=0&&hit!=lastHover)EscUiSound(false);
        else if(!mouseMode&&g_escUiFocus!=oldFocus)EscUiSound(false);
    }
    lastHover=hit;
    memcpy(g_escUiKeys,down,sizeof(down));g_escUiSeed=false;
    if(action>=0){
        if(!g_escUiEditing&&action>=2100&&action<=2111){EscUiPresetAction(action);return;}
        const int oldPick=g_escUiPick,oldFlyout=g_escUiFlyout;
        if(!g_escUiPick&&action>=902&&action<=905){
            if(action==902||action==903){EscUiNavigate(l,action==903);EscUiSelectOne(g_escUiSelected);}
            else{g_escUiFlyoutPage=std::max(0,std::min(pages-1,g_escUiFlyoutPage+(action==904?-1:1)));g_escUiFocus=-1;g_escUiTextDirty=true;}
            return;
        }
        if(g_escUiEditing){
            if(action==920){if(!g_escUiIoBusy)EscUiPickOpen(12);return;}
            if(action==921){g_escUiSnap=!g_escUiSnap;return;}
            if(action==911){if(!g_escUiIoBusy)l.items[g_escUiSelected].opacity=-1;}
            else if(oldPick)EscUiPickerChoose(action);else if(action>=300)EscUiEditorAction(kEscUiEditItems[action-300].action);
            // Controls/view were built before this action. A page flip, erase,
            // load or picker change invalidates that frame's indices. Rebuild
            // on the next Paint rather than render old controls with new state.
            return;
        }
        else if(action==900)EscUiBeginEditor();
        else if(l.items[action].action==skesc::kCvarPresets){g_escUiExtraPanel=1;g_escUiFlyout=-1;g_escUiFlyoutPage=0;return;}
        else if(l.items[action].action==skesc::kServerInfo){const char* field=l.items[action].parameter;
            if(!field[0]||!strcmp(field,"all")||!strcmp(field,"address"))EscUiCopyServer();}
        else if(l.items[action].action==skesc::kMapInfo){EscUiCopyMap();return;}
        else if(l.items[action].action==skesc::kNativeOptions)EscUiActivate(skesc::kNativeOptions);
        else if(l.items[action].action>=skesc::kClock)return;
        else if(l.items[action].action==skesc::kBattleReport){
            EscUiExtraNotice(MainUiOpenBattleReport()?UiText(UiId::EscReportQueued,L"正在打开近期战报"):UiText(UiId::EscReportBusy,L"战报服务忙碌，请稍后再试"));
        }
        else if(skesc::IsMenu(l.items[action].action))EscUiOpenFlyout(action);
        else if(l.items[action].action==skesc::kCommand){
            const char* command=l.items[action].parameter;
            EscUiSuspendInput();EngClientCmdUnrestricted(command);
        }
        else if(!skesc::IsSlider(l.items[action].action))EscUiActivate(l.items[action].action);
        if(!g_escUiDraw||oldPick!=g_escUiPick||oldFlyout!=g_escUiFlyout||&l!=(g_escUiEditing?&g_escUiDraft:&g_escUiLayout))return;
    }
    SurfFillFlush();SurfDrawColorInvalidate();
    if(l.backdrop){SurfColor(0,0,0,l.backdrop*255/100);SurfFill(0,0,g_escUiWidth,g_escUiHeight);}
    auto drawItem=[&](int i,bool interactive){const auto& b=l.items[i];int x=cx+C(b.x),y=cy+C(b.y),w=C(b.w),h=C(b.h);
        bool selected=interactive&&(g_escUiEditing?i==g_escUiSelected:mouseMode?hit==i:g_escUiFocus==i);
        const bool slider=skesc::IsSlider(b.action);const bool muted=slider&&!g_escUiEditing&&!g_escUiSliders[i].writable;
        COLORREF text=b.visible&&!muted?EscUiItemColor(b):RGB(130,134,140);
        if(b.action){
            const int background=b.background<0?l.background:b.background;
            const int opacity=b.opacity<0?l.opacity:b.opacity;
            if(background&&opacity){COLORREF tint=background==1?EscUiItemColor(b):EscUiTextColor(background-2);
                int alpha=opacity*255/100;if(!b.visible)alpha/=3;
                SurfColor(GetRValue(tint),GetGValue(tint),GetBValue(tint),alpha);SurfFill(x,y,x+w,y+h);
            }
            if(l.transparentBorder||(background&&opacity)){
                const COLORREF edge=selected?EscUiAccentColor(l.accent,text):text;
                SurfColor(GetRValue(edge),GetGValue(edge),GetBValue(edge),selected?255:100);
                SurfFill(x,y,x+C(selected?4:2),y+h);
                SurfFill(x,y+h-C(1),x+w,y+h);
            }
        }
        if(!g_escUiEditing&&(b.action==skesc::kServerInfo||(b.action>=skesc::kMapInfo&&b.action<=skesc::kRoundTimer))){
            wchar_t value[192]{};EscUiComponentText(b,value);
            EscUiUiText(112+i,x+C(12),y+std::max(0,(h-g_escUiFontPx[b.font])/2),w-C(24),text,value,b.font);return;
        }
        EscUiText(i,b,x+(b.action?C(selected?24:18):0),y+(slider?C(6):std::max(0,(h-g_escUiFontPx[b.font])/2)),w-(b.action?C(slider?116:54):0),text,g_escUiTextDirty);
        if(slider){float fraction=0;auto& c=g_escUiSliders[i];if(!g_escUiEditing&&c.available)fraction=std::max(0.f,std::min(1.f,(c.value-c.spec.minimum)/(c.spec.maximum-c.spec.minimum)));
            int start=x+C(18),end=x+w-C(18),line=y+h-C(16),knob=start+(int)((end-start)*fraction);SurfColor(220,228,238,80);SurfFill(start,line,end,line+C(2));
            SurfColor(GetRValue(text),GetGValue(text),GetBValue(text),220);SurfFill(start,line,knob,line+C(2));
            SurfFill(knob-C(3),line-C(4),knob+C(3),line+C(6));
            if(!g_escUiEditing)SurfTextAt(g_escUiFonts[2],x+w-C(90),y+C(10),text,c.number);
        }
        if(skesc::IsMenu(b.action))SurfTextAt(g_escUiFonts[2],x+w-C(28),y+C(14),text,L">");
        if(g_escUiEditing&&g_escUiMulti[i]&&!selected){SurfColor(147,197,253,190);SurfOutline(x,y,x+w,y+h);}
        if(g_escUiEditing&&selected){const COLORREF edge=EscUiAccentColor(l.accent,text);SurfColor(GetRValue(edge),GetGValue(edge),GetBValue(edge),255);SurfOutline(x,y,x+w,y+h);SurfFill(x+w-C(10),y+h-C(10),x+w,y+h);}
    };
    if(g_escUiFlyout>=0&&pages>1){
        wchar_t number[32]{};swprintf_s(number,L"%d / %d",g_escUiFlyoutPage+1,pages);
        SurfTextAt(g_escUiFonts[0],cx+C(1620),cy+C(970),RGB(210,224,235),number);
    }
    for(int n=0;n<count;++n)if(!g_escUiExtraPanel||g_escUiEditing||l.items[view[n]].action==skesc::kServerInfo)drawItem(view[n],true);
    if(g_escUiEditing&&down[0]&&g_escUiDrag>=0&&!g_escUiPick){SurfColor(147,197,253,180);
        if(g_escUiGuideX>=0)SurfFill(cx+C(g_escUiGuideX),cy,cx+C(g_escUiGuideX)+1,cy+C(1080));
        if(g_escUiGuideY>=0)SurfFill(cx,cy+C(g_escUiGuideY),cx+C(1920),cy+C(g_escUiGuideY)+1);
    }
    g_escUiTextDirty=false;
    auto drawButton=[&](const Hit& c,const wchar_t* label,int slot,COLORREF ink=RGB(194,225,236)){int x=ox+U(c.x),y=oy+U(c.y);
        SurfColor(GetRValue(ink),GetGValue(ink),GetBValue(ink),hit==c.id?65:24);SurfFill(x,y,x+U(c.w),y+U(c.h));
        SurfColor(GetRValue(ink),GetGValue(ink),GetBValue(ink),hit==c.id?230:90);SurfFill(x,y+U(c.h-1),x+U(c.w),y+U(c.h));
        EscUiUiText(slot,x+U(12),y+U(7),U(c.w-24),ink,label);};
    auto drawOpacity=[&](int x,int y,int w,int h,int opacity,int slot){
        const int percent=100-opacity;wchar_t label[64]{};
        swprintf_s(label,UiText(UiId::EscEdTransparencyValue,L"透明度：%d%%"),percent);
        EscUiUiText(slot,ox+U(x),oy+U(y+(h-22)/2),U(224),RGB(194,225,236),label);
        int start=ox+U(x+236),end=ox+U(x+w-124),line=oy+U(y+h/2),knob=start+(end-start)*percent/100;
        SurfColor(194,225,236,75);SurfFill(start,line,end,line+U(2));
        SurfColor(105,222,235,255);SurfFill(start,line,knob,line+U(2));SurfFill(knob-U(4),line-U(6),knob+U(4),line+U(8));
    };
    if(!g_escUiEditing)drawButton(controls[0],g_escUiDraftAvailable?UiText(UiId::EscEdResume,L"继续编辑草稿"):UiText(UiId::EscEdRoot,L"编辑 ESC 布局"),24);
    else{
        SurfColor(18,22,29,245);SurfFill(ox+U(1280),oy+U(16),ox+U(1920),oy+U(1080));
        if(!g_escUiPick){for(int n=0;n<controlCount;++n){const auto& c=controls[n];
            if(c.id==920)drawButton(c,UiText(UiId::EscArrangeTools,L"对齐与尺寸（Ctrl 点击多选）"),100);
            if(c.id==921)drawButton(c,g_escUiSnap?UiText(UiId::EscSnapOn,L"吸附：开启 / Alt 暂停"):UiText(UiId::EscSnapOff,L"吸附：关闭"),101);
        }}
        EscUiUiText(0,ox+U(1300),oy+U(38),U(600),RGB(255,199,86),UiText(UiId::EscEdTitle,L"拖动移动 / 右下角缩放 / 方向键微调"));
        wchar_t geometry[128]{};const auto& b=l.items[g_escUiSelected];swprintf_s(geometry,UiText(UiId::EscGeometrySelection,L"X %d  Y %d  W %d  H %d  /  选中 %d"),b.x,b.y,b.w,b.h,skescgeo::Count(l,g_escUiMulti));
        EscUiUiText(1,ox+U(1300),oy+U(78),U(600),RGB(210,214,222),geometry);
        // Keep properties readable behind a picker; only picker rectangles get input.
        for(int i=0;i<(int)_countof(kEscUiEditItems);++i){auto& item=kEscUiEditItems[i];wchar_t label[192]{};
            if(item.action==EscUiEditAction::Opacity){int y=118+i*32;drawOpacity(1300,y,600,30,b.opacity<0?l.opacity:b.opacity,2+i);
                Hit inherit{1790,y,110,30,911};drawButton(inherit,UiText(UiId::EscEdOpacityInherit,L"继承"),79,b.opacity<0?RGB(137,224,175):RGB(194,225,236));continue;}
            switch(item.action){
            case EscUiEditAction::Next:swprintf_s(label,UiText(item.id,item.chinese),g_escUiSelected+1,l.count);break;
            case EscUiEditAction::Action:swprintf_s(label,UiText(item.id,item.chinese),EscUiActionLabel(b.action));break;
            case EscUiEditAction::Parent:{wchar_t name[128]{};swprintf_s(label,UiText(item.id,item.chinese),b.parent<0?UiText(UiId::EscEdHomepage,L"首页"):EscUiLabel(l.items[b.parent],name));break;}
            case EscUiEditAction::Visible:swprintf_s(label,UiText(item.id,item.chinese),b.visible?UiText(UiId::EscEdYes,L"开启"):UiText(UiId::EscEdNo,L"隐藏"));break;
            case EscUiEditAction::Color:{wchar_t rgb[48]{};COLORREF color=EscUiItemColor(b);
                swprintf_s(rgb,L"RGB %d %d %d",GetRValue(color),GetGValue(color),GetBValue(color));swprintf_s(label,UiText(item.id,item.chinese),rgb);break;}
            case EscUiEditAction::Accent:swprintf_s(label,UiText(item.id,item.chinese),EscUiAccentLabel(l.accent));break;
            case EscUiEditAction::Font:swprintf_s(label,UiText(item.id,item.chinese),b.font?UiText(UiId::EscEdTitleSize,L"标题"):UiText(UiId::EscEdBodySize,L"按钮"));break;
            case EscUiEditAction::Profile:swprintf_s(label,UiText(item.id,item.chinese),g_escUiProfile);break;
            default:wcsncpy_s(label,UiText(item.id,item.chinese),_TRUNCATE);break;}
            if(item.action==EscUiEditAction::Apply&&g_escUiConfirmCurrent)wcscpy_s(label,UiText(UiId::EscEdConfirmCurrent,L"确认替换损坏的当前布局"));
            if(item.action==EscUiEditAction::SaveProfile&&g_escUiConfirmProfile)wcscpy_s(label,UiText(UiId::EscEdConfirmProfile,L"确认保存（覆盖该方案槽位）"));
            Hit row{1300,118+i*32,600,30,300+i};drawButton(row,label,2+i);
        }
        EscUiUiText(54,ox+U(1300),oy+U(982),U(600),RGB(255,199,86),EscUiStatusText());
        EscUiUiText(55,ox+U(1300),oy+U(1024),U(600),RGB(190,196,206),UiText(UiId::EscEdHint,L"应用后保存；ESC 暂离保留本次草稿。"));
        if(g_escUiPick){
            SurfColor(12,16,22,255);SurfFill(ox+U(128),oy+U(128),ox+U(1230),oy+U(960));
            const UiTextEntry titles[]={{UiId::EscEdElements,L"选择元素"},{UiId::EscEdActions,L"选择动作"},{UiId::EscEdColors,L"选择文字颜色"},{UiId::EscEdFonts,L"选择字号类型"},{UiId::EscEdSlots,L"选择方案槽位"},{UiId::EscEdSliderFields,L"ConVar：变量名 / 最小值 / 最大值 / 步进"},{UiId::EscEdPickParent,L"选择所属菜单"},{UiId::EscEdBackground,L"按钮底色 / 统一配色 / 透明度"},{UiId::EscEdBgItem,L"当前按钮背景"},{UiId::EscEdBgUniform,L"统一背景颜色"},{UiId::EscEdBgOpacity,L"统一背景透明度（0% 不透明，100% 透明）"},{UiId::EscArrangeTitle,L"对齐与尺寸"}};
            EscUiUiText(56,ox+U(160),oy+U(168),U(840),RGB(255,199,86),UiText(titles[g_escUiPick-1].id,titles[g_escUiPick-1].chinese));
            for(int i=0;i<controlCount;++i){const auto& c=controls[i];wchar_t label[192]{};
                if(c.id==912){drawOpacity(160,352,1040,52,l.opacity,50);continue;}
                if(c.id==2000)wcscpy_s(label,UiText(UiId::EscEdClose,L"关闭选择"));
                else if(c.id==2001)wcscpy_s(label,UiText(UiId::EscEdPrevious,L"上一页"));
                else if(c.id==2002)wcscpy_s(label,UiText(UiId::EscEdFollowing,L"下一页"));
                else{int n=c.id-1000+g_escUiPickPage*16;
                    if(g_escUiPick==1){wchar_t converted[128]{};swprintf_s(label,L"%d. %s%s",n+1,EscUiLabel(l.items[n],converted),l.items[n].visible?L"":UiText(UiId::EscEdHiddenSuffix,L" (隐藏)"));}
                    else if(g_escUiPick==2)wcscpy_s(label,EscUiActionLabel(n));
                    else if(g_escUiPick==3)wcscpy_s(label,n==7?UiText(UiId::EscEdRgbInput,L"输入 RGB：R G B，每项 0 到 255"):EscUiColorLabel(n));
                    else if(g_escUiPick==4)wcscpy_s(label,n?UiText(UiId::EscEdTitleSize,L"标题"):UiText(UiId::EscEdBodySize,L"按钮"));
                    else if(g_escUiPick==5)swprintf_s(label,UiText(UiId::EscEdSlotChoice,L"方案槽位 %d"),n+1);
                    else if(g_escUiPick==7){int parent=EscUiParentChoice(n);wchar_t name[128]{};wcscpy_s(label,parent<0?UiText(UiId::EscEdHomepage,L"首页"):EscUiLabel(l.items[parent],name));}
                    else if(g_escUiPick==8){
                        if(n==0)swprintf_s(label,UiText(UiId::EscEdBgItemValue,L"当前按钮背景：%s"),EscUiBackgroundLabel(b.background));
                        else if(n==1)swprintf_s(label,UiText(UiId::EscEdBgUniformValue,L"统一背景：%s"),EscUiBackgroundLabel(l.background));
                        else if(n==2)swprintf_s(label,UiText(UiId::EscEdBgOpacityValue,L"统一背景透明度：%d%%"),100-l.opacity);
                        else if(n==3)wcscpy_s(label,UiText(UiId::EscEdBgAll,L"所有按钮使用统一背景及透明度"));
                        else swprintf_s(label,UiText(UiId::EscEdTransparentBorder,L"完全透明时显示边框：%s"),l.transparentBorder?UiText(UiId::EscEdYes,L"开启"):UiText(UiId::EscEdNo,L"隐藏"));
                    }
                    else if(g_escUiPick==12){const UiTextEntry tools[]={
                        {UiId::EscAlignLeft,L"左边对齐"},{UiId::EscAlignCenter,L"水平居中对齐"},{UiId::EscAlignRight,L"右边对齐"},
                        {UiId::EscAlignTop,L"顶边对齐"},{UiId::EscAlignMiddle,L"垂直居中对齐"},{UiId::EscAlignBottom,L"底边对齐"},
                        {UiId::EscMatchWidth,L"统一宽度"},{UiId::EscMatchHeight,L"统一高度"},{UiId::EscMatchSize,L"统一宽高"},
                        {UiId::EscSpaceHorizontal,L"水平等间距"},{UiId::EscSpaceVertical,L"垂直等间距"}};wcscpy_s(label,UiText(tools[n].id,tools[n].chinese));}
                    else if(g_escUiPick==9||g_escUiPick==10)wcscpy_s(label,EscUiBackgroundLabel(n-(g_escUiPick==9?1:0)));
                    else if(n==4)wcscpy_s(label,UiText(UiId::EscEdApplyParameters,L"应用这些参数"));
                    else{const UiTextEntry fields[]={{UiId::MainUiCvarName,L"变量名"},{UiId::MainUiCvarMin,L"最小值"},{UiId::MainUiCvarMax,L"最大值"},{UiId::MainUiCvarStep,L"步进"}};swprintf_s(label,L"%s: %s",UiText(fields[n].id,fields[n].chinese),g_escUiCvarFields[n]);}
                }
                COLORREF ink=RGB(194,225,236);
                if(c.id>=1000&&c.id<1016){int n=c.id-1000+g_escUiPickPage*16;
                    if(g_escUiPick==3)ink=n==7?EscUiItemColor(b):EscUiTextColor(n);
                    if((g_escUiPick==9&&n>=3)||(g_escUiPick==10&&n>=2))ink=EscUiTextColor(n-(g_escUiPick==9?3:2));
                }
                drawButton(c,label,57+i,ink);
            }
            EscUiUiText(77,ox+U(160),oy+U(940),U(1020),RGB(190,196,206),(g_escUiPick==12?UiText(UiId::EscArrangeHint,L"对齐/尺寸以当前项为基准；等间距保留两端位置，至少选三项。"):UiText(UiId::EscEdPickHint,L"直接点选；选择期间不会移动元素或执行游戏动作。")));
        }
    }
    if(!g_escUiEditing&&(g_escUiStatus==EscUiStatus::PinError||g_escUiStatus==EscUiStatus::PinDeferred))
        EscUiUiText(78,ox+U(850),oy+U(112),U(940),RGB(255,199,86),EscUiStatusText());
    for(int i=0;i<controlCount;++i){const auto& c=controls[i];const wchar_t* label=nullptr;
        if(c.id==902)label=UiText(UiId::EscEdBackMenu,L"返回上一级");
        else if(c.id==903)label=UiText(UiId::EscEdHomepage,L"首页");
        else if(c.id==904)label=UiText(UiId::EscEdPrevious,L"上一页");
        else if(c.id==905)label=UiText(UiId::EscEdFollowing,L"下一页");
        if(label)drawButton(c,label,30+i%18);
    }
    if(!g_escUiEditing&&g_escUiExtraPanel){
        EscUiUiText(98,ox+U(460),oy+U(176),U(980),RGB(238,240,245),UiText(UiId::EscCvarPresets,L"ConVar 预设"));
        for(int i=0;i<controlCount;++i){const auto& c=controls[i];if(c.id<2100||c.id>2111)continue;
            wchar_t label[192]{};const wchar_t* text=nullptr;COLORREF ink=RGB(194,225,236);int slot=94;
            if(c.id<2108){int n=c.id-2100;const auto& preset=skescpreset::bank.slots[n];
                swprintf_s(label,UiText(UiId::EscPresetTile,L"%d / %s / %d 项"),n+1,preset.name[0]?preset.name:UiText(UiId::EscPresetUnnamed,L"未命名"),preset.count);
                text=label;slot=85+n;if(n==g_escUiPresetSlot)ink=RGB(255,199,86);
            }else{slot=94+c.id-2108;
                if(c.id==2108)text=UiText(UiId::EscPresetExecute,L"执行所选预设");
                if(c.id==2109)text=UiText(UiId::EscPresetCapture,L"保存 / 覆盖所选槽：当前滑块值");
                if(c.id==2110)text=UiText(UiId::EscPresetRename,L"命名 ConVar 预设");
                if(c.id==2111)text=UiText(UiId::EscPresetClose,L"关闭预设页");
            }
            if(text)drawButton(c,text,slot,ink);
        }
        EscUiUiText(99,ox+U(460),oy+U(900),U(980),RGB(190,196,206),UiText(UiId::EscPresetHint,L"先选择槽位；保存当前布局全部滑块的实际值。预设不会自动执行。"));
    }
    if(!g_escUiEditing&&g_escUiExtraNotice[0]&&(LONG)(g_escUiExtraNoticeUntil-GetTickCount())>0)
        EscUiUiText(93,ox+U(440),oy+U(1024),U(1040),RGB(255,199,86),g_escUiExtraNotice);
    SurfFillFlush();SurfDrawColorInvalidate();
}
