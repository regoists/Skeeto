// Homepage hub: Paint-owned presentation/profile application; worker snapshots.
// Included by proxy after homepage/editor/config helpers.
#pragma once
#include <ctime>
static MainUiHubSnapshot g_mainUiHub{};
static DWORD g_mainUiHubReadAt=0;
static int g_mainUiServerTab=0,g_mainUiHubPage=0,g_mainUiSaveSlot=0;
static unsigned g_mainUiHubRowsRevision=~0u;
static wchar_t g_mainUiHubRows[12][192]{},g_mainUiHubDetails[12][192]{};
static char g_mainUiQueryAddresses[kMainUiQueryMax][96]{};static int g_mainUiQueryAddressCount=-1;static DWORD g_mainUiQueryCollectAt=0;
static char g_mainUiLastRecordedServer[96]{};
static void MainUiHubJob(bool ok){g_mainUiNotice=ok?0:3;}
static void MainUiHubParameterized(int action,const char* parameter){
 if(action>=904&&action<=906){MainUiHubJob(MainUiServicesOpen(action-904,parameter));return;}
 if(action==907){MainUiHubJob(MainUiServicesProfile(2,parameter));return;}

}
static void MainUiHubSelect(int panel){MainUiSelectPanel(panel);g_mainUiHubPage=0;g_mainUiHubRowsRevision=~0u;MainUiHubJob(MainUiServicesRefresh());}
static int MainUiHubFavorites(int* rows){
 static int saved[kMainUiLayoutMax]{},n=0,layoutCount=-1;static unsigned revision=~0u;
 if(layoutCount!=g_mainUiLayoutCount||revision!=g_mainUiTextRevision){n=0;
  for(int i=0;i<g_mainUiLayoutCount;++i){const auto& b=g_mainUiLayout[i];if(!b.visible||b.action!=900||!MainUiLayoutParamValid(900,b.parameter))continue;bool duplicate=false;for(int j=0;j<n;++j)if(!_stricmp(g_mainUiLayout[saved[j]].parameter,b.parameter))duplicate=true;if(!duplicate)saved[n++]=i;}
  layoutCount=g_mainUiLayoutCount;revision=g_mainUiTextRevision;
 }
 memcpy(rows,saved,n*sizeof(int));return n;
}
static bool MainUiSameServer(const char* a,const char* b){
 if(!a||!b||!a[0]||!b[0])return false;if(!_stricmp(a,b))return true;
 return ListenAddrIsLoopback(a)&&ListenAddrIsLoopback(b);
}
static bool MainUiOpenBattleReport(){
  CareerArchiveRefresh();MainUiCareerReport career{};career.stats=g_careerArchive;
  for(int i=0;i<career.stats.topWeaponCount&&i<6;++i)WideCharToMultiByte(CP_UTF8,0,CareerWeaponName(career.stats.topWeapons[i].source),-1,career.weaponNames[i],96,nullptr,nullptr);
  MainUiFillReportIdentity(&career);
  MainUiBattleLabel labels[10]{};int labelCount=0;MainUiHubSnapshot snap{};
  if(MainUiServicesCopy(&snap)){labelCount=snap.battleCount<0?0:snap.battleCount>10?10:snap.battleCount;
   MainUiServerRecord server{};if(TryAcquireSRWLockShared(&g_mainUiServerLock)){server=g_mainUiCurrentServer;ReleaseSRWLockShared(&g_mainUiServerLock);}
   for(int i=0;i<labelCount;++i){const auto& b=snap.battles[i];auto& label=labels[i];label.time=b.time;strncpy_s(label.map,b.map,_TRUNCATE);
    if(!b.campaign[0]||!b.chapter[0]){wchar_t campaign[64]{},chapter[64]{};MapHudCopyDisplayNames(b.map,campaign,64,chapter,64);
     if(!b.campaign[0])WideCharToMultiByte(CP_UTF8,0,campaign,-1,label.campaign,sizeof(label.campaign),nullptr,nullptr);
     if(!b.chapter[0])WideCharToMultiByte(CP_UTF8,0,chapter,-1,label.chapter,sizeof(label.chapter),nullptr,nullptr);}
    if(!b.serverName[0]&&server.name[0]&&MainUiSameServer(b.server,server.address))strncpy_s(label.serverName,server.name,_TRUNCATE);}}
  return MainUiServicesOpenReport(career,labels,labelCount);
}
static bool MainUiHubDispatch(int action){
 if(action==0){MainUiOpenLobbyPreset("coop - - Impossible friends listen");return true;}
 if(action==217){g_mainUiServerTab=1;MainUiHubSelect(5);return true;}
 if(action==218||action==219){if(action==219)MainUiHubJob(MainUiServicesProfile(1,nullptr));else{char* body=g_mainUiConfigBody;MainUiHubJob(MainUiConfigCapture(body)&&MainUiServicesProfile(0,nullptr,body));}return true;}
 if(action>=209&&action<=216){switch(action){case 209:g_mainUiServerTab=0;MainUiHubSelect(5);break;case 210:MainUiHubSelect(6);break;case 211:MainUiHubSelect(7);break;case 212:MainUiHubJob(MainUiServicesOpen(3,nullptr));break;case 213:MainUiHubJob(MainUiServicesOpen(2,"steam://url/CommunityFilePage/3787239937"));break;case 214:MainUiHubJob(MainUiServicesOpen(1,"left4dead2\\skeeto"));break;case 216:MainUiHubSelect(8);break;}return true;}
 if(g_mainUiPanel<5)return false;
 if(action==540){g_optMainUiRememberServers=!g_optMainUiRememberServers;MainUiSettingsChanged();}
 else if(action==541){g_optMainUiServerQuery=!g_optMainUiServerQuery;MainUiSettingsChanged();}
 else if(action==542){g_mainUiServerTab=!g_mainUiServerTab;g_mainUiHubPage=0;}
 else if(action==543){g_optMainUiBattleArchive=!g_optMainUiBattleArchive;MainUiSettingsChanged();}
 else if(action==544){
  MainUiHubJob(MainUiOpenBattleReport());
 }
 else if(action==545){g_mainUiSaveSlot=(g_mainUiSaveSlot+1)%8;}
 else if(action==546||action==547){char* body=g_mainUiConfigBody;if(MainUiConfigCapture(body)){char name[48]{};sprintf_s(name,"profile_%02d.hud",g_mainUiSaveSlot+1);MainUiHubJob(MainUiServicesProfile(action==546?3:0,name,body));}}
 else if(action==553){MainUiEditorAddServer(217);return true;}
 else if(action==548)MainUiHubJob(MainUiServicesProfile(1,nullptr));
 else if(action==549){int rows[kMainUiLayoutMax]{};int count=g_mainUiPanel==5?(g_mainUiServerTab?MainUiHubFavorites(rows):g_mainUiHub.recentCount):g_mainUiPanel==6?g_mainUiHub.battleCount:g_mainUiHub.profileCount;int pages=(count+4)/5;g_mainUiHubPage=(g_mainUiHubPage+1)%(pages?pages:1);}
 else if(action>=560&&action<572){int at=action-560+g_mainUiHubPage*5;
  if(g_mainUiPanel==5){const char* address=nullptr;int favorites[kMainUiLayoutMax]{};int n=MainUiHubFavorites(favorites);if(g_mainUiServerTab){if(at<n)address=g_mainUiLayout[favorites[at]].parameter;}else if(at<g_mainUiHub.recentCount)address=g_mainUiHub.recent[at].address;
   if(address&&MainUiLayoutParamValid(900,address)){char cmd[128]{},connect[96]{};MainUiServerConnectParameter(address,connect,sizeof(connect));sprintf_s(cmd,"connect %s",connect);MainUiRestore();EngClientCmdUnrestricted(cmd);}}
  else if(g_mainUiPanel==7&&at<g_mainUiHub.profileCount)MainUiHubJob(MainUiServicesProfile(2,g_mainUiHub.profiles[at]));
 }else return false;
 g_mainUiHubRowsRevision=~0u;++g_mainUiTextRevision;return true;
}
static void MainUiHubPump(){
 MainUiConfigPersistPump();
 const bool on=g_optMainMenuUi&&SkeetoFeaturesOn();const bool show=on&&g_mainUiDraw&&g_mainUiInputReady.load(std::memory_order_relaxed)&&!g_mainUiNative&&!g_mainUiEditing&&!g_mainUiUnderlay;
 MainUiServicesSetWanted(show,show&&g_optMainUiServerQuery&&(g_mainUiPanel==5||(g_mainUiPanel==0&&(g_mainUiFlyout<0||g_mainUiFlyout==12))));
 if(show)MainUiPrefetchReportAvatar();
 static bool importsAllowed=false;bool allowImports=on&&!g_mainUiConnected&&!g_mainUiInGame&&!g_mainUiLoading;
 if(importsAllowed&&!allowImports)MainUiServicesCancelProfiles();importsAllowed=allowImports;
 // Server history consumes copied connection events once, not a 1Hz poll.
 static unsigned consumedServer=~0u;static bool rememberWasOn=false;static DWORD retryServerAt=0;
 if(!on){g_mainUiLastRecordedServer[0]=0;rememberWasOn=false;consumedServer=~0u;retryServerAt=0;return;}
 DWORD now=GetTickCount();
 if(!g_optMainUiRememberServers){rememberWasOn=false;consumedServer=~0u;}
 else {
  if(!rememberWasOn){consumedServer=~0u;retryServerAt=0;rememberWasOn=true;}
  if(!g_mainUiConnected)g_mainUiLastRecordedServer[0]=0;
  else if(g_mainUiInGame&&!g_mainUiLoading){
   unsigned generation=g_mainUiServerGeneration.load(std::memory_order_acquire);
   if(generation!=consumedServer&&(!retryServerAt||(LONG)(now-retryServerAt)>=0)){
    retryServerAt=now+200u;MainUiServerRecord row{};bool copied=false;
    if(TryAcquireSRWLockShared(&g_mainUiServerLock)){row=g_mainUiCurrentServer;generation=g_mainUiServerGeneration.load(std::memory_order_acquire);ReleaseSRWLockShared(&g_mainUiServerLock);copied=true;}
    if(copied){bool handled=true;
     if(row.address[0]&&!ListenAddrIsLoopback(row.address)&&!ListenAddrIsStrictLoopback(row.address)&&MainUiLayoutParamValid(900,row.address)&&_stricmp(row.address,g_mainUiLastRecordedServer)){
      strncpy_s(row.map,g_elimLastMap,_TRUNCATE);handled=MainUiServicesRecent(row);if(handled)strcpy_s(g_mainUiLastRecordedServer,row.address);
     }if(handled){consumedServer=generation;retryServerAt=0;}
    }
   }
  }
 }
 // Retire timing once on disable; re-enabling must not inherit an older map.
 if((!g_optMainMenuUi||!g_optMainUiBattleArchive||!g_optChapStats)&&g_mainUiBattleStart){g_mainUiBattleStart=0;g_mainUiBattleCaptured=false;}
 else if(g_optMainMenuUi&&g_optMainUiBattleArchive&&g_optChapStats&&g_mainUiInGame&&!g_mainUiLoading&&!g_mainUiBattleStart){g_mainUiBattleStart=now;g_mainUiBattleCaptured=false;}
 if(!show)return;
 if(!g_mainUiHubReadAt||(LONG)(now-g_mainUiHubReadAt)>=200){g_mainUiHubReadAt=now;unsigned before=g_mainUiHub.revision;if(MainUiServicesCopy(&g_mainUiHub)&&before!=g_mainUiHub.revision){g_mainUiHubRowsRevision=~0u;if(g_mainUiHub.status==2)g_mainUiNotice=2;else if(g_mainUiHub.status==3)g_mainUiNotice=5;else if(g_mainUiHub.status==4)g_mainUiNotice=3;}}
 char* profile=g_mainUiConfigBody;if(MainUiServicesTakeProfile(profile,kMainUiProfileBytes))g_mainUiNotice=MainUiConfigApply(profile)?4:2;
 if(g_optMainUiServerQuery){if(g_mainUiQueryCollectAt&&(LONG)(now-g_mainUiQueryCollectAt)<0)return;g_mainUiQueryCollectAt=now+200u;char addresses[kMainUiQueryMax][96]{};int n=0;
  auto add=[&](const char* a){if(!MainUiLayoutParamValid(900,a)||n>=kMainUiQueryMax)return;for(int j=0;j<n;++j)if(!_stricmp(addresses[j],a))return;strcpy_s(addresses[n++],a);};
  if(g_mainUiPanel==5){int indices[kMainUiLayoutMax]{},total=MainUiHubFavorites(indices),first=g_mainUiHubPage*5;for(int i=first;i<first+5;++i){if(g_mainUiServerTab){if(i<total)add(g_mainUiLayout[indices[i]].parameter);}else if(i<g_mainUiHub.recentCount)add(g_mainUiHub.recent[i].address);}}
  else if(g_mainUiPanel==0&&g_mainUiFlyout==12){int indices[kMainUiLayoutMax]{},total=MainUiCustomChildren(indices),perPage=MainUiCustomPageSize(),first=g_mainUiCustomPage*perPage;for(int i=first;i<first+perPage&&i<total;++i)if(g_mainUiLayout[indices[i]].action==900)add(g_mainUiLayout[indices[i]].parameter);}
  else if(g_mainUiPanel==0&&g_mainUiFlyout<0){for(int i=0;i<g_mainUiLayoutCount;++i){const auto& b=g_mainUiLayout[i];if(b.visible&&b.parent<0&&b.action==900)add(b.parameter);}}
  if(n!=g_mainUiQueryAddressCount||memcmp(addresses,g_mainUiQueryAddresses,sizeof(addresses))){if(MainUiServicesQuery((const char*)addresses,n)){memcpy(g_mainUiQueryAddresses,addresses,sizeof(addresses));g_mainUiQueryAddressCount=n;}}
 }else {g_mainUiQueryAddressCount=-1;g_mainUiQueryCollectAt=0;}

}
static void MainUiServerPresentation(const MainUiLayoutSlot& b,const wchar_t** name,const wchar_t** details,unsigned* version){
 struct Card{unsigned revision=0,layout=0;bool ready=false,query=false;wchar_t name[192]{},details[192]{};};static Card cards[kMainUiLayoutMax]{};
 uintptr_t offset=(uintptr_t)&b-(uintptr_t)g_mainUiLayout;int index=offset<sizeof(g_mainUiLayout)?(int)(offset/sizeof(b)):-1;
 if(index<0){*name=b.text;*details=L"";*version=0;return;}auto& c=cards[index];
 if(!c.ready||c.revision!=g_mainUiHub.revision||c.layout!=g_mainUiTextRevision||c.query!=g_optMainUiServerQuery){
  const MainUiServerStatus* status=nullptr;for(int j=0;j<g_mainUiHub.queryCount&&j<kMainUiQueryMax;++j)if(!_stricmp(g_mainUiHub.queries[j].address,b.parameter))status=&g_mainUiHub.queries[j];
  wchar_t address[96]{},map[96]{};char connect[96]{};MainUiServerConnectParameter(b.parameter,connect,sizeof(connect));MultiByteToWideChar(CP_UTF8,0,connect,-1,address,96);
  if(status&&status->name[0])MultiByteToWideChar(CP_UTF8,0,status->name,-1,c.name,192);else wcscpy_s(c.name,b.text[0]?b.text:address);
  if(g_optMainUiServerQuery&&status&&status->state==1){MultiByteToWideChar(CP_UTF8,0,status->map,-1,map,96);swprintf_s(c.details,L"%s | %d/%d | %s",address,status->players,status->maxPlayers,map);}
  else swprintf_s(c.details,L"%s | %s",address,UiText(!g_optMainUiServerQuery?UiId::MainUiServerQueryOff:status&&status->state==3?UiId::MainUiQueryIpOnly:UiId::MainUiQueryUnknown,!g_optMainUiServerQuery?L"状态查询关闭":status&&status->state==3?L"查询需有效 IPv4 / IPv6 地址":L"状态未知 / 尚未查询"));
  c.ready=true;c.revision=g_mainUiHub.revision;c.layout=g_mainUiTextRevision;c.query=g_optMainUiServerQuery;
 }
 *name=c.name;*details=c.details;*version=g_mainUiHub.revision+1;
}

static void MainUiHubDraw(bool registration){
 float s=(g_mainUiSw-48.f)/1040.f;if((g_mainUiSh-48.f)/600.f<s)s=(g_mainUiSh-48.f)/600.f;auto U=[=](int v){return (int)(v*s+.5f);};int ox=(g_mainUiSw-U(1040))/2,oy=(g_mainUiSh-U(600))/2,x=ox+U(312);
 auto button=[&](int px,int py,int width,int id,const wchar_t* label){MainUiLayoutSlot b=MainUiLayoutBlank();b.id=id;b.font=id>=540&&id<=549?0:1;if(id==540||id==541||id==543)b.color=(id==540?g_optMainUiRememberServers:id==541?g_optMainUiServerQuery:g_optMainUiBattleArchive)?2:4;int mx=0,my=0;int hover=!registration&&!g_mainUiUnderlay&&MainUiMouse(&mx,&my)?MainUiHit(mx,my):-1;MainUiDrawSlot(b,px,py,width,U(34),registration,hover,-1,-1,label);};
 auto text=[&](int px,int py,const wchar_t* label,int width,COLORREF c=RGB(180,188,197)){if(!registration)MainUiText(g_mainUiFonts[2],px,py,c,label,width,g_mainUiHubRowsRevision);};
 if(g_mainUiPanel==8){static const int actions[]={209,210,211,212,213,214};for(int i=0;i<6;++i)button(x+U(28),oy+U(211+i*44),U(672),actions[i],MainUiActionTitle(actions[i]));return;}

 int rows[kMainUiLayoutMax]{},count=0;int first=g_mainUiHubPage*5;
 if(g_mainUiPanel==5){
  button(x+U(28),oy+U(201),U(216),540,UiText(g_optMainUiRememberServers?UiId::MainUiRecentOn:UiId::MainUiRecentOff,g_optMainUiRememberServers?L"记录服务器：开":L"记录服务器：关"));
  button(x+U(256),oy+U(201),U(216),541,UiText(g_optMainUiServerQuery?UiId::MainUiQueryOn:UiId::MainUiQueryOff,g_optMainUiServerQuery?L"状态查询：开":L"状态查询：关"));
  button(x+U(484),oy+U(201),U(216),542,UiText(g_mainUiServerTab?UiId::MainUiFavoritesTab:UiId::MainUiRecentTab,g_mainUiServerTab?L"收藏服务器":L"服务器游玩记录"));count=g_mainUiServerTab?MainUiHubFavorites(rows):g_mainUiHub.recentCount;
 }else if(g_mainUiPanel==6){button(x+U(28),oy+U(201),U(326),543,UiText(g_optMainUiBattleArchive?UiId::MainUiBattleOn:UiId::MainUiBattleOff,g_optMainUiBattleArchive?L"记录战报：开":L"记录战报：关"));button(x+U(374),oy+U(201),U(326),544,UiText(UiId::MainUiOpenReport,L"浏览器查看完整战报"));count=g_mainUiHub.battleCount;
 }else if(g_mainUiPanel==7){button(x+U(28),oy+U(201),U(160),545,UiText(UiId::MainUiProfileSlot,L"切换保存槽"));button(x+U(198),oy+U(201),U(160),546,UiText(UiId::MainUiSaveProfile,L"保存当前方案"));button(x+U(368),oy+U(201),U(160),547,UiText(UiId::MainUiExportProfile,L"导出 / 分享"));button(x+U(538),oy+U(201),U(162),548,UiText(UiId::MainUiImportProfile,L"导入方案"));count=g_mainUiHub.profileCount;}
 int pages=(count+4)/5;if(pages<1)pages=1;if(g_mainUiHubPage>=pages){g_mainUiHubPage=0;first=0;g_mainUiHubRowsRevision=~0u;}
 // Format only on data/tab/page changes. Stable addresses feed the shared fit cache.
 if(g_mainUiHubRowsRevision!=g_mainUiHub.revision){
  memset(g_mainUiHubRows,0,sizeof(g_mainUiHubRows));memset(g_mainUiHubDetails,0,sizeof(g_mainUiHubDetails));
  for(int i=0;i<5&&first+i<count;++i){int at=first+i;
   if(g_mainUiPanel==5){const char* address=g_mainUiServerTab?g_mainUiLayout[rows[at]].parameter:g_mainUiHub.recent[at].address;const char* name=g_mainUiServerTab?g_mainUiLayout[rows[at]].title:g_mainUiHub.recent[at].name;MainUiServerStatus* status=nullptr;for(int j=0;j<g_mainUiHub.queryCount;++j)if(!_stricmp(g_mainUiHub.queries[j].address,address))status=&g_mainUiHub.queries[j];
    if(status&&status->name[0])name=status->name;MultiByteToWideChar(CP_UTF8,0,name[0]?name:address,-1,g_mainUiHubRows[i],192);wchar_t addr[96]{},map[96]{};char connect[96]{};MainUiServerConnectParameter(address,connect,sizeof(connect));MultiByteToWideChar(CP_UTF8,0,connect,-1,addr,96);
    if(status&&status->state==1){MultiByteToWideChar(CP_UTF8,0,status->map,-1,map,96);swprintf_s(g_mainUiHubDetails[i],L"%s | %d/%d | %s | %d ms",addr,status->players,status->maxPlayers,map,status->ping);}
    else swprintf_s(g_mainUiHubDetails[i],L"%s | %s",addr,UiText(status&&status->state==3?UiId::MainUiQueryIpOnly:UiId::MainUiQueryUnknown,status&&status->state==3?L"查询需有效 IPv4 / IPv6 地址":L"状态未知 / 尚未查询"));
   }else if(g_mainUiPanel==6){const auto& b=g_mainUiHub.battles[at];wchar_t map[96]{},date[32]{},score[48]{};MultiByteToWideChar(CP_UTF8,0,b.map,-1,map,96);time_t stamp=(time_t)b.time;tm when{};if(!localtime_s(&when,&stamp))wcsftime(date,_countof(date),L"%m-%d %H:%M",&when);swprintf_s(g_mainUiHubRows[i],L"%s | %s",date,map);if(b.scoreEnabled)swprintf_s(score,UiText(UiId::MainUiRecordedScore,L"评分 %d"),b.score/100);else wcscpy_s(score,UiText(UiId::MainUiScoreDisabled,L"评分关闭"));int si=0;for(int k=1;k<=6;++k)si+=b.stats[k];swprintf_s(g_mainUiHubDetails[i],L"%d:%02d | %s | CI %d / SI %d | %s",b.seconds/60,b.seconds%60,UiText(b.result==0?UiId::MainUiSettled:b.result==1?UiId::MainUiFailed:UiId::MainUiIncomplete,b.result==0?L"章节结算":b.result==1?L"失败":L"未完成"),b.stats[0],si,score);
   }else MultiByteToWideChar(CP_UTF8,0,g_mainUiHub.profiles[at],-1,g_mainUiHubRows[i],192);
  }g_mainUiHubRowsRevision=g_mainUiHub.revision;++g_mainUiTextRevision;
 }
 for(int i=0;i<5&&first+i<count;++i){int y=oy+U(244+i*52);
  if(g_mainUiPanel==5){MainUiLayoutSlot b=MainUiLayoutBlank();b.id=560+i;int mx=0,my=0,hover=!registration&&!g_mainUiUnderlay&&MainUiMouse(&mx,&my)?MainUiHit(mx,my):-1;
   const int h=U(48);MainUiDrawSlot(b,x+U(28),y,U(672),h,registration,hover,-1,-1,L"");
   if(!registration){int nf=1;if(g_mainUiFontPx[nf]+g_mainUiFontPx[2]+4>h)nf=2;
    if(g_mainUiFontPx[nf]+g_mainUiFontPx[2]+4<=h){const int top=y+(h-g_mainUiFontPx[nf]-g_mainUiFontPx[2]-2)/2;
     MainUiText(g_mainUiFonts[nf],x+U(38),top,hover==b.id?MainUiAccentColor():RGB(244,245,246),g_mainUiHubRows[i],U(652),g_mainUiHubRowsRevision);
     MainUiText(g_mainUiFonts[2],x+U(38),top+g_mainUiFontPx[nf]+2,RGB(180,188,197),g_mainUiHubDetails[i],U(652),g_mainUiHubRowsRevision);
    }else MainUiText(g_mainUiFonts[2],x+U(38),y+(h-g_mainUiFontPx[2])/2,RGB(244,245,246),g_mainUiHubRows[i],U(652),g_mainUiHubRowsRevision);}}
  else{if(g_mainUiPanel==6)text(x+U(28),y,g_mainUiHubRows[i],U(672),RGB(244,245,246));else button(x+U(28),y,U(672),560+i,g_mainUiHubRows[i]);text(x+U(38),y+U(29),g_mainUiHubDetails[i],U(652));}}
 if(!count)text(x+U(28),oy+U(272),g_mainUiPanel==7?UiText(UiId::MainUiProfilesEmpty,L"暂无方案。保存当前主菜单，或导入主菜单方案。") : g_mainUiPanel==6?UiText(UiId::MainUiBattlesEmpty,L"暂无成功通关的战报。开启记录和章节统计后通关一章再查看。") : UiText(UiId::MainUiHubEmpty,L"暂无记录。开启记录后游玩；收藏服务器可在编辑器中添加。"),U(672));
 if(g_mainUiPanel==5)button(x+U(486),oy+U(510),U(214),553,UiText(UiId::MainUiAddServer,L"添加服务器"));
 button(x+U(28),oy+U(510),U(160),549,UiText(UiId::MainUiHubNext,L"下一页"));
 wchar_t page[96]{};swprintf_s(page,L"%d / %d%s%d",g_mainUiHubPage+1,pages,g_mainUiPanel==7?L" | Slot ":L" | ",g_mainUiPanel==7?g_mainUiSaveSlot+1:count);if(!registration)SurfTextAt(g_mainUiFonts[2],x+U(208),oy+U(518),RGB(180,188,197),page);
 text(x+U(28),oy+U(560)-g_mainUiFontPx[2]-U(10),g_mainUiPanel==6?UiText(UiId::MainUiBattleHint,L"需开启章节统计；只记录开启后成功通关的章节与小队数据。") : g_mainUiPanel==7?UiText(UiId::MainUiProfileHint,L"只切换主菜单按钮、子菜单与外观；不修改游戏内 HUD。") : UiText(UiId::MainUiServerHint,L"点击条目重连；地址后可追加 @查询端口；超时不代表离线。"),U(672));
}
