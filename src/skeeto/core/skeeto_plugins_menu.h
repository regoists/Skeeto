#pragma once
// Menu state is separate from the public ABI and from all built-in feature pages.
static unsigned g_pluginMenuSlot=0,g_pluginMenuDepth=0;
static int g_pluginMenuSelection=-1;
static int g_pluginMenuRestoreSub=-1;
struct PluginMenuFrame { uint32_t page;int sub,selection; };
static PluginMenuFrame g_pluginMenuStack[skplugins::kMaxDepth];
static bool g_pluginMenuEnglish=false;
static const wchar_t* PluginStateText(skplugins::State state) {
    switch(state){
    case skplugins::State::Loaded:return UiText(UiId::PluginLoaded,L"已载入，等待初始化");
    case skplugins::State::Active:return UiText(UiId::PluginActive,L"运行中");
    case skplugins::State::LoadError:return UiText(UiId::PluginLoadError,L"装载失败（镜像或依赖）");
    case skplugins::State::EntryMissing:return UiText(UiId::PluginEntryMissing,L"已载入，缺少插件入口");
    case skplugins::State::ApiMismatch:return UiText(UiId::PluginApiMismatch,L"已载入，API 不兼容");
    case skplugins::State::DuplicateId:return UiText(UiId::PluginDuplicateId,L"已载入，插件 ID 重复");
    case skplugins::State::InvalidDescriptor:return UiText(UiId::PluginInvalidDescriptor,L"已载入，描述无效");
    case skplugins::State::InitError:return UiText(UiId::PluginInitError,L"初始化失败");
    case skplugins::State::PageError:return UiText(UiId::PluginPageError,L"菜单数据或回调失败");
    case skplugins::State::ActionError:return UiText(UiId::PluginActionError,L"操作回调失败");
    }
    return L"";
}
static const wchar_t* PluginMenuName(const skplugins::Plugin& plugin) {
    const auto* name=g_uiEnglish.load(std::memory_order_relaxed)?plugin.nameEn:plugin.nameZh;
    return name[0]?name:plugin.file;
}
static bool PluginMenuSnapshotReady() {
    const auto& page=skplugins::CurrentPage();return page.valid && page.plugin==g_pluginMenuSlot;
}
static int PluginMenuPageCount(int page) {
    const int count=page==kMenuPagePluginPlatform?1+(int)skplugins::Count()
        :PluginMenuSnapshotReady()?(int)skplugins::CurrentPage().count:0;
    return MenuPageCountForItems(count);
}
static bool PluginMenuRefresh() {
    if(g_menuPage!=kMenuPagePluginPage)return false;
    const uint32_t page=PluginMenuSnapshotReady()?skplugins::CurrentPage().page_id:0;
    return skplugins::RequestPage(g_pluginMenuSlot,page);
}
static void PluginMenuPaintPump(int mode) {
    if(!(mode&kPaintUiPanels) || !g_run.load(std::memory_order_relaxed) || !g_ready.load(std::memory_order_relaxed))return;
    const bool english=g_uiEnglish.load(std::memory_order_relaxed);
    if(g_menuVisible && !g_menuParked && g_menuPage==kMenuPagePluginPage
        && PluginMenuSnapshotReady() && g_pluginMenuEnglish!=english && !skplugins::Pending())PluginMenuRefresh();
    // The host's default-off/idle path is one atomic load, no plugin polling.
    skplugins::PaintPump(english);
    if(PluginMenuSnapshotReady())g_pluginMenuEnglish=english;
    if(g_menuPage==kMenuPagePluginPage && PluginMenuSnapshotReady() && !skplugins::Pending() && g_pluginMenuRestoreSub>=0){
        g_menuSub=g_pluginMenuRestoreSub;g_pluginMenuRestoreSub=-1;MenuMarkDirty();
    }
    if(g_menuPage==kMenuPagePluginPage && PluginMenuSnapshotReady()){
        const int pages=PluginMenuPageCount(kMenuPagePluginPage);
        if(g_menuSub>=pages){g_menuSub=pages-1;MenuMarkDirty();}
    }
}
static void PluginMenuEnter(unsigned slot) {
    g_pluginMenuSlot=slot;g_pluginMenuDepth=0;g_pluginMenuSelection=-1;g_pluginMenuRestoreSub=-1;
    g_menuPage=kMenuPagePluginPage;g_menuSub=0;
    skplugins::RequestPage(slot,0);MenuMarkDirty();
}
static void PluginMenuStep(int digit) {
    if(g_menuPage==kMenuPagePluginPlatform){
        if(digit==0){g_menuPage=g_menuPagePrev=0;g_menuSub=2;return;}
        const int index=g_menuSub*kMenuItemsPerPage+digit-1;
        if(index==0){g_optPluginPlatform=!g_optPluginPlatform;g_settingsDirty=true;g_settingsSaveAt=GetTickCount()+400;MenuMarkDirty();}
        else if(index>0 && index<=int(skplugins::Count()))PluginMenuEnter((unsigned)index-1);
        return;
    }
    if(digit==0){
        if(skplugins::Pending())return;
        // A failed callback is no longer callable. Leave its navigation stack
        // instead of waiting for a parent-page request that must be rejected.
        if(g_pluginMenuDepth && skplugins::At(g_pluginMenuSlot)->state!=skplugins::State::Active){
            g_pluginMenuDepth=0;g_pluginMenuRestoreSub=-1;
        }
        if(g_pluginMenuDepth){
            const auto frame=g_pluginMenuStack[g_pluginMenuDepth-1];
            if(skplugins::RequestPage(g_pluginMenuSlot,frame.page)){--g_pluginMenuDepth;g_menuSub=0;g_pluginMenuRestoreSub=frame.sub;g_pluginMenuSelection=frame.selection;}
        }else {g_menuPage=g_menuPagePrev=kMenuPagePluginPlatform;g_menuSub=int(g_pluginMenuSlot+1)/kMenuItemsPerPage;}
        MenuMarkDirty();return;
    }
    if(!PluginMenuSnapshotReady() || skplugins::Pending())return;
    const auto& page=skplugins::CurrentPage();const unsigned index=g_menuSub*kMenuItemsPerPage+digit-1;
    if(index>=page.count)return;
    const auto& item=page.items[index];g_pluginMenuSelection=(int)index;MenuMarkDirty();
    if(item.flags&SKT_MENU_DISABLED)return;
    if(item.type==SKT_MENU_SUBMENU){
        if(g_pluginMenuDepth==skplugins::kMaxDepth)return;
        const PluginMenuFrame frame{page.page_id,g_menuSub,g_pluginMenuSelection};
        if(skplugins::RequestPage(g_pluginMenuSlot,item.targetPage)){
            g_pluginMenuStack[g_pluginMenuDepth++]=frame;g_menuSub=0;g_pluginMenuSelection=-1;
        }
    }else if(item.type==SKT_MENU_ACTION)skplugins::RequestAction(item.id,0);
    else if(item.type==SKT_MENU_TOGGLE)skplugins::RequestAction(item.id,item.value?0:1);
    else if(item.type==SKT_MENU_CHOICE)skplugins::RequestAction(item.id,fmod(item.value+1,item.choice_count));
    // Number keys select numeric controls; only +/- submits a new value.
}
static void PluginMenuAdjust(int delta) {
    if(!PluginMenuSnapshotReady() || skplugins::Pending() || g_pluginMenuSelection<0
        || g_pluginMenuSelection/kMenuItemsPerPage!=g_menuSub)return;
    const auto& page=skplugins::CurrentPage();if((unsigned)g_pluginMenuSelection>=page.count)return;
    const auto& row=page.items[g_pluginMenuSelection];if(row.type!=SKT_MENU_NUMBER || (row.flags&SKT_MENU_DISABLED))return;
    const double value=fmin(row.max,fmax(row.min,row.value+delta*row.step));
    if(value!=row.value)skplugins::RequestAction(row.id,value);
}
static void PluginMenuValue(const skplugins::Item& row,wchar_t (&out)[160]) {
    out[0]=0;
    if(row.type==SKT_MENU_TOGGLE){wcscpy_s(out,row.value?UiText(UiId::Text_10ffed01,L"【开】"):UiText(UiId::Text_d50b7932,L"【关】"));return;}
    if(row.type==SKT_MENU_NUMBER){swprintf_s(out,L"%.6g  - / +",row.value);return;}
    if(row.type==SKT_MENU_SUBMENU){wcscpy_s(out,UiText(UiId::PluginSubmenu,L"【进入设置】"));return;}
    if(row.type==SKT_MENU_CHOICE){
        const wchar_t* first=row.choices;for(unsigned i=0;i<(unsigned)row.value;++i)first=wcschr(first,L'|')+1;
        const wchar_t* end=wcschr(first,L'|');const size_t n=end?size_t(end-first):wcslen(first);
        wcsncpy_s(out,first,n<159?n:159);return;
    }
    wcscpy_s(out,row.text);
}
template<class Line,class Rule,class Footer,class Pager>
static void PluginMenuPaint(Line line,Rule rule,Footer footer,Pager pager,COLORREF cTitle,COLORREF cText,COLORREF cHint,COLORREF cMuted,COLORREF cAdjust) {
    wchar_t text[320]{};
    if(g_menuPage==kMenuPagePluginPlatform){
        line(10,cTitle,UiText(UiId::PluginPlatformTitle,L"Skeeto插件平台"));rule();
        const int count=1+(int)skplugins::Count();
        for(int row=0;row<kMenuItemsPerPage;++row){
            const int index=g_menuSub*kMenuItemsPerPage+row;if(index>=count)break;
            if(index==0){
                swprintf_s(text,UiText(UiId::PluginStartupOption,L"[%d] 启动时加载插件 %s"),row+1,
                    g_optPluginPlatform?UiText(UiId::Text_10ffed01,L"【开】"):UiText(UiId::Text_d50b7932,L"【关】"));
                line(52+row*30,g_optPluginPlatform?RGB(50,255,50):RGB(255,50,50),text);
            }else {const auto& plugin=*skplugins::At((unsigned)index-1);
                swprintf_s(text,L"[%d] %s %s  [%s]",row+1,PluginMenuName(plugin),plugin.version,PluginStateText(plugin.state));line(52+row*30,cText,text);}
        }
        int hintY=SurfWrapDesign(338,cHint,UiText(UiId::PluginRestartHint,L"装载开关下次启动生效；已载入插件需完全退出游戏才会移除。"),MenuHintGap());
        hintY=SurfWrapDesign(hintY,cHint,UiText(UiId::PluginLaunchHint,L"启动项 -skeeto_plugin_platform 0/1 只覆盖本次启动，不改保存的开关。"),MenuHintGap());
        line(hintY,cHint,skplugins::Loading()?UiText(UiId::PluginLoading,L"正在装载插件")
            :skplugins::Started()?UiText(UiId::PluginSessionOn,L"本进程已启动平台；入口 DLL 放在 bin\\skeeto\\plugins\\。")
            :UiText(UiId::PluginSessionOff,L"本进程未启动平台，不会扫描插件目录或执行插件。"));
        hintY+=MenuDesignLineH()+6;
        if(skplugins::ScanError()){swprintf_s(text,UiText(UiId::PluginScanError,L"插件目录读取失败，系统错误码：%u"),skplugins::ScanError());line(hintY,cAdjust,text);}
        else if(skplugins::Limited())line(hintY,cAdjust,UiText(UiId::PluginLimitHint,L"入口数量超过上限；本次最多处理 32 个 DLL。"));
        else if(!skplugins::Count() && skplugins::Started() && !skplugins::Loading())line(hintY,cMuted,UiText(UiId::PluginEmpty,L"未发现插件入口 DLL。"));
        pager(PluginMenuPageCount(g_menuPage));footer();return;
    }
    const auto* plugin=skplugins::At(g_pluginMenuSlot);
    if(!plugin){line(10,cTitle,UiText(UiId::PluginPlatformTitle,L"Skeeto插件平台"));rule();footer();return;}
    const bool ready=PluginMenuSnapshotReady();const auto& page=skplugins::CurrentPage();
    line(10,cTitle,ready?page.title:PluginMenuName(*plugin));rule();
    if(ready && !skplugins::Pending())for(int row=0;row<kMenuItemsPerPage;++row){
        const int index=g_menuSub*kMenuItemsPerPage+row;if(index>=int(page.count))break;
        const auto& item=page.items[index];wchar_t value[160]{};PluginMenuValue(item,value);
        swprintf_s(text,L"[%d] %s%s  %s",row+1,index==g_pluginMenuSelection?L"> ":L"",item.label,value);
        COLORREF color=cText;
        if(item.flags&SKT_MENU_DISABLED || item.type==SKT_MENU_INFO)color=cMuted;
        else if(item.type==SKT_MENU_NUMBER)color=cAdjust;
        else if(item.type==SKT_MENU_TOGGLE)color=item.value?RGB(50,255,50):RGB(255,50,50);
        line(52+row*30,color,text);
    }
    else line(52,cMuted,skplugins::Pending()?UiText(UiId::PluginWorking,L"正在更新插件菜单")
        :plugin->hasMenu?PluginStateText(plugin->state):UiText(UiId::PluginNoMenu,L"此插件未提供菜单页。"));
    swprintf_s(text,UiText(UiId::PluginAttribution,L"作者：%s    版本：%s"),plugin->author,plugin->version);line(338,cHint,text);
    swprintf_s(text,UiText(UiId::PluginResult,L"状态：%s    返回码：%u"),PluginStateText(plugin->state),plugin->error);line(371,cHint,text);
    line(404,cHint,UiText(UiId::PluginKeysHint,L"数字键操作；数值项先选中再 -/+；R 刷新；0 返回。"));
    pager(PluginMenuPageCount(g_menuPage));footer();
}
