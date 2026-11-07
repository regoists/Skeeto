#pragma once
struct NativeUiMenuItem { UiId label; const wchar_t* chinese; sknativeui::Field field; int targets; };
static constexpr NativeUiMenuItem kNativeUiMenuItems[]={
    {UiId::NativeUiEnabled,L"自定义样式",sknativeui::Enabled,3},
    {UiId::NativeUiSayTime,L"消息停留秒数",sknativeui::SayTime,1},
    {UiId::NativeUiFontOverride,L"覆盖原生字体",sknativeui::FontOverride,3},
    {UiId::NativeUiFontFace,L"字体",sknativeui::FontFace,3},
    {UiId::NativeUiFontSize,L"字号",sknativeui::FontSize,3},
    {UiId::NativeUiBold,L"粗体",sknativeui::Bold,3},
    {UiId::NativeUiShadow,L"文字阴影",sknativeui::Shadow,3},
    {UiId::NativeUiOutline,L"文字描边",sknativeui::Outline,3},
    {UiId::NativeUiAutoScale,L"字号跟随分辨率缩放",sknativeui::AutoScale,3},
    {UiId::NativeUiChatText,L"聊天正文颜色",sknativeui::Text,1},
    {UiId::NativeUiText,L"命令回显颜色",sknativeui::Text,2},
    {UiId::NativeUiInput,L"输入文字颜色",sknativeui::Input,3},
    {UiId::NativeUiChatBackground,L"聊天底色（输入时）",sknativeui::Background,1},
    {UiId::NativeUiBackground,L"背景颜色",sknativeui::Background,2},
    {UiId::NativeUiAlphaStep,L"不透明度调节步进",sknativeui::AlphaStep,3},
    {UiId::NativeUiChatBgAlpha,L"聊天底色不透明度（输入时）",sknativeui::BgAlpha,1},
    {UiId::NativeUiBgAlpha,L"背景不透明度",sknativeui::BgAlpha,2},
    {UiId::NativeUiInputBackground,L"输入框背景颜色",sknativeui::InputBackground,3},
    {UiId::NativeUiInputBgAlpha,L"输入框背景不透明度",sknativeui::InputBgAlpha,3},
    {UiId::NativeUiBorder,L"原生主题边框",sknativeui::Border,3},
    {UiId::NativeUiGeometry,L"覆盖原生位置与大小",sknativeui::Geometry,1},
    {UiId::NativeUiPositionEntry,L"调整聊天框位置 【进入设置】",sknativeui::X,1},
    {UiId::NativeUiWidth,L"宽度（屏幕百分比）",sknativeui::Width,1},
    {UiId::NativeUiHeight,L"高度（屏幕百分比）",sknativeui::Height,1},
    {UiId::NativeUiSurvivor,L"生还者名称颜色",sknativeui::Survivor,1},
    {UiId::NativeUiInfected,L"感染者名称颜色",sknativeui::Infected,1},
    {UiId::NativeUiDeveloper,L"开发输出颜色",sknativeui::Developer,2},
    {UiId::NativeUiTitleText,L"标题栏文字颜色",sknativeui::TitleText,2},
    {UiId::NativeUiTitleBackground,L"标题栏背景颜色",sknativeui::TitleBackground,2},
    {UiId::NativeUiReset,L"恢复本页默认样式",sknativeui::Count,3},
};
static int g_nativeUiMenuSelection[2]={0,0};
static int NativeUiMenuTarget(int page) {return page==kMenuPageChatUi?0:page==kMenuPageConsoleUi?1:-1;}
static int NativeUiMenuCount(int target) {
    int n=0;for(const auto& item:kNativeUiMenuItems)if(item.targets&(1<<target))++n;return n;
}
static const NativeUiMenuItem* NativeUiMenuItemAt(int target,int index) {
    for(const auto& item:kNativeUiMenuItems)if(item.targets&(1<<target)){if(index--==0)return &item;}return nullptr;
}
static void NativeUiChanged(int target,const sknativeui::Config& c) {
    NativeUiStore(target,c);g_settingsDirty=true;g_settingsSaveAt=GetTickCount()+400;MenuMarkDirty();
}
static void NativeUiMenuStep(int target,int index,int delta,bool clicked) {
    using namespace sknativeui;
    const auto* item=NativeUiMenuItemAt(target,index);if(!item)return;
    g_nativeUiMenuSelection[target]=index;const Field f=item->field;
    Config c=NativeUiConfig(target);
    if(f==X){if(clicked){g_menuPage=kMenuPageChatUiPosition;g_menuSub=0;MenuMarkDirty();}return;}
    if(f==Count){if(clicked){const int on=c.v[Enabled];c=Defaults(target);c.v[Enabled]=on;NativeUiChanged(target,c);}return;}
    if(clicked && !Toggle(f) && !Color(f) && f!=FontFace){MenuMarkDirty();return;}
    if(Toggle(f))c.v[f]=!c.v[f];
    else if(Color(f)){
        static constexpr int palette[]={0xe0eaf4,0x99ccff,0x9bd6b0,0xffca80,0xe9a8bf,0xcab9ef,0x101820,0x000000};
        int at=delta>0?-1:0;for(int i=0;i<(int)_countof(palette);++i)if(c.v[f]==palette[i])at=i;
        c.v[f]=palette[(at+delta+(int)_countof(palette))%_countof(palette)];
    }else if(f==FontFace)c.v[f]=(c.v[f]+delta+4)%4;
    else if(f==BgAlpha || f==InputBgAlpha)c.v[f]=Clamp(f,c.v[f]+delta*c.v[AlphaStep]);
    else c.v[f]=Clamp(f,c.v[f]+delta);
    NativeUiChanged(target,c);
}
static void NativeUiMenuAdjust(int target,int delta) {
    const int selected=g_nativeUiMenuSelection[target];
    if(selected/kMenuItemsPerPage!=g_menuSub)return;
    NativeUiMenuStep(target,selected,delta,false);
}
static void NativeUiMenuNudge(int dx,int dy,bool fast) {
    auto c=NativeUiConfig(0);if(sknativeui::Nudge(c,dx,dy,fast))NativeUiChanged(0,c);
}
static void NativeUiMenuResetPosition() {
    auto c=NativeUiConfig(0);const auto defaults=sknativeui::Defaults(0);
    c.v[sknativeui::X]=defaults.v[sknativeui::X];c.v[sknativeui::Y]=defaults.v[sknativeui::Y];
    c.v[sknativeui::Geometry]=1;NativeUiChanged(0,c);
}
static void NativeUiMenuValue(const sknativeui::Config& c,sknativeui::Field f,wchar_t (&out)[96]) {
    using namespace sknativeui;
    if(f==Count){out[0]=0;return;}
    if(f==X){swprintf_s(out,L"X %d%%  Y %d%%",c.v[X],c.v[Y]);return;}
    if(Toggle(f)){wcscpy_s(out,c.v[f]?UiText(UiId::Text_10ffed01,L"【开】"):UiText(UiId::Text_d50b7932,L"【关】"));return;}
    if(f==SayTime && c.v[f]==0){wcscpy_s(out,UiText(UiId::NativeUiSayTimeHidden,L"0（隐藏消息，输入时可查看）"));return;}
    if(Color(f)){const int v=c.v[f];swprintf_s(out,L"RGB %d %d %d",v>>16,(v>>8)&255,v&255);return;}
    if(f==FontFace){static const wchar_t* faces[]={L"Microsoft YaHei",L"SimHei",L"Consolas",L"Tahoma"};wcscpy_s(out,faces[c.v[f]]);return;}
    swprintf_s(out,L"%d%s",c.v[f],f==BgAlpha || f==InputBgAlpha || f==AlphaStep || (f>=X && f<=Height)?L"%":L"");
}
static const wchar_t* NativeUiStatusText(int target) {
    switch(g_nativeUiStatus[target].load(std::memory_order_relaxed)){
    case 1:return UiText(UiId::NativeUiWaiting,L"等待原生控件出现（打开聊天 / 控制台）");
    case 2:return UiText(UiId::NativeUiActive,L"正在应用原生控件样式");
    case 3:return UiText(UiId::NativeUiUnsupported,L"部分接口与当前游戏不匹配，相关属性未应用");
    case 4:return UiText(UiId::NativeUiFault,L"控件重建或字体应用失败；请重新打开后检查");
    default:return UiText(UiId::NativeUiOff,L"关闭时使用接管前的原生样式");
    }
}
