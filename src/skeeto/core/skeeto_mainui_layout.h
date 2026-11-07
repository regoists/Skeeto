// Homepage layout nest: bounded data, validation and standalone UI serialization.
// Included only by proxy; no engine calls, file scans or recurring allocations.
#pragma once
#include "skeeto_menu_params.h"

static constexpr char kSkeetoDeveloperUrl[] = "https://space.bilibili.com/424692729?spm_id_from=333.1007.0.0";
static constexpr char kSkeetoOpenSourceUrl[] = "https://github.com/regoists/Skeeto";
static constexpr int kMainUiLayoutMax = 512;
static constexpr int kMainUiHitMax=kMainUiLayoutMax+64, kMainUiEditorHitBase=2000;
static int g_mainUiButtonLimit=256;
struct MainUiLayoutSlot {
	int id=0, action=0, x=0, y=0, w=0, h=0;
	int visible=0, color=0, opacity=0, font=0, parent=0;
	char title[192]{}, parameter[384]{};
	wchar_t text[64]{},paramText[384]{};
};
// Unused array storage is zero-filled; construct defaults only for actual slots.
static MainUiLayoutSlot MainUiLayoutBlank(){MainUiLayoutSlot b{};b.w=232;b.h=38;b.visible=1;b.color=-1;b.opacity=-1;b.font=1;b.parent=-1;return b;}
static MainUiLayoutSlot g_mainUiLayout[kMainUiLayoutMax]{};
static int g_mainUiLayoutCount=0;
static bool g_mainUiHomeDecor=true;
static bool g_mainUiLayoutRead=false,g_mainUiLayoutBad=false;
static bool g_mainUiLayoutSeen[kMainUiLayoutMax]{};
static int g_mainUiLayoutVersion=0;

static bool MainUiActionKnown(int action);
static bool MainUiLayoutActionValid(int action) { return MainUiActionKnown(action); }
// Shared by the default preset and explicit editor insertion, never auto-overwrites a user layout.
static void MainUiLayoutWebPresets(MainUiLayoutSlot* slots,int* count,int parent) {
 const char* urls[]={"https://www.kitasoda.com/serverList","https://www.kitasoda.com/maps","http://61.147.247.41:23361/","https://niguban.echocn.cn/index.php?p=servers"};
 const UiTextEntry titles[]={{UiId::MainUiWebSoda,L"汽水查服网页"},{UiId::MainUiWebMaps,L"三方图下载"},{UiId::MainUiWebSherlock,L"夏洛克查服网页"},{UiId::MainUiWebCat,L"猫服查服网页"}};
 for(int i=0;i<4;++i){int id=-1;for(int offset=0;offset<kMainUiLayoutMax;++offset){int candidate=600+(offset+40)%kMainUiLayoutMax;bool used=false;for(int k=0;k<*count;++k)if(slots[k].id==candidate)used=true;if(!used){id=candidate;break;}}
  if(id<0||*count>=kMainUiLayoutMax)return;auto& b=slots[(*count)++];b=MainUiLayoutBlank();b.id=id;b.action=904;b.parent=parent;
  wcscpy_s(b.text,UiText(titles[i].id,titles[i].chinese));WideCharToMultiByte(CP_UTF8,0,b.text,-1,b.title,192,nullptr,nullptr);strcpy_s(b.parameter,urls[i]);MultiByteToWideChar(CP_UTF8,0,b.parameter,-1,b.paramText,384);
 }
}
using MainUiCvarSpec=skmenu::CvarSpec;
static bool MainUiCvarParse(const char* text,MainUiCvarSpec* result){return skmenu::CvarParse(text,result);}
static void MainUiLayoutDefault(MainUiLayoutSlot* slots,int* count) {
 memset(slots,0,sizeof(MainUiLayoutSlot)*kMainUiLayoutMax);*count=0;
 auto add=[&](int id,int action,int parent,int x,int y,int color,int w=232,int h=36,const char* parameter=nullptr){auto& b=slots[(*count)++];b=MainUiLayoutBlank();b.id=id;b.action=action;b.parent=parent;b.x=x;b.y=y;b.w=w;b.h=h;b.color=color;if(parameter){strcpy_s(b.parameter,parameter);MultiByteToWideChar(CP_UTF8,0,parameter,-1,b.paramText,384);}};
 add(4,4,-1,312,34,10,232,40);add(208,208,-1,560,34,5,232,40);add(8,8,-1,808,34,5,232,40);
 add(108,108,-1,0,174,2,260,32);add(0,916,-1,0,208,2,260,32,"coop");add(622,220,-1,0,242,6,260,32);
 for(int i=0;i<8;++i)add(100+i,100+i,-1,0,282+i*32,i==2||i==3||i==5?8:i==7?3:2,260,30);
 add(5,5,-1,0,548,8,124,28);
 const int game[]={200,201,205,1,202,203,206};for(int i=0;i<7;++i)add(game[i],game[i],-1,312,192+i*40,i==2?7:1);
 const int personal[]={217,209,210,12,3,11,207};for(int i=0;i<7;++i)add(personal[i],personal[i]==217?903:personal[i],-1,560,192+i*40,i==3?6:i==4?7:i==6?10:0);
 add(620,903,-1,808,192,3);add(211,211,-1,808,232,6);add(219,219,-1,808,272,7);add(218,218,-1,808,312,9);
 add(636,905,-1,808,352,5,232,36,"left4dead2\\cfg\\autoexec.cfg");add(637,906,-1,808,392,7,232,36,"steam://openurl/https://steamcommunity.com/app/550/workshop/");
 add(621,903,-1,808,432,3);MainUiLayoutWebPresets(slots,count,621);
 add(2,2,620,0,0,3);add(630,212,620,0,0,5);add(631,213,620,0,0,7);
 add(632,906,620,0,0,7,232,36,"steam://url/CommunityFilePage/3782512354");add(633,214,620,0,0,5);
 add(644,904,620,0,0,3,232,36,kSkeetoDeveloperUrl);
 add(645,904,620,0,0,3,232,36,kSkeetoOpenSourceUrl);
 add(634,905,620,0,0,5,232,36,"left4dead2\\skeeto\\mainmenu\\profiles");add(635,905,620,0,0,5,232,36,"left4dead2\\cfg");
 add(700,930,-1,312,131,5,460,30);add(701,931,-1,312,169,10,460,18);
 add(702,932,-1,0,18,5,280,48);add(703,933,-1,2,72,10,62,18);add(704,934,-1,80,72,10,230,18);
 add(706,936,-1,312,568,0,728,22);add(707,937,-1,808,472,6,232,92);
 for(int i=0;i<*count;++i){auto& b=slots[i];b.title[0]=0;b.text[0]=0;if(b.action>=930&&b.action<=937)b.font=1000+(b.action==930?30:b.action==932?44:14);if(b.action==904)b.color=3;}
}
// Subpages follow the same hint component visibility as the homepage preset.
static bool MainUiLayoutHasNavigationHint() {
 for(int i=0;i<g_mainUiLayoutCount;++i){const auto& b=g_mainUiLayout[i];if(b.action==935&&b.visible&&b.opacity!=0&&b.parent<0)return true;}
 return false;
}
static void MainUiLayoutReset() {
	MainUiLayoutDefault(g_mainUiLayout,&g_mainUiLayoutCount);g_mainUiHomeDecor=true;
	g_mainUiLayoutRead=false;g_mainUiLayoutBad=false;g_mainUiLayoutVersion=0;
	memset(g_mainUiLayoutSeen,0,sizeof(g_mainUiLayoutSeen));
}
static bool MainUiColorValid(int c){return c>=-1&&c<12||(c>=0x1000000&&c<=0x1ffffff);}
static void MainUiLayoutClamp(MainUiLayoutSlot& b) {
	auto clamp=[](int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;};
	if(!(b.font>=1008&&b.font<=1096))b.font=clamp(b.font,0,2);b.w=clamp(b.w,40,1040);b.h=clamp(b.h,b.action>=930&&b.action<=937?8:b.font==2?38:b.font==1?30:22,540);
	if(b.action==910){if(b.w<160)b.w=160;if(b.h<36)b.h=36;}
	if(b.action==929){if(b.w<160)b.w=160;if(b.h<36)b.h=36;}
	b.x=clamp(b.x,0,1040-b.w);b.y=clamp(b.y,0,(b.action>=930&&b.action<=937?600:578)-b.h);
	b.visible=b.visible?1:0;if(!MainUiColorValid(b.color))b.color=-1;b.opacity=clamp(b.opacity,-1,100);
}
// Click-time action specifications; bounded parsing only, no engine work.
struct MainUiGameSpec {char token[10][96]{};int count=0;};
static bool MainUiGameTokens(const char* p,MainUiGameSpec* out){
 if(!p||!*p||strlen(p)>=384)return false;MainUiGameSpec v{};
 while(*p){while(*p==' ')++p;if(!*p)break;if(v.count>=10)return false;int n=0;
  while(*p&&*p!=' '){unsigned char c=(unsigned char)*p++;if(n>=95||!(isalnum(c)||c=='_'||c=='-'||c=='.'||c=='/'))return false;v.token[v.count][n++]=(char)c;}
  if(!n||strstr(v.token[v.count],".."))return false;++v.count;
 }if(!v.count)return false;if(out)*out=v;return true;
}
static bool MainUiGameName(const char* p){if(!p||!*p||strlen(p)>=48)return false;for(;*p;++p)if(!isalnum((unsigned char)*p)&&*p!='_')return false;return true;}
static bool MainUiGameNumber(const char* p){char* end=nullptr;float n=strtof(p,&end);return end!=p&&!*end&&_finite(n)&&n>=-1e9f&&n<=1e9f;}
static bool MainUiNativeRouteValid(const char* p){
 const char* fixed[]={"Video","Audio","KeyboardMouse","MultiplayerSettings","SteamController","CloudSettings","Addons","StatsAndAchievements","Leaderboards_survival","DeveloperCommentary","Credits","ChooseChallengeMode","OpenServerBrowser"};
 for(auto name:fixed)if(!strcmp(p,name))return true;
 const char* prefixes[]={"QuickMatch_","CustomMatch_","FriendsMatch_","GroupServer_","SoloPlay_"};
 for(auto prefix:prefixes){size_t n=strlen(prefix);if(!strncmp(p,prefix,n))return MainUiGameName(p+n);}return false;
}
// Positional lobby parameters; every inserted KeyValues token is validated.
static bool MainUiLobbySpecValid(const MainUiGameSpec& v){
 if(v.count<1||v.count>7||!MainUiGameName(v.token[0]))return false;
 if(v.count>1&&strcmp(v.token[1],"-")&&!MainUiGameName(v.token[1]))return false;
 if(v.count>2&&strcmp(v.token[2],"-")){char* end=nullptr;long n=strtol(v.token[2],&end,10);if(end==v.token[2]||*end||n<1||n>99)return false;}
 const char* options[][5]={{"Easy","Normal","Hard","Impossible",nullptr},{"friends","public","private",nullptr,nullptr},{"official","dedicated","listen","group",nullptr},{"LIVE","lan",nullptr,nullptr,nullptr}};
 for(int at=3;at<v.count;++at){if(!strcmp(v.token[at],"-"))continue;bool valid=false;for(int k=0;k<5&&options[at-3][k];++k)if(!strcmp(v.token[at],options[at-3][k]))valid=true;if(!valid)return false;}
 return true;
}
static bool MainUiLobbySettingsText(const char* parameter,char* body,size_t cap){
 MainUiGameSpec v{};if(!body||!cap||!MainUiGameTokens(parameter,&v)||!MainUiLobbySpecValid(v))return false;
 auto value=[&](int at,const char* fallback){return v.count>at&&strcmp(v.token[at],"-")?v.token[at]:fallback;};
 char campaign[96]{},chapter[48]{};if(v.count>1&&strcmp(v.token[1],"-"))snprintf(campaign,sizeof(campaign),"campaign %s",v.token[1]);
 if(v.count>2&&strcmp(v.token[2],"-"))snprintf(chapter,sizeof(chapter),"chapter %s",v.token[2]);
 else if(campaign[0])strcpy_s(chapter,"chapter 1");
 // Defaults: Expert (Impossible) + listen (local) when tokens are omitted.
 int n=snprintf(body,cap,"System { network %s access %s %s } Game { mode %s %s %s difficulty %s } Options { action create server %s }",
  value(6,"LIVE"),value(4,"friends"),!strncmp(v.token[0],"team",4)?"netflag teamlobby":"",v.token[0],campaign,chapter,value(3,"Impossible"),value(5,"listen"));
 return n>0&&(size_t)n<cap;
}
static bool MainUiGameSpecValid(int action,const char* p){MainUiGameSpec v{};if(!MainUiGameTokens(p,&v))return false;
 if(action==916)return MainUiLobbySpecValid(v);
 if(action>=911&&action<=915||action==924)return v.count==1&&MainUiGameName(v.token[0]);
 if(action==917){const char* pages[]={"Video","Audio","KeyboardMouse","MultiplayerSettings","SteamController","CloudSettings","Addons","StatsAndAchievements","Leaderboards_survival","DeveloperCommentary","Credits","ChooseChallengeMode","OpenServerBrowser"};for(auto page:pages)if(v.count==1&&!strcmp(v.token[0],page))return true;return false;}
 if(action==918){if(v.count<2||v.count>3||!MainUiGameName(v.token[1]))return false;if(v.count==2)return true;return !strcmp(v.token[2],"Easy")||!strcmp(v.token[2],"Normal")||!strcmp(v.token[2],"Hard")||!strcmp(v.token[2],"Impossible");}
 if(action>=919&&action<=921){if(!MainUiGameName(v.token[0])||v.count<2||action==919&&v.count!=2||action==920&&v.count!=3||action==921&&v.count<3)return false;for(int i=1;i<v.count;++i)if(!MainUiGameNumber(v.token[i]))return false;return true;}
 if(action==922)return v.count==1&&v.token[0][0]!='/'&&v.token[0][0]!='.';
 if(action==925)return v.count==1&&MainUiNativeRouteValid(p);return false;
}
static bool MainUiLayoutParamValid(int action,const char* p) {
	if (!p || !p[0] || strlen(p)>=384) return false;
	if(action>=930&&action<=937){for(const unsigned char* q=(const unsigned char*)p;*q;++q)if(*q<32||*q==127)return false;return true;}
	if(action>=911&&action<=925)return MainUiGameSpecValid(action,p);
	if(action==910)return MainUiCvarParse(p,nullptr);
	if(action==900&&strlen(p)>=96)return false;
	if(action==900){const char* at=strchr(p,'@');if(at){char base[96]{};size_t length=at-p;
		if(!length||length>=96||strchr(at+1,'@'))return false;memcpy(base,p,length);char* end=nullptr;long port=strtol(at+1,&end,10);
		return end!=at+1&&!*end&&port>0&&port<=65535&&MainUiLayoutParamValid(900,base);}}
	if(action>=904&&action<=906)return MainUiExternalValid(action-904,p);

	if(action==907){size_t n=strlen(p);if(n>100||n<5||_stricmp(p+n-4,".hud"))return false;for(const char* q=p;*q;++q)if(!isalnum((unsigned char)*q)&&*q!='.'&&*q!='_'&&*q!='-')return false;return !strstr(p,"..");}
	if(action==902){for(const unsigned char* q=(const unsigned char*)p;*q;++q)if(*q<32||*q==127)return false;return strspn(p," ")!=strlen(p);}
	if (action!=900 && action!=901) return true;
	if(strlen(p)>=96)return false;
 if(action==900&&strchr(p,':')&&(p[0]=='['||strchr(strchr(p,':')+1,':'))){
  const char* host=p+(p[0]=='['?1:0);const char* close=p[0]=='['?strchr(host,']'):p+strlen(p);
  if(!close||close==host)return false;int colons=0;
  for(const char* q=host;q<close;++q){char c=*q;if(c==':')++colons;else if(!isxdigit((unsigned char)c)&&c!='.'&&c!='%')return false;}
  if(colons<2)return false;
  if(p[0]=='['&&close[1]){if(close[1]!=':')return false;const char* q=close+2;if(!*q)return false;for(const char* v=q;*v;++v)if(!isdigit((unsigned char)*v))return false;long port=strtol(q,nullptr,10);if(port<1||port>65535)return false;}
  return true;
 }

	for (const char* q=p;*q;++q) {
		const unsigned char c=(unsigned char)*q;
		if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')
			||c=='_'||c=='-'||(action==900&&(c=='.'||c==':'))||(action==901&&c=='/'))) return false;
	}
	if (!isalnum((unsigned char)p[0]) || !isalnum((unsigned char)p[strlen(p)-1])) return false;
	if (action==900) {
		const char* colon=strchr(p,':');
		if (colon) {
			if (strchr(colon+1,':')||colon==p) return false;
			char* end=nullptr;long port=strtol(colon+1,&end,10);
			if (!end || *end || port<1 || port>65535) return false;
		}
		if (strstr(p,"..")) return false;
	} else if (strstr(p,"//")) return false;
	return true;
}
static void MainUiServerConnectParameter(const char* p,char* out,int cap){size_t n=strcspn(p,"@");if(n>=(size_t)cap)n=cap-1;memcpy(out,p,n);out[n]=0;}
static int MainUiHexDigit(char c) {
	return c>='0'&&c<='9'?c-'0':c>='A'&&c<='F'?c-'A'+10:c>='a'&&c<='f'?c-'a'+10:-1;
}
template<class Append> static bool MainUiLayoutSerialize(Append append) {
 bool ok=append("mainmenu_layout_v=1\nmainmenu_layout_count=%d\nmainmenu_layout_decor=%d\n",g_mainUiLayoutCount,g_mainUiHomeDecor?1:0);
 for(int i=0;i<g_mainUiLayoutCount;++i){const auto& b=g_mainUiLayout[i];ok=append("mainmenu_layout_row_%d=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",i,b.id,b.action,b.x,b.y,b.w,b.h,b.visible,b.color,b.opacity,b.font)&&ok;ok=append("mainmenu_layout_parent_%d=%d\n",i,b.parent)&&ok;
  for(int field=0;field<2;++field){const char* text=field?b.parameter:b.title;int length=(int)strlen(text);for(int chunk=0;chunk<(field?8:4);++chunk){char hex[97]{};int n=0;for(int k=chunk*48;k<(chunk+1)*48&&k<length;++k){unsigned c=(unsigned char)text[k];hex[n++]="0123456789ABCDEF"[c>>4];hex[n++]="0123456789ABCDEF"[c&15];}ok=append("mainmenu_layout_%s_%d_%d=%s\n",field?"param":"title",i,chunk,hex)&&ok;}}
 }return ok;
}
// Used by offline serialization fixtures; runtime saves via the module's worker.
static void MainUiLayoutWrite(FILE* f){MainUiLayoutSerialize([&](const char* format,auto... args){return fprintf(f,format,args...)>=0;});}
static bool MainUiLayoutReadLine(const char* line) {
	if (strncmp(line,"mainmenu_layout_",16)) return false;
	g_mainUiLayoutRead=true;
	int v=0,index=-1,chunk=-1,pos=0;
	if (sscanf(line,"mainmenu_layout_v=%d",&v)==1) {g_mainUiLayoutVersion=v;return true;}
	if (sscanf(line,"mainmenu_layout_count=%d",&v)==1) {
		if(v<0||v>kMainUiLayoutMax)g_mainUiLayoutBad=true;else g_mainUiLayoutCount=v;return true;
	}
	if (sscanf(line,"mainmenu_layout_decor=%d",&v)==1) {g_mainUiHomeDecor=v!=0;return true;}
	if (sscanf(line,"mainmenu_layout_row_%d=%n",&index,&pos)==1 && pos) {
		if(index<0||index>=kMainUiLayoutMax){g_mainUiLayoutBad=true;return true;}
		MainUiLayoutSlot b=MainUiLayoutBlank();b.parent=g_mainUiLayout[index].parent;int* fields[]={&b.id,&b.action,&b.x,&b.y,&b.w,&b.h,&b.visible,&b.color,&b.opacity,&b.font};
		const char* p=line+pos;
		for(int k=0;k<10;++k) {
			char* end=nullptr;const long n=strtol(p,&end,10);
			if(end==p||n<-100000||(k==7?!MainUiColorValid((int)n):n>100000)||(k<9?*end!=',':*end!='\r'&&*end!='\n'&&*end!=0)){g_mainUiLayoutBad=true;return true;}
			*fields[k]=(int)n;p=end+(k<9?1:0);
		}
		// Metadata may precede geometry: preserve accumulated text chunks.
		memcpy(b.title,g_mainUiLayout[index].title,sizeof(b.title));memcpy(b.parameter,g_mainUiLayout[index].parameter,sizeof(b.parameter));
		g_mainUiLayout[index]=b;g_mainUiLayoutSeen[index]=true;return true;
	}
	pos=0;
	if(sscanf(line,"mainmenu_layout_parent_%d=%n",&index,&pos)==1&&pos){
		char* end=nullptr;long parent=strtol(line+pos,&end,10);
		if(index<0||index>=kMainUiLayoutMax||end==line+pos||parent<-1||parent>=600+kMainUiLayoutMax||(*end&&*end!='\r'&&*end!='\n'))g_mainUiLayoutBad=true;
		else g_mainUiLayout[index].parent=(int)parent;return true;
	}
	pos=0;bool parameter=false;
	if(!(sscanf(line,"mainmenu_layout_title_%d_%d=%n",&index,&chunk,&pos)==2 && pos)) {
		pos=0;parameter=true;
		if(!(sscanf(line,"mainmenu_layout_param_%d_%d=%n",&index,&chunk,&pos)==2 && pos)){g_mainUiLayoutBad=true;return true;}
	}
	if(index<0||index>=kMainUiLayoutMax||chunk<0||chunk>=(parameter?8:4)){g_mainUiLayoutBad=true;return true;}
	char* dst=parameter?g_mainUiLayout[index].parameter:g_mainUiLayout[index].title;
	const int cap=parameter?384:192;const char* p=line+pos;const size_t n=strcspn(p,"\r\n");
	if(n>96||n%2){g_mainUiLayoutBad=true;return true;}
	memset(dst+chunk*48,0,48);
	for(size_t k=0;k<n/2;++k) {
		int a=MainUiHexDigit(p[2*k]),b=MainUiHexDigit(p[2*k+1]);int offset=chunk*48+(int)k;
		if(a<0||b<0||offset>=cap-1||!(a||b)){g_mainUiLayoutBad=true;return true;}
		dst[offset]=(char)((a<<4)|b);
	}
	return true;
}
static bool MainUiLayoutFinalize() {
	if(!g_mainUiLayoutRead)return true;
	bool good=!g_mainUiLayoutBad&&g_mainUiLayoutVersion==1;
	for(int i=0;i<g_mainUiLayoutCount && good;++i) {
		auto& b=g_mainUiLayout[i];good=g_mainUiLayoutSeen[i]&&MainUiLayoutActionValid(b.action)
			&& (!(b.action>=930&&b.action<=937)||b.parent<0)
			&& (MainUiLayoutActionValid(b.id)||b.id>=600&&b.id<600+kMainUiLayoutMax);
		for(int j=0;j<i;++j) if(g_mainUiLayout[j].id==b.id)good=false;
		if(b.title[0]&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,b.title,-1,b.text,64))good=false;
		for(const wchar_t* p=b.text;*p;++p)if(*p<32||*p==127)good=false;
		MainUiLayoutClamp(b);
		if(b.parameter[0]&&!MainUiLayoutParamValid(b.action,b.parameter))b.parameter[0]=0;
		if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,b.parameter,-1,b.paramText,384)){b.parameter[0]=0;b.paramText[0]=0;}
	}
 int parents[kMainUiLayoutMax]{};
 for(int i=0;i<g_mainUiLayoutCount&&good;++i){parents[i]=-1;if(g_mainUiLayout[i].parent<0)continue;
  for(int j=0;j<g_mainUiLayoutCount;++j)if(g_mainUiLayout[j].id==g_mainUiLayout[i].parent)parents[i]=j;
  if(parents[i]<0||g_mainUiLayout[parents[i]].action!=903)good=false;
 }
 for(int i=0;i<g_mainUiLayoutCount&&good;++i){int at=parents[i],steps=0;while(at>=0){if(at==i||++steps>g_mainUiLayoutCount){good=false;break;}at=parents[at];}}

	if(!good)MainUiLayoutReset();
	return good;
}
