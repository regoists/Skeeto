// Homepage services: bounded snapshots/jobs; worker owns OS/network/file work.
// Game/Paint supplies copied data only. No Source interface is used by worker.
#pragma once
#include <cstdint>
#include "skeeto_career.h"
// Paint fills Steam identity before OpenReport; worker never calls Steam.
// Steam large friend avatar (184x184, 32-bpp BMP) as a data:image/bmp;base64 URI.
// One buffer, not a field on the queued job: four jobs would otherwise store
// four copies of this zeroed array in the DLL image.
constexpr int kMainUiAvatarUriChars = 180672;
extern char g_mainUiAvatarUri[kMainUiAvatarUriChars];
struct MainUiCareerReport {
 CareerArchiveSnapshot stats{};char weaponNames[6][96]{};
 char personaName[128]{};char steamId[32]{};
};
constexpr int kMainUiRecentMax=10, kMainUiQueryMax=16, kMainUiProfileMax=12;
constexpr int kMainUiProfileBytes=1048576;
struct MainUiServerRecord {char address[96]{},name[192]{},map[96]{}; uint64_t visited=0;};
struct MainUiServerStatus {char address[96]{},name[192]{},map[96]{};int state=0,players=0,maxPlayers=0,bots=0,ping=0;uint64_t checked=0;};
constexpr int kMainUiBattleMates=8;
// One squad member (survivor, bots included) copied from the chapter stats rows at freeze.
// Scores are chapter-stats hundredths (ChapStatsScoreTotal units): 1250 means 12.5 points.
struct MainUiBattleMate {
 char name[64]{}; int userid=0; int bot=0; int local=0; int score=0; int mvp=0;
 int stats[32]{}; int headshots[9]{};
};
// Only successful chapters (result==0) are stored; the personal fields summarise the local player.
struct MainUiBattleRecord {
 uint64_t time=0; char map[96]{},server[96]{},player[192]{},version[32]{};
 int result=0,seconds=0,score=0,scoreEnabled=0,pageMask=0,statCount=25,stats[32]{},headshots[9]{},weights[32]{};
 int mateCount=0; MainUiBattleMate mates[kMainUiBattleMates]{};
 char campaign[192]{},chapter[192]{},serverName[192]{};
};
// Names resolved on Paint for records that were saved before those fields existed.
struct MainUiBattleLabel {
 uint64_t time=0; char map[96]{},campaign[192]{},chapter[192]{},serverName[192]{};
};
struct MainUiHubSnapshot {
 unsigned revision=0;int recentCount=0,battleCount=0,queryCount=0,profileCount=0,status=0;
 MainUiServerRecord recent[10]{};MainUiBattleRecord battles[10]{};MainUiServerStatus queries[kMainUiQueryMax]{};
 char profiles[12][128]{};
};
// Init/Pump/Shutdown run on the existing Win32-only startup worker.
void MainUiServicesInit(const char* gameRoot);
int MainUiServicesReadCurrent(char* body,int cap); // Startup worker only: 0 absent, 1 read, -1 error.
bool MainUiServicesSaveCurrent(const char* body); // Paint posts copy; worker atomically replaces current.hud.
void MainUiServicesWorkerPump();
void MainUiServicesShutdown();
void MainUiServicesMarkCurrentInvalid(); // Startup failure: worker must preserve original before replacement.
void MainUiServicesSetWanted(bool visible,bool query);
bool MainUiServicesCopy(MainUiHubSnapshot* out); // Try-lock: keep old snapshot on contention.
bool MainUiServicesRecent(const MainUiServerRecord& row);
bool MainUiServicesBattle(const MainUiBattleRecord& row);
bool MainUiServicesQuery(const char* addresses,int count); // fixed kMainUiQueryMax x 96 copied slots
bool MainUiServicesRefresh();
bool MainUiServicesOpen(int kind,const char* parameter); // 0 web,1 path,2 Steam,3 changelog,4 HTML report
bool MainUiServicesOpenReport(const MainUiCareerReport& career, const MainUiBattleLabel* labels=nullptr, int labelCount=0); // Paint copies Steam cache; worker never calls Steam.
bool MainUiServicesProfile(int kind,const char* parameter,const char* body=nullptr); // 0 save dialog,1 import dialog,2 load,3 named save
bool MainUiServicesTakeProfile(char* body,int cap); // One completed import; game thread validates/applies.
void MainUiServicesCancelProfiles(); // Invalidate imports on disable/connection; exports still finish.
bool MainUiExternalValid(int kind,const char* parameter);
bool MainUiA2sParse(const unsigned char* packet,int bytes,MainUiServerStatus* out);

#ifdef SKEETO_MAINUI_SERVICE_TEST
void MainUiServicesTestDialogGate(void* gate);
bool MainUiServicesTestDialogActive();
#endif

// Standard edit control/IME on the existing short-lived dialog thread.
// Each editor/search owns its ticket; consuming/cancelling another ticket is inert.
using MainUiTextRequest = unsigned;
enum class MainUiTextResult { None, Accepted, Cancelled, Failed };
MainUiTextRequest MainUiServicesText(const wchar_t* seed,const char* caption,int limit=383);
MainUiTextResult MainUiServicesTakeText(MainUiTextRequest request,wchar_t* text,int cap);
void MainUiServicesCancelText(MainUiTextRequest request);

// True only for this live text request's native top-level dialog.
bool MainUiServicesTextOwnsWindow(MainUiTextRequest request,std::uintptr_t window);
