// Homepage data/service nest. Existing worker owns file, shell and UDP operations.
// SRW locks only protect fixed copies; Paint never waits for worker IO.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <ctime>
#include <cstdlib>
#include <cstddef>
#include <cerrno>
#include <objbase.h>
#include "skeeto_mainui_services.h"
#include "skeeto_mainui_report.h"

char g_mainUiAvatarUri[kMainUiAvatarUriChars];

namespace {
struct Job {int type=0,kind=0,textLimit=383,labelCount=0;unsigned generation=0;char parameter[512]{};MainUiServerRecord server{};MainUiBattleRecord battle{};MainUiCareerReport career{};MainUiBattleLabel labels[10]{};size_t bytes=0;};
SRWLOCK lock=SRWLOCK_INIT;
Job queue[4]{};char queuedBodies[4][kMainUiProfileBytes]{},workerBody[kMainUiProfileBytes]{};int head=0,count=0;
MainUiHubSnapshot published{},data{};
MainUiBattleDays battleDays{};
bool historyWritable=true;
char prepared[kMainUiProfileBytes]{};bool preparedReady=false;
std::atomic_uint activeTextTicket{0};unsigned nextTextTicket=0,textReadyTicket=0;
wchar_t completedText[384]{};MainUiTextResult textResult=MainUiTextResult::None;
std::atomic_bool dialogBusy{false};
std::atomic_uint dialogTextTicket{0};
std::atomic_uint profileGeneration{0};unsigned preparedGeneration=0;
std::atomic_bool visible{false},queryWanted{false},pending{false};
std::atomic_bool accepting{false};
wchar_t root[MAX_PATH]{},directory[MAX_PATH]{};
bool loaded=false,network=false;
std::atomic_bool currentInvalid{false};
char targets[kMainUiQueryMax][96]{};int targetCount=0,nextTarget=0;
SOCKET socketHandle=INVALID_SOCKET;DWORD sentAt=0,cycleAt=0;int active=-1,challengeCount=0,challengePing=0;
unsigned char request[29]{};
uint64_t Now(){return (uint64_t)time(nullptr);}
bool Utf8(const char* s,wchar_t* d,int n){return s&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,d,n)>0;}
void Clean(char* s,int cap){s[cap-1]=0;for(int i=0;s[i];++i)if((unsigned char)s[i]<32||s[i]==127)s[i]=' ';}
void Publish(){++data.revision;AcquireSRWLockExclusive(&lock);published=data;ReleaseSRWLockExclusive(&lock);}
bool Post(const Job& job,const void* body=nullptr,size_t bytes=0){if(bytes>kMainUiProfileBytes||(!body&&bytes)||!TryAcquireSRWLockExclusive(&lock))return false;
 bool ok=accepting.load()&&count<4;if(ok){int slot=(head+count)%4;queue[slot]=job;queue[slot].bytes=bytes;if(bytes)memcpy(queuedBodies[slot],body,bytes);++count;pending=true;}ReleaseSRWLockExclusive(&lock);return ok;}
bool Pop(Job& job){AcquireSRWLockExclusive(&lock);bool ok=count>0;if(ok){job=queue[head];if(job.bytes)memcpy(workerBody,queuedBodies[head],job.bytes);head=(head+1)%4;--count;}pending=count>0;ReleaseSRWLockExclusive(&lock);return ok;}
void PublishText(const Job& job,MainUiTextResult result,const wchar_t* text=nullptr){
 AcquireSRWLockExclusive(&lock);
 if(job.generation==activeTextTicket.load()){textReadyTicket=job.generation;textResult=result;if(text)wcscpy_s(completedText,text);}
 ReleaseSRWLockExclusive(&lock);
}
void CloseQuery(){if(socketHandle!=INVALID_SOCKET)closesocket(socketHandle);socketHandle=INVALID_SOCKET;active=-1;}
bool DataPath(const wchar_t* name,wchar_t* path){return directory[0]&&swprintf_s(path,MAX_PATH,L"%s\\%s",directory,name)>0;}
bool AtomicWrite(const wchar_t* path,const void* bytes,size_t size){
 wchar_t temp[MAX_PATH]{};if(swprintf_s(temp,L"%s.tmp",path)<0)return false;
 FILE* f=_wfopen(temp,L"wb");if(!f)return false;bool ok=fwrite(bytes,1,size,f)==size;
 if(fclose(f))ok=false;if(ok)ok=MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 if(!ok)DeleteFileW(temp);return ok;
}
// history.bin v4 stores seven calendar-day totals, ten detailed chapters and their display names.
// Older files keep their original bytes until a successful load copies them forward.
struct MainUiBattleRecordV1 {uint64_t time=0;char map[96]{},server[96]{},player[192]{},version[32]{};int result=0,seconds=0,score=0,scoreEnabled=0,pageMask=0,statCount=25,stats[32]{},headshots[9]{},weights[32]{};};
struct MainUiBattleRecordV3 {uint64_t time=0;char map[96]{},server[96]{},player[192]{},version[32]{};int result=0,seconds=0,score=0,scoreEnabled=0,pageMask=0,statCount=25,stats[32]{},headshots[9]{},weights[32]{};int mateCount=0;MainUiBattleMate mates[kMainUiBattleMates]{};};
struct DiskV1 {unsigned magic=0x534B4831,version=1;int recent=0,battles=0;MainUiServerRecord servers[10]{};MainUiBattleRecordV1 reports[10]{};unsigned checksum=0;};
struct DiskV2 {unsigned magic=0x534B4831,version=2;int recent=0,battles=0;MainUiServerRecord servers[10]{};MainUiBattleRecordV3 reports[10]{};unsigned checksum=0;};
struct DiskV3 {unsigned magic=0x534B4831,version=3;int recent=0,battles=0;MainUiServerRecord servers[10]{};MainUiBattleRecordV3 reports[10]{};MainUiBattleDays days{};unsigned checksum=0;};
struct Disk {unsigned magic=0x534B4831,version=4;int recent=0,battles=0;MainUiServerRecord servers[10]{};MainUiBattleRecord reports[10]{};MainUiBattleDays days{};unsigned checksum=0;};
void ImportV3(MainUiBattleRecord& dst,const MainUiBattleRecordV3& src){dst={};dst.time=src.time;memcpy(dst.map,src.map,sizeof(dst.map));memcpy(dst.server,src.server,sizeof(dst.server));memcpy(dst.player,src.player,sizeof(dst.player));memcpy(dst.version,src.version,sizeof(dst.version));dst.result=src.result;dst.seconds=src.seconds;dst.score=src.score;dst.scoreEnabled=src.scoreEnabled;dst.pageMask=src.pageMask;dst.statCount=src.statCount;memcpy(dst.stats,src.stats,sizeof(dst.stats));memcpy(dst.headshots,src.headshots,sizeof(dst.headshots));memcpy(dst.weights,src.weights,sizeof(dst.weights));dst.mateCount=src.mateCount;memcpy(dst.mates,src.mates,sizeof(dst.mates));}
unsigned Hash(const void* b,size_t n){unsigned h=2166136261u;auto* p=(const unsigned char*)b;for(size_t i=0;i<n;++i)h=(h^p[i])*16777619u;return h;}
void Save(){if(!historyWritable){data.status=3;return;}Disk d{};d.days=battleDays;d.recent=data.recentCount;d.battles=data.battleCount;memcpy(d.servers,data.recent,sizeof(d.servers));memcpy(d.reports,data.battles,sizeof(d.reports));d.checksum=Hash(&d,offsetof(Disk,checksum));wchar_t path[MAX_PATH]{};if(!DataPath(L"history.bin",path)||!AtomicWrite(path,&d,sizeof(d)))data.status=3;}
// Clean text and range-check one stored chapter. Mates are fixed copies; no pointers are stored.
bool BattleValid(MainUiBattleRecord& b){Clean(b.map,96);Clean(b.server,96);Clean(b.player,192);Clean(b.version,32);Clean(b.campaign,192);Clean(b.chapter,192);Clean(b.serverName,192);
 if(b.statCount<0||b.statCount>32||b.seconds<0||b.mateCount<0||b.mateCount>kMainUiBattleMates)return false;
 for(int m=0;m<kMainUiBattleMates;++m){Clean(b.mates[m].name,64);b.mates[m].mvp=b.mates[m].mvp!=0;b.mates[m].bot=b.mates[m].bot!=0;b.mates[m].local=b.mates[m].local!=0;}return true;}
void Load(){
 loaded=true;wchar_t path[MAX_PATH]{};if(!DataPath(L"history.bin",path))return;
 FILE* f=_wfopen(path,L"rb");if(!f){if(errno!=ENOENT){historyWritable=false;data.status=3;}return;}fseek(f,0,SEEK_END);const long size=ftell(f);fseek(f,0,SEEK_SET);
 Disk d{};bool ok=false,hasDays=false;
 if(size==(long)sizeof(Disk)){ok=fread(&d,1,sizeof(d),f)==sizeof(d)&&d.magic==0x534B4831&&d.version==4&&d.recent>=0&&d.recent<=10&&d.battles>=0&&d.battles<=10&&d.checksum==Hash(&d,offsetof(Disk,checksum))&&MainUiBattleDaysValid(d.days);hasDays=ok;}
 else if(size==(long)sizeof(DiskV3)){
  DiskV3 v{};ok=fread(&v,1,sizeof(v),f)==sizeof(v)&&v.magic==0x534B4831&&v.version==3&&v.recent>=0&&v.recent<=10&&v.battles>=0&&v.battles<=10&&v.checksum==Hash(&v,offsetof(DiskV3,checksum))&&MainUiBattleDaysValid(v.days);
  if(ok){d.recent=v.recent;d.battles=v.battles;memcpy(d.servers,v.servers,sizeof(d.servers));d.days=v.days;for(int i=0;i<v.battles&&i<10;++i)ImportV3(d.reports[i],v.reports[i]);hasDays=true;}
 }
 else if(size==(long)sizeof(DiskV2)){
  DiskV2 v{};ok=fread(&v,1,sizeof(v),f)==sizeof(v)&&v.magic==0x534B4831&&v.version==2&&v.recent>=0&&v.recent<=10&&v.battles>=0&&v.battles<=10&&v.checksum==Hash(&v,offsetof(DiskV2,checksum));
  if(ok){d.recent=v.recent;d.battles=v.battles;memcpy(d.servers,v.servers,sizeof(d.servers));for(int i=0;i<v.battles&&i<10;++i)ImportV3(d.reports[i],v.reports[i]);}
 }
 else if(size==(long)sizeof(DiskV1)){ // Keep servers and successful personal rows. Squad names were not stored.
  DiskV1 v{};ok=fread(&v,1,sizeof(v),f)==sizeof(v)&&v.magic==0x534B4831&&v.version==1&&v.recent>=0&&v.recent<=10&&v.battles>=0&&v.battles<=10&&v.checksum==Hash(&v,offsetof(DiskV1,checksum));
  if(ok){d.recent=v.recent;memcpy(d.servers,v.servers,sizeof(d.servers));d.battles=0;
   for(int i=0;i<v.battles;++i)if(v.reports[i].result==0){auto& b=d.reports[d.battles++];memcpy(&b,&v.reports[i],offsetof(MainUiBattleRecord,mateCount));b.mateCount=0;}}}
 fclose(f);
 if(!ok){historyWritable=false;data.status=3;return;}
 for(int i=0;i<d.recent;++i){Clean(d.servers[i].address,96);Clean(d.servers[i].name,192);Clean(d.servers[i].map,96);}
 int kept=0;for(int i=0;i<d.battles;++i){auto& b=d.reports[i];if(!BattleValid(b)){historyWritable=false;data.status=3;return;}if(b.result==0)d.reports[kept++]=b;} // Failures are never kept.
 for(int i=kept;i<10;++i)d.reports[i]=MainUiBattleRecord{};d.battles=kept;
 battleDays=d.days;
 if(!hasDays)for(int i=0;i<d.battles;++i)MainUiBattleDaysAdd(battleDays,d.reports[i],(time_t)Now());
 data.recentCount=d.recent;data.battleCount=d.battles;memcpy(data.recent,d.servers,sizeof(data.recent));memcpy(data.battles,d.reports,sizeof(data.battles));
}
bool Record(const Job& job){
 if(job.type==1){auto row=job.server;Clean(row.address,96);Clean(row.name,192);Clean(row.map,96);row.visited=Now();int at=-1;for(int i=0;i<data.recentCount;++i)if(!_stricmp(data.recent[i].address,row.address))at=i;
  if(at<0)at=data.recentCount<10?data.recentCount++:9;for(int i=at;i>0;--i)data.recent[i]=data.recent[i-1];data.recent[0]=row;return true;}
 if(job.type==2){auto row=job.battle;if(row.result!=0||!BattleValid(row))return false;row.time=Now();MainUiBattleDaysAdd(battleDays,row,(time_t)row.time);if(data.battleCount<10)++data.battleCount;for(int i=data.battleCount-1;i>0;--i)data.battles[i]=data.battles[i-1];data.battles[0]=row;return true;}
 return false;
}
bool ProfileLeaf(const char* p){size_t n=p?strlen(p):0;if(!n||n>100)return false;
 for(size_t i=0;i<n;++i)if(!((p[i]>='a'&&p[i]<='z')||(p[i]>='A'&&p[i]<='Z')||(p[i]>='0'&&p[i]<='9')||p[i]=='_'||p[i]=='-'||p[i]=='.'))return false;
 return !strstr(p,"..")&&n>4&&!_stricmp(p+n-4,".hud");}
void ScanProfiles(){
 data.profileCount=0;wchar_t pattern[MAX_PATH]{};if(!DataPath(L"profiles\\*.hud",pattern))return;
 WIN32_FIND_DATAW item{};HANDLE h=FindFirstFileW(pattern,&item);if(h==INVALID_HANDLE_VALUE)return;
 int budget=0;do{if(++budget>256)break;if(item.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;
  char name[128]{};if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,item.cFileName,-1,name,128,nullptr,nullptr)||!ProfileLeaf(name))continue;
  int n=data.profileCount;if(n<12){strcpy_s(data.profiles[n],name);++data.profileCount;}
 }while(FindNextFileW(h,&item));FindClose(h);
 for(int i=1;i<data.profileCount;++i)for(int j=i;j>0&&strcmp(data.profiles[j-1],data.profiles[j])>0;--j){char t[128];strcpy_s(t,data.profiles[j]);strcpy_s(data.profiles[j],data.profiles[j-1]);strcpy_s(data.profiles[j-1],t);}
}
void EnsureData(){
 if(loaded||!directory[0])return;wchar_t p[MAX_PATH]{};swprintf_s(p,L"%s\\left4dead2\\skeeto",root);CreateDirectoryW(p,nullptr);CreateDirectoryW(directory,nullptr);DataPath(L"profiles",p);CreateDirectoryW(p,nullptr);Load();ScanProfiles();Publish();
}
bool ProfilePath(const char* leaf,wchar_t* path){wchar_t name[128]{};return ProfileLeaf(leaf)&&Utf8(leaf,name,128)&&swprintf_s(path,MAX_PATH,L"%s\\profiles\\%s",directory,name)>0;}

// Only the dialog thread runs blocking common-dialog UI. It writes a copied
// path/result; the existing worker retains all file IO and published state.
struct ProfileDialog {
 HANDLE thread=nullptr;HMODULE module=nullptr;Job job{};char* body=nullptr;
 wchar_t path[MAX_PATH]{},text[384]{};int result=0;
 std::atomic<HWND> window{nullptr};std::atomic_bool done{false},cancel{false};
} profileDialog;
#ifdef SKEETO_MAINUI_SERVICE_TEST
std::atomic<HANDLE> dialogTestGate{nullptr};
#endif
UINT_PTR CALLBACK ProfileDialogHook(HWND child,UINT message,WPARAM,LPARAM){
 if(message==WM_INITDIALOG){HWND window=GetParent(child);profileDialog.window.store(window);
  if(profileDialog.cancel.load()||!accepting.load())PostMessageW(window,WM_CLOSE,0,0);}
 return 0;
}
INT_PTR CALLBACK TextInputProc(HWND window,UINT message,WPARAM wp,LPARAM){
 if(message==WM_INITDIALOG){profileDialog.window=window;SetDlgItemTextW(window,100,(const wchar_t*)profileDialog.body);SendDlgItemMessageW(window,100,EM_SETLIMITTEXT,profileDialog.job.textLimit,0);SendDlgItemMessageW(window,100,EM_SETSEL,0,-1);SetFocus(GetDlgItem(window,100));if(profileDialog.cancel||!accepting)PostMessageW(window,WM_CLOSE,0,0);return FALSE;}
 if(message==WM_COMMAND){if(LOWORD(wp)==IDOK){if(GetWindowTextLengthW(GetDlgItem(window,100))>profileDialog.job.textLimit){MessageBeep(MB_ICONWARNING);return TRUE;}GetDlgItemTextW(window,100,profileDialog.text,384);EndDialog(window,1);return TRUE;}if(LOWORD(wp)==IDCANCEL){EndDialog(window,0);return TRUE;}}
 if(message==WM_CLOSE){EndDialog(window,0);return TRUE;}return FALSE;
}
int TextInputDialog(){
 // Stack template lives for the modal call; standard Unicode EDIT owns IME.
 alignas(DWORD) WORD storage[1024]{};WORD* at=storage;
 auto word=[&](WORD v){*at++=v;};auto dword=[&](DWORD v){memcpy(at,&v,4);at+=2;};auto text=[&](const wchar_t* s){while(*s)word((WORD)*s++);word(0);};auto align=[&](){if((uintptr_t)at&3)word(0);};
 wchar_t caption[192]{};Utf8(profileDialog.job.parameter,caption,192);
 dword(WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_SETFONT|DS_CENTER);dword(0);word(3);word(0);word(0);word(320);word(86);word(0);word(0);text(caption);word(10);text(L"Segoe UI");
 auto item=[&](DWORD style,short x,short y,short w,short h,WORD id,WORD cls,const wchar_t* title){align();dword(style|WS_CHILD|WS_VISIBLE);dword(0);word(x);word(y);word(w);word(h);word(id);word(0xffff);word(cls);text(title);word(0);};
 item(WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,10,12,300,22,100,0x81,L"");
 item(WS_TABSTOP|BS_DEFPUSHBUTTON,152,49,74,22,IDOK,0x80,L"OK");item(WS_TABSTOP|BS_PUSHBUTTON,236,49,74,22,IDCANCEL,0x80,L"Cancel");
 INT_PTR result=DialogBoxIndirectParamW(profileDialog.module,(DLGTEMPLATE*)storage,FindWindowW(L"Valve001",nullptr),TextInputProc,0);return result==1?1:result==-1?-1:0;
}
DWORD WINAPI ProfileDialogThread(void*){
 bool save=profileDialog.job.kind==0;
 wcscpy_s(profileDialog.path,L"my_mainmenu.hud");OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);
 dialog.hwndOwner=FindWindowW(L"Valve001",nullptr);dialog.lpstrFilter=L"Skeeto Main Menu UI (*.hud)\0*.hud\0\0";dialog.lpstrFile=profileDialog.path;dialog.nMaxFile=MAX_PATH;dialog.lpstrDefExt=L"hud";dialog.lpstrInitialDir=directory;
 dialog.lpfnHook=ProfileDialogHook;dialog.Flags=OFN_ENABLEHOOK|OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
#ifdef SKEETO_MAINUI_SERVICE_TEST
 HANDLE testGate=dialogTestGate.load();
 if(testGate){while(!profileDialog.cancel.load()&&WaitForSingleObject(testGate,10)==WAIT_TIMEOUT){}profileDialog.result=0;}
 else
#endif
 profileDialog.result=profileDialog.job.type==8?(!profileDialog.cancel.load()?TextInputDialog():0):!profileDialog.cancel.load()&&(save?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog))?1:CommDlgExtendedError()? -1:0;
 profileDialog.window.store(nullptr);profileDialog.done.store(true,std::memory_order_release);
 HMODULE module=profileDialog.module;
 if(module)FreeLibraryAndExitThread(module,0);
 return 0;
}
bool StartProfileDialog(const Job& job){
 if(profileDialog.thread){data.status=4;return false;}
 char* body=nullptr;if(job.bytes){body=(char*)HeapAlloc(GetProcessHeap(),0,job.bytes);if(!body){data.status=3;return false;}memcpy(body,workerBody,job.bytes);}
 HMODULE module=nullptr;
 if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,(LPCWSTR)&ProfileDialogThread,&module)){if(body)HeapFree(GetProcessHeap(),0,body);data.status=3;return false;}
 profileDialog.job=job;profileDialog.body=body;profileDialog.module=module;profileDialog.result=0;profileDialog.cancel=false;profileDialog.done=false;
 dialogTextTicket=job.type==8?job.generation:0;dialogBusy=true;profileDialog.thread=CreateThread(nullptr,0,ProfileDialogThread,nullptr,0,nullptr);
 if(!profileDialog.thread){dialogTextTicket=0;dialogBusy=false;FreeLibrary(module);if(body)HeapFree(GetProcessHeap(),0,body);profileDialog.body=nullptr;data.status=3;return false;}
 return true;
}
void CancelProfileDialog(){profileDialog.cancel=true;HWND window=profileDialog.window.load();if(window)PostMessageW(window,WM_CLOSE,0,0);}
void Profile(const Job& job,const wchar_t* selected=nullptr,const char* exportBody=nullptr){
 wchar_t path[MAX_PATH]{};bool save=job.kind==0||job.kind==3;
 if(!save&&job.generation!=profileGeneration.load())return;
 if(job.kind==0||job.kind==1){if(!selected){StartProfileDialog(job);return;}wcscpy_s(path,selected);
 }else if(!ProfilePath(job.parameter,path)){data.status=2;return;}
 if(save){const char* body=exportBody?exportBody:workerBody;size_t length=strnlen(body,kMainUiProfileBytes);if(length>=kMainUiProfileBytes||!AtomicWrite(path,body,length))data.status=3;else data.status=1;}
 else {
  FILE* f=_wfopen(path,L"rb");if(!f){data.status=3;return;}
  char* body=workerBody;size_t n=fread(body,1,kMainUiProfileBytes-1,f);bool ok=!ferror(f)&&fgetc(f)==EOF;fclose(f);body[n]=0;
  if(!ok||memchr(body,0,n)||strncmp(body,"SKEETO_MAINMENU_UI=1\n",21)){data.status=2;return;}
  AcquireSRWLockExclusive(&lock);if(job.generation!=profileGeneration.load()){ReleaseSRWLockExclusive(&lock);return;}if(preparedReady&&preparedGeneration!=job.generation)preparedReady=false;if(!preparedReady){memcpy(prepared,body,n+1);preparedReady=true;preparedGeneration=job.generation;data.status=1;}else data.status=4;ReleaseSRWLockExclusive(&lock);
 }
 ScanProfiles();
}

void PollProfileDialog(){
 if(!profileDialog.thread)return;
 if(!accepting.load()||(profileDialog.job.type==8?profileDialog.job.generation!=activeTextTicket.load():profileDialog.job.kind==1&&profileDialog.job.generation!=profileGeneration.load()))CancelProfileDialog();
 if(!profileDialog.done.load(std::memory_order_acquire)||WaitForSingleObject(profileDialog.thread,0)!=WAIT_OBJECT_0)return;
 CloseHandle(profileDialog.thread);profileDialog.thread=nullptr;dialogTextTicket=0;dialogBusy=false;
 if(profileDialog.job.type==8)PublishText(profileDialog.job,profileDialog.result<0?MainUiTextResult::Failed:
  !profileDialog.cancel.load()&&profileDialog.result==1?MainUiTextResult::Accepted:MainUiTextResult::Cancelled,profileDialog.text);
 else if(!profileDialog.cancel.load()&&profileDialog.result==1)Profile(profileDialog.job,profileDialog.path,profileDialog.body);
 else if(profileDialog.result<0)data.status=3;
 if(profileDialog.body)HeapFree(GetProcessHeap(),0,profileDialog.body);profileDialog.body=nullptr;Publish();
}
bool ApplyLabels(const Job& job){bool changed=false;if(job.labelCount<=0)return false;
 for(int i=0;i<data.battleCount&&i<10;++i){auto& b=data.battles[i];
  for(int k=0;k<job.labelCount&&k<10;++k){const auto& label=job.labels[k];if(label.time!=b.time||_stricmp(label.map,b.map))continue;
   if(!b.campaign[0]&&label.campaign[0]){strncpy_s(b.campaign,label.campaign,_TRUNCATE);changed=true;}
   if(!b.chapter[0]&&label.chapter[0]){strncpy_s(b.chapter,label.chapter,_TRUNCATE);changed=true;}
   if(!b.serverName[0]&&label.serverName[0]){strncpy_s(b.serverName,label.serverName,_TRUNCATE);changed=true;}}}
 if(changed)for(int i=0;i<data.battleCount&&i<10;++i)BattleValid(data.battles[i]);
 return changed;}
bool Report(wchar_t* path,const MainUiCareerReport& career,const char* avatarUri){
 if(!DataPath(L"battle-report.html",path))return false;wchar_t temp[MAX_PATH]{};swprintf_s(temp,L"%s.tmp",path);FILE* f=_wfopen(temp,L"wb");if(!f)return false;
 const bool rendered=MainUiReportWrite(f,data.battles,data.battleCount,time(nullptr),battleDays,&career,avatarUri);
 bool ok=rendered;if(fclose(f))ok=false;if(ok)ok=MoveFileExW(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;if(!ok)DeleteFileW(temp);return ok;
}
void Open(const Job& job){
 wchar_t path[MAX_PATH*2]{};if(job.kind==3){swprintf_s(path,L"%s\\Skeeto_更新日志.txt",root);}
 else if(job.kind==4){if(ApplyLabels(job))Save();wchar_t report[MAX_PATH]{};const char* avatarUri=job.bytes&&workerBody[0]=='d'?workerBody:nullptr;if(!Report(report,job.career,avatarUri)){data.status=3;return;}wcscpy_s(path,report);}
 else {
  if(!MainUiExternalValid(job.kind,job.parameter)||!Utf8(job.parameter,path,(int)_countof(path))){data.status=2;return;}
  if(job.kind==1&&!(path[0]&&path[1]==':')&&wcsncmp(path,L"\\\\",2)){wchar_t relative[MAX_PATH*2]{};wcscpy_s(relative,path);if(swprintf_s(path,L"%s\\%s",root,relative)<0){data.status=2;return;}}
 }
 if((job.kind==1||job.kind==3||job.kind==4)&&GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES){data.status=3;return;}
 HRESULT com=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED|COINIT_DISABLE_OLE1DDE);
 SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);info.fMask=SEE_MASK_FLAG_NO_UI|SEE_MASK_ASYNCOK;info.lpVerb=L"open";info.lpFile=path;info.lpDirectory=root;info.nShow=SW_SHOWNORMAL;
 data.status=ShellExecuteExW(&info)?1:3;if(SUCCEEDED(com))CoUninitialize();
}
bool Endpoint(const char* text,sockaddr_storage* addr,int* length){
 if(!text||!*text||strlen(text)>=96)return false;
 char ip[96]{};strcpy_s(ip,text);unsigned long port=27015;
 auto number=[](const char* p,unsigned long* value,unsigned long max){if(!p||!*p)return false;for(const char* q=p;*q;++q)if(*q<'0'||*q>'9')return false;char* end=nullptr;unsigned long n=strtoul(p,&end,10);if(*end||!n||n>max)return false;*value=n;return true;};
 char* query=strchr(ip,'@');unsigned long queryPort=0;if(query){*query++=0;if(!number(query,&queryPort,65535))return false;}
 char* host=ip;bool ipv6=false;
 if(ip[0]=='['){host=ip+1;char* close=strchr(host,']');if(!close)return false;*close=0;ipv6=true;if(close[1]&&(close[1]!=':'||!number(close+2,&port,65535)))return false;}
 else {char* colon=strchr(ip,':');ipv6=colon&&strchr(colon+1,':');if(colon&&!ipv6){*colon=0;if(!number(colon+1,&port,65535))return false;}}
 if(queryPort)port=queryPort;memset(addr,0,sizeof(*addr));
 if(ipv6){auto* v6=(sockaddr_in6*)addr;v6->sin6_family=AF_INET6;v6->sin6_port=htons((u_short)port);char* scope=strchr(host,'%');if(scope){*scope++=0;unsigned long id=0;if(!number(scope,&id,0xffffffffUL))return false;v6->sin6_scope_id=id;}*length=sizeof(*v6);return InetPtonA(AF_INET6,host,&v6->sin6_addr)==1;}
 auto* v4=(sockaddr_in*)addr;v4->sin_family=AF_INET;v4->sin_port=htons((u_short)port);*length=sizeof(*v4);return InetPtonA(AF_INET,host,&v4->sin_addr)==1;
}

void QueryPump(){
 if(!visible.load()||!queryWanted.load()){CloseQuery();return;}
 DWORD now=GetTickCount();
 if(socketHandle!=INVALID_SOCKET){
  unsigned char packet[4096]{};int n=recv(socketHandle,(char*)packet,sizeof(packet),0);
  if(n>=9&&packet[0]==255&&packet[1]==255&&packet[2]==255&&packet[3]==255&&packet[4]==0x41&&challengeCount++==0){challengePing=(int)(now-sentAt);memcpy(request+25,packet+5,4);send(socketHandle,(const char*)request,29,0);}
  else if(n>0){MainUiServerStatus result=data.queries[active];if(MainUiA2sParse(packet,n,&result)){result.state=1;result.ping=challengeCount?challengePing:(int)(now-sentAt);result.checked=Now();data.queries[active]=result;CloseQuery();Publish();}}
  if(socketHandle!=INVALID_SOCKET&&(LONG)(now-sentAt)>=1200){data.queries[active].state=2;data.queries[active].checked=Now();CloseQuery();Publish();}
  return;
 }
 if(!targetCount)return;
 if(nextTarget>=targetCount){nextTarget=0;cycleAt=now+15000;return;}
 if((LONG)(now-cycleAt)<0)return;
 active=nextTarget++;sockaddr_storage addr{};int addressLength=0;
 if(!Endpoint(targets[active],&addr,&addressLength)){data.queries[active].state=3;active=-1;Publish();return;}
 socketHandle=socket(addr.ss_family,SOCK_DGRAM,IPPROTO_UDP);if(socketHandle==INVALID_SOCKET){active=-1;return;}
 u_long nonblock=1;if(ioctlsocket(socketHandle,FIONBIO,&nonblock)||connect(socketHandle,(sockaddr*)&addr,addressLength)){CloseQuery();return;}
 memset(request,255,4);request[4]=0x54;memcpy(request+5,"Source Engine Query",20);challengeCount=0;challengePing=0;sentAt=now;
 if(send(socketHandle,(const char*)request,25,0)!=25){CloseQuery();return;}
 data.queries[active].state=4;Publish();
}
} // namespace

bool MainUiExternalValid(int kind,const char* s){
 size_t n=s?strlen(s):0;if(!n||n>=384)return false;for(size_t i=0;i<n;++i)if((unsigned char)s[i]<32||s[i]==127||s[i]=='"')return false;
 wchar_t text[384]{};if(!Utf8(s,text,384))return false;
 if(kind==0)return (!_strnicmp(s,"https://",8)&&s[8])||(!_strnicmp(s,"http://",7)&&s[7]);
 if(kind==2)return !_strnicmp(s,"steam://",8)&&s[8];
 return kind==1&&s[0]!=' '&&strncmp(s,"\\\\",2)&&strstr(s,"://")==nullptr;
}
bool MainUiA2sParse(const unsigned char* p,int n,MainUiServerStatus* out){
 if(!p||!out||n<6||p[0]!=255||p[1]!=255||p[2]!=255||p[3]!=255||p[4]!=0x49)return false;
 int at=6;auto string=[&](char* dst,int cap){int start=at;while(at<n&&p[at])++at;if(at>=n)return false;
  if(dst){int length=at-start;if(length>=cap)length=cap-1;memcpy(dst,p+start,length);dst[length]=0;Clean(dst,cap);}++at;return true;};
 MainUiServerStatus result=*out;
 if(!string(result.name,192)||!string(result.map,96)||!string(nullptr,0)||!string(nullptr,0)||at+9>n)return false;
 unsigned app=p[at]|(p[at+1]<<8);if(app!=550)return false;at+=2;
 result.players=p[at++];result.maxPlayers=p[at++];result.bots=p[at++];at+=4;
 if(!string(nullptr,0)||result.players>result.maxPlayers||result.bots>result.players)return false;*out=result;return true;
}
void MainUiServicesInit(const char* gameRoot){
 // SkeetoGameRoot is GetModuleFileNameA-derived (ACP); user action parameters are UTF-8.
 if(!gameRoot||!MultiByteToWideChar(CP_ACP,0,gameRoot,-1,root,MAX_PATH)||swprintf_s(directory,L"%s\\left4dead2\\skeeto\\mainmenu",root)<0){root[0]=directory[0]=0;return;}
 accepting=true;
}

int MainUiServicesReadCurrent(char* body,int cap){
 if(!body||cap<kMainUiProfileBytes)return -1;wchar_t path[MAX_PATH]{};if(!DataPath(L"current.hud",path))return -1;
 if(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES)return GetLastError()==ERROR_FILE_NOT_FOUND||GetLastError()==ERROR_PATH_NOT_FOUND?0:-1;
 FILE* f=_wfopen(path,L"rb");if(!f)return -1;size_t n=fread(body,1,cap-1,f);bool ok=!ferror(f)&&fgetc(f)==EOF&&!memchr(body,0,n);fclose(f);body[n]=0;return ok?1:-1;
}
bool MainUiServicesSaveCurrent(const char* body){if(!body)return false;size_t n=strnlen(body,kMainUiProfileBytes);if(n>=kMainUiProfileBytes)return false;Job j{};j.type=7;return Post(j,body,n+1);}
void MainUiServicesMarkCurrentInvalid(){currentInvalid=true;}
void SaveCurrent(const Job& job){
 wchar_t path[MAX_PATH]{};if(!DataPath(L"current.hud",path)){data.status=3;return;}
 if(currentInvalid.load()){
  DWORD attributes=GetFileAttributesW(path);
  if(attributes!=INVALID_FILE_ATTRIBUTES){bool copied=false;
   for(int i=0;i<16&&!copied;++i){wchar_t backup[MAX_PATH]{};int n=i?swprintf_s(backup,L"%s.bad.%u.%d",path,GetTickCount(),i):swprintf_s(backup,L"%s.bad",path);
    if(n<0)break;if(CopyFileW(path,backup,TRUE))copied=true;else if(GetLastError()!=ERROR_FILE_EXISTS&&GetLastError()!=ERROR_ALREADY_EXISTS)break;}
   if(!copied){data.status=3;return;}
  }else if(GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND){data.status=3;return;}
  currentInvalid=false;
 }
 data.status=AtomicWrite(path,workerBody,job.bytes?job.bytes-1:0)?1:3;
}
void MainUiServicesSetWanted(bool home,bool query){visible.store(home);queryWanted.store(query);}
bool MainUiServicesCopy(MainUiHubSnapshot* out){if(!out||!TryAcquireSRWLockShared(&lock))return false;*out=published;ReleaseSRWLockShared(&lock);return true;}
bool MainUiServicesRecent(const MainUiServerRecord& row){Job j{};j.type=1;j.server=row;return Post(j);}
bool MainUiServicesBattle(const MainUiBattleRecord& row){Job j{};j.type=2;j.battle=row;return Post(j);}
bool MainUiServicesQuery(const char* addresses,int n){if(!addresses||n<0||n>kMainUiQueryMax)return false;Job j{};j.type=3;j.kind=n;return Post(j,addresses,n*96);}
bool MainUiServicesRefresh(){Job j{};j.type=4;return Post(j);}
bool MainUiServicesOpen(int kind,const char* text){if(kind<0||kind>4||(kind<3&&!MainUiExternalValid(kind,text)))return false;Job j{};j.type=5;j.kind=kind;if(text)strncpy_s(j.parameter,text,_TRUNCATE);return Post(j);}
bool MainUiServicesOpenReport(const MainUiCareerReport& career,const MainUiBattleLabel* labels,int labelCount){Job j{};j.type=5;j.kind=4;j.career=career;if(labels&&labelCount>0){if(labelCount>10)labelCount=10;j.labelCount=labelCount;memcpy(j.labels,labels,sizeof(MainUiBattleLabel)*labelCount);}const size_t n=strnlen(g_mainUiAvatarUri,kMainUiAvatarUriChars);if(n&&n<kMainUiAvatarUriChars&&g_mainUiAvatarUri[0]=='d')return Post(j,g_mainUiAvatarUri,n+1);return Post(j);}
bool MainUiServicesProfile(int kind,const char* text,const char* body){if(kind<0||kind>3||((kind==0||kind==3)&&!body)||((kind==2||kind==3)&&!ProfileLeaf(text)))return false;Job j{};j.type=6;j.kind=kind;j.generation=profileGeneration.load();if(text)strncpy_s(j.parameter,text,_TRUNCATE);if(body){size_t n=strnlen(body,kMainUiProfileBytes);if(n>=kMainUiProfileBytes)return false;return Post(j,body,n+1);}return Post(j);}
bool MainUiServicesTakeProfile(char* body,int cap){if(!body||cap<kMainUiProfileBytes||!TryAcquireSRWLockExclusive(&lock))return false;bool ok=preparedReady&&preparedGeneration==profileGeneration.load();if(ok)memcpy(body,prepared,strlen(prepared)+1);preparedReady=false;ReleaseSRWLockExclusive(&lock);return ok;}
void MainUiServicesCancelProfiles(){profileGeneration.fetch_add(1);}
void MainUiServicesWorkerPump(){
 if(!directory[0])return;PollProfileDialog();if(!visible.load()&&!pending.load()){CloseQuery();return;}
 if(visible.load())EnsureData();
 Job job{};if(Pop(job)){
  if(job.type!=8)EnsureData();
  if(Record(job))Save();
  else if(job.type==3){CloseQuery();int n=job.kind;bool same=n==targetCount&&!memcmp(targets,workerBody,n*96);nextTarget=0;cycleAt=GetTickCount();if(!same){memset(targets,0,sizeof(targets));memcpy(targets,workerBody,n*96);targetCount=n;data.queryCount=n;memset(data.queries,0,sizeof(data.queries));for(int i=0;i<n;++i){targets[i][95]=0;strcpy_s(data.queries[i].address,targets[i]);}}}
  else if(job.type==4)ScanProfiles();else if(job.type==5)Open(job);else if(job.type==6)Profile(job);
  else if(job.type==8&&job.generation==activeTextTicket.load()){if(!StartProfileDialog(job))PublishText(job,MainUiTextResult::Failed);}
  else if(job.type==7)SaveCurrent(job);Publish();
 }
 if(visible.load()&&queryWanted.load()){if(!network){WSADATA w{};network=WSAStartup(MAKEWORD(2,2),&w)==0;}if(network)QueryPump();}else CloseQuery();
}
void MainUiServicesShutdown(){
 accepting=false;activeTextTicket=0;CancelProfileDialog();PollProfileDialog();visible=false;queryWanted=false;profileGeneration.fetch_add(1);CloseQuery();
 // Flush at most four copied records; never launch queued dialogs/programs during exit.
 Job job{};bool changed=false;for(int i=0;i<4&&Pop(job);++i){if(job.type==1||job.type==2||job.type==7)EnsureData();if(Record(job))changed=true;else if(job.type==7)SaveCurrent(job);}
 if(changed&&loaded)Save();if(network){WSACleanup();network=false;}
}

#ifdef SKEETO_MAINUI_SERVICE_TEST
void MainUiServicesTestDialogGate(HANDLE gate){dialogTestGate=gate;}
bool MainUiServicesTestDialogActive(){return profileDialog.thread!=nullptr;}
#endif

MainUiTextRequest MainUiServicesText(const wchar_t* seed,const char* caption,int limit){
 if(!seed||!caption||limit<1||limit>383||wcsnlen(seed,384)>(size_t)limit||strlen(caption)>=512||dialogBusy.load()||!TryAcquireSRWLockExclusive(&lock))return 0;
 if(activeTextTicket.load()||!accepting.load()){ReleaseSRWLockExclusive(&lock);return 0;}
 if(++nextTextTicket==0)++nextTextTicket;unsigned ticket=nextTextTicket;activeTextTicket=ticket;textResult=MainUiTextResult::None;textReadyTicket=0;
 ReleaseSRWLockExclusive(&lock);Job job{};job.type=8;job.generation=ticket;job.textLimit=limit;strcpy_s(job.parameter,caption);
 if(Post(job,seed,(wcslen(seed)+1)*sizeof(wchar_t)))return ticket;
 activeTextTicket.compare_exchange_strong(ticket,0);return 0;
}
MainUiTextResult MainUiServicesTakeText(MainUiTextRequest request,wchar_t* text,int cap){
 if(!request||!text||cap<1||!TryAcquireSRWLockExclusive(&lock))return MainUiTextResult::None;
 MainUiTextResult result=MainUiTextResult::None;
 if(textReadyTicket==request&&activeTextTicket.compare_exchange_strong(request,0)){
  result=textResult;if(result==MainUiTextResult::Accepted){if(wcslen(completedText)>=(size_t)cap)result=MainUiTextResult::Failed;else wcscpy_s(text,cap,completedText);}textReadyTicket=0;textResult=MainUiTextResult::None;
 }
 ReleaseSRWLockExclusive(&lock);return result;
}
void MainUiServicesCancelText(MainUiTextRequest request){if(request)activeTextTicket.compare_exchange_strong(request,0);}

bool MainUiServicesTextOwnsWindow(MainUiTextRequest request,std::uintptr_t window){
 return request&&window&&request==activeTextTicket.load()&&request==dialogTextTicket.load()&&reinterpret_cast<HWND>(window)==profileDialog.window.load();
}
