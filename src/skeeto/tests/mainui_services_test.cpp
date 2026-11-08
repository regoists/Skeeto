#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <thread>
#include <random>
#include "../core/skeeto_mainui_services.h"

static std::vector<unsigned char> Packet(){
 std::vector<unsigned char> p={255,255,255,255,0x49,17};
 auto text=[&](const char* s){while(*s)p.push_back((unsigned char)*s++);p.push_back(0);};
 text("Skeeto test server");text("c1m1_hotel");text("left4dead2");text("Left 4 Dead 2");
 unsigned char tail[]={0x26,0x02,3,8,1,'d','w',0,1};p.insert(p.end(),tail,tail+sizeof(tail));text("2.2.4.3");return p;
}
static void Parser(){auto p=Packet();MainUiServerStatus s{};assert(MainUiA2sParse(p.data(),(int)p.size(),&s));assert(s.players==3&&s.maxPlayers==8&&s.bots==1&&!strcmp(s.map,"c1m1_hotel"));
 for(int i=0;i<(int)p.size();++i)assert(!MainUiA2sParse(p.data(),i,&s));
 auto invalid=p;invalid[4]=0x41;assert(!MainUiA2sParse(invalid.data(),(int)invalid.size(),&s));
 std::mt19937 random(42);for(int i=0;i<20000;++i){std::vector<unsigned char> bytes(random()%512);for(auto& b:bytes)b=(unsigned char)random();MainUiA2sParse(bytes.data(),(int)bytes.size(),&s);}
 assert(MainUiExternalValid(0,"https://steamcommunity.com/sharedfiles/filedetails/?id=3787239937"));assert(!MainUiExternalValid(0,"javascript:alert(1)"));assert(!MainUiExternalValid(0,"https://x\nquit"));
 assert(MainUiExternalValid(1,"left4dead2\\skeeto"));assert(MainUiExternalValid(1,"D:\\my files\\example.txt"));assert(!MainUiExternalValid(1,"\\\\server\\share"));assert(MainUiExternalValid(2,"steam://url/CommunityFilePage/3787239937"));
}
// Snapshots carry ten squad records (~40 KB); share one buffer so unoptimised test frames stay small.
static const MainUiHubSnapshot& Snapshot(){static MainUiHubSnapshot s;assert(MainUiServicesCopy(&s));return s;}
static void Network(int family,bool bracket=true){
 WSADATA w{};assert(WSAStartup(MAKEWORD(2,2),&w)==0);SOCKET server=socket(family,SOCK_DGRAM,IPPROTO_UDP);assert(server!=INVALID_SOCKET);
 sockaddr_storage addr{};int length=0;unsigned short port=0;
 if(family==AF_INET6){auto* v=(sockaddr_in6*)&addr;v->sin6_family=AF_INET6;v->sin6_addr=in6addr_loopback;length=sizeof(*v);}else{auto* v=(sockaddr_in*)&addr;v->sin_family=AF_INET;v->sin_addr.s_addr=htonl(INADDR_LOOPBACK);length=sizeof(*v);}
 assert(bind(server,(sockaddr*)&addr,length)==0);assert(getsockname(server,(sockaddr*)&addr,&length)==0);port=family==AF_INET6?ntohs(((sockaddr_in6*)&addr)->sin6_port):ntohs(((sockaddr_in*)&addr)->sin_port);DWORD timeout=2000;setsockopt(server,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
 std::thread responder([&]{unsigned char input[64]{};sockaddr_storage peer{};int size=sizeof(peer);int n=recvfrom(server,(char*)input,sizeof(input),0,(sockaddr*)&peer,&size);assert(n==25&&input[4]==0x54);unsigned char challenge[]={255,255,255,255,0x41,1,2,3,4};assert(sendto(server,(char*)challenge,sizeof(challenge),0,(sockaddr*)&peer,size)==9);
  n=recvfrom(server,(char*)input,sizeof(input),0,(sockaddr*)&peer,&size);assert(n==29&&!memcmp(input+25,challenge+5,4));auto p=Packet();assert(sendto(server,(char*)p.data(),(int)p.size(),0,(sockaddr*)&peer,size)==(int)p.size());});
 char targets[1][96]{};if(family==AF_INET6)sprintf_s(targets[0],bracket?"[::1]:27015@%d":"::1@%d",port);else sprintf_s(targets[0],"127.0.0.1:27015@%d",port);assert(MainUiServicesQuery((char*)targets,1));MainUiServicesSetWanted(true,true);
 DWORD begin=GetTickCount();MainUiHubSnapshot state{};do{MainUiServicesWorkerPump();state=Snapshot();if(state.queries[0].state==1)break;Sleep(5);}while(GetTickCount()-begin<2000);
 assert(state.queryCount==1&&state.queries[0].state==1&&state.queries[0].players==3);responder.join();
 MainUiServicesSetWanted(false,false);MainUiServicesWorkerPump();
 // Closing the view must send no further packets.
 u_long nonblock=1;ioctlsocket(server,FIONBIO,&nonblock);char input[64]{};assert(recv(server,input,sizeof(input),0)==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK);
 closesocket(server);WSACleanup();
}
static void TextRequests(const char* gameRoot){
 MainUiServicesInit(gameRoot);wchar_t output[384]{};
 auto first=MainUiServicesText(L"", "Skeeto search input test",64);assert(first);
 assert(!MainUiServicesText(L"other", "Skeeto competing editor test"));
 MainUiServicesCancelText(first+1);MainUiServicesWorkerPump();
 HWND window=nullptr;DWORD until=GetTickCount()+2000;
 while((LONG)(GetTickCount()-until)<0){window=FindWindowW(nullptr,L"Skeeto search input test");if(window&&GetDlgItem(window,100))break;Sleep(1);}assert(window);
 assert(SendDlgItemMessageW(window,100,EM_GETLIMITTEXT,0,0)==64);
 assert(MainUiServicesTextOwnsWindow(first,(uintptr_t)window));
 assert(!MainUiServicesTextOwnsWindow(first+1,(uintptr_t)window));
 assert(!MainUiServicesTextOwnsWindow(first,(uintptr_t)GetDesktopWindow()));
 assert(MainUiServicesTakeText(first+1,output,384)==MainUiTextResult::None);
 SetDlgItemTextW(window,100,L"持枪 镜像");SendMessageW(window,WM_COMMAND,IDOK,0);
 MainUiTextResult result=MainUiTextResult::None;until=GetTickCount()+2000;
 while((LONG)(GetTickCount()-until)<0){MainUiServicesWorkerPump();result=MainUiServicesTakeText(first,output,384);if(result!=MainUiTextResult::None)break;Sleep(1);}
 assert(result==MainUiTextResult::Accepted&&!wcscmp(output,L"持枪 镜像"));
 auto second=MainUiServicesText(L"kept", "Skeeto cancelled input test");assert(second&&second!=first);MainUiServicesWorkerPump();until=GetTickCount()+2000;window=nullptr;
 while((LONG)(GetTickCount()-until)<0){window=FindWindowW(nullptr,L"Skeeto cancelled input test");if(window&&GetDlgItem(window,100))break;Sleep(1);}assert(window);
 MainUiServicesCancelText(first);assert(MainUiServicesTakeText(first,output,384)==MainUiTextResult::None);
 SendMessageW(window,WM_COMMAND,IDCANCEL,0);until=GetTickCount()+2000;
 while((LONG)(GetTickCount()-until)<0){MainUiServicesWorkerPump();result=MainUiServicesTakeText(second,output,384);if(result!=MainUiTextResult::None)break;Sleep(1);}assert(result==MainUiTextResult::Cancelled);
 auto queued=MainUiServicesText(L"discard", "Skeeto never opened input");assert(queued);MainUiServicesCancelText(queued);MainUiServicesWorkerPump();assert(!FindWindowW(nullptr,L"Skeeto never opened input"));
 auto pending=MainUiServicesText(L"", "Skeeto shutdown input");assert(pending);MainUiServicesShutdown();assert(!FindWindowW(nullptr,L"Skeeto shutdown input"));
 char path[MAX_PATH]{};sprintf_s(path,"%s\\left4dead2\\skeeto\\mainmenu",gameRoot);assert(GetFileAttributesA(path)==INVALID_FILE_ATTRIBUTES);
 puts("PASS: text tickets isolate editors/search, wrong cancellation is inert, Unicode accepted, 64-char input, cancellation delivered, no main-menu files created");
}
int main(int argc,char** argv){assert(argc==3);if(!strcmp(argv[2],"text")){TextRequests(argv[1]);return 0;}Parser();MainUiServicesInit(argv[1]);MainUiServicesSetWanted(true,false);MainUiServicesWorkerPump();
 if(!strcmp(argv[2],"write")){
  for(int i=0;i<12;++i){MainUiServerRecord row{};sprintf_s(row.address,"192.0.2.%d:27015",i+1);sprintf_s(row.name,"Server %d",i+1);assert(MainUiServicesRecent(row));MainUiServicesWorkerPump();}
  auto s=Snapshot();assert(s.recentCount==10&&!strcmp(s.recent[0].address,"192.0.2.12:27015"));auto row=s.recent[4];assert(MainUiServicesRecent(row));MainUiServicesWorkerPump();s=Snapshot();assert(s.recentCount==10&&!strcmp(s.recent[0].address,row.address));
  for(int i=0;i<12;++i){MainUiBattleRecord b{};b.score=i;b.seconds=100;strcpy_s(b.map,"c1m1_hotel");strcpy_s(b.player,"<player&name>");b.stats[0]=12;b.mateCount=2;strcpy_s(b.mates[0].name,"me");strcpy_s(b.mates[1].name,"Bot\tNick");b.mates[1].bot=1;b.mates[1].mvp=1;b.mates[1].stats[0]=77;assert(MainUiServicesBattle(b));MainUiServicesWorkerPump();}
  assert(Snapshot().battleCount==10&&Snapshot().battles[0].score==11&&Snapshot().battles[0].mates[1].stats[0]==77&&!strcmp(Snapshot().battles[0].mates[1].name,"Bot Nick"));
  // Failures are never archived: mission_lost/disconnect rows are rejected by the worker.
  {MainUiBattleRecord failed{};failed.result=1;failed.score=999;assert(MainUiServicesBattle(failed));MainUiServicesWorkerPump();failed.result=2;assert(MainUiServicesBattle(failed));MainUiServicesWorkerPump();assert(Snapshot().battleCount==10&&Snapshot().battles[0].score==11);}
  assert(MainUiServicesProfile(3,"test.hud","SKEETO_MAINMENU_UI=1\nenabled=1\n"));MainUiServicesWorkerPump();assert(Snapshot().profileCount==1);
  assert(!MainUiServicesProfile(3,"../bad.hud","x"));assert(MainUiServicesProfile(2,"test.hud"));MainUiServicesCancelProfiles();MainUiServicesWorkerPump();static char body[kMainUiProfileBytes]{};assert(!MainUiServicesTakeProfile(body,sizeof(body)));
  assert(MainUiServicesProfile(2,"test.hud"));MainUiServicesWorkerPump();assert(MainUiServicesTakeProfile(body,sizeof(body))&&!strcmp(body,"SKEETO_MAINMENU_UI=1\nenabled=1\n"));assert(!MainUiServicesTakeProfile(body,sizeof(body)));
  MainUiServicesSetWanted(false,false);for(int i=0;i<4;++i)assert(MainUiServicesRefresh());assert(!MainUiServicesRefresh());for(int i=0;i<4;++i)MainUiServicesWorkerPump();
  HANDLE dialogGate=CreateEventW(nullptr,TRUE,FALSE,nullptr);assert(dialogGate);MainUiServicesTestDialogGate(dialogGate);
  assert(MainUiServicesProfile(0,nullptr,"SKEETO_MAINMENU_UI=1\nenabled=1\n"));MainUiServicesWorkerPump();assert(MainUiServicesTestDialogActive());
  // Held dialog thread must not block shared IO, copied state or UDP service.
  unsigned revision=Snapshot().revision;assert(MainUiServicesRefresh());MainUiServicesWorkerPump();assert(Snapshot().revision>revision);
  Network(AF_INET);Network(AF_INET6);Network(AF_INET6,false); // includes actual challenge/response while the dialog is held.
  assert(MainUiServicesTestDialogActive());
  SetEvent(dialogGate);DWORD dialogEnd=GetTickCount()+2000;while(MainUiServicesTestDialogActive()&&(LONG)(GetTickCount()-dialogEnd)<0){MainUiServicesWorkerPump();Sleep(1);}assert(!MainUiServicesTestDialogActive());CloseHandle(dialogGate);MainUiServicesTestDialogGate(nullptr);
  // Expanded file limit: queue owns the payload even if the producer changes it.
  static char large[kMainUiProfileBytes]{};memset(large,'x',sizeof(large)-1);memcpy(large,"SKEETO_MAINMENU_UI=1\n",21);assert(MainUiServicesSaveCurrent(large));large[0]='Y';MainUiServicesWorkerPump();
  assert(MainUiServicesReadCurrent(body,sizeof(body))==1&&body[0]=='S'&&strlen(body)==sizeof(large)-1);
  assert(!MainUiServicesProfile(3,"missing_payload.hud",nullptr));
  assert(MainUiServicesSaveCurrent("broken-mainmenu-data"));MainUiServicesWorkerPump();MainUiServicesMarkCurrentInvalid();
  assert(MainUiServicesSaveCurrent("SKEETO_MAINMENU_UI=1\nenabled=1\n"));MainUiServicesWorkerPump();
  char badPath[MAX_PATH]{};sprintf_s(badPath,"%s\\left4dead2\\skeeto\\mainmenu\\current.hud.bad",argv[1]);FILE* backup=nullptr;assert(!fopen_s(&backup,badPath,"rb")&&backup);char original[64]{};assert(fread(original,1,sizeof(original)-1,backup)>0);fclose(backup);assert(!strcmp(original,"broken-mainmenu-data"));
  assert(MainUiServicesSaveCurrent("SKEETO_MAINMENU_UI=1\nenabled=0\n"));
  MainUiBattleRecord last{};last.score=123;assert(MainUiServicesBattle(last));
  assert(MainUiServicesProfile(2,"missing.hud")); // No file task should execute at shutdown.
 }else if(!strcmp(argv[2],"read")){static char current[kMainUiProfileBytes]{};assert(MainUiServicesReadCurrent(current,sizeof(current))==1&&!strcmp(current,"SKEETO_MAINMENU_UI=1\nenabled=0\n"));auto s=Snapshot();assert(s.recentCount==10&&s.battleCount==10&&s.profileCount==1&&s.battles[0].score==123&&s.battles[1].mateCount==2&&s.battles[1].mates[1].mvp==1&&s.battles[1].mates[1].stats[0]==77);}
 if(!strcmp(argv[2],"read")){
  auto request=MainUiServicesText(L"seed", "Skeeto text test");assert(request);MainUiServicesWorkerPump();
  HWND inputWindow=nullptr;DWORD textEnd=GetTickCount()+2000;
  while((LONG)(GetTickCount()-textEnd)<0){inputWindow=FindWindowW(nullptr,L"Skeeto text test");if(inputWindow&&GetDlgItem(inputWindow,100))break;Sleep(1);}assert(inputWindow&&GetDlgItem(inputWindow,100));
  assert(SetDlgItemTextW(inputWindow,100,L"中文 ABC Mixed"));SendMessageW(inputWindow,WM_COMMAND,IDOK,0);
  wchar_t text[384]{};bool ready=false;textEnd=GetTickCount()+2000;
  while((LONG)(GetTickCount()-textEnd)<0){MainUiServicesWorkerPump();if(MainUiServicesTakeText(request,text,384)==MainUiTextResult::Accepted){ready=true;break;}Sleep(1);}assert(ready&&!wcscmp(text,L"中文 ABC Mixed"));assert(MainUiServicesTakeText(request,text,384)==MainUiTextResult::None);
  // A queued input cancelled before starting cannot create a window or publish text.
  request=MainUiServicesText(L"discard", "Skeeto cancelled text test");assert(request);MainUiServicesCancelText(request);MainUiServicesWorkerPump();assert(!FindWindowW(nullptr,L"Skeeto cancelled text test"));assert(MainUiServicesTakeText(request,text,384)==MainUiTextResult::None);
 }
 HANDLE shutdownGate=nullptr;
 if(!strcmp(argv[2],"read")){shutdownGate=CreateEventW(nullptr,TRUE,FALSE,nullptr);assert(shutdownGate);MainUiServicesTestDialogGate(shutdownGate);
 assert(MainUiServicesProfile(0,nullptr,"SKEETO_MAINMENU_UI=1\nenabled=1\n"));MainUiServicesWorkerPump();assert(MainUiServicesTestDialogActive());}
 MainUiServicesShutdown();assert(!MainUiServicesRefresh());
 // Shutdown cancels a held dialog without waiting for UI or abandoning its context.
 DWORD end=GetTickCount()+2000;while(MainUiServicesTestDialogActive()&&(LONG)(GetTickCount()-end)<0){MainUiServicesWorkerPump();Sleep(1);}assert(!MainUiServicesTestDialogActive());
 if(shutdownGate)CloseHandle(shutdownGate);MainUiServicesTestDialogGate(nullptr);
 puts("PASS: malformed/truncated/fuzzed A2S, IPv4/IPv6 loopback challenge handshake, bounded MRU/report retention, persistence restart, profile IO/cancellation, queue cap, network stop, shutdown record flush, held-dialog shutdown cancellation");
}
