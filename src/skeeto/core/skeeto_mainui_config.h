// Main-menu UI persistence only. Startup worker loads before readiness; interactive imports apply on Paint.
// Fixed buffers and copied worker jobs; no game-HUD settings or engine calls.
#pragma once
static constexpr const char* kMainUiConfigHeader="SKEETO_MAINMENU_UI=1\n";
// One serial Paint-owned scratch area; consumers copy before returning. Startup
// uses it before the early Paint bridge is installed and never overlaps Paint.
static char g_mainUiConfigBody[kMainUiProfileBytes]{};
struct MainUiConfigValues {int enabled,accent,backdrop,mode,difficulty,recent,query,battles;int backdropFollow=0,backdropRgb=0;int limit=256;int hints=1,version=1,gameName=1,textAutoScale=1,logo=1,accentLine=1,fixedShortcuts=1;};
static MainUiConfigValues MainUiConfigValuesRead(){return {g_optMainMenuUi?1:0,g_mainUiAccent,g_mainUiBackdrop,g_mainUiQuickMode,g_mainUiQuickDifficulty,g_optMainUiRememberServers?1:0,g_optMainUiServerQuery?1:0,g_optMainUiBattleArchive?1:0,g_mainUiBackdropFollow?1:0,g_mainUiBackdropRgb,g_mainUiButtonLimit,g_mainUiShowHints,g_mainUiShowVersion,g_mainUiShowGameName,g_mainUiTextAutoScale,g_mainUiShowLogo,g_mainUiShowAccentLine,g_mainUiFixedShortcuts?1:0};}
static bool MainUiConfigCapture(char* body){
 size_t used=0;auto append=[&](const char* format,auto... args){int n=snprintf(body+used,kMainUiProfileBytes-used,format,args...);if(n<0||(size_t)n>=kMainUiProfileBytes-used)return false;used+=n;return true;};
 const auto v=MainUiConfigValuesRead();
 return append("%s",kMainUiConfigHeader)&&append("enabled=%d\naccent=%d\nbackdrop=%d\nmode=%d\ndifficulty=%d\nrecord_servers=%d\nquery_servers=%d\nrecord_battles=%d\nbutton_limit=%d\n",v.enabled,v.accent,v.backdrop,v.mode,v.difficulty,v.recent,v.query,v.battles,v.limit)&&append("show_hints=%d\nshow_version=%d\nshow_game_name=%d\ntext_auto_scale=%d\nshow_logo=%d\nshow_accent_line=%d\n",v.hints,v.version,v.gameName,v.textAutoScale,v.logo,v.accentLine)&&append("backdrop_follow=%d\nbackdrop_rgb=%d\nfixed_shortcuts=%d\n",v.backdropFollow,v.backdropRgb,v.fixedShortcuts)&&MainUiLayoutSerialize(append);
}
static bool MainUiConfigApply(char* body,bool startup=false){
 const size_t header=strlen(kMainUiConfigHeader);
 if(strncmp(body,kMainUiConfigHeader,header))return false;
 // The existing bounded layout validator works on Paint-owned globals. Restore
 // every staging field before returning on failure; never dispatch any actions.
 // Serialized on startup or Paint; no reentrant callbacks during validation.
 static MainUiLayoutSlot prior[kMainUiLayoutMax]{};memcpy(prior,g_mainUiLayout,sizeof(prior));
 const int count=g_mainUiLayoutCount,version=g_mainUiLayoutVersion;const bool decor=g_mainUiHomeDecor,read=g_mainUiLayoutRead,bad=g_mainUiLayoutBad;
 bool priorSeen[kMainUiLayoutMax];memcpy(priorSeen,g_mainUiLayoutSeen,sizeof(priorSeen));
 auto restore=[&](){memcpy(g_mainUiLayout,prior,sizeof(prior));g_mainUiLayoutCount=count;g_mainUiHomeDecor=decor;g_mainUiLayoutRead=read;g_mainUiLayoutBad=bad;g_mainUiLayoutVersion=version;memcpy(g_mainUiLayoutSeen,priorSeen,sizeof(priorSeen));};
 MainUiLayoutReset();MainUiConfigValues v{};int* values[]={&v.enabled,&v.accent,&v.backdrop,&v.mode,&v.difficulty,&v.recent,&v.query,&v.battles,&v.limit,&v.hints,&v.version,&v.gameName,&v.textAutoScale,&v.logo,&v.accentLine,&v.backdropFollow,&v.backdropRgb,&v.fixedShortcuts};
 static const char* names[]={"enabled","accent","backdrop","mode","difficulty","record_servers","query_servers","record_battles","button_limit","show_hints","show_version","show_game_name","text_auto_scale","show_logo","show_accent_line","backdrop_follow","backdrop_rgb","fixed_shortcuts"};
 static const int maximum[]={1,5,100,6,3,1,1,1,kMainUiLayoutMax,1,1,1,1,1,1,1,0xffffff,1};unsigned mask=0;bool layoutVersion=false,layoutCount=false,layoutDecor=false;
 unsigned short fields[kMainUiLayoutMax]{};int keyCount=0;const char* p=body+header;bool ok=true;
 while(*p&&ok){const char* end=strchr(p,'\n');if(!end)end=p+strlen(p);size_t length=end-p;if(length&&p[length-1]=='\r')--length;
  if(length>240){ok=false;break;}if(length&&*p!='#'&&*p!=';'){
   char line[256]{};memcpy(line,p,length);char* eq=strchr(line,'=');if(!eq||eq==line||(size_t)(eq-line)>=80||keyCount>=kMainUiLayoutMax*14+32){ok=false;break;}
   char key[80]{};memcpy(key,line,eq-line);++keyCount;
   if(!strncmp(key,"mainmenu_layout_",16)){
    const bool versionKey=!strcmp(key,"mainmenu_layout_v"),countKey=!strcmp(key,"mainmenu_layout_count"),decorKey=!strcmp(key,"mainmenu_layout_decor");
    if(versionKey||countKey||decorKey){if((versionKey&&layoutVersion)||(countKey&&layoutCount)||(decorKey&&layoutDecor)){ok=false;break;}char* tail=nullptr;long n=strtol(eq+1,&tail,10);if(tail==eq+1||*tail||(versionKey?n!=1:countKey?n<0||n>kMainUiLayoutMax:n<0||n>1)){ok=false;break;}}
    else {int index=-1,chunk=-1,pos=0,bit=-1;
     if(sscanf(key,"mainmenu_layout_row_%d%n",&index,&pos)==1&&pos&&key[pos]==0)bit=0;
     else {pos=0;if(sscanf(key,"mainmenu_layout_parent_%d%n",&index,&pos)==1&&pos&&key[pos]==0)bit=1;
      else {pos=0;if(sscanf(key,"mainmenu_layout_title_%d_%d%n",&index,&chunk,&pos)==2&&pos&&key[pos]==0&&chunk>=0&&chunk<4)bit=2+chunk;
       else {pos=0;if(sscanf(key,"mainmenu_layout_param_%d_%d%n",&index,&chunk,&pos)==2&&pos&&key[pos]==0&&chunk>=0&&chunk<8)bit=6+chunk;}}}
     if(index<0||index>=kMainUiLayoutMax||bit<0||(fields[index]&(1u<<bit))){ok=false;break;}fields[index]|=1u<<bit;
    }
    layoutVersion|=versionKey;layoutCount|=countKey;layoutDecor|=decorKey;MainUiLayoutReadLine(line);
   }
   else {int at=-1;for(int i=0;i<(int)_countof(names);++i)if(!strcmp(key,names[i]))at=i;if(at>=0){if(mask&(1u<<at)){ok=false;break;}char* tail=nullptr;long number=strtol(eq+1,&tail,10);if(tail==eq+1||*tail||number<(at==8?256:0)||number>maximum[at]||(at==8&&number!=256&&number!=512)){ok=false;break;}*values[at]=(int)number;mask|=1u<<at;}}
  }p=*end?end+1:end;
 }
 ok=ok&&(mask&255)==255&&layoutVersion&&layoutCount&&layoutDecor&&g_mainUiLayoutCount<=v.limit;
 for(int i=0;i<kMainUiLayoutMax&&ok;++i){if(i>=g_mainUiLayoutCount){if(fields[i])ok=false;continue;}
  const auto& b=g_mainUiLayout[i];wchar_t text[384]{};
  if(fields[i]!=16383||(b.parameter[0]&&(!MainUiLayoutParamValid(b.action,b.parameter)||!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,b.parameter,-1,text,384))))ok=false;
 }
 ok=ok&&MainUiLayoutFinalize();

 if(!ok){restore();return false;}
 g_mainUiBackdropFollow=v.backdropFollow!=0;g_mainUiBackdropRgb=v.backdropRgb;g_mainUiAccent=v.accent;g_mainUiBackdrop=v.backdrop;g_mainUiQuickMode=v.mode;g_mainUiQuickDifficulty=v.difficulty;
 g_optMainUiRememberServers=v.recent!=0;g_optMainUiServerQuery=v.query!=0;g_optMainUiBattleArchive=v.battles!=0;
 g_mainUiButtonLimit=v.limit;
 g_mainUiShowHints=v.hints!=0;g_mainUiShowVersion=v.version!=0;g_mainUiShowGameName=v.gameName!=0;
 g_mainUiShowLogo=v.logo!=0;g_mainUiShowAccentLine=v.accentLine!=0;g_mainUiTextAutoScale=v.textAutoScale!=0;g_mainUiKeyCapture=-1;g_mainUiHotkeySeed=true;
 // Importing an otherwise disabled preset must not hide the UI mid-interaction.
 if(startup){g_optMainMenuUi=v.enabled!=0;g_mainUiFixedShortcuts=v.fixedShortcuts!=0;}
 else {g_mainUiFlyout=-1;g_mainUiPanel=0;g_mainUiPressed=-1;g_mainUiFocus=208;g_mainUiInputSeed=true;MainUiSettingsChanged();}
 ++g_mainUiTextRevision;return true;
}
static void MainUiConfigStartupLoad(){
 char* body=g_mainUiConfigBody;int status=MainUiServicesReadCurrent(body,kMainUiProfileBytes);
 if(status<0||(status>0&&!MainUiConfigApply(body,true))){g_mainUiNotice=5;MainUiServicesMarkCurrentInvalid();}
}
static void MainUiConfigPersistPump(){
 if(!g_mainUiConfigDirty||(LONG)(GetTickCount()-g_mainUiConfigSaveAt)<0)return;
 char* body=g_mainUiConfigBody;if(MainUiConfigCapture(body)&&MainUiServicesSaveCurrent(body))g_mainUiConfigDirty=false;else g_mainUiConfigSaveAt=GetTickCount()+200u;
}
