// Homepage editor nest. Paint thread only; drafts never dispatch game actions.
// Fixed storage and cached titles; existing fonts, input gate and settings writer.
#pragma once

struct MainUiActionDef {int action;UiTextEntry title;};
static constexpr MainUiActionDef kMainUiActions[]={
 {100,{UiId::MainUiModeCoop,L"战役"}},
 {101,{UiId::MainUiModeRealism,L"写实"}},
 {102,{UiId::MainUiModeVersus,L"对抗"}},
 {103,{UiId::MainUiModeRealismVersus,L"写实对抗"}},
 {104,{UiId::MainUiModeSurvival,L"生存"}},
 {105,{UiId::MainUiModeSurvivalVersus,L"生存对抗"}},
 {106,{UiId::MainUiModeScavenge,L"清道夫"}},
 {107,{UiId::MainUiModeMutation,L"突变"}},
 {108,{UiId::MainUiSoloMode,L"单人游戏"}},
 {0,{UiId::MainUiCreateLobby,L"快速创建大厅"}},
 {916,{UiId::MainUiParamLobby,L"创建大厅（预设参数）"}},
 {911,{UiId::MainUiParamQuick,L"指定模式快速匹配"}},
 {912,{UiId::MainUiParamCustom,L"指定模式联机游戏"}},
 {913,{UiId::MainUiParamFriends,L"指定模式好友游戏"}},
 {924,{UiId::MainUiParamTeam,L"指定模式团队好友游戏"}},
 {914,{UiId::MainUiParamGroup,L"指定模式组服务器"}},
 {915,{UiId::MainUiParamSolo,L"指定模式单人选图"}},
 {918,{UiId::MainUiParamMap,L"地图 / 模式 / 难度启动"}},
 {900,{UiId::MainUiFavoriteServer,L"服务器条目"}},
 {217,{UiId::MainUiFavoritesListAction,L"收藏服务器列表"}},
 {209,{UiId::MainUiRecentTab,L"服务器游玩记录"}},
 {1,{UiId::MainUiServers,L"服务器浏览器"}},
 {205,{UiId::MainUiGroupServer,L"Steam 组服务器"}},
 {901,{UiId::MainUiFavoriteMap,L"收藏地图"}},
 {220,{UiId::MainUiOfficialMaps,L"官图快速入口"}},
 {206,{UiId::MainUiTestMaps,L"快速测试图"}},
 {200,{UiId::MainUiGameOptions,L"游戏设置"}},
 {917,{UiId::MainUiParamPage,L"游戏设置／内容页（填页面代码）"}},
 {925,{UiId::MainUiParamNative,L"游戏菜单入口（填入口代码）"}},
 {201,{UiId::MainUiAddons,L"模组列表"}},
 {202,{UiId::MainUiAchievements,L"成就"}},
 {203,{UiId::MainUiLeaderboards,L"生还者榜单"}},
 {204,{UiId::MainUiExtras,L"更多内容"}},
 {922,{UiId::MainUiParamDemo,L"播放指定录像"}},
 {207,{UiId::MainUiRebuildAudio,L"重建音频库"}},
 {4,{UiId::MainUiNativeShort,L"原版主菜单"}},
 {5,{UiId::MainUiQuit,L"退出游戏"}},
 {2,{UiId::MainUiAdvanced,L"Skeeto 高级设置"}},
 {3,{UiId::MainUiBackground,L"主菜单背景"}},
 {27,{UiId::MainUiBackgroundView,L"欣赏背景（Esc 返回）"}},
 {928,{UiId::MainUiPlaybackPause,L"暂停"}},
 {929,{UiId::MainUiPlaybackProgress,L"MP4 播放进度"}},
 {11,{UiId::MainUiVolumes,L"分类音量"}},
 {12,{UiId::MainUiCareer,L"生涯档案"}},
 {210,{UiId::MainUiBattleHistory,L"近期战报"}},
 {212,{UiId::MainUiChangelog,L"Skeeto 更新日志"}},
 {213,{UiId::MainUiCommunity,L"Skeeto 工坊"}},
 {214,{UiId::MainUiAssetFolder,L"打开素材目录"}},
 {216,{UiId::MainUiHomeServices,L"主菜单服务"}},
 {8,{UiId::MainUiPersonalize,L"主菜单设置"}},
 {208,{UiId::MainUiEditHomepage,L"编辑主菜单"}},
 {211,{UiId::MainUiManageProfilesAction,L"主菜单方案管理（打开页面）"}},
 {218,{UiId::MainUiExportUi,L"导出主菜单（选择保存位置）"}},
 {219,{UiId::MainUiImportUi,L"导入主菜单（选择文件）"}},
 {907,{UiId::MainUiLoadHudAction,L"切换方案（填文件名）"}},
 {919,{UiId::MainUiParamSet,L"ConVar 一键设值"}},
 {920,{UiId::MainUiParamToggle,L"ConVar 双值开关"}},
 {921,{UiId::MainUiParamCycle,L"ConVar 多值循环"}},
 {910,{UiId::MainUiCvarSlider,L"ConVar 滑块"}},
 {902,{UiId::MainUiConsoleAction,L"控制台指令"}},
 {903,{UiId::MainUiCustomMenu,L"自定义子菜单"}},
 {904,{UiId::MainUiWebAction,L"打开网站"}},
 {905,{UiId::MainUiPathAction,L"打开文件 / 文件夹"}},
 {906,{UiId::MainUiSteamAction,L"打开 Steam 入口"}},
 {930,{UiId::MainUiElementWelcome,L"欢迎标题（文字元素）"}},
 {931,{UiId::MainUiElementSubtitle,L"欢迎副标题（文字元素）"}},
 {932,{UiId::MainUiElementLogo,L"Skeeto 标识（文字元素）"}},
 {933,{UiId::MainUiElementVersion,L"版本号（文字元素）"}},
 {934,{UiId::MainUiElementGame,L"游戏名称（文字元素）"}},
 {935,{UiId::MainUiElementHint,L"操作提示（文字元素）"}},
 {936,{UiId::MainUiElementNotice,L"状态提示（文字元素）"}},
 {937,{UiId::MainUiElementFriends,L"正在玩 L4D2 的好友"}},
};
static constexpr bool MainUiActionCatalogUnique() {
	for (int i = 0; i < (int)(sizeof(kMainUiActions) / sizeof(kMainUiActions[0])); ++i)
		for (int j = i + 1; j < (int)(sizeof(kMainUiActions) / sizeof(kMainUiActions[0])); ++j)
			if (kMainUiActions[i].action == kMainUiActions[j].action) return false;
	return true;
}
static_assert(MainUiActionCatalogUnique(), "duplicate serialized main-menu action id");
static bool MainUiActionKnown(int action) {
	for (const auto& item : kMainUiActions) if (item.action == action) return true;
	return false;
}
static int MainUiActionGroup(int action){
 if(action>=911&&action<=916||action==918||action==924)return 0;
 if(action==917||action==922||action==925)return 2;
 if(action>=100&&action<=108||action==0)return 0;
 if(action==1||action==205||action==217||action==209||action==900||action==901||action==206||action==220)return 1;
 if(action==200||action==201||action==202||action==203||action==204||action==207||action==4||action==5)return 2;
 if(action==928||action==929||action==27||action==2||action==3||action==11||action==12||action==210||action==212||action==213||action==214||action==215||action==216)return 3;
 if(action>=930&&action<=937)return 6;
 if(action==8||action==208||action==211||action==218||action==219||action==907)return 4;
 return 5;
}
static const UiTextEntry kMainUiActionGroups[]={
 {UiId::MainUiGroupModes,L"游戏模式与开局"},{UiId::MainUiGroupServers,L"服务器与地图"},{UiId::MainUiGroupGame,L"游戏界面与工具"},
 {UiId::MainUiGroupFeatures,L"Skeeto 功能"},{UiId::MainUiGroupLayout,L"主菜单编辑与方案"},{UiId::MainUiGroupCustom,L"自定义动作 / 子菜单 / 滑块"},{UiId::MainUiGroupElements,L"文字与好友组件"}};
static int MainUiActionGroupIndices(int group,int* indices){int count=0;for(int i=0;i<(int)(sizeof(kMainUiActions)/sizeof(kMainUiActions[0]));++i)if(MainUiActionGroup(kMainUiActions[i].action)==group)indices[count++]=i;return count;}
static const wchar_t* MainUiActionTitle(int action) {
	for(const auto& a:kMainUiActions)if(a.action==action)return UiText(a.title.id,a.title.chinese);
	return L"";
}
// Empty built-in titles remain language resources, not serialized Chinese.
// An explicit user title always wins, including on a default slot.
static const wchar_t* MainUiSlotTitle(const MainUiLayoutSlot& b){
 if(b.id==0&&b.action==916&&!b.text[0])return UiText(UiId::MainUiCreateLobby,L"快速创建大厅");
 if(b.action>=930&&b.action<=937&&b.paramText[0])return b.paramText;
 if(b.text[0])return b.text;
 if(b.action==928){const auto state=SkeetoBackgroundGetPlayback();return !state.available?UiText(UiId::MainUiPlaybackUnavailable,L"MP4 不可用"):state.paused?UiText(UiId::MainUiPlaybackResume,L"继续"):UiText(UiId::MainUiPlaybackPause,L"暂停");}
 if(b.action>=930&&b.action<=937)return MainUiElementText(b.action);
 if(b.action==218)return UiText(UiId::MainUiExportButton,L"导出主菜单");
 if(b.action==219)return UiText(UiId::MainUiImportButton,L"导入主菜单");
 if(b.action==211)return UiText(UiId::MainUiHudProfiles,L"主菜单方案");
 if(b.id==644&&b.action==904)return UiText(UiId::SkeetoDeveloper,L"Skeeto开发者");
if(b.id==645&&b.action==904)return UiText(UiId::SkeetoOpenSource,L"skeeto开源仓库");
 if(b.id==620&&b.action==903)return UiText(UiId::MainUiRelated,L"Skeeto 相关");
 if(b.id==636&&b.action==905)return UiText(UiId::MainUiAutoexec,L"打开 autoexec.cfg");
 if(b.id==637&&b.action==906)return UiText(UiId::MainUiWorkshopL4d2,L"创意工坊");
 if(b.id==217&&b.action==903)return UiText(UiId::MainUiFavoritesTab,L"收藏服务器");
 if(b.id==621&&b.action==903)return UiText(UiId::MainUiWebPresets,L"常用网页");
 if(b.id==622&&b.action==901)return UiText(UiId::MainUiMapSample,L"地图入口：死亡中心");
 if(b.id==632&&b.action==906)return UiText(UiId::MainUiAssetsSample,L"预设素材包");
 if(b.id==634&&b.action==905)return UiText(UiId::MainUiProfileFolderSample,L"主菜单方案目录");
 if(b.id==635&&b.action==905)return UiText(UiId::MainUiFolderSample,L"打开 cfg 文件夹");
 if(b.action==904){switch(b.id){case 640:return UiText(UiId::MainUiWebSoda,L"汽水查服网页");case 641:return UiText(UiId::MainUiWebMaps,L"三方图下载");case 642:return UiText(UiId::MainUiWebSherlock,L"夏洛克查服网页");case 643:return UiText(UiId::MainUiWebCat,L"猫服查服网页");}}
 return MainUiActionTitle(b.action);
}
static COLORREF MainUiSlotColor(int preset) {
	static const COLORREF colors[]={RGB(255,199,86),RGB(105,222,235),RGB(137,224,175),RGB(223,164,245),RGB(255,153,125),RGB(238,240,245),RGB(150,225,210),RGB(145,195,255),RGB(255,145,150),RGB(255,184,125),RGB(190,201,215),RGB(215,229,145)};
	return preset>=0x1000000&&preset<=0x1ffffff?(COLORREF)(preset&0xffffff):preset>=0&&preset<12?colors[preset]:MainUiAccentColor();
}
static MainUiLayoutSlot* MainUiLayoutFind(int id) {
	for(int i=0;i<g_mainUiLayoutCount;++i)if(g_mainUiLayout[i].id==id)return &g_mainUiLayout[i];
	return nullptr;
}
static void MainUiHubSelect(int);
static void MainUiGameExecute(int action,const char* parameter){
 MainUiGameSpec v{};if(!MainUiGameSpecValid(action,parameter)||!MainUiGameTokens(parameter,&v)){g_mainUiNotice=2;return;}
 if(action>=911&&action<=915||action==924){const char* prefixes[]={"QuickMatch_","CustomMatch_","FriendsMatch_","GroupServer_","SoloPlay_"};char command[128]{};snprintf(command,sizeof(command),"%s%s",action==924?"FriendsMatch_":prefixes[action-911],v.token[0]);
  if(action==915)MainUiClearExpertPending();
  else if(action==911||action==912||action==913||action==924)MainUiPushExpertDifficulty();
  MainUiOpenNative(command);return;}
 if(action==916){if(v.count==1)MainUiOpenLobbyPreset("coop - - Impossible friends listen");else MainUiOpenLobbyPreset(parameter);return;}
 if(action==917||action==925){MainUiOpenNative(v.token[0]);return;}
 if(action>=919&&action<=921){void* var=CvarFind(v.token[0]);float current=0;if(!var||(CvarFlagsOf(var)&(kFCvarCheat|kFCvarDevOnly|kFCvarHidden))||!CvarReadPair(var,nullptr,&current)||!_finite(current)){g_mainUiNotice=2;return;}
  int next=1;if(action==920){if(current==strtof(v.token[1],nullptr))next=2;}else if(action==921){for(int i=1;i<v.count;++i)if(current==strtof(v.token[i],nullptr)){next=i+1<v.count?i+1:1;break;}}
  char command[192]{};snprintf(command,sizeof(command),"%s %s",v.token[0],v.token[next]);EngClientCmd(command);return;
 }
 char command[384]{};if(action==918){if(v.count==3)snprintf(command,sizeof(command),"z_difficulty %s; map %s %s",v.token[2],v.token[0],v.token[1]);else snprintf(command,sizeof(command),"z_difficulty Impossible; map %s %s",v.token[0],v.token[1]);}
 else if(action==922)snprintf(command,sizeof(command),"playdemo %s",v.token[0]);else return;
 MainUiRestore();g_mainUiFlyout=-1;g_mainUiPressed=-1;EngClientCmdUnrestricted(command);
}
static bool MainUiLayoutDispatch(int id) {
	if(g_mainUiPanel!=0)return false;
	auto* b=MainUiLayoutFind(id);if(!b)return false;if(!b->visible)return true;
	if(b->action==903){g_mainUiFlyout=g_mainUiFlyout==12&&g_mainUiCustomMenu==id?-1:12;
		g_mainUiCustomMenu=id;g_mainUiCustomPage=0;g_mainUiFlyAnchor=id;g_mainUiFocus=g_mainUiFlyout<0?id:300;g_mainUiPressed=-1;return true;}
	if(b->action==910)return true;
 if(b->action>=911&&b->action<=925){if(!MainUiLayoutParamValid(b->action,b->parameter)){g_mainUiNotice=2;return true;}MainUiGameExecute(b->action,b->parameter);return true;}
	if(b->action>=904&&b->action<=909){
  if(!MainUiLayoutParamValid(b->action,b->parameter)){g_mainUiNotice=2;return true;}
  MainUiHubParameterized(b->action,b->parameter);return true;
 }
 if(b->action==900||b->action==901||b->action==902) {
		if(!MainUiLayoutParamValid(b->action,b->parameter)){g_mainUiNotice=2;return true;}
		char command[512]{},parameter[384]{};if(b->action==902)strcpy_s(command,b->parameter);else {MainUiServerConnectParameter(b->parameter,parameter,sizeof(parameter));snprintf(command,sizeof(command),"%s %s",b->action==900?"connect":"map",parameter);}
		g_mainUiNotice=0;g_mainUiFlyout=-1;if(b->action!=902)MainUiRestore();
		g_mainUiPressed=-1;g_mainUiInputSeed=true;g_mainUiPopupAt=0;EngClientCmdUnrestricted(command);return true;
	}
	{
		// Dispatch one resolved fixed action; never recurse through another slot.
		g_mainUiResolving=true;MainUiDispatch(b->action);g_mainUiResolving=false;
		if(g_mainUiFlyout>=0)g_mainUiFlyAnchor=id;return true;
	}
}
struct MainUiSliderCache {unsigned revision=0,version=0;DWORD readAt=0,lastSendAt=0;MainUiCvarSpec spec{};float value=0,lastSent=0;bool valid=false,available=false,sent=false,initialized=false;wchar_t number[48]{};};
static MainUiSliderCache g_mainUiCvarCache[kMainUiLayoutMax]{};
static MainUiLayoutSlot* MainUiCustomSliderSlot(int action){
 if(g_mainUiEditing||g_mainUiPanel!=0)return nullptr;
 if(g_mainUiFlyout==12 && action>=300 && action<309){int count=0,perPage=MainUiCustomPageSize(),wanted=g_mainUiCustomPage*perPage+action-300;
  for(int i=0;i<g_mainUiLayoutCount;++i){auto& b=g_mainUiLayout[i];if(!b.visible||b.parent!=g_mainUiCustomMenu)continue;if(count++==wanted)return b.action==910?&b:nullptr;}return nullptr;}
 auto* b=MainUiLayoutFind(action);return b&&b->visible&&b->parent<0&&b->action==910?b:nullptr;
}
static MainUiSliderCache* MainUiCvarCache(const MainUiLayoutSlot& slot){
 const uintptr_t offset=(uintptr_t)&slot-(uintptr_t)g_mainUiLayout;if(offset>=sizeof(g_mainUiLayout)||offset%sizeof(MainUiLayoutSlot))return nullptr;const int index=(int)(offset/sizeof(MainUiLayoutSlot));if(index>=g_mainUiLayoutCount)return nullptr;
 auto& cache=g_mainUiCvarCache[index];if(cache.revision!=g_mainUiTextRevision||!cache.initialized){MainUiCvarSpec spec{};bool valid=MainUiCvarParse(slot.parameter,&spec);
  if(!cache.initialized||valid!=cache.valid||strcmp(cache.spec.name,spec.name)||cache.spec.minimum!=spec.minimum||cache.spec.maximum!=spec.maximum||cache.spec.step!=spec.step){unsigned version=cache.version+1;cache=MainUiSliderCache{};cache.version=version;cache.spec=spec;cache.valid=valid;wcscpy_s(cache.number,L"--");}
  cache.revision=g_mainUiTextRevision;cache.initialized=true;
 }
 if(!cache.valid)return &cache;DWORD now=GetTickCount();if(!cache.readAt||(DWORD)(now-cache.readAt)>=250u){
  cache.readAt=now;void* var=CvarFind(cache.spec.name);float current=0;bool available=var && !(CvarFlagsOf(var)&(kFCvarCheat|kFCvarDevOnly|kFCvarHidden)) && CvarReadPair(var,nullptr,&current) && _finite(current);
  if(available!=cache.available||available&&current!=cache.value||!cache.version){cache.sent=false;cache.available=available;cache.value=current;++cache.version;if(available)swprintf_s(cache.number,L"%.4g",double(current));else wcscpy_s(cache.number,L"--");}
 }return &cache;
}
static bool MainUiCustomSlider(int action){return MainUiCustomSliderSlot(action)!=nullptr;}
static bool MainUiPlaybackSlider(int action){
 if(g_mainUiEditing)return false;
 if(g_mainUiPanel==1)return action==929;
 if(g_mainUiPanel!=0)return false;
 if(g_mainUiFlyout==12&&action>=300&&action<309){int count=0,wanted=g_mainUiCustomPage*MainUiCustomPageSize()+action-300;for(int i=0;i<g_mainUiLayoutCount;++i){const auto& b=g_mainUiLayout[i];if(!b.visible||b.parent!=g_mainUiCustomMenu)continue;if(count++==wanted)return b.action==929;}return false;}
 auto* b=MainUiLayoutFind(action);return b&&b->visible&&b->parent<0&&b->action==929;
}
static void MainUiPlaybackDrag(int action,int x,bool final){
 const auto state=SkeetoBackgroundGetPlayback();if(!state.seekable){g_mainUiPlaybackDragId=-1;return;}
 for(int i=0;i<g_mainUiButtonCount;++i){const auto& b=g_mainUiButtons[i];if(b.action!=action)continue;int pad=(b.x1-b.x0)/24;if(pad<4)pad=4;int span=b.x1-b.x0-2*pad;if(span<=0)return;
  double fraction=double(x-b.x0-pad)/span;if(fraction<0)fraction=0;if(fraction>1)fraction=1;g_mainUiPlaybackFraction=fraction;g_mainUiPlaybackDragId=action;
  if(final){SkeetoBackgroundSeek((LONGLONG)(fraction*state.duration));g_mainUiPlaybackDragId=-1;}return;
 }
}
static void MainUiPlaybackStep(int action,int direction){if(!MainUiPlaybackSlider(action))return;const auto state=SkeetoBackgroundGetPlayback();if(state.seekable)SkeetoBackgroundSeek(state.position+direction*50000000LL);}
static void MainUiCustomSliderSet(int action,float fraction,int direction=0,bool throttled=false,bool final=false){auto* b=MainUiCustomSliderSlot(action);if(!b)return;auto* c=MainUiCvarCache(*b);if(!c||!c->available)return;
 const auto& spec=c->spec;double raw=direction?double(c->value)+direction*spec.step:spec.minimum+double(fraction)*(spec.maximum-spec.minimum);
 double steps=floor((raw-spec.minimum)/spec.step+.5);float value=(float)(spec.minimum+steps*spec.step);if(value<spec.minimum)value=spec.minimum;if(value>spec.maximum)value=spec.maximum;
 if(c->sent && c->lastSent==value)return;if(value==c->value)return;DWORD now=GetTickCount();if(throttled&&!final&&c->lastSendAt&&(DWORD)(now-c->lastSendAt)<50u)return;char command[128]{};snprintf(command,sizeof(command),"%s %.9g",spec.name,double(value));EngClientCmd(command);c->sent=true;c->lastSent=value;c->lastSendAt=now;c->value=value;c->readAt=now;++c->version;swprintf_s(c->number,L"%.4g",double(value));
}
static void MainUiCustomSliderDrag(int action,int x,bool final=false){for(int i=0;i<g_mainUiButtonCount;++i){const auto& b=g_mainUiButtons[i];if(b.action!=action)continue;int padding=(b.x1-b.x0)/24;if(padding<4)padding=4;int span=b.x1-b.x0-2*padding;if(span<=0)return;float f=float(x-b.x0-padding)/span;if(f<0)f=0;if(f>1)f=1;MainUiCustomSliderSet(action,f,0,true,final);return;}}
static void MainUiDrawSlot(const MainUiLayoutSlot& b,int x,int y,int w,int h,bool registration,int hoverId,int inputId=-1,int fontOverride=-1,const wchar_t* titleOverride=nullptr) {
	const int id=inputId<0?b.id:inputId;
	if((b.action==910||b.action==929)&&!titleOverride&&h<g_mainUiFontPx[2]+16)h=g_mainUiFontPx[2]+16;
	if(registration){if(g_mainUiButtonCount<kMainUiHitMax)g_mainUiButtons[g_mainUiButtonCount++]={x,y,x+w,y+h,id};return;}
	const bool addonButton=b.action==201||(b.action==917&&!strcmp(b.parameter,"Addons"));
	const int addonState=addonButton&&!g_mainUiEditing?MainUiAddonsRead():0;
	const bool addonBadge=addonButton&&!g_mainUiEditing&&addonState!=0&&w>=32&&h>=16;
	const bool enabled=b.action==928||b.action==929?SkeetoBackgroundGetPlayback().available:(!(b.action>=900&&b.action<=925)||b.action==903)||MainUiLayoutParamValid(b.action,b.parameter);
	bool hot=!g_mainUiUnderlay.load(std::memory_order_relaxed)&&(g_mainUiMouseMode?hoverId==id:g_mainUiFocus==id);
	COLORREF accent=MainUiSlotColor(b.color),color=!enabled?RGB(145,155,167):hot||b.color>=0?accent:RGB(244,245,246);
	int alpha=MulDiv(hot?244:216,b.opacity<0?g_mainUiBackdrop:b.opacity,100);
	if(alpha>0){COLORREF backdrop=MainUiBackdropColor(color);SurfColor(GetRValue(backdrop),GetGValue(backdrop),GetBValue(backdrop),alpha);SurfFill(x,y,x+w,y+h);}
	if(hot){SurfColor(GetRValue(accent),GetGValue(accent),GetBValue(accent),255);SurfFill(x,y,x+2,y+h);}
	int size=fontOverride<0?b.font:fontOverride;int f=size==0?2:size==2?3:1;
	int addonTitleWidth=0,addonRadius=h/5,addonGap=g_mainUiFontPx[f]/5;
	if(addonRadius<4)addonRadius=4;if(addonRadius>10)addonRadius=10;if(addonGap<4)addonGap=4;
	const wchar_t* addonCaption=addonBadge?UiText(UiId::MainUiAddonsLoading,L"Loading..."):nullptr;
	int addonReserve=0;
	if(addonBadge){
		// Font resizing reuses HFont: include the text revision to invalidate measured width.
		static unsigned long captionFont=0;static unsigned captionRevision=0;static int captionEpoch=0,captionWidth=0;static const wchar_t* caption=nullptr;
		if(captionFont!=g_mainUiFonts[2]||captionEpoch!=g_sktFontEpoch||captionRevision!=g_mainUiTextRevision||caption!=addonCaption){captionFont=g_mainUiFonts[2];captionEpoch=g_sktFontEpoch;captionRevision=g_mainUiTextRevision;caption=addonCaption;captionWidth=SurfTextWFont(captionFont,caption);}
		addonReserve=addonState<0?addonGap+16:addonGap*2+addonRadius*2+3+captionWidth;
		if(addonReserve>(w-16)/2)addonReserve=(w-16)/2;
	}
	bool arrow=(b.action>=100&&b.action<=108)||b.action==200||b.action==204||b.action==206||b.action==903;
    if(b.action==929&&!titleOverride){
        const auto state=SkeetoBackgroundGetPlayback();int pad=w/24;if(pad<4)pad=4;int span=w-2*pad;
        LONGLONG position=state.position;if(g_mainUiPlaybackDragId==id&&g_mainUiPressed==id)position=(LONGLONG)(g_mainUiPlaybackFraction*state.duration);
        struct TimeText{LONGLONG current=-2,total=-2;unsigned version=0;wchar_t text[64]{};};static TimeText times[2]{};auto& time=times[g_mainUiPlaybackDragId==id&&g_mainUiPressed==id?1:0];
        const LONGLONG current=state.available?position/10000000:-1,total=state.available?state.duration/10000000:-1;
        if(time.current!=current||time.total!=total){time.current=current;time.total=total;++time.version;if(state.available)swprintf_s(time.text,L"%02lld:%02lld / %02lld:%02lld",current/60,current%60,total/60,total%60);else wcscpy_s(time.text,L"--:-- / --:--");}
        MainUiText(g_mainUiFonts[2],x+pad,y+2,color,time.text,span,time.version);
        float fraction=state.duration>0?(float)((double)position/state.duration):0;if(fraction<0)fraction=0;if(fraction>1)fraction=1;
        int rail=y+h-8,cursor=x+pad+(int)(span*fraction);SurfColor(75,85,98,255);SurfFill(x+pad,rail,x+pad+span,rail+3);
        SurfColor(GetRValue(accent),GetGValue(accent),GetBValue(accent),enabled?255:90);SurfFill(x+pad,rail,cursor,rail+3);
        if(state.seekable){SurfColor(244,245,246,255);SurfFill(cursor-2,rail-3,cursor+2,rail+6);}
    }else if(b.action==910&&!titleOverride){auto* c=MainUiCvarCache(b);int pad=w/24;if(pad<4)pad=4;
        int numberWidth=w/3;if(numberWidth<28)numberWidth=28;
        MainUiText(g_mainUiFonts[2],x+pad,y+2,color,MainUiSlotTitle(b),w-2*pad-numberWidth-8);
        if(c)MainUiText(g_mainUiFonts[2],x+w-pad-numberWidth,y+2,c->available?color:RGB(145,155,167),c->number,numberWidth,c->version);
        int railY=y+h-7,span=w-2*pad;float fraction=c&&c->available?(c->value-c->spec.minimum)/(c->spec.maximum-c->spec.minimum):0;if(fraction<0)fraction=0;if(fraction>1)fraction=1;
        SurfColor(75,85,98,255);SurfFill(x+pad,railY,x+pad+span,railY+2);SurfColor(GetRValue(accent),GetGValue(accent),GetBValue(accent),255);SurfFill(x+pad,railY,x+pad+(int)(span*fraction),railY+2);SurfColor(244,245,246,255);int cursor=x+pad+(int)(span*fraction);SurfFill(cursor-2,railY-3,cursor+2,railY+5);
    }else if(b.action==900&&!titleOverride){const wchar_t* name=nullptr;const wchar_t* details=nullptr;unsigned version=0;MainUiServerPresentation(b,&name,&details,&version);
  if(h>=g_mainUiFontPx[2]*2+4){int nf=f;if(g_mainUiFontPx[nf]+g_mainUiFontPx[2]+4>h)nf=2;int top=y+(h-g_mainUiFontPx[nf]-g_mainUiFontPx[2]-2)/2;
   MainUiText(g_mainUiFonts[nf],x+8,top,color,name,w-16,version);MainUiText(g_mainUiFonts[2],x+8,top+g_mainUiFontPx[nf]+2,RGB(180,188,197),details,w-16,version);
  }else MainUiText(g_mainUiFonts[f],x+8,y+(h-g_mainUiFontPx[f])/2,color,name,w-16,version);
 }else MainUiText(g_mainUiFonts[f],x+8,y+(h-g_mainUiFontPx[f])/2,color,titleOverride?titleOverride:MainUiSlotTitle(b),w-(arrow?28:16)-addonReserve,0,&addonTitleWidth);
	if(arrow){SurfColor(GetRValue(accent),GetGValue(accent),GetBValue(accent),160);SurfLine(x+w-14,y+h/2-4,x+w-10,y+h/2);SurfLine(x+w-10,y+h/2,x+w-14,y+h/2+4);}
	if(addonBadge){
		int afterTitle=x+8+addonTitleWidth+addonGap,right=x+w-8;
		if(addonState<0)MainUiText(g_mainUiFonts[2],afterTitle,y+(h-g_mainUiFontPx[2])/2,RGB(180,188,197),L"?",right-afterTitle);
		else {
			// Anchor to the fitted label's actual end, including renamed labels.
			// The measured width comes from MainUiText's existing cache. Keep the
			// fixed eight dots and caption within the actual button's right edge.
			static const int dx[8]={10,7,0,-7,-10,-7,0,7},dy[8]={0,7,10,7,0,-7,-10,-7};
			int radius=addonRadius,dot=radius>=6?3:2,cx=afterTitle+radius+dot/2,cy=y+h/2;unsigned phase=(GetTickCount()/90u)%8u;
			if(cx+radius+dot<=right){
				for(unsigned i=0;i<8;++i){int alpha=96+int((i+8-phase)%8)*22;SurfColor(255,220,96,alpha);int px=cx+dx[i]*radius/10-dot/2,py=cy+dy[i]*radius/10-dot/2;SurfFill(px,py,px+dot,py+dot);}
				int captionX=cx+radius+dot+addonGap;
				MainUiText(g_mainUiFonts[2],captionX,y+(h-g_mainUiFontPx[2])/2,RGB(255,220,96),addonCaption,right-captionX);
			}
		}
	}
}
static void MainUiDrawLayout(bool registration) {
    // Any open flyout owns the screen. Home buttons stay out of both pixels and hits.
    if(g_mainUiFlyout>=0)return;
	float s=(g_mainUiSw-48.f)/1040.f;float v=(g_mainUiSh-48.f)/600.f;if(v<s)s=v;
	auto U=[=](int n){return(int)(n*s+.5f);};int ox=(g_mainUiSw-U(1040))/2,oy=(g_mainUiSh-U(600))/2;
	int mx=0,my=0,hover=-1;
	if(!registration&&g_mainUiMouseMode&&!g_mainUiUnderlay.load(std::memory_order_relaxed)&&MainUiMouse(&mx,&my))hover=MainUiHit(mx,my);
	for(int i=0;i<g_mainUiLayoutCount;++i) {
		const auto& b=g_mainUiLayout[i];if(!b.visible||b.parent>=0)continue;
        if(b.action>=930&&b.action<=937){if(MainUiElementVisible(b.action)){if(b.action==937)MainUiDrawFriends(b,ox+U(b.x),oy+U(b.y),U(b.w),U(b.h),registration);else if(!registration)MainUiDrawElement(b,ox+U(b.x),oy+U(b.y),U(b.w),U(b.h));}continue;}
		MainUiDrawSlot(b,ox+U(b.x),oy+U(b.y),U(b.w),U(b.h),registration,hover);
	}
}
static int MainUiCustomChildren(int* indices) {
 int count=0;for(int i=0;i<g_mainUiLayoutCount;++i)if(g_mainUiLayout[i].visible&&g_mainUiLayout[i].parent==g_mainUiCustomMenu)indices[count++]=i;return count;
}
static int MainUiCustomPageSize(){for(int i=0;i<g_mainUiLayoutCount;++i)if(g_mainUiLayout[i].visible&&g_mainUiLayout[i].parent==g_mainUiCustomMenu&&(g_mainUiLayout[i].action==900||g_mainUiLayout[i].action==910))return 5;return g_mainUiCustomMenu==217?5:9;}
static void MainUiEditorAddServer(int parent);
static void MainUiCustomBack() {
 auto* menu=MainUiLayoutFind(g_mainUiCustomMenu);int parent=menu?menu->parent:-1;
 if(parent>=0){g_mainUiCustomMenu=parent;g_mainUiCustomPage=0;g_mainUiFocus=300;}
 else {g_mainUiFlyout=-1;g_mainUiFocus=g_mainUiCustomMenu;}g_mainUiPressed=-1;
}
static bool MainUiCustomDispatch(int action) {
 if(action==318){MainUiCustomBack();return true;}
 if(action==320){g_optMainUiServerQuery=!g_optMainUiServerQuery;MainUiSettingsChanged();++g_mainUiTextRevision;return true;}
 if(action==321){MainUiEditorAddServer(g_mainUiCustomMenu);return true;}
 int indices[kMainUiLayoutMax]{};int count=MainUiCustomChildren(indices),perPage=MainUiCustomPageSize();
 if(action==319){g_mainUiCustomPage=(g_mainUiCustomPage+1)%((count+perPage-1)/perPage? (count+perPage-1)/perPage:1);g_mainUiFocus=300;return true;}
 if(action<300||action>=309)return false;
 int at=g_mainUiCustomPage*perPage+action-300;if(at>=count)return true;
 int id=g_mainUiLayout[indices[at]].id;if(g_mainUiLayout[indices[at]].action==910)return true;g_mainUiFlyout=-1;
 MainUiLayoutDispatch(id);return true;
}
static void MainUiDrawCustomFlyout(bool registration) {
 auto* menu=MainUiLayoutFind(g_mainUiCustomMenu);if(!menu||menu->action!=903)return;
 int indices[kMainUiLayoutMax]{};int count=MainUiCustomChildren(indices),perPage=MainUiCustomPageSize(),pages=(count+perPage-1)/perPage;
 if(g_mainUiCustomPage<0||g_mainUiCustomPage>=(pages?pages:1))g_mainUiCustomPage=0;
 float s=(g_mainUiSw-48.f)/1040.f;float v=(g_mainUiSh-48.f)/600.f;if(v<s)s=v;
 auto U=[=](int n){return(int)(n*s+.5f);};int ox=(g_mainUiSw-U(1040))/2,oy=(g_mainUiSh-U(600))/2;
 int x=ox+U(312),y=oy+U(122),hover=-1,mx=0,my=0;COLORREF accent=MainUiAccentColor();
 if(!registration){if(!g_mainUiUnderlay&&MainUiMouse(&mx,&my))hover=MainUiHit(mx,my);
  COLORREF backdrop=MainUiBackdropColor(MainUiSlotColor(menu->color));SurfColor(GetRValue(backdrop),GetGValue(backdrop),GetBValue(backdrop),MulDiv(245,g_mainUiBackdrop,100));SurfFill(x,y,x+U(728),oy+U(558));SurfColor(GetRValue(accent),GetGValue(accent),GetBValue(accent),255);SurfFill(x,y,x+U(3),oy+U(558));
  MainUiText(g_mainUiFonts[1],x+U(18),y+U(14),accent,MainUiSlotTitle(*menu),U(692));
  if(!count)MainUiText(g_mainUiFonts[2],x+U(18),y+U(70),RGB(180,188,197),UiText(UiId::MainUiCustomEmpty,L"暂无子项；进入编辑模式后新增按钮。"),U(692));
 }
 for(int i=0;i<perPage&&g_mainUiCustomPage*perPage+i<count;++i){const auto& b=g_mainUiLayout[indices[g_mainUiCustomPage*perPage+i]];
  MainUiDrawSlot(b,x+U(18),y+U(53+i*(perPage==5?60:36)),U(b.action==910?(b.w<320?b.w:320):692),U(b.action==910?36:perPage==5?56:32),registration,hover,300+i);}
 bool servers=g_mainUiCustomMenu==217;for(int i=0;i<count;++i)if(g_mainUiLayout[indices[i]].action==900)servers=true;
 if(servers){MainUiLayoutSlot toggle=MainUiLayoutBlank();toggle.font=0;toggle.id=320;
  MainUiDrawSlot(toggle,x+U(18),oy+U(480),U(330),U(28),registration,hover,-1,-1,UiText(g_optMainUiServerQuery?UiId::MainUiQueryOn:UiId::MainUiQueryOff,g_optMainUiServerQuery?L"状态查询：开":L"状态查询：关"));
  toggle.id=321;MainUiDrawSlot(toggle,x+U(380),oy+U(480),U(330),U(28),registration,hover,-1,-1,UiText(UiId::MainUiAddServer,L"添加服务器"));
 }
 MainUiLayoutSlot back=MainUiLayoutBlank();back.id=318;back.font=0;
 MainUiDrawSlot(back,x+U(18),oy+U(514),U(316),U(32),registration,hover,-1,-1,UiText(UiId::MainUiCustomBack,L"返回上级"));
 if(pages>1){back.id=319;MainUiDrawSlot(back,x+U(380),oy+U(514),U(330),U(32),registration,hover,-1,-1,UiText(UiId::MainUiCustomNext,L"下一页"));}
}
static bool MainUiEditorParentAllowed(int parent,int id) {
 for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].id==id&&g_mainUiDraft[i].action>=930&&g_mainUiDraft[i].action<=937&&parent>=0)return false;
 for(int step=0;parent>=0&&step<kMainUiLayoutMax;++step){if(parent==id)return false;int at=-1;
  for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].id==parent)at=i;
  if(at<0||g_mainUiDraft[at].action!=903)return false;parent=g_mainUiDraft[at].parent;
 }return parent<0;
}
static void MainUiEditorReparentChildren(int id,int parent) {
 for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].parent==id)g_mainUiDraft[i].parent=parent;
}
static void MainUiEditorMarkTree(int selected,bool* remove){
 remove[selected]=true;
 int parents[kMainUiLayoutMax]{};
 for(int i=0;i<g_mainUiDraftCount;++i){parents[i]=-1;for(int j=0;j<g_mainUiDraftCount;++j)if(g_mainUiDraft[j].id==g_mainUiDraft[i].parent){parents[i]=j;break;}}
 // Compute ancestry against the original array before compacting it.
 for(int i=0;i<g_mainUiDraftCount;++i){int at=parents[i];
  for(int step=0;at>=0&&step<g_mainUiDraftCount;++step){if(at==selected){remove[i]=true;break;}at=parents[at];}}
}
static void MainUiEditorDeleteTree(){
 bool remove[kMainUiLayoutMax]{};MainUiEditorMarkTree(g_mainUiEditSelected,remove);
 int kept=0;for(int i=0;i<g_mainUiDraftCount;++i)if(!remove[i])g_mainUiDraft[kept++]=g_mainUiDraft[i];
 memset(g_mainUiDraft+kept,0,sizeof(MainUiLayoutSlot)*(g_mainUiDraftCount-kept));g_mainUiDraftCount=kept;
 if(g_mainUiEditSelected>=kept)g_mainUiEditSelected=kept?kept-1:0;g_mainUiEditDrag=-1;
}

static bool g_mainUiDraftDefaultBackdrop=false;
static void MainUiEditorAbort() {
 g_mainUiDraftDefaultBackdrop=false;
 MainUiServicesCancelText(g_mainUiTextRequest);g_mainUiTextRequest=0;
	g_mainUiEditing=false;g_mainUiCvarEditing=false;g_mainUiEditTextField=0;g_mainUiEditActionPage=-1;g_mainUiEditDrag=-1;g_mainUiEditInputSeed=true;
	g_mainUiInputSeed=true;g_mainUiPressed=-1;g_mainUiFocus=-1;g_mainUiPanel=0;g_mainUiFlyout=-1;++g_mainUiTextRevision;
}
static void MainUiEditorBegin() {
	if(!g_mainUiInputReady.load(std::memory_order_relaxed))return;
 g_mainUiDraftDefaultBackdrop=false;
	memcpy(g_mainUiDraft,g_mainUiLayout,sizeof(g_mainUiDraft));g_mainUiDraftCount=g_mainUiLayoutCount;g_mainUiDraftDecor=g_mainUiHomeDecor;
	g_mainUiEditing=true;g_mainUiEditSelected=0;g_mainUiEditStatus=0;g_mainUiEditDrag=-1;g_mainUiEditTextField=0;g_mainUiEditActionPage=-1;g_mainUiEditActionGroup=-1;g_mainUiEditShowHidden=false;g_mainUiCvarEditing=false;
	g_mainUiPanel=0;g_mainUiFlyout=-1;g_mainUiFocus=kMainUiEditorHitBase;g_mainUiEditInputSeed=true;++g_mainUiTextRevision;
}
static int MainUiEditorUnusedId() {
	for(int id=600;id<600+kMainUiLayoutMax;++id){bool used=false;for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].id==id)used=true;if(!used)return id;}return -1;
}
static void MainUiEditorCvarInit(){MainUiCvarSpec spec{};const auto& b=g_mainUiDraft[g_mainUiEditSelected];if(!MainUiCvarParse(b.parameter,&spec)){strcpy_s(spec.name,"volume");spec.maximum=1.f;spec.step=.01f;}
 MultiByteToWideChar(CP_UTF8,0,spec.name,-1,g_mainUiCvarFields[0],64);swprintf_s(g_mainUiCvarFields[1],L"%.7g",double(spec.minimum));swprintf_s(g_mainUiCvarFields[2],L"%.7g",double(spec.maximum));swprintf_s(g_mainUiCvarFields[3],L"%.7g",double(spec.step));g_mainUiCvarEditing=true;
}
static bool MainUiEditorCvarStore(){if(g_mainUiEditTextField<3||g_mainUiEditTextField>6)return false;char value[384]{};if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,g_mainUiEditText,-1,value,384,nullptr,nullptr))return false;
 if(g_mainUiEditTextField==3){MainUiCvarSpec tuple{};
  if(MainUiCvarParse(value,&tuple)){MultiByteToWideChar(CP_UTF8,0,tuple.name,-1,g_mainUiCvarFields[0],64);swprintf_s(g_mainUiCvarFields[1],L"%.7g",double(tuple.minimum));swprintf_s(g_mainUiCvarFields[2],L"%.7g",double(tuple.maximum));swprintf_s(g_mainUiCvarFields[3],L"%.7g",double(tuple.step));return true;}
  if(!value[0]||strlen(value)>=64)return false;for(const unsigned char* p=(const unsigned char*)value;*p;++p)if(!isalnum(*p)&&*p!='_')return false;}
 else{if(wcslen(g_mainUiEditText)>=64)return false;char* end=nullptr;float number=strtof(value,&end);if(end==value||*end||!_finite(number))return false;}
 wcscpy_s(g_mainUiCvarFields[g_mainUiEditTextField-3],g_mainUiEditText);return true;
}
static const wchar_t* MainUiExternalHint(int action){
 if(action==905)return UiText(UiId::MainUiPathHint,L"填游戏根相对路径或本机绝对路径；例如 left4dead2\\cfg\\autoexec.cfg 或 D:\\MyFiles\\readme.txt。不加引号，支持空格。");
 if(action==906)return UiText(UiId::MainUiSteamHint,L"填 steam:// 链接；例如 steam://openurl/https://steamcommunity.com/app/550/workshop/ 打开求生工坊。");
 if(action==904)return UiText(UiId::MainUiWebHint,L"填完整 http:// 或 https:// 网址，例如 https://steamcommunity.com/app/550/workshop/。");
 if(action==907)return UiText(UiId::MainUiProfileNameHint,L"填受管方案文件名，例如 night.hud；位于 skeeto/mainmenu/profiles。点击直接读取并切换，不弹文件窗口。");

 return UiText(UiId::MainUiEditorActionHint,L"先选择动作，再填写内容或设置子项。文字元素可编辑标题、字号与颜色。");
}
static const wchar_t* MainUiGameHint(int action){
 if(action==916)return UiText(UiId::MainUiParamLobbyHint,L"格式：模式 [战役ID 章节 难度 权限 服务器 网络]；例：coop L4D2C1 1 Hard friends official LIVE。详见教程。");
 if(action>=911&&action<=915||action==924)return UiText(UiId::MainUiParamModeHint,L"填模式ID：coop / realism / versus / survival / scavenge；突变例如 community1、mutation4、mutation17。团队模式例如 teamversus。");
 if(action==917)return UiText(UiId::MainUiParamPageHint,L"填原生入口：Video / Audio / KeyboardMouse / MultiplayerSettings / Addons / StatsAndAchievements / CloudSettings / Credits 等，详见教程。");
 if(action==918)return UiText(UiId::MainUiParamMapHint,L"格式：地图 模式 [难度]；例如 c1m1_hotel coop Hard。难度可省略：Easy / Normal / Hard / Impossible。");
 if(action>=919&&action<=921)return UiText(UiId::MainUiParamCvarHint,L"设值：volume 0.5；开关：cl_showfps 0 1；循环：volume 0 0.25 0.5 1。遵循游戏变量权限。");
 if(action==922)return UiText(UiId::MainUiParamDemoHint,L"填录像文件名，例如 my_match；可用相对路径，不填 playdemo，文件需存在于游戏搜索路径。");
 return UiText(UiId::MainUiParamNativeHint,L"填已支持的原生菜单动作，例如 SoloPlay_mutation17、FriendsMatch_coop、Video。不是控制台指令或函数地址，完整清单见教程。");
}
static void MainUiEditorTextStart(int field) {
 MainUiServicesCancelText(g_mainUiTextRequest);g_mainUiTextRequest=0;
	g_mainUiEditTextField=field;const auto& b=g_mainUiDraft[g_mainUiEditSelected];
	if(field>=3&&field<=6){if(!g_mainUiCvarEditing)MainUiEditorCvarInit();wcscpy_s(g_mainUiEditText,g_mainUiCvarFields[field-3]);
    }else if(field==8){swprintf_s(g_mainUiEditText,L"%d",b.font>=1008?b.font-1000:b.font==0?14:b.font==2?30:22);}
 else if(field==7){COLORREF rgb=MainUiSlotColor(b.color);swprintf_s(g_mainUiEditText,L"%d %d %d",GetRValue(rgb),GetGValue(rgb),GetBValue(rgb));}else if(field==1)wcscpy_s(g_mainUiEditText,MainUiSlotTitle(b));else MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,b.parameter,-1,g_mainUiEditText,384);
	g_mainUiEditTextAll=true;g_mainUiEditInputSeed=true;g_mainUiEditStatus=0;++g_mainUiTextRevision;

}
static void MainUiEditorPaste() {
	if(!OpenClipboard(g_gameHwnd)){g_mainUiEditStatus=3;return;}
	HANDLE h=GetClipboardData(CF_UNICODETEXT);SIZE_T bytes=h?GlobalSize(h):0;
	const wchar_t* p=bytes>=2&&bytes<=4096?(const wchar_t*)GlobalLock(h):nullptr;
	if(p){size_t length=0,limit=bytes/2;while(length<limit&&p[length])++length;
		if(length<limit&&length<(g_mainUiEditTextField==1?64u:384u)){
			bool valid=true;for(size_t i=0;i<length;++i)if(p[i]<32||p[i]==127)valid=false;
			if(valid){wmemcpy(g_mainUiEditText,p,length);g_mainUiEditText[length]=0;g_mainUiEditTextAll=false;++g_mainUiTextRevision;}else g_mainUiEditStatus=3;
		}else g_mainUiEditStatus=3;GlobalUnlock(h);
	}else g_mainUiEditStatus=3;
	CloseClipboard();
}
static void MainUiEditorTextApply() {
 if(g_mainUiEditTextField==1&&!(g_mainUiDraft[g_mainUiEditSelected].action>=930&&g_mainUiDraft[g_mainUiEditSelected].action<=937)&&wcslen(g_mainUiEditText)>=64){g_mainUiEditStatus=2;return;}
 for(const wchar_t* p=g_mainUiEditText;*p;++p)if(*p<32||*p==127){g_mainUiEditStatus=2;return;}
	auto& b=g_mainUiDraft[g_mainUiEditSelected];char utf8[384]{};
 if(g_mainUiEditTextField==8){wchar_t* end=nullptr;long n=wcstol(g_mainUiEditText,&end,10);if(end==g_mainUiEditText||*end||n<8||n>96){g_mainUiEditStatus=2;return;}b.font=1000+(int)n;g_mainUiEditTextField=0;g_mainUiEditInputSeed=true;++g_mainUiTextRevision;return;}
 if(g_mainUiEditTextField==7){int rgb[3]{};const wchar_t* at=g_mainUiEditText;for(int i=0;i<3;++i){wchar_t* end=nullptr;long n=wcstol(at,&end,10);if(end==at||n<0||n>255||(i<2&&*end!=L' ')){g_mainUiEditStatus=2;return;}rgb[i]=(int)n;at=end;}while(*at==L' ')++at;if(*at){g_mainUiEditStatus=2;return;}b.color=0x1000000|(int)RGB(rgb[0],rgb[1],rgb[2]);g_mainUiEditTextField=0;g_mainUiEditInputSeed=true;++g_mainUiTextRevision;return;}

	int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,g_mainUiEditText,-1,utf8,sizeof(utf8),nullptr,nullptr);
	if(!n||(g_mainUiEditTextField==1&&!(b.action>=930&&b.action<=937)&&strlen(utf8)>=192)
		||(g_mainUiEditTextField==2&&utf8[0]&&!MainUiLayoutParamValid(b.action,utf8))){g_mainUiEditStatus=2;return;}
    if(g_mainUiEditTextField>=3&&g_mainUiEditTextField<=6){if(!MainUiEditorCvarStore()){g_mainUiEditStatus=2;return;}char fields[4][64]{};
        for(int i=0;i<4;++i)if(!WideCharToMultiByte(CP_UTF8,0,g_mainUiCvarFields[i],-1,fields[i],64,nullptr,nullptr)){g_mainUiEditStatus=2;return;}
        snprintf(utf8,sizeof(utf8),"%s %s %s %s",fields[0],fields[1],fields[2],fields[3]);if(!MainUiCvarParse(utf8,nullptr)){g_mainUiEditStatus=2;return;}
        MultiByteToWideChar(CP_UTF8,0,utf8,-1,g_mainUiEditText,384);g_mainUiCvarEditing=false;
    }
	if(g_mainUiEditTextField==1&&b.action>=930&&b.action<=937){strcpy_s(b.parameter,utf8);wcscpy_s(b.paramText,g_mainUiEditText);b.title[0]=0;b.text[0]=0;}
 else if(g_mainUiEditTextField==1){strcpy_s(b.title,utf8);wcscpy_s(b.text,g_mainUiEditText);}else {strcpy_s(b.parameter,utf8);wcscpy_s(b.paramText,g_mainUiEditText);}
	g_mainUiEditTextField=0;g_mainUiEditStatus=0;g_mainUiEditInputSeed=true;++g_mainUiTextRevision;
}
static void MainUiEditorAction(int action) {
	if(action>=kMainUiEditorHitBase&&action<kMainUiEditorHitBase+g_mainUiDraftCount){g_mainUiEditSelected=action-kMainUiEditorHitBase;return;}
	if(!g_mainUiDraftCount&&action!=720&&action!=723&&action!=724&&action!=725&&action!=715&&action!=716&&action!=718)return;
	auto& b=g_mainUiDraft[g_mainUiEditSelected];
    if(action>=892&&action<=898&&g_mainUiEditActionPage>=0){g_mainUiEditActionGroup=action-892;g_mainUiEditActionPage=0;g_mainUiFocus=880;return;}
    if(action>=880&&action<889&&g_mainUiEditActionPage>=0){int at=action-880;
        if(g_mainUiEditActionGroup<0)return;
        int indices[sizeof(kMainUiActions)/sizeof(kMainUiActions[0])]{};int count=MainUiActionGroupIndices(g_mainUiEditActionGroup,indices);at+=g_mainUiEditActionPage*9;
        if(at<count){int chosen=kMainUiActions[indices[at]].action;if(b.action==903&&chosen!=903)MainUiEditorReparentChildren(b.id,b.parent);if(b.action!=chosen){b.parameter[0]=0;b.paramText[0]=0;}b.action=chosen;if(b.action>=930&&b.action<=937)b.parent=-1;else if(b.font>=1008)b.font=1;if(b.action==900){if(b.h<56)b.h=56;}else if(b.action==910){b.w=180;b.h=36;}g_mainUiEditActionPage=-1;g_mainUiEditInputSeed=true;++g_mainUiTextRevision;}return;}
    if(action==889||action==890){int indices[sizeof(kMainUiActions)/sizeof(kMainUiActions[0])]{};int count=g_mainUiEditActionGroup<0?7:MainUiActionGroupIndices(g_mainUiEditActionGroup,indices);int pages=(count+8)/9;g_mainUiEditActionPage=(g_mainUiEditActionPage+(action==889?pages-1:1))%pages;return;}
    if(action==891){g_mainUiEditActionPage=-1;g_mainUiEditInputSeed=true;return;}
	switch(action) {
	case 700:case 701:for(int i=0;i<g_mainUiDraftCount;++i){g_mainUiEditSelected=(g_mainUiEditSelected+(action==700?g_mainUiDraftCount-1:1))%g_mainUiDraftCount;if(g_mainUiDraft[g_mainUiEditSelected].visible||g_mainUiEditShowHidden)break;}break;
	case 738:MainUiEditorTextStart(7);break;
 case 739:MainUiEditorTextStart(8);break;
	case 702:MainUiEditorTextStart(1);break;
	case 703:g_mainUiEditActionGroup=-1;g_mainUiEditActionPage=0;g_mainUiFocus=892;g_mainUiEditInputSeed=true;break;
	case 704:if(b.action==910){g_mainUiCvarEditing=false;MainUiEditorTextStart(3);}else if((b.action>=900&&b.action<=925&&b.action!=903))MainUiEditorTextStart(2);break;
	case 705:b.visible=!b.visible;break;
    case 718:g_mainUiEditShowHidden=!g_mainUiEditShowHidden;break;
	case 706:b.font=(b.font+1)%3;break;
	case 707:b.color=b.color>=11?-1:b.color+1;break;
	case 708:b.opacity=b.opacity>=100?-1:b.opacity<0?0:b.opacity+10;break;
	case 709:b.w-=10;break;case 710:b.w+=10;break;case 711:b.h-=2;break;case 712:b.h+=2;break;
	case 713:case 714:{int to=g_mainUiEditSelected+(action==713?-1:1);if(to>=0&&to<g_mainUiDraftCount){auto tmp=g_mainUiDraft[to];g_mainUiDraft[to]=b;b=tmp;g_mainUiEditSelected=to;}break;}
	case 715:g_mainUiDraftDecor=!g_mainUiDraftDecor;break;
	case 716:g_mainUiEditSnap=!g_mainUiEditSnap;break;
	case 717:{int parents[kMainUiLayoutMax+1]{-1},n=1,at=0;
		bool descendants[kMainUiLayoutMax]{};MainUiEditorMarkTree(g_mainUiEditSelected,descendants);
		for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].action==903&&!descendants[i])parents[n++]=g_mainUiDraft[i].id;
		for(int i=0;i<n;++i)if(parents[i]==b.parent)at=i;b.parent=parents[(at+1)%n];break;}
	case 720:case 721: {
		if(g_mainUiDraftCount>=g_mainUiButtonLimit){g_mainUiEditStatus=1;return;}
		MainUiLayoutSlot next=action==721?b:MainUiLayoutBlank();next.id=MainUiEditorUnusedId();
		if(action==720){next.action=902;next.parent=!g_mainUiDraftCount?-1:b.action==903?b.id:b.parent;next.x=340;next.y=125;next.w=232;next.h=42;}
		else {next.x+=12;next.y+=12;}
		MainUiLayoutClamp(next);g_mainUiDraft[g_mainUiDraftCount]=next;g_mainUiEditSelected=g_mainUiDraftCount++;break;}
	case 722:
		MainUiEditorDeleteTree();
		break;
	case 723:g_mainUiDraftDefaultBackdrop=true;MainUiLayoutDefault(g_mainUiDraft,&g_mainUiDraftCount);g_mainUiDraftDecor=true;g_mainUiEditSelected=0;break;
	case 724:
		memcpy(g_mainUiLayout,g_mainUiDraft,sizeof(g_mainUiLayout));g_mainUiLayoutCount=g_mainUiDraftCount;g_mainUiHomeDecor=g_mainUiDraftDecor;
		if(g_mainUiDraftDefaultBackdrop)g_mainUiBackdrop=0;MainUiEditorAbort();MainUiSettingsChanged();return;
	case 725:MainUiEditorAbort();return;
	case 730:MainUiEditorPaste();return;
	case 731:MainUiEditorTextApply();return;
	case 737:{if(g_mainUiTextRequest){g_mainUiEditStatus=4;return;}char caption[512]{};WideCharToMultiByte(CP_UTF8,0,UiText(UiId::MainUiSystemInput,L"Skeeto 文本输入（支持中文）"),-1,caption,512,nullptr,nullptr);g_mainUiTextRequest=MainUiServicesText(g_mainUiEditText,caption);if(!g_mainUiTextRequest)g_mainUiEditStatus=3;return;}
	case 732:MainUiServicesCancelText(g_mainUiTextRequest);g_mainUiTextRequest=0;g_mainUiCvarEditing=false;g_mainUiEditTextField=0;g_mainUiEditInputSeed=true;return;
    case 733:case 734:case 735:case 736:if(MainUiEditorCvarStore())MainUiEditorTextStart(action-730);else g_mainUiEditStatus=2;return;
	}
	for(int i=0;i<g_mainUiDraftCount;++i)MainUiLayoutClamp(g_mainUiDraft[i]);
	++g_mainUiTextRevision;
}
static void MainUiEditorAddServer(int parent){
 MainUiEditorBegin();if(!g_mainUiEditing)return;
 int at=-1;for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].id==parent&&(g_mainUiDraft[i].action==903||g_mainUiDraft[i].action==217))at=i;
 if(at<0)for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].action==903&&g_mainUiDraft[i].parent<0&&!wcscmp(g_mainUiDraft[i].text,UiText(UiId::MainUiFavoritesTab,L"收藏服务器"))){at=i;break;}
 if(g_mainUiDraftCount+(at<0?2:1)>g_mainUiButtonLimit){g_mainUiEditStatus=1;return;}
 // Explicit Add also works from a list action, never placing the entry on Home.
 if(at<0){at=g_mainUiDraftCount++;auto& menu=g_mainUiDraft[at];menu=MainUiLayoutBlank();menu.id=MainUiEditorUnusedId();menu.x=560;menu.y=262;
  wcscpy_s(menu.text,UiText(UiId::MainUiFavoritesTab,L"收藏服务器"));WideCharToMultiByte(CP_UTF8,0,menu.text,-1,menu.title,192,nullptr,nullptr);}
 g_mainUiDraft[at].action=903;parent=g_mainUiDraft[at].id;g_mainUiEditSelected=at;
 MainUiEditorAction(720);if(g_mainUiEditStatus==1)return;
 auto& b=g_mainUiDraft[g_mainUiEditSelected];b.action=900;b.parent=parent;b.h=64;
 wcscpy_s(b.text,UiText(UiId::MainUiNewServer,L"新服务器（点击修改标题）"));WideCharToMultiByte(CP_UTF8,0,b.text,-1,b.title,192,nullptr,nullptr);++g_mainUiTextRevision;
}
static void MainUiEditorPoll() {
 if(g_mainUiEditTextField&&g_mainUiTextRequest){wchar_t input[384]{};auto result=MainUiServicesTakeText(g_mainUiTextRequest,input,384);if(result!=MainUiTextResult::None){g_mainUiTextRequest=0;if(result==MainUiTextResult::Accepted){wcscpy_s(g_mainUiEditText,input);g_mainUiEditTextAll=false;++g_mainUiTextRevision;MainUiEditorTextApply();}else if(result==MainUiTextResult::Failed)g_mainUiEditStatus=3;}}

	const int keys[]={VK_LBUTTON,VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_TAB,VK_RETURN,VK_ESCAPE,0,VK_BACK,VK_DELETE,
		'V','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','W','X','Y','Z',
		'0','1','2','3','4','5','6','7','8','9',VK_SPACE,VK_OEM_PERIOD,VK_OEM_MINUS,VK_OEM_1,VK_OEM_2,VK_OEM_5,VK_OEM_7};
	bool down[64]{};const int n=(int)(sizeof(keys)/sizeof(keys[0]));
	const int pollCount=g_mainUiEditTextField?n:9;
	for(int i=0;i<pollCount;++i)down[i]=keys[i] && (GetAsyncKeyState(keys[i])&0x8000)!=0;
	if(g_mainUiEditInputSeed){memcpy(g_mainUiEditKeys,down,sizeof(down));g_mainUiEditInputSeed=false;g_mainUiEditDrag=-1;g_mainUiEditPressed=-1;return;}
	auto edge=[&](int key){for(int i=0;i<n;++i)if(keys[i]==key)return down[i]&&!g_mainUiEditKeys[i];return false;};
	bool shift=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0,ctrl=(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;
	int mx=0,my=0;bool mouse=MainUiMouse(&mx,&my);int hit=mouse?MainUiHit(mx,my):-1;
	static int lastHover=-1;
	if(mouse&&hit>=0&&hit!=lastHover&&!down[0])MainUiSound(false);
	lastHover=hit;
	if(g_mainUiEditTextField) {
		if(edge(VK_ESCAPE))MainUiEditorAction(732);
		else if(edge(VK_RETURN))MainUiEditorTextApply();
		else if(ctrl&&edge('V'))MainUiEditorPaste();
		else if(ctrl&&edge('A'))g_mainUiEditTextAll=true;
		else if(edge(VK_BACK)||edge(VK_DELETE)) {size_t len=wcslen(g_mainUiEditText);if(g_mainUiEditTextAll||edge(VK_DELETE))g_mainUiEditText[0]=0;else if(len)g_mainUiEditText[len-1]=0;g_mainUiEditTextAll=false;++g_mainUiTextRevision;}
		else if(!ctrl)for(int i=0;i<n;++i)if(down[i]&&!g_mainUiEditKeys[i]) {
			int key=keys[i];wchar_t c=key>='A'&&key<='Z'?(wchar_t)((shift!=((GetKeyState(VK_CAPITAL)&1)!=0))?key:key+32):key>='0'&&key<='9'?(wchar_t)key:key==VK_SPACE?L' ':key==VK_OEM_PERIOD?L'.':key==VK_OEM_MINUS?(shift?L'_':L'-'):key==VK_OEM_1?(shift?L':':L';'):key==VK_OEM_2?L'/':key==VK_OEM_5?(shift?L'|':L'\\'):key==VK_OEM_7?(shift?L'"':L'\''):0;
			if(c){if(g_mainUiEditTextAll)g_mainUiEditText[0]=0;g_mainUiEditTextAll=false;size_t len=wcslen(g_mainUiEditText);if(len<(g_mainUiEditTextField==1?63u:383u)){g_mainUiEditText[len]=c;g_mainUiEditText[len+1]=0;++g_mainUiTextRevision;}}
		}
	} else if(g_mainUiEditActionPage>=0) {
		if(edge(VK_ESCAPE))MainUiEditorAction(891);
		else if(edge(VK_TAB)||edge(VK_UP)||edge(VK_DOWN))g_mainUiFocus=MainUiFocusStep(edge(VK_UP)||shift?-1:1);
		else if(edge(VK_RETURN))MainUiEditorAction(g_mainUiFocus);
	} else {
		if(edge(VK_ESCAPE))MainUiEditorAbort();
		else if(edge(VK_TAB))g_mainUiFocus=MainUiFocusStep(shift?-1:1);
		else if(edge(VK_RETURN)&&g_mainUiFocus>=700)MainUiEditorAction(g_mainUiFocus);
		else if(edge(VK_UP)||edge(VK_DOWN)||edge(VK_LEFT)||edge(VK_RIGHT)) {
			auto& b=g_mainUiDraft[g_mainUiEditSelected];int d=shift?10:1;if(b.parent>=0)d=0;
			int dx=edge(VK_RIGHT)?d:edge(VK_LEFT)?-d:0,dy=edge(VK_DOWN)?d:edge(VK_UP)?-d:0;
			if(ctrl){b.w+=dx;b.h+=dy;}else{b.x+=dx;b.y+=dy;}MainUiLayoutClamp(b);
		}
	}
	if(edge(VK_LBUTTON)){g_mainUiEditPressed=hit;
		if(!g_mainUiEditTextField&&hit>=kMainUiEditorHitBase&&hit<kMainUiEditorHitBase+g_mainUiDraftCount){MainUiSound(true);g_mainUiEditSelected=hit-kMainUiEditorHitBase;g_mainUiEditDrag=g_mainUiDraft[g_mainUiEditSelected].parent<0?g_mainUiEditSelected:-1;g_mainUiEditStartX=mx;g_mainUiEditStartY=my;g_mainUiEditOriginal=g_mainUiDraft[g_mainUiEditSelected];
			g_mainUiEditResize=false;for(int i=0;i<g_mainUiButtonCount;++i){const auto& r=g_mainUiButtons[i];if(r.action==hit&&mx>=r.x1-12&&my>=r.y1-12)g_mainUiEditResize=true;}}
	}
	if(down[0]&&mouse&&g_mainUiEditDrag>=0&&g_mainUiEditDrag<g_mainUiDraftCount) {
		auto& b=g_mainUiDraft[g_mainUiEditDrag];int dx=(int)((mx-g_mainUiEditStartX)/g_mainUiEditCanvasScale),dy=(int)((my-g_mainUiEditStartY)/g_mainUiEditCanvasScale);
		if(g_mainUiEditResize){b.w=g_mainUiEditOriginal.w+dx;b.h=g_mainUiEditOriginal.h+dy;}else{b.x=g_mainUiEditOriginal.x+dx;b.y=g_mainUiEditOriginal.y+dy;}
		if(g_mainUiEditSnap&&!shift){b.x=(b.x+4)/8*8;b.y=(b.y+4)/8*8;b.w=(b.w+4)/8*8;b.h=(b.h+4)/8*8;}MainUiLayoutClamp(b);
	}
	if(!down[0]&&g_mainUiEditKeys[0]) {
		if(g_mainUiEditDrag<0&&hit==g_mainUiEditPressed&&hit>=700){g_mainUiFocus=hit;MainUiSound(true);MainUiEditorAction(hit);}
		g_mainUiEditDrag=-1;g_mainUiEditPressed=-1;
	}
	memcpy(g_mainUiEditKeys,down,sizeof(down));
}
static void MainUiDrawEditor(bool registration) {
	float s=(g_mainUiSw-48.f)/1040.f;float q=(g_mainUiSh-48.f)/600.f;if(q<s)s=q;
	auto U=[=](int n){return(int)(n*s+.5f);};int ox=(g_mainUiSw-U(1040))/2,oy=(g_mainUiSh-U(600))/2;
	const COLORREF white=RGB(244,245,246),muted=RGB(180,188,197),accent=MainUiAccentColor();
	if(registration)g_mainUiButtonCount=0;
	auto text=[&](int x,int y,const wchar_t* value,int w,COLORREF color){if(!registration)MainUiText(g_mainUiFonts[2],ox+U(x),oy+U(y),color,value,U(w));};
	static wchar_t valueText[5][128]{};static unsigned valueVersion[5]{};
	auto value=[&](int row,int x,int y,int w,const wchar_t* number,COLORREF color){
		if(registration)return;
		if(wcscmp(valueText[row],number)){wcscpy_s(valueText[row],number);++valueVersion[row];}
		MainUiText(g_mainUiFonts[2],ox+U(x),oy+U(y),color,valueText[row],U(w),valueVersion[row]);
	};
	auto button=[&](int x,int y,int w,int h,int id,const wchar_t* title){
		if(registration){if(g_mainUiButtonCount<kMainUiHitMax)g_mainUiButtons[g_mainUiButtonCount++]={ox+U(x),oy+U(y),ox+U(x+w),oy+U(y+h),id};return;}
		SurfColor(18,25,32,230);SurfFill(ox+U(x),oy+U(y),ox+U(x+w),oy+U(y+h));
		text(x+8,y+8,title,w-16,(g_mainUiFocus==id||(id>=733&&id<=736&&g_mainUiEditTextField==id-730))?accent:white);
	};
	text(0,6,UiText(UiId::MainUiEditorTitle,L"主菜单编辑"),640,white);
	text(0,38,UiText(UiId::MainUiEditorHint,L"拖动移动；右下角缩放；方向键微调，Ctrl 调整大小，Shift 加速。"),1040,muted);
	const int cx=ox,cy=oy+U(85),cw=U(710),ch=U(410);g_mainUiEditCanvasScale=(float)cw/1040.f;
	static unsigned long previewFonts[3]{};static int previewPx[3]{},previewActualPx[3]{},previewEpoch=0;
	if(!registration){const int base[]={g_mainUiFontPx[2],g_mainUiFontPx[1],g_mainUiFontPx[3]};
		for(int i=0;i<3;++i){int px=(int)(base[i]*710.f/1040.f+.5f);if(previewEpoch!=g_sktFontEpoch||previewPx[i]!=px||!previewFonts[i]){
			int actual=0;if(HudAcquireLayoutFont(px,&previewFonts[i],&actual,700)){previewActualPx[i]=actual;}
			else {previewFonts[i]=g_mainUiFonts[2];previewActualPx[i]=g_mainUiFontPx[2];}
			// Cache a fallback too; retry only on resolution/font-epoch changes.
			previewPx[i]=px;}}
		previewEpoch=g_sktFontEpoch;}
	if(!registration){SurfColor(12,18,24,180);SurfFill(cx,cy,cx+cw,cy+ch);SurfColor(99,115,127,180);SurfLine(cx,cy,cx+cw,cy);SurfLine(cx,cy+ch,cx+cw,cy+ch);}
	int viewParent=g_mainUiDraftCount?g_mainUiDraft[g_mainUiEditSelected].parent:-1;
	int childRow=0;
	for(int i=0;i<g_mainUiDraftCount;++i){const auto& b=g_mainUiDraft[i];if(b.parent!=viewParent||(!b.visible&&!g_mainUiEditShowHidden))continue;float z=g_mainUiEditCanvasScale;
		int bx=b.x,by=b.y,bw=b.w,bh=b.h;
		if(viewParent>=0){int row=childRow++;int selectedRow=0;for(int j=0;j<g_mainUiEditSelected;++j)if(g_mainUiDraft[j].parent==viewParent&&(g_mainUiDraft[j].visible||g_mainUiEditShowHidden))++selectedRow;
		bool serverMenu=viewParent==217;for(int k=0;k<g_mainUiDraftCount;++k)if(g_mainUiDraft[k].parent==viewParent&&(g_mainUiDraft[k].action==900||g_mainUiDraft[k].action==910))serverMenu=true;int perPage=serverMenu?5:9;if(row/perPage!=selectedRow/perPage)continue;bx=312;by=175+(row%perPage)*(serverMenu?60:36);bw=b.action==910?(b.w<320?b.w:320):728;bh=b.action==910?36:serverMenu?56:32;}
		int x=cx+(int)(bx*z+.5f),y=cy+(int)(by*z+.5f),w=(int)(bw*z+.5f),h=(int)(bh*z+.5f);
		if(registration){if(g_mainUiButtonCount<kMainUiHitMax)g_mainUiButtons[g_mainUiButtonCount++]={x,y,x+w,y+h,kMainUiEditorHitBase+i};continue;}
		COLORREF c=!b.visible?muted:b.color>=0?MainUiSlotColor(b.color):white;
		int opacity=b.opacity<0?(g_mainUiDraftDefaultBackdrop?0:g_mainUiBackdrop):b.opacity;
		if(b.visible&&opacity>0)SurfColor(20,25,31,MulDiv(216,opacity,100));
		else SurfColor(GetRValue(c),GetGValue(c),GetBValue(c),!b.visible?35:i==g_mainUiEditSelected?28:18);
		SurfFill(x,y,x+w,y+h); // Subtle editable bounds, even for text-only buttons.
		// At small resolutions the engine's 12px font floor exceeds tiny preview
		// cells. Keep their selectable outlines and show the title in the inspector.
		if(b.action>=930&&b.action<=937){int px=MainUiElementPixels(b,z);if(px>h-2)px=h-2;if(h>=14)MainUiText(MainUiElementFont(px),x+3,y+2,c,MainUiSlotTitle(b),w-6);}
        else {int tier=b.font>2?1:b.font;if(h>=previewActualPx[tier]+2)MainUiText(previewFonts[tier],x+3,y+2,c,MainUiSlotTitle(b),w-6);}
		if(i==g_mainUiEditSelected){SurfColor(GetRValue(accent),GetGValue(accent),GetBValue(accent),255);SurfLine(x,y,x+w,y);SurfLine(x+w,y,x+w,y+h);SurfLine(x+w,y+h,x,y+h);SurfLine(x,y+h,x,y);SurfFill(x+w-5,y+h-5,x+w,y+h);}
	}
	wchar_t number[128]{};
	if(g_mainUiDraftCount){auto& b=g_mainUiDraft[g_mainUiEditSelected];
	button(738,85,44,30,700,L"<");button(986,85,44,30,701,L">");
	if(!registration){swprintf_s(number,L"%d / %d",g_mainUiEditSelected+1,g_mainUiDraftCount);value(0,806,94,172,number,white);}
	button(738,122,292,32,702,MainUiSlotTitle(b));
	button(738,160,292,32,703,MainUiActionTitle(b.action));
	text(738,197,b.action==910?UiText(UiId::MainUiCvarFormat,L"独立设置变量名、最小值、最大值和步进"):b.action==900?UiText(UiId::MainUiEditorServerEntryHint,L"标题填服名，参数填 IP:端口。"):UiText(UiId::MainUiEditorActionHint,L"先选择动作，再填写内容或设置子项。文字元素可编辑标题、字号与颜色。"),292,muted);
	if((b.action>=900&&b.action<=925&&b.action!=903)){
		button(738,222,292,32,704,UiText(b.action==900?UiId::MainUiEditorServerAddress:b.action==902?UiId::MainUiEditorCommand:UiId::MainUiEditorParameter,b.action==900?L"编辑服务器 IP / 端口":b.action==902?L"编辑控制台指令":L"编辑动作参数"));
		text(746,260,b.paramText,276,muted);
	}else if(b.action==217){
        text(746,232,UiText(UiId::MainUiEditorServerListHint,L"这是列表入口，不填写 IP。"),276,muted);
        text(746,260,UiText(UiId::MainUiEditorAddServerHint,L"新增条目选「服务器条目」。"),276,muted);
    }else if(b.action==903){
        text(746,232,UiText(UiId::MainUiEditorAddChildHint,L"选中菜单，点「新增按钮」。"),276,muted);
        text(746,260,UiText(UiId::MainUiEditorChildHint,L"子项自动放入此菜单。"),276,muted);
    }else text(746,232,UiText(UiId::MainUiEditorNoParameter,L"此动作无需填写参数。"),276,muted);
	button(738,286,140,30,705,UiText(b.visible?UiId::MainUiEditorVisible:UiId::MainUiEditorHidden,b.visible?L"显示按钮":L"隐藏按钮"));
	static const UiTextEntry fonts[]={{UiId::MainUiEditorSmall,L"小号文字"},{UiId::MainUiEditorNormal,L"标准文字"},{UiId::MainUiEditorLarge,L"大号文字"}};
	button(890,286,140,30,b.action>=930&&b.action<=937?739:706,b.action>=930&&b.action<=937?UiText(UiId::MainUiExactFont,L"精确字号"):UiText(fonts[b.font>2?1:b.font].id,fonts[b.font>2?1:b.font].chinese));
	button(738,322,140,30,707,UiText(UiId::MainUiEditorColor,L"切换文字颜色"));
	button(890,322,140,30,708,UiText(UiId::MainUiEditorOpacity,L"切换背景强度"));
	button(738,357,140,28,738,UiText(UiId::MainUiExactRgb,L"精确 RGB"));
	if(!registration){if(b.color>=0x1000000){COLORREF c=MainUiSlotColor(b.color);swprintf_s(number,L"RGB %d %d %d",GetRValue(c),GetGValue(c),GetBValue(c));}else swprintf_s(number,UiText(UiId::MainUiEditorAppearance,L"颜色 %d / 12   背景 %d%%"),b.color+1,b.opacity);value(1,890,359,140,number,muted);}
	button(738,384,56,28,709,L"W -");button(800,384,56,28,710,L"W +");button(864,384,76,28,711,L"H -");button(946,384,84,28,712,L"H +");
	if(!registration){swprintf_s(number,L"X %d   Y %d   W %d   H %d",b.x,b.y,b.w,b.h);value(2,738,420,292,number,muted);}
	button(738,447,140,30,713,UiText(UiId::MainUiEditorLayerBack,L"排列前移"));button(890,447,140,30,714,UiText(UiId::MainUiEditorLayerFront,L"排列后移"));
	const wchar_t* parentTitle=UiText(UiId::MainUiEditorHomeParent,L"主菜单");
	for(int i=0;i<g_mainUiDraftCount;++i)if(g_mainUiDraft[i].id==b.parent)parentTitle=g_mainUiDraft[i].text[0]?g_mainUiDraft[i].text:MainUiActionTitle(903);
	if(!registration)swprintf_s(number,UiText(UiId::MainUiEditorParent,L"按钮从属：%ls（点击更换）"),parentTitle);
	button(738,483,292,28,717,L"");if(!registration)value(3,746,491,276,number,white);
	}else text(738,122,UiText(UiId::MainUiCustomEmpty,L"暂无按钮；点击「新增按钮」。"),292,muted);
	button(0,510,226,30,715,UiText(g_mainUiDraftDecor?UiId::MainUiEditorDecorOn:UiId::MainUiEditorDecorOff,g_mainUiDraftDecor?L"主菜单欢迎文字：开":L"主菜单欢迎文字：关"));
	button(238,510,226,30,716,UiText(g_mainUiEditSnap?UiId::MainUiEditorSnapOn:UiId::MainUiEditorSnapOff,g_mainUiEditSnap?L"8 像素吸附：开（Shift 暂停）":L"8 像素吸附：关"));
	button(476,510,234,30,718,UiText(g_mainUiEditShowHidden?UiId::MainUiEditorHideHidden:UiId::MainUiEditorShowHidden,g_mainUiEditShowHidden?L"隐藏项：显示（点击收起）":L"显示隐藏项（用于恢复）"));
	const UiTextEntry toolbar[]={{UiId::MainUiEditorAdd,L"新增按钮"},{UiId::MainUiEditorDuplicate,L"复制按钮"},{UiId::MainUiEditorDelete,L"删除按钮及子项"},{UiId::MainUiEditorReset,L"恢复默认"},{UiId::MainUiEditorApply,L"应用并保存"},{UiId::MainUiCancel,L"取消"}};
	for(int i=0;i<6;++i)button(i*174,551,164,34,720+i,UiText(toolbar[i].id,toolbar[i].chinese));
	if(g_mainUiEditStatus==1){if(!registration){swprintf_s(number,UiText(UiId::MainUiEditorLimit,L"按钮数量已达 %d 个；在主菜单设置调整上限。"),g_mainUiButtonLimit);text(0,590,number,1040,accent);}}
	else if(g_mainUiEditStatus)text(0,590,UiText(g_mainUiEditStatus==2?UiId::MainUiEditorInvalid:UiId::MainUiEditorPasteError,
		g_mainUiEditStatus==2?L"内容无效或超长；请检查动作参数或主菜单方案格式。":L"无法粘贴：请使用较短的纯文本并重试。"),1040,accent);
	if(g_mainUiEditTextField) {
		if(registration)g_mainUiButtonCount=0;
		if(!registration){SurfColor(12,18,24,255);SurfFill(ox+U(80),oy+U(190),ox+U(960),oy+U(432));}
		text(106,211,UiText(UiId::MainUiEditorTextHint,L"键盘输入 / Ctrl+V 粘贴（支持中文） / Ctrl+A 全选；Enter 应用，Esc 取消。"),826,white);
		if(g_mainUiDraft[g_mainUiEditSelected].action==910&&g_mainUiEditTextField!=7){const UiTextEntry fields[]={{UiId::MainUiCvarName,L"变量名"},{UiId::MainUiCvarMin,L"最小值"},{UiId::MainUiCvarMax,L"最大值"},{UiId::MainUiCvarStep,L"步进"}};static wchar_t titles[4][96]{};for(int i=0;i<4;++i){if(!registration)swprintf_s(titles[i],L"%s: %s",UiText(fields[i].id,fields[i].chinese),g_mainUiCvarFields[i]);button(106+i*210,237,196,28,733+i,titles[i]);}}
        text(106,278,g_mainUiEditText,826,accent);
        if(g_mainUiEditTextField==8)text(106,306,UiText(UiId::MainUiFontHint,L"输入参考字号8–96；分辨率自适应开启时按布局比例缩放。"),826,muted);
        if(g_mainUiEditTextField==7)text(106,306,UiText(UiId::MainUiRgbHint,L"依次输入 R G B，范围0–255，例如 105 222 235。"),826,muted);
        else if(g_mainUiDraft[g_mainUiEditSelected].action>=911&&g_mainUiDraft[g_mainUiEditSelected].action<=925)text(106,306,MainUiGameHint(g_mainUiDraft[g_mainUiEditSelected].action),826,muted);
        else if(g_mainUiDraft[g_mainUiEditSelected].action>=904&&g_mainUiDraft[g_mainUiEditSelected].action<=909)text(106,306,MainUiExternalHint(g_mainUiDraft[g_mainUiEditSelected].action),826,muted);
        else if(g_mainUiDraft[g_mainUiEditSelected].action==910)text(106,306,UiText(UiId::MainUiCvarExample,L"分别填写变量名 / 最小值 / 最大值 / 步进；变量名栏也可粘贴完整四项。"),826,muted);
		button(106,347,238,40,730,UiText(UiId::MainUiEditorPaste,L"从剪贴板粘贴"));button(356,347,270,40,731,UiText(UiId::MainUiEditorTextApply,L"应用文字"));button(638,347,294,40,732,UiText(UiId::MainUiCancel,L"取消"));
        button(106,394,826,34,737,UiText(UiId::MainUiSystemInput,L"Skeeto 文本输入（支持中文）"));
	}
    if(g_mainUiEditActionPage>=0){
        if(registration)g_mainUiButtonCount=0;
        if(!registration){SurfColor(12,18,24,255);SurfFill(ox+U(86),oy+U(65),ox+U(950),oy+U(552));SurfColor(42,52,64,255);SurfFill(ox+U(340),oy+U(110),ox+U(342),oy+U(484));}
        text(104,85,UiText(UiId::MainUiEditorChooseAction,L"选择按钮动作"),826,accent);
        for(int i=0;i<7;++i){button(104,125+i*52,218,42,892+i,UiText(kMainUiActionGroups[i].id,kMainUiActionGroups[i].chinese));if(!registration&&g_mainUiEditActionGroup==i){SurfColor(GetRValue(accent),GetGValue(accent),GetBValue(accent),255);SurfFill(ox+U(104),oy+U(125+i*52),ox+U(106),oy+U(167+i*52));}}
        int indices[sizeof(kMainUiActions)/sizeof(kMainUiActions[0])]{};int total=g_mainUiEditActionGroup<0?0:MainUiActionGroupIndices(g_mainUiEditActionGroup,indices),first=g_mainUiEditActionPage*9;
        for(int i=0;i<9&&first+i<total;++i){const auto& title=kMainUiActions[indices[first+i]].title;button(360,121+i*40,568,34,880+i,UiText(title.id,title.chinese));}
        if(!total)text(360,140,UiText(UiId::MainUiChooseCategory,L"选择左侧分类，再选择具体动作。"),568,muted);
        if(total>9){button(360,502,174,34,889,L"<");button(754,502,174,34,890,L">");}
        button(546,502,196,34,891,UiText(UiId::MainUiCancel,L"取消"));
    }
	if(!registration&&!g_mainUiUnderlay.load(std::memory_order_relaxed)){int mx=0,my=0;if(MainUiMouse(&mx,&my)){
		SurfColor(0,0,0,255);SurfLine(mx+1,my+1,mx+1,my+17);SurfLine(mx+1,my+1,mx+12,my+12);
		SurfColor(255,255,255,255);SurfLine(mx,my,mx,my+16);SurfLine(mx,my,mx+11,my+11);}}
}
