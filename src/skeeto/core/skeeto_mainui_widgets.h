// Main-menu-only text and Steam friends components. Paint owns all state.
#pragma once
static COLORREF MainUiSlotColor(int);

static const wchar_t* MainUiElementText(int action) {
 static wchar_t version[32]{};if(!version[0])swprintf_s(version,L"v%s",SKEETO_VERSION_W);
 switch(action){
 case 930:return UiText(UiId::MainUiWelcome,L"Welcome to Skeeto");
 case 931:return UiText(UiId::MainUiWelcomeSub,L"Make L4D2 yours.");
 case 932:return L"SKEETO";case 933:return version;case 934:return L"Beyond Vanilla.";
 case 935:return UiText(UiId::MainUiNavigationHint,L"鼠标 / ↑↓ / Tab / Enter    ←→ 调节滑块    Esc 返回；快捷键在主菜单设置中自定义。");
 case 936:return UiText(UiId::MainUiElementNotice,L"状态提示");
 case 937:return UiText(UiId::MainUiElementFriends,L"正在玩 L4D2 的好友");
 }return L"";
}
static bool MainUiElementVisible(int action){
 if(action==930||action==931)return g_mainUiHomeDecor&&g_mainUiFlyout<0;
 if(action==932)return g_mainUiShowLogo;if(action==933)return g_mainUiShowVersion;
 if(action==934)return g_mainUiShowGameName;if(action==935)return g_mainUiShowHints;
 if(action==936)return g_mainUiNotice!=0;return g_mainUiFlyout<0;
}
// Fixed two-pixel buckets, at most 91 handles; never allocate by frame/element.
static unsigned long MainUiElementFont(int px){
 static unsigned long fonts[91]{};static bool tried[91]{},valid[91]{};static int epoch=0;
 if(px<12)px=12;if(px>192)px=192;px=(px+1)&~1;int slot=(px-12)/2;
 if(epoch!=g_sktFontEpoch){memset(tried,0,sizeof(tried));memset(valid,0,sizeof(valid));epoch=g_sktFontEpoch;}
 if(!tried[slot]){tried[slot]=true;
  if(!fonts[slot]){fonts[slot]=SurfMakeGameFontRaw(px,500,kFontFlagAntiAlias|kFontFlagDropShadow);valid[slot]=fonts[slot]!=0;}
  else {using Fn=bool(__thiscall*)(void*,unsigned long,const char*,int,int,int,int,int,int,int);auto glyph=(Fn)VGet(g_surf,kVmtSurfSetFontGlyphSet);
   if(glyph&&IsExec((void*)glyph)){__try{valid[slot]=SurfApplyGlyphFace(g_surf,glyph,fonts[slot],px,500,kFontFlagAntiAlias|kFontFlagDropShadow);}__except(EXCEPTION_EXECUTE_HANDLER){valid[slot]=false;}}
  }
 }return valid[slot]?fonts[slot]:g_mainUiFonts[2];
}
static int MainUiElementPixels(const MainUiLayoutSlot& b,float scale){int base=b.font>=1008&&b.font<=1096?b.font-1000:b.font==0?14:b.font==2?30:22;return (int)(base*(g_mainUiTextAutoScale?scale:1.f)+.5f);}
static void MainUiDrawElement(const MainUiLayoutSlot& b,int x,int y,int w,int h){
 float scale=(g_mainUiSw-48.f)/1040.f;float v=(g_mainUiSh-48.f)/600.f;if(v<scale)scale=v;
 int px=MainUiElementPixels(b,scale);if(px>h)px=h;unsigned long font=MainUiElementFont(px);
 const wchar_t* text=b.paramText[0]?b.paramText:b.text[0]?b.text:MainUiElementText(b.action);
 if(b.action==936&&!b.paramText[0]&&!b.text[0])text=g_mainUiNotice==5?UiText(UiId::MainUiHubIoError,L"文件读取、写入或打开失败；请检查路径、文件与权限。"):g_mainUiNotice==3?UiText(UiId::MainUiHubBusy,L"任务队列忙；请稍后重试。"):g_mainUiNotice==4?UiText(UiId::MainUiProfileApplied,L"主菜单方案已应用。"):g_mainUiNotice==2?UiText(UiId::MainUiEditorInvalid,L"内容无效或超长；请检查动作参数或主菜单方案格式。"):UiText(UiId::MainUiNativeUnavailable,L"原生入口未通过兼容核验；可从原版主菜单入口操作。");
 MainUiText(font,x,y,MainUiSlotColor(b.color),text,w);
}

struct MainUiFriendGame{uint64_t game;uint32_t ip;uint16_t port,query;uint64_t lobby;};
static_assert(sizeof(MainUiFriendGame)==24,"Steam x86 FriendGameInfo layout");
struct MainUiFriendRow{uint64_t sid=0;MainUiFriendGame game{};wchar_t name[64]{};};
struct MainUiFriendsApi{
 void*(__cdecl* friends)()=nullptr;void*(__cdecl* utils)()=nullptr;
 int(__cdecl* count)(void*,int)=nullptr;uint64_t(__cdecl* index)(void*,int,int)=nullptr;
 const char*(__cdecl* name)(void*,uint64_t)=nullptr;bool(__cdecl* game)(void*,uint64_t,MainUiFriendGame*)=nullptr;
 int(__cdecl* avatar)(void*,uint64_t)=nullptr;int(__cdecl* avatarMedium)(void*,uint64_t)=nullptr;
 int(__cdecl* avatarLarge)(void*,uint64_t)=nullptr;bool(__cdecl* size)(void*,int,uint32_t*,uint32_t*)=nullptr;
 bool(__cdecl* rgba)(void*,int,unsigned char*,int)=nullptr;
};
static MainUiFriendsApi g_mainUiFriendsApi{};
static MainUiFriendRow g_mainUiFriends[16]{},g_mainUiFriendScan[16]{};
static int g_mainUiFriendCount=0,g_mainUiFriendTotal=0,g_mainUiFriendCursor=0,g_mainUiFriendLimit=0,g_mainUiFriendFound=0;
static DWORD g_mainUiFriendNext=0,g_mainUiFriendSlice=0;static bool g_mainUiFriendScanning=false,g_mainUiFriendsActive=false,g_mainUiFriendsAvailable=false;
static unsigned g_mainUiFriendsRevision=1;
static void MainUiFriendsStop(){if(!g_mainUiFriendsActive)return;g_mainUiFriendsActive=false;g_mainUiFriendScanning=false;g_mainUiFriendCount=g_mainUiFriendTotal=0;g_mainUiFriendNext=g_mainUiFriendSlice=0;++g_mainUiFriendsRevision;}
static bool MainUiFriendsBind(){
 if(g_mainUiFriendsApi.friends)return true;HMODULE api=GetModuleHandleW(L"steam_api.dll");if(!api)return false;
 auto& f=g_mainUiFriendsApi;
#define MAINUI_STEAM_BIND(member,exportName) f.member=(decltype(f.member))GetProcAddress(api,exportName)
 MAINUI_STEAM_BIND(friends,"SteamAPI_SteamFriends_v017");MAINUI_STEAM_BIND(utils,"SteamAPI_SteamUtils_v010");
 MAINUI_STEAM_BIND(count,"SteamAPI_ISteamFriends_GetFriendCount");MAINUI_STEAM_BIND(index,"SteamAPI_ISteamFriends_GetFriendByIndex");
 MAINUI_STEAM_BIND(name,"SteamAPI_ISteamFriends_GetFriendPersonaName");MAINUI_STEAM_BIND(game,"SteamAPI_ISteamFriends_GetFriendGamePlayed");
 MAINUI_STEAM_BIND(avatar,"SteamAPI_ISteamFriends_GetSmallFriendAvatar");
 MAINUI_STEAM_BIND(avatarMedium,"SteamAPI_ISteamFriends_GetMediumFriendAvatar");
 MAINUI_STEAM_BIND(avatarLarge,"SteamAPI_ISteamFriends_GetLargeFriendAvatar");
 MAINUI_STEAM_BIND(size,"SteamAPI_ISteamUtils_GetImageSize");
 MAINUI_STEAM_BIND(rgba,"SteamAPI_ISteamUtils_GetImageRGBA");
#undef MAINUI_STEAM_BIND
 if(!f.friends||!f.count||!f.index||!f.name||!f.game){f=MainUiFriendsApi{};return false;}return true;
}
static void MainUiFriendsSample(){
 if(g_mainUiUnderlay||!g_mainUiInputReady||(g_mainUiPressed>=10000&&g_mainUiPressed<50000))return;DWORD now=GetTickCount();g_mainUiFriendsActive=true;
 if((LONG)(now-g_mainUiFriendNext)<0&&!g_mainUiFriendScanning)return;
 if(g_mainUiFriendSlice&&(DWORD)(now-g_mainUiFriendSlice)<50u)return;g_mainUiFriendSlice=now;
 if(!MainUiFriendsBind()){g_mainUiFriendsAvailable=false;g_mainUiFriendNext=now+30000u;return;}
 auto& f=g_mainUiFriendsApi;
 __try{
  void* fr=f.friends();if(!fr){g_mainUiFriendsAvailable=false;g_mainUiFriendScanning=false;g_mainUiFriendNext=now+5000u;return;}
  if(!g_mainUiFriendScanning){int n=f.count(fr,4);g_mainUiFriendsAvailable=n>=0;if(n<0){g_mainUiFriendNext=now+5000u;return;}g_mainUiFriendLimit=n>4096?4096:n;g_mainUiFriendCursor=g_mainUiFriendFound=0;g_mainUiFriendScanning=true;memset(g_mainUiFriendScan,0,sizeof(g_mainUiFriendScan));}
  for(int budget=0;budget<16&&g_mainUiFriendCursor<g_mainUiFriendLimit;++budget){uint64_t sid=f.index(fr,g_mainUiFriendCursor++,4);MainUiFriendGame game{};
   if(!sid||!f.game(fr,sid,&game)||(game.game&0xffffffu)!=550u)continue;int row=g_mainUiFriendFound++;
   if(row<16){auto& r=g_mainUiFriendScan[row];r.sid=sid;r.game=game;const char* name=f.name(fr,sid);if(name)MultiByteToWideChar(CP_UTF8,0,name,-1,r.name,64);r.name[63]=0;for(wchar_t* p=r.name;*p;++p)if(*p<32||*p==127)*p=L' ';if(!r.name[0])wcscpy_s(r.name,UiText(UiId::MainUiFriendUnknown,L"Steam 好友"));}
  }
  if(g_mainUiFriendCursor>=g_mainUiFriendLimit){g_mainUiFriendScanning=false;g_mainUiFriendTotal=g_mainUiFriendFound;g_mainUiFriendCount=g_mainUiFriendFound<16?g_mainUiFriendFound:16;memcpy(g_mainUiFriends,g_mainUiFriendScan,sizeof(g_mainUiFriends));g_mainUiFriendNext=now+5000u;++g_mainUiFriendsRevision;}
 }__except(EXCEPTION_EXECUTE_HANDLER){g_mainUiFriendsAvailable=false;g_mainUiFriendScanning=false;g_mainUiFriendCount=0;g_mainUiFriendNext=now+30000u;}
}
static int MainUiFriendAvatar(int row){
 struct Portrait{uint64_t sid=0;int texture=0,epoch=0;DWORD next=0;bool ready=false;};static Portrait portraits[16]{};static DWORD lastFetch=0;static unsigned char pixels[64*64*4]{};
 if(row<0||row>=g_mainUiFriendCount)return -1;auto& p=portraits[row];auto& f=g_mainUiFriendsApi;uint64_t sid=g_mainUiFriends[row].sid;DWORD now=GetTickCount();
 if(p.sid!=sid){p.sid=sid;p.ready=false;p.next=0;}if(p.epoch!=g_sktFontEpoch){p.ready=false;p.next=0;}
 if(p.ready)return p.texture-1;if(g_mainUiUnderlay||(LONG)(now-p.next)<0||(lastFetch&&(DWORD)(now-lastFetch)<100u)||!f.utils||!f.avatar||!f.size||!f.rgba)return -1;
 lastFetch=now;p.next=now+2000u;
 __try{void* fr=f.friends();void* ut=f.utils();if(!fr||!ut)return -1;int image=f.avatar(fr,sid);uint32_t w=0,h=0;if(image<=0||!f.size(ut,image,&w,&h)||!w||!h||w>64||h>64||!f.rgba(ut,image,pixels,(int)(w*h*4)))return -1;
  if(!p.texture){using Create=int(__thiscall*)(void*,bool);auto create=(Create)VGet(g_surf,kVmtSurfCreateNewTextureID);if(!create||!IsExec((void*)create))return -1;int id=create(g_surf,true);if(id<0)return -1;p.texture=id+1;}
  SurfSetTextureRGBA(p.texture-1,pixels,(int)w,(int)h,1);p.epoch=g_sktFontEpoch;p.ready=true;return p.texture-1;
 }__except(EXCEPTION_EXECUTE_HANDLER){p.next=now+30000u;return -1;}
}

static int g_mainUiFriendPages[600+kMainUiLayoutMax]{};
static void MainUiFriendsNext(int id){if(id>=0&&id<600+kMainUiLayoutMax)g_mainUiFriendPages[id]=(g_mainUiFriendPages[id]+1)%16;}
static bool MainUiFriendHitAllowed(int action){int id=(action-10000)/32,row=(action-10000)%32;if(g_mainUiPanel!=0||g_mainUiFlyout>=0||(row>=g_mainUiFriendCount&&row!=30&&row!=31))return false;for(int i=0;i<g_mainUiLayoutCount;++i){const auto& b=g_mainUiLayout[i];if(b.id==id&&b.action==937&&b.visible&&b.parent<0)return true;}return false;}
static void MainUiDrawFriends(const MainUiLayoutSlot& b,int x,int y,int w,int h,bool registration){
 if(w<1||h<12)return;
 if(!registration)MainUiFriendsSample();float scale=(g_mainUiSw-48.f)/1040.f;float vertical=(g_mainUiSh-48.f)/600.f;if(vertical<scale)scale=vertical;int fontPx=MainUiElementPixels(b,scale);if(fontPx>h)fontPx=h;if(fontPx<12)fontPx=12;if(fontPx>192)fontPx=192;fontPx=(fontPx+1)&~1;int rowH=fontPx+8,titleH=fontPx+6;if(titleH>h)titleH=h;unsigned long font=registration?0:MainUiElementFont(fontPx);
 int rows=(h-titleH)/rowH;if(rows<0)rows=0;if(rows>6)rows=6;if(rows>g_mainUiFriendCount)rows=g_mainUiFriendCount;g_mainUiFriendVisibleRows=rows;int arrowW=fontPx+6;if(arrowW>w)arrowW=w;int pages=rows?(g_mainUiFriendCount+rows-1)/rows:1;if(pages<1)pages=1;int page=g_mainUiFriendPages[b.id]%pages,first=page*rows;
 if(!registration){static wchar_t title[128]{};_snwprintf_s(title,_countof(title),_TRUNCATE,L"%s: %d",b.paramText[0]?b.paramText:b.text[0]?b.text:MainUiElementText(937),g_mainUiFriendTotal);MainUiText(font,x,y,MainUiSlotColor(b.color),title,w-(pages>1?arrowW:0),g_mainUiFriendsRevision+g_mainUiTextRevision);
  if(!rows&&h>=titleH+fontPx)MainUiText(font,x,y+titleH,RGB(180,188,197),UiText(g_mainUiFriendsAvailable?UiId::MainUiFriendsEmpty:UiId::MainUiFriendsUnavailable,g_mainUiFriendsAvailable?L"暂无好友正在玩 L4D2":L"Steam 好友状态暂不可用"),w);}
 if(registration&&g_mainUiButtonCount<kMainUiHitMax&&w>(pages>1?arrowW:0))g_mainUiButtons[g_mainUiButtonCount++]={x,y,x+w-(pages>1?arrowW:0),y+titleH,10000+b.id*32+30};
 if(pages>1){int action=10000+b.id*32+31;if(registration){if(g_mainUiButtonCount<kMainUiHitMax)g_mainUiButtons[g_mainUiButtonCount++]={x+w-arrowW,y,x+w,y+titleH,action};}else MainUiText(font,x+w-arrowW,y,MainUiSlotColor(b.color),L">",arrowW);}
 for(int i=0;i<rows&&first+i<g_mainUiFriendCount;++i){int at=first+i;int top=y+titleH+i*rowH;if(registration){if(g_mainUiButtonCount<kMainUiHitMax)g_mainUiButtons[g_mainUiButtonCount++]={x,top,x+w,top+rowH,10000+b.id*32+at};continue;}
  int texture=MainUiFriendAvatar(at),avatar=rowH-4;if(avatar>w/3)avatar=w/3;if(texture>=0){SurfColor(255,255,255,255);SurfDrawTextured(x,top,x+avatar,top+avatar,texture);}else{SurfColor(75,85,98,180);SurfFill(x,top,x+avatar,top+avatar);}
  MainUiText(font,x+avatar+4,top+2,g_mainUiFocus==10000+b.id*32+at?MainUiSlotColor(b.color):RGB(225,235,239),g_mainUiFriends[at].name,w-avatar-4,g_mainUiFriendsRevision);
 }
}

// Paint-only. Ask Steam for the local large avatar ahead of the report so the
// first HTML open is not stuck on the 32px image. At most one call per 2s.
static void MainUiPrefetchReportAvatar() {
 static DWORD next=0;static bool ready=false;
 if(ready)return;
 const DWORD now=GetTickCount();
 if(next&&(LONG)(now-next)<0)return;
 next=now+2000u;
 if(!MainUiFriendsBind())return;
 HMODULE api=GetModuleHandleW(L"steam_api.dll");if(!api)return;
 using SteamUserFn=void*(__cdecl*)();
 SteamUserFn steamUser=(SteamUserFn)GetProcAddress(api,"SteamAPI_SteamUser_v021");
 if(!steamUser)steamUser=(SteamUserFn)GetProcAddress(api,"SteamAPI_SteamUser_v020");
 if(!steamUser)steamUser=(SteamUserFn)GetProcAddress(api,"SteamAPI_SteamUser_v019");
 auto getSteamId=(uint64_t(__cdecl*)(void*))GetProcAddress(api,"SteamAPI_ISteamUser_GetSteamID");
 auto& f=g_mainUiFriendsApi;
 if(!steamUser||!getSteamId||!f.avatarLarge){ready=true;return;}
 __try{
  void* user=steamUser();void* fr=f.friends();
  if(!user||!fr)return;
  const uint64_t sid=getSteamId(user);
  if(sid&&f.avatarLarge(fr,sid)>0)ready=true;
 }__except(EXCEPTION_EXECUTE_HANDLER){}
}

// Paint-only: copy local Steam persona / SteamID64 / BMP data-URI into the
// career report payload before the worker writes HTML (worker never calls Steam).
static void MainUiFillReportIdentity(MainUiCareerReport* out) {
 if(!out)return;
 out->personaName[0]=0;out->steamId[0]=0;g_mainUiAvatarUri[0]=0;
 HMODULE api=GetModuleHandleW(L"steam_api.dll");if(!api)return;
 using SteamUserFn=void*(__cdecl*)();
 SteamUserFn steamUser=(SteamUserFn)GetProcAddress(api,"SteamAPI_SteamUser_v021");
 if(!steamUser)steamUser=(SteamUserFn)GetProcAddress(api,"SteamAPI_SteamUser_v020");
 if(!steamUser)steamUser=(SteamUserFn)GetProcAddress(api,"SteamAPI_SteamUser_v019");
 auto getSteamId=(uint64_t(__cdecl*)(void*))GetProcAddress(api,"SteamAPI_ISteamUser_GetSteamID");
 if(!MainUiFriendsBind()||!steamUser||!getSteamId)return;
 auto& f=g_mainUiFriendsApi;
 __try{
  void* user=steamUser();void* fr=f.friends();void* ut=f.utils();
  if(!user||!fr)return;
  const uint64_t sid=getSteamId(user);if(!sid)return;
  snprintf(out->steamId,sizeof(out->steamId),"%llu",(unsigned long long)sid);
  const char* name=f.name(fr,sid);
  if(name&&name[0]){strncpy_s(out->personaName,name,_TRUNCATE);for(char* p=out->personaName;*p;++p)if((unsigned char)*p<32||*p==127)*p=' ';}
  if(!ut||!f.size||!f.rgba)return;
  int image=0;
  if(f.avatarLarge)image=f.avatarLarge(fr,sid);
  if(image<=0&&f.avatarMedium)image=f.avatarMedium(fr,sid);
  if(image<=0&&f.avatar)image=f.avatar(fr,sid);
  uint32_t w=0,h=0;
  constexpr int kAvatarPx=184;
  static unsigned char pixels[kAvatarPx*kAvatarPx*4];
  static unsigned char bmp[14+40+kAvatarPx*kAvatarPx*4];
  if(image<=0||!f.size(ut,image,&w,&h)||!w||!h||w>kAvatarPx||h>kAvatarPx||!f.rgba(ut,image,pixels,(int)(w*h*4)))return;
  // Uncompressed 32-bpp BMP. Bottom-up BGRA rows, already 4-byte aligned.
  const int rowBytes=(int)(w*4);const int pixelBytes=rowBytes*(int)h;
  const int fileBytes=14+40+pixelBytes;
  if(fileBytes<=0||fileBytes>(int)sizeof(bmp))return;
  bmp[0]='B';bmp[1]='M';
  bmp[2]=(unsigned char)fileBytes;bmp[3]=(unsigned char)(fileBytes>>8);bmp[4]=(unsigned char)(fileBytes>>16);bmp[5]=(unsigned char)(fileBytes>>24);
  bmp[10]=54;
  bmp[14]=40;bmp[18]=(unsigned char)w;bmp[19]=(unsigned char)(w>>8);bmp[20]=(unsigned char)(w>>16);bmp[21]=(unsigned char)(w>>24);
  bmp[22]=(unsigned char)h;bmp[23]=(unsigned char)(h>>8);bmp[24]=(unsigned char)(h>>16);bmp[25]=(unsigned char)(h>>24);
  bmp[26]=1;bmp[28]=32;
  for(uint32_t y=0;y<h;++y){
   const unsigned char* src=pixels+((h-1-y)*w*4);
   unsigned char* dst=bmp+54+(int)y*rowBytes;
   for(uint32_t x=0;x<w;++x){dst[x*4+0]=src[x*4+2];dst[x*4+1]=src[x*4+1];dst[x*4+2]=src[x*4+0];dst[x*4+3]=src[x*4+3];}
  }
  static const char kB64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char* d=g_mainUiAvatarUri;const char* prefix="data:image/bmp;base64,";
  const size_t prefixN=strlen(prefix);if(prefixN>=kMainUiAvatarUriChars)return;
  memcpy(d,prefix,prefixN);d+=prefixN;size_t left=kMainUiAvatarUriChars-prefixN-1;
  for(int i=0;i<fileBytes;i+=3){
   const unsigned a=bmp[i],b=(i+1<fileBytes)?bmp[i+1]:0,c=(i+2<fileBytes)?bmp[i+2]:0;
   const unsigned n=(a<<16)|(b<<8)|c;char enc[5]{kB64[(n>>18)&63],kB64[(n>>12)&63],(i+1<fileBytes)?kB64[(n>>6)&63]:'=',(i+2<fileBytes)?kB64[n&63]:'=',0};
   if(left<4){g_mainUiAvatarUri[0]=0;return;}memcpy(d,enc,4);d+=4;left-=4;
  }
  *d=0;
 }__except(EXCEPTION_EXECUTE_HANDLER){out->personaName[0]=0;out->steamId[0]=0;g_mainUiAvatarUri[0]=0;}
}
