#pragma once
#include "skeeto_host.h"

// Public nest API + nest types. Internals stay as protos in skeeto_hud.cpp.
// Menu / Save/Load / Wanted aggregation stay in proxy.

// Tracked remote survivors (world names / world HP / bile / idle links).
constexpr int kTeamHudMax = 16;
// On-screen teammate blood-bar cards and their layout slots.
constexpr int kTeamHudCardMax = 8;
// L4D2 survivor character indices (portraits / glow): Nick..Ellis.
constexpr int kTeamHudCharMax = 8;
enum TeamWep : int {
	TW_None = 0,
	TW_Pistol, TW_Magnum, TW_Melee, TW_Saw,
	TW_Smg, TW_SmgS, TW_Mp5,
	TW_Pump, TW_Chrome, TW_Auto, TW_Spas,
	TW_M16, TW_Ak, TW_Scar, TW_Sg, TW_M60,
	TW_Hunt, TW_Mil, TW_Scout, TW_Awp, TW_GL,
	TW_Molly, TW_Pipe, TW_Bile,
	TW_Kit, TW_Defib, TW_Exp, TW_Inc,
	TW_Pills, TW_Adren,
	TW_Carry,
	TW_Count
};

constexpr int kTeamHudSelSelf = kTeamHudCardMax;
constexpr int kTeamHudShowAll = 0;
constexpr int kTeamHudShowSelf = 1;

constexpr int kTeamHudFontMin = kHudFontMin;
constexpr int kTeamHudFontMax = kHudFontMax;
constexpr int kTeamHudStyleCount = 7;
constexpr int kTeamHudScaleMin = kHudScaleMin;
constexpr int kTeamHudScaleMax = kHudScaleMax;
constexpr int kTeamHudScaleStep = kHudScaleStep;

struct WorldNoticeState {
	int ent = 0;
	DWORD born = 0;
	DWORD until = 0;
};
struct WorldInfectedNoticeState {
	float world[3]{};
	DWORD born = 0;
	DWORD until = 0;
};
struct WorldBileNoticeState {
	int ent = 0;
	DWORD born = 0;
};
struct TeamHudSlot {
	bool present;
	bool isLocal;
	int ent;
	void* sampledPawn; // identity guard for nameplate projection until the next gather
	int character;
	int hp;
	int maxHp;
	int tempHp;
	bool dead;
	bool incap;
	bool hanging;
	bool third;    // m_bIsOnThirdStrike: second revive done, i.e. black and white
	int cap;       // 0 none, 1 smoker, 2 hunter, 3 jockey, 4 charger carry, 5 charger pummel
	float revive;  // <0 none; 0..1 progress; >1 indeterminate
	float actionProgress; // own generic progress bar: heal/revive/hold-E interaction
	float actionRemain;
	int userid;
	int capAtk;
	int capAtkUserId;
	wchar_t capAtkName[24];
	int reviveBy;
	int reviveByUserId;
	wchar_t reviveByName[24];
	int wep;
	int side;
	int clip;
	int ammo;
	int sideClip; // secondary gun magazine (-1 if none / melee)
	int priWepIdx;
	int sideWepIdx;
	int nade;
	bool nadeValid;
	int kit;
	int pills;
	int intensity;
	char melee[24];
	wchar_t name[24];
};
struct TeamHudNameCacheEntry {
	int ent;
	int userid;
	wchar_t name[24];
	bool fromPlayer;
};
struct TeamHudSprite {
	const char* sheet;
	short x, y, w, h;
};
struct ModelNameCacheEntry {
	int idx;
	char name[80];
};
struct TeamHudGearCache {
	void* pawn;
	DWORD nextReadAt;
	TeamHudSlot gear;
};

extern int g_teamHudFont, g_teamHudSel, g_teamHudStyle, g_teamHudScale, g_teamHudShow;
extern int g_teamHudMaxOthers; // 1..kTeamHudCardMax teammate cards when not self-only
extern int g_teamHudX[kTeamHudCardMax];
extern int g_teamHudY[kTeamHudCardMax];
extern int g_selfHudX, g_selfHudY;
extern TeamHudSlot g_teamHudSlots[kTeamHudMax];
extern TeamHudSlot g_teamHudLocal;
extern WorldNoticeState g_worldFfNotice, g_worldWitchNotice, g_worldTankNotice, g_worldRescueNotice;
extern WorldInfectedNoticeState g_worldMeleeSkeetNotice, g_worldLevelNotice, g_worldSkeetNotice;
extern WorldInfectedNoticeState g_worldTongueCutNotice, g_worldCrownNotice, g_worldTankSlayerNotice;
extern int g_worldCrownKind;
extern WorldBileNoticeState g_worldBileNotices[kTeamHudMax];
extern int g_worldControlledBy;

extern bool g_worldBileAnyActive;
extern WorldNoticeState g_worldWitchStartleNotice[kTeamHudMax];
extern bool g_worldWitchStartleAny;
extern unsigned long g_worldKillFont;
extern int g_worldKillFontTall;

extern int g_teamHudPxOverride;
extern int g_mapHudAlign, g_mapHudOffX, g_mapHudOffY, g_mapHudFont, g_mapHudScale;
extern bool g_mapHudAbs;
extern unsigned long g_mapHudFontId, g_mapHudFontSmId;
extern int g_mapHudFontTall, g_mapHudFontSmTall;
extern char g_mapHudLayoutStem[80];
extern int g_mapHudLayoutScale, g_mapHudLayoutFontBase, g_mapHudLayoutFont;

extern int g_capHudAlign, g_capHudOffX, g_capHudOffY, g_capHudFont, g_capHudScale;
extern bool g_capHudAbs;
extern int g_revHudAlign, g_revHudOffX, g_revHudOffY, g_revHudFont, g_revHudScale;
extern bool g_revHudAbs;
extern int g_tankTargetHudAlign, g_tankTargetHudOffX, g_tankTargetHudOffY;
extern int g_tankTargetHudFont, g_tankTargetHudScale;
extern bool g_tankTargetHudAbs;
extern int g_optInteractHudAlign, g_interactHudOffX, g_interactHudOffY;
extern int g_interactHudFont, g_interactHudScale;
extern bool g_optInteractHudAbs;
extern int g_witchStartleHudAlign, g_witchStartleHudOffX, g_witchStartleHudOffY;
extern int g_witchStartleHudFont, g_witchStartleHudScale;
extern bool g_witchStartleHudAbs;
extern unsigned long g_witchStartleFontId;
extern int g_witchStartleFontTall;

extern bool g_ammoHudAbs;
extern int g_ammoHudOffX, g_ammoHudOffY, g_ammoHudFont, g_ammoHudScale;
extern unsigned long g_ammoFontId, g_ammoFontSmId;
extern int g_ammoFontTall, g_ammoFontSmTall;
extern int g_itemHudAlign, g_itemHudOffX, g_itemHudOffY, g_itemHudScale;
extern bool g_itemHudAbs;

extern bool g_optTeamHud, g_optTeamWorldHud, g_optReviveHud, g_optCapHud, g_optTankTargetHud;
extern bool g_optDamageSourceHud;
extern bool g_optFriendlyFireHud;
extern int g_damageSourceHudMode;
extern bool g_optInteractHud;
extern bool g_optCustomDirectorHud, g_optThrowNoticeHud, g_optWorldActionProgress;
extern bool g_optWorldRescue, g_optWorldFriendlyFire, g_optAmmoHud, g_optItemHud, g_optMapHud;
extern int g_ammoGfxStyle;   // 0 off / 1 segments / 3 straight bar; 2 is retired
extern int g_ammoGfxPos;     // arc/bar: 0 up / 1 down / 2 left / 3 right (default 3)
extern int g_ammoGfxScale;   // 50-200 shared size
extern bool g_ammoSegAbs;    // segment free XY (kept when style==1)
extern int g_ammoSegOffX;    // screen-pixel top-left via HudPlaceAbs
extern int g_ammoSegOffY;
extern bool g_optHideAllCustomHud;
extern bool g_optMateLastLife, g_optMateSave, g_optMateLowHp;
void MateAlertOnRescue(int helper);
void WorldNoticeRescuePump();
extern bool g_optUpgradeAmmoHud;
extern bool g_upgradeAmmoHudAbs;
extern int g_upgradeAmmoHudAlign, g_upgradeAmmoHudOffX, g_upgradeAmmoHudOffY, g_upgradeAmmoHudScale;
extern bool g_optBwSelfHud;
extern bool g_bwSelfHudAbs;
extern int g_bwSelfHudAlign, g_bwSelfHudOffX, g_bwSelfHudOffY, g_bwSelfHudScale;

extern int g_teamWorldNamesColor, g_teamWorldNamesSize, g_teamWorldNamesFontPx;
extern unsigned long g_teamWorldNamesFont;
unsigned long SurfAcquireWorldNamesFont(int tall);
bool TeamWorldNamesWanted();
bool NativeWorldNamesWanted();
bool TeamHudEntityPaintWanted();
void MapHudResetPos();
void CapHudResetPos();
void ReviveHudResetPos();
void TankTargetHudResetPos();
void TankTargetHudClamp();
void InteractHudResetPos();
void WitchStartleHudResetPos();
void TeamHudClampStyleScale();
const wchar_t* TeamHudStyleName(int s);
void TeamHudResetPos();
void AmmoHudResetPos();
void UpgradeAmmoHudResetPos();
void BwSelfHudResetPos();
void ItemHudResetPos();
void AmmoHudInvalidateFont();
void WitchStartleHudInvalidateFont();
void MapHudNudgeScreen(int dx, int dy);
void CapHudNudgeScreen(int dx, int dy);
void ReviveHudNudgeScreen(int dx, int dy);
void TankTargetHudNudgeScreen(int dx, int dy);
extern bool g_damageSourceHudAbs;
extern int g_damageSourceHudAlign, g_damageSourceHudOffX, g_damageSourceHudOffY;
extern int g_damageSourceHudFont, g_damageSourceHudScale;
void DamageSourceHudResetPos();
void DamageSourceHudNudgeScreen(int dx, int dy);
void DamageSourceHudClamp();
void DamageSourceHudClear();
void DamageSourceHudInvalidateFont();
void DamageSourceHudOnEvent(int attackerEnt, int damage);
void DamageSourceHudPaint();
extern bool g_friendlyFireHudAbs;
extern int g_friendlyFireHudAlign, g_friendlyFireHudOffX, g_friendlyFireHudOffY;
extern int g_friendlyFireHudFont, g_friendlyFireHudScale;
void FriendlyFireHudResetPos();
void FriendlyFireHudNudgeScreen(int dx, int dy);
void FriendlyFireHudClamp();
void FriendlyFireHudClear();
void FriendlyFireHudInvalidateFont();
void FriendlyFireHudOnEvent(int victimEnt, int damage);
void FriendlyFireHudPaint();

// SiAggro / WitchRage / WitchHp / BossDmgBoard / Ignite HUDs. Layout globals live in
// skeeto_hud.cpp; g_opt* and netvar offsets live in the proxy. Sampling stays on Paint.
extern bool g_optSiAggroHud, g_optWitchRageHud, g_optWitchHpHud, g_optBossDmgBoardHud, g_optIgniteHud;
extern bool g_witchRageHudWander;
extern int g_offWitchRage, g_offWitchWanderRage;
extern int g_offCheckpointDamageToTank, g_offCheckpointDamageToWitch;
extern bool g_siAggroHudAbs;
extern int g_siAggroHudAlign, g_siAggroHudOffX, g_siAggroHudOffY, g_siAggroHudFont, g_siAggroHudScale;
extern bool g_witchRageHudAbs;
extern int g_witchRageHudAlign, g_witchRageHudOffX, g_witchRageHudOffY, g_witchRageHudFont, g_witchRageHudScale;
extern bool g_witchHpHudAbs;
extern int g_witchHpHudAlign, g_witchHpHudOffX, g_witchHpHudOffY, g_witchHpHudFont, g_witchHpHudScale;
extern bool g_bossDmgBoardHudAbs;
extern int g_bossDmgBoardHudAlign, g_bossDmgBoardHudOffX, g_bossDmgBoardHudOffY;
extern int g_bossDmgBoardHudFont, g_bossDmgBoardHudScale;
extern bool g_igniteHudAbs;
extern int g_igniteHudAlign, g_igniteHudOffX, g_igniteHudOffY, g_igniteHudFont, g_igniteHudScale;
void SiAggroHudResetPos();
void SiAggroHudNudgeScreen(int dx, int dy);
void SiAggroHudClamp();
void SiAggroHudClear();
void SiAggroHudInvalidateFont();
void SiAggroHudPaint();
void WitchRageHudResetPos();
void WitchRageHudNudgeScreen(int dx, int dy);
void WitchRageHudClamp();
void WitchRageHudClear();
void WitchRageHudInvalidateFont();
void WitchRageHudNoteSpawn(int ent); // witch_spawn; event thread, atomic hint only
void WitchRageHudPaint();
void WitchHpHudResetPos();
void WitchHpHudNudgeScreen(int dx, int dy);
void WitchHpHudClamp();
void WitchHpHudClear();
void WitchHpHudInvalidateFont();
void WitchHpHudNoteSpawn(int ent); // witch_spawn; event thread, atomic only
void WitchHpHudNoteKilled();       // witch_killed; event thread, atomic only
void WitchHpHudPaint();
void BossDmgBoardResetPos();
void BossDmgBoardNudgeScreen(int dx, int dy);
void BossDmgBoardClamp();
void BossDmgBoardClear();
void BossDmgBoardInvalidateFont();
void BossDmgBoardOnTankKilled();   // event thread: stamps an atomic only
void BossDmgBoardOnWitchKilled();  // event thread: stamps an atomic only
void BossDmgBoardOnRoundStart();   // event thread: stamps an atomic only
void BossDmgBoardPaint();
void IgniteHudResetPos();
void IgniteHudNudgeScreen(int dx, int dy);
void IgniteHudClamp();
void IgniteHudClear();
void IgniteHudInvalidateFont();
void IgniteHudOnEvent(void* ev);   // zombie_ignited; event thread, bounded queue only
void IgniteHudPaint();
void InteractHudNudgeScreen(int dx, int dy);
void WitchStartleHudNudgeScreen(int dx, int dy);
void TeamHudNudgeScreen(int dx, int dy);
void AmmoHudNudgeScreen(int dx, int dy);
void UpgradeAmmoHudNudgeScreen(int dx, int dy);
void BwSelfHudNudgeScreen(int dx, int dy);
void ItemHudNudgeScreen(int dx, int dy);
void MapHudInvalidateFont();
void MapHudPaintHud();
bool MapHudChapterCounts(const char* stem, int* chapter, int* total);
const wchar_t* MapHudMissionName(const char* stem, bool chapterName); // completed current-map cache only
// Paint only. Official chapters resolve one localization token; custom names come from the mission cache.
void MapHudCopyDisplayNames(const char* map, wchar_t* campaign, int campaignCap, wchar_t* chapter, int chapterCap);
void MapHudMissionPump(const char* mode); // null cancels/clears on Paint
bool TeamHudCnIHas(const char* cn, const char* needle);
bool TeamHudIsHuntRifle(const char* cn);
int TeamHudWepKind(const char* cn);
// player_death / witch_killed "weapon" string (usually lowercase). TW_None if not a gun/melee icon.
int TeamHudWepKindFromDeathWeapon(const char* weapon);
void TeamHudDrawIcon(int x, int y, int w, int h, int kind, const char* meleeStem);
// Letterbox path used by teammate primary + killfeed game icons (not Fill/crop).
void TeamHudDrawWepIconLetterbox(int x, int y, int w, int h, int kind, const char* meleeStem);
void TeamHudSetIconAlpha(int a); // 0..255; blit uses this until reset (default 255)
void TeamHudSetIconMirror(bool on); // horizontal flip for icon blits until reset (default off)
bool TeamHudIsGun(int k);
int TeamHudGunMaxClipKind(int kind);
const wchar_t* TeamHudPlayerSlotLabel(int slot);
void TeamHudMatFlush();
void TeamHudResetSurvivorSetCache();
COLORREF CapHudColor(int cap);
const wchar_t* CapHudColorName(int slot);
bool TeamHudLocalIsSpectating();
const char* TeamHudModelNameInner(int modelIndex, bool skipLoadGate, bool allowWhileSpectating = false);
int TeamHudWepKindCorrected(int wepIdx, const char* cn);
// forDeathEvent: kill-feed may resolve while local is spectating; TeamHud paint stays gated off.
void TeamHudFillMeleeStem(int wepIdx, char* out, int outN, bool forDeathEvent = false);
void TeamHudReadGunAmmo(void* owner, int wepIdx, int kind, int* clip, int* ammo);
void TeamHudResetRuntimeState();
void TeamHudOnBotSwap(const char* eventName, int playerUserId, int botUserId);
void TeamHudGather();
const wchar_t* TeamHudCharName(int ch);
void TeamWorldHudPaint();
void WorldNoticeClear();
void WorldNoticeSetFriendlyFire(int attackerEnt);
enum WorldPinKind : int {
	kWorldPinNone = 0,
	kWorldPinTongue = 1,
	kWorldPinPounce = 2,
	kWorldPinJockey = 3,
	kWorldPinCharger = 4,
};
void WorldNoticeClearPinTrack();
void WorldNoticeOnControlStarted(void* ev, int kind);
void WorldNoticeOnPinShove(void* ev);
void WorldNoticeOnPinKill(void* ev);
// whoKey is userid or rescuer. Null only waits out the pin; it does not invent a rescuer.
void WorldNoticeOnPinEnd(void* ev, const char* whoKey);
bool WorldNoticeAirborneTrackingWanted();
bool WorldNoticeChargerTrackingWanted();
void WorldNoticeClearUnusedTrackers();
bool WorldNoticeReadAttackLeap(void* ent, int zombieClass);
void WorldNoticeTrackAirborneEntity(void* ent, int victim);
void WorldNoticeOnChargerCharge(void* ev, bool started);
bool WorldNoticeChargeWindow(int ent);
bool WorldNoticeWasCharging(int ent);
void WorldNoticeOnReviveSuccess(void* ev);
void WorldNoticeOnDefibSuccess(void* ev);
void WorldNoticeOnBile(void* ev, bool active);
bool WorldNoticeSnapshotInfected(WorldInfectedNoticeState* out, int ent,
	float eventX = 0.f, float eventY = 0.f, float eventZ = 0.f);
void WorldNoticeOnWitchKilled(void* ev);
void WorldNoticeOnTankKilled(void* ev);
void WorldNoticeOnTankSlayer(void* ev);
void WorldNoticePaint();
void TeamHudPaintHud();
void CapHudInvalidateFont();
void ReviveHudInvalidateFont();
void TankTargetHudInvalidateFont();
void CapHudPaint();
void ReviveHudPaint();
void TankTargetHudPaint();
void InteractHudPaint();
void InteractHudDropFont();
void AmmoHudPaintHud();
void UpgradeAmmoHudPaint();
void BwSelfHudPaint();
void HudOverlayAmmoArcPaint();
void AmmoBarHudPaint();
void AmmoSegHudPaint();
void AmmoGfxHudPaint(); // paints active style (Wanted/paint entry)
void ClampAmmoSegHud();
void ClampAmmoGfxHud();
void AmmoSegHudResetPos();
void AmmoSegHudNudgeScreen(int dx, int dy);
void HudOverlayReloadRingPaint();
void ItemHudPaint();
void DirectorHudPaint();
void WitchStartleHudClear();
void WitchStartleHudPaint();
void WitchStartleOnEvent(void* ev);

// hud-nest extras
extern int g_ammoHudLayoutFont;
extern int g_capHudColor[4];
extern unsigned long g_capHudFontId;
extern unsigned long g_capHudFontSmId;
extern int g_capHudFontSmTall;
extern int g_capHudFontTall;
extern int g_capHudLayoutFont;
extern DWORD g_chargerChargeUntil[65];
extern unsigned long g_dirFont;
extern int g_dirFontPx;
extern bool g_dirHudAbs;
extern int g_dirHudFont;
extern int g_dirHudScale;
extern int g_dirHudX;
extern int g_dirHudY;
extern int g_itemHudKit;
extern int g_itemHudNade;
extern int g_itemHudPills;
extern DWORD g_itemHudSampleAt;
extern DWORD g_localAirHitAt[65];
extern int g_mapHudLayoutChapPct, g_mapHudLayoutGaugeSegs;
extern int g_mapHudLayoutH1, g_mapHudLayoutH2;
extern int g_mapHudLayoutW1, g_mapHudLayoutW2;
extern int g_mapHudLayoutInner, g_mapHudLayoutPanelW, g_mapHudLayoutPanelH;
extern wchar_t g_mapHudLayoutLine1[80];
extern wchar_t g_mapHudLayoutLine2[80];
extern unsigned long g_mapLayoutFontId;
extern int g_mapLayoutFontPx;
extern unsigned long g_mapLayoutFontSmId;
extern int g_mapLayoutFontSmPx;
extern ModelNameCacheEntry g_modelNameCache[32];
extern int g_modelNameCacheN;
extern unsigned long g_revHudFontId;
extern unsigned long g_revHudFontSmId;
extern int g_revHudFontSmTall;
extern int g_revHudFontTall;
extern int g_revHudLayoutFont;
extern unsigned long g_teamFont;
extern unsigned long g_teamFontLg;
extern int g_teamFontLgPx;
extern int g_teamFontPx;
extern unsigned long g_teamFontSm;
extern int g_teamFontSmPx;
extern DWORD g_teamHudGatherPaintAt;
extern TeamHudGearCache g_teamHudGearCache[33];
extern TeamHudNameCacheEntry g_teamHudNameCache[16];
extern int g_teamHudNameCacheN;
extern bool g_teamHudNameVoted;
extern PlayerInfo g_teamHudPlayerInfoCache[33];
extern bool g_teamHudPlayerInfoValid[33];
extern int g_teamHudSetCached;
extern bool g_teamHudSetLogged;
extern char g_teamHudSetMap[64];
extern unsigned long g_teamWorldFont;
extern wchar_t g_witchStartleName[24];
extern DWORD g_witchStartleT0;
extern DWORD g_witchStartleUntil;
constexpr int kMenuPageItemHud = 40;
constexpr int kMenuPageUpgradeAmmoHud = 60;
constexpr int kMenuPageAmmoGfxHud = 72; // unified graphic ammo HUD subpage (seg/bar)
constexpr int kMenuPageAmmoSegHud = 74; // legacy; entry removed, kept so old page ids ignore cleanly
constexpr int kMenuPageBwSelfHud = 76; // local third-strike black-and-white cracked-heart badge
constexpr int kMenuPageWitchStartleHud = 52;
constexpr int kMenuPageTankTargetHud = 105;
constexpr int kMenuPageDamageSourceHud = 106;
constexpr int kMenuPageFriendlyFireHud = 107;
constexpr int kMenuPageSiAggroHud = 108;       // SI m_lookatPlayer on local
constexpr int kMenuPageWitchRageHud = 109;     // Witch m_rage / m_wanderRage
constexpr int kMenuPageBossDmgBoardHud = 110;  // post-kill Tank/Witch dmg board
constexpr int kMenuPageIgniteHud = 111;        // zombie_ignited Witch/Tank toast
constexpr int kMenuPageWitchHpHud = 112;       // Witch HP estimate from checkpoint damage
constexpr int kMenuPageLaserRoundNotice = 123; // chapter-start laser-sight toast
constexpr int kTeamCapCharger = 4;
constexpr int kTeamCapChargerPummel = 5;
constexpr int kTeamCapHunter = 2;
constexpr int kTeamCapJockey = 3;
constexpr int kTeamCapNone = 0;
constexpr int kTeamCapSmoker = 1;

// Skeeto TAB board (proxy) reuses TeamHud / MapHud readers. No default arguments
// here: the skeeto_hud.cpp prototypes keep theirs.
int TeamHudResolveProp(int entIdx, void* e, int rawProp);
// Returns a canonical portrait index; only unresolved raw slots use cast fallback.
// playerInfo must describe e at this sampling point (no engine queries or retained pointer).
int TeamHudResolveAvatarProp(void* e, int rawProp, const PlayerInfo* playerInfo);
bool TeamHudEntQualifies(int entIdx, void* e, int* outProp);
void TeamHudFillSlot(TeamHudSlot& s, void* e, int entIdx, int faceCh, bool isLocal, bool refreshName);
void TeamHudReadWepsCached(void* e, int entIdx, TeamHudSlot* s, DWORD maxAgeMs);
bool TeamHudFullReady(bool menuPreview);
int SktAnimatingStudioHdrOff();
int TeamHudBindFace(int ch, bool incap, int entIdx = 0);
COLORREF TeamHudCharColor(int ch);
COLORREF TeamHudCapColor(int cap);
void MapHudStem(const char* map, char* out, int outN);
const wchar_t* MapHudCampaignName(int camp);
int MapHudCampaignChapters(int camp);
void MapHudParse(const char* stem, int* camp, int* chap);
