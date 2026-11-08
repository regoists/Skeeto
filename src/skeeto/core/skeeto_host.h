#pragma once
#include "skeeto_ui_text.h"
#include "skeeto_runtime.h"
#include <Windows.h>
#include <cstdint>
#include <cstddef>
#include <atomic>

// Shared kernel decls. Definitions stay in proxy. Not a toolbox.
// Do not add modules, HUD, xhair tables, or debug vars here.

struct PlayerInfo {
	char pad0[0x8];
	char name[32];
	int userid;
	char pad1[0x150];
};
static_assert(offsetof(PlayerInfo, pad1) == 44, "unexpected L4D2 PlayerInfo layout");
inline bool PlayerInfoIsBot(const PlayerInfo& pi) {
	return (unsigned char)pi.pad1[72] != 0;
}

extern void* g_engine;
extern void* g_entlist;
extern int g_offTeam, g_offHealth, g_offMaxHealth, g_offLifeState, g_offClip1;
extern int g_offZombieClass, g_offTankLookAtPlayer;
extern int g_offUpgradeBitVec, g_offUpgradedAmmo, g_offHasDualWeapons;
extern int g_offAmmoType, g_offMyWeapons, g_offAmmo, g_offHanging, g_offThirdStrike;
extern int g_offHealthBuffer, g_offHealthBufferTime, g_offClientIntensity;
extern int g_offTongueOwner, g_offPounceAttacker, g_offJockeyAttacker;
extern int g_offCarryAttacker, g_offPummelAttacker, g_offHangingTongue;
extern int g_offReviveOwner, g_offProgStart, g_offProgDur, g_offModelIndex;
extern int g_offSurvivorCharacter, g_offObserverMode, g_offIsIncapacitated;
extern bool g_menuVisible, g_menuParked;
extern int g_hudResolutionScalePct;
extern bool g_hudPostMissionQuiet;

bool HudMenuPage(int page);
bool HudWantsDraw(bool opt, int page);
void HudClampFont(int* font);
int HudFontPx(int base);
void HudPlaceAbs(int sw, int sh, int pw, int ph, int ox, int oy, int* x, int* y);
void HudNudgeAbs(bool* abs, int* ox, int* oy, int align, int dx, int dy);
void HudMatRebuildHot();
bool HudAnyCustomPaintWanted();
bool MenuEnsureSurf();
bool IdleSwapHudQuietActive();
bool IdleSwapDeferEntityHud(bool menuPreview);
bool ClientUxIsPlayerPawnClass(const char* name);

bool SurfGetScreenSize(int* sw, int* sh);
void SurfColor(int r, int g, int b, int a);
void SurfFill(int x0, int y0, int x1, int y1);
void SurfLine(int x0, int y0, int x1, int y1);
void SurfOutline(int x0, int y0, int x1, int y1);
void SurfTextAt(unsigned long font, int x, int y, COLORREF rgb, const wchar_t* text);
int SurfTextWFont(unsigned long font, const wchar_t* text);
unsigned long SurfAcquireVguiFont(int tall, int weight, int flags);
void SurfEnsureHudFont();
int SurfHudMat(const char* mat);
bool SurfHudMatSize(int id, int* tw, int* th);
bool SurfDrawTexturedUv(int x0, int y0, int x1, int y1, int id, float s0, float t0, float s1, float t1);
void SurfDrawTextured(int x0, int y0, int x1, int y1, int id);
void SurfDrawArc(int cx, int cy, int r, float a0, float a1, int segs, int rr, int gg, int bb, int aa);

void* EntGet(int i);
bool EntReadable(void* e);
void* EntNetworkable(int i);
int EntIndexFromHandle(int h);
void* EntFromHandle(int h);
int EntHighestIndex();
const char* EntNetClassName(int i);
int EngLocal();
bool EngInGame();
bool EngDrawingLoading();
bool EngConnected();
float EngLastTimeStamp();
float EngClientCurTime();
bool EngPlayerInfo(int ent, PlayerInfo* out);
void* CvarFind(const char* name);
bool CvarReadPair(void* var, int* nOut, float* fOut);
int CvarGetIntRange(const char* name, int lo, int hi, int fallback);
void CvarSetInt(const char* name, int value);
void FireFX(const char* name, float x, float y, float z);

constexpr int kHudFontMin = 10;
constexpr int kHudFontMax = 72;
constexpr int kHudFontStep = 2;
constexpr int kHudScaleMin = 50;
constexpr int kHudScaleMax = 200;
constexpr int kHudScaleStep = 5;
constexpr int kFontFlagAntiAlias = 0x010;
constexpr int kFontFlagDropShadow = 0x080;
constexpr int kFontFlagOutline = 0x200;
constexpr int kMenuPageWorldHud = 35;

constexpr int kBotNameCount = 8;
constexpr int kBotNameMaxUnits = 23;
constexpr int kHudMatMax = 256;
struct HudMatCache {
	char name[72];
	int id;
	int tw;
	int th;
	uint8_t probeMisses;
	bool retry;
	// Unused: LevelInit drops every id. Kept so cache records stay POD-stable.
	bool rebind;
};

void HudMatOnLevelInit();

extern bool g_worldHudRealism;
extern bool g_optWitchStartle;
extern bool g_optWorldRescue;
extern bool g_optWorldFriendlyFire;
extern bool g_optClientBotNames;

void ClampMapHud();
unsigned long SurfMakeYahei(int tall, int weight, int flags);
bool SurvivorGlowWanted();
bool ModelPathIsHuntRifle(const char* path);
bool WeaponEntIsHuntRifle(int wepIdx);

// Extract scar: HUD compile exports. Not a public API. Do not grow this block.
void ClampAmmoHud();
void ClampAmmoSegHud();
void ClampAmmoGfxHud();
bool ReloadProgressSample(bool* reloading, float* reloadFrac, float* fireRemainOut = nullptr);
bool HudOverlayGetAmmoArcConfig(int* offX, int* offY, int* scalePct, int* side, bool* preview);
bool HudOverlayGetReloadRingConfig(int* offX, int* offY, int* radius, bool* preview);
void HudOverlayFrameBegin();
void HudOverlaySetAmmoArc(int sw, int sh, int offX, int offY, int radius, int thick,
	float a0, float a1, float frac, int clip, int maxClip,
	int red, int green, int blue, bool preview);
void HudOverlaySetReloadRing(int sw, int sh, int offX, int offY, int radius, int thick,
	float frac, bool preview);
void HudOverlayFrameCommit(bool allowVisible);
void ClampCapHud();
void ClampReviveHud();
void ClampWitchStartleHud();
COLORREF DirectorTempoColor(int tempo);
const wchar_t* DirectorTempoName(int tempo);
bool DirectorTempoRead(int* outTempo);
int EngPlayerForUserID(int id);
bool EvBool(void* ev, const char* k, bool def = false);
float EvFloat(void* ev, const char* k, float def = 0.f);
int EvInt(void* ev, const char* k, int def = 0);
const char* EvStr(void* ev, const char* k, const char* def = "");
void* GetIface(const char* mod, const char* name);
int SurfCachedFontTall(unsigned long font);
unsigned long HudAcquireLayoutFont(int layoutPx, unsigned long* slot, int* slotPx, int weight);
int HudClampScale(int s);
void HudCornerBrackets(int x, int y, int w, int h, int len, COLORREF c, int alpha);
void HudFinalizePos(bool* abs, int* ox, int* oy, int align, int sw, int sh, int pw, int ph, int* x, int* y);
void HudGlassBody(int x, int y, int w, int h);
void HudIntensityColor(int iv, int* r, int* g, int* b);
bool HudLayoutPeer(bool opt, int page);
bool HudLayoutSubPage();
void HudPanelShadow(int x, int y, int w, int h);
int HudScalePx(int base, int scalePct);
void HudSegGauge(int x, int y, int w, int h, int pct, COLORREF fg, int segs);
int HudSmLayoutBase(int layoutMain);
void HudTagPill(int x, int y, unsigned long font, const wchar_t* text, COLORREF fg, COLORREF bg);
void HudTopAccent(int x, int y, int w, COLORREF c, int h = 2);
bool IfaceAlive(void* inst);
bool IsExec(void* p);
bool IsLocalAttacker(void* ev);
bool LocalEyePos(float* outEye);
float LocalPlayHeadLift(int zc);
bool LocalPlayWorldToScreen(const float* world, float* outSx, float* outSy, int sw, int sh);
bool PtrCommitted(const void* p);
bool SkeetoFeaturesOn();
void StatusHudNameId(wchar_t* out, int outN, const wchar_t* name, int uid, bool showUid = true);
void SurfEnsureSpeedTimerFonts();
void* VGet(void* inst, int idx);
int WepClipCapResolve(int wepIdx, int clip);
bool WeaponEntHasDualPistols(int index);
extern wchar_t g_clientBotNames[kBotNameCount][kBotNameMaxUnits + 1];
extern std::atomic<int> g_dbgLifeState;
extern std::atomic<int> g_dbgLocalTeam;
extern std::atomic<int> g_dbgObsMode;
extern char g_elimLastMap[64];
extern HudMatCache g_hudMat[kHudMatMax];
extern int g_hudMatN;
extern unsigned long g_hudSubFont;
extern int g_offActiveWeapon;
extern int g_offCustomAbility;
extern int g_offFlags;
extern int g_offIsAttemptingToPounce;
extern int g_offIsLunging;
extern int g_offOrigin;
extern int g_offViewOffset;
extern int g_offZombieClass;
extern bool g_optAmmoHudClipOnly;
extern bool g_optAmmoHudSecClip;
extern bool g_optElim;
extern bool g_optKillFeed;
extern int g_optKillFeedStyle; // 0 classic / 1 CS2
extern bool g_optKillFx;
extern bool g_optWitchStartleHud;
extern bool g_optWorldBileMark;
extern bool g_optWorldCrown;
extern bool g_optWorldLevel;
extern bool g_optWorldMeleeCrown;
extern bool g_optWorldMeleeSkeet;
extern bool g_optWorldSkeet;
extern bool g_optWorldTankKill;
extern bool g_optWorldTankSlayer;
extern bool g_optWorldTongueCut;
extern bool g_optWorldWitchKill;
extern bool g_optWorldWitchStartle;
extern bool g_optWorldBlackWhite;
// Chapter-stats special ops tracking (skeet / level) when that page is enabled.
bool ChapStatsSpecialTrackWanted();
constexpr int kXhairColorCount = 10;
static constexpr UiTextList<kXhairColorCount> kXhairColorNames = {{
	{UiId::ColorWhite, L"白"}, {UiId::ColorGreen, L"绿"}, {UiId::ColorCyan, L"青"},
	{UiId::ColorRed, L"红"}, {UiId::ColorYellow, L"黄"}, {UiId::ColorOrange, L"橙"},
	{UiId::ColorPink, L"粉"}, {UiId::ColorBlue, L"蓝"}, {UiId::ColorLime, L"黄绿"},
	{UiId::ColorMint, L"水绿"}
}};
static const COLORREF kXhairColorRgb[kXhairColorCount] = {
	RGB(255, 255, 255), RGB(0, 255, 0), RGB(0, 255, 255), RGB(255, 40, 40), RGB(255, 220, 0),
	RGB(255, 140, 0), RGB(255, 80, 180), RGB(80, 140, 255), RGB(180, 255, 0), RGB(0, 200, 120)
};
