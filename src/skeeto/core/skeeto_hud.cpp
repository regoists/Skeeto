#include "skeeto_ui_text.h"
#include "skeeto_hud.h"
#include "skeeto_memread.h"
#include "skeeto_elim_persist.h"
#include "skeeto_style.h"
#include "skeeto_mission.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cwchar>

// =============================================================================
// TeamHudSlot nest. Teammate / world-notice / revive / cap / map / ammo / item /
// WitchStartle / DirectorHudPaint. Paint + nest events only.
// g_opt* / menu / ini stay in proxy. No Listen / Spotlight / Xhair / HandleEvent.
// Closed module: Wanted() then O(1) return. Gather on the game thread.
// =============================================================================

WorldNoticeState g_worldFfNotice{};
WorldNoticeState g_worldWitchNotice{};
WorldNoticeState g_worldTankNotice{};
WorldNoticeState g_worldRescueNotice{};
WorldInfectedNoticeState g_worldMeleeSkeetNotice{};
WorldInfectedNoticeState g_worldLevelNotice{};
WorldInfectedNoticeState g_worldSkeetNotice{};
WorldInfectedNoticeState g_worldTongueCutNotice{};
WorldInfectedNoticeState g_worldCrownNotice{};
WorldInfectedNoticeState g_worldTankSlayerNotice{};
int g_worldCrownKind = 0; // 1 melee crown, 2 crown, 3 perfect crown
WorldBileNoticeState g_worldBileNotices[kTeamHudMax]{};
bool g_worldBileAnyActive = false;
struct TeamHudIdleLink {
	int playerUserId = 0;
	int botUserId = 0;
	int playerEnt = 0;
	int botEnt = 0;
	DWORD born = 0;
	wchar_t playerName[24]{};
};
TeamHudIdleLink g_teamHudIdleLinks[kTeamHudMax]{};
WorldNoticeState g_worldWitchStartleNotice[kTeamHudMax]{};
bool g_worldWitchStartleAny = false;
int g_worldControlledBy = 0;
static int g_worldPinKind = 0;
static int g_worldRescueCandidate = 0;
static DWORD g_worldRescueCandidateAt = 0;
static DWORD g_worldControlPollAt = 0;
static DWORD g_worldPinArmedAt = 0;
static DWORD g_worldPinReleaseAt = 0;
static int g_worldRecentController = 0;
static int g_worldRecentPinKind = 0;
static DWORD g_worldControlledRecentUntil = 0;
static int g_worldPendingShoveHelper = 0;
static int g_worldPendingShoveTarget = 0;
static DWORD g_worldPendingShoveAt = 0;
static int g_worldPendingKillVictim = 0;
static int g_worldPendingKillHelper = 0;
static DWORD g_worldPinProbeUntil = 0;
static constexpr DWORD kWorldPinPollMs = 200;
static constexpr DWORD kWorldPinArmMs = 500;
static constexpr DWORD kWorldPinKillWaitMs = 200;
static constexpr DWORD kWorldPinKillGraceMs = 1200;
static constexpr DWORD kWorldPinShoveMs = 3000;
int g_selfHudX = 16;
int g_selfHudY = 520;
unsigned long g_worldKillFont = 0;
int g_worldKillFontTall = 0;
int g_tankTargetHudAlign = 4;
int g_tankTargetHudOffX = 0;
int g_tankTargetHudOffY = 48;
int g_tankTargetHudFont = 20;
int g_tankTargetHudScale = 100;
bool g_tankTargetHudAbs = false;
static unsigned long g_tankTargetHudFontId = 0;
static int g_tankTargetHudFontPx = 0;
struct TankTargetHudRow {
	wchar_t targetName[24];
	bool localTarget;
};
static constexpr int kTankTargetHudMaxRows = 4;
static TankTargetHudRow g_tankTargetHudRows[kTankTargetHudMaxRows]{};
static int g_tankTargetHudRowCount = 0;
static DWORD g_tankTargetHudNextSampleAt = 0;
bool g_damageSourceHudAbs = false;
int g_damageSourceHudAlign = 1;
int g_damageSourceHudOffX = 20;
int g_damageSourceHudOffY = 64;
int g_damageSourceHudFont = 18;
int g_damageSourceHudScale = 100;
static unsigned long g_damageSourceHudFontId = 0;
static int g_damageSourceHudFontPx = 0;
static constexpr int kDamageSourceHudMaxRows = 5;
struct DamageSourceHudRow {
	int attackerEnt;
	int damage;
	DWORD until;
	wchar_t name[32];
};
static DamageSourceHudRow g_damageSourceHudRows[kDamageSourceHudMaxRows]{};
bool g_friendlyFireHudAbs = false;
int g_friendlyFireHudAlign = 1;
int g_friendlyFireHudOffX = 20;
int g_friendlyFireHudOffY = 96;
int g_friendlyFireHudFont = 18;
int g_friendlyFireHudScale = 100;
static unsigned long g_friendlyFireHudFontId = 0;
static int g_friendlyFireHudFontPx = 0;
static constexpr int kFriendlyFireHudMaxRows = 4;
struct FriendlyFireHudRow {
	int victimEnt;
	int damage;
	DWORD until;
	DWORD mergeUntil;
	wchar_t name[32];
};
static FriendlyFireHudRow g_friendlyFireHudRows[kFriendlyFireHudMaxRows]{};
// Teammate layout slots 0..6 → 玩家2..玩家8 (自己单独 g_selfHudX/Y).

/* nest-local only. Do not copy these into skeeto_hud.h. */
int TeamHudPx(int v);
int TeamHudFontPx();
void TeamHudCardWHAtScale(int* w, int* h, int scalePct);
void TeamHudCardWH(int* w, int* h);
void MapHudEnsureFonts();
void MapHudStem(const char* map, char* out, int outN);
const wchar_t* MapHudCampaignName(int camp);
int MapHudCampaignChapters(int camp);
void MapHudParse(const char* stem, int* camp, int* chap);
void TeamHudPlayerInfoCacheClear();
const PlayerInfo* TeamHudPlayerInfoCached(int entIdx);
void TeamHudNameCacheClear();
const wchar_t* TeamHudNameCacheFind(int ent, int userid);
void TeamHudNameCachePut(int ent, int userid, const wchar_t* name, bool fromPlayer);
const wchar_t* TeamHudIdleNameForBot(int ent, int userid);
int TeamHudIdleBodyForEnt(int ent);
void TeamHudApplyName(TeamHudSlot& s, int entIdx);
int TeamHudReadHandleIdx(void* e, int off);
bool TeamHudHandleLive(void* e, int off);
bool TeamHudReadBool(void* e, int off);
float TeamHudReadProgressFrac(void* e, float* remainOut = nullptr);
void TeamHudFillTag(int ent, int* uid, wchar_t* name, int nameN);
bool TeamHudHas(const char* cn, const char* s);
COLORREF AmmoHudClipColor(int clip, int maxClip, COLORREF healthy = RGB(72, 210, 88));
bool TeamHudIsSide(int k);
bool TeamHudIsPrimary(int k);
bool TeamHudIsNade(int k);
bool TeamHudIsKit(int k);
bool TeamHudIsPills(int k);
bool TeamHudIsItemKind(int kind);
const wchar_t* TeamHudWepLabel(int k);
int TeamHudWepMatList(int k, const char** out, int maxOut);
bool TeamHudMeleeSprite(const char* stem, TeamHudSprite* out);
bool TeamHudSpriteFor(int kind, const char* meleeStem, int pistolClipHint, TeamHudSprite* out);
bool TeamHudLooksLikeSheet(int tw, int th);
bool TeamHudBlitIconFill(int x, int y, int w, int h, int id, float s0, float t0, float s1, float t1);
bool TeamHudBlitIconContain(int x, int y, int w, int h, int id, float s0, float t0, float s1, float t1, float tileAr);
bool TeamHudBlitSpriteVanilla(int x, int y, int w, int h, const TeamHudSprite& sp);
bool TeamHudBlitSpriteFill(int x, int y, int w, int h, const TeamHudSprite& sp, int sheetIdOpt);
const char* TeamHudFaceMat(int ch, bool incap);
COLORREF TeamHudCharColor(int ch);
float TeamHudSafeUvWidth(int kind, int tw, int th);
void TeamHudDrawPrimaryIcon(int x, int y, int w, int h, int kind);
void TeamHudDrawIconEx(int x, int y, int w, int h, int kind, const char* meleeStem, int pistolClipHint, bool vanillaOnly);
void TeamHudDrawIcon(int x, int y, int w, int h, int kind, const char* meleeStem);
bool TeamHudStrHasI(const char* hay, const char* needle);
int TeamHudOfficialSet(const char* map);
int TeamHudQuerySurvivorSet();
int TeamHudNameSetHint(const char* n);
int TeamHudFaceFromProp(int prop, bool l4d1Slots);
int TeamHudBindFace(int ch, bool incap);
int TeamHudBindSi(int cap);
COLORREF TeamHudCapColor(int cap);
int CapHudColorSlot(int cap);
const wchar_t* TeamHudCapLetter(int cap);
void TeamHudDrawSiMark(int x, int y, int sz, int cap);
void TeamHudDrawCornerTicks(int x, int y, int w, int h, COLORREF rgb, int a);
void TeamHudEnsureFonts();
const wchar_t* TeamHudMeleeLabel(const char* stem);
const char* TeamHudModelName(int modelIndex);
bool TeamHudMeleeStemFromModel(const char* model, char* out, int outN);
void TeamHudFillMeleeStem(int wepIdx, char* out, int outN, bool forDeathEvent);
void TeamHudReadWeps(void* e, TeamHudSlot* s);
bool TeamHudFullReady(bool menuPreview = false);
void TeamHudCopyGear(TeamHudSlot* dst, const TeamHudSlot& src);
void TeamHudReadWepsCached(void* e, int entIdx, TeamHudSlot* s, DWORD maxAgeMs = 250u);
int TeamHudPropFromName(const char* n);
int TeamHudPropFromModel(void* e);
int TeamHudResolveProp(int entIdx, void* e, int rawProp);
bool TeamHudEntGhostObs(void* e);
int TeamHudEntPriority(void* e, int entIdx);
int TeamHudDedupRecs(int nRec, int* idx, int* prop, void** ents, int* userids);
bool TeamHudEntQualifies(int entIdx, void* e, int* outProp);
void TeamHudFillSlot(TeamHudSlot& s, void* e, int entIdx, int faceCh, bool isLocal, bool refreshName);
void TeamHudFillChamfer(int x, int y, int w, int h, int cut);
void TeamHudStrokeChamfer(int x, int y, int w, int h, int cut);
COLORREF TeamHudHpCol(const TeamHudSlot& s);
bool WorldNoticeSurvivorEnt(int ent);
int WorldNoticeEventPlayer(void* ev, const char* key, const char* fallbackKey = nullptr);
bool WorldNoticeEventIsLocal(void* ev, const char* key, const char* fallbackKey = nullptr);
void WorldNoticeTriggerRescue(int helper);
void WorldNoticeControlTick();
bool WorldNoticeHeadScreen(int ent, float* sx, float* sy, int sw, int sh);
void WorldNoticeDrawTrophy(int cx, int cy, int alpha, const wchar_t* text);
void WorldNoticeDrawRescue(int cx, int cy, int alpha);
void WorldNoticeThickLine(int x0, int y0, int x1, int y1, int thick, int r, int g, int b, int alpha);
void WorldNoticeDrawTechDiamond(int cx, int cy, int radius, int r, int g, int b, int alpha);
int WorldNoticeKillPx(int v);
bool WorldNoticeProjectCloseKill(const float* world, float* sx, float* sy, int sw, int sh);
void WorldNoticeDrawMeleeSkeet(int cx, int cy, int alpha);
void WorldNoticeDrawLevel(int cx, int cy, int alpha);
void WorldNoticeDrawSkeet(int cx, int cy, int alpha);
void WorldNoticeDrawTongueCut(int cx, int cy, int alpha);
void WorldNoticeDrawCrown(int cx, int cy, int alpha, int kind);
void WorldNoticeDrawActionProgress(int cx, int cy, float frac, float remain);
void WorldNoticeDrawBile(int cx, int cy, int alpha);
void WorldNoticeDrawTankSlayer(int cx, int cy, int alpha);
void WorldNoticeDrawWitchStartle(int cx, int cy, int alpha);
void TeamHudPaintFaceAt(const TeamHudSlot& s, int layoutSlot, int fx, int fy, int face, bool empty);
void TeamHudPaintSolidBar(int barX, int barY, int barW, int barH, int hp, int capHp, int tempHp, bool incap, COLORREF hc, int look);
void TeamHudPaintPipsVert(int x, int y, int pw, int ph, int n, int hp, int tempHp, int capHp, COLORREF hc);
void TeamHudPaintRevive(int barX, int hpY, int barW, float revive);
void TeamHudPaintGear(const TeamHudSlot& s, int wx, int wy, int wepW, int wepH, int item, int rightEdge);
void TeamHudPaintCard(const TeamHudSlot& s, int layoutSlot, int x, int y, int w, int h, bool preview, bool selfPanel);
const wchar_t* TeamHudCapVerb(int cap, bool hanging);
void CapHudEnsureFonts();
void ReviveHudEnsureFonts();
void AmmoHudEnsureFonts();
void ItemHudSample();
void WitchStartleFillName(int entIdx, void* e, wchar_t* out, int cap);
void WitchStartleHudEnsureFonts();


bool TeamHudEntityPaintWanted() {
	if (IdleSwapHudQuietActive()) return false;
	return NativeWorldNamesWanted() || TeamWorldNamesWanted() || HudWantsDraw(g_optTeamHud, 15)
		|| (!g_worldHudRealism && HudWantsDraw(g_optTeamWorldHud, kMenuPageWorldHud))
		|| (!g_worldHudRealism && HudWantsDraw(g_optWorldBlackWhite, kMenuPageWorldHud))
		|| (!g_worldHudRealism && SurvivorGlowWanted())
		|| g_optMateLastLife || g_optMateLowHp
		|| HudWantsDraw(g_optWorldActionProgress, kMenuPageWorldHud)
		|| HudWantsDraw(g_optCapHud, 23)
		|| HudWantsDraw(g_optReviveHud, 24) || HudWantsDraw(g_optCustomDirectorHud, 17);
}

void MapHudResetPos() {
	g_mapHudAlign = 4;
	g_mapHudOffX = 0;
	g_mapHudOffY = 8;
	g_mapHudAbs = false;
}

void CapHudResetPos() {
	g_capHudAlign = 4;
	g_capHudOffX = 0;
	g_capHudOffY = 72;
	g_capHudAbs = false;
}

void ReviveHudResetPos() {
	g_revHudAlign = 4;
	g_revHudOffX = 0;
	g_revHudOffY = 168;
	g_revHudAbs = false;
}

void TankTargetHudResetPos() {
	g_tankTargetHudAlign = 4;
	g_tankTargetHudOffX = 0;
	g_tankTargetHudOffY = 48;
	g_tankTargetHudAbs = false;
}

void InteractHudResetPos() {
	g_optInteractHudAlign = 4;
	g_interactHudOffX = 0;
	g_interactHudOffY = 168;
	g_optInteractHudAbs = false;
}

void WitchStartleHudResetPos() {
	g_witchStartleHudAlign = 4;
	g_witchStartleHudOffX = 0;
	g_witchStartleHudOffY = 90;
	g_witchStartleHudAbs = false;
}

int TeamHudPx(int v) {
	int s = g_teamHudPxOverride > 0 ? g_teamHudPxOverride : g_teamHudScale;
	if (s < kTeamHudScaleMin) s = kTeamHudScaleMin;
	if (s > kTeamHudScaleMax) s = kTeamHudScaleMax;
	const int autoPct = g_hudResolutionScalePct;
	int n = (v * s * autoPct + 5000) / 10000;
	return n < 1 ? 1 : n;
}

int TeamHudFontPx() {
	int f = g_teamHudFont;
	if (f < kTeamHudFontMin) f = kTeamHudFontMin;
	if (f > kTeamHudFontMax) f = kTeamHudFontMax;
	const int autoPct = g_hudResolutionScalePct;
	f = (f * autoPct + 50) / 100;
	return f < 1 ? 1 : f;
}

void TeamHudClampStyleScale() {
	if (g_teamHudStyle < 0) g_teamHudStyle = 0;
	if (g_teamHudStyle >= kTeamHudStyleCount) g_teamHudStyle = kTeamHudStyleCount - 1;
	if (g_teamHudScale < kTeamHudScaleMin) g_teamHudScale = kTeamHudScaleMin;
	if (g_teamHudScale > kTeamHudScaleMax) g_teamHudScale = kTeamHudScaleMax;
	g_teamHudScale = (g_teamHudScale / kTeamHudScaleStep) * kTeamHudScaleStep;
	if (g_teamHudScale < kTeamHudScaleMin) g_teamHudScale = kTeamHudScaleMin;
}

void TeamHudCardWHAtScale(int* w, int* h, int scalePct) {
	int bw = 360, bh = 108;
	switch (g_teamHudStyle) {
	case 1: bw = 400; bh = 56; break;
	case 2: bw = 320; bh = 48; break;
	case 3: bw = 340; bh = 104; break;
	case 4: bw = 380; bh = 120; break;
	case 5: bw = 360; bh = 104; break;
	case 6: bw = 360; bh = 90; break;
	default: bw = 360; bh = 108; break;
	}
	int s = scalePct;
	if (s < kTeamHudScaleMin) s = kTeamHudScaleMin;
	if (s > kTeamHudScaleMax) s = kTeamHudScaleMax;
	if (w) {
		const int autoPct = g_hudResolutionScalePct;
		int n = (bw * s * autoPct + 5000) / 10000;
		*w = n < 1 ? 1 : n;
	}
	if (h) {
		const int autoPct = g_hudResolutionScalePct;
		int n = (bh * s * autoPct + 5000) / 10000;
		*h = n < 1 ? 1 : n;
	}
}

void TeamHudCardWH(int* w, int* h) {
	TeamHudCardWHAtScale(w, h, g_teamHudScale);
}

const wchar_t* TeamHudStyleName(int s) {
	switch (s) {
	case 1: return UiText(UiId::Text_13974bf8, L"竞技条");
	case 2: return UiText(UiId::Text_d8cb2cee, L"极简");
	case 3: return UiText(UiId::Text_c487d4ab, L"战术格");
	case 4: return UiText(UiId::Text_be3acf81, L"大数字");
	case 5: return UiText(UiId::Text_c4522936, L"角色色");
	case 6: return UiText(UiId::Text_c9730865, L"切角框");
	default: return UiText(UiId::Text_045dc8f8, L"经典卡片");
	}
}

void TeamHudResetPos() {
	int cw = 368, ch = 96;
	TeamHudCardWH(&cw, &ch);
	const int gap = TeamHudPx(12);
	const int left = 16;
	const int right = 16 + cw + TeamHudPx(8);
	for (int i = 0; i < kTeamHudMax; ++i) {
		g_teamHudX[i] = (i < 4) ? left : right;
		g_teamHudY[i] = 16 + (i % 4) * (ch + gap);
	}
	// 自己的卡略大一档，默认摆在左栏下方，不跟角色栏位走
	const int selfScale = g_teamHudScale + 20;
	int sw = 368, sh = 96;
	TeamHudCardWHAtScale(&sw, &sh, (selfScale > kTeamHudScaleMax) ? kTeamHudScaleMax : selfScale);
	g_selfHudX = left;
	g_selfHudY = 16 + 4 * (ch + gap) + TeamHudPx(20);
}

void AmmoHudResetPos() {
	g_ammoHudAbs = false;
	g_ammoHudOffX = 0;
	g_ammoHudOffY = 0;
}

void UpgradeAmmoHudResetPos() {
	g_upgradeAmmoHudAbs = false;
	g_upgradeAmmoHudAlign = 4;
	g_upgradeAmmoHudOffX = 0;
	g_upgradeAmmoHudOffY = 0;
}

void BwSelfHudResetPos() {
	g_bwSelfHudAbs = false;
	g_bwSelfHudAlign = 4;
	g_bwSelfHudOffX = 0;
	g_bwSelfHudOffY = 0;
}

void ItemHudResetPos() {
	g_itemHudAlign = 4;
	g_itemHudAbs = false;
	g_itemHudOffX = 150;
	g_itemHudOffY = 90;
}

void AmmoHudInvalidateFont() {
	g_ammoFontId = 0;
	g_ammoFontTall = 0;
	g_ammoFontSmId = 0;
	g_ammoFontSmTall = 0;
}

void WitchStartleHudInvalidateFont() { g_witchStartleFontId = 0; g_witchStartleFontTall = 0; }

void MapHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_mapHudAbs, &g_mapHudOffX, &g_mapHudOffY, g_mapHudAlign, dx, dy);
}

void CapHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_capHudAbs, &g_capHudOffX, &g_capHudOffY, g_capHudAlign, dx, dy);
}

void ReviveHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_revHudAbs, &g_revHudOffX, &g_revHudOffY, g_revHudAlign, dx, dy);
}

void TankTargetHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_tankTargetHudAbs, &g_tankTargetHudOffX, &g_tankTargetHudOffY,
		g_tankTargetHudAlign, dx, dy);
}

void DamageSourceHudResetPos() {
	g_damageSourceHudAlign = 1;
	g_damageSourceHudOffX = 20;
	g_damageSourceHudOffY = 64;
	g_damageSourceHudAbs = false;
}

void DamageSourceHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_damageSourceHudAbs, &g_damageSourceHudOffX,
		&g_damageSourceHudOffY, g_damageSourceHudAlign, dx, dy);
}

void DamageSourceHudClamp() {
	if (g_damageSourceHudMode != 0 && g_damageSourceHudMode != 1) g_damageSourceHudMode = 0;
	if (g_damageSourceHudAlign < 0 || g_damageSourceHudAlign > 5) g_damageSourceHudAlign = 1;
	g_damageSourceHudScale = HudClampScale(g_damageSourceHudScale);
	HudClampFont(&g_damageSourceHudFont);
	if (g_damageSourceHudAbs) {
		if (g_damageSourceHudOffX < 0) g_damageSourceHudOffX = 0;
		if (g_damageSourceHudOffX > 7680) g_damageSourceHudOffX = 7680;
		if (g_damageSourceHudOffY < 0) g_damageSourceHudOffY = 0;
		if (g_damageSourceHudOffY > 4320) g_damageSourceHudOffY = 4320;
	} else {
		if (g_damageSourceHudOffX < -400) g_damageSourceHudOffX = -400;
		if (g_damageSourceHudOffX > 600) g_damageSourceHudOffX = 600;
		if (g_damageSourceHudOffY < -400) g_damageSourceHudOffY = -400;
		if (g_damageSourceHudOffY > 600) g_damageSourceHudOffY = 600;
	}
}

void DamageSourceHudClear() {
	memset(g_damageSourceHudRows, 0, sizeof(g_damageSourceHudRows));
}

void DamageSourceHudInvalidateFont() {
	g_damageSourceHudFontId = 0;
	g_damageSourceHudFontPx = 0;
}

static bool TankTargetHudReadInt(void* ent, int offset, int* out);

static void DamageSourceHudResolveName(int attackerEnt, wchar_t* out, size_t cap) {
	if (!out || cap <= 0) return;
	out[0] = 0;
	if (attackerEnt > 0 && attackerEnt <= 64) {
		PlayerInfo pi{};
		if (EngPlayerInfo(attackerEnt, &pi) && pi.name[0])
			MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, out, (int)cap);
	}
	if (out[0]) return;
	const char* className = attackerEnt > 0 && attackerEnt < 2048
		? EntNetClassName(attackerEnt) : nullptr;
	const wchar_t* fallback = UiText(UiId::DamageSourceHudEnvironment, L"环境 / 未知来源");
	if (className && (!_stricmp(className, "Infected") || !_stricmp(className, "C_Infected")))
		fallback = UiText(UiId::DamageSourceHudCommon, L"普通感染者");
	else if (attackerEnt > 0 && attackerEnt <= 64 && g_offTeam >= 0 && g_offZombieClass >= 0) {
		void* attacker = EntGet(attackerEnt);
		int team = 0, zombieClass = 0;
		if (TankTargetHudReadInt(attacker, g_offTeam, &team) && team == 3
			&& TankTargetHudReadInt(attacker, g_offZombieClass, &zombieClass)) {
			switch (zombieClass) {
			case 1: fallback = UiText(UiId::TabBoardZcSmoker, L"Smoker"); break;
			case 2: fallback = UiText(UiId::TabBoardZcBoomer, L"Boomer"); break;
			case 3: fallback = UiText(UiId::TabBoardZcHunter, L"Hunter"); break;
			case 4: fallback = UiText(UiId::TabBoardZcSpitter, L"Spitter"); break;
			case 5: fallback = UiText(UiId::TabBoardZcJockey, L"Jockey"); break;
			case 6: fallback = UiText(UiId::TabBoardZcCharger, L"Charger"); break;
			case 8: fallback = L"Tank"; break;
			}
		}
	}
	else if (className && (!_stricmp(className, "Witch") || !_stricmp(className, "C_Witch")))
		fallback = L"Witch";
	else if (className && (!_stricmp(className, "Tank") || !_stricmp(className, "CTank")))
		fallback = L"Tank";
	wcsncpy_s(out, cap, fallback, _TRUNCATE);
}

void DamageSourceHudOnEvent(int attackerEnt, int damage) {
	if (!g_optDamageSourceHud || damage <= 0) return;
	const DWORD now = GetTickCount();
	wchar_t name[32]{};
	DamageSourceHudResolveName(attackerEnt, name, _countof(name));
	int slot = -1;
	for (int i = 0; i < kDamageSourceHudMaxRows; ++i) {
		DamageSourceHudRow& row = g_damageSourceHudRows[i];
		if (row.until > now && row.attackerEnt == attackerEnt && !wcscmp(row.name, name)) {
			slot = i;
			break;
		}
		if (slot < 0 && row.until <= now) slot = i;
	}
	if (slot < 0) {
		slot = 0;
		for (int i = 1; i < kDamageSourceHudMaxRows; ++i)
			if (g_damageSourceHudRows[i].until < g_damageSourceHudRows[slot].until) slot = i;
	}
	DamageSourceHudRow& row = g_damageSourceHudRows[slot];
	if (row.attackerEnt != attackerEnt || wcscmp(row.name, name) || row.until <= now) {
		row = {};
		row.attackerEnt = attackerEnt;
		wcsncpy_s(row.name, name, _TRUNCATE);
	}
	if (damage > 0x7fffffff - row.damage) row.damage = 0x7fffffff;
	else row.damage += damage;
	row.until = now + 3000u;
}

void DamageSourceHudPaint() {
	// Demo rows for own page AND peer layout preview (see HudLayoutSubPage rules).
	const bool preview = HudMenuPage(kMenuPageDamageSourceHud)
		|| HudLayoutPeer(g_optDamageSourceHud, kMenuPageDamageSourceHud);
	if (!HudWantsDraw(g_optDamageSourceHud, kMenuPageDamageSourceHud)) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		DamageSourceHudClear();
		return;
	}
	if (!MenuEnsureSurf()) return;
	DamageSourceHudClamp();
	const int sc = g_damageSourceHudScale;
	const int fontPx = HudScalePx(g_damageSourceHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_damageSourceHudFontId,
		&g_damageSourceHudFontPx, 750);
	if (!font) return;
	const DWORD now = GetTickCount();
	DamageSourceHudRow previewRow{};
	if (preview) {
		previewRow.attackerEnt = 2;
		previewRow.damage = 12;
		previewRow.until = now + 1;
		wcsncpy_s(previewRow.name, UiText(UiId::DamageSourceHudPreviewName, L"Coach"), _TRUNCATE);
	}
	const DamageSourceHudRow* rows = preview ? &previewRow : g_damageSourceHudRows;
	int count = preview ? 1 : 0;
	if (!preview) {
		for (int i = 0; i < kDamageSourceHudMaxRows; ++i) {
			if (rows[i].until && now >= rows[i].until) g_damageSourceHudRows[i] = {};
			else if (rows[i].until) ++count;
		}
	}
	if (count <= 0) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	const int padX = HudScalePx(12, sc), padY = HudScalePx(8, sc);
	const int textW = HudScalePx(260, sc);
	const int titleH = fontPx + HudScalePx(5, sc), rowH = fontPx + HudScalePx(6, sc);
	const int panelW = padX * 2 + textW, panelH = padY * 2 + titleH + 4 + count * rowH;
	int x = 0, y = 0;
	HudFinalizePos(&g_damageSourceHudAbs, &g_damageSourceHudOffX, &g_damageSourceHudOffY,
		g_damageSourceHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 76, 66);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 8, accent, preview ? 200 : 145);
	const int tx = x + padX;
	int ty = y + padY;
	SurfTextAt(font, tx, ty, RGB(255, 145, 132), UiText(UiId::DamageSourceHudTitle, L"伤害来源"));
	ty += titleH + 4;
	for (int i = 0; i < (preview ? 1 : kDamageSourceHudMaxRows); ++i) {
		const DamageSourceHudRow& row = rows[i];
		if (!row.until) continue;
		wchar_t line[80]{};
		swprintf_s(line, _countof(line), UiText(UiId::DamageSourceHudRow, L"%s  -%d HP"), row.name, row.damage);
		while (wcslen(line) > 3 && SurfTextWFont(font, line) > textW - HudScalePx(4, sc))
			line[wcslen(line) - 1] = 0;
		SurfTextAt(font, tx, ty, RGB(242, 244, 248), line);
		ty += rowH;
	}
}

void FriendlyFireHudResetPos() {
	g_friendlyFireHudAlign = 1;
	g_friendlyFireHudOffX = 20;
	g_friendlyFireHudOffY = 96;
	g_friendlyFireHudAbs = false;
}

void FriendlyFireHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_friendlyFireHudAbs, &g_friendlyFireHudOffX,
		&g_friendlyFireHudOffY, g_friendlyFireHudAlign, dx, dy);
}

void FriendlyFireHudClamp() {
	if (g_friendlyFireHudAlign < 0 || g_friendlyFireHudAlign > 5) g_friendlyFireHudAlign = 1;
	g_friendlyFireHudScale = HudClampScale(g_friendlyFireHudScale);
	HudClampFont(&g_friendlyFireHudFont);
	if (g_friendlyFireHudAbs) {
		if (g_friendlyFireHudOffX < 0) g_friendlyFireHudOffX = 0;
		if (g_friendlyFireHudOffX > 7680) g_friendlyFireHudOffX = 7680;
		if (g_friendlyFireHudOffY < 0) g_friendlyFireHudOffY = 0;
		if (g_friendlyFireHudOffY > 4320) g_friendlyFireHudOffY = 4320;
	} else {
		if (g_friendlyFireHudOffX < -400) g_friendlyFireHudOffX = -400;
		if (g_friendlyFireHudOffX > 600) g_friendlyFireHudOffX = 600;
		if (g_friendlyFireHudOffY < -400) g_friendlyFireHudOffY = -400;
		if (g_friendlyFireHudOffY > 600) g_friendlyFireHudOffY = 600;
	}
}

void FriendlyFireHudClear() {
	memset(g_friendlyFireHudRows, 0, sizeof(g_friendlyFireHudRows));
}

void FriendlyFireHudInvalidateFont() {
	g_friendlyFireHudFontId = 0;
	g_friendlyFireHudFontPx = 0;
}

void FriendlyFireHudOnEvent(int victimEnt, int damage) {
	if (!g_optFriendlyFireHud || victimEnt <= 0 || victimEnt > 64 || damage <= 0) return;
	PlayerInfo pi{};
	if (!EngPlayerInfo(victimEnt, &pi) || !pi.name[0]) return;
	wchar_t name[32]{};
	MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, name, (int)_countof(name));
	if (!name[0]) return;
	const DWORD now = GetTickCount();
	int slot = -1;
	for (int i = 0; i < kFriendlyFireHudMaxRows; ++i) {
		FriendlyFireHudRow& row = g_friendlyFireHudRows[i];
		if (row.until > now && row.victimEnt == victimEnt && !wcscmp(row.name, name)) {
			slot = i;
			break;
		}
		if (slot < 0 && row.until <= now) slot = i;
	}
	if (slot < 0) {
		slot = 0;
		for (int i = 1; i < kFriendlyFireHudMaxRows; ++i)
			if (g_friendlyFireHudRows[i].until < g_friendlyFireHudRows[slot].until) slot = i;
	}
	FriendlyFireHudRow& row = g_friendlyFireHudRows[slot];
	const bool sameLiveVictim = row.victimEnt == victimEnt && !wcscmp(row.name, name) && row.until > now;
	if (!sameLiveVictim) {
		row = {};
		row.victimEnt = victimEnt;
		wcsncpy_s(row.name, name, _TRUNCATE);
	}
	if (row.mergeUntil <= now) row.damage = damage;
	else if (damage > 0x7fffffff - row.damage) row.damage = 0x7fffffff;
	else row.damage += damage;
	row.mergeUntil = now + 450u;
	row.until = now + 2200u;
}

void FriendlyFireHudPaint() {
	// Demo rows for own page AND peer layout preview (see HudLayoutSubPage rules).
	const bool preview = HudMenuPage(kMenuPageFriendlyFireHud)
		|| HudLayoutPeer(g_optFriendlyFireHud, kMenuPageFriendlyFireHud);
	if (!HudWantsDraw(g_optFriendlyFireHud, kMenuPageFriendlyFireHud)) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		FriendlyFireHudClear();
		return;
	}
	if (!MenuEnsureSurf()) return;
	FriendlyFireHudClamp();
	const int sc = g_friendlyFireHudScale;
	const int fontPx = HudScalePx(g_friendlyFireHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_friendlyFireHudFontId,
		&g_friendlyFireHudFontPx, 750);
	if (!font) return;
	const DWORD now = GetTickCount();
	FriendlyFireHudRow previewRow{};
	if (preview) {
		previewRow.victimEnt = 2;
		previewRow.damage = 12;
		previewRow.until = now + 1;
		wcsncpy_s(previewRow.name, UiText(UiId::FriendlyFireHudPreviewName, L"Coach"), _TRUNCATE);
	}
	const FriendlyFireHudRow* rows = preview ? &previewRow : g_friendlyFireHudRows;
	int count = preview ? 1 : 0;
	if (!preview) {
		for (int i = 0; i < kFriendlyFireHudMaxRows; ++i) {
			if (rows[i].until && now >= rows[i].until) g_friendlyFireHudRows[i] = {};
			else if (rows[i].until) ++count;
		}
	}
	if (count <= 0) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	const int padX = HudScalePx(12, sc), padY = HudScalePx(8, sc);
	const int textW = HudScalePx(300, sc);
	const int titleH = fontPx + HudScalePx(5, sc), rowH = fontPx + HudScalePx(6, sc);
	const int panelW = padX * 2 + textW, panelH = padY * 2 + titleH + 4 + count * rowH;
	int x = 0, y = 0;
	HudFinalizePos(&g_friendlyFireHudAbs, &g_friendlyFireHudOffX, &g_friendlyFireHudOffY,
		g_friendlyFireHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 190, 48);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 8, accent, preview ? 210 : 155);
	const int tx = x + padX;
	int ty = y + padY;
	SurfTextAt(font, tx, ty, RGB(255, 205, 88), UiText(UiId::FriendlyFireHudTitle, L"黑枪伤害提示"));
	ty += titleH + 4;
	for (int i = 0; i < (preview ? 1 : kFriendlyFireHudMaxRows); ++i) {
		const FriendlyFireHudRow& row = rows[i];
		if (!row.until) continue;
		wchar_t line[80]{};
		swprintf_s(line, _countof(line), UiText(UiId::FriendlyFireHudRow, L"你黑了 %s  %d HP"), row.name, row.damage);
		while (wcslen(line) > 3 && SurfTextWFont(font, line) > textW - HudScalePx(4, sc))
			line[wcslen(line) - 1] = 0;
		SurfTextAt(font, tx, ty, RGB(255, 239, 205), line);
		ty += rowH;
	}
}

void InteractHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_optInteractHudAbs, &g_interactHudOffX, &g_interactHudOffY,
		g_optInteractHudAlign, dx, dy);
}

void WitchStartleHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_witchStartleHudAbs, &g_witchStartleHudOffX, &g_witchStartleHudOffY,
		g_witchStartleHudAlign, dx, dy);
}

void TeamHudNudgeScreen(int dx, int dy) {
	if (g_teamHudShow == kTeamHudShowSelf)
		g_teamHudSel = kTeamHudSelSelf;
	if (g_teamHudSel < 0) g_teamHudSel = 0;
	if (g_teamHudSel > kTeamHudSelSelf) g_teamHudSel = kTeamHudSelSelf;
	if (g_teamHudSel == kTeamHudSelSelf)
		HudNudgeAbs(nullptr, &g_selfHudX, &g_selfHudY, 0, dx, dy);
	else
		HudNudgeAbs(nullptr, &g_teamHudX[g_teamHudSel], &g_teamHudY[g_teamHudSel], 0, dx, dy);
}

void AmmoHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_ammoHudAbs, &g_ammoHudOffX, &g_ammoHudOffY, 0, dx, dy);
}

void UpgradeAmmoHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_upgradeAmmoHudAbs, &g_upgradeAmmoHudOffX, &g_upgradeAmmoHudOffY,
		g_upgradeAmmoHudAlign, dx, dy);
}

void BwSelfHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_bwSelfHudAbs, &g_bwSelfHudOffX, &g_bwSelfHudOffY,
		g_bwSelfHudAlign, dx, dy);
}

void ItemHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_itemHudAbs, &g_itemHudOffX, &g_itemHudOffY, g_itemHudAlign, dx, dy);
}

void MapHudInvalidateFont() {
	g_mapHudFontId = 0;
	g_mapHudFontTall = 0;
	g_mapHudFontSmId = 0;
	g_mapHudFontSmTall = 0;
	g_mapHudLayoutStem[0] = 0;
	g_mapHudLayoutScale = -1;
	g_mapHudLayoutFontBase = -1;
}

void MapHudEnsureFonts() {
	ClampMapHud();
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	int sm = g_mapHudFont * 3 / 4;
	if (sm < 16) sm = 16;
	const int mainWant = HudFontPx(g_mapHudFont), smWant = HudFontPx(sm);
	if (!g_mapHudFontId || g_mapHudFontTall != mainWant) {
		unsigned long f = SurfMakeYahei(mainWant, 800, flags);
		if (f) { g_mapHudFontId = f; g_mapHudFontTall = mainWant; }
	}
	if (!g_mapHudFontSmId || g_mapHudFontSmTall != smWant) {
		unsigned long f = SurfMakeYahei(smWant, 650, kFontFlagAntiAlias | kFontFlagDropShadow);
		if (f) { g_mapHudFontSmId = f; g_mapHudFontSmTall = smWant; }
	}
}

void MapHudStem(const char* map, char* out, int outN) {
	if (!out || outN <= 0) return;
	out[0] = 0;
	if (!map || !map[0]) return;
	const char* p = map;
	if (!_strnicmp(p, "maps/", 5) || !_strnicmp(p, "maps\\", 5)) p += 5;
	const char* slash = strrchr(p, '/');
	if (!slash) slash = strrchr(p, '\\');
	if (slash) p = slash + 1;
	strncpy(out, p, outN - 1);
	out[outN - 1] = 0;
	const size_t n = strlen(out);
	if (n > 4 && !_stricmp(out + n - 4, ".bsp"))
		out[n - 4] = 0;
}

const wchar_t* MapHudCampaignName(int camp) {
	switch (camp) {
	case 1: return UiText(UiId::Text_0650cbc4, L"死亡中心");
	case 2: return UiText(UiId::Text_aeebb913, L"黑色狂欢节");
	case 3: return UiText(UiId::Text_fbc9b38d, L"沼泽激战");
	case 4: return UiText(UiId::Text_01e9e325, L"暴风骤雨");
	case 5: return UiText(UiId::Text_56f58412, L"教区");
	case 6: return UiText(UiId::Text_d484ccce, L"短暂时刻");
	case 7: return UiText(UiId::Text_66ec98fa, L"牺牲");
	case 8: return UiText(UiId::Text_7357c8a0, L"毫不留情");
	case 9: return UiText(UiId::Text_ce6e5cf8, L"坠机险途");
	case 10: return UiText(UiId::Text_d0d4e433, L"死亡丧钟");
	case 11: return UiText(UiId::Text_6bc66717, L"寂静时分");
	case 12: return UiText(UiId::Text_012171a0, L"血腥收获");
	case 13: return UiText(UiId::Text_771ae2e6, L"刺骨寒溪");
	case 14: return UiText(UiId::Text_f420c849, L"临死一搏");
	default: return nullptr;
	}
}

int MapHudCampaignChapters(int camp) {
	static const int k[] = {
		0,
		4, // c1 Dead Center
		5, // c2 Dark Carnival
		4, // c3 Swamp Fever
		5, // c4 Hard Rain
		5, // c5 The Parish
		3, // c6 The Passing
		3, // c7 The Sacrifice
		5, // c8 No Mercy
		2, // c9 Crash Course
		5, // c10 Death Toll
		5, // c11 Dead Air
		5, // c12 Blood Harvest
		4, // c13 Cold Stream
		2, // c14 The Last Stand
	};
	if (camp < 1 || camp > 14) return 0;
	return k[camp];
}

void MapHudParse(const char* stem, int* camp, int* chap) {
	if (camp) *camp = 0;
	if (chap) *chap = 0;
	if (!stem || !stem[0]) return;
	if ((stem[0] == 'c' || stem[0] == 'C') && stem[1] >= '1' && stem[1] <= '9') {
		int i = 1, c = 0;
		while (stem[i] >= '0' && stem[i] <= '9') {
			c = c * 10 + (stem[i] - '0');
			++i;
		}
		if ((stem[i] == 'm' || stem[i] == 'M') && stem[i + 1] >= '1' && stem[i + 1] <= '9') {
			int j = i + 1, m = 0;
			while (stem[j] >= '0' && stem[j] <= '9') {
				m = m * 10 + (stem[j] - '0');
				++j;
			}
			if (c >= 1 && c <= 99 && m >= 1 && m <= 20 && (stem[j] == '_' || stem[j] == 0)) {
				if (camp) *camp = c;
				if (chap) *chap = m;
				return;
			}
		}
	}
	int found = 0;
	for (int i = 0; stem[i]; ++i) {
		if ((stem[i] != 'm' && stem[i] != 'M') || stem[i + 1] < '1' || stem[i + 1] > '9')
			continue;
		if (i > 0 && stem[i - 1] != '_')
			continue;
		int j = i + 1, m = 0;
		while (stem[j] >= '0' && stem[j] <= '9') {
			m = m * 10 + (stem[j] - '0');
			++j;
		}
		if (m >= 1 && m <= 20 && (stem[j] == '_' || stem[j] == 0 || stem[j] == '.'))
			found = m;
	}
	if (chap) *chap = found;
}

// Paint-owned, one mission (<=64 KiB) per 100 ms, <=256 enumeration entries.
// No recurring IO after completion on the same map. A new map must check its
// own potentially conflicting definitions. Disable/disconnect cancels on Paint.
static DlcGameGlobScan* g_mapMissionScan = nullptr;
static char g_mapMissionStem[80]{}, g_mapMissionMode[64]{};
static skmission::Chapters g_mapMissionChapters{};
static wchar_t g_mapMissionTitle[64]{}, g_mapMissionChapterName[64]{};
static int g_mapMissionChap = 0, g_mapMissionEntries = 0;
static bool g_mapMissionDone = false, g_mapMissionConflict = false;
static DWORD g_mapMissionNext = 0;

// Resolve at scan completion on Paint, never in either HUD's draw loop.
// Mission text is UTF-8; older locally encoded mission files use the ANSI fallback.
static void MapHudResolveMissionText(const char* text, wchar_t (&out)[64]) {
	out[0] = 0;
	if (!text[0]) return;
	if (text[0] == '#') {
		void* localize = GetIface("vgui2.dll", "VGUI_Localize005");
		if (!localize || !IfaceAlive(localize)) return;
		using FindFn = wchar_t*(__thiscall*)(void*, const char*);
		// Same verified interface/slot as the existing BOT-name localization path.
		const wchar_t* found = ((FindFn)(*(void***)localize)[2])(localize, text);
		if (!found || found[0] == '#') return;
		wcsncpy_s(out, found, _TRUNCATE);
	} else {
		wchar_t wide[260]{};
		if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, wide, 260)
			&& !MultiByteToWideChar(CP_ACP, 0, text, -1, wide, 260)) return;
		wcsncpy_s(out, wide, _TRUNCATE);
	}
	// Names occupy one line even when author metadata contains tabs/newlines.
	for (wchar_t* p = out; *p; ++p) if (*p < L' ') *p = L' ';
	size_t len = wcslen(out);
	while (len && out[len - 1] == L' ') out[--len] = 0;
	size_t first = 0; while (out[first] == L' ') ++first;
	if (first) memmove(out, out + first, (len - first + 1) * sizeof(wchar_t));
}

static void MapHudMissionFile(const char* filename, void*) {
	if (!filename || strchr(filename, '/') || strchr(filename, '\\') || strstr(filename, "..")) return;
	char path[300]{}; snprintf(path, sizeof(path), "missions/%s", filename);
	void* data = nullptr; int size = 0;
	if (!DlcReadGameBinaryLimited(nullptr, path, &data, &size, 65536)) return;
	skmission::Chapters chapters{};
	const bool valid = skmission::Parse((const char*)data, (size_t)size, g_mapMissionMode, chapters);
	free(data);
	if (!valid) return;
	const int chapter = skmission::Index(chapters, g_mapMissionStem);
	if (!chapter) return;
	if (!g_mapMissionChap) { g_mapMissionChapters = chapters; g_mapMissionChap = chapter; }
	else if (g_mapMissionChapters.count != chapters.count
		|| memcmp(g_mapMissionChapters.maps, chapters.maps, sizeof(chapters.maps))) g_mapMissionConflict = true;
	else {
		// Agreeing chapter lists can still have ambiguous display names. Preserve
		// the known count and suppress only the conflicting labels.
		if (strcmp(g_mapMissionChapters.title, chapters.title)) g_mapMissionChapters.title[0] = 0;
		if (strcmp(g_mapMissionChapters.names[chapter - 1], chapters.names[chapter - 1]))
			g_mapMissionChapters.names[chapter - 1][0] = 0;
	}
}

void MapHudMissionPump(const char* mode) {
	if (!mode || !mode[0] || !g_elimLastMap[0]) {
		if (g_mapMissionScan) { DlcGameGlobScanCancel(g_mapMissionScan); g_mapMissionScan = nullptr; }
		// Keep a finished name cache across the loading paint. Chapter stats
		// freezes the display names, and a null mode must not wipe them first.
		if (!g_mapMissionDone && g_mapMissionStem[0]) {
			g_mapMissionStem[0] = g_mapMissionMode[0] = 0;
			g_mapMissionChapters = {};
			g_mapMissionChap = 0;
			g_mapHudLayoutStem[0] = 0;
			g_mapMissionTitle[0] = g_mapMissionChapterName[0] = 0;
		}
		return;
	}
	char stem[80]{}; MapHudStem(g_elimLastMap, stem, 80);
	if (_stricmp(g_mapMissionStem, stem) || _stricmp(g_mapMissionMode, mode)) {
		if (g_mapMissionScan) { DlcGameGlobScanCancel(g_mapMissionScan); g_mapMissionScan = nullptr; }
		strncpy_s(g_mapMissionStem, stem, _TRUNCATE); strncpy_s(g_mapMissionMode, mode, _TRUNCATE);
		g_mapMissionChap = 0; g_mapMissionEntries = 0; g_mapMissionNext = 0; g_mapMissionConflict = false; g_mapMissionDone = false;
		g_mapMissionChapters = {};
		g_mapMissionTitle[0] = g_mapMissionChapterName[0] = 0;
		int camp = 0, chapter = 0; MapHudParse(stem, &camp, &chapter);
		if (camp >= 1 && camp <= 14) {
			g_mapMissionDone = true; // official fast path: no mission enumeration/IO
			if (chapter > 0) {
				char token[80]{};
				snprintf(token, sizeof(token), "#L4D360UI_LevelName_COOP_C%dM%d", camp, chapter);
				MapHudResolveMissionText(token, g_mapMissionChapterName);
			}
		}
		g_mapHudLayoutStem[0] = 0;
	}
	if (g_mapMissionDone) return;
	const DWORD now = GetTickCount(); if (g_mapMissionNext && (LONG)(now - g_mapMissionNext) < 0) return;
	g_mapMissionNext = now + 100;
	if (!g_mapMissionScan) g_mapMissionScan = DlcGameGlobScanBegin("missions/*.txt");
	if (!g_mapMissionScan) { g_mapMissionDone = true; return; }
	const int result = DlcGameGlobScanPump(g_mapMissionScan, 1, &MapHudMissionFile, nullptr);
	if (result || ++g_mapMissionEntries >= 256) {
		DlcGameGlobScanCancel(g_mapMissionScan); g_mapMissionScan = nullptr; g_mapMissionDone = true;
		// An incomplete inventory or conflicting definitions cannot establish a denominator.
		if (g_mapMissionConflict || result != 1) { g_mapMissionChap = 0; g_mapMissionChapters = {}; }
		if (g_mapMissionChap > 0) {
			MapHudResolveMissionText(g_mapMissionChapters.title, g_mapMissionTitle);
			MapHudResolveMissionText(g_mapMissionChapters.names[g_mapMissionChap - 1], g_mapMissionChapterName);
		}
		g_mapHudLayoutStem[0] = 0;
	}
}

bool MapHudChapterCounts(const char* stem, int* chapter, int* total) {
	int camp = 0; MapHudParse(stem, &camp, chapter);
	*total = MapHudCampaignChapters(camp);
	if (!*total && g_mapMissionDone && !g_mapMissionConflict
		&& !_stricmp(g_mapMissionStem, stem) && g_mapMissionChap > 0) {
		*chapter = g_mapMissionChap; *total = g_mapMissionChapters.count;
	}
	return *chapter > 0 && *total > 0;
}

const wchar_t* MapHudMissionName(const char* stem, bool chapterName) {
	if (!g_mapMissionDone || g_mapMissionConflict || _stricmp(g_mapMissionStem, stem)) return nullptr;
	const wchar_t* name = chapterName ? g_mapMissionChapterName : g_mapMissionTitle;
	return name[0] ? name : nullptr;
}

void MapHudCopyDisplayNames(const char* map, wchar_t* campaign, int campaignCap, wchar_t* chapter, int chapterCap) {
	if (campaign && campaignCap > 0) campaign[0] = 0;
	if (chapter && chapterCap > 0) chapter[0] = 0;
	if (!campaign || campaignCap <= 1 || !chapter || chapterCap <= 1) return;
	char stem[80]{}; MapHudStem(map, stem, 80);
	if (!stem[0]) return;
	int camp = 0, chap = 0; MapHudParse(stem, &camp, &chap);
	const wchar_t* campName = (camp >= 1 && camp <= 14) ? MapHudCampaignName(camp) : nullptr;
	if (!campName) campName = MapHudMissionName(stem, false);
	if (campName) wcsncpy_s(campaign, campaignCap, campName, _TRUNCATE);
	const wchar_t* level = MapHudMissionName(stem, true);
	if (level) wcsncpy_s(chapter, chapterCap, level, _TRUNCATE);
	else if (camp >= 1 && camp <= 14 && chap > 0) {
		char token[80]{};
		snprintf(token, sizeof(token), "#L4D360UI_LevelName_COOP_C%dM%d", camp, chap);
		wchar_t resolved[64]{};
		MapHudResolveMissionText(token, resolved);
		if (resolved[0]) wcsncpy_s(chapter, chapterCap, resolved, _TRUNCATE);
	}
}

void MapHudPaintHud() {
	const bool preview = HudMenuPage(22);
	if (!HudWantsDraw(g_optMapHud, 22)) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame()) return;
	if (!MenuEnsureSurf()) return;
	MapHudEnsureFonts();
	if (!g_mapHudFontId) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const int sc = g_mapHudScale;

	char stem[80]{};
	MapHudStem(g_elimLastMap[0] ? g_elimLastMap : nullptr, stem, 80);
	if (!stem[0] && (preview || HudLayoutPeer(g_optMapHud, 22)))
		strncpy(stem, "c1m2_streets", 79);
	if (!stem[0]) return;

	const int layoutMainPx = HudScalePx(g_mapHudLayoutFont, sc);
	int layoutSmBase = HudSmLayoutBase(g_mapHudLayoutFont);
	const int layoutSmPx = HudScalePx(layoutSmBase, sc);
	unsigned long layoutMainFont = HudAcquireLayoutFont(layoutMainPx, &g_mapLayoutFontId, &g_mapLayoutFontPx, 800);
	unsigned long layoutSmFont = HudAcquireLayoutFont(layoutSmPx, &g_mapLayoutFontSmId, &g_mapLayoutFontSmPx, 650);
	if (!layoutMainFont) return;

	const bool layoutStale = _stricmp(g_mapHudLayoutStem, stem) != 0
		|| g_mapHudLayoutScale != sc
		|| g_mapHudLayoutFontBase != g_mapHudLayoutFont;
	if (layoutStale) {
		strncpy(g_mapHudLayoutStem, stem, 79);
		g_mapHudLayoutStem[79] = 0;
		g_mapHudLayoutScale = sc;
		g_mapHudLayoutFontBase = g_mapHudLayoutFont;
		int camp = 0, chap = 0;
		MapHudParse(stem, &camp, &chap);
		int total = 0; MapHudChapterCounts(stem, &chap, &total);
		const wchar_t* campName = (camp >= 1 && camp <= 14) ? MapHudCampaignName(camp) : nullptr;
		if (!campName) campName = MapHudMissionName(stem, false);
		if (campName && chap > 0 && total > 0)
			swprintf_s(g_mapHudLayoutLine1, 80, UiText(UiId::Text_c995d810, L"%s  第%d/%d关"), campName, chap, total);
		else if (campName)
			swprintf_s(g_mapHudLayoutLine1, 80, L"%s", campName);
		else if (chap > 0 && total > 0)
			swprintf_s(g_mapHudLayoutLine1, 80, UiText(UiId::Text_3a60e4ed, L"三方图  第%d/%d关"), chap, total);
		else if (chap > 0)
			swprintf_s(g_mapHudLayoutLine1, 80, UiText(UiId::Text_55c6fb10, L"三方图  第%d关"), chap);
		else
			swprintf_s(g_mapHudLayoutLine1, 80, UiText(UiId::Text_18d473a8, L"三方图"));
		const wchar_t* chapterName = MapHudMissionName(stem, true);
		if (chapterName)
			swprintf_s(g_mapHudLayoutLine2, 80, L"%s", chapterName);
		else if (camp > 0 && chap > 0)
			swprintf_s(g_mapHudLayoutLine2, 80, L"c%dm%d  %S", camp, chap, stem);
		else
			swprintf_s(g_mapHudLayoutLine2, 80, L"%S", stem);
		g_mapHudLayoutW1 = SurfTextWFont(layoutMainFont, g_mapHudLayoutLine1);
		g_mapHudLayoutW2 = layoutSmFont ? SurfTextWFont(layoutSmFont, g_mapHudLayoutLine2) : g_mapHudLayoutW1;
		g_mapHudLayoutH1 = layoutMainPx + 2;
		g_mapHudLayoutH2 = layoutSmPx + 2;
		g_mapHudLayoutInner = g_mapHudLayoutW1;
		if (g_mapHudLayoutW2 > g_mapHudLayoutInner) g_mapHudLayoutInner = g_mapHudLayoutW2;
		if (g_mapHudLayoutInner < 128) g_mapHudLayoutInner = 128;
		const int padX = 14;
		const int padY = 10;
		const int barH = 4;
		g_mapHudLayoutPanelW = padX * 2 + g_mapHudLayoutInner;
		g_mapHudLayoutPanelH = padY * 2 + barH + 6 + g_mapHudLayoutH1 + 4 + g_mapHudLayoutH2 + 2;
		g_mapHudLayoutChapPct = (chap > 0 && total > 0) ? chap * 100 / total : 0;
		g_mapHudLayoutGaugeSegs = (total > 0 && total <= 12) ? total : 8;
	}
	const wchar_t* line1 = g_mapHudLayoutLine1;
	const wchar_t* line2 = g_mapHudLayoutLine2;
	const int w1 = g_mapHudLayoutW1;
	const int w2 = g_mapHudLayoutW2;
	const int h1 = g_mapHudLayoutH1;
	const int h2 = g_mapHudLayoutH2;
	const int inner = g_mapHudLayoutInner;
	const int panelW = HudScalePx(g_mapHudLayoutPanelW, sc);
	const int panelH = HudScalePx(g_mapHudLayoutPanelH, sc);
	const int chapPct = g_mapHudLayoutChapPct;
	const int gaugeSegs = g_mapHudLayoutGaugeSegs;
	const int padX = HudScalePx(14, sc);
	const int padY = HudScalePx(10, sc);
	const int barH = 4;
	int x = 0, y = 0;
	HudFinalizePos(&g_mapHudAbs, &g_mapHudOffX, &g_mapHudOffY, g_mapHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(148, 112, 255);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 10, accent, preview ? 200 : 120);

	const int tx = x + padX;
	int ty = y + padY;
	int tagW = 0;
	if (g_mapHudFontSmId)
		tagW = SurfTextWFont(g_mapHudFontSmId, L"MAP") + 10;
	const int gaugeW = (tagW > 0 && inner > tagW + 48) ? (inner - tagW - 4) : inner;
	HudSegGauge(tx, ty, gaugeW, barH, chapPct, accent, gaugeSegs);
	if (g_mapHudFontSmId && tagW > 0) {
		HudTagPill(tx + inner - tagW + 4, ty - 1, g_mapHudFontSmId, L"MAP",
			RGB(220, 210, 255), RGB(100, 72, 180));
	}
	ty += barH + 6;
	SurfTextAt(g_mapHudFontId, tx, ty, RGB(236, 228, 255), line1);
	ty += h1 + 2;
	if (g_mapHudFontSmId)
		SurfTextAt(g_mapHudFontSmId, tx, ty, RGB(170, 158, 200), line2);
	if (preview) {
		SurfColor(148, 112, 255, 170);
		SurfOutline(x, y, x + panelW, y + panelH);
	}
}

void TeamHudPlayerInfoCacheClear() {
	memset(g_teamHudPlayerInfoCache, 0, sizeof(g_teamHudPlayerInfoCache));
	memset(g_teamHudPlayerInfoValid, 0, sizeof(g_teamHudPlayerInfoValid));
}

const PlayerInfo* TeamHudPlayerInfoCached(int entIdx) {
	if (entIdx <= 0 || entIdx >= (int)(sizeof(g_teamHudPlayerInfoCache) / sizeof(g_teamHudPlayerInfoCache[0])))
		return nullptr;
	return g_teamHudPlayerInfoValid[entIdx] ? &g_teamHudPlayerInfoCache[entIdx] : nullptr;
}

void TeamHudNameCacheClear() {
	g_teamHudNameCacheN = 0;
}

const wchar_t* TeamHudNameCacheFind(int ent, int userid) {
	for (int i = 0; i < g_teamHudNameCacheN; ++i) {
		if (userid > 0 && g_teamHudNameCache[i].userid == userid && g_teamHudNameCache[i].fromPlayer)
			return g_teamHudNameCache[i].name;
		if (ent > 0 && g_teamHudNameCache[i].ent == ent && g_teamHudNameCache[i].fromPlayer)
			return g_teamHudNameCache[i].name;
	}
	return nullptr;
}

void TeamHudNameCachePut(int ent, int userid, const wchar_t* name, bool fromPlayer) {
	if (!name || !name[0]) return;
	for (int i = 0; i < g_teamHudNameCacheN; ++i) {
		const bool sameUid = (userid > 0 && g_teamHudNameCache[i].userid == userid);
		const bool sameEnt = (ent > 0 && g_teamHudNameCache[i].ent == ent);
		if (!sameUid && !sameEnt) continue;
		g_teamHudNameCache[i].ent = ent;
		if (userid > 0) g_teamHudNameCache[i].userid = userid;
		wcsncpy(g_teamHudNameCache[i].name, name, 23);
		g_teamHudNameCache[i].name[23] = 0;
		if (fromPlayer) g_teamHudNameCache[i].fromPlayer = true;
		return;
	}
	if (g_teamHudNameCacheN >= 16) return;
	auto& e = g_teamHudNameCache[g_teamHudNameCacheN++];
	e.ent = ent;
	e.userid = userid;
	wcsncpy(e.name, name, 23);
	e.name[23] = 0;
	e.fromPlayer = fromPlayer;
}

const wchar_t* TeamHudIdleNameForBot(int ent, int userid) {
	for (int i = 0; i < kTeamHudMax; ++i) {
		TeamHudIdleLink& link = g_teamHudIdleLinks[i];
		if (!((userid > 0 && link.botUserId == userid) || (ent > 0 && link.botEnt == ent)))
			continue;
		int humanEnt = link.playerEnt;
		if (humanEnt <= 0) humanEnt = EngPlayerForUserID(link.playerUserId);
		PlayerInfo pi{};
		if (humanEnt > 0 && EngPlayerInfo(humanEnt, &pi)
			&& pi.userid == link.playerUserId && !PlayerInfoIsBot(pi)) {
			link.playerEnt = humanEnt;
			return link.playerName[0] ? link.playerName : nullptr;
		}
		if (!link.born || GetTickCount() - link.born > 1500u)
			link = {};
		return nullptr;
	}
	return nullptr;
}

int TeamHudIdleBodyForEnt(int ent) {
	if (ent <= 0) return 0;
	for (int i = 0; i < kTeamHudMax; ++i) {
		const TeamHudIdleLink& link = g_teamHudIdleLinks[i];
		if (link.playerEnt == ent && link.botEnt > 0)
			return link.botEnt;
	}
	return 0;
}

void TeamHudOnBotSwap(const char* eventName, int playerUserId, int botUserId) {
	if (!g_optTeamHud) return;
	if (!eventName || playerUserId <= 0 || botUserId <= 0) return;
	const bool takeover = eventName[0] == 'b';
	const int playerEnt = EngPlayerForUserID(playerUserId);
	const int botEnt = EngPlayerForUserID(botUserId);
	int slot = -1;
	for (int i = 0; i < kTeamHudMax; ++i) {
		if (g_teamHudIdleLinks[i].playerUserId == playerUserId
			|| g_teamHudIdleLinks[i].botUserId == botUserId) {
			slot = i;
			break;
		}
		if (slot < 0 && !g_teamHudIdleLinks[i].playerUserId)
			slot = i;
	}
	if (takeover) {
		if (slot >= 0 && g_teamHudIdleLinks[slot].playerUserId)
			g_teamHudIdleLinks[slot] = {};
		return;
	}
	if (slot < 0) slot = 0;
	TeamHudIdleLink& link = g_teamHudIdleLinks[slot];
	link = {};
	link.playerUserId = playerUserId;
	link.botUserId = botUserId;
	link.playerEnt = playerEnt;
	link.botEnt = botEnt;
	link.born = GetTickCount();
	PlayerInfo pi{};
	if (playerEnt > 0 && EngPlayerInfo(playerEnt, &pi) && pi.name[0]) {
		MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, link.playerName, 24);
		link.playerName[23] = 0;
	}
	if (!link.playerName[0]) {
		const wchar_t* cached = TeamHudNameCacheFind(playerEnt, playerUserId);
		if (cached) wcscpy_s(link.playerName, 24, cached);
	}
}

void TeamHudApplyName(TeamHudSlot& s, int entIdx) {
	wchar_t playerName[24]{};
	bool gotPlayer = false;
	PlayerInfo pi{};
	const PlayerInfo* cachedPi = TeamHudPlayerInfoCached(entIdx);
	const bool havePi = cachedPi ? (pi = *cachedPi, true) : EngPlayerInfo(entIdx, &pi);
	if (havePi && pi.userid > 0)
		s.userid = pi.userid;
	if (havePi && PlayerInfoIsBot(pi)) {
		const wchar_t* idleName = TeamHudIdleNameForBot(entIdx, pi.userid);
		if (idleName && idleName[0]) {
			swprintf_s(s.name, 24, UiText(UiId::Text_07846cc7, L"闲置：%.19s"), idleName);
			return;
		}
	}
	// The stock HUD uses survivor localization tokens for BOTs. Skeeto's own HUD
	// keeps this explicit wide-string path because it renders its cached snapshot.
	if (havePi && PlayerInfoIsBot(pi) && g_optClientBotNames
		&& s.character >= 0 && s.character < kBotNameCount
		&& g_clientBotNames[s.character][0]) {
		wcscpy_s(s.name, 24, g_clientBotNames[s.character]);
		return;
	}
	if (havePi && pi.name[0]) {
		MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, playerName, 24);
		playerName[23] = 0;
		gotPlayer = true;
	}
	if (gotPlayer) {
		wcsncpy(s.name, playerName, 23);
		s.name[23] = 0;
		TeamHudNameCachePut(entIdx, s.userid, s.name, true);
		return;
	}
	const wchar_t* cached = TeamHudNameCacheFind(entIdx, s.userid);
	if (cached && cached[0]) {
		wcsncpy(s.name, cached, 23);
		s.name[23] = 0;
		return;
	}
	wcsncpy(s.name, TeamHudCharName(s.character), 23);
	s.name[23] = 0;
	TeamHudNameCachePut(entIdx, s.userid, s.name, false);
}

int TeamHudReadHandleIdx(void* e, int off) {
	if (!e || off < 0 || off > 0x4000) return 0;
	int* p = (int*)((uint8_t*)e + off);
	if (!PtrCommitted(p)) return 0;
	return EntIndexFromHandle(*p);
}

bool TeamHudHandleLive(void* e, int off) {
	const int idx = TeamHudReadHandleIdx(e, off);
	if (idx <= 0) return false;
	return EntReadable(EntGet(idx));
}

bool TeamHudReadBool(void* e, int off) {
	if (!e || off < 0 || off > 0x4000) return false;
	unsigned char* p = (unsigned char*)e + off;
	if (!PtrCommitted(p)) return false;
	return *p != 0;
}

float TeamHudReadProgressFrac(void* e, float* remainOut) {
	if (remainOut) *remainOut = -1.f;
	if (!e || g_offProgStart < 0 || g_offProgDur < 0) return -1.f;
	if (g_offProgStart > 0x4000 || g_offProgDur > 0x4000) return -1.f;
	float* ps = (float*)((uint8_t*)e + g_offProgStart);
	float* pd = (float*)((uint8_t*)e + g_offProgDur);
	if (!PtrCommitted(ps) || !PtrCommitted(pd)) return -1.f;
	const float dur = *pd;
	if (!(dur >= 0.2f && dur <= 30.f)) return -1.f;
	const float elapsed = EngLastTimeStamp() - *ps;
	float t = elapsed / dur;
	if (t < 0.f) t = 0.f;
	if (t > 1.f) t = 1.f;
	if (remainOut) {
		float remain = dur - elapsed;
		*remainOut = (remain > 0.f) ? remain : 0.f;
	}
	return t;
}

void TeamHudFillTag(int ent, int* uid, wchar_t* name, int nameN) {
	if (uid) *uid = 0;
	if (name && nameN > 0) name[0] = 0;
	if (ent <= 0) return;
	PlayerInfo pi{};
	if (!EngPlayerInfo(ent, &pi)) return;
	if (uid && pi.userid > 0) *uid = pi.userid;
	if (name && nameN > 0 && pi.name[0]) {
		MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, name, nameN);
		name[nameN - 1] = 0;
	}
}

bool TeamHudHas(const char* cn, const char* s) {
	return cn && s && strstr(cn, s) != nullptr;
}

bool TeamHudCnIHas(const char* cn, const char* needle) {
	if (!cn || !needle || !needle[0]) return false;
	for (const char* p = cn; *p; ++p) {
		const char* a = p;
		const char* b = needle;
		while (*a && *b) {
			char ca = (char)tolower((unsigned char)*a);
			char cb = (char)tolower((unsigned char)*b);
			if (ca != cb) break;
			++a;
			++b;
		}
		if (!*b) return true;
	}
	return false;
}

bool TeamHudIsHuntRifle(const char* cn) {
	if (!cn || !cn[0]) return false;
	// EntNetClassName is often mixed case; "rifle" alone must not beat hunt.
	if (TeamHudCnIHas(cn, "hunting")) return true;
	return TeamHudCnIHas(cn, "sniper_hunting")
		|| TeamHudCnIHas(cn, "hunting_rifle")
		|| TeamHudCnIHas(cn, "huntingrifle")
		|| TeamHudCnIHas(cn, "sniperhunting");
}

int TeamHudWepKind(const char* cn) {
	if (!cn || !cn[0]) return TW_None;
	if (TeamHudIsHuntRifle(cn)) return TW_Hunt;
	if (TeamHudHas(cn, "Molotov") || TeamHudHas(cn, "molotov")) return TW_Molly;
	if (TeamHudHas(cn, "PipeBomb") || TeamHudHas(cn, "Pipebomb") || TeamHudHas(cn, "pipe_bomb")
		|| TeamHudHas(cn, "pipebomb") || TeamHudHas(cn, "Pipe_bomb"))
		return TW_Pipe;
	if (TeamHudHas(cn, "Vomit") || TeamHudHas(cn, "vomitjar") || TeamHudHas(cn, "VomitJar")
		|| TeamHudHas(cn, "Bile") || TeamHudHas(cn, "bile_flask"))
		return TW_Bile;
	if (TeamHudHas(cn, "Defib") || TeamHudHas(cn, "defibrillator")) return TW_Defib;
	if (TeamHudHas(cn, "FirstAid") || TeamHudHas(cn, "first_aid") || TeamHudHas(cn, "Medkit")
		|| TeamHudHas(cn, "MedKit"))
		return TW_Kit;
	if (TeamHudHas(cn, "Incendiary") || TeamHudHas(cn, "upgradepack_incendiary")) return TW_Inc;
	if (TeamHudHas(cn, "Explosive") || TeamHudHas(cn, "upgradepack_explosive")) return TW_Exp;
	if (TeamHudHas(cn, "Adrenaline") || TeamHudHas(cn, "adrenaline")) return TW_Adren;
	if (TeamHudHas(cn, "PainPills") || TeamHudHas(cn, "pain_pills") || TeamHudHas(cn, "Pain_pills")
		|| TeamHudHas(cn, "Pills"))
		return TW_Pills;
	if (TeamHudHas(cn, "AK47") || TeamHudHas(cn, "Ak47")) return TW_Ak;
	if (TeamHudHas(cn, "SG552") || TeamHudHas(cn, "Sg552")) return TW_Sg;
	if (TeamHudHas(cn, "Desert") || TeamHudHas(cn, "SCAR") || TeamHudHas(cn, "Scar")) return TW_Scar;
	if (TeamHudHas(cn, "M60") || TeamHudHas(cn, "m60")) return TW_M60;
	if (TeamHudHas(cn, "GrenadeLauncher") || TeamHudHas(cn, "grenade_launcher")) return TW_GL;
	if (TeamHudHas(cn, "Sniper_AWP") || TeamHudHas(cn, "AWP") || TeamHudHas(cn, "awp")) return TW_Awp;
	if (TeamHudHas(cn, "Scout") || TeamHudHas(cn, "scout")) return TW_Scout;
	if (TeamHudHas(cn, "Military") || TeamHudHas(cn, "sniper_military")) return TW_Mil;
	if (TeamHudHas(cn, "MP5") || TeamHudHas(cn, "Mp5") || TeamHudHas(cn, "mp5")) return TW_Mp5;
	if (TeamHudHas(cn, "Silenced") || TeamHudHas(cn, "smg_silenced")) return TW_SmgS;
	if (TeamHudHas(cn, "Spas") || TeamHudHas(cn, "SPAS") || TeamHudHas(cn, "shotgun_spas")) return TW_Spas;
	if (TeamHudHas(cn, "Chrome") || TeamHudHas(cn, "shotgun_chrome")) return TW_Chrome;
	if (TeamHudHas(cn, "AutoShotgun") || TeamHudHas(cn, "autoshotgun") || TeamHudHas(cn, "Autoshotgun")) return TW_Auto;
	if (TeamHudHas(cn, "Pump") || TeamHudHas(cn, "pumpshotgun")) return TW_Pump;
	if (TeamHudHas(cn, "Magnum") || TeamHudHas(cn, "Pistol_Magnum") || TeamHudHas(cn, "pistol_magnum")) return TW_Magnum;
	if (TeamHudHas(cn, "Chainsaw") || TeamHudHas(cn, "chainsaw")) return TW_Saw;
	if (TeamHudHas(cn, "Melee") || TeamHudHas(cn, "melee")) return TW_Melee;
	if (TeamHudHas(cn, "SubMachine") || TeamHudHas(cn, "SMG") || TeamHudHas(cn, "Smg") || TeamHudHas(cn, "smg")) return TW_Smg;
	if ((TeamHudHas(cn, "Rifle") || TeamHudHas(cn, "rifle") || TeamHudHas(cn, "M16") || TeamHudHas(cn, "m16"))
		&& !TeamHudIsHuntRifle(cn))
		return TW_M16;
	if (TeamHudHas(cn, "Pistol") || TeamHudHas(cn, "pistol")) return TW_Pistol;
	if (TeamHudHas(cn, "GasCan") || TeamHudHas(cn, "Gnome") || TeamHudHas(cn, "Cola")
		|| TeamHudHas(cn, "Firework") || TeamHudHas(cn, "Oxygen") || TeamHudHas(cn, "Propane"))
		return TW_Carry;
	return TW_None;
}

int TeamHudWepKindFromDeathWeapon(const char* weapon) {
	if (!weapon || !weapon[0]) return TW_None;
	char buf[64];
	int n = 0;
	for (; weapon[n] && n < (int)sizeof(buf) - 1; ++n)
		buf[n] = (char)tolower((unsigned char)weapon[n]);
	buf[n] = 0;
	// Same priority as TeamHudWepKind; death strings are usually lowercase.
	if (strstr(buf, "hunting_rifle") || strstr(buf, "sniper_hunting") || strstr(buf, "huntingrifle"))
		return TW_Hunt;
	if (strstr(buf, "molotov")) return TW_Molly;
	if (strstr(buf, "pipe_bomb") || strstr(buf, "pipebomb")) return TW_Pipe;
	if (strstr(buf, "vomitjar") || strstr(buf, "bile")) return TW_Bile;
	if (strstr(buf, "defib")) return TW_Defib;
	if (strstr(buf, "first_aid") || strstr(buf, "medkit")) return TW_Kit;
	if (strstr(buf, "upgradepack_incendiary") || strstr(buf, "incendiary")) return TW_Inc;
	if (strstr(buf, "upgradepack_explosive") || (strstr(buf, "explosive") && strstr(buf, "upgrade")))
		return TW_Exp;
	if (strstr(buf, "adrenaline")) return TW_Adren;
	if (strstr(buf, "pain_pills") || strstr(buf, "painpills")) return TW_Pills;
	if (strstr(buf, "ak47")) return TW_Ak;
	if (strstr(buf, "sg552")) return TW_Sg;
	if (strstr(buf, "rifle_desert") || strstr(buf, "desert_rifle") || strstr(buf, "scar")) return TW_Scar;
	if (strstr(buf, "m60")) return TW_M60;
	if (strstr(buf, "grenade_launcher")) return TW_GL;
	if (strstr(buf, "awp")) return TW_Awp;
	if (strstr(buf, "scout")) return TW_Scout;
	if (strstr(buf, "sniper_military") || strstr(buf, "military")) return TW_Mil;
	if (strstr(buf, "mp5")) return TW_Mp5;
	if (strstr(buf, "smg_silenced") || strstr(buf, "silenced")) return TW_SmgS;
	if (strstr(buf, "shotgun_spas") || strstr(buf, "spas")) return TW_Spas;
	if (strstr(buf, "shotgun_chrome") || strstr(buf, "chrome")) return TW_Chrome;
	if (strstr(buf, "autoshotgun")) return TW_Auto;
	if (strstr(buf, "pumpshotgun") || strstr(buf, "pump")) return TW_Pump;
	if (strstr(buf, "magnum")) return TW_Magnum;
	if (strstr(buf, "chainsaw")) return TW_Saw;
	if (strstr(buf, "melee") || strstr(buf, "fireaxe") || strstr(buf, "katana")
		|| strstr(buf, "machete") || strstr(buf, "crowbar") || strstr(buf, "guitar")
		|| strstr(buf, "baseball_bat") || strstr(buf, "cricket_bat") || strstr(buf, "tonfa")
		|| strstr(buf, "frying_pan") || strstr(buf, "knife") || strstr(buf, "golfclub")
		|| strstr(buf, "pitchfork") || strstr(buf, "shovel"))
		return TW_Melee;
	if (strstr(buf, "smg")) return TW_Smg;
	if (strstr(buf, "rifle") || strstr(buf, "m16")) return TW_M16;
	if (strstr(buf, "pistol") || strstr(buf, "dual_pistols")) return TW_Pistol;
	return TW_None;
}

bool TeamHudIsGun(int k) {
	return k >= TW_Pistol && k <= TW_GL;
}

int TeamHudGunMaxClipKind(int kind) {
	switch (kind) {
	case TW_Pistol: return 15;
	case TW_Magnum: return 8;
	case TW_Smg:
	case TW_SmgS:
	case TW_Mp5:
	case TW_M16:
	case TW_Sg:
		return 50;
	case TW_Ak:
		return 40;
	case TW_Scar:
		return 60;
	case TW_Pump:
	case TW_Chrome:
		return 8;
	case TW_Auto:
	case TW_Spas:
		return 10;
	case TW_M60:
		return 150;
	case TW_GL:
		return 1;
	case TW_Hunt:
	case TW_Scout:
		return 15;
	case TW_Mil:
		return 30;
	case TW_Awp:
		return 20;
	default:
		return 0;
	}
}

COLORREF AmmoHudClipColor(int clip, int maxClip, COLORREF healthy) {
	if (clip < 0) return healthy;
	if (maxClip <= 0) maxClip = clip > 0 ? clip : 1;
	int yellowTh = (maxClip * 20 + 99) / 100;
	int redTh = (maxClip * 10 + 99) / 100;
	if (yellowTh < 1) yellowTh = 1;
	if (redTh < 1) redTh = 1;
	if (clip <= redTh) return RGB(255, 72, 72);
	if (clip <= yellowTh) return RGB(255, 196, 72);
	return healthy;
}

bool TeamHudIsSide(int k) {
	return k == TW_Pistol || k == TW_Magnum || k == TW_Melee || k == TW_Saw;
}

bool TeamHudIsPrimary(int k) {
	return k >= TW_Smg && k <= TW_GL;
}

bool TeamHudIsNade(int k) { return k == TW_Molly || k == TW_Pipe || k == TW_Bile; }

bool TeamHudIsKit(int k) { return k == TW_Kit || k == TW_Defib || k == TW_Exp || k == TW_Inc; }

bool TeamHudIsPills(int k) { return k == TW_Pills || k == TW_Adren; }

bool TeamHudIsItemKind(int kind) {
	return TeamHudIsNade(kind) || TeamHudIsKit(kind) || TeamHudIsPills(kind);
}

const wchar_t* TeamHudWepLabel(int k) {
	switch (k) {
	case TW_Pistol: return UiText(UiId::Text_7f1448b1, L"手枪");
	case TW_Magnum: return UiText(UiId::Text_a129e9f1, L"马格南");
	case TW_Melee: return UiText(UiId::Text_2283d3cc, L"近战");
	case TW_Saw: return UiText(UiId::Text_50b93562, L"电锯");
	case TW_Smg: return UiText(UiId::Text_93f3cca9, L"冲锋枪");
	case TW_SmgS: return UiText(UiId::Text_05f424aa, L"消音冲锋");
	case TW_Mp5: return L"MP5";
	case TW_Pump: return UiText(UiId::Text_49a41c07, L"木喷");
	case TW_Chrome: return UiText(UiId::Text_826b934e, L"铁喷");
	case TW_Auto: return UiText(UiId::Text_b6c25de4, L"连喷");
	case TW_Spas: return L"SPAS";
	case TW_M16: return L"M16";
	case TW_Ak: return L"AK47";
	case TW_Scar: return L"SCAR";
	case TW_Sg: return L"SG552";
	case TW_M60: return L"M60";
	case TW_Hunt: return UiText(UiId::Text_414e5dc6, L"猎枪");
	case TW_Mil: return UiText(UiId::Text_30aed739, L"军用狙击");
	case TW_Scout: return L"Scout";
	case TW_Awp: return L"AWP";
	case TW_GL: return UiText(UiId::Text_e20cacf7, L"榴弹");
	case TW_Molly: return UiText(UiId::Text_d5d47924, L"燃烧瓶");
	case TW_Pipe: return UiText(UiId::Text_64eedd37, L"土制");
	case TW_Bile: return UiText(UiId::Text_3341288f, L"胆汁");
	case TW_Kit: return UiText(UiId::Text_be7e1d54, L"医疗包");
	case TW_Defib: return UiText(UiId::Text_d9fa0654, L"电击器");
	case TW_Exp: return UiText(UiId::Text_5177aadc, L"高爆包");
	case TW_Inc: return UiText(UiId::Text_9ccc30d2, L"燃烧包");
	case TW_Pills: return UiText(UiId::Text_31585ad1, L"止痛药");
	case TW_Adren: return UiText(UiId::Text_d8df1ca7, L"肾上腺素");
	case TW_Carry: return UiText(UiId::Text_709c923b, L"携带物");
	default: return L"";
	}
}

int TeamHudWepMatList(int k, const char** out, int maxOut) {
	if (!out || maxOut <= 0) return 0;
	const char* a = nullptr;
	const char* b = nullptr;
	const char* c = nullptr;
	const char* d = nullptr;
	switch (k) {
	case TW_Pistol: a = "vgui/hud/icon_pistol"; break;
	case TW_Magnum: a = "vgui/hud/icon_pistol_magnum"; b = "vgui/hud/icon_pistol"; break;
	case TW_Melee: a = "vgui/hud/icon_melee"; break;
	case TW_Saw: a = "vgui/hud/icon_chainsaw"; break;
	case TW_Smg: a = "vgui/hud/icon_smg"; break;
	case TW_SmgS: a = "vgui/hud/icon_smg_silenced"; break;
	case TW_Mp5: a = "vgui/hud/icon_smg_mp5"; break;
	case TW_Pump: a = "vgui/hud/icon_pumpshotgun"; break;
	case TW_Chrome: a = "vgui/hud/icon_shotgun_chrome"; break;
	case TW_Auto: a = "vgui/hud/icon_autoshotgun"; break;
	case TW_Spas: a = "vgui/hud/icon_shotgun_spas"; break;
	case TW_M16: a = "vgui/hud/icon_rifle"; break;
	case TW_Ak: a = "vgui/hud/icon_rifle_ak47"; b = "vgui/hud/icon_ak47"; break;
	case TW_Scar: a = "vgui/hud/icon_rifle_desert"; b = "vgui/hud/icon_desert_rifle"; break;
	case TW_Sg: a = "vgui/hud/icon_rifle_sg552"; b = "vgui/hud/icon_sg552"; break;
	case TW_M60: a = "vgui/hud/icon_rifle_m60"; b = "vgui/hud/icon_m60"; break;
	case TW_Hunt: a = "vgui/hud/icon_hunting_rifle"; b = "vgui/hud/icon_sniper_hunting"; break;
	case TW_Mil: a = "vgui/hud/icon_sniper_military"; break;
	case TW_Scout: a = "vgui/hud/icon_sniper_scout"; b = "vgui/hud/icon_scout"; break;
	case TW_Awp: a = "vgui/hud/icon_sniper_awp"; b = "vgui/hud/icon_awp"; break;
	case TW_GL: a = "vgui/hud/icon_grenade_launcher"; break;
	case TW_Molly: a = "vgui/hud/icon_molotov"; b = "vgui/hud/icon_molotov_small"; break;
	case TW_Pipe: a = "vgui/hud/icon_pipebomb"; b = "vgui/hud/icon_pipe_bomb"; break;
	case TW_Bile: a = "vgui/hud/icon_vomitjar"; b = "vgui/hud/icon_bile"; break;
	case TW_Kit: a = "vgui/hud/icon_first_aid_kit"; b = "vgui/hud/icon_first_aid"; c = "vgui/hud/icon_medkit"; break;
	case TW_Defib: a = "vgui/hud/icon_defibrillator"; b = "vgui/hud/icon_defib"; break;
	case TW_Exp: a = "vgui/hud/icon_upgradepack_explosive"; break;
	case TW_Inc: a = "vgui/hud/icon_upgradepack_incendiary"; break;
	case TW_Pills: a = "vgui/hud/icon_pills"; b = "vgui/hud/icon_pain_pills"; c = "vgui/hud/icon_painpills"; break;
	case TW_Adren: a = "vgui/hud/icon_adrenaline"; b = "vgui/hud/icon_adren"; break;
	default: return 0;
	}
	int n = 0;
	if (a && n < maxOut) out[n++] = a;
	if (b && n < maxOut) out[n++] = b;
	if (c && n < maxOut) out[n++] = c;
	if (d && n < maxOut) out[n++] = d;
	return n;
}

bool TeamHudMeleeSprite(const char* stem, TeamHudSprite* out) {
	if (!stem || !stem[0] || !out) return false;
	struct MeleeMap { const char* stem; TeamHudSprite sp; };
	static const MeleeMap kMap[] = {
		{ "fireaxe", { "vgui/hud/iconsheet2", 0, 320, 128, 64 } },
		{ "baseball_bat", { "vgui/hud/iconsheet2", 128, 320, 128, 64 } },
		{ "frying_pan", { "vgui/hud/iconsheet2", 256, 320, 128, 64 } },
		{ "machete", { "vgui/hud/iconsheet2", 384, 320, 128, 64 } },
		{ "cricket_bat", { "vgui/hud/iconsheet2", 0, 384, 128, 64 } },
		{ "tonfa", { "vgui/hud/iconsheet2", 0, 448, 128, 64 } },
		{ "katana", { "vgui/hud/iconsheet2", 128, 448, 128, 64 } },
		{ "crowbar", { "vgui/hud/iconsheet2", 256, 448, 128, 64 } },
		{ "electric_guitar", { "vgui/hud/iconsheet2", 256, 256, 128, 64 } },
		{ "guitar", { "vgui/hud/iconsheet2", 256, 256, 128, 64 } },
		{ "knife", { "vgui/hud/iconsheet3", 392, 0, 128, 64 } },
		{ "golfclub", { "vgui/hud/iconsheet2", 128, 320, 128, 64 } },
		{ "pitchfork", { "vgui/hud/iconsheet2", 0, 320, 128, 64 } },
		{ "shovel", { "vgui/hud/iconsheet2", 0, 320, 128, 64 } },
	};
	for (int i = 0; i < (int)(sizeof(kMap) / sizeof(kMap[0])); ++i) {
		if (!_stricmp(stem, kMap[i].stem)) { *out = kMap[i].sp; return true; }
	}
	return false;
}

bool TeamHudSpriteFor(int kind, const char* meleeStem, int pistolClipHint, TeamHudSprite* out) {
	if (!out) return false;
	if (kind == TW_Melee) {
		// No stem → no sheet tile. Callers must use icon_melee / icon_<stem> mats.
		// Old default was katana UV, which made every unknown melee look like a katana.
		return TeamHudMeleeSprite(meleeStem, out);
	}
	switch (kind) {
	case TW_Pistol:
		if (pistolClipHint >= 16)
			*out = { "vgui/hud/iconsheet", 192, 320, 64, 64 };
		else
			*out = { "vgui/hud/iconsheet", 256, 320, 64, 64 };
		return true;
	case TW_Magnum: *out = { "vgui/hud/iconsheet", 320, 256, 64, 64 }; return true;
	case TW_Saw: *out = { "vgui/hud/iconsheet2", 384, 384, 128, 64 }; return true;
	case TW_Smg: *out = { "vgui/hud/iconsheet", 0, 384, 128, 64 }; return true;
	case TW_SmgS: *out = { "vgui/hud/iconsheet2", 256, 0, 128, 64 }; return true;
	case TW_Mp5: return false; // no distinct sheet tile (don't fake as uzi)
	case TW_Pump: *out = { "vgui/hud/iconsheet", 320, 320, 192, 64 }; return true;
	case TW_Chrome: *out = { "vgui/hud/iconsheet2", 192, 192, 192, 64 }; return true;
	case TW_Auto: *out = { "vgui/hud/iconsheet", 0, 448, 192, 64 }; return true;
	case TW_Spas: *out = { "vgui/hud/iconsheet2", 256, 64, 128, 64 }; return true;
	// L4D1 sheet names: equip_machinegun = M16, equip_rifle = hunting rifle.
	case TW_M16: *out = { "vgui/hud/iconsheet", 128, 384, 192, 64 }; return true;
	case TW_Ak: return false;
	case TW_Scar: *out = { "vgui/hud/iconsheet2", 128, 128, 128, 64 }; return true;
	case TW_Sg: return false;
	case TW_M60: return false;
	case TW_Hunt: *out = { "vgui/hud/iconsheet", 320, 384, 192, 64 }; return true;
	case TW_Mil: *out = { "vgui/hud/iconsheet2", 0, 192, 192, 64 }; return true;
	case TW_Scout: return false;
	case TW_Awp: return false;
	case TW_GL: *out = { "vgui/hud/iconsheet2", 0, 256, 191, 64 }; return true;
	case TW_Molly: *out = { "vgui/hud/iconsheet", 192, 256, 64, 64 }; return true;
	case TW_Pipe: *out = { "vgui/hud/iconsheet", 128, 256, 64, 64 }; return true;
	case TW_Bile: return false;
	// Current weapon script uses vgui/hud/icon_first_aid_kit (64x64). The older
	// base script used iconsheet 128,320; icon_medkit is a different UI texture.
	case TW_Kit: *out = { "vgui/hud/iconsheet", 128, 320, 64, 64 }; return true;
	case TW_Defib: *out = { "vgui/hud/iconsheet2", 192, 256, 64, 64 }; return true;
	case TW_Exp: *out = { "vgui/hud/iconsheet2", 448, 128, 64, 64 }; return true;
	case TW_Inc: *out = { "vgui/hud/iconsheet", 256, 448, 64, 64 }; return true;
	case TW_Pills: *out = { "vgui/hud/iconsheet", 0, 256, 64, 64 }; return true;
	case TW_Adren: *out = { "vgui/hud/iconsheet2", 384, 0, 64, 64 }; return true;
	default: return false;
	}
}

bool TeamHudLooksLikeSheet(int tw, int th) {
	// iconsheet / iconsheet2 = 512²; iconsheet3 = 512×128.
	if (tw >= 480 && th >= 480) return true;
	if (tw >= 480 && th >= 96 && tw >= th * 2) return true;
	return false;
}

static int g_teamHudIconAlpha = 255;

void TeamHudSetIconAlpha(int a) {
	if (a < 0) a = 0;
	if (a > 255) a = 255;
	g_teamHudIconAlpha = a;
}

static bool g_teamHudIconMirror = false;

void TeamHudSetIconMirror(bool on) {
	g_teamHudIconMirror = on;
}

// All icon blits end here. Mirror = swap s0/s1; DrawTexturedSubRect and its clip remap texcoords
// linearly, so a reversed range flips the image without changing the rect.
static bool TeamHudBlitUv(int x0, int y0, int x1, int y1, int id, float s0, float t0, float s1, float t1) {
	if (g_teamHudIconMirror)
		return SurfDrawTexturedUv(x0, y0, x1, y1, id, s1, t0, s0, t1);
	return SurfDrawTexturedUv(x0, y0, x1, y1, id, s0, t0, s1, t1);
}

bool TeamHudBlitIconFill(int x, int y, int w, int h, int id, float s0, float t0, float s1, float t1) {
	if (id < 0 || w < 1 || h < 1) return false;
	SurfColor(255, 255, 255, g_teamHudIconAlpha);
	return TeamHudBlitUv(x, y, x + w, y + h, id, s0, t0, s1, t1);
}

bool TeamHudBlitIconContain(int x, int y, int w, int h, int id, float s0, float t0, float s1, float t1, float tileAr) {
	if (id < 0 || w < 1 || h < 1) return false;
	float ar = tileAr > 0.05f ? tileAr : 2.5f;
	int dw = w, dh = h;
	const float boxAr = (float)w / (float)h;
	if (ar > boxAr) {
		dh = (int)(w / ar + 0.5f);
		if (dh < 1) dh = 1;
		if (dh > h) dh = h;
	} else {
		dw = (int)(h * ar + 0.5f);
		if (dw < 1) dw = 1;
		if (dw > w) dw = w;
	}
	const int dx = x + (w - dw) / 2;
	const int dy = y + (h - dh) / 2;
	SurfColor(255, 255, 255, g_teamHudIconAlpha);
	return TeamHudBlitUv(dx, dy, dx + dw, dy + dh, id, s0, t0, s1, t1);
}

bool TeamHudBlitSpriteVanilla(int x, int y, int w, int h, const TeamHudSprite& sp) {
	if (!sp.sheet || w < 1 || h < 1) return false;
	const int id = SurfHudMat(sp.sheet);
	if (id < 0) return false;
	int sheetW = 512, sheetH = 512;
	if (strstr(sp.sheet, "iconsheet3"))
		sheetH = 128;
	const float s0 = (float)sp.x / (float)sheetW;
	const float t0 = (float)sp.y / (float)sheetH;
	const float s1 = (float)(sp.x + sp.w) / (float)sheetW;
	const float t1 = (float)(sp.y + sp.h) / (float)sheetH;
	const float ar = (sp.h > 0) ? ((float)sp.w / (float)sp.h) : 2.f;
	int dw = w, dh = h;
	const float boxAr = (float)w / (float)h;
	if (ar > boxAr) {
		dh = (int)(w / ar + 0.5f);
		if (dh < 1) dh = 1;
		if (dh > h) dh = h;
	} else {
		dw = (int)(h * ar + 0.5f);
		if (dw < 1) dw = 1;
		if (dw > w) dw = w;
	}
	const int dx = x + (w - dw) / 2;
	const int dy = y + (h - dh) / 2;
	SurfColor(255, 255, 255, g_teamHudIconAlpha);
	return TeamHudBlitUv(dx, dy, dx + dw, dy + dh, id, s0, t0, s1, t1);
}

bool TeamHudBlitSpriteFill(int x, int y, int w, int h, const TeamHudSprite& sp, int sheetIdOpt) {
	// Prefer vanilla path (hardcoded sheet UV). sheetIdOpt kept for mod-aliased sheets in non-primary slots.
	if (sheetIdOpt < 0)
		return TeamHudBlitSpriteVanilla(x, y, w, h, sp);
	int tw = 0, th = 0;
	if (!SurfHudMatSize(sheetIdOpt, &tw, &th) || tw < 1 || th < 1)
		return TeamHudBlitSpriteVanilla(x, y, w, h, sp);
	// If size isn't a real sheet, UV math would sliver — fall back to vanilla sheet.
	if (!TeamHudLooksLikeSheet(tw, th))
		return TeamHudBlitSpriteVanilla(x, y, w, h, sp);
	const float s0 = (float)sp.x / (float)tw;
	const float t0 = (float)sp.y / (float)th;
	const float s1 = (float)(sp.x + sp.w) / (float)tw;
	const float t1 = (float)(sp.y + sp.h) / (float)th;
	return TeamHudBlitIconFill(x, y, w, h, sheetIdOpt, s0, t0, s1, t1);
}

const char* TeamHudFaceMat(int ch, bool incap) {
	static const char* k[] = {
		"vgui/s_panel_gambler", "vgui/s_panel_producer", "vgui/s_panel_coach", "vgui/s_panel_mechanic",
		"vgui/s_panel_namvet", "vgui/s_panel_teenangst", "vgui/s_panel_biker", "vgui/s_panel_manager"
	};
	static const char* ki[] = {
		"vgui/s_panel_gambler_incap", "vgui/s_panel_producer_incap", "vgui/s_panel_coach_incap", "vgui/s_panel_mechanic_incap",
		"vgui/s_panel_namvet_incap", "vgui/s_panel_teenangst_incap", "vgui/s_panel_biker_incap", "vgui/s_panel_manager_incap"
	};
	if (ch < 0 || ch >= 8) ch = 0;
	return incap ? ki[ch] : k[ch];
}

const wchar_t* TeamHudCharName(int ch) {
	static const wchar_t* k[] = {
		L"Nick", L"Rochelle", L"Coach", L"Ellis",
		L"Bill", L"Zoey", L"Francis", L"Louis"
	};
	if (ch < 0 || ch >= 8) return L"?";
	return k[ch];
}

const wchar_t* TeamHudPlayerSlotLabel(int slot) {
	switch (slot) {
	case 0: return UiText(UiId::Text_10676922, L"玩家2");
	case 1: return UiText(UiId::Text_097c5863, L"玩家3");
	case 2: return UiText(UiId::Text_463dcea4, L"玩家4");
	case 3: return UiText(UiId::Text_5f26ffe5, L"玩家5");
	case 4: return UiText(UiId::Text_740bac26, L"玩家6");
	case 5: return UiText(UiId::Text_6d109d67, L"玩家7");
	case 6: return UiText(UiId::Text_ea8881a8, L"玩家8");
	case 7: return UiText(UiId::Text_f393b0e9, L"玩家9");
	default: return UiText(UiId::Text_a5c9176f, L"玩家?");
	}
}

COLORREF TeamHudCharColor(int ch) {
	switch (ch) {
	case 0: return RGB(232, 148, 72);  // Nick
	case 1: return RGB(204, 92, 148);  // Rochelle
	case 2: return RGB(92, 176, 84);   // Coach
	case 3: return RGB(232, 212, 72);  // Ellis
	case 4: return RGB(148, 156, 96);  // Bill
	case 5: return RGB(232, 132, 72);  // Zoey
	case 6: return RGB(88, 144, 204);  // Francis
	case 7: return RGB(200, 184, 140); // Louis
	default: return RGB(80, 210, 230);
	}
}

void TeamHudMatFlush() {
	// Map change frees materialsystem objects behind live vgui texture IDs.
	// Drop the whole cache; Paint recreates under the per-map create budget.
	HudMatOnLevelInit();
}

float TeamHudSafeUvWidth(int kind, int tw, int th) {
	if (tw < 1 || th < 1) return 1.f;
	if (TeamHudLooksLikeSheet(tw, th))
		return 1.f; // caller uses sprite UV instead
	if (TeamHudIsItemKind(kind) || kind == TW_Pistol || kind == TW_Magnum) {
		if (tw > th + (th / 5))
			return (float)th / (float)tw; // left square only
		return 1.f;
	}
	// Guns / melee: only crop extreme dual-packs (left ≈2:1).
	if (tw >= th * 3 + th / 2)
		return (float)(th * 2) / (float)tw;
	return 1.f;
}

void TeamHudDrawPrimaryIcon(int x, int y, int w, int h, int kind) {
	TeamHudDrawWepIconLetterbox(x, y, w, h, kind, nullptr);
}

// Same path as teammate primary weapon icons: letterbox Contain + full/standalone mats.
// Do not use TeamHudDrawIconEx Fill here — small HUD slots + failed GetTextureSize
// used to crop to a barrel sliver or skip the mat entirely.
void TeamHudDrawWepIconLetterbox(int x, int y, int w, int h, int kind, const char* meleeStem) {
	TeamHudSprite sp{};
	const bool haveSp = TeamHudSpriteFor(kind, meleeStem, -1, &sp);

	const char* mats[8]{};
	char meleeMat[80]{};
	int nMat = 0;
	if (kind == TW_Melee && meleeStem && meleeStem[0]) {
		snprintf(meleeMat, sizeof(meleeMat), "vgui/hud/icon_%s", meleeStem);
		mats[nMat++] = meleeMat;
	}
	nMat += TeamHudWepMatList(kind, mats + nMat, 8 - nMat);

	for (int i = 0; i < nMat; ++i) {
		const int id = SurfHudMat(mats[i]);
		if (id < 0) continue;
		int tw = 0, th = 0;
		if (!SurfHudMatSize(id, &tw, &th) || tw <= 0 || th <= 0) continue;
		if (TeamHudLooksLikeSheet(tw, th)) {
			if (haveSp && TeamHudBlitSpriteFill(x, y, w, h, sp, id))
				return;
			continue;
		}
		float s1 = 1.f;
		float ar = (float)tw / (float)th;
		if (TeamHudIsItemKind(kind)) {
			s1 = TeamHudSafeUvWidth(kind, tw, th);
			if (s1 > 0.05f) ar = (s1 * (float)tw) / (float)th;
		}
		if (TeamHudBlitIconContain(x, y, w, h, id, 0.f, 0.f, s1, 1.f, ar))
			return;
	}

	if (haveSp && TeamHudBlitSpriteVanilla(x, y, w, h, sp))
		return;
	if (kind == TW_Melee) return;

	const wchar_t* lab = TeamHudWepLabel(kind);
	if (!lab || !lab[0] || !g_teamFontSm) return;
	SurfColor(20, 24, 32, (g_teamHudIconAlpha * 180) / 255);
	SurfFill(x, y, x + w, y + h);
	SurfTextAt(g_teamFontSm, x + 2, y + (h > 16 ? 4 : 1), RGB(230, 230, 220), lab);
}

void TeamHudDrawIconEx(int x, int y, int w, int h, int kind, const char* meleeStem, int pistolClipHint, bool vanillaOnly) {
	TeamHudSprite sp{};
	const bool haveSp = TeamHudSpriteFor(kind, meleeStem, pistolClipHint, &sp);

	if (vanillaOnly) {
		if (haveSp && TeamHudBlitSpriteVanilla(x, y, w, h, sp))
			return;
		if (kind == TW_Melee) {
			const int id = SurfHudMat("vgui/hud/icon_melee");
			int tw = 0, th = 0;
			if (id >= 0 && SurfHudMatSize(id, &tw, &th) && tw > 0 && th > 0)
				TeamHudBlitIconContain(x, y, w, h, id, 0.f, 0.f, 1.f, 1.f, (float)tw / (float)th);
			return;
		}
		const wchar_t* lab = TeamHudWepLabel(kind);
		if (!lab || !lab[0] || !g_teamFontSm) return;
		SurfColor(20, 24, 32, 180);
		SurfFill(x, y, x + w, y + h);
		SurfTextAt(g_teamFontSm, x + 2, y + (h > 16 ? 4 : 1), RGB(230, 230, 220), lab);
		return;
	}

	const char* mats[8]{};
	char meleeMat[80]{};
	int nMat = 0;
	if (kind == TW_Melee && meleeStem && meleeStem[0]) {
		snprintf(meleeMat, sizeof(meleeMat), "vgui/hud/icon_%s", meleeStem);
		mats[nMat++] = meleeMat;
	}
	nMat += TeamHudWepMatList(kind, mats + nMat, 8 - nMat);

	for (int i = 0; i < nMat; ++i) {
		const int id = SurfHudMat(mats[i]);
		if (id < 0) continue;
		int tw = 0, th = 0;
		if (!SurfHudMatSize(id, &tw, &th) || tw < 1 || th < 1)
			continue;

		if (TeamHudLooksLikeSheet(tw, th)) {
			if (haveSp && TeamHudBlitSpriteFill(x, y, w, h, sp, id))
				return;
			continue;
		}

		const float s1 = TeamHudSafeUvWidth(kind, tw, th);
		if (TeamHudBlitIconFill(x, y, w, h, id, 0.f, 0.f, s1, 1.f))
			return;
	}

	if (haveSp && TeamHudBlitSpriteVanilla(x, y, w, h, sp))
		return;
	if (kind == TW_Melee) return;

	const wchar_t* lab = TeamHudWepLabel(kind);
	if (!lab || !lab[0] || !g_teamFontSm) return;
	SurfColor(20, 24, 32, 180);
	SurfFill(x, y, x + w, y + h);
	SurfTextAt(g_teamFontSm, x + 2, y + (h > 16 ? 4 : 1), RGB(230, 230, 220), lab);
}

void TeamHudDrawIcon(int x, int y, int w, int h, int kind, const char* meleeStem) {
	TeamHudDrawIconEx(x, y, w, h, kind, meleeStem, -1, false);
}

void TeamHudResetSurvivorSetCache() {
	g_teamHudSetCached = 0;
	g_teamHudSetMap[0] = 0;
	g_teamHudSetLogged = false;
	g_teamHudNameVoted = false;
	g_modelNameCacheN = 0;
	TeamHudNameCacheClear();
}

bool TeamHudStrHasI(const char* hay, const char* needle) {
	if (!hay || !needle || !needle[0]) return false;
	for (const char* p = hay; *p; ++p) {
		const char* a = p;
		const char* b = needle;
		while (*a && *b) {
			char ca = *a, cb = *b;
			if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
			if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
			if (ca != cb) break;
			++a;
			++b;
		}
		if (!*b) return true;
	}
	return false;
}

int TeamHudOfficialSet(const char* map) {
	if (!map || !map[0]) return 0;
	const char* slash = strrchr(map, '/');
	if (!slash) slash = strrchr(map, '\\');
	const char* base = slash ? slash + 1 : map;
	if (TeamHudStrHasI(base, "l4d1")) return 1;
	if ((base[0] == 'c' || base[0] == 'C') && base[1] >= '1' && base[1] <= '9') {
		int camp = 0;
		int i = 1;
		while (base[i] >= '0' && base[i] <= '9') {
			camp = camp * 10 + (base[i] - '0');
			++i;
		}
		if (base[i] == 'm' || base[i] == 'M') {
			if (camp == 7 || camp == 8 || camp == 9 || camp == 10
				|| camp == 11 || camp == 12 || camp == 14)
				return 1;
			if (camp >= 1 && camp <= 13)
				return 2;
		}
	}
	if (TeamHudStrHasI(base, "l4d2")) return 2;
	return 0;
}

int TeamHudQuerySurvivorSet() {
	if (!g_elimLastMap[0])
		return g_teamHudSetCached;
	if (g_teamHudSetMap[0] && _stricmp(g_teamHudSetMap, g_elimLastMap) == 0)
		return g_teamHudSetCached;
	strncpy(g_teamHudSetMap, g_elimLastMap, 63);
	g_teamHudSetMap[63] = 0;
	g_teamHudSetCached = TeamHudOfficialSet(g_elimLastMap);
	return g_teamHudSetCached;
}

int TeamHudNameSetHint(const char* n) {
	if (!n || !n[0]) return 0;
	if (!_stricmp(n, "Bill") || !_stricmp(n, "Zoey")
		|| !_stricmp(n, "Francis") || !_stricmp(n, "Louis"))
		return 1;
	if (!_stricmp(n, "Nick") || !_stricmp(n, "Rochelle")
		|| !_stricmp(n, "Coach") || !_stricmp(n, "Ellis"))
		return 2;
	return 0;
}

int TeamHudFaceFromProp(int prop, bool l4d1Slots) {
	if (prop >= 4 && prop <= 7) return prop;
	if (prop < 0 || prop > 3) return -1;
	if (!l4d1Slots) return prop;
	// L4D1 set 0–3 is Bill/Zoey/Louis/Francis. L4D2 slots 4–7 are Bill/Zoey/Francis/Louis.
	static const int kL4d1ToSlot[4] = { 4, 5, 7, 6 };
	return kL4d1ToSlot[prop];
}


int TeamHudBindFace(int ch, bool incap) {
	int id = SurfHudMat(TeamHudFaceMat(ch, incap));
	if (id >= 0) return id;
	if (incap) return SurfHudMat(TeamHudFaceMat(ch, false));
	return -1;
}

int TeamHudBindSi(int cap) {
	const char* a = nullptr;
	const char* b = nullptr;
	const char* c = nullptr;
	switch (cap) {
	case kTeamCapSmoker:
		a = "vgui/hud/icon_smoker";
		b = "vgui/s_panel_smoking";
		c = "vgui/hud/l4d_icons/icon_smoker";
		break;
	case kTeamCapHunter:
		a = "vgui/hud/icon_hunter";
		b = "vgui/s_panel_pounced";
		c = "vgui/hud/l4d_icons/icon_hunter";
		break;
	case kTeamCapJockey:
		a = "vgui/hud/icon_jockey";
		b = "vgui/s_panel_ridden";
		c = "vgui/hud/l4d_icons/icon_jockey";
		break;
	case kTeamCapCharger:
	case kTeamCapChargerPummel:
		a = "vgui/hud/icon_charger";
		b = "vgui/s_panel_carried";
		c = "vgui/hud/l4d_icons/icon_charger";
		break;
	default:
		return -1;
	}
	const char* mats[3] = { a, b, c };
	for (int i = 0; i < 3; ++i) {
		if (!mats[i] || !mats[i][0]) continue;
		const int id = SurfHudMat(mats[i]);
		if (id >= 0) return id;
	}
	return -1;
}

COLORREF TeamHudCapColor(int cap) {
	switch (cap) {
	case kTeamCapSmoker: return RGB(160, 110, 62);
	case kTeamCapHunter: return RGB(72, 148, 255);
	case kTeamCapJockey: return RGB(255, 176, 64);
	case kTeamCapCharger:
	case kTeamCapChargerPummel: return RGB(72, 200, 96);
	default: return RGB(255, 90, 90);
	}
}

int CapHudColorSlot(int cap) {
	switch (cap) {
	case kTeamCapHunter: return 0;
	case kTeamCapJockey: return 1;
	case kTeamCapSmoker: return 2;
	case kTeamCapCharger:
	case kTeamCapChargerPummel: return 3;
	default: return -1;
	}
}

COLORREF CapHudColor(int cap) {
	const int slot = CapHudColorSlot(cap);
	if (slot < 0) return TeamHudCapColor(cap);
	const int choice = g_capHudColor[slot];
	if (choice <= 0 || choice > kXhairColorCount) return TeamHudCapColor(cap);
	return kXhairColorRgb[choice - 1];
}

const wchar_t* CapHudColorName(int slot) {
	if (slot < 0 || slot >= 4) return UiText(UiId::Text_bd99cc81, L"默认");
	const int choice = g_capHudColor[slot];
	if (choice <= 0 || choice > kXhairColorCount) return UiText(UiId::Text_51154652, L"特感默认");
	return kXhairColorNames[choice - 1];
}

const wchar_t* TeamHudCapLetter(int cap) {
	switch (cap) {
	case kTeamCapSmoker: return L"S";
	case kTeamCapHunter: return L"H";
	case kTeamCapJockey: return L"J";
	case kTeamCapCharger:
	case kTeamCapChargerPummel: return L"C";
	default: return L"!";
	}
}

void TeamHudDrawSiMark(int x, int y, int sz, int cap) {
	if (sz < 12 || cap <= 0) return;
	// Keep the compact SI badge (box + H/J/S/C fallback letter) in sync with
	// the corresponding captured-HUD color selected by the user.
	const COLORREF c = CapHudColor(cap);
	SurfColor(8, 6, 10, 170);
	SurfFill(x, y, x + sz, y + sz);
	SurfColor(GetRValue(c), GetGValue(c), GetBValue(c), 235);
	SurfOutline(x, y, x + sz, y + sz);
	const int id = TeamHudBindSi(cap);
	if (id >= 0) {
		SurfColor(255, 255, 255, 255);
		SurfDrawTextured(x + 2, y + 2, x + sz - 2, y + sz - 2, id);
		return;
	}
	if (g_teamFontSm)
		SurfTextAt(g_teamFontSm, x + 5, y + sz / 2 - 8, c, TeamHudCapLetter(cap));
}

void TeamHudDrawCornerTicks(int x, int y, int w, int h, COLORREF rgb, int a) {
	int len = TeamHudPx(10);
	if (len < 6) len = 6;
	int t = TeamHudPx(2);
	if (t < 1) t = 1;
	SurfColor(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb), a);
	SurfFill(x, y, x + len, y + t);
	SurfFill(x, y, x + t, y + len);
	SurfFill(x + w - len, y, x + w, y + t);
	SurfFill(x + w - t, y, x + w, y + len);
	SurfFill(x, y + h - t, x + len, y + h);
	SurfFill(x, y + h - len, x + t, y + h);
	SurfFill(x + w - len, y + h - t, x + w, y + h);
	SurfFill(x + w - t, y + h - len, x + w, y + h);
}

void TeamHudEnsureFonts() {
	if (g_teamHudFont < kTeamHudFontMin) g_teamHudFont = kTeamHudFontMin;
	if (g_teamHudFont > kTeamHudFontMax) g_teamHudFont = kTeamHudFontMax;
	const int want = TeamHudFontPx();
	int smWant = want * 5 / 6;
	if (smWant < 14) smWant = 14;
	int lgWant = want + 12;
	if (lgWant < 28) lgWant = 28;
	if (lgWant > 36) lgWant = 36;
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow;
	if (!g_teamFont || g_teamFontPx != want) {
		g_teamFont = SurfAcquireVguiFont(want, 700, flags);
		g_teamFontPx = want;
	}
	if (!g_teamFontSm || g_teamFontSmPx != smWant) {
		g_teamFontSm = SurfAcquireVguiFont(smWant, 600, flags);
		g_teamFontSmPx = smWant;
	}
	if (!g_teamFontLg || g_teamFontLgPx != lgWant) {
		g_teamFontLg = SurfAcquireVguiFont(lgWant, 800, flags);
		g_teamFontLgPx = lgWant;
	}
	const int dirWant = HudFontPx(g_dirHudFont);
	if (!g_dirFont || g_dirFontPx != dirWant) {
		HudClampFont(&g_dirHudFont);
		g_dirFont = SurfAcquireVguiFont(dirWant, 700, flags);
		g_dirFontPx = dirWant;
	}
}

const wchar_t* TeamHudMeleeLabel(const char* stem) {
	if (!stem || !stem[0]) return UiText(UiId::Text_2283d3cc, L"近战");
	if (!_stricmp(stem, "katana")) return UiText(UiId::Text_fafa9a64, L"武士刀");
	if (!_stricmp(stem, "fireaxe")) return UiText(UiId::Text_66adca8d, L"斧头");
	if (!_stricmp(stem, "machete")) return UiText(UiId::Text_13f55e5f, L"砍刀");
	if (!_stricmp(stem, "crowbar")) return UiText(UiId::Text_93928d5b, L"撬棍");
	if (!_stricmp(stem, "cricket_bat")) return UiText(UiId::Text_f6e691af, L"球棒");
	if (!_stricmp(stem, "baseball_bat")) return UiText(UiId::Text_f6e691af, L"球棒");
	if (!_stricmp(stem, "electric_guitar")) return UiText(UiId::Text_f7996760, L"吉他");
	if (!_stricmp(stem, "frying_pan")) return UiText(UiId::Text_ccef4908, L"平底锅");
	if (!_stricmp(stem, "golfclub") || !_stricmp(stem, "golf_club")) return UiText(UiId::Text_f6bd8e60, L"球杆");
	if (!_stricmp(stem, "pitchfork")) return UiText(UiId::Text_f354c80f, L"叉子");
	if (!_stricmp(stem, "shovel")) return UiText(UiId::Text_9611e90c, L"铲子");
	if (!_stricmp(stem, "tonfa")) return UiText(UiId::Text_7c315bdf, L"警棍");
	if (!_stricmp(stem, "knife") || !_stricmp(stem, "knife_t")) return UiText(UiId::Text_fd7c1e8d, L"小刀");
	if (!_stricmp(stem, "riotshield") || !_stricmp(stem, "riot_shield")) return UiText(UiId::Text_9cb786d7, L"盾牌");
	return UiText(UiId::Text_2283d3cc, L"近战");
}

bool TeamHudLocalIsSpectating() {
	return g_dbgObsMode.load(std::memory_order_relaxed) > 0
		|| g_dbgLifeState.load(std::memory_order_relaxed) > 0
		|| g_dbgLocalTeam.load(std::memory_order_relaxed) == 1;
}

const char* TeamHudModelNameInner(int modelIndex, bool skipLoadGate, bool allowWhileSpectating) {
	// Out-of-range model indices (stale weapon / wrong ent) can fault inside engine GetModel.
	if (modelIndex <= 0 || modelIndex > 8192) return nullptr;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return nullptr;
	// TeamHud inventory paint stays off while spectating; kill-feed death resolve may opt in.
	if (!allowWhileSpectating && TeamHudLocalIsSpectating()) return nullptr;
	if (!skipLoadGate && !TeamHudFullReady(false)) return nullptr;
	for (int i = 0; i < g_modelNameCacheN; ++i) {
		if (g_modelNameCache[i].idx != modelIndex) continue;
		if (g_modelNameCache[i].name[0]) return g_modelNameCache[i].name;
		g_modelNameCache[i] = g_modelNameCache[--g_modelNameCacheN];
		break;
	}
	static void* s_mi = nullptr;
	static bool s_tried = false;
	if (!s_tried) {
		s_tried = true;
		s_mi = GetIface("engine.dll", "VModelInfoClient004");
		if (!s_mi) s_mi = GetIface("engine.dll", "VModelInfoClient003");
		Log("teamhud modelinfo=%p", s_mi);
	}
	if (!s_mi || !IfaceAlive(s_mi)) return nullptr;
	using GetModelFn = void*(__thiscall*)(void*, int);
	using GetNameFn = const char*(__thiscall*)(void*, const void*);
	const char* n = nullptr;
	for (int pass = 0; pass < 2 && !n; ++pass) {
		const int gm = (pass == 0) ? 1 : 0;
		const int gn = (pass == 0) ? 3 : 2;
		auto getModel = (GetModelFn)VGet(s_mi, gm);
		auto getName = (GetNameFn)VGet(s_mi, gn);
		if (!getModel || !getName || !IsExec((void*)getModel) || !IsExec((void*)getName))
			continue;
		__try {
			void* mdl = getModel(s_mi, modelIndex);
			if (!mdl || !PtrCommitted(mdl)) continue;
			const char* t = getName(s_mi, mdl);
			if (!t || !PtrCommitted(t) || !t[0]) continue;
			if (!strstr(t, ".mdl") && !strstr(t, ".MDL")) continue;
			n = t;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			n = nullptr;
		}
	}
	if (n && g_modelNameCacheN < 32) {
		g_modelNameCache[g_modelNameCacheN].idx = modelIndex;
		strncpy(g_modelNameCache[g_modelNameCacheN].name, n, 79);
		g_modelNameCache[g_modelNameCacheN].name[79] = 0;
		++g_modelNameCacheN;
	}
	return n;
}

const char* TeamHudModelName(int modelIndex) {
	return TeamHudModelNameInner(modelIndex, false);
}

int TeamHudWepKindCorrected(int wepIdx, const char* cn) {
	int k = TeamHudWepKind(cn);
	if (k == TW_None && TeamHudIsHuntRifle(cn)) k = TW_Hunt;
	if ((k == TW_M16 || k == TW_None) && WeaponEntIsHuntRifle(wepIdx)) k = TW_Hunt;
	return k;
}

bool TeamHudMeleeStemFromModel(const char* model, char* out, int outN) {
	if (!model || !out || outN < 4) return false;
	const char* slash = strrchr(model, '/');
	if (!slash) slash = strrchr(model, '\\');
	const char* base = slash ? slash + 1 : model;
	if ((base[0] == 'w' || base[0] == 'W' || base[0] == 'v' || base[0] == 'V') && base[1] == '_')
		base += 2;
	int n = 0;
	for (; base[n] && n < outN - 1; ++n) {
		if (base[n] == '.') break;
		char c = base[n];
		if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
		out[n] = c;
	}
	out[n] = 0;
	if (n <= 0) return false;
	if (!_stricmp(out, "knife_t")) strncpy(out, "knife", outN - 1);
	else if (!_stricmp(out, "bat")) strncpy(out, "baseball_bat", outN - 1);
	else if (!_stricmp(out, "golf_club")) strncpy(out, "golfclub", outN - 1);
	else if (!_stricmp(out, "riot_shield")) strncpy(out, "riotshield", outN - 1);
	out[outN - 1] = 0;
	return true;
}

void TeamHudFillMeleeStem(int wepIdx, char* out, int outN, bool forDeathEvent) {
	if (!out || outN <= 0) return;
	out[0] = 0;
	if (wepIdx <= 0 || wepIdx >= 2048 || g_offModelIndex < 0 || g_offModelIndex > 0x4000) return;
	void* wep = EntGet(wepIdx);
	if (!EntReadable(wep)) return;
	// Spectators often lack full weapon networkability; death events still need the model stem.
	if (!forDeathEvent && !EntNetworkable(wepIdx)) return;
	int* pm = (int*)((uint8_t*)wep + g_offModelIndex);
	if (!PtrCommitted(pm) || *pm <= 0 || *pm > 8192) return;
	const char* model = forDeathEvent
		? TeamHudModelNameInner(*pm, true, true)
		: TeamHudModelName(*pm);
	if (!model) return;
	TeamHudMeleeStemFromModel(model, out, outN);
}

void TeamHudReadGunAmmo(void* owner, int wepIdx, int kind, int* clip, int* ammo) {
	if (!owner || wepIdx <= 0 || wepIdx >= 2048 || !TeamHudIsGun(kind) || !clip) return;
	void* wep = EntGet(wepIdx);
	if (!EntReadable(wep) || !EntNetworkable(wepIdx)) return;
	if (g_offClip1 >= 0 && g_offClip1 <= 0x4000) {
		int* pc = (int*)((uint8_t*)wep + g_offClip1);
		if (PtrCommitted(pc) && *pc >= 0 && *pc <= 255)
			*clip = *pc;
	}
	if (!ammo) return;
	if (g_offAmmoType >= 0 && g_offAmmoType <= 0x4000 && g_offAmmo >= 0 && g_offAmmo <= 0x4000) {
		int* pt = (int*)((uint8_t*)wep + g_offAmmoType);
		if (PtrCommitted(pt)) {
			const int t = *pt;
			if (t >= 0 && t < 32) {
				int* pa = (int*)((uint8_t*)owner + g_offAmmo + t * 4);
				if (PtrCommitted(pa) && *pa >= 0 && *pa <= 999)
					*ammo = *pa;
			}
		}
	}
}

void TeamHudReadWeps(void* e, TeamHudSlot* s) {
	s->wep = TW_None;
	s->side = TW_None;
	s->clip = -1;
	s->ammo = -1;
	s->sideClip = -1;
	s->priWepIdx = 0;
	s->sideWepIdx = 0;
	s->nade = TW_None;
	s->nadeValid = false;
	s->kit = TW_None;
	s->pills = TW_None;
	s->melee[0] = 0;
	if (TeamHudLocalIsSpectating()) return;
	if (!e || !EntReadable(e)) return;
	int pri = TW_None, side = TW_None;
	int priIdx = 0, sideIdx = 0;
	if (g_offMyWeapons >= 0 && g_offMyWeapons <= 0x4000) {
		bool inventoryValid = true;
		for (int i = 0; i < 16; ++i) {
			int* ph = (int*)((uint8_t*)e + g_offMyWeapons + i * 4);
			if (!PtrCommitted(ph)) {
				inventoryValid = false;
				break;
			}
			const int idx = EntIndexFromHandle(*ph);
			if (idx <= 0) continue;
			void* wep = EntGet(idx);
			if (!EntNetworkable(idx) || !EntReadable(wep)) {
				// A just-thrown weapon can retain a stale handle for one snapshot.
				// It is not evidence that the whole owner inventory is unreadable.
				continue;
			}
			const char* wcn = EntNetClassName(idx);
			if (!wcn || !wcn[0]) {
				// Other slots may be transiently non-networked while inventory state
				// remains usable. Do not suppress a confirmed throwable disappearance.
				continue;
			}
			int k = TeamHudWepKindCorrected(idx, wcn);
			if (k == TW_None) continue;
			if (TeamHudIsNade(k)) { s->nade = k; continue; }
			if (TeamHudIsKit(k)) { s->kit = k; continue; }
			if (TeamHudIsPills(k)) { s->pills = k; continue; }
			if (TeamHudIsSide(k)) {
				side = k;
				sideIdx = idx;
				if (k == TW_Melee)
					TeamHudFillMeleeStem(idx, s->melee, 24);
				continue;
			}
			if (TeamHudIsPrimary(k)) { pri = k; priIdx = idx; }
		}
		s->nadeValid = inventoryValid;
	}
	if (pri != TW_None)
		TeamHudReadGunAmmo(e, priIdx, pri, &s->clip, &s->ammo);
	else if (side != TW_None)
		TeamHudReadGunAmmo(e, sideIdx, side, &s->clip, &s->ammo);
	if (side != TW_None && TeamHudIsGun(side))
		TeamHudReadGunAmmo(e, sideIdx, side, &s->sideClip, nullptr);
	s->priWepIdx = priIdx;
	s->sideWepIdx = sideIdx;
	s->wep = pri;
	s->side = side;
}

bool TeamHudFullReady(bool menuPreview) {
	if (menuPreview) return true;
	return g_engine && EngInGame() && !EngDrawingLoading();
}

void TeamHudCopyGear(TeamHudSlot* dst, const TeamHudSlot& src) {
	if (!dst) return;
	dst->wep = src.wep;
	dst->side = src.side;
	dst->clip = src.clip;
	dst->ammo = src.ammo;
	dst->sideClip = src.sideClip;
	dst->priWepIdx = src.priWepIdx;
	dst->sideWepIdx = src.sideWepIdx;
	dst->nade = src.nade;
	dst->nadeValid = src.nadeValid;
	dst->kit = src.kit;
	dst->pills = src.pills;
	memcpy(dst->melee, src.melee, sizeof(dst->melee));
}

void TeamHudReadWepsCached(void* e, int entIdx, TeamHudSlot* s,
	DWORD maxAgeMs) {
	if (!s || !e || entIdx <= 0 || entIdx > 32) return;
	if (maxAgeMs < 16u) maxAgeMs = 16u;
	if (maxAgeMs > 1000u) maxAgeMs = 1000u;
	TeamHudGearCache& c = g_teamHudGearCache[entIdx];
	if (TeamHudLocalIsSpectating()) {
		if (c.pawn == e) TeamHudCopyGear(s, c.gear);
		return;
	}
	const DWORD now = GetTickCount();
	const DWORD remain = c.nextReadAt ? (c.nextReadAt - now) : 0u;
	if (c.pawn != e || !c.nextReadAt || (LONG)(now - c.nextReadAt) >= 0
		|| (LONG)remain > (LONG)maxAgeMs) {
		TeamHudSlot fresh{};
		TeamHudReadWeps(e, &fresh);
		const bool retryStem = (fresh.side == TW_Melee || fresh.wep == TW_Melee) && !fresh.melee[0];
		c.pawn = e;
		c.nextReadAt = now + (retryStem ? 50u : maxAgeMs);
		c.gear = fresh;
	}
	TeamHudCopyGear(s, c.gear);
}

void TeamHudResetRuntimeState() {
	g_teamHudGatherPaintAt = 0;
	TeamHudPlayerInfoCacheClear();
	memset(g_teamHudGearCache, 0, sizeof(g_teamHudGearCache));
	memset(g_teamHudIdleLinks, 0, sizeof(g_teamHudIdleLinks));
	memset(g_teamHudSlots, 0, sizeof(g_teamHudSlots));
	g_teamHudLocal = {};
}

int TeamHudPropFromName(const char* n) {
	if (!n || !n[0]) return -1;
	char buf[48]{};
	strncpy(buf, n, 47);
	for (char* p = buf; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
	}
	for (char* p = buf; *p; ++p) {
		if (*p == '(' || *p == '[') {
			*p = 0;
			break;
		}
	}
	struct Tok { const char* s; int prop; };
	static const Tok kMap[] = {
		{ "nick", 0 }, { "gambler", 0 },
		{ "rochelle", 1 }, { "producer", 1 },
		{ "coach", 2 },
		{ "ellis", 3 }, { "mechanic", 3 },
		{ "bill", 4 }, { "namvet", 4 },
		{ "zoey", 5 }, { "teengirl", 5 },
		{ "francis", 6 }, { "biker", 6 },
		{ "louis", 7 }, { "manager", 7 },
	};
	for (const Tok& t : kMap) {
		if (strstr(buf, t.s)) return t.prop;
	}
	if (strstr(n, "佐伊")) return 5;
	if (strstr(n, "路易斯")) return 7;
	if (strstr(n, "弗朗西斯")) return 6;
	if (strstr(n, "尼克")) return 0;
	if (strstr(n, "教练")) return 2;
	if (strstr(n, "埃利斯")) return 3;
	if (strstr(n, "比尔")) return 4;
	return -1;
}

int TeamHudPropFromModel(void* e) {
	if (!e || g_offModelIndex < 0 || g_offModelIndex > 0x4000) return -1;
	int* pm = (int*)((uint8_t*)e + g_offModelIndex);
	if (!PtrCommitted(pm)) return -1;
	const char* model = TeamHudModelName(*pm);
	if (!model || !model[0]) return -1;
	char buf[80]{};
	strncpy(buf, model, 79);
	for (char* p = buf; *p; ++p) {
		if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
	}
	struct Tok { const char* s; int prop; };
	static const Tok kMap[] = {
		{ "survivor_gambler", 0 },
		{ "survivor_producer", 1 },
		{ "survivor_coach", 2 },
		{ "survivor_mechanic", 3 },
		{ "survivor_namvet", 4 },
		{ "survivor_teenangst", 5 },
		{ "survivor_biker", 6 },
		{ "survivor_manager", 7 },
	};
	for (const Tok& t : kMap) {
		if (strstr(buf, t.s)) return t.prop;
	}
	return -1;
}

int TeamHudResolveProp(int entIdx, void* e, int rawProp) {
	// Model first: both survivor generations may reuse raw values 0..3.
	const int fromModel = TeamHudPropFromModel(e);
	if (fromModel >= 0) return fromModel;
	if (rawProp >= 4 && rawProp <= 7) return rawProp;
	PlayerInfo pi{};
	const bool hasPi = EngPlayerInfo(entIdx, &pi) && pi.name[0];
	if (hasPi) {
		const int fromName = TeamHudPropFromName(pi.name);
		if (fromName >= 0) return fromName;
	}
	// 0..3 is generation-ambiguous; briefly keep the original name rather than
	// applying an L4D2 name to an L4D1 survivor while its model is unavailable.
	return -1;
}

int TeamHudResolveAvatarProp(void* e, int rawProp, const PlayerInfo* playerInfo) {
	// Return canonical portrait indices (Nick..Ellis=0..3, Bill/Zoey/Francis/Louis=4..7).
	// Explicit 4..7 character IDs take precedence over replacement models.
	if (rawProp >= 4 && rawProp <= 7) return rawProp;
	// A model-resolved index must NEVER be remapped using the campaign's cast.
	const int fromModel = TeamHudPropFromModel(e);
	if (fromModel >= 0) return fromModel;

	// Only exact default BOT names are identity evidence. Human nicknames and
	// customized BOT names must not select a portrait by substring matching.
	if (playerInfo && PlayerInfoIsBot(*playerInfo)
		&& TeamHudNameSetHint(playerInfo->name) != 0)
		return TeamHudPropFromName(playerInfo->name);

	// Only the unresolved raw 0..3 fallback is generation-dependent. Known
	// L4D1 BOT portraits in the same team must not disable this local fallback.
	return TeamHudFaceFromProp(rawProp, TeamHudQuerySurvivorSet() == 1);
}

bool TeamHudEntGhostObs(void* e) {
	if (!EntReadable(e)) return true;
	if (g_offObserverMode < 0 || g_offObserverMode > 0x4000) return false;
	int* po = (int*)((uint8_t*)e + g_offObserverMode);
	if (!PtrCommitted(po) || *po == 0) return false;
	if (g_offIsIncapacitated >= 0 && g_offIsIncapacitated <= 0x4000) {
		unsigned char* pi = (unsigned char*)e + g_offIsIncapacitated;
		if (PtrCommitted(pi) && *pi) return false;
	}
	if (g_offLifeState >= 0 && g_offLifeState <= 0x4000) {
		unsigned char* pl = (unsigned char*)e + g_offLifeState;
		if (PtrCommitted(pl) && *pl == 0) return false;
	}
	if (g_offHealth >= 0 && g_offHealth <= 0x4000) {
		int* ph = (int*)((uint8_t*)e + g_offHealth);
		if (PtrCommitted(ph) && *ph > 0) return false;
	}
	return true;
}

int TeamHudEntPriority(void* e, int entIdx) {
	if (!EntReadable(e)) return -100000;
	int score = 0;
	if (g_offLifeState >= 0 && g_offLifeState <= 0x4000) {
		unsigned char* pl = (unsigned char*)e + g_offLifeState;
		if (PtrCommitted(pl) && *pl == 0) score += 200;
	}
	if (g_offHealth >= 0 && g_offHealth <= 0x4000) {
		int* ph = (int*)((uint8_t*)e + g_offHealth);
		if (PtrCommitted(ph) && *ph > 0) score += 100;
	}
	if (g_offIsIncapacitated >= 0 && g_offIsIncapacitated <= 0x4000) {
		unsigned char* pi = (unsigned char*)e + g_offIsIncapacitated;
		if (PtrCommitted(pi) && *pi) score += 80;
	}
	if (g_offObserverMode >= 0 && g_offObserverMode <= 0x4000) {
		int* po = (int*)((uint8_t*)e + g_offObserverMode);
		if (PtrCommitted(po) && *po != 0) score -= 120;
	}
	score -= entIdx;
	return score;
}

int TeamHudDedupRecs(int nRec, int* idx, int* prop, void** ents, int* userids) {
	int out = 0;
	for (int i = 0; i < nRec; ++i) {
		int found = -1;
		for (int j = 0; j < out; ++j) {
			if (userids[i] > 0 && userids[i] == userids[j]) {
				found = j;
				break;
			}
		}
		if (found >= 0) {
			if (TeamHudEntPriority(ents[i], idx[i]) > TeamHudEntPriority(ents[found], idx[found])) {
				idx[found] = idx[i];
				prop[found] = prop[i];
				ents[found] = ents[i];
				userids[found] = userids[i];
			}
			continue;
		}
		if (out != i) {
			idx[out] = idx[i];
			prop[out] = prop[i];
			ents[out] = ents[i];
			userids[out] = userids[i];
		}
		++out;
	}
	return out;
}

bool TeamHudEntQualifies(int entIdx, void* e, int* outProp) {
	if (!e || !EntReadable(e) || entIdx <= 0 || entIdx > 32) return false;
	if (TeamHudEntGhostObs(e)) return false;
	PlayerInfo pi{};
	const bool havePi = EngPlayerInfo(entIdx, &pi);
	if (havePi) {
		g_teamHudPlayerInfoCache[entIdx] = pi;
		g_teamHudPlayerInfoValid[entIdx] = true;
	}
	const bool hasPi = havePi && pi.name[0];
	const char* cn = EntNetClassName(entIdx);
	const bool pawn = ClientUxIsPlayerPawnClass(cn);
	if (!hasPi && !pawn) return false;
	if (g_offTeam >= 0 && g_offTeam <= 0x4000) {
		int* pt = (int*)((uint8_t*)e + g_offTeam);
		if (!PtrCommitted(pt) || *pt != 2) return false;
	} else return false;
	if (g_offZombieClass >= 0 && g_offZombieClass <= 0x4000) {
		int* pz = (int*)((uint8_t*)e + g_offZombieClass);
		if (PtrCommitted(pz) && *pz >= 1 && *pz <= 8) return false;
	}
	int rawProp = -1;
	if (g_offSurvivorCharacter >= 0 && g_offSurvivorCharacter <= 0x4000) {
		int* pc = (int*)((uint8_t*)e + g_offSurvivorCharacter);
		if (PtrCommitted(pc)) rawProp = *pc;
	}
	const int prop = TeamHudResolveAvatarProp(e, rawProp, havePi ? &pi : nullptr);
	if (outProp) *outProp = prop;
	return true;
}

void TeamHudFillSlot(TeamHudSlot& s, void* e, int entIdx, int faceCh, bool isLocal, bool refreshName) {
	memset(&s, 0, sizeof(s));
	if (!e || !EntReadable(e) || entIdx <= 0 || entIdx >= 2048) return;
	s.present = true;
	s.ent = entIdx;
	s.sampledPawn = e;
	s.isLocal = isLocal;
	s.character = (faceCh >= 0 && faceCh < kTeamHudCharMax) ? faceCh : 0;
	s.hp = 0;
	s.maxHp = 100;
	s.tempHp = 0;
	s.dead = false;
	s.incap = false;
	s.hanging = false;
	s.third = false;
	s.cap = kTeamCapNone;
	s.revive = -1.f;
	s.actionProgress = -1.f;
	s.actionRemain = 0.f;
	s.userid = 0;
	s.capAtk = 0;
	s.capAtkUserId = 0;
	s.capAtkName[0] = 0;
	s.reviveBy = 0;
	s.reviveByUserId = 0;
	s.reviveByName[0] = 0;
	s.intensity = 0;
	// Health is required: without it (netvar not resolved yet, or the read failed) this is not a
	// valid sample. The default 0 must never be interpreted below as "dead".
	if (g_offHealth < 0 || g_offHealth > 0x4000 || !PtrCommitted((uint8_t*)e + g_offHealth)) {
		memset(&s, 0, sizeof(s));
		return;
	}
	s.hp = *(int*)((uint8_t*)e + g_offHealth);
	if (g_offMaxHealth >= 0 && g_offMaxHealth <= 0x4000) {
		int* pm = (int*)((uint8_t*)e + g_offMaxHealth);
		if (PtrCommitted(pm) && *pm > 0 && *pm <= 1000) s.maxHp = *pm;
	}
	if (g_offHealthBuffer >= 0 && g_offHealthBuffer <= 0x4000) {
		float* pb = (float*)((uint8_t*)e + g_offHealthBuffer);
		if (PtrCommitted(pb) && *pb > 0.f && *pb < 200.f) {
			float buf = *pb;
			float t0 = 0.f;
			if (g_offHealthBufferTime >= 0 && g_offHealthBufferTime <= 0x4000) {
				float* pt = (float*)((uint8_t*)e + g_offHealthBufferTime);
				if (PtrCommitted(pt)) t0 = *pt;
			}
			float rate = 0.27f;
			{
				static void* s_decayCv = nullptr;
				static DWORD s_decayAt = 0;
				const DWORD nowCv = GetTickCount();
				if (!s_decayCv || !s_decayAt || (nowCv - s_decayAt) > 5000u) {
					s_decayCv = CvarFind("pain_pills_decay_rate");
					s_decayAt = nowCv;
				}
				float f = rate;
				int n = 0;
				if (s_decayCv && CvarReadPair(s_decayCv, &n, &f) && f == f && f > 0.f && f < 10.f)
					rate = f;
			}
			const float now = EngClientCurTime();
			float live = buf;
			if (t0 > 0.f && now > t0)
				live = buf - (now - t0) * rate;
			if (live < 0.f) live = 0.f;
			s.tempHp = (int)(live + 0.5f);
		}
	}
	if (g_offIsIncapacitated >= 0 && g_offIsIncapacitated <= 0x4000) {
		unsigned char* pi = (unsigned char*)e + g_offIsIncapacitated;
		if (PtrCommitted(pi)) s.incap = (*pi != 0);
	}
	if (g_offHanging >= 0 && g_offHanging <= 0x4000) {
		unsigned char* phg = (unsigned char*)e + g_offHanging;
		if (PtrCommitted(phg)) s.hanging = (*phg != 0);
	}
	if (g_offThirdStrike >= 0 && g_offThirdStrike <= 0x4000) {
		unsigned char* pt = (unsigned char*)e + g_offThirdStrike;
		if (PtrCommitted(pt)) s.third = (*pt != 0);
	}
	{
		auto atkLive = [](int idx) -> bool {
			return idx > 0 && idx < 2048 && EntNetworkable(idx) && EntReadable(EntGet(idx));
		};
		int atk = TeamHudReadHandleIdx(e, g_offPummelAttacker);
		if (atkLive(atk))
			s.cap = kTeamCapChargerPummel;
		else {
			atk = TeamHudReadHandleIdx(e, g_offCarryAttacker);
			if (atkLive(atk))
				s.cap = kTeamCapCharger;
			else {
				atk = TeamHudReadHandleIdx(e, g_offPounceAttacker);
				if (atkLive(atk))
					s.cap = kTeamCapHunter;
				else {
					atk = TeamHudReadHandleIdx(e, g_offJockeyAttacker);
					if (atkLive(atk))
						s.cap = kTeamCapJockey;
					else {
						atk = TeamHudReadHandleIdx(e, g_offTongueOwner);
						if (atkLive(atk) || TeamHudReadBool(e, g_offHangingTongue))
							s.cap = kTeamCapSmoker;
						else
							atk = 0;
					}
				}
			}
		}
		s.capAtk = atkLive(atk) ? atk : 0;
	}
	{
		const float action = TeamHudReadProgressFrac(e);
		if (action >= 0.f) {
			float* ps = (float*)((uint8_t*)e + g_offProgStart);
			float* pd = (float*)((uint8_t*)e + g_offProgDur);
			if (PtrCommitted(ps) && PtrCommitted(pd) && *pd >= 0.2f && *pd <= 30.f
				&& EngLastTimeStamp() <= *ps + *pd + 0.12f) {
				s.actionProgress = action;
				s.actionRemain = *pd * (1.f - action);
			}
		}
	}
	{
		const int ownerIdx = TeamHudReadHandleIdx(e, g_offReviveOwner);
		if (ownerIdx > 0 && ownerIdx < 2048 && EntNetworkable(ownerIdx)) {
			s.reviveBy = ownerIdx;
			float rv = TeamHudReadProgressFrac(e);
			if (rv < 0.f) {
				void* ow = EntGet(ownerIdx);
				if (EntReadable(ow))
					rv = TeamHudReadProgressFrac(ow);
			}
			s.revive = (rv >= 0.f) ? rv : 2.f;
		}
	}
	if (g_offClientIntensity >= 0 && g_offClientIntensity <= 0x4000) {
		int* pi = (int*)((uint8_t*)e + g_offClientIntensity);
		if (PtrCommitted(pi)) {
			int v = *pi;
			if (v < 0) v = 0;
			if (v > 100) v = 100;
			s.intensity = v;
		}
	}
	if (g_offLifeState >= 0 && g_offLifeState <= 0x4000) {
		unsigned char* pl = (unsigned char*)e + g_offLifeState;
		if (PtrCommitted(pl)) s.dead = (*pl != 0) && !s.incap;
	}
	if (s.hp <= 0 && !s.incap) s.dead = true;
	if (s.dead) {
		// m_healthBuffer can retain its last networked value after lifeState flips.
		// A dead card must never carry that stale temporary health into bar paint.
		s.hp = 0;
		s.tempHp = 0;
		s.cap = kTeamCapNone;
		s.capAtk = 0;
		s.capAtkUserId = 0;
		s.capAtkName[0] = 0;
		s.revive = -1.f;
		s.actionProgress = -1.f;
		s.actionRemain = 0.f;
		s.reviveBy = 0;
		s.reviveByUserId = 0;
		s.reviveByName[0] = 0;
		s.hanging = false;
	}
	// During death/observer transitions weapon handles and model indices are the
	// least stable part of the pawn. Keep names/HP/status, but omit gear reads.
	// Weapon/model/ammo traversal is exclusive to the full left-side TeamHud.
	// Captured/revive/world HUDs share survivor snapshots but never display gear.
	if (!s.dead && !TeamHudLocalIsSpectating()
		&& HudWantsDraw(g_optTeamHud, 15)
		&& TeamHudFullReady(HudMenuPage(15)))
		TeamHudReadWepsCached(e, entIdx, &s);
	const PlayerInfo* cachedPi = TeamHudPlayerInfoCached(entIdx);
	TeamHudApplyName(s, entIdx);
	if (s.userid <= 0 && !cachedPi) {
		PlayerInfo pi{};
		if (EngPlayerInfo(entIdx, &pi) && pi.userid > 0)
			s.userid = pi.userid;
	}
	if (s.cap && s.capAtk > 0)
		TeamHudFillTag(s.capAtk, &s.capAtkUserId, s.capAtkName, 24);
	if (s.revive >= 0.f && s.reviveBy > 0)
		TeamHudFillTag(s.reviveBy, &s.reviveByUserId, s.reviveByName, 24);
}

void TeamHudGather() {
	if (IdleSwapHudQuietActive())
		return;
	if (g_hudPostMissionQuiet)
		return;
	if (!g_engine || !EngInGame() || EngDrawingLoading())
		return;
	const DWORD nowTick = GetTickCount();
	const bool entityHud = NativeWorldNamesWanted() || TeamWorldNamesWanted() || HudWantsDraw(g_optTeamHud, 15)
		|| (!g_worldHudRealism && HudWantsDraw(g_optTeamWorldHud, kMenuPageWorldHud))
		|| (!g_worldHudRealism && HudWantsDraw(g_optWorldBlackWhite, kMenuPageWorldHud))
		|| g_optMateLastLife || g_optMateLowHp
		|| HudWantsDraw(g_optWorldActionProgress, kMenuPageWorldHud)
		|| HudWantsDraw(g_optCapHud, 23) || HudWantsDraw(g_optReviveHud, 24)
		|| HudWantsDraw(g_optCustomDirectorHud, 17);
	const bool fastHud = entityHud;
	const bool preciseProgress = HudWantsDraw(g_optWorldActionProgress, kMenuPageWorldHud)
		|| HudWantsDraw(g_optReviveHud, 24);
	// HP/status does not need 20 Hz entity/weapon traversal. 10 Hz remains visually
	// responsive while halving the fixed player-slot and inventory read cost. Keep
	// interaction/revive progress at the old 20 Hz so its timing remains precise.
	const DWORD gatherGap = TeamHudLocalIsSpectating() ? 200u
		: (preciseProgress ? 50u : (fastHud ? 100u : 250u));
	if (g_teamHudGatherPaintAt && (nowTick - g_teamHudGatherPaintAt) < gatherGap)
		return;
	g_teamHudGatherPaintAt = nowTick;
	TeamHudPlayerInfoCacheClear();
	for (int i = 0; i < kTeamHudMax; ++i)
		g_teamHudSlots[i].present = false;
	g_teamHudLocal = {};
	if (!g_entlist) return;
	const int local = EngLocal();

	int recIdx[16]{};
	int recProp[16]{};
	void* recEnt[16]{};
	int recUid[16]{};
	int nRec = 0;
	for (int i = 1; i <= 32 && nRec < 16; ++i) {
		void* e = EntGet(i);
		int prop = -1;
		if (!TeamHudEntQualifies(i, e, &prop)) continue;
		recIdx[nRec] = i;
		recProp[nRec] = prop;
		recEnt[nRec] = e;
		recUid[nRec] = 0;
		const PlayerInfo* pi = TeamHudPlayerInfoCached(i);
		if (pi && pi->userid > 0)
			recUid[nRec] = pi->userid;
		++nRec;
	}
	for (int i = 1; i < nRec; ++i) {
		const int ti = recIdx[i];
		const int tp = recProp[i];
		void* te = recEnt[i];
		const int tu = recUid[i];
		int j = i;
		while (j > 0 && recIdx[j - 1] > ti) {
			recIdx[j] = recIdx[j - 1];
			recProp[j] = recProp[j - 1];
			recEnt[j] = recEnt[j - 1];
			recUid[j] = recUid[j - 1];
			--j;
		}
		recIdx[j] = ti;
		recProp[j] = tp;
		recEnt[j] = te;
		recUid[j] = tu;
	}
	nRec = TeamHudDedupRecs(nRec, recIdx, recProp, recEnt, recUid);

	const int set = TeamHudQuerySurvivorSet();
	// Retain the custom-map fallback using this gather's cached BOT information.
	// It applies only to unresolved raw indices, never to confirmed portraits.
	if (set == 0 && !g_teamHudNameVoted) {
		int n1 = 0, n2 = 0;
		for (int i = 0; i < nRec; ++i) {
			const PlayerInfo* pi = TeamHudPlayerInfoCached(recIdx[i]);
			if (!pi || !PlayerInfoIsBot(*pi)) continue;
			const int h = TeamHudNameSetHint(pi->name);
			if (h == 1) ++n1;
			else if (h == 2) ++n2;
		}
		if (n1 >= 2 && n2 == 0) {
			g_teamHudSetCached = 1;
			g_teamHudNameVoted = true;
		} else if (n2 >= 2 && n1 == 0) {
			g_teamHudSetCached = 2;
			g_teamHudNameVoted = true;
		}
	}
	if (!g_teamHudSetLogged) {
		g_teamHudSetLogged = true;
		Log("teamhud set=%d portraits=canonical n=%d map=%s", g_teamHudSetCached, nRec,
			g_teamHudSetMap[0] ? g_teamHudSetMap : "?");
	}

	int teammateSlot = 0;
	for (int r = 0; r < nRec; ++r) {
		const int i = recIdx[r];
		void* e = recEnt[r];
		int faceCh = recProp[r];
		if (set == 0 && g_teamHudSetCached == 1) {
			// Re-evaluate on the gather that learned an unknown map's cast, so
			// the local portrait does not spend a refresh interval on the wrong set.
			int rawProp = -1;
			if (g_offSurvivorCharacter >= 0 && g_offSurvivorCharacter <= 0x4000) {
				const int* pc = (const int*)((uint8_t*)e + g_offSurvivorCharacter);
				if (PtrCommitted(pc)) rawProp = *pc;
			}
			faceCh = TeamHudResolveAvatarProp(e, rawProp, TeamHudPlayerInfoCached(i));
		}
		if (faceCh < 0) faceCh = 0;
		const bool isLocal = (i == local);
		if (isLocal) {
			TeamHudFillSlot(g_teamHudLocal, e, i, faceCh, true, false);
			continue;
		}
		if (teammateSlot >= kTeamHudMax)
			continue;
		TeamHudFillSlot(g_teamHudSlots[teammateSlot], e, i, faceCh, false, false);
		++teammateSlot;
	}
}

void TeamHudFillChamfer(int x, int y, int w, int h, int cut) {
	if (w < 12 || h < 12) {
		SurfFill(x, y, x + w, y + h);
		return;
	}
	if (cut < 6) cut = 6;
	if (cut > 18) cut = 18;
	if (cut > w / 4) cut = w / 4;
	if (cut > h / 3) cut = h / 3;
	if (cut < 2) {
		SurfFill(x, y, x + w, y + h);
		return;
	}
	SurfFill(x, y + cut, x + w, y + h - cut);
	for (int i = 0; i < cut; ++i) {
		const int topR = x + w - cut + i;
		if (topR > x)
			SurfFill(x, y + i, topR, y + i + 1);
		const int botL = x + cut - i;
		if (botL < x + w)
			SurfFill(botL, y + h - cut + i, x + w, y + h - cut + i + 1);
	}
}

void TeamHudStrokeChamfer(int x, int y, int w, int h, int cut) {
	if (cut < 2) {
		SurfOutline(x, y, x + w, y + h);
		return;
	}
	SurfLine(x, y, x + w - cut, y);
	SurfLine(x + w - cut, y, x + w - 1, y + cut);
	SurfLine(x + w - 1, y + cut, x + w - 1, y + h - 1);
	SurfLine(x + w - 1, y + h - 1, x + cut, y + h - 1);
	SurfLine(x + cut, y + h - 1, x, y + h - 1 - cut);
	SurfLine(x, y + h - 1 - cut, x, y);
}

COLORREF TeamHudHpCol(const TeamHudSlot& s) {
	if (s.dead) return RGB(90, 90, 96);
	if (s.incap) return RGB(255, 150, 70);
	if (s.hp <= 24) return RGB(255, 72, 72);
	if (s.hp <= 39) return RGB(255, 186, 64);
	return RGB(80, 220, 120);
}

void TeamWorldHudPaint() {
	const bool preview = HudMenuPage(kMenuPageWorldHud);
	if (IdleSwapDeferEntityHud(preview)) return;
	const bool names = TeamWorldNamesWanted();
	const bool health = HudWantsDraw(g_optTeamWorldHud, kMenuPageWorldHud);
	if ((!names && !health) || g_worldHudRealism) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading() || g_hudPostMissionQuiet) return;
	if (!MenuEnsureSurf()) return;
	if (g_offOrigin < 0 || g_offOrigin > 0x4000 || g_offTeam < 0 || g_offTeam > 0x4000) return;
	if (health && !g_teamWorldFont)
		g_teamWorldFont = SurfAcquireVguiFont(14, 700, kFontFlagAntiAlias | kFontFlagDropShadow);
	if (health && !g_teamWorldFont) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh) || sw < 80 || sh < 80) return;
	const int local = EngLocal();
	struct NameLayout { wchar_t source[24]; wchar_t text[24]; unsigned long font; int limit, width; };
	static NameLayout nameCache[kTeamHudMax]{};
	struct NameRect { int left, top, right, bottom; };
	NameRect placed[kTeamHudMax]{};
	int placedCount = 0;
	unsigned long nameFont=0;
	if (names) {
		nameFont = SurfAcquireWorldNamesFont(g_teamWorldNamesSize);
		g_teamWorldNamesFont = nameFont;
		g_teamWorldNamesFontPx = nameFont ? g_teamWorldNamesSize : 0;
	}
	const int nameHeight=nameFont?SurfCachedFontTall(nameFont)+2:16;
	const int nameLimit = sw / 4 < 180 ? sw / 4 : 180;
	// An old sample must not reappear after disabling/re-enabling or a stalled gather.
	const bool freshNames = names && nameFont && g_teamHudGatherPaintAt
		&& GetTickCount() - g_teamHudGatherPaintAt <= 300u;

	// Reuse TeamHudGather's bounded survivor cache (up to kTeamHudMax remote slots)
	// and never searches the entity table.
	for (int i = 0; i < kTeamHudMax; ++i) {
		const TeamHudSlot& s = g_teamHudSlots[i];
		if (!s.present || s.isLocal || s.dead || s.ent <= 0 || s.ent > 64 || s.ent == local) continue;
		if (!EntNetworkable(s.ent)) continue;
		void* e = EntGet(s.ent);
		if (!e) continue;
		size_t need = (size_t)g_offOrigin + sizeof(float) * 3;
		const size_t teamNeed = (size_t)g_offTeam + sizeof(int);
		if (teamNeed > need) need = teamNeed;
		if (g_offViewOffset >= 0 && g_offViewOffset <= 0x4000) {
			const size_t viewNeed = (size_t)g_offViewOffset + sizeof(float) * 3;
			if (viewNeed > need) need = viewNeed;
		}
		if (!skmem::SpanProbe(e, need)) continue;
		int teamValue = 0;
		if (!EntRead(e, g_offTeam, &teamValue) || teamValue != 2) continue;

		const float* o = (const float*)((const uint8_t*)e + g_offOrigin);
		float world[3] = { o[0], o[1], o[2] };
		if (!(world[0] == world[0]) || !(world[1] == world[1]) || !(world[2] == world[2])) continue;
		float lift = s.incap ? 42.f : 70.f;
		if (g_offViewOffset >= 0 && g_offViewOffset <= 0x4000) {
			const float* vo = (const float*)((const uint8_t*)e + g_offViewOffset);
			if (vo[2] > 16.f && vo[2] < 120.f)
				lift = vo[2] + 9.f;
		}
		world[2] += lift;

		float sx = 0.f, sy = 0.f;
		if (!LocalPlayWorldToScreen(world, &sx, &sy, sw, sh)) continue;

		// Screen-space labels deliberately do not Trace or apply view-cone/distance fades.
		if (freshNames && e == s.sampledPawn && s.name[0]
			&& sx >= 0.f && sy >= 0.f && sx < sw && sy < sh) {
			NameLayout& c = nameCache[i];
			if (c.font != nameFont || c.limit != nameLimit || wcscmp(c.source, s.name)) {
				wcscpy_s(c.source, s.name);
				wcscpy_s(c.text, s.name);
				for (int n = 0; c.text[n]; ++n)
					if (c.text[n] < 32) c.text[n] = L' ';
				c.font = nameFont;
				c.limit = nameLimit;
				c.width = SurfTextWFont(c.font, c.text);
				int n = (int)wcslen(c.text);
				while (c.width > nameLimit && n > 1) {
					--n;
					// Do not leave half of a UTF-16 surrogate pair before the ellipsis.
					if (n > 0 && c.text[n - 1] >= 0xD800 && c.text[n - 1] <= 0xDBFF) --n;
					c.text[n] = L'…'; c.text[n + 1] = 0;
					c.width = SurfTextWFont(c.font, c.text);
				}
			}
			int x = (int)sx - c.width / 2;
			if (x < 2) x = 2;
			if (x + c.width > sw - 2) x = sw - 2 - c.width;
			int y = (int)sy - (health ? 38 + nameHeight : 12 + nameHeight);
			if (y < 2) y = 2;
			const int anchorY = y;
			// At most eight deterministic upward moves; no unbounded layout search.
			for (int attempt = 0; attempt < kTeamHudMax; ++attempt) {
				bool overlap = false;
				for (int r = 0; r < placedCount; ++r) {
					const NameRect& p = placed[r];
					if (x < p.right + 3 && x + c.width + 3 > p.left
						&& y < p.bottom + 2 && y + nameHeight + 2 > p.top) {
						y = p.top - nameHeight - 3;
						overlap = true; break;
					}
				}
				if (!overlap) break;
			}
			// At the top edge, retain the label rather than discard a teammate.
			if (y < 2) y = anchorY;
			if (y + nameHeight < sh) {
				placed[placedCount++] = {x, y, x + c.width, y + nameHeight};
				SurfTextAt(c.font, x - 1, y, RGB(12, 18, 24), c.text);
				SurfTextAt(c.font, x + 1, y, RGB(12, 18, 24), c.text);
				SurfTextAt(c.font, x, y, (COLORREF)g_teamWorldNamesColor, c.text);
			}
		}
		if (!health) continue;

		const int capHp = s.incap ? 300 : (s.maxHp > 0 ? s.maxHp : 100);
		int solidHp = s.hp;
		if (solidHp < 0) solidHp = 0;
		if (solidHp > capHp) solidHp = capHp;
		int shownHp = solidHp + (s.incap ? 0 : s.tempHp);
		if (shownHp < 0) shownHp = 0;
		if (shownHp > capHp) shownHp = capHp;

		const int barW = 42;
		const int barH = 5;
		const int x0 = (int)(sx + 0.5f) - 39;
		const int y0 = (int)(sy + 0.5f) - 34;
		if (x0 < -50 || y0 < -30 || x0 > sw + 20 || y0 > sh + 20) continue;
		const int markX = x0 + 3;
		const int barX = x0 + 11;
		const int barY = y0 + 6;
		const COLORREF hc = (s.hanging && !s.incap) ? RGB(255, 176, 66) : TeamHudHpCol(s);
		const int solidW = capHp > 0 ? (barW * solidHp + capHp / 2) / capHp : 0;
		const int totalW = capHp > 0 ? (barW * shownHp + capHp / 2) / capHp : 0;

		// Small asymmetric survivor marker: no enclosing panel, so clustered teammates
		// occupy only a thin strip above their heads.
		SurfColor(0, 0, 0, 145);
		SurfFill(markX - 3, y0 + 4, markX + 4, y0 + 9);
		SurfFill(markX - 1, y0 + 2, markX + 2, y0 + 11);
		SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 240);
		SurfFill(markX - 2, y0 + 5, markX + 3, y0 + 8);
		SurfFill(markX, y0 + 3, markX + 2, y0 + 10);

		SurfColor(0, 0, 0, 155);
		SurfFill(barX - 1, barY - 1, barX + barW + 1, barY + barH + 1);
		SurfColor(24, 31, 28, 205);
		SurfFill(barX, barY, barX + barW, barY + barH);
		if (totalW > solidW) {
			SurfColor(238, 190, 74, 225);
			SurfFill(barX + solidW, barY, barX + totalW, barY + barH);
		}
		if (solidW > 0) {
			SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 238);
			SurfFill(barX, barY, barX + solidW, barY + barH);
		}
		SurfColor(255, 255, 255, 55);
		SurfFill(barX, barY, barX + barW - 3, barY + 1);
		SurfColor(0, 0, 0, 205);
		SurfFill(barX + barW - 3, barY + barH - 1, barX + barW, barY + barH);

		wchar_t hpText[12]{};
		swprintf_s(hpText, 12, L"%d", shownHp);
		const COLORREF tc = s.incap ? RGB(255, 198, 118) : RGB(228, 244, 232);
		SurfTextAt(g_teamWorldFont, barX + barW + 5, y0 - 1, tc, hpText);
	}
}

void WorldNoticeClear() {
	g_worldFfNotice = {};
	g_worldWitchNotice = {};
	g_worldTankNotice = {};
	g_worldRescueNotice = {};
	g_worldMeleeSkeetNotice = {};
	g_worldLevelNotice = {};
	g_worldSkeetNotice = {};
	g_worldTongueCutNotice = {};
	g_worldCrownNotice = {};
	g_worldTankSlayerNotice = {};
	g_worldCrownKind = 0;
	memset(g_worldBileNotices, 0, sizeof(g_worldBileNotices));
	g_worldBileAnyActive = false;
	memset(g_worldWitchStartleNotice, 0, sizeof(g_worldWitchStartleNotice));
	g_worldWitchStartleAny = false;
	WorldNoticeClearPinTrack();
	memset(g_localAirHitAt, 0, sizeof(g_localAirHitAt));
	memset(g_chargerChargeUntil, 0, sizeof(g_chargerChargeUntil));
}

bool WorldNoticeSurvivorEnt(int ent) {
	if (ent <= 0 || ent > 64 || ent == EngLocal()) return false;
	void* e = EntGet(ent);
	if (!EntReadable(e) || g_offTeam < 0 || g_offTeam > 0x4000) return false;
	const int* team = (const int*)((const uint8_t*)e + g_offTeam);
	return PtrCommitted(team) && *team == 2;
}

int WorldNoticeEventPlayer(void* ev, const char* key, const char* fallbackKey) {
	if (!ev || !key) return 0;
	int uid = EvInt(ev, key);
	int ent = uid > 0 ? EngPlayerForUserID(uid) : 0;
	if (ent <= 0 && uid > 0 && uid <= 64) ent = uid;
	if (ent <= 0 && fallbackKey) {
		uid = EvInt(ev, fallbackKey);
		ent = uid > 0 ? EngPlayerForUserID(uid) : 0;
		if (ent <= 0 && uid > 0 && uid <= 64) ent = uid;
	}
	return ent;
}

bool WorldNoticeEventIsLocal(void* ev, const char* key, const char* fallbackKey) {
	const int local = EngLocal();
	return local > 0 && WorldNoticeEventPlayer(ev, key, fallbackKey) == local;
}

// Screen "关键保护" shares the badge. The world marker still draws only when
// g_optWorldRescue is on.
static bool RescueTrackWanted() {
	return g_optWorldRescue || g_optMateSave;
}

void WorldNoticeClearPinTrack() {
	g_worldControlledBy = 0;
	g_worldPinKind = 0;
	g_worldRescueCandidate = 0;
	g_worldRescueCandidateAt = 0;
	g_worldControlPollAt = 0;
	g_worldPinArmedAt = 0;
	g_worldPinReleaseAt = 0;
	g_worldRecentController = 0;
	g_worldRecentPinKind = 0;
	g_worldControlledRecentUntil = 0;
	g_worldPendingShoveHelper = 0;
	g_worldPendingShoveTarget = 0;
	g_worldPendingShoveAt = 0;
	g_worldPendingKillVictim = 0;
	g_worldPendingKillHelper = 0;
	g_worldPinProbeUntil = 0;
}

static bool WorldNoticeReadLocalPin(int* entOut, int* kindOut) {
	const int local = EngLocal();
	void* me = local > 0 ? EntGet(local) : nullptr;
	if (!EntReadable(me)) return false;
	const int offsets[] = { g_offTongueOwner, g_offPounceAttacker, g_offJockeyAttacker,
		g_offCarryAttacker, g_offPummelAttacker };
	const int kinds[] = { kWorldPinTongue, kWorldPinPounce, kWorldPinJockey,
		kWorldPinCharger, kWorldPinCharger };
	for (int i = 0; i < 5; ++i) {
		const int ent = TeamHudReadHandleIdx(me, offsets[i]);
		if (ent > 0 && ent <= 64) {
			if (entOut) *entOut = ent;
			if (kindOut) *kindOut = kinds[i];
			return true;
		}
	}
	return false;
}

static void WorldNoticeRememberPin(int controller, int kind) {
	if (controller <= 0 || controller > 64) return;
	if (controller != g_worldControlledBy) {
		g_worldRescueCandidate = 0;
		g_worldRescueCandidateAt = 0;
		g_worldPinArmedAt = GetTickCount();
	}
	g_worldControlledBy = controller;
	g_worldPinKind = kind;
	g_worldRecentController = controller;
	g_worldRecentPinKind = kind;
	g_worldControlledRecentUntil = GetTickCount() + kWorldPinKillGraceMs;
	g_worldPinReleaseAt = 0;
}

static int WorldNoticeKillController() {
	const DWORD now = GetTickCount();
	if (g_worldControlledBy > 0) return g_worldControlledBy;
	if (g_worldRecentController > 0 && g_worldControlledRecentUntil
		&& now <= g_worldControlledRecentUntil)
		return g_worldRecentController;
	return 0;
}

static bool WorldNoticeShoveCanFree(int kind) {
	return kind == kWorldPinTongue || kind == kWorldPinPounce || kind == kWorldPinJockey;
}

static void WorldNoticeClearPending() {
	g_worldPendingShoveHelper = 0;
	g_worldPendingShoveTarget = 0;
	g_worldPendingShoveAt = 0;
	g_worldPendingKillVictim = 0;
	g_worldPendingKillHelper = 0;
	g_worldPinProbeUntil = 0;
}

static bool WorldNoticeShoveMatches(int kind, int controller, int shoved) {
	if (kind == kWorldPinPounce || kind == kWorldPinJockey)
		return controller > 0 && shoved == controller;
	// Smoker: shove the victim or shove/interrupt the controlling Smoker.
	if (kind == kWorldPinTongue)
		return shoved == EngLocal() || (controller > 0 && shoved == controller);
	return false;
}

static void WorldNoticeAcceptShove(int shoved, int helper, DWORD at) {
	if (helper <= 0 || !at || GetTickCount() - at > kWorldPinShoveMs) return;
	const int kind = g_worldControlledBy > 0 ? g_worldPinKind : g_worldRecentPinKind;
	const int controller = g_worldControlledBy > 0 ? g_worldControlledBy : g_worldRecentController;
	if (!WorldNoticeShoveCanFree(kind)) return;
	if (!WorldNoticeShoveMatches(kind, controller, shoved)) return;
	g_worldRescueCandidate = helper;
	g_worldRescueCandidateAt = at;
}

static void WorldNoticeAskPinProbe() {
	const DWORD until = GetTickCount() + kWorldPinArmMs;
	if (until > g_worldPinProbeUntil) g_worldPinProbeUntil = until;
}

void WorldNoticeOnControlStarted(void* ev, int kind) {
	if (!RescueTrackWanted() || !ev) return;
	if (kind == kWorldPinNone) return;
	if (!WorldNoticeEventIsLocal(ev, "victim", "subject")) return;
	const int controller = WorldNoticeEventPlayer(ev, "userid", "attacker");
	WorldNoticeRememberPin(controller, kind);
	g_worldRescueCandidate = 0;
	g_worldRescueCandidateAt = 0;
	WorldNoticeClearPending();
}

bool WorldNoticeAirborneTrackingWanted() {
	// CS2 kill-feed gold Airshot rows reuse the same verified airborne stamps.
	return g_optWorldMeleeSkeet || g_optWorldSkeet || g_optElim || g_optKillFx
		|| ChapStatsSpecialTrackWanted()
		|| (g_optKillFeed && g_optKillFeedStyle == 1);
}

bool WorldNoticeChargerTrackingWanted() {
	// CS2 kill-feed Charge Break rows reuse the charge-window tracker.
	return g_optWorldLevel || g_optKillFx || ChapStatsSpecialTrackWanted()
		|| (g_optKillFeed && g_optKillFeedStyle == 1);
}

void WorldNoticeClearUnusedTrackers() {
	if (!WorldNoticeAirborneTrackingWanted())
		memset(g_localAirHitAt, 0, sizeof(g_localAirHitAt));
	if (!WorldNoticeChargerTrackingWanted())
		memset(g_chargerChargeUntil, 0, sizeof(g_chargerChargeUntil));
}

bool WorldNoticeReadAttackLeap(void* ent, int zombieClass) {
	if (!EntReadable(ent) || zombieClass != 3) return false;
	// Hunter's actual flight boolean lives on its replicated CLunge ability.
	// Read the damaged/victim ability on attributed HP drops or death, not a scan.
	if (g_offCustomAbility >= 0 && g_offCustomAbility <= 0x4000) {
		const int* handle = (const int*)((const uint8_t*)ent + g_offCustomAbility);
		if (PtrCommitted(handle)) {
			const int abilityIndex = EntIndexFromHandle(*handle);
			void* ability = abilityIndex > 0 ? EntGet(abilityIndex) : nullptr;
			if (EntReadable(ability) && g_offIsLunging >= 0 && g_offIsLunging <= 0x4000) {
				const unsigned char* active =
					(const unsigned char*)((const uint8_t*)ability + g_offIsLunging);
				if (PtrCommitted(active) && *active != 0) return true;
			}
		}
	}
	// Conservative Hunter-only fallback for client builds where CLunge is not
	// exposed.
	if (g_offIsAttemptingToPounce >= 0
		&& g_offIsAttemptingToPounce <= 0x4000) {
		const unsigned char* active =
			(const unsigned char*)((const uint8_t*)ent + g_offIsAttemptingToPounce);
		return PtrCommitted(active) && *active != 0;
	}
	return false;
}

// Called on Paint after HP attribution; only the damaged slot and its ability
// are read. No dependence on the unavailable client hurt events.
void WorldNoticeTrackAirborneEntity(void* e, int victim) {
	if (!WorldNoticeAirborneTrackingWanted()) return;
	if (victim <= 0 || victim >= 65 || g_offFlags < 0 || g_offFlags > 0x4000) return;
	if (!EntReadable(e)) return;
	int zombieClass = 0;
	if (g_offZombieClass >= 0 && g_offZombieClass <= 0x4000) {
		const int* zc = (const int*)((const uint8_t*)e + g_offZombieClass);
		if (!PtrCommitted(zc) || (*zc != 3 && *zc != 5)) return; // Hunter / Jockey only
		zombieClass = *zc;
	}
	const int* flags = (const int*)((const uint8_t*)e + g_offFlags);
	if (!PtrCommitted(flags)) return;
	// Jockey's client base sequence is not a verified attack signal. Keep
	// its accepted airborne rule until runtime diagnostics prove a replacement.
	const bool attackAirborne = ((*flags & 1) == 0)
		&& (zombieClass == 5 || WorldNoticeReadAttackLeap(e, zombieClass));
	g_localAirHitAt[victim] = attackAirborne ? GetTickCount() : 0;
}


void WorldNoticeOnChargerCharge(void* ev, bool started) {
	if (!SkeetoFeaturesOn() || !ev) return;
	const int charger = WorldNoticeEventPlayer(ev, "userid", "attacker");
	if (charger <= 0 || charger >= 65) return;
	const DWORD now = GetTickCount();
	// charge_end can precede player_death in the same attack; retain a tiny grace window.
	g_chargerChargeUntil[charger] = started ? now + 8000u : now + 450u;
}

bool WorldNoticeChargeWindow(int ent) {
	return ent > 0 && ent < 65 && g_chargerChargeUntil[ent]
		&& GetTickCount() <= g_chargerChargeUntil[ent];
}

bool WorldNoticeWasCharging(int ent) {
	return WorldNoticeChargerTrackingWanted() && WorldNoticeChargeWindow(ent);
}

void WorldNoticeTriggerRescue(int helper) {
	if (!RescueTrackWanted() || !WorldNoticeSurvivorEnt(helper)) return;
	if (g_optMateSave)
		MateAlertOnRescue(helper);
	if (!g_optWorldRescue) return;
	const DWORD now = GetTickCount();
	g_worldRescueNotice.ent = helper;
	g_worldRescueNotice.born = now;
	g_worldRescueNotice.until = now + 2000u;
}

void WorldNoticeRescuePump() {
	WorldNoticeControlTick();
}

static void WorldNoticeBeginReleaseWait() {
	if (g_worldPinReleaseAt) return;
	if (g_worldControlledBy > 0) {
		g_worldRecentController = g_worldControlledBy;
		g_worldRecentPinKind = g_worldPinKind;
		g_worldControlledRecentUntil = GetTickCount() + kWorldPinKillGraceMs;
	}
	g_worldControlledBy = 0;
	g_worldPinKind = 0;
	g_worldPinArmedAt = 0;
	g_worldPinReleaseAt = GetTickCount();
	g_worldControlPollAt = g_worldPinReleaseAt;
}

static void WorldNoticeFinishShoveCandidate() {
	const int kind = g_worldRecentPinKind;
	const int helper = g_worldRescueCandidate;
	const DWORD marked = g_worldRescueCandidateAt;
	g_worldPinReleaseAt = 0;
	g_worldRescueCandidate = 0;
	g_worldRescueCandidateAt = 0;
	g_worldControlledBy = 0;
	g_worldPinKind = 0;
	if (!WorldNoticeShoveCanFree(kind) || helper <= 0 || !marked) return;
	if (GetTickCount() - marked > kWorldPinShoveMs) return;
	WorldNoticeTriggerRescue(helper);
}

void WorldNoticeControlTick() {
	if (!RescueTrackWanted()) {
		WorldNoticeClearPinTrack();
		return;
	}
	const DWORD now = GetTickCount();
	const bool probing = g_worldPinProbeUntil && now <= g_worldPinProbeUntil;
	if (g_worldControlledBy <= 0 && !g_worldPinReleaseAt && !probing) {
		if (g_worldPendingShoveHelper || g_worldPendingKillVictim) WorldNoticeClearPending();
		return;
	}
	if (g_worldControlPollAt && now - g_worldControlPollAt < kWorldPinPollMs) return;
	g_worldControlPollAt = now;
	if (g_worldPinReleaseAt) {
		if (now - g_worldPinReleaseAt >= kWorldPinKillWaitMs)
			WorldNoticeFinishShoveCandidate();
		return;
	}
	int ent = 0, kind = 0;
	if (WorldNoticeReadLocalPin(&ent, &kind)) {
		if (ent != g_worldControlledBy)
			WorldNoticeRememberPin(ent, kind);
		else {
			g_worldRecentController = ent;
			g_worldControlledRecentUntil = now + kWorldPinKillGraceMs;
		}
		if (g_worldPendingKillVictim == ent && WorldNoticeSurvivorEnt(g_worldPendingKillHelper)) {
			const int helper = g_worldPendingKillHelper;
			WorldNoticeClearPending();
			g_worldRescueCandidate = 0;
			g_worldRescueCandidateAt = 0;
			g_worldPinReleaseAt = 0;
			g_worldControlledBy = 0;
			g_worldPinKind = 0;
			g_worldControlledRecentUntil = 0;
			WorldNoticeTriggerRescue(helper);
			return;
		}
		if (g_worldPendingShoveHelper)
			WorldNoticeAcceptShove(g_worldPendingShoveTarget, g_worldPendingShoveHelper,
				g_worldPendingShoveAt);
		WorldNoticeClearPending();
		return;
	}
	if (g_worldControlledBy <= 0) {
		if (!probing) WorldNoticeClearPending();
		return;
	}
	if (g_worldPinArmedAt && now - g_worldPinArmedAt < kWorldPinArmMs) return;
	WorldNoticeBeginReleaseWait();
}

void WorldNoticeOnPinShove(void* ev) {
	if (!RescueTrackWanted() || !ev) return;
	int shoved = WorldNoticeEventPlayer(ev, "userid", "entityid");
	int helper = WorldNoticeEventPlayer(ev, "attacker", "attackerentid");
	if (!WorldNoticeSurvivorEnt(helper) && WorldNoticeSurvivorEnt(shoved)) {
		const int swap = shoved;
		shoved = helper;
		helper = swap;
	}
	if (!WorldNoticeSurvivorEnt(helper)) return;
	const DWORD now = GetTickCount();
	const int kind = g_worldControlledBy > 0 ? g_worldPinKind
		: ((g_worldPinReleaseAt || (g_worldControlledRecentUntil && now <= g_worldControlledRecentUntil))
			? g_worldRecentPinKind : 0);
	if (kind != 0) {
		WorldNoticeAcceptShove(shoved, helper, now);
		return;
	}
	g_worldPendingShoveHelper = helper;
	g_worldPendingShoveTarget = shoved;
	g_worldPendingShoveAt = now;
	WorldNoticeAskPinProbe();
}

void WorldNoticeOnPinKill(void* ev) {
	if (!RescueTrackWanted() || !ev) return;
	const char* name = EvStr(ev, "victimname");
	const bool pinnedSi = name && (!strcmp(name, "Smoker") || !strcmp(name, "Hunter")
		|| !strcmp(name, "Jockey") || !strcmp(name, "Charger"));
	if (!pinnedSi) return;
	const int victim = WorldNoticeEventPlayer(ev, "userid", "entityid");
	const int helper = WorldNoticeEventPlayer(ev, "attacker", "attackerentid");
	if (victim <= 0 || !WorldNoticeSurvivorEnt(helper)) return;
	const int controller = WorldNoticeKillController();
	if (controller <= 0) {
		g_worldPendingKillVictim = victim;
		g_worldPendingKillHelper = helper;
		WorldNoticeAskPinProbe();
		return;
	}
	if (victim != controller) return;
	WorldNoticeClearPending();
	g_worldRescueCandidate = 0;
	g_worldRescueCandidateAt = 0;
	g_worldPinReleaseAt = 0;
	g_worldControlledBy = 0;
	g_worldPinKind = 0;
	g_worldControlledRecentUntil = 0;
	WorldNoticeTriggerRescue(helper);
}

void WorldNoticeOnPinEnd(void* ev, const char* whoKey) {
	if (!RescueTrackWanted() || !ev) return;
	const int victimId = EvInt(ev, "victim", 0);
	const bool localVictim = WorldNoticeEventIsLocal(ev, "victim", "subject");
	const int eventController = WorldNoticeEventPlayer(ev, "userid", "attacker");
	if (victimId > 0 && !localVictim) return;
	if (!localVictim && (g_worldControlledBy <= 0 || eventController != g_worldControlledBy)
		&& (g_worldRecentController <= 0 || eventController != g_worldRecentController))
		return;
	if (whoKey && whoKey[0] && localVictim) {
		const int who = EvInt(ev, whoKey, 0);
		if (who > 0 && who != victimId) {
			WorldNoticeClearPending();
			g_worldRescueCandidate = 0;
			g_worldRescueCandidateAt = 0;
			g_worldPinReleaseAt = 0;
			g_worldControlledBy = 0;
			g_worldPinKind = 0;
			g_worldControlledRecentUntil = 0;
			WorldNoticeTriggerRescue(WorldNoticeEventPlayer(ev, whoKey));
			return;
		}
	}
	WorldNoticeBeginReleaseWait();
}

void WorldNoticeOnReviveSuccess(void* ev) {
	if (!RescueTrackWanted() || !ev || !WorldNoticeEventIsLocal(ev, "subject")) return;
	WorldNoticeTriggerRescue(WorldNoticeEventPlayer(ev, "userid"));
}

void WorldNoticeOnDefibSuccess(void* ev) {
	if (!RescueTrackWanted() || !ev || !WorldNoticeEventIsLocal(ev, "subject")) return;
	WorldNoticeTriggerRescue(WorldNoticeEventPlayer(ev, "userid"));
}

// Event-only: player_now_it / player_no_longer_it. No idle remapping —
// that path caused false triggers and stuck markers worse than idle hide.
void WorldNoticeOnBile(void* ev, bool active) {
	if (!g_optWorldBileMark || !ev) return;
	const int ent = WorldNoticeEventPlayer(ev, "userid", "subject");
	if (!WorldNoticeSurvivorEnt(ent)) return;
	if (!active) {
		for (int i = 0; i < kTeamHudMax; ++i)
			if (g_worldBileNotices[i].ent == ent) g_worldBileNotices[i] = {};
		g_worldBileAnyActive = false;
		for (int i = 0; i < kTeamHudMax; ++i)
			if (g_worldBileNotices[i].ent) { g_worldBileAnyActive = true; break; }
		return;
	}
	int freeSlot = -1;
	for (int i = 0; i < kTeamHudMax; ++i) {
		if (g_worldBileNotices[i].ent == ent) { freeSlot = i; break; }
		if (freeSlot < 0 && g_worldBileNotices[i].ent == 0) freeSlot = i;
	}
	if (freeSlot < 0) freeSlot = 0;
	g_worldBileNotices[freeSlot].ent = ent;
	g_worldBileNotices[freeSlot].born = GetTickCount();
	g_worldBileAnyActive = true;
}

bool WorldNoticeSnapshotInfected(WorldInfectedNoticeState* out, int ent, float eventX, float eventY, float eventZ) {
	if (!out) return false;
	float x = eventX, y = eventY, z = eventZ;
	void* e = ent > 0 ? EntGet(ent) : nullptr;
	if ((x == 0.f && y == 0.f && z == 0.f) && EntReadable(e)
		&& g_offOrigin >= 0 && g_offOrigin <= 0x4000) {
		const float* o = (const float*)((const uint8_t*)e + g_offOrigin);
		if (PtrCommitted(o)) { x = o[0]; y = o[1]; z = o[2]; }
	}
	if (x == 0.f && y == 0.f && z == 0.f) return false;
	int zc = 0;
	if (EntReadable(e) && g_offZombieClass >= 0 && g_offZombieClass <= 0x4000) {
		const int* pz = (const int*)((const uint8_t*)e + g_offZombieClass);
		if (PtrCommitted(pz)) zc = *pz;
	}
	out->world[0] = x;
	out->world[1] = y;
	out->world[2] = z + LocalPlayHeadLift(zc) + 12.f;
	const DWORD now = GetTickCount();
	out->born = now;
	out->until = now + 2000u;
	return true;
}

void WorldNoticeOnWitchKilled(void* ev) {
	if (!g_optWorldWitchKill || !ev) return;
	int killerUid = EvInt(ev, "userid");
	if (killerUid <= 0) killerUid = EvInt(ev, "attacker");
	const int killer = killerUid > 0 ? EngPlayerForUserID(killerUid) : 0;
	if (!WorldNoticeSurvivorEnt(killer)) return;
	const DWORD now = GetTickCount();
	g_worldWitchNotice.ent = killer;
	g_worldWitchNotice.born = now;
	g_worldWitchNotice.until = now + 5000u;
}

void WorldNoticeOnTankKilled(void* ev) {
	if (!g_optWorldTankKill || !ev) return;
	const char* victim = EvStr(ev, "victimname");
	if (!victim || _stricmp(victim, "Tank")) return;
	int killerUid = EvInt(ev, "attacker");
	if (killerUid <= 0) killerUid = EvInt(ev, "userid");
	const int killer = killerUid > 0 ? EngPlayerForUserID(killerUid) : 0;
	if (!WorldNoticeSurvivorEnt(killer)) return;
	const DWORD now = GetTickCount();
	g_worldTankNotice.ent = killer;
	g_worldTankNotice.born = now;
	g_worldTankNotice.until = now + 5000u;
}

void WorldNoticeOnTankSlayer(void* ev) {
	if (!g_optWorldTankSlayer || !ev || !IsLocalAttacker(ev)) return;
	const char* victim = EvStr(ev, "victimname");
	if (!victim || _stricmp(victim, "Tank")) return;
	const int ent = WorldNoticeEventPlayer(ev, "userid", "entityid");
	if (WorldNoticeSnapshotInfected(&g_worldTankSlayerNotice, ent,
		EvFloat(ev, "victim_x"), EvFloat(ev, "victim_y"), EvFloat(ev, "victim_z"))) {
		// SnapshotInfected already applies Tank's head-height anchor. Lower this
		// large badge to the upper torso instead of placing it above the model.
		g_worldTankSlayerNotice.world[2] -= 18.f;
		g_worldTankSlayerNotice.until = g_worldTankSlayerNotice.born + 2600u;
	}
}

bool WorldNoticeHeadScreen(int ent, float* sx, float* sy, int sw, int sh) {
	if (!sx || !sy || !WorldNoticeSurvivorEnt(ent)) return false;
	void* e = EntGet(ent);
	if (!e || g_offOrigin < 0 || g_offOrigin > 0x4000) return false;
	const float* o = (const float*)((const uint8_t*)e + g_offOrigin);
	if (!PtrCommitted(o)) return false;
	float world[3] = { o[0], o[1], o[2] };
	float lift = 70.f;
	if (g_offViewOffset >= 0 && g_offViewOffset <= 0x4000) {
		const float* vo = (const float*)((const uint8_t*)e + g_offViewOffset);
		if (PtrCommitted(vo) && vo[2] > 16.f && vo[2] < 120.f)
			lift = vo[2] + 9.f;
	}
	world[2] += lift;
	return LocalPlayWorldToScreen(world, sx, sy, sw, sh);
}

void WorldNoticeDrawTrophy(int cx, int cy, int alpha, const wchar_t* text) {
	// Tiny trophy silhouette: two handles, tapered cup, stem and base.
	SurfColor(0, 0, 0, alpha * 2 / 3);
	SurfFill(cx - 15, cy - 13, cx + 16, cy - 8);
	SurfFill(cx - 12, cy - 10, cx + 13, cy + 7);
	SurfFill(cx - 3, cy + 6, cx + 4, cy + 16);
	SurfFill(cx - 10, cy + 15, cx + 11, cy + 20);
	SurfColor(255, 188, 46, alpha);
	SurfFill(cx - 12, cy - 12, cx + 13, cy - 9);
	for (int r = 0; r < 12; ++r) {
		const int inset = r / 4;
		SurfFill(cx - 10 + inset, cy - 9 + r, cx + 11 - inset, cy - 8 + r);
	}
	SurfLine(cx - 10, cy - 8, cx - 17, cy - 8);
	SurfLine(cx - 17, cy - 8, cx - 14, cy + 2);
	SurfLine(cx - 14, cy + 2, cx - 7, cy + 4);
	SurfLine(cx + 10, cy - 8, cx + 17, cy - 8);
	SurfLine(cx + 17, cy - 8, cx + 14, cy + 2);
	SurfLine(cx + 14, cy + 2, cx + 7, cy + 4);
	SurfFill(cx - 2, cy + 3, cx + 3, cy + 15);
	SurfFill(cx - 9, cy + 14, cx + 10, cy + 18);
	SurfColor(255, 236, 145, alpha * 3 / 4);
	SurfLine(cx - 8, cy - 9, cx + 8, cy - 9);
	if (g_teamWorldFont) {
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, cy + 20, RGB(255, 210, 92), text);
	}
}

void WorldNoticeDrawRescue(int cx, int cy, int alpha) {
	// Cyan shield + medical cross, deliberately compact and texture-free.
	SurfColor(8, 20, 24, alpha * 3 / 4);
	SurfFill(cx - 17, cy - 15, cx + 18, cy + 9);
	SurfColor(92, 232, 238, alpha);
	SurfLine(cx - 16, cy - 14, cx + 16, cy - 14);
	SurfLine(cx - 16, cy - 14, cx - 13, cy + 8);
	SurfLine(cx + 16, cy - 14, cx + 13, cy + 8);
	SurfLine(cx - 13, cy + 8, cx, cy + 18);
	SurfLine(cx + 13, cy + 8, cx, cy + 18);
	SurfFill(cx - 3, cy - 10, cx + 4, cy + 11);
	SurfFill(cx - 10, cy - 3, cx + 11, cy + 4);
}

static void WorldNoticeDrawFriendlyFire(int cx, int cy, int alpha) {
	// Restored original compact warning diamond; primitive-only, no material allocation.
	SurfColor(0, 0, 0, alpha * 3 / 4);
	for (int d = 0; d < 3; ++d) {
		SurfLine(cx, cy - 18 - d, cx + 18 + d, cy);
		SurfLine(cx + 18 + d, cy, cx, cy + 18 + d);
		SurfLine(cx, cy + 18 + d, cx - 18 - d, cy);
		SurfLine(cx - 18 - d, cy, cx, cy - 18 - d);
	}
	SurfColor(255, 54, 46, alpha);
	SurfLine(cx, cy - 17, cx + 17, cy);
	SurfLine(cx + 17, cy, cx, cy + 17);
	SurfLine(cx, cy + 17, cx - 17, cy);
	SurfLine(cx - 17, cy, cx, cy - 17);
	SurfFill(cx - 2, cy - 10, cx + 3, cy + 5);
	SurfFill(cx - 2, cy + 9, cx + 3, cy + 14);
	if (g_teamWorldFont) {
		const wchar_t* text = UiText(UiId::WorldFriendlyFireLabel, L"友伤");
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, cy + 20, RGB(255, 105, 92), text);
	}
}

void WorldNoticeThickLine(int x0, int y0, int x1, int y1, int thick,
	int r, int g, int b, int alpha) {
	if (thick < 1) thick = 1;
	SurfColor(r, g, b, alpha);
	const bool wide = abs(x1 - x0) >= abs(y1 - y0);
	const int half = thick / 2;
	for (int d = -half; d <= half; ++d) {
		if (wide) SurfLine(x0, y0 + d, x1, y1 + d);
		else SurfLine(x0 + d, y0, x1 + d, y1);
	}
}

void WorldNoticeDrawTechDiamond(int cx, int cy, int radius, int r, int g, int b, int alpha) {
	// Dense dark core isolates the emblem from bright maps; broken outer rails add motion.
	SurfColor(3, 8, 13, alpha * 4 / 5);
	for (int y = -radius + 7; y <= radius - 7; ++y) {
		const int span = radius - 7 - abs(y);
		SurfFill(cx - span, cy + y, cx + span + 1, cy + y + 1);
	}
	for (int d = 0; d < 3; ++d) {
		SurfColor(r, g, b, alpha * (3 - d) / 3);
		const int rr = radius - d * 2;
		SurfLine(cx, cy - rr, cx + rr, cy);
		SurfLine(cx + rr, cy, cx, cy + rr);
		SurfLine(cx, cy + rr, cx - rr, cy);
		SurfLine(cx - rr, cy, cx, cy - rr);
	}
	SurfColor(r, g, b, alpha / 2);
	SurfLine(cx - radius - 8, cy - 8, cx - radius - 2, cy - 2);
	SurfLine(cx + radius + 8, cy + 8, cx + radius + 2, cy + 2);
	SurfLine(cx - 8, cy + radius + 8, cx - 2, cy + radius + 2);
	SurfLine(cx + 8, cy - radius - 8, cx + 2, cy - radius - 2);
}

int WorldNoticeKillPx(int v) {
	if (v == 0) return 0;
	const int mag = HudScalePx(v < 0 ? -v : v, 140);
	return v < 0 ? -mag : mag;
}

bool WorldNoticeProjectCloseKill(const float* world, float* sx, float* sy, int sw, int sh) {
	if (!world || !sx || !sy) return false;
	if (!LocalPlayWorldToScreen(world, sx, sy, sw, sh)) {
		// At melee distance the victim anchor can cross the camera near plane. Keep
		// this short-lived local achievement visible near the upper crosshair only
		// when the stored world point is genuinely close; far off-screen targets
		// remain hidden like ordinary world HUD elements.
		float eye[3]{};
		if (!LocalEyePos(eye)) return false;
		const float dx = world[0] - eye[0];
		const float dy = world[1] - eye[1];
		const float dz = world[2] - eye[2];
		if (dx * dx + dy * dy + dz * dz > 180.f * 180.f) return false;
		*sx = sw * 0.5f;
		*sy = sh * 0.40f;
	}
	const int marginX = WorldNoticeKillPx(48);
	const int marginTop = WorldNoticeKillPx(52);
	const int marginBottom = WorldNoticeKillPx(64);
	if (*sx < marginX) *sx = (float)marginX;
	if (*sx > sw - marginX) *sx = (float)(sw - marginX);
	if (*sy < marginTop) *sy = (float)marginTop;
	if (*sy > sh - marginBottom) *sy = (float)(sh - marginBottom);
	return true;
}

void WorldNoticeDrawMeleeSkeet(int cx, int cy, int alpha) {
	auto p = [](int v) { return WorldNoticeKillPx(v); };
	WorldNoticeDrawTechDiamond(cx, cy, p(29), 42, 178, 255, alpha);
	// Ice wings / impact shards.
	WorldNoticeThickLine(cx + p(-25), cy + p(8), cx + p(-9), cy + p(-5), p(3), 48, 185, 255, alpha);
	WorldNoticeThickLine(cx + p(-23), cy + p(16), cx + p(-8), cy + p(3), p(2), 86, 221, 255, alpha);
	WorldNoticeThickLine(cx + p(25), cy + p(8), cx + p(9), cy + p(-5), p(3), 48, 185, 255, alpha);
	WorldNoticeThickLine(cx + p(23), cy + p(16), cx + p(8), cy + p(3), p(2), 86, 221, 255, alpha);
	// Large diagonal blade with dark shadow, bright spine and guard.
	WorldNoticeThickLine(cx + p(-10), cy + p(21), cx + p(11), cy + p(-22), p(8), 0, 10, 18, alpha);
	WorldNoticeThickLine(cx + p(-8), cy + p(18), cx + p(12), cy + p(-23), p(5), 214, 244, 255, alpha);
	WorldNoticeThickLine(cx + p(-5), cy + p(15), cx + p(11), cy + p(-19), p(2), 255, 255, 255, alpha);
	WorldNoticeThickLine(cx + p(-14), cy + p(12), cx + p(4), cy + p(20), p(5), 22, 104, 170, alpha);
	SurfColor(100, 220, 255, alpha);
	SurfFill(cx + p(-10), cy + p(19), cx + p(-4), cy + p(27));
	if (g_worldKillFont) {
		const wchar_t* text = L"MELEE";
		const int tw = SurfTextWFont(g_worldKillFont, text);
		SurfTextAt(g_worldKillFont, cx - tw / 2, cy + p(33), RGB(118, 220, 255), text);
	}
}

void WorldNoticeDrawLevel(int cx, int cy, int alpha) {
	auto p = [](int v) { return WorldNoticeKillPx(v); };
	WorldNoticeDrawTechDiamond(cx, cy, p(30), 255, 116, 22, alpha);
	// Twin forward chevrons make the charging direction readable at a glance.
	WorldNoticeThickLine(cx + p(-25), cy + p(-16), cx + p(-7), cy, p(6), 116, 40, 4, alpha);
	WorldNoticeThickLine(cx + p(-7), cy, cx + p(-25), cy + p(16), p(6), 230, 72, 8, alpha);
	WorldNoticeThickLine(cx + p(2), cy + p(-16), cx + p(20), cy, p(6), 180, 52, 4, alpha);
	WorldNoticeThickLine(cx + p(20), cy, cx + p(2), cy + p(16), p(6), 255, 130, 24, alpha);
	// Heavy knife cutting against the direction of travel.
	WorldNoticeThickLine(cx + p(-14), cy + p(22), cx + p(13), cy + p(-22), p(9), 12, 6, 2, alpha);
	WorldNoticeThickLine(cx + p(-11), cy + p(19), cx + p(15), cy + p(-23), p(5), 255, 225, 182, alpha);
	WorldNoticeThickLine(cx + p(-7), cy + p(15), cx + p(13), cy + p(-18), p(2), 255, 255, 238, alpha);
	WorldNoticeThickLine(cx + p(-18), cy + p(12), cx + p(2), cy + p(23), p(5), 122, 48, 8, alpha);
	SurfColor(255, 171, 54, alpha);
	SurfFill(cx + p(-14), cy + p(20), cx + p(-7), cy + p(29));
	if (g_worldKillFont) {
		const wchar_t* text = L"LEVEL";
		const int tw = SurfTextWFont(g_worldKillFont, text);
		SurfTextAt(g_worldKillFont, cx - tw / 2, cy + p(34), RGB(255, 166, 66), text);
	}
}

void WorldNoticeDrawSkeet(int cx, int cy, int alpha) {
	WorldNoticeDrawTechDiamond(cx, cy, 29, 255, 48, 42, alpha);
	// Ballistic seal: concentric broken rails + white-hot tracer into a dark impact core.
	SurfColor(255, 70, 58, alpha);
	for (int d = 0; d < 2; ++d) {
		const int r = 21 - d * 4;
		SurfLine(cx - r, cy - 7, cx - 12, cy - r);
		SurfLine(cx + 7, cy - r, cx + r, cy - 10);
		SurfLine(cx + r, cy + 7, cx + 11, cy + r);
		SurfLine(cx - 7, cy + r, cx - r, cy + 10);
	}
	WorldNoticeThickLine(cx - 25, cy + 18, cx + 17, cy - 14, 7, 18, 2, 2, alpha);
	WorldNoticeThickLine(cx - 23, cy + 16, cx + 18, cy - 15, 4, 255, 88, 68, alpha);
	WorldNoticeThickLine(cx - 19, cy + 13, cx + 16, cy - 13, 1, 255, 246, 224, alpha);
	// Impact crack, deliberately asymmetric so it reads as a shot rather than a generic star.
	WorldNoticeThickLine(cx, cy, cx - 4, cy - 23, 3, 255, 58, 48, alpha);
	WorldNoticeThickLine(cx, cy, cx + 23, cy + 4, 3, 255, 58, 48, alpha);
	WorldNoticeThickLine(cx, cy, cx + 9, cy + 22, 2, 255, 120, 80, alpha);
	WorldNoticeThickLine(cx, cy, cx - 21, cy - 5, 2, 255, 120, 80, alpha);
	SurfColor(2, 2, 3, alpha);
	SurfFill(cx - 6, cy - 6, cx + 7, cy + 7);
	SurfColor(255, 218, 186, alpha);
	SurfOutline(cx - 7, cy - 7, cx + 8, cy + 8);
	if (g_teamWorldFont) {
		const wchar_t* text = L"SKEET";
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, cy + 33, RGB(255, 106, 84), text);
	}
}

void WorldNoticeDrawTongueCut(int cx, int cy, int alpha) {
	WorldNoticeDrawTechDiamond(cx, cy, 23, 156, 236, 32, alpha);
	// Two severed tongue segments, curled away from a bright cutting slash.
	WorldNoticeThickLine(cx - 18, cy - 6, cx - 9, cy - 10, 6, 62, 90, 18, alpha);
	WorldNoticeThickLine(cx - 10, cy - 10, cx - 3, cy - 4, 5, 128, 190, 30, alpha);
	WorldNoticeThickLine(cx + 4, cy + 5, cx + 10, cy + 11, 5, 128, 190, 30, alpha);
	WorldNoticeThickLine(cx + 10, cy + 11, cx + 18, cy + 6, 6, 62, 90, 18, alpha);
	SurfColor(220, 255, 92, alpha);
	SurfFill(cx - 5, cy - 7, cx - 2, cy - 4);
	SurfFill(cx + 2, cy + 2, cx + 6, cy + 6);
	// Neon sever line and two small fragments.
	WorldNoticeThickLine(cx - 18, cy + 18, cx + 18, cy - 18, 6, 4, 12, 2, alpha);
	WorldNoticeThickLine(cx - 17, cy + 17, cx + 17, cy - 17, 3, 202, 255, 62, alpha);
	WorldNoticeThickLine(cx - 14, cy + 14, cx + 15, cy - 15, 1, 248, 255, 190, alpha);
	SurfColor(174, 248, 42, alpha);
	SurfFill(cx - 21, cy + 4, cx - 17, cy + 8);
	SurfFill(cx + 17, cy - 9, cx + 22, cy - 5);
	if (g_teamWorldFont) {
		const wchar_t* text = L"CUT";
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, cy + 27, RGB(180, 245, 58), text);
	}
}

void WorldNoticeDrawCrown(int cx, int cy, int alpha, int kind) {
	auto p = [](int v) { return WorldNoticeKillPx(v); };
	const bool melee = kind == 1;
	const bool perfect = kind == 3;
	const int r = melee ? 72 : 255;
	const int g = melee ? 205 : (perfect ? 92 : 190);
	const int b = melee ? 255 : (perfect ? 210 : 38);
	WorldNoticeDrawTechDiamond(cx, cy, p(perfect ? 31 : 27), r, g, b, alpha);
	// Wide crown silhouette with three readable spikes and a bright lower rail.
	SurfColor(0, 0, 0, alpha * 3 / 4);
	SurfFill(cx + p(-25), cy + p(4), cx + p(26), cy + p(17));
	WorldNoticeThickLine(cx + p(-24), cy + p(5), cx + p(-20), cy + p(-17), p(7), 0, 0, 0, alpha);
	WorldNoticeThickLine(cx + p(-5), cy + p(5), cx, cy + p(-25), p(8), 0, 0, 0, alpha);
	WorldNoticeThickLine(cx + p(20), cy + p(5), cx + p(24), cy + p(-17), p(7), 0, 0, 0, alpha);
	SurfColor(r, g, b, alpha);
	SurfFill(cx + p(-22), cy + p(5), cx + p(23), cy + p(14));
	WorldNoticeThickLine(cx + p(-20), cy + p(6), cx + p(-18), cy + p(-15), p(4), r, g, b, alpha);
	WorldNoticeThickLine(cx + p(-18), cy + p(-15), cx + p(-6), cy + p(5), p(4), r, g, b, alpha);
	WorldNoticeThickLine(cx + p(-5), cy + p(5), cx, cy + p(-23), p(5), r, g, b, alpha);
	WorldNoticeThickLine(cx, cy + p(-23), cx + p(7), cy + p(5), p(5), r, g, b, alpha);
	WorldNoticeThickLine(cx + p(7), cy + p(5), cx + p(21), cy + p(-15), p(4), r, g, b, alpha);
	WorldNoticeThickLine(cx + p(21), cy + p(-15), cx + p(22), cy + p(6), p(4), r, g, b, alpha);
	SurfColor(255, 255, 230, alpha * 3 / 4);
	SurfFill(cx + p(-18), cy + p(7), cx + p(19), cy + p(9));
	if (perfect) {
		for (int d = 0; d < 3; ++d) {
			SurfLine(cx + p(-36) - d, cy, cx + p(-29) - d, cy);
			SurfLine(cx + p(29) + d, cy, cx + p(36) + d, cy);
		}
	}
	if (g_worldKillFont) {
		const wchar_t* text = melee ? L"MELEE CROWN" : (perfect ? L"PERFECT" : L"CROWN");
		const int tw = SurfTextWFont(g_worldKillFont, text);
		SurfTextAt(g_worldKillFont, cx - tw / 2, cy + p(23), RGB(r, g, b), text);
	}
}

void WorldNoticeDrawActionProgress(int cx, int cy, float frac, float remain) {
	if (frac < 0.f) return;
	if (frac > 1.f) frac = 1.f;
	const int w = 86, h = 9, cut = 7;
	const int x = cx - w / 2, y = cy - 72;
	SurfColor(3, 10, 15, 205);
	TeamHudFillChamfer(x - 3, y - 3, w + 6, h + 6, cut);
	SurfColor(39, 70, 82, 235);
	TeamHudFillChamfer(x, y, w, h, 5);
	int fill = (int)(w * frac + 0.5f);
	if (fill > 0) {
		SurfColor(50, 220, 242, 245);
		SurfFill(x, y, x + fill, y + h);
		SurfColor(210, 255, 255, 210);
		SurfFill(x, y, x + fill, y + 2);
	}
	SurfColor(92, 238, 255, 225);
	SurfLine(x - 7, y + h / 2, x - 2, y + h / 2);
	SurfLine(x + w + 2, y + h / 2, x + w + 7, y + h / 2);
	if (g_teamWorldFont) {
		wchar_t text[24]{};
		swprintf_s(text, 24, UiText(UiId::Text_e27703b2, L"行动受限中  %.1fs"), remain > 0.f ? remain : 0.f);
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, y - 18, RGB(118, 235, 248), text);
	}
}

void WorldNoticeDrawBile(int cx, int cy, int alpha) {
	WorldNoticeDrawTechDiamond(cx, cy, 25, 184, 235, 36, alpha);
	// Acid droplet plus warning slash: bright enough to survive green/brown maps.
	SurfColor(10, 16, 2, alpha * 3 / 4);
	SurfFill(cx - 10, cy - 7, cx + 11, cy + 13);
	SurfColor(190, 246, 42, alpha);
	for (int y = -16; y <= 8; ++y) {
		const int half = y < 0 ? (y + 16) / 2 : 12 - y / 2;
		SurfFill(cx - half, cy + y, cx + half + 1, cy + y + 1);
	}
	SurfColor(248, 255, 196, alpha);
	SurfFill(cx - 3, cy - 7, cx + 1, cy + 5);
	SurfColor(24, 30, 5, alpha);
	SurfFill(cx - 2, cy + 13, cx + 3, cy + 18);
	if (g_teamWorldFont) {
		const wchar_t* text = UiText(UiId::Text_df334bd4, L"胆汁危险");
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, cy + 28, RGB(202, 247, 58), text);
	}
}

void WorldNoticeDrawTankSlayer(int cx, int cy, int alpha) {
	// Heavy crimson hunter crest: fractured outer diamond, horned skull and kill slash.
	WorldNoticeDrawTechDiamond(cx, cy, 34, 255, 52, 34, alpha);
	SurfColor(8, 2, 2, alpha * 4 / 5);
	TeamHudFillChamfer(cx - 25, cy - 22, 50, 47, 9);
	SurfColor(255, 66, 38, alpha);
	WorldNoticeThickLine(cx - 31, cy - 22, cx - 17, cy - 8, 4, 255, 66, 38, alpha);
	WorldNoticeThickLine(cx + 31, cy - 22, cx + 17, cy - 8, 4, 255, 66, 38, alpha);
	// Broad skull silhouette and two angular horns.
	SurfColor(224, 226, 212, alpha);
	for (int y = -15; y <= 9; ++y) {
		const int half = y < -7 ? 13 + (y + 15) : (y > 3 ? 14 - (y - 3) : 20);
		SurfFill(cx - half, cy + y, cx + half + 1, cy + y + 1);
	}
	WorldNoticeThickLine(cx - 15, cy - 10, cx - 28, cy - 24, 6, 224, 226, 212, alpha);
	WorldNoticeThickLine(cx + 15, cy - 10, cx + 28, cy - 24, 6, 224, 226, 212, alpha);
	SurfColor(14, 2, 2, alpha);
	SurfFill(cx - 13, cy - 3, cx - 4, cy + 5);
	SurfFill(cx + 4, cy - 3, cx + 13, cy + 5);
	SurfFill(cx - 3, cy + 6, cx + 4, cy + 13);
	SurfColor(255, 57, 35, alpha);
	SurfFill(cx - 10, cy, cx - 6, cy + 3);
	SurfFill(cx + 6, cy, cx + 10, cy + 3);
	// White-hot diagonal execution slash keeps the badge readable at distance.
	WorldNoticeThickLine(cx - 28, cy + 25, cx + 27, cy - 27, 7, 20, 2, 2, alpha);
	WorldNoticeThickLine(cx - 27, cy + 24, cx + 26, cy - 26, 3, 255, 96, 58, alpha);
	WorldNoticeThickLine(cx - 24, cy + 21, cx + 23, cy - 23, 1, 255, 244, 210, alpha);
	if (g_teamWorldFont) {
		const wchar_t* text = L"TANK SLAYER";
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, cy + 39, RGB(255, 92, 58), text);
	}
}

void WorldNoticeDrawWitchStartle(int cx, int cy, int alpha) {
	// Warning triangle + claw slashes: readable at teammate-head scale without textures.
	const int h = 36, half = 18;
	for (int y = 0; y < h; ++y) {
		const int span = (half * y + h / 2) / h;
		SurfColor(18, 6, 6, alpha * 4 / 5);
		SurfFill(cx - span - 1, cy - 18 + y, cx + span + 2, cy - 17 + y);
	}
	for (int y = 2; y < h - 2; ++y) {
		int span = (half * y + h / 2) / h - 2;
		if (span < 1) continue;
		SurfColor(40, 8, 8, alpha);
		SurfFill(cx - span, cy - 18 + y, cx + span + 1, cy - 17 + y);
	}
	SurfColor(255, 56, 48, alpha);
	for (int y = 0; y < h; ++y) {
		const int span = (half * y + h / 2) / h;
		SurfFill(cx - span, cy - 18 + y, cx - span + 2, cy - 17 + y);
		SurfFill(cx + span - 1, cy - 18 + y, cx + span + 1, cy - 17 + y);
	}
	SurfFill(cx - half, cy + 17, cx + half + 1, cy + 19);
	// Three silver claw marks across the body.
	WorldNoticeThickLine(cx - 8, cy - 2, cx + 10, cy + 8, 3, 236, 228, 214, alpha);
	WorldNoticeThickLine(cx - 10, cy + 3, cx + 8, cy + 13, 3, 236, 228, 214, alpha);
	WorldNoticeThickLine(cx - 6, cy - 7, cx + 11, cy + 3, 3, 236, 228, 214, alpha);
	WorldNoticeThickLine(cx - 7, cy - 2, cx + 9, cy + 8, 1, 255, 80, 70, alpha);
	// Bang at the apex.
	SurfColor(255, 236, 220, alpha);
	SurfFill(cx - 1, cy - 14, cx + 2, cy - 6);
	SurfFill(cx - 1, cy - 4, cx + 2, cy - 2);
	if (g_teamWorldFont) {
		const wchar_t* text = UiText(UiId::Text_284d8661, L"惊扰女巫");
		const int tw = SurfTextWFont(g_teamWorldFont, text);
		SurfTextAt(g_teamWorldFont, cx - tw / 2, cy + 24, RGB(255, 92, 72), text);
	}
}

// Team black-and-white badge: cracked heart + ECG trace + pointer triangle, drawn the way
// the other world notices are (procedural lines and rect spans, no texture). The heart body
// is filled scanline by scanline with the even-odd rule so the notch between the two lobes
// stays open, then split by a zigzag crack and crossed by the red trace.
static int s_bwHeartN = 0;
static float s_bwHeartX[32];
static float s_bwHeartY[32];  // unit heart, x/y in -1..1, y up

static void WorldNoticeBwHeartUnit() {
	if (s_bwHeartN) return;
	// Normalize to the curve's own bounds (y spans -0.971..0.681), so the heart fills the
	// box instead of leaving a third of it empty above the lobes.
	const float yMid = -0.145f, yHalf = 0.826f;
	const int n = 28;
	for (int i = 0; i < n; ++i) {
		const float t = (float)i * 6.2831853f / (float)n;
		const float st = sinf(t), ct = cosf(t);
		const float y = (13.f * ct - 5.f * cosf(2.f * t) - 2.f * cosf(3.f * t)
			- cosf(4.f * t)) / 17.5f;
		s_bwHeartX[i] = 16.f * st * st * st / 16.5f;
		s_bwHeartY[i] = (y - yMid) / yHalf;
	}
	s_bwHeartN = n;
}

void WorldNoticeDrawBlackWhite(int cx, int cy, int alpha, int scalePct) {
	// Allow up to 400 so the screen self-badge can be 2× the menu scale (100%→200% visual).
	int sc = scalePct > 0 ? scalePct : 140;
	if (sc < 50) sc = 50;
	if (sc > 400) sc = 400;
	auto p = [sc](int v) {
		if (v == 0) return 0;
		const int autoPct = g_hudResolutionScalePct;
		int n = ((v < 0 ? -v : v) * sc * autoPct + 5000) / 10000;
		if (n < 1) n = 1;
		return v < 0 ? -n : n;
	};
	WorldNoticeBwHeartUnit();
	const int halfW = p(10);
	const int halfH = p(11);
	const int a = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);

	// No panel behind the badge: like the other world notices it stays transparent, and the
	// dark silhouette stroke below is what keeps it readable over bright geometry.
	// Silhouette first, so the filled body keeps a clean dark edge all the way round.
	for (int i = 0; i < s_bwHeartN; ++i) {
		const int j = (i + 1) % s_bwHeartN;
		WorldNoticeThickLine(
			cx + (int)(s_bwHeartX[i] * (float)halfW), cy - (int)(s_bwHeartY[i] * (float)halfH),
			cx + (int)(s_bwHeartX[j] * (float)halfW), cy - (int)(s_bwHeartY[j] * (float)halfH),
			p(1) + 1, 16, 18, 16, a * 3 / 4);
	}

	// Heart body, even-odd spans (two spans on the top rows = the notch).
	float xs[8];
	for (int r = 0; r < halfH * 2; ++r) {
		const int yy = cy - halfH + r;
		const float fy = (float)(cy - yy) / (float)halfH;  // y up: -1 top, +1 bottom
		int nx = 0;
		for (int i = 0; i < s_bwHeartN; ++i) {
			const int j = (i + 1) % s_bwHeartN;
			const float y0 = s_bwHeartY[i], y1 = s_bwHeartY[j];
			if ((y0 <= fy && y1 <= fy) || (y0 > fy && y1 > fy)) continue;
			if (nx >= 8 - 1) break;
			xs[nx++] = s_bwHeartX[i] + (fy - y0) / (y1 - y0) * (s_bwHeartX[j] - s_bwHeartX[i]);
		}
		for (int i = 1; i < nx; ++i) {  // insertion sort, few crossings per row
			const float v = xs[i];
			int k = i - 1;
			while (k >= 0 && xs[k] > v) { xs[k + 1] = xs[k]; --k; }
			xs[k + 1] = v;
		}
		const bool lower = fy < 0.f;
		SurfColor(lower ? 206 : 238, lower ? 210 : 240, lower ? 204 : 234, a);
		for (int i = 0; i + 1 < nx; i += 2) {
			const int xa = cx + (int)(xs[i] * (float)halfW);
			const int xb = cx + (int)(xs[i + 1] * (float)halfW);
			if (xb > xa) SurfFill(xa, yy, xb + 1, yy + 1);
		}
	}

	// Crack: one-pixel bright slit on a two-pixel dark edge, stepping left/right across the
	// heart's full height. Width stays minimal (a heavier stroke tears the 28 px heart);
	// the sideways amplitude is what makes it read as a split.
	for (int i = 0; i < 7; ++i) {
		const int y0 = cy - halfH + p(2) + i * p(3);
		const int y1 = y0 + p(3);
		if (y1 > cy + halfH - p(1)) break;  // never run past the tip
		const int off = (i % 2 == 0) ? -p(2) : p(2);
		WorldNoticeThickLine(cx + off, y0, cx + off, y1, p(1) + 1, 20, 22, 20, a * 3 / 4);
		WorldNoticeThickLine(cx + off, y0, cx + off, y1, p(1) + 1, 252, 252, 248, a);
	}

	// ECG trace across the middle, one dark pixel under it so it reads over the crack
	// without cutting the heart in half.
	const int lx = cx - p(14), my = cy + p(1), st = p(2);
	const int pts[8][2] = {
		{ lx, my }, { lx + st * 3, my }, { lx + st * 4, my - p(3) },
		{ lx + st * 5, my + p(6) }, { lx + st * 6, my - p(5) },
		{ lx + st * 7, my }, { cx + p(14), my }, { cx + p(14), my }
	};
	for (int i = 0; i + 1 < 7; ++i)
		WorldNoticeThickLine(pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1],
			p(1) + 1, 14, 4, 4, a * 3 / 4);
	for (int i = 0; i + 1 < 7; ++i)
		WorldNoticeThickLine(pts[i][0], pts[i][1], pts[i + 1][0], pts[i + 1][1],
			p(1), 255, 52, 46, a);
}

void WorldNoticePaint() {
	if ((!g_optWorldFriendlyFire && !g_optWorldWitchKill && !g_optWorldTankKill
		&& !g_optWorldRescue && !g_optWorldMeleeSkeet && !g_optWorldLevel
		&& !g_optWorldSkeet && !g_optWorldTongueCut && !g_optWorldMeleeCrown
		&& !g_optWorldCrown && !g_optWorldActionProgress && !g_optWorldBileMark
		&& !g_optWorldBlackWhite
		&& !g_optWorldTankSlayer && !g_optWorldWitchStartle) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading() || g_hudPostMissionQuiet) return;
	const DWORD now = GetTickCount();
	WorldNoticeControlTick();
	if (g_worldFfNotice.until && now >= g_worldFfNotice.until) g_worldFfNotice = {};
	if (g_worldWitchNotice.until && now >= g_worldWitchNotice.until) g_worldWitchNotice = {};
	if (g_worldTankNotice.until && now >= g_worldTankNotice.until) g_worldTankNotice = {};
	if (g_worldRescueNotice.until && now >= g_worldRescueNotice.until) g_worldRescueNotice = {};
	if (g_worldMeleeSkeetNotice.until && now >= g_worldMeleeSkeetNotice.until) g_worldMeleeSkeetNotice = {};
	if (g_worldLevelNotice.until && now >= g_worldLevelNotice.until) g_worldLevelNotice = {};
	if (g_worldSkeetNotice.until && now >= g_worldSkeetNotice.until) g_worldSkeetNotice = {};
	if (g_worldTongueCutNotice.until && now >= g_worldTongueCutNotice.until) g_worldTongueCutNotice = {};
	if (g_worldCrownNotice.until && now >= g_worldCrownNotice.until) { g_worldCrownNotice = {}; g_worldCrownKind = 0; }
	if (g_worldTankSlayerNotice.until && now >= g_worldTankSlayerNotice.until) g_worldTankSlayerNotice = {};
	if (g_optWorldWitchStartle) {
		bool anyStartle = false;
		for (int i = 0; i < kTeamHudMax; ++i) {
			if (g_worldWitchStartleNotice[i].until && now >= g_worldWitchStartleNotice[i].until)
				g_worldWitchStartleNotice[i] = {};
			if (g_worldWitchStartleNotice[i].until) anyStartle = true;
		}
		g_worldWitchStartleAny = anyStartle;
	} else if (g_worldWitchStartleAny) {
		memset(g_worldWitchStartleNotice, 0, sizeof(g_worldWitchStartleNotice));
		g_worldWitchStartleAny = false;
	}
	if (!g_worldFfNotice.until && !g_worldWitchNotice.until && !g_worldTankNotice.until
		&& !g_worldRescueNotice.until && !g_worldMeleeSkeetNotice.until && !g_worldLevelNotice.until
		&& !g_worldSkeetNotice.until && !g_worldTongueCutNotice.until && !g_worldCrownNotice.until
		&& !g_worldTankSlayerNotice.until && !g_optWorldActionProgress && !g_optWorldBileMark
		&& !g_optWorldBlackWhite
		&& !g_worldWitchStartleAny) return;
	if (!MenuEnsureSurf()) return;
	if (!g_teamWorldFont)
		g_teamWorldFont = SurfAcquireVguiFont(14, 700, kFontFlagAntiAlias | kFontFlagDropShadow);
	const bool closeKillActive = g_worldMeleeSkeetNotice.until || g_worldLevelNotice.until
		|| g_worldCrownNotice.until;
	if (closeKillActive) {
		const int want = HudScalePx(16, 140);
		if (!g_worldKillFont || g_worldKillFontTall != want) {
			g_worldKillFont = SurfAcquireVguiFont(want, 800,
				kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline);
			g_worldKillFontTall = g_worldKillFont ? want : 0;
		}
	}
	int sw = 0, sh = 0;
	if (!g_teamWorldFont || !SurfGetScreenSize(&sw, &sh)) return;
	WorldNoticeState* notices[4] = { &g_worldFfNotice, &g_worldWitchNotice, &g_worldTankNotice, &g_worldRescueNotice };
	for (int kind = 0; kind < 4; ++kind) {
		WorldNoticeState& n = *notices[kind];
		if (!n.until || now >= n.until) { n = {}; continue; }
		if ((kind == 0 && !g_optWorldFriendlyFire) || (kind == 1 && !g_optWorldWitchKill)
			|| (kind == 2 && !g_optWorldTankKill) || (kind == 3 && !g_optWorldRescue)) {
			n = {};
			continue;
		}
		float sx = 0.f, sy = 0.f;
		if (!WorldNoticeHeadScreen(n.ent, &sx, &sy, sw, sh)) continue;
		const DWORD life = n.until - n.born;
		const DWORD left = n.until - now;
		int alpha = 245;
		if (life && left < 350u) alpha = 80 + (int)(165u * left / 350u);
		const int cx = (int)(sx + 0.5f);
		const int cy = (int)(sy + 0.5f) - (kind == 0 ? 72 : (kind == 3 ? 82 : 104));
		if (kind == 0) WorldNoticeDrawFriendlyFire(cx, cy, alpha);
		else if (kind == 3) WorldNoticeDrawRescue(cx, cy, alpha);
		else WorldNoticeDrawTrophy(cx, cy, alpha, kind == 1 ? L"WITCH" : L"TANK");
	}
	WorldInfectedNoticeState* infected[4] = { &g_worldMeleeSkeetNotice, &g_worldLevelNotice,
		&g_worldSkeetNotice, &g_worldTongueCutNotice };
	const bool enabled[4] = { g_optWorldMeleeSkeet, g_optWorldLevel, g_optWorldSkeet, g_optWorldTongueCut };
	for (int kind = 0; kind < 4; ++kind) {
		WorldInfectedNoticeState& n = *infected[kind];
		if (!enabled[kind]) { n = {}; continue; }
		if (!n.until || now >= n.until) continue;
		float sx = 0.f, sy = 0.f;
		const bool closeKill = kind == 0 || kind == 1;
		if (closeKill) {
			if (!WorldNoticeProjectCloseKill(n.world, &sx, &sy, sw, sh)) continue;
		} else if (!LocalPlayWorldToScreen(n.world, &sx, &sy, sw, sh)) continue;
		// These achievements are earned at point-blank range. Keep them below the
		// upper-crosshair fallback/kill anchor so the badge does not sit too high.
		if (kind == 0)
			sy += (float)WorldNoticeKillPx(52);
		else if (kind == 1)
			sy += (float)WorldNoticeKillPx(46);
		const DWORD left = n.until - now;
		int alpha = left < 350u ? 80 + (int)(165u * left / 350u) : 245;
		const int cx = (int)(sx + 0.5f);
		const int cy = (int)(sy + 0.5f);
		if (kind == 0) WorldNoticeDrawMeleeSkeet(cx, cy, alpha);
		else if (kind == 1) WorldNoticeDrawLevel(cx, cy, alpha);
		else if (kind == 2) WorldNoticeDrawSkeet(cx, cy, alpha);
		else WorldNoticeDrawTongueCut(cx, cy, alpha);
	}
	if (g_worldCrownNotice.until && now < g_worldCrownNotice.until) {
		const bool enabled = (g_worldCrownKind == 1) ? g_optWorldMeleeCrown : g_optWorldCrown;
		if (!enabled) { g_worldCrownNotice = {}; g_worldCrownKind = 0; }
		else {
			float sx = 0.f, sy = 0.f;
			if (WorldNoticeProjectCloseKill(g_worldCrownNotice.world, &sx, &sy, sw, sh)) {
				const DWORD left = g_worldCrownNotice.until - now;
				const int alpha = left < 350u ? 80 + (int)(165u * left / 350u) : 245;
				WorldNoticeDrawCrown((int)(sx + 0.5f), (int)(sy + 0.5f), alpha, g_worldCrownKind);
			}
		}
	}
	if (g_optWorldTankSlayer && g_worldTankSlayerNotice.until && now < g_worldTankSlayerNotice.until) {
		float sx = 0.f, sy = 0.f;
		if (LocalPlayWorldToScreen(g_worldTankSlayerNotice.world, &sx, &sy, sw, sh)) {
			const DWORD left = g_worldTankSlayerNotice.until - now;
			const int alpha = left < 400u ? 70 + (int)(175u * left / 400u) : 245;
			WorldNoticeDrawTankSlayer((int)(sx + 0.5f), (int)(sy + 0.5f), alpha);
		}
	}
	if (g_optWorldActionProgress) {
		for (int i = 0; i < kTeamHudMax; ++i) {
			const TeamHudSlot& s = g_teamHudSlots[i];
			if (!s.present || s.dead || s.actionProgress < 0.f) continue;
			float sx = 0.f, sy = 0.f;
			if (WorldNoticeHeadScreen(s.ent, &sx, &sy, sw, sh))
				WorldNoticeDrawActionProgress((int)(sx + 0.5f), (int)(sy + 0.5f), s.actionProgress, s.actionRemain);
		}
	}
	if (g_optWorldBlackWhite) {
		// State-driven, so the badge lives as long as the state does. Black and white is the
		// second-revive state, i.e. exactly m_bIsOnThirdStrike — the same netprop the team
		// card already renders as its 「危」 label. It sits directly above the teammate's
		// world bar: that cluster's top row is sy-35 (HP text), so the badge bottom stays at
		// sy-38 and it is centred on the bar, whose own centre is sx-7.
		static DWORD s_bwSince[kTeamHudMax]{};
		static DWORD s_bwUntil[kTeamHudMax]{};
		static int s_bwLogs = 0;
		const DWORD holdMs = 260;
		const int halfW = WorldNoticeKillPx(10);
		const int halfH = WorldNoticeKillPx(11);
		for (int i = 0; i < kTeamHudMax; ++i) {
			const TeamHudSlot& s = g_teamHudSlots[i];
			if (s.present && !s.dead && !s.incap && s.third) {
				if (!s_bwSince[i]) {
					s_bwSince[i] = now;
					if (s_bwLogs < 8) {
						++s_bwLogs;
						Log("world-bw: ent=%d hp=%d temp=%d incap=%d", s.ent, s.hp, s.tempHp, s.incap ? 1 : 0);
					}
				}
				s_bwUntil[i] = now + holdMs;
			} else if (!s_bwUntil[i] || now > s_bwUntil[i]) {
				s_bwSince[i] = 0;
				s_bwUntil[i] = 0;
				continue;
			}
			if (s.isLocal) continue;
			float sx = 0.f, sy = 0.f;
			if (!WorldNoticeHeadScreen(s.ent, &sx, &sy, sw, sh)) continue;
			const DWORD age = now - s_bwSince[i];
			const DWORD left = (s_bwUntil[i] > now) ? (s_bwUntil[i] - now) : 0u;
			int alpha = 240;
			if (age < 180u) alpha = 40 + (int)(200u * age / 180u);
			else if (left < 220u) alpha = 40 + (int)(200u * left / 220u);
			const int cx = (int)(sx + 0.5f) - 7;
			const int cy = (int)(sy + 0.5f) - 53;
			if (cx + halfW + 4 < 0 || cx - halfW - 4 > sw
				|| cy + halfH + 4 < 0 || cy - halfH - 4 > sh) continue;
			WorldNoticeDrawBlackWhite(cx, cy, alpha, 140);
		}
	}
	if (g_optWorldBileMark) {
		bool anyBile = false;
		for (int i = 0; i < kTeamHudMax; ++i) {
			WorldBileNoticeState& bile = g_worldBileNotices[i];
			if (!bile.ent) continue;
			if (!WorldNoticeSurvivorEnt(bile.ent)) { bile = {}; continue; }
			anyBile = true;
			float sx = 0.f, sy = 0.f;
			if (!WorldNoticeHeadScreen(bile.ent, &sx, &sy, sw, sh)) continue;
			const DWORD age = now - bile.born;
			const int pulse = 210 + (int)((age / 90u) % 2u) * 35;
			WorldNoticeDrawBile((int)(sx + 0.5f), (int)(sy + 0.5f) - 88, pulse);
		}
		g_worldBileAnyActive = anyBile;
	}
	if (g_optWorldWitchStartle && g_worldWitchStartleAny) {
		bool anyStartle = false;
		for (int i = 0; i < kTeamHudMax; ++i) {
			WorldNoticeState& n = g_worldWitchStartleNotice[i];
			if (!n.until || now >= n.until) { n = {}; continue; }
			if (!WorldNoticeSurvivorEnt(n.ent)) { n = {}; continue; }
			anyStartle = true;
			float sx = 0.f, sy = 0.f;
			if (!WorldNoticeHeadScreen(n.ent, &sx, &sy, sw, sh)) continue;
			const DWORD life = n.until - n.born;
			const DWORD left = n.until - now;
			int alpha = 245;
			if (life && left < 400u) alpha = 70 + (int)(175u * left / 400u);
			WorldNoticeDrawWitchStartle((int)(sx + 0.5f), (int)(sy + 0.5f) - 96, alpha);
		}
		g_worldWitchStartleAny = anyStartle;
	}
}

void TeamHudPaintFaceAt(const TeamHudSlot& s, int layoutSlot, int fx, int fy, int face, bool empty) {
	(void)layoutSlot;
	const int faceCh = empty ? 0 : ((s.character >= 0 && s.character < 8) ? s.character : 0);
	const int idFace = TeamHudBindFace(faceCh, s.incap && !empty);
	if (idFace >= 0) {
		if (empty)
			SurfColor(255, 255, 255, 70);
		else if (s.dead)
			SurfColor(110, 110, 118, 255);
		else if (s.third)
			SurfColor(168, 170, 178, 255);
		else if (s.cap)
			SurfColor(255, 186, 186, 255);
		else
			SurfColor(255, 255, 255, 255);
		SurfDrawTextured(fx, fy, fx + face, fy + face, idFace);
	} else {
		SurfColor(30, 36, 46, 200);
		SurfFill(fx, fy, fx + face, fy + face);
		if (g_teamFontSm)
			SurfTextAt(g_teamFontSm, fx + 4, fy + face / 2 - 8, RGB(200, 200, 200), TeamHudCharName(faceCh));
	}
	if (!empty && s.third && !s.dead) {
		SurfColor(12, 14, 18, 70);
		SurfFill(fx, fy, fx + face, fy + face);
		TeamHudDrawCornerTicks(fx, fy, face, face, RGB(236, 238, 246), 230);
		if (g_teamFontSm) {
			const int bw = TeamHudPx(22);
			const int bh = g_teamFontSmPx > 0 ? g_teamFontSmPx + 2 : 16;
			SurfColor(18, 18, 22, 210);
			SurfFill(fx + 2, fy + 2, fx + 2 + bw, fy + 2 + bh);
			SurfColor(236, 238, 246, 230);
			SurfOutline(fx + 2, fy + 2, fx + 2 + bw, fy + 2 + bh);
			SurfTextAt(g_teamFontSm, fx + 5, fy + 3, RGB(236, 238, 246), UiText(UiId::Text_52abf014, L"危"));
		}
	}
	if (!empty && s.cap && !s.dead) {
		const COLORREF cc = TeamHudCapColor(s.cap);
		SurfColor(GetRValue(cc), GetGValue(cc), GetBValue(cc), 42);
		SurfFill(fx, fy, fx + face, fy + TeamHudPx(4));
		SurfFill(fx, fy + face - TeamHudPx(4), fx + face, fy + face);
		const int mark = TeamHudPx(26);
		TeamHudDrawSiMark(fx + face - mark + 2, fy + face - mark + 2, mark, s.cap);
	}
}

void TeamHudPaintSolidBar(int barX, int barY, int barW, int barH, int hp, int capHp, int tempHp, bool incap, COLORREF hc, int look) {
	if (barW < 4 || barH < 1) return;
	int fill = hp * barW / capHp;
	if (fill > barW) fill = barW;
	if (look == 2)
		SurfColor(255, 255, 255, 16);
	else
		SurfColor(255, 255, 255, 28);
	SurfFill(barX, barY, barX + barW, barY + barH);
	if (look == 1) {
		SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 50);
		SurfOutline(barX, barY, barX + barW, barY + barH);
	}
	SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 230);
	if (fill > 0)
		SurfFill(barX, barY, barX + fill, barY + barH);
	if (look == 1 && fill > 0) {
		SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 80);
		SurfFill(barX, barY - 1, barX + fill, barY);
	}
	if (!incap && tempHp > 0) {
		int tw = tempHp * barW / capHp;
		if (tw > barW - fill) tw = barW - fill;
		if (tw > 0) {
			SurfColor(255, 220, 90, 180);
			SurfFill(barX + fill, barY, barX + fill + tw, barY + barH);
		}
	}
}

void TeamHudPaintPipsVert(int x, int y, int pw, int ph, int n, int hp, int tempHp, int capHp, COLORREF hc) {
	if (n > 10) n = 10;
	if (n < 4) n = 4;
	if (pw < 4 || ph < 8) return;
	if (hp < 0) hp = 0;
	if (tempHp < 0) tempHp = 0;
	if (capHp < 1) capHp = 1;
	const int gap = TeamHudPx(2);
	int cell = (ph - (n - 1) * gap) / n;
	if (cell < 3) cell = 3;
	int solidN = (hp * n + capHp / 2) / capHp;
	if (solidN > n) solidN = n;
	int totalN = ((hp + tempHp) * n + capHp / 2) / capHp;
	if (totalN > n) totalN = n;
	if (totalN < solidN) totalN = solidN;
	for (int i = 0; i < n; ++i) {
		const int cy = y + i * (cell + gap);
		int ch = cell;
		if (cy + ch > y + ph) ch = y + ph - cy;
		if (ch < 1) break;
		SurfColor(255, 255, 255, 22);
		SurfFill(x, cy, x + pw, cy + ch);
		const int rank = n - 1 - i; // bottom = 0
		if (rank < solidN) {
			SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 235);
			SurfFill(x, cy, x + pw, cy + ch);
		} else if (rank < totalN) {
			SurfColor(255, 220, 90, 200);
			SurfFill(x, cy, x + pw, cy + ch);
		}
	}
}

void TeamHudPaintRevive(int barX, int hpY, int barW, float revive) {
	if (revive < 0.f) return;
	float frac = revive;
	if (frac > 1.f) {
		const DWORD t = GetTickCount() % 1100;
		frac = t / 1100.f;
	}
	if (frac < 0.f) frac = 0.f;
	if (frac > 1.f) frac = 1.f;
	const int rvH = TeamHudPx(3);
	const int rvY = hpY - rvH - 1;
	if (barW < 8) return;
	SurfColor(255, 232, 140, 36);
	SurfFill(barX, rvY, barX + barW, rvY + rvH);
	const int rw = (int)(barW * frac + 0.5f);
	if (rw > 0) {
		SurfColor(255, 214, 72, 240);
		SurfFill(barX, rvY, barX + rw, rvY + rvH);
		SurfColor(255, 250, 210, 200);
		SurfFill(barX + rw - 2, rvY, barX + rw, rvY + rvH);
	}
	if (g_teamFontSm) {
		const wchar_t* lab = UiText(UiId::Text_e8f5f3ea, L"救援");
		const int lw = SurfTextWFont(g_teamFontSm, lab);
		SurfTextAt(g_teamFontSm, barX + barW - lw, hpY, RGB(255, 220, 120), lab);
	}
}

void TeamHudPaintGear(const TeamHudSlot& s, int wx, int wy,
	int wepW, int wepH, int item, int rightEdge) {
	if (s.dead || TeamHudLocalIsSpectating()) return;
	// Pack left→right (pri / ammo / sec / items). Wide weapon cells keep native icon AR.
	int priKind = TeamHudIsPrimary(s.wep) ? s.wep : TW_None;
	int secKind = TeamHudIsSide(s.side) ? s.side : TW_None;
	if (!priKind && secKind) {
		priKind = secKind;
		secKind = TW_None;
	}
	if (TeamHudIsNade(priKind) || TeamHudIsKit(priKind) || TeamHudIsPills(priKind))
		priKind = TW_None;
	if (TeamHudIsNade(secKind) || TeamHudIsKit(secKind) || TeamHudIsPills(secKind))
		secKind = TW_None;

	const int gap = TeamHudPx(5);
	int x = wx;

	if (priKind && x + wepW <= rightEdge) {
		if (priKind == TW_Melee)
			TeamHudDrawWepIconLetterbox(x, wy, wepW, wepH, priKind, s.melee);
		else if (TeamHudIsPrimary(priKind))
			TeamHudDrawPrimaryIcon(x, wy, wepW, wepH, priKind);
		else {
			const int clipHint = priKind == TW_Pistol ? (WeaponEntHasDualPistols(s.priWepIdx) ? 30 : 15) : -1;
			TeamHudDrawIconEx(x, wy, wepW, wepH, priKind, nullptr, clipHint, false);
		}
		x += wepW + gap;
		wchar_t ammoBuf[24]{};
		if (TeamHudIsGun(priKind) && s.clip >= 0) {
			if (s.ammo >= 0)
				swprintf_s(ammoBuf, 24, L"%d/%d", s.clip, s.ammo);
			else
				swprintf_s(ammoBuf, 24, L"%d", s.clip);
		}
		if (ammoBuf[0]) {
			unsigned long ammoFont = g_teamFontSm ? g_teamFontSm : g_teamFont;
			const int ammoH = g_teamFontSmPx > 0 ? g_teamFontSmPx : 14;
			const int aw = SurfTextWFont(ammoFont, ammoBuf);
			if (x + aw <= rightEdge) {
				SurfTextAt(ammoFont, x, wy + (wepH - ammoH) / 2, RGB(220, 226, 232), ammoBuf);
				x += aw + gap;
			}
		}
	}
	if (secKind) {
		const int sw = wepW * 3 / 4;
		const int sh = wepH;
		if (x + sw <= rightEdge) {
			if (secKind == TW_Melee)
				TeamHudDrawWepIconLetterbox(x, wy + (wepH - sh) / 2, sw, sh, secKind, s.melee);
			else {
				const int clipHint = secKind == TW_Pistol ? (WeaponEntHasDualPistols(s.sideWepIdx) ? 30 : 15) : -1;
				TeamHudDrawIconEx(x, wy + (wepH - sh) / 2, sw, sh, secKind, nullptr, clipHint, false);
			}
			x += sw + gap;
		}
	}
	if (s.pills && x + item <= rightEdge) {
		TeamHudDrawIcon(x, wy + (wepH - item) / 2, item, item, s.pills, nullptr);
		x += item + gap;
	}
	if (s.kit && x + item <= rightEdge) {
		TeamHudDrawIcon(x, wy + (wepH - item) / 2, item, item, s.kit, nullptr);
		x += item + gap;
	}
	if (s.nade && x + item <= rightEdge)
		TeamHudDrawIcon(x, wy + (wepH - item) / 2, item, item, s.nade, nullptr);
}

void TeamHudPaintCard(const TeamHudSlot& s, int layoutSlot, int x, int y, int w, int h, bool preview, bool selfPanel) {
	const bool empty = !s.present;
	int st = g_teamHudStyle;
	if (st < 0 || st >= kTeamHudStyleCount) st = 0;

	COLORREF accent = (selfPanel || s.isLocal) ? RGB(255, 196, 92) : RGB(80, 210, 230);
	if (st == 5 && !selfPanel)
		accent = TeamHudCharColor(empty ? 0 : s.character);
	if (selfPanel && !s.dead && !s.cap && !s.incap && !s.hanging && !s.third)
		accent = RGB(255, 214, 96);
	if (s.dead) accent = RGB(140, 140, 148);
	else if (s.cap) accent = TeamHudCapColor(s.cap);
	else if (s.incap || s.hanging) accent = RGB(255, 140, 64);
	else if (s.third) accent = RGB(226, 228, 236);

	int stripeA = empty ? 90 : 220;
	if (s.cap && !empty) {
		const DWORD t = GetTickCount() % 900;
		const DWORD ph = (t > 450) ? (900 - t) : t;
		stripeA = 150 + (int)(ph * 105 / 450);
	}

	const int ar = GetRValue(accent), ag = GetGValue(accent), ab = GetBValue(accent);
	int face = TeamHudPx(64);
	int item = TeamHudPx(26); // near-square for med/throwable
	int wepW = TeamHudPx(56), wepH = TeamHudPx(28); // wide like vanilla weapon icons
	int barH = TeamHudPx(8);
	int pad = TeamHudPx(8);
	int colW = 0;
	int cut = 0;
	int pipN = 0, pipW = 0, pipX = 0;
	int headerH = 0;
	int stripY = y;
	switch (st) {
	case 1:
		face = TeamHudPx(38);
		item = TeamHudPx(20);
		wepW = TeamHudPx(44); wepH = TeamHudPx(20);
		barH = TeamHudPx(3);
		pad = TeamHudPx(4);
		stripY = y + TeamHudPx(8);
		break;
	case 2:
		face = TeamHudPx(28);
		item = TeamHudPx(18);
		wepW = TeamHudPx(40); wepH = TeamHudPx(18);
		barH = TeamHudPx(2);
		pad = TeamHudPx(2);
		break;
	case 3:
		face = TeamHudPx(56);
		item = TeamHudPx(24);
		wepW = TeamHudPx(52); wepH = TeamHudPx(26);
		barH = 0;
		pipN = 8;
		pipW = TeamHudPx(10);
		pad = TeamHudPx(8);
		break;
	case 4:
		face = TeamHudPx(44);
		item = TeamHudPx(24);
		wepW = TeamHudPx(52); wepH = TeamHudPx(26);
		barH = TeamHudPx(6);
		pad = TeamHudPx(10);
		break;
	case 5:
		face = TeamHudPx(56);
		item = TeamHudPx(24);
		wepW = TeamHudPx(52); wepH = TeamHudPx(26);
		barH = TeamHudPx(7);
		pad = TeamHudPx(8);
		colW = face + TeamHudPx(16);
		break;
	case 6:
		face = TeamHudPx(48);
		item = TeamHudPx(22);
		wepW = TeamHudPx(48); wepH = TeamHudPx(24);
		barH = TeamHudPx(6);
		pad = TeamHudPx(10);
		cut = TeamHudPx(14);
		break;
	default:
		headerH = TeamHudPx(28);
		break;
	}
	if (face > h - TeamHudPx(6)) face = h - TeamHudPx(6);
	if (face < 16) face = 16;

	if (st == 2) {
		if (preview) {
			SurfColor(ar, ag, ab, 90);
			SurfOutline(x, y, x + w, y + h);
		}
	} else if (st == 6) {
		SurfColor(6, 14, 20, empty ? 50 : 120);
		TeamHudFillChamfer(x, y, w, h, cut);
		SurfColor(ar, ag, ab, empty ? 70 : 210);
		TeamHudStrokeChamfer(x, y, w, h, cut);
		TeamHudDrawCornerTicks(x, y, w, h, accent, empty ? 80 : 200);
	} else if (st == 5) {
		SurfColor(10, 8, 6, empty ? 40 : 86);
		SurfFill(x + colW, y, x + w, y + h);
		SurfColor(ar, ag, ab, empty ? 70 : stripeA);
		SurfFill(x, y, x + colW, y + h);
		SurfColor(0, 0, 0, 50);
		SurfFill(x, y, x + colW, y + TeamHudPx(3));
		SurfFill(x, y + h - TeamHudPx(3), x + colW, y + h);
	} else if (st == 1) {
		SurfColor(36, 32, 40, empty ? 40 : 150);
		SurfFill(x, stripY, x + w, y + h);
		SurfColor(ar, ag, ab, empty ? 80 : 220);
		SurfFill(x, stripY, x + TeamHudPx(4), y + h);
		SurfColor(255, 255, 255, 28);
		SurfFill(x + TeamHudPx(4), stripY, x + w, stripY + 1);
	} else if (st == 3) {
		SurfColor(8, 12, 10, empty ? 55 : 115);
		SurfFill(x, y, x + w, y + h);
		SurfColor(ar, ag, ab, empty ? 80 : stripeA);
		SurfFill(x, y, x + TeamHudPx(6), y + h);
		SurfColor(ar, ag, ab, 40);
		SurfOutline(x, y, x + w, y + h);
	} else if (st == 4) {
		SurfColor(8, 14, 28, empty ? 50 : 108);
		SurfFill(x, y, x + w, y + h);
		SurfColor(ar, ag, ab, empty ? 90 : 230);
		SurfFill(x, y, x + w, y + TeamHudPx(4));
		SurfColor(ar, ag, ab, stripeA);
		SurfFill(x, y, x + TeamHudPx(5), y + h);
	} else {
		SurfColor(0, 0, 0, empty ? 48 : 78);
		SurfFill(x, y, x + w, y + h);
		SurfColor(12, 16, 22, empty ? 70 : 98);
		SurfFill(x + 1, y + 1, x + w - 1, y + h - 1);
		SurfColor(ar, ag, ab, stripeA);
		SurfFill(x, y, x + TeamHudPx(5), y + h);
		if (headerH > 0) {
			SurfColor(ar, ag, ab, empty ? 18 : 36);
			SurfFill(x + TeamHudPx(5), y, x + w, y + headerH);
			SurfColor(255, 255, 255, 28);
			SurfFill(x + TeamHudPx(5), y, x + w, y + 1);
			SurfColor(ar, ag, ab, 50);
			SurfFill(x + TeamHudPx(5), y + headerH, x + w, y + headerH + 1);
		}
	}
	if (preview && st != 2 && st != 6) {
		SurfColor(ar, ag, ab, 200);
		SurfOutline(x, y, x + w, y + h);
	} else if (empty && !preview && g_menuVisible && !g_menuParked && HudMenuPage(15)) {
		SurfColor(ar, ag, ab, 110);
		SurfOutline(x, y, x + w, y + h);
	}

	int fx = x + pad;
	if (st == 5) fx = x + (colW - face) / 2;
	else if (st == 1) fx = x + TeamHudPx(8);
	else if (st == 0) fx = x + pad + TeamHudPx(5);
	else if (st == 3 || st == 4) fx = x + pad + TeamHudPx(5);
	int fy = y + (h - face) / 2;
	if (st == 1) fy = y;
	if (st == 5) fy = y + (h - face) / 2;
	TeamHudPaintFaceAt(s, layoutSlot, fx, fy, face, empty);

	if (st == 3) {
		pipX = fx + face + TeamHudPx(6);
	}

	if (!g_teamFont) return;
	const int fontH = g_teamFontPx > 0 ? g_teamFontPx : TeamHudFontPx();
	int tx = fx + face + TeamHudPx(10);
	if (st == 3) tx = pipX + pipW + TeamHudPx(8);
	if (st == 5) tx = x + colW + TeamHudPx(10);

	int nameY = y + TeamHudPx(6);
	int hpY = nameY + fontH + TeamHudPx(2);
	int barX = tx;
	int barY = y + h - TeamHudPx(14);
	int wepY = y + h - wepH - TeamHudPx(8);
	int itemY = wepY;
	if (st == 0) {
		nameY = y + (headerH > fontH ? (headerH - fontH) / 2 : TeamHudPx(4));
		hpY = nameY;
		barY = y + headerH + TeamHudPx(8);
		wepY = y + h - wepH - TeamHudPx(8);
		itemY = wepY;
	} else if (st == 1) {
		nameY = stripY + TeamHudPx(2);
		hpY = nameY;
		barY = y + h - barH - TeamHudPx(4);
		wepY = barY - wepH - TeamHudPx(1);
		if (wepY < nameY + fontH) wepY = nameY + fontH;
		itemY = nameY;
	} else if (st == 2) {
		nameY = y + TeamHudPx(2);
		hpY = nameY;
		barY = y + fontH + TeamHudPx(4);
		wepY = barY + barH + TeamHudPx(3);
		itemY = wepY;
	} else if (st == 3) {
		nameY = y + TeamHudPx(8);
		hpY = nameY + fontH + TeamHudPx(2);
		barX = tx;
		barY = hpY;
		wepY = y + h - wepH - TeamHudPx(8);
		itemY = wepY;
	} else if (st == 4) {
		nameY = y + TeamHudPx(8);
		hpY = fy + (face - (g_teamFontLgPx > 0 ? g_teamFontLgPx : fontH)) / 2;
		if (hpY < nameY + fontH) hpY = nameY + fontH + TeamHudPx(2);
		barY = y + h - barH - TeamHudPx(10);
		wepY = barY - wepH - TeamHudPx(4);
		itemY = wepY;
	} else if (st == 5) {
		nameY = y + TeamHudPx(8);
		hpY = nameY;
		barY = nameY + fontH + TeamHudPx(6);
		wepY = y + h - wepH - TeamHudPx(8);
		itemY = wepY;
	} else if (st == 6) {
		nameY = y + TeamHudPx(8);
		hpY = nameY;
		barY = nameY + fontH + TeamHudPx(4);
		wepY = y + h - wepH - TeamHudPx(10);
		itemY = wepY;
	}

	int barW = (x + w) - barX - TeamHudPx(10);
	if (st == 4) barW = w / 2;
	if (barW < 8) barW = 8;

	const int faceCh = empty ? 0 : ((s.character >= 0 && s.character < 8) ? s.character : 0);
	if (empty) {
		const wchar_t* slotLab = selfPanel ? UiText(UiId::Text_659f4d33, L"自己") : TeamHudPlayerSlotLabel(layoutSlot);
		SurfTextAt(g_teamFont, tx, nameY, RGB(140, 148, 160), slotLab);
		if (g_teamFontSm) {
			int subY = hpY;
			if (hpY <= nameY + 1)
				subY = nameY + fontH + TeamHudPx(2);
			SurfTextAt(g_teamFontSm, tx, subY, RGB(110, 118, 128), UiText(UiId::Text_f6c72650, L"不在局内"));
		}
		if (st == 3 && pipN > 0)
			TeamHudPaintPipsVert(pipX, fy, pipW, face, pipN, 0, 0, 100, RGB(70, 70, 74));
		return;
	}

	COLORREF nameCol = RGB(236, 240, 244);
	if (selfPanel && !s.dead)
		nameCol = RGB(255, 236, 160);
	else if (st == 5 && !s.dead)
		nameCol = TeamHudCharColor(s.character);
	SurfTextAt(g_teamFont, tx, nameY, nameCol, s.name);

	const wchar_t* stLab = nullptr;
	COLORREF stc = RGB(200, 200, 200);
	if (s.dead) { stLab = UiText(UiId::Text_34b1486b, L"死亡"); stc = RGB(180, 180, 186); }
	else if (s.cap) { stLab = UiText(UiId::Text_2a093420, L"被控"); stc = TeamHudCapColor(s.cap); }
	else if (s.hanging) { stLab = UiText(UiId::Text_9bb4cb32, L"挂边"); stc = RGB(255, 170, 80); }
	else if (s.incap) { stLab = UiText(UiId::Text_a51f774c, L"倒地"); stc = RGB(255, 140, 64); }
	else if (s.third) { stLab = UiText(UiId::Text_52abf014, L"危"); stc = RGB(226, 228, 236); }
	if (stLab && g_teamFontSm) {
		const int tw = SurfTextWFont(g_teamFontSm, stLab);
		SurfTextAt(g_teamFontSm, x + w - TeamHudPx(10) - tw, nameY, stc, stLab);
	}

	int capHp = s.incap ? 300 : (s.maxHp > 0 ? s.maxHp : 100);
	if (capHp < 1) capHp = 1;
	int hp = s.hp;
	if (hp < 0) hp = 0;
	if (hp > capHp) hp = capHp;
	COLORREF hc = TeamHudHpCol(s);
	if (st == 5 && !s.dead && !s.incap && hp > 39)
		hc = accent;

	if (st == 3 && pipN > 0)
		TeamHudPaintPipsVert(pipX, fy, pipW, face, pipN, hp, s.incap ? 0 : s.tempHp, capHp, hc);
	else if (barH > 0)
		TeamHudPaintSolidBar(barX, barY, barW, barH, hp, capHp, s.tempHp, s.incap, hc, st == 1 ? 1 : (st == 2 ? 2 : 0));

	if (!s.dead && s.revive >= 0.f)
		TeamHudPaintRevive(barX, (st == 3) ? hpY : barY, barW, s.revive);

	int shownHp = s.hp;
	if (!s.incap && !s.dead && s.tempHp > 0)
		shownHp += s.tempHp;
	if (shownHp < 0) shownHp = 0;
	if (shownHp > 999) shownHp = 999;
	wchar_t hpBuf[16]{};
	swprintf_s(hpBuf, 16, L"%d", shownHp);
	unsigned long hpFont = g_teamFont;
	int hpFontH = fontH;
	if (st == 4 && g_teamFontLg) {
		hpFont = g_teamFontLg;
		hpFontH = g_teamFontLgPx > 0 ? g_teamFontLgPx : fontH;
	}
	int hpX = tx;
	if (st == 0 || st == 1 || st == 2 || st == 5 || st == 6) {
		hpX = tx + SurfTextWFont(g_teamFont, s.name) + TeamHudPx(10);
		if (hpX + TeamHudPx(28) > x + w - TeamHudPx(56))
			hpX = tx;
	}
	if (st == 4) {
		const int nw = SurfTextWFont(hpFont, hpBuf);
		const int padN = TeamHudPx(6);
		SurfColor(0, 0, 0, 140);
		SurfFill(hpX - padN, hpY - 2, hpX + nw + padN + 2, hpY + hpFontH + 2);
		SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 100);
		SurfOutline(hpX - padN, hpY - 2, hpX + nw + padN + 2, hpY + hpFontH + 2);
	}
	SurfTextAt(hpFont, hpX, hpY, hc, hpBuf);

	int itemLeft = tx;
	if (st == 6) itemLeft = tx;
	int wepX = tx;
	if (st == 4)
		wepX = hpX + SurfTextWFont(hpFont, hpBuf) + TeamHudPx(16);
	const int rightEdge = x + w - TeamHudPx(10) - ((st == 6) ? (cut / 2) : 0);
	(void)itemLeft;
	TeamHudPaintGear(s, wepX, wepY, wepW, wepH, item, rightEdge);
}

void TeamHudPaintHud() {
	const bool preview = HudMenuPage(15);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optTeamHud, 15)) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!TeamHudFullReady(preview)) return;
	if (!MenuEnsureSurf()) return;
	TeamHudEnsureFonts();
	if (!g_teamFont) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	int cardW = 368, cardH = 96;
	TeamHudCardWH(&cardW, &cardH);

	// 菜单调位：自己 + 玩家2~9 共 9 个锚点全部显示；仅当前选中项高亮
	const bool layoutEdit = preview;
	const bool selfOnly = (g_teamHudShow == kTeamHudShowSelf);
	const bool moveSelf = (g_teamHudSel == kTeamHudSelSelf);
	const bool showSelf = g_teamHudLocal.present || layoutEdit;
	if (showSelf) {
		int selfScale = g_teamHudScale + 20;
		if (selfScale > kTeamHudScaleMax) selfScale = kTeamHudScaleMax;
		int selfW = 368, selfH = 96;
		TeamHudCardWHAtScale(&selfW, &selfH, selfScale);
		int x = 0, y = 0;
		HudPlaceAbs(sw, sh, selfW, selfH, g_selfHudX, g_selfHudY, &x, &y);
		g_teamHudPxOverride = selfScale;
		TeamHudPaintCard(g_teamHudLocal, kTeamHudSelSelf, x, y, selfW, selfH, layoutEdit && moveSelf, true);
		g_teamHudPxOverride = 0;
	}

	if (selfOnly)
		return;
	int shownOthers = 0;
	for (int i = 0; i < kTeamHudCardMax; ++i) {
		const bool selHere = layoutEdit && g_teamHudSel == i;
		if (!layoutEdit && !g_teamHudSlots[i].present)
			continue;
		if (!layoutEdit) {
			int cap = g_teamHudMaxOthers;
			if (cap < 1) cap = 1;
			if (cap > kTeamHudCardMax) cap = kTeamHudCardMax;
			if (shownOthers >= cap)
				continue;
		}
		int x = 0, y = 0;
		HudPlaceAbs(sw, sh, cardW, cardH, g_teamHudX[i], g_teamHudY[i], &x, &y);
		TeamHudPaintCard(g_teamHudSlots[i], i, x, y, cardW, cardH, selHere, false);
		if (!layoutEdit && g_teamHudSlots[i].present)
			++shownOthers;
	}
}

const wchar_t* TeamHudCapVerb(int cap, bool hanging) {
	switch (cap) {
	case kTeamCapSmoker: return hanging ? UiText(UiId::Text_1ebd0525, L"被 Smoker 吊在舌头上") : UiText(UiId::Text_5e74f3c0, L"被 Smoker 拉走");
	case kTeamCapHunter: return UiText(UiId::Text_5571395a, L"被 Hunter 扑倒");
	case kTeamCapJockey: return UiText(UiId::Text_052ac4b4, L"被 Jockey 骑乘");
	case kTeamCapCharger: return UiText(UiId::Text_c50ace51, L"被 Charger 带走");
	case kTeamCapChargerPummel: return UiText(UiId::Text_484aed0e, L"被 Charger 殴打");
	default: return UiText(UiId::Text_50a638e0, L"被特感控制");
	}
}

void CapHudInvalidateFont() {
	g_capHudFontId = 0;
	g_capHudFontTall = 0;
	g_capHudFontSmId = 0;
	g_capHudFontSmTall = 0;
}

void ReviveHudInvalidateFont() {
	g_revHudFontId = 0;
	g_revHudFontTall = 0;
	g_revHudFontSmId = 0;
	g_revHudFontSmTall = 0;
}

void CapHudEnsureFonts() {
	ClampCapHud();
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	int sm = g_capHudFont * 3 / 4;
	if (sm < 15) sm = 15;
	const int mainWant = HudFontPx(g_capHudFont), smWant = HudFontPx(sm);
	if (!g_capHudFontId || g_capHudFontTall != mainWant) {
		unsigned long f = SurfMakeYahei(mainWant, 800, flags);
		if (f) { g_capHudFontId = f; g_capHudFontTall = mainWant; }
	}
	if (!g_capHudFontSmId || g_capHudFontSmTall != smWant) {
		unsigned long f = SurfMakeYahei(smWant, 650, kFontFlagAntiAlias | kFontFlagDropShadow);
		if (f) { g_capHudFontSmId = f; g_capHudFontSmTall = smWant; }
	}
}

void ReviveHudEnsureFonts() {
	ClampReviveHud();
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	int sm = g_revHudFont * 3 / 4;
	if (sm < 15) sm = 15;
	const int mainWant = HudFontPx(g_revHudFont), smWant = HudFontPx(sm);
	if (!g_revHudFontId || g_revHudFontTall != mainWant) {
		unsigned long f = SurfMakeYahei(mainWant, 800, flags);
		if (f) { g_revHudFontId = f; g_revHudFontTall = mainWant; }
	}
	if (!g_revHudFontSmId || g_revHudFontSmTall != smWant) {
		unsigned long f = SurfMakeYahei(smWant, 650, kFontFlagAntiAlias | kFontFlagDropShadow);
		if (f) { g_revHudFontSmId = f; g_revHudFontSmTall = smWant; }
	}
}

void CapHudPaint() {
	const bool preview = HudMenuPage(23);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optCapHud, 23)) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	CapHudEnsureFonts();
	if (!g_capHudFontId) return;
	TeamHudEnsureFonts();
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const int sc = g_capHudScale;

	struct Row { const TeamHudSlot* s; };
	Row rows[kTeamHudMax + 1]{};
	int n = 0;
	auto pushRow = [&](const TeamHudSlot& sl) {
		if (!sl.present || sl.dead || !sl.cap) return;
		if (n < kTeamHudMax + 1) rows[n++].s = &sl;
	};
	pushRow(g_teamHudLocal);
	for (int i = 0; i < kTeamHudMax; ++i)
		pushRow(g_teamHudSlots[i]);
	const bool fake = (preview || HudLayoutPeer(g_optCapHud, 23)) && n == 0;
	const int shown = fake ? 4 : n;
	if (shown <= 0) return;

	const int padX = HudScalePx(12, sc);
	const int padY = HudScalePx(8, sc);
	const int icon = HudScalePx(36, sc);
	const int h1 = HudScalePx(g_capHudLayoutFont, sc) + 2;
	const int h2 = HudScalePx(HudSmLayoutBase(g_capHudLayoutFont), sc) + 2;
	const int rowH = (icon > h1 + h2 + HudScalePx(4, sc)) ? icon : (h1 + h2 + HudScalePx(4, sc));
	const int innerW = HudScalePx(360, sc);
	const int panelW = padX * 2 + innerW;
	const int rowBoxH = rowH + padY * 2;
	const int panelH = shown * rowBoxH + (shown - 1) * HudScalePx(8, sc);

	int x = 0, y = 0;
	HudFinalizePos(&g_capHudAbs, &g_capHudOffX, &g_capHudOffY, g_capHudAlign, sw, sh, panelW, panelH, &x, &y);
	const int tx = x + padX;
	int ty = y + padY;

	for (int r = 0; r < shown; ++r) {
		int cap = kTeamCapHunter;
		bool hanging = false;
		wchar_t who[40]{};
		wchar_t verb[40]{};
		if (fake) {
			static const int kPreviewCaps[4] = {
				kTeamCapHunter, kTeamCapJockey, kTeamCapSmoker, kTeamCapChargerPummel
			};
			static const wchar_t* kPreviewNames[4] = { L"Ellis", L"Rochelle", L"Nick", L"Coach" };
			cap = kPreviewCaps[r];
			swprintf_s(who, 40, L"%s", kPreviewNames[r]);
			wcsncpy(verb, TeamHudCapVerb(cap, false), 39);
			verb[39] = 0;
			hanging = false;
		} else {
			const TeamHudSlot& s = *rows[r].s;
			cap = s.cap;
			hanging = s.hanging;
			StatusHudNameId(who, 40, s.name, s.userid, false);
			wcsncpy(verb, TeamHudCapVerb(s.cap, s.hanging), 39);
			verb[39] = 0;
		}
		const COLORREF cc = CapHudColor(cap);
		const int rowX = x;
		const int rowY = ty - padY;
		const int rowW = panelW;
		HudPanelShadow(rowX, rowY, rowW, rowBoxH);
		SurfColor(8, 10, 16, 92);
		SurfFill(rowX, rowY, rowX + rowW, rowY + rowBoxH);
		SurfColor(GetRValue(cc), GetGValue(cc), GetBValue(cc), 34);
		SurfFill(rowX + 1, rowY + 1, rowX + rowW - 1, rowY + rowBoxH - 1);
		SurfColor(GetRValue(cc), GetGValue(cc), GetBValue(cc), 215);
		SurfFill(rowX, rowY, rowX + 3, rowY + rowBoxH);
		HudCornerBrackets(rowX, rowY, rowW, rowBoxH, 9, cc, preview ? 190 : 125);
		const int iconX = tx;
		const int iconY = ty + (rowH - icon) / 2;
		TeamHudSlot faceSl{};
		if (fake) {
			faceSl.character = r == 0 ? 1 : 2;
			faceSl.cap = cap;
		} else {
			faceSl = *rows[r].s;
		}
		TeamHudPaintFaceAt(faceSl, 0, iconX, iconY, icon, false);
		const int textX = tx + icon + 10;
		SurfTextAt(g_capHudFontId, textX, ty, RGB(255, 236, 228), who);
		if (g_capHudFontSmId)
			SurfTextAt(g_capHudFontSmId, textX, ty + h1, cc, verb);
		(void)hanging;
		ty += rowBoxH + 8;
	}
}

// Screen HUD: the local player's own interaction progress (revive, heal, refuel, hold-E interactions).
// Same visual language as the world notice bar, but laid out through the standard custom-HUD position,
// scale and font settings, so it can be moved and sized like every other screen HUD. Its own menu page
// draws a preview even without an interaction, which is what makes positioning possible.
static unsigned long g_interactHudFontId = 0;
static int g_interactHudFontPx = 0;

void InteractHudDropFont() {
	g_interactHudFontId = 0;
	g_interactHudFontPx = 0;
}

void InteractHudPaint() {
	const bool preview = HudMenuPage(66);   // its own settings page, kMenuPageInteractHud in proxy
	if (!HudWantsDraw(g_optInteractHud, 66)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh) || sw <= 0 || sh <= 0) return;

	float frac = -1.f, remain = -1.f;
	const int local = EngLocal();
	if (local >= 1) {
		void* e = EntGet(local);
		if (EntReadable(e)) frac = TeamHudReadProgressFrac(e, &remain);
	}
	if (frac < 0.f) {
		if (!preview) return;   // not interacting: nothing to draw
		frac = 0.62f;           // preview while its own page is open, so it can be positioned
		remain = 1.4f;
	}
	if (frac > 1.f) frac = 1.f;

	const int sc = g_interactHudScale;
	const int w = HudScalePx(86, sc);
	const int h = HudScalePx(9, sc);
	const int pad = HudScalePx(3, sc);
	const int fontPx = HudScalePx(g_interactHudFont, sc);
	if (!g_interactHudFontId || g_interactHudFontPx != fontPx) {
		g_interactHudFontPx = fontPx;
		g_interactHudFontId = SurfAcquireVguiFont(fontPx, 700,
			kFontFlagAntiAlias | kFontFlagDropShadow);
	}
	if (!g_interactHudFontId) return;

	const int labelH = fontPx + HudScalePx(4, sc);
	const int panelW = w + HudScalePx(20, sc);
	const int panelH = labelH + HudScalePx(6, sc) + h + pad * 2;
	int x = 0, y = 0;
	HudFinalizePos(&g_optInteractHudAbs, &g_interactHudOffX, &g_interactHudOffY,
		g_optInteractHudAlign, sw, sh, panelW, panelH, &x, &y);
	const int cx = x + panelW / 2;
	const int bx = cx - w / 2;
	const int barY = y + pad + labelH + HudScalePx(6, sc);

	wchar_t text[40]{};
	swprintf_s(text, 40, UiText(UiId::TextInteractHudLabel, L"互动进度  %.1fs"),
		remain > 0.f ? remain : 0.f);
	const int tw = SurfTextWFont(g_interactHudFontId, text);
	SurfTextAt(g_interactHudFontId, cx - tw / 2, y + pad, RGB(118, 235, 248), text);

	SurfColor(3, 10, 15, 205);
	TeamHudFillChamfer(bx - pad, barY - pad, w + pad * 2, h + pad * 2, HudScalePx(7, sc));
	SurfColor(39, 70, 82, 235);
	TeamHudFillChamfer(bx, barY, w, h, HudScalePx(5, sc));
	const int fill = (int)(w * frac + 0.5f);
	if (fill > 0) {
		SurfColor(50, 220, 242, 245);
		SurfFill(bx, barY, bx + fill, barY + h);
		SurfColor(210, 255, 255, 210);
		SurfFill(bx, barY, bx + fill, barY + HudScalePx(2, sc));
	}
	SurfColor(92, 238, 255, 225);
	SurfLine(bx - HudScalePx(7, sc), barY + h / 2, bx - HudScalePx(2, sc), barY + h / 2);
	SurfLine(bx + w + HudScalePx(2, sc), barY + h / 2, bx + w + HudScalePx(7, sc), barY + h / 2);
}

void ReviveHudPaint() {
	const bool preview = HudMenuPage(24);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optReviveHud, 24)) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	ReviveHudEnsureFonts();
	if (!g_revHudFontId) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const int sc = g_revHudScale;

	struct Row { const TeamHudSlot* s; };
	Row rows[kTeamHudMax + 1]{};
	int n = 0;
	auto pushRev = [&](const TeamHudSlot& sl) {
		if (!sl.present || sl.dead || sl.revive < 0.f) return;
		if (n < kTeamHudMax + 1) rows[n++].s = &sl;
	};
	pushRev(g_teamHudLocal);
	for (int i = 0; i < kTeamHudMax; ++i)
		pushRev(g_teamHudSlots[i]);
	const bool fake = (preview || HudLayoutPeer(g_optReviveHud, 24)) && n == 0;
	const int shown = fake ? 1 : n;
	if (shown <= 0) return;

	const int padX = HudScalePx(12, sc);
	const int padY = HudScalePx(8, sc);
	const int h1 = HudScalePx(g_revHudLayoutFont, sc) + 2;
	const int h2 = HudScalePx(HudSmLayoutBase(g_revHudLayoutFont), sc) + 2;
	const int barH = HudScalePx(8, sc);
	const int rowH = h1 + HudScalePx(4, sc) + barH + HudScalePx(4, sc) + h2;
	const int titleH = h2;
	const int innerW = HudScalePx(200, sc);
	const int panelW = padX * 2 + innerW;
	const int panelH = padY * 2 + titleH + HudScalePx(6, sc) + shown * (rowH + HudScalePx(10, sc)) - HudScalePx(10, sc);

	int x = 0, y = 0;
	HudFinalizePos(&g_revHudAbs, &g_revHudOffX, &g_revHudOffY, g_revHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 168, 72);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 10, accent, preview ? 200 : 120);

	const int tx = x + padX;
	int ty = y + padY;
	if (g_revHudFontSmId)
		SurfTextAt(g_revHudFontSmId, tx, ty, RGB(255, 224, 174), UiText(UiId::Text_8048d5e0, L"倒地救援"));
	ty += titleH + 6;

	for (int r = 0; r < shown; ++r) {
		wchar_t who[40]{};
		wchar_t pct[16]{};
		wchar_t by[48]{};
		float frac = 0.62f;
		if (fake) {
			swprintf_s(who, 40, L"Ellis");
			wcsncpy(pct, L"62%", 15);
			pct[15] = 0;
			swprintf_s(by, 48, UiText(UiId::Text_96b37180, L"救援者  Nick"));
		} else {
			const TeamHudSlot& s = *rows[r].s;
			StatusHudNameId(who, 40, s.name, s.userid, false);
			frac = s.revive;
			if (frac > 1.f) frac = 0.f;
			if (frac < 0.f) frac = 0.f;
			swprintf_s(pct, 16, L"%d%%", (int)(frac * 100.f + 0.5f));
			if (s.reviveByName[0] || s.reviveByUserId > 0) {
				wchar_t bn[32]{};
				StatusHudNameId(bn, 32, s.reviveByName, s.reviveByUserId, false);
				swprintf_s(by, 48, UiText(UiId::Text_99afa0ec, L"救援者  %s"), bn);
			} else {
				wcsncpy(by, UiText(UiId::Text_fda6dd7a, L"正在救援"), 47);
			}
		}
		SurfTextAt(g_revHudFontId, tx, ty, RGB(255, 242, 220), who);
		const int pw = SurfTextWFont(g_revHudFontId, pct);
		const int pctX = x + panelW - padX - pw;
		SurfTextAt(g_revHudFontId, pctX, ty, RGB(255, 202, 112), pct);
		ty += h1 + 4;
		const int barW = innerW;
		int fw = (int)(barW * frac + 0.5f);
		if (fw < 0) fw = 0;
		if (fw > barW) fw = barW;
		SurfColor(255, 255, 255, 28);
		SurfFill(tx, ty, tx + barW, ty + barH);
		SurfColor(255, 168, 64, 220);
		if (fw > 0)
			SurfFill(tx, ty, tx + fw, ty + barH);
		if (fw > 0) {
			SurfColor(255, 244, 210, 50);
			SurfFill(tx, ty, tx + fw, ty + 1);
		}
		SurfColor(255, 214, 140, 80);
		SurfOutline(tx, ty, tx + barW, ty + barH);
		ty += barH + 4;
		if (g_revHudFontSmId && by[0])
			SurfTextAt(g_revHudFontSmId, tx, ty, RGB(210, 198, 180), by);
		ty += h2 + 10;
	}
}

void WorldNoticeSetFriendlyFire(int attackerEnt) {
	if (!g_optWorldFriendlyFire || !WorldNoticeSurvivorEnt(attackerEnt)) return;
	const DWORD now = GetTickCount();
	g_worldFfNotice.ent = attackerEnt;
	g_worldFfNotice.born = now;
	g_worldFfNotice.until = now + 2200u;
}

static int TankTargetHudTargetIndex(int rawHandle) {
	const int index = EntIndexFromHandle(rawHandle);
	return index > 0 && index <= 64 ? index : 0;
}

static bool TankTargetHudReadInt(void* ent, int offset, int* out) {
	if (!out || !EntReadable(ent) || offset < 0 || offset > 0x4000) return false;
	int* field = (int*)((uint8_t*)ent + offset);
	if (!PtrCommitted(field)) return false;
	*out = *field;
	return true;
}

static bool TankTargetHudReadByte(void* ent, int offset, uint8_t* out) {
	if (!out || !EntReadable(ent) || offset < 0 || offset > 0x4000) return false;
	uint8_t* field = (uint8_t*)ent + offset;
	if (!PtrCommitted(field)) return false;
	*out = *field;
	return true;
}

static void TankTargetHudClampOffsets(bool absolute, int* x, int* y) {
	if (!x || !y) return;
	if (absolute) {
		if (*x < 0) *x = 0;
		if (*x > 7680) *x = 7680;
		if (*y < 0) *y = 0;
		if (*y > 4320) *y = 4320;
		return;
	}
	if (*x < -400) *x = -400;
	if (*x > 600) *x = 600;
	if (*y < -400) *y = -400;
	if (*y > 600) *y = 600;
}

void TankTargetHudClamp() {
	if (g_tankTargetHudAlign < 0 || g_tankTargetHudAlign > 5) g_tankTargetHudAlign = 4;
	g_tankTargetHudScale = HudClampScale(g_tankTargetHudScale);
	HudClampFont(&g_tankTargetHudFont);
	TankTargetHudClampOffsets(g_tankTargetHudAbs, &g_tankTargetHudOffX, &g_tankTargetHudOffY);
}

void TankTargetHudInvalidateFont() {
	g_tankTargetHudFontId = 0;
	g_tankTargetHudFontPx = 0;
}

static void TankTargetHudSample(DWORD now) {
	if (!g_optTankTargetHud || !g_engine || !EngInGame() || EngDrawingLoading()) {
		g_tankTargetHudRowCount = 0;
		memset(g_tankTargetHudRows, 0, sizeof(g_tankTargetHudRows));
		g_tankTargetHudNextSampleAt = 0;
		return;
	}
	if (g_tankTargetHudNextSampleAt && (LONG)(now - g_tankTargetHudNextSampleAt) < 0) return;
	g_tankTargetHudNextSampleAt = now + 250u;
	if (g_offTeam < 0 || g_offZombieClass < 0 || g_offTankLookAtPlayer < 0) {
		g_tankTargetHudRowCount = 0;
		return;
	}

	TankTargetHudRow next[kTankTargetHudMaxRows]{};
	int count = 0;
	// Player entities are bounded to slots 1..32. This scan runs at most 4 Hz
	// and only while this HUD is enabled; menu preview uses a fixed sample row.
	for (int tankEnt = 1; tankEnt <= 32 && count < kTankTargetHudMaxRows; ++tankEnt) {
		void* tank = EntGet(tankEnt);
		int team = 0, zombieClass = 0;
		if (!TankTargetHudReadInt(tank, g_offTeam, &team) || team != 3
			|| !TankTargetHudReadInt(tank, g_offZombieClass, &zombieClass) || zombieClass != 8)
			continue;
		uint8_t lifeState = 0;
		if (!TankTargetHudReadByte(tank, g_offLifeState, &lifeState) || lifeState != 0) continue;
		int rawHandle = 0;
		if (!TankTargetHudReadInt(tank, g_offTankLookAtPlayer, &rawHandle)) continue;
		const int targetEnt = TankTargetHudTargetIndex(rawHandle);
		if (!targetEnt) continue;
		void* target = EntGet(targetEnt);
		int targetTeam = 0;
		if (!TankTargetHudReadInt(target, g_offTeam, &targetTeam) || targetTeam != 2) continue;
		uint8_t targetLifeState = 0;
		if (!TankTargetHudReadByte(target, g_offLifeState, &targetLifeState)
			|| targetLifeState != 0) continue;
		PlayerInfo pi{};
		if (!EngPlayerInfo(targetEnt, &pi) || pi.userid <= 0) continue;

		TankTargetHudRow& row = next[count];
		row.localTarget = targetEnt == EngLocal();
		if (pi.name[0])
			MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, row.targetName,
				(int)(sizeof(row.targetName) / sizeof(row.targetName[0])));
		if (!row.targetName[0])
			wcsncpy_s(row.targetName, UiText(UiId::TankTargetHudFallbackName, L"队友"), _TRUNCATE);
		++count;
	}
	memcpy(g_tankTargetHudRows, next, sizeof(next));
	g_tankTargetHudRowCount = count;
}

void TankTargetHudPaint() {
	// Demo row for own page AND peer layout preview (see HudLayoutSubPage rules).
	const bool preview = HudMenuPage(kMenuPageTankTargetHud)
		|| HudLayoutPeer(g_optTankTargetHud, kMenuPageTankTargetHud);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optTankTargetHud, kMenuPageTankTargetHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		g_tankTargetHudRowCount = 0;
		return;
	}
	if (!MenuEnsureSurf()) return;
	TankTargetHudClamp();
	const int sc = g_tankTargetHudScale;
	const int fontPx = HudScalePx(g_tankTargetHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_tankTargetHudFontId,
		&g_tankTargetHudFontPx, 800);
	if (!font) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	TankTargetHudRow previewRow{};
	const TankTargetHudRow* rows = g_tankTargetHudRows;
	int count = 0;
	if (preview) {
		wcsncpy_s(previewRow.targetName, UiText(UiId::TankTargetHudPreviewName, L"Nick"), _TRUNCATE);
		rows = &previewRow;
		count = 1;
	} else {
		TankTargetHudSample(GetTickCount());
		count = g_tankTargetHudRowCount;
	}
	if (count <= 0) return;

	const int padX = HudScalePx(12, sc);
	const int padY = HudScalePx(8, sc);
	const int textW = HudScalePx(270, sc);
	const int titleH = fontPx + HudScalePx(5, sc);
	const int rowH = fontPx + HudScalePx(6, sc);
	const int panelW = padX * 2 + textW;
	const int panelH = padY * 2 + titleH + HudScalePx(4, sc) + count * rowH;
	int x = 0, y = 0;
	HudFinalizePos(&g_tankTargetHudAbs, &g_tankTargetHudOffX, &g_tankTargetHudOffY,
		g_tankTargetHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 158, 72);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 9, accent, preview ? 200 : 125);
	const int tx = x + padX;
	int ty = y + padY;
	SurfTextAt(font, tx, ty, RGB(255, 207, 151),
		UiText(UiId::TankTargetHudTitle, L"Tank 仇恨目标"));
	ty += titleH + HudScalePx(4, sc);
	for (int i = 0; i < count; ++i) {
		wchar_t name[24]{};
		wcsncpy_s(name, rows[i].targetName, _TRUNCATE);
		wchar_t line[80]{};
		auto formatLine = [&]() {
			swprintf_s(line, _countof(line), UiText(UiId::TankTargetHudRow, L"Tank  >  %s"), name);
		};
		formatLine();
		while (wcslen(name) > 3 && SurfTextWFont(font, line) > textW - HudScalePx(4, sc)) {
			name[wcslen(name) - 1] = 0;
			formatLine();
		}
		SurfTextAt(font, tx, ty, rows[i].localTarget ? RGB(255, 54, 46) : RGB(242, 244, 248), line);
		ty += rowH;
	}
}

// =============================================================================
// SiAggro / WitchRage / BossDmgBoard / Ignite HUDs (all default off).
// Entity reads and sampling run only from Paint, at most 4 Hz, only while the
// module is on. Events stamp atomics or write a bounded ring; Paint resolves.
// =============================================================================

static bool NewHudReadFloat(void* ent, int offset, float* out) {
	if (!out || !EntReadable(ent) || offset < 0 || offset > 0x4000) return false;
	float* field = (float*)((uint8_t*)ent + offset);
	if (!PtrCommitted(field)) return false;
	*out = *field;
	return true;
}

static bool NewHudReadInt(void* ent, int offset, int* out) {
	if (!out || !EntReadable(ent) || offset < 0 || offset > 0x4000) return false;
	int* field = (int*)((uint8_t*)ent + offset);
	if (!PtrCommitted(field)) return false;
	*out = *field;
	return true;
}

static void NewHudClampLayout(bool abs, int* align, int* offX, int* offY, int* font, int* scale) {
	if (*align < 0 || *align > 5) *align = 4;
	*scale = HudClampScale(*scale);
	HudClampFont(font);
	TankTargetHudClampOffsets(abs, offX, offY);
}

static void NewHudPlayerName(int ent, wchar_t* out, int cap) {
	if (!out || cap <= 0) return;
	out[0] = 0;
	PlayerInfo pi{};
	if (ent > 0 && ent <= 64 && EngPlayerInfo(ent, &pi) && pi.name[0])
		MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, out, cap);
	out[cap - 1] = 0;
}

static float NewHudClamp01(float v) {
	if (!(v > 0.f)) return 0.f;
	return v > 1.f ? 1.f : v;
}

// ---- SiAggro: special infected whose m_lookatPlayer is the local player ------
bool g_siAggroHudAbs = false;
int g_siAggroHudAlign = 4;
int g_siAggroHudOffX = 0;
int g_siAggroHudOffY = 112;
int g_siAggroHudFont = 20;
int g_siAggroHudScale = 100;
static unsigned long g_siAggroHudFontId = 0;
static int g_siAggroHudFontPx = 0;
struct SiAggroHudRow {
	int zombieClass;
	wchar_t label[32];
};
static constexpr int kSiAggroHudMaxRows = 6;
static SiAggroHudRow g_siAggroHudRows[kSiAggroHudMaxRows]{};
static int g_siAggroHudRowCount = 0;
static DWORD g_siAggroHudNextSampleAt = 0;

void SiAggroHudResetPos() {
	g_siAggroHudAlign = 4;
	g_siAggroHudOffX = 0;
	g_siAggroHudOffY = 112;
	g_siAggroHudAbs = false;
}

void SiAggroHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_siAggroHudAbs, &g_siAggroHudOffX, &g_siAggroHudOffY,
		g_siAggroHudAlign, dx, dy);
}

void SiAggroHudClamp() {
	NewHudClampLayout(g_siAggroHudAbs, &g_siAggroHudAlign, &g_siAggroHudOffX,
		&g_siAggroHudOffY, &g_siAggroHudFont, &g_siAggroHudScale);
}

void SiAggroHudClear() {
	memset(g_siAggroHudRows, 0, sizeof(g_siAggroHudRows));
	g_siAggroHudRowCount = 0;
	g_siAggroHudNextSampleAt = 0;
}

void SiAggroHudInvalidateFont() {
	g_siAggroHudFontId = 0;
	g_siAggroHudFontPx = 0;
}

static const wchar_t* SiAggroClassName(int zombieClass) {
	switch (zombieClass) {
	case 1: return UiText(UiId::TabBoardZcSmoker, L"Smoker");
	case 2: return UiText(UiId::TabBoardZcBoomer, L"Boomer");
	case 3: return UiText(UiId::TabBoardZcHunter, L"Hunter");
	case 4: return UiText(UiId::TabBoardZcSpitter, L"Spitter");
	case 5: return UiText(UiId::TabBoardZcJockey, L"Jockey");
	case 6: return UiText(UiId::TabBoardZcCharger, L"Charger");
	}
	return L"SI";
}

static void SiAggroHudSample(DWORD now) {
	if (!g_optSiAggroHud || !g_engine || !EngInGame() || EngDrawingLoading()) {
		SiAggroHudClear();
		return;
	}
	if (g_siAggroHudNextSampleAt && (LONG)(now - g_siAggroHudNextSampleAt) < 0) return;
	g_siAggroHudNextSampleAt = now + 250u;
	g_siAggroHudRowCount = 0;
	if (g_offTeam < 0 || g_offZombieClass < 0 || g_offTankLookAtPlayer < 0
		|| g_offLifeState < 0) return;
	const int local = EngLocal();
	if (local <= 0 || local > 32) return;
	void* me = EntGet(local);
	int myTeam = 0;
	uint8_t myLife = 0;
	if (!TankTargetHudReadInt(me, g_offTeam, &myTeam) || myTeam != 2
		|| !TankTargetHudReadByte(me, g_offLifeState, &myLife) || myLife != 0)
		return;

	SiAggroHudRow next[kSiAggroHudMaxRows]{};
	int count = 0;
	// Player slots 1..32 only, <= 4 Hz, only while this HUD is enabled.
	for (int ent = 1; ent <= 32 && count < kSiAggroHudMaxRows; ++ent) {
		if (ent == local) continue;
		void* si = EntGet(ent);
		int team = 0, zombieClass = 0;
		if (!TankTargetHudReadInt(si, g_offTeam, &team) || team != 3
			|| !TankTargetHudReadInt(si, g_offZombieClass, &zombieClass)
			|| zombieClass < 1 || zombieClass > 6)
			continue;
		uint8_t life = 0;
		if (!TankTargetHudReadByte(si, g_offLifeState, &life) || life != 0) continue;
		int rawHandle = 0;
		if (!TankTargetHudReadInt(si, g_offTankLookAtPlayer, &rawHandle)) continue;
		if (TankTargetHudTargetIndex(rawHandle) != local) continue;
		SiAggroHudRow& row = next[count++];
		row.zombieClass = zombieClass;
		wchar_t name[32]{};
		NewHudPlayerName(ent, name, (int)_countof(name));
		// Bots are named after their class; only show a label that adds information.
		if (name[0] && _wcsicmp(name, SiAggroClassName(zombieClass)) != 0)
			wcsncpy_s(row.label, name, _TRUNCATE);
	}
	memcpy(g_siAggroHudRows, next, sizeof(next));
	g_siAggroHudRowCount = count;
}

void SiAggroHudPaint() {
	const bool preview = HudMenuPage(kMenuPageSiAggroHud)
		|| HudLayoutPeer(g_optSiAggroHud, kMenuPageSiAggroHud);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optSiAggroHud, kMenuPageSiAggroHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		SiAggroHudClear();
		return;
	}
	if (!MenuEnsureSurf()) return;
	SiAggroHudClamp();
	const int sc = g_siAggroHudScale;
	const int fontPx = HudScalePx(g_siAggroHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_siAggroHudFontId,
		&g_siAggroHudFontPx, 800);
	if (!font) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	SiAggroHudRow previewRow{};
	const SiAggroHudRow* rows = g_siAggroHudRows;
	int count = 0;
	if (preview) {
		previewRow.zombieClass = 3;
		wcsncpy_s(previewRow.label, UiText(UiId::SiAggroHudPreviewLabel, L"Nick"), _TRUNCATE);
		rows = &previewRow;
		count = 1;
	} else {
		SiAggroHudSample(GetTickCount());
		count = g_siAggroHudRowCount;
	}
	if (count <= 0) return;

	const int padX = HudScalePx(12, sc);
	const int padY = HudScalePx(8, sc);
	const int textW = HudScalePx(250, sc);
	const int titleH = fontPx + HudScalePx(5, sc);
	const int rowH = fontPx + HudScalePx(6, sc);
	const int panelW = padX * 2 + textW;
	const int panelH = padY * 2 + titleH + HudScalePx(4, sc) + count * rowH;
	int x = 0, y = 0;
	HudFinalizePos(&g_siAggroHudAbs, &g_siAggroHudOffX, &g_siAggroHudOffY,
		g_siAggroHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 64, 56);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 9, accent, preview ? 200 : 170);
	const int tx = x + padX;
	int ty = y + padY;
	SurfTextAt(font, tx, ty, RGB(255, 150, 140),
		UiText(UiId::SiAggroHudTitle, L"特感仇恨"));
	ty += titleH + HudScalePx(4, sc);
	for (int i = 0; i < count; ++i) {
		wchar_t label[32]{};
		wcsncpy_s(label, rows[i].label, _TRUNCATE);
		wchar_t line[96]{};
		auto formatLine = [&]() {
			if (label[0])
				swprintf_s(line, _countof(line), UiText(UiId::SiAggroHudRowLabel, L"%s  (%s)"),
					SiAggroClassName(rows[i].zombieClass), label);
			else
				wcsncpy_s(line, SiAggroClassName(rows[i].zombieClass), _TRUNCATE);
		};
		formatLine();
		while (label[0] && wcslen(label) > 3 && SurfTextWFont(font, line) > textW - HudScalePx(4, sc)) {
			label[wcslen(label) - 1] = 0;
			formatLine();
		}
		SurfTextAt(font, tx, ty, RGB(255, 96, 86), line);
		ty += rowH;
	}
}

// ---- WitchRage: Witch m_rage (startle) with optional m_wanderRage bar ---------
bool g_witchRageHudAbs = false;
int g_witchRageHudAlign = 4;
int g_witchRageHudOffX = 0;
int g_witchRageHudOffY = 196;
int g_witchRageHudFont = 18;
int g_witchRageHudScale = 100;
bool g_witchRageHudWander = true;
static unsigned long g_witchRageHudFontId = 0;
static int g_witchRageHudFontPx = 0;
struct WitchRageTrack {
	int ent;          // 0 = free slot
	float rage;
	float wander;
	DWORD lastActiveAt; // 0 = never active; hidden ~2 s after the last activity
};
static constexpr int kWitchRageMaxTrack = 8;
static constexpr int kWitchRageMaxRows = 3;
static constexpr int kWitchRageScanPerTick = 48;
static constexpr DWORD kWitchRageIdleMs = 2000u;
static WitchRageTrack g_witchRageTrack[kWitchRageMaxTrack]{};
static int g_witchRageScanCursor = 33;
static DWORD g_witchRageNextSampleAt = 0;
static std::atomic<int> g_witchRageHint[4]{};
static std::atomic<unsigned> g_witchRageHintSeq{0};

void WitchRageHudResetPos() {
	g_witchRageHudAlign = 4;
	g_witchRageHudOffX = 0;
	g_witchRageHudOffY = 196;
	g_witchRageHudAbs = false;
}

void WitchRageHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_witchRageHudAbs, &g_witchRageHudOffX, &g_witchRageHudOffY,
		g_witchRageHudAlign, dx, dy);
}

void WitchRageHudClamp() {
	NewHudClampLayout(g_witchRageHudAbs, &g_witchRageHudAlign, &g_witchRageHudOffX,
		&g_witchRageHudOffY, &g_witchRageHudFont, &g_witchRageHudScale);
}

void WitchRageHudClear() {
	memset(g_witchRageTrack, 0, sizeof(g_witchRageTrack));
	g_witchRageScanCursor = 33;
	g_witchRageNextSampleAt = 0;
	for (auto& hint : g_witchRageHint) hint.store(0, std::memory_order_relaxed);
}

void WitchRageHudInvalidateFont() {
	g_witchRageHudFontId = 0;
	g_witchRageHudFontPx = 0;
}

// witch_spawn: remember the entity so Paint does not wait for the slow scan.
void WitchRageHudNoteSpawn(int ent) {
	if (!g_optWitchRageHud || ent <= 32 || ent >= 2048) return;
	const unsigned i = g_witchRageHintSeq.fetch_add(1, std::memory_order_relaxed) & 3u;
	g_witchRageHint[i].store(ent, std::memory_order_relaxed);
}

static bool WitchRageIsWitchClass(int ent) {
	const char* cn = EntNetClassName(ent);
	return cn && (!_stricmp(cn, "Witch") || !_stricmp(cn, "C_Witch"));
}

static void WitchRageTrackAdd(int ent, DWORD now) {
	WitchRageTrack* freeSlot = nullptr;
	for (WitchRageTrack& t : g_witchRageTrack) {
		if (t.ent == ent) return;
		if (!t.ent && !freeSlot) freeSlot = &t;
	}
	if (!freeSlot) return;
	float rage = 0.f;
	if (!NewHudReadFloat(EntGet(ent), g_offWitchRage, &rage) || !(rage >= 0.f)) return;
	freeSlot->ent = ent;
	freeSlot->rage = NewHudClamp01(rage);
	freeSlot->wander = 0.f;
	freeSlot->lastActiveAt = rage > 0.01f ? now : 0;
}

static void WitchRageHudSample(DWORD now) {
	if (!g_optWitchRageHud || !g_engine || !g_entlist || !EngInGame() || EngDrawingLoading()) {
		WitchRageHudClear();
		return;
	}
	if (g_witchRageNextSampleAt && (LONG)(now - g_witchRageNextSampleAt) < 0) return;
	g_witchRageNextSampleAt = now + 250u;
	if (g_offWitchRage < 0) return;

	// Tracked Witches: at most 8, cheap class + float reads.
	for (WitchRageTrack& t : g_witchRageTrack) {
		if (!t.ent) continue;
		float rage = 0.f;
		if (!WitchRageIsWitchClass(t.ent)
			|| !NewHudReadFloat(EntGet(t.ent), g_offWitchRage, &rage) || !(rage >= 0.f)) {
			t = {};
			continue;
		}
		rage = NewHudClamp01(rage);
		float wander = 0.f;
		if (g_witchRageHudWander && g_offWitchWanderRage >= 0
			&& !NewHudReadFloat(EntGet(t.ent), g_offWitchWanderRage, &wander))
			wander = 0.f;
		if (rage > 0.01f || fabsf(rage - t.rage) > 0.004f) t.lastActiveAt = now;
		t.rage = rage;
		t.wander = NewHudClamp01(wander);
	}

	for (auto& hint : g_witchRageHint) {
		const int ent = hint.exchange(0, std::memory_order_relaxed);
		if (ent > 32 && ent < 2048 && WitchRageIsWitchClass(ent)) WitchRageTrackAdd(ent, now);
	}

	// Rotating discovery scan, <= 48 entities per tick (4 Hz) over non-player slots.
	int hi = EntHighestIndex();
	if (hi > 2047) hi = 2047;
	if (hi >= 33) {
		int n = hi - 32;
		if (n > kWitchRageScanPerTick) n = kWitchRageScanPerTick;
		int cur = g_witchRageScanCursor;
		if (cur < 33 || cur > hi) cur = 33;
		for (int i = 0; i < n; ++i) {
			if (WitchRageIsWitchClass(cur)) WitchRageTrackAdd(cur, now);
			if (++cur > hi) cur = 33;
		}
		g_witchRageScanCursor = cur;
	}
}

void WitchRageHudPaint() {
	const bool preview = HudMenuPage(kMenuPageWitchRageHud)
		|| HudLayoutPeer(g_optWitchRageHud, kMenuPageWitchRageHud);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optWitchRageHud, kMenuPageWitchRageHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		WitchRageHudClear();
		return;
	}
	if (!MenuEnsureSurf()) return;
	WitchRageHudClamp();
	const int sc = g_witchRageHudScale;
	const int fontPx = HudScalePx(g_witchRageHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_witchRageHudFontId,
		&g_witchRageHudFontPx, 800);
	if (!font) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	WitchRageTrack previewRows[2]{};
	const WitchRageTrack* shown[kWitchRageMaxRows]{};
	int count = 0;
	if (preview) {
		previewRows[0].rage = 0.62f;
		previewRows[0].wander = 0.35f;
		previewRows[1].rage = 0.18f;
		previewRows[1].wander = 0.f;
		shown[count++] = &previewRows[0];
		shown[count++] = &previewRows[1];
	} else {
		const DWORD now = GetTickCount();
		WitchRageHudSample(now);
		// Visible rows only; keep the highest rage first (<= 3 rows).
		for (const WitchRageTrack& t : g_witchRageTrack) {
			if (!t.ent || !t.lastActiveAt || (LONG)(now - t.lastActiveAt) >= (LONG)kWitchRageIdleMs)
				continue;
			int at = count < kWitchRageMaxRows ? count : kWitchRageMaxRows - 1;
			if (count >= kWitchRageMaxRows && shown[at]->rage >= t.rage) continue;
			if (count < kWitchRageMaxRows) ++count;
			shown[at] = &t;
			for (; at > 0 && shown[at - 1]->rage < shown[at]->rage; --at) {
				const WitchRageTrack* tmp = shown[at];
				shown[at] = shown[at - 1];
				shown[at - 1] = tmp;
			}
		}
	}
	if (count <= 0) return;

	const bool wander = g_witchRageHudWander && (preview || g_offWitchWanderRage >= 0);
	const int padX = HudScalePx(12, sc);
	const int padY = HudScalePx(8, sc);
	const int textW = HudScalePx(230, sc);
	const int titleH = fontPx + HudScalePx(5, sc);
	const int rowH = fontPx + HudScalePx(4, sc);
	const int barH = HudScalePx(6, sc) > 3 ? HudScalePx(6, sc) : 3;
	const int thinH = HudScalePx(3, sc) > 2 ? HudScalePx(3, sc) : 2;
	const int gap = HudScalePx(6, sc);
	const int blockH = rowH + barH + (wander ? thinH + HudScalePx(2, sc) : 0) + gap;
	const int panelW = padX * 2 + textW;
	const int panelH = padY * 2 + titleH + HudScalePx(4, sc) + count * blockH;
	int x = 0, y = 0;
	HudFinalizePos(&g_witchRageHudAbs, &g_witchRageHudOffX, &g_witchRageHudOffY,
		g_witchRageHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 120, 190);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 9, accent, preview ? 200 : 140);
	const int tx = x + padX;
	int ty = y + padY;
	SurfTextAt(font, tx, ty, RGB(255, 170, 214),
		UiText(UiId::WitchRageHudTitle, L"坐妹惊扰度"));
	ty += titleH + HudScalePx(4, sc);
	for (int i = 0; i < count; ++i) {
		const WitchRageTrack& t = *shown[i];
		wchar_t line[48]{};
		swprintf_s(line, _countof(line), UiText(UiId::WitchRageHudRow, L"Witch  %d%%"),
			(int)(t.rage * 100.f + 0.5f));
		SurfTextAt(font, tx, ty, RGB(242, 244, 248), line);
		ty += rowH;
		SurfColor(255, 255, 255, 38);
		SurfFill(tx, ty, tx + textW, ty + barH);
		const int fillW = (int)(textW * t.rage + 0.5f);
		if (fillW > 0) {
			SurfColor(255, 200 - (int)(t.rage * 140.f), 70, 225);
			SurfFill(tx, ty, tx + fillW, ty + barH);
		}
		ty += barH;
		if (wander) {
			ty += HudScalePx(2, sc);
			SurfColor(255, 255, 255, 26);
			SurfFill(tx, ty, tx + textW, ty + thinH);
			const int wanderW = (int)(textW * t.wander + 0.5f);
			if (wanderW > 0) {
				SurfColor(120, 180, 255, 210);
				SurfFill(tx, ty, tx + wanderW, ty + thinH);
			}
			ty += thinH;
		}
		ty += gap;
	}
}

// ---- WitchHp: screen bar from survivors' m_checkpointDamageToWitch ------------
// Client DT_Witch has no networked m_iHealth (verified). Damage totals on each
// survivor are networked; remaining HP = z_witch_health - delta since fight start.
bool g_witchHpHudAbs = false;
int g_witchHpHudAlign = 4;
int g_witchHpHudOffX = 0;
int g_witchHpHudOffY = 248;
int g_witchHpHudFont = 18;
int g_witchHpHudScale = 100;
static unsigned long g_witchHpHudFontId = 0;
static int g_witchHpHudFontPx = 0;
static constexpr DWORD kWitchHpIdleMs = 2000u;
static constexpr int kWitchHpDefaultMax = 1000;
static DWORD g_witchHpNextSampleAt = 0;
static bool g_witchHpBaseValid = false;
static int g_witchHpBaseSum = 0;
static int g_witchHpLastDmg = 0;
static int g_witchHpRemain = 0;
static int g_witchHpMax = kWitchHpDefaultMax;
static DWORD g_witchHpLastActiveAt = 0;
static std::atomic_bool g_witchHpSpawnPending{false};
static std::atomic_bool g_witchHpKillPending{false};

void WitchHpHudResetPos() {
	g_witchHpHudAlign = 4;
	g_witchHpHudOffX = 0;
	g_witchHpHudOffY = 248;
	g_witchHpHudAbs = false;
}

void WitchHpHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_witchHpHudAbs, &g_witchHpHudOffX, &g_witchHpHudOffY,
		g_witchHpHudAlign, dx, dy);
}

void WitchHpHudClamp() {
	NewHudClampLayout(g_witchHpHudAbs, &g_witchHpHudAlign, &g_witchHpHudOffX,
		&g_witchHpHudOffY, &g_witchHpHudFont, &g_witchHpHudScale);
}

void WitchHpHudClear() {
	g_witchHpNextSampleAt = 0;
	g_witchHpBaseValid = false;
	g_witchHpBaseSum = 0;
	g_witchHpLastDmg = 0;
	g_witchHpRemain = 0;
	g_witchHpMax = kWitchHpDefaultMax;
	g_witchHpLastActiveAt = 0;
	g_witchHpSpawnPending.store(false, std::memory_order_relaxed);
	g_witchHpKillPending.store(false, std::memory_order_relaxed);
}

void WitchHpHudInvalidateFont() {
	g_witchHpHudFontId = 0;
	g_witchHpHudFontPx = 0;
}

void WitchHpHudNoteSpawn(int ent) {
	if (!g_optWitchHpHud) return;
	(void)ent;
	g_witchHpSpawnPending.store(true, std::memory_order_relaxed);
}

void WitchHpHudNoteKilled() {
	if (!g_optWitchHpHud) return;
	g_witchHpKillPending.store(true, std::memory_order_relaxed);
}

static int WitchHpMaxHp() {
	return CvarGetIntRange("z_witch_health", 1, 100000, kWitchHpDefaultMax);
}

static int WitchHpSumCheckpointDmg() {
	if (g_offCheckpointDamageToWitch < 0 || g_offTeam < 0) return -1;
	int sum = 0;
	for (int ent = 1; ent <= 32; ++ent) {
		void* e = EntGet(ent);
		int team = 0;
		if (!TankTargetHudReadInt(e, g_offTeam, &team) || team != 2) continue;
		int dmg = 0;
		if (!TankTargetHudReadInt(e, g_offCheckpointDamageToWitch, &dmg)) continue;
		if (dmg < 0 || dmg > 1000000) continue;
		sum += dmg;
	}
	return sum;
}

static void WitchHpCaptureBaseline() {
	const int sum = WitchHpSumCheckpointDmg();
	if (sum < 0) return;
	g_witchHpBaseSum = sum;
	g_witchHpLastDmg = 0;
	g_witchHpRemain = WitchHpMaxHp();
	g_witchHpMax = g_witchHpRemain;
	g_witchHpBaseValid = true;
	g_witchHpLastActiveAt = 0;
}

static void WitchHpHudSample(DWORD now) {
	if (!g_optWitchHpHud || !g_engine || !g_entlist || !EngInGame() || EngDrawingLoading()) {
		WitchHpHudClear();
		return;
	}
	if (g_witchHpNextSampleAt && (LONG)(now - g_witchHpNextSampleAt) < 0) return;
	g_witchHpNextSampleAt = now + 250u;
	if (g_offCheckpointDamageToWitch < 0) return;

	if (g_witchHpSpawnPending.exchange(false, std::memory_order_relaxed))
		WitchHpCaptureBaseline();
	if (g_witchHpKillPending.exchange(false, std::memory_order_relaxed)) {
		g_witchHpRemain = 0;
		g_witchHpLastDmg = g_witchHpMax;
		g_witchHpLastActiveAt = now;
		g_witchHpBaseValid = false; // next fight re-baselines on spawn or first sample
		return;
	}
	if (!g_witchHpBaseValid) WitchHpCaptureBaseline();
	if (!g_witchHpBaseValid) return;

	const int sum = WitchHpSumCheckpointDmg();
	if (sum < 0) return;
	if (sum < g_witchHpBaseSum) {
		WitchHpCaptureBaseline();
		return;
	}
	const int dmg = sum - g_witchHpBaseSum;
	g_witchHpMax = WitchHpMaxHp();
	int remain = g_witchHpMax - dmg;
	if (remain < 0) remain = 0;
	if (dmg != g_witchHpLastDmg) {
		g_witchHpLastDmg = dmg;
		g_witchHpRemain = remain;
		if (dmg > 0) g_witchHpLastActiveAt = now;
	} else {
		g_witchHpRemain = remain;
	}
}

void WitchHpHudPaint() {
	const bool preview = HudMenuPage(kMenuPageWitchHpHud)
		|| HudLayoutPeer(g_optWitchHpHud, kMenuPageWitchHpHud);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optWitchHpHud, kMenuPageWitchHpHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		WitchHpHudClear();
		return;
	}
	if (!MenuEnsureSurf()) return;
	WitchHpHudClamp();
	const int sc = g_witchHpHudScale;
	const int fontPx = HudScalePx(g_witchHpHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_witchHpHudFontId,
		&g_witchHpHudFontPx, 800);
	if (!font) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	int nowHp = 0, maxHp = kWitchHpDefaultMax;
	bool show = false;
	if (preview) {
		nowHp = 720;
		maxHp = 1000;
		show = true;
	} else {
		const DWORD now = GetTickCount();
		WitchHpHudSample(now);
		if (g_witchHpLastActiveAt && (LONG)(now - g_witchHpLastActiveAt) < (LONG)kWitchHpIdleMs) {
			nowHp = g_witchHpRemain;
			maxHp = g_witchHpMax > 0 ? g_witchHpMax : kWitchHpDefaultMax;
			show = true;
		}
	}
	if (!show) return;
	if (nowHp < 0) nowHp = 0;
	if (nowHp > maxHp) nowHp = maxHp;
	const float ratio = (float)nowHp / (float)(maxHp > 0 ? maxHp : 1);

	const int padX = HudScalePx(12, sc);
	const int padY = HudScalePx(8, sc);
	const int textW = HudScalePx(230, sc);
	const int titleH = fontPx + HudScalePx(5, sc);
	const int rowH = fontPx + HudScalePx(4, sc);
	const int barH = HudScalePx(7, sc) > 4 ? HudScalePx(7, sc) : 4;
	const int gap = HudScalePx(6, sc);
	const int panelW = padX * 2 + textW;
	const int panelH = padY * 2 + titleH + HudScalePx(4, sc) + rowH + barH + gap;
	int x = 0, y = 0;
	HudFinalizePos(&g_witchHpHudAbs, &g_witchHpHudOffX, &g_witchHpHudOffY,
		g_witchHpHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(70, 210, 90);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 9, accent, preview ? 200 : 140);
	const int tx = x + padX;
	int ty = y + padY;
	SurfTextAt(font, tx, ty, RGB(180, 240, 190),
		UiText(UiId::WitchHpHudTitle, L"Witch 血量"));
	ty += titleH + HudScalePx(4, sc);
	wchar_t line[48]{};
	swprintf_s(line, _countof(line), L"Witch  %d/%d", nowHp, maxHp);
	SurfTextAt(font, tx, ty, RGB(255, 245, 220), line);
	ty += rowH;
	int fr = 70, fg = 210, fb = 90;
	if (ratio <= 0.25f) { fr = 230; fg = 55; fb = 55; }
	else if (ratio <= 0.5f) { fr = 240; fg = 190; fb = 50; }
	SurfColor(0, 0, 0, 160);
	SurfFill(tx - 2, ty - 2, tx + textW + 2, ty + barH + 2);
	SurfColor(35, 35, 35, 220);
	SurfFill(tx, ty, tx + textW, ty + barH);
	const int fillW = (int)(textW * ratio + 0.5f);
	if (fillW > 0) {
		SurfColor(fr, fg, fb, 235);
		SurfFill(tx, ty, tx + fillW, ty + barH);
	}
	SurfColor(255, 255, 255, 40);
	SurfFill(tx, ty, tx + textW, ty + 1);
}

// ---- BossDmgBoard: survivors' damage to the Tank/Witch that just died ---------
bool g_bossDmgBoardHudAbs = false;
int g_bossDmgBoardHudAlign = 1;
int g_bossDmgBoardHudOffX = 20;
int g_bossDmgBoardHudOffY = 180;
int g_bossDmgBoardHudFont = 18;
int g_bossDmgBoardHudScale = 100;
static unsigned long g_bossDmgBoardFontId = 0;
static int g_bossDmgBoardFontPx = 0;
struct BossDmgSlot {
	int userid;
	int tank;
	int witch;
};
struct BossDmgRow {
	wchar_t name[32];
	int damage;
	bool local;
};
static constexpr int kBossDmgBoardMaxRows = 8;
static constexpr DWORD kBossDmgBoardShowMs = 10000u;
static constexpr DWORD kBossDmgBoardSettleMs = 300u; // let the last hit's netvar land
static BossDmgSlot g_bossDmgPrev[33]{};
static bool g_bossDmgBaseValid = false;
static DWORD g_bossDmgBaseRetryAt = 0;
static DWORD g_bossDmgBaseNotBefore = 0;
static BossDmgRow g_bossDmgRows[kBossDmgBoardMaxRows]{};
static int g_bossDmgRowCount = 0;
static bool g_bossDmgRowsTank = true;
static DWORD g_bossDmgRowsUntil = 0;
static std::atomic<DWORD> g_bossDmgTankKillAt{0};
static std::atomic<DWORD> g_bossDmgWitchKillAt{0};
static std::atomic<DWORD> g_bossDmgRoundStartAt{0};

void BossDmgBoardResetPos() {
	g_bossDmgBoardHudAlign = 1;
	g_bossDmgBoardHudOffX = 20;
	g_bossDmgBoardHudOffY = 180;
	g_bossDmgBoardHudAbs = false;
}

void BossDmgBoardNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_bossDmgBoardHudAbs, &g_bossDmgBoardHudOffX, &g_bossDmgBoardHudOffY,
		g_bossDmgBoardHudAlign, dx, dy);
}

void BossDmgBoardClamp() {
	NewHudClampLayout(g_bossDmgBoardHudAbs, &g_bossDmgBoardHudAlign, &g_bossDmgBoardHudOffX,
		&g_bossDmgBoardHudOffY, &g_bossDmgBoardHudFont, &g_bossDmgBoardHudScale);
}

void BossDmgBoardClear() {
	memset(g_bossDmgPrev, 0, sizeof(g_bossDmgPrev));
	memset(g_bossDmgRows, 0, sizeof(g_bossDmgRows));
	g_bossDmgBaseValid = false;
	g_bossDmgBaseRetryAt = 0;
	g_bossDmgBaseNotBefore = 0;
	g_bossDmgRowCount = 0;
	g_bossDmgRowsUntil = 0;
	g_bossDmgTankKillAt.store(0, std::memory_order_relaxed);
	g_bossDmgWitchKillAt.store(0, std::memory_order_relaxed);
	g_bossDmgRoundStartAt.store(0, std::memory_order_relaxed);
}

void BossDmgBoardInvalidateFont() {
	g_bossDmgBoardFontId = 0;
	g_bossDmgBoardFontPx = 0;
}

static DWORD BossDmgStamp() {
	const DWORD now = GetTickCount();
	return now ? now : 1u;
}

void BossDmgBoardOnTankKilled() {
	if (g_optBossDmgBoardHud) g_bossDmgTankKillAt.store(BossDmgStamp(), std::memory_order_relaxed);
}

void BossDmgBoardOnWitchKilled() {
	if (g_optBossDmgBoardHud) g_bossDmgWitchKillAt.store(BossDmgStamp(), std::memory_order_relaxed);
}

void BossDmgBoardOnRoundStart() {
	if (g_optBossDmgBoardHud) g_bossDmgRoundStartAt.store(BossDmgStamp(), std::memory_order_relaxed);
}

// Reads one survivor's checkpoint totals. Returns false when the slot is not a
// readable survivor with a valid userid.
static bool BossDmgReadSurvivor(int ent, int* userid, int* tankDmg, int* witchDmg) {
	void* e = EntGet(ent);
	int team = 0;
	if (!TankTargetHudReadInt(e, g_offTeam, &team) || team != 2) return false;
	PlayerInfo pi{};
	if (!EngPlayerInfo(ent, &pi) || pi.userid <= 0) return false;
	int tank = 0, witch = 0;
	if (g_offCheckpointDamageToTank >= 0 && !TankTargetHudReadInt(e, g_offCheckpointDamageToTank, &tank))
		return false;
	if (g_offCheckpointDamageToWitch >= 0 && !TankTargetHudReadInt(e, g_offCheckpointDamageToWitch, &witch))
		return false;
	if (tank < 0 || tank > 1000000) tank = 0;
	if (witch < 0 || witch > 1000000) witch = 0;
	*userid = pi.userid;
	*tankDmg = tank;
	*witchDmg = witch;
	return true;
}

static void BossDmgCaptureBaseline() {
	BossDmgSlot next[33]{};
	for (int ent = 1; ent <= 32; ++ent) {
		int uid = 0, tank = 0, witch = 0;
		if (!BossDmgReadSurvivor(ent, &uid, &tank, &witch)) continue;
		next[ent] = { uid, tank, witch };
	}
	memcpy(g_bossDmgPrev, next, sizeof(next));
	g_bossDmgBaseValid = true;
}

// Deltas since the last baseline for the boss that just died, then prev = current.
static void BossDmgBuildBoard(bool tank, DWORD now) {
	BossDmgRow all[32]{};
	int total = 0;
	const int local = EngLocal();
	for (int ent = 1; ent <= 32; ++ent) {
		int uid = 0, tankDmg = 0, witchDmg = 0;
		if (!BossDmgReadSurvivor(ent, &uid, &tankDmg, &witchDmg)) continue;
		BossDmgSlot& prev = g_bossDmgPrev[ent];
		const bool known = g_bossDmgBaseValid && prev.userid == uid;
		const int cur = tank ? tankDmg : witchDmg;
		const int before = known ? (tank ? prev.tank : prev.witch) : 0;
		const int delta = cur - before;
		if (!known) { // new or replaced slot: track the other boss type from now on
			prev.userid = uid;
			if (tank) prev.witch = witchDmg; else prev.tank = tankDmg;
		}
		if (tank) prev.tank = cur; else prev.witch = cur;
		if (delta <= 0) continue;
		BossDmgRow& row = all[total++];
		NewHudPlayerName(ent, row.name, (int)_countof(row.name));
		if (!row.name[0])
			wcsncpy_s(row.name, UiText(UiId::TankTargetHudFallbackName, L"队友"), _TRUNCATE);
		row.damage = delta;
		row.local = ent == local;
	}
	g_bossDmgBaseValid = true;
	// Top 8 by damage (insertion sort over <= 32 rows).
	for (int i = 1; i < total; ++i) {
		BossDmgRow key = all[i];
		int j = i - 1;
		for (; j >= 0 && all[j].damage < key.damage; --j) all[j + 1] = all[j];
		all[j + 1] = key;
	}
	if (total > kBossDmgBoardMaxRows) total = kBossDmgBoardMaxRows;
	if (total <= 0) return;
	memset(g_bossDmgRows, 0, sizeof(g_bossDmgRows));
	memcpy(g_bossDmgRows, all, sizeof(BossDmgRow) * total);
	g_bossDmgRowCount = total;
	g_bossDmgRowsTank = tank;
	g_bossDmgRowsUntil = now + kBossDmgBoardShowMs;
}

static void BossDmgBoardPump(DWORD now) {
	if (!g_optBossDmgBoardHud || !g_engine || !EngInGame() || EngDrawingLoading()) {
		if (g_bossDmgBaseValid || g_bossDmgRowCount) BossDmgBoardClear();
		return;
	}
	if (g_offTeam < 0 || (g_offCheckpointDamageToTank < 0 && g_offCheckpointDamageToWitch < 0)) return;
	if (g_bossDmgRoundStartAt.exchange(0, std::memory_order_relaxed)) {
		// round_start: totals restart; wait briefly for the new round's entities.
		g_bossDmgBaseValid = false;
		g_bossDmgRowCount = 0;
		g_bossDmgRowsUntil = 0;
		g_bossDmgTankKillAt.store(0, std::memory_order_relaxed);
		g_bossDmgWitchKillAt.store(0, std::memory_order_relaxed);
		g_bossDmgBaseNotBefore = now + 1500u;
	}
	if (!g_bossDmgBaseValid) {
		if (g_bossDmgBaseNotBefore && (LONG)(now - g_bossDmgBaseNotBefore) < 0) return;
		if (g_bossDmgBaseRetryAt && (LONG)(now - g_bossDmgBaseRetryAt) < 0) return;
		g_bossDmgBaseRetryAt = now + 250u;
		if (EngLocal() > 0 && EntReadable(EntGet(EngLocal())))
			BossDmgCaptureBaseline();
		return;
	}
	for (int pass = 0; pass < 2; ++pass) {
		std::atomic<DWORD>& stamp = pass == 0 ? g_bossDmgTankKillAt : g_bossDmgWitchKillAt;
		const DWORD at = stamp.load(std::memory_order_relaxed);
		if (!at) continue;
		const LONG age = (LONG)(now - at);
		if (age < (LONG)kBossDmgBoardSettleMs) continue;
		stamp.store(0, std::memory_order_relaxed);
		if (age > 5000) continue; // Paint was hidden for a while; skip stale kills
		BossDmgBuildBoard(pass == 0, now);
	}
}

void BossDmgBoardPaint() {
	const bool preview = HudMenuPage(kMenuPageBossDmgBoardHud)
		|| HudLayoutPeer(g_optBossDmgBoardHud, kMenuPageBossDmgBoardHud);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optBossDmgBoardHud, kMenuPageBossDmgBoardHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		BossDmgBoardClear();
		return;
	}
	if (!MenuEnsureSurf()) return;
	BossDmgBoardClamp();
	const DWORD now = GetTickCount();
	BossDmgRow previewRows[4]{};
	const BossDmgRow* rows = g_bossDmgRows;
	int count = 0;
	bool tank = true;
	if (preview) {
		static const int kDemoDmg[4] = { 1520, 980, 640, 210 };
		static const wchar_t* const kDemoName[4] = { L"Coach", L"Nick", L"Ellis", L"Rochelle" };
		for (int i = 0; i < 4; ++i) {
			wcsncpy_s(previewRows[i].name, kDemoName[i], _TRUNCATE);
			previewRows[i].damage = kDemoDmg[i];
			previewRows[i].local = i == 1;
		}
		rows = previewRows;
		count = 4;
	} else {
		BossDmgBoardPump(now);
		if (g_bossDmgRowCount > 0 && (LONG)(now - g_bossDmgRowsUntil) >= 0) {
			memset(g_bossDmgRows, 0, sizeof(g_bossDmgRows));
			g_bossDmgRowCount = 0;
		}
		count = g_bossDmgRowCount;
		tank = g_bossDmgRowsTank;
	}
	if (count <= 0) return;

	const int sc = g_bossDmgBoardHudScale;
	const int fontPx = HudScalePx(g_bossDmgBoardHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_bossDmgBoardFontId,
		&g_bossDmgBoardFontPx, 750);
	if (!font) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const int padX = HudScalePx(12, sc), padY = HudScalePx(8, sc);
	const int textW = HudScalePx(280, sc);
	const int titleH = fontPx + HudScalePx(5, sc);
	const int rowH = fontPx + HudScalePx(8, sc);
	const int barH = HudScalePx(2, sc) > 1 ? HudScalePx(2, sc) : 1;
	const int panelW = padX * 2 + textW;
	const int panelH = padY * 2 + titleH + HudScalePx(4, sc) + count * rowH;
	int x = 0, y = 0;
	HudFinalizePos(&g_bossDmgBoardHudAbs, &g_bossDmgBoardHudOffX, &g_bossDmgBoardHudOffY,
		g_bossDmgBoardHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = tank ? RGB(255, 158, 72) : RGB(255, 120, 190);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 9, accent, preview ? 200 : 150);
	const int tx = x + padX;
	int ty = y + padY;
	SurfTextAt(font, tx, ty, tank ? RGB(255, 207, 151) : RGB(255, 170, 214),
		tank ? UiText(UiId::BossDmgBoardTitleTank, L"Tank 伤害榜")
			: UiText(UiId::BossDmgBoardTitleWitch, L"Witch 伤害榜"));
	ty += titleH + HudScalePx(4, sc);
	const int topDmg = rows[0].damage > 0 ? rows[0].damage : 1;
	for (int i = 0; i < count; ++i) {
		wchar_t dmg[16]{};
		swprintf_s(dmg, _countof(dmg), L"%d", rows[i].damage);
		const int dmgW = SurfTextWFont(font, dmg);
		wchar_t name[40]{};
		auto formatName = [&](const wchar_t* src) {
			swprintf_s(name, _countof(name), L"%d. %s", i + 1, src);
		};
		wchar_t shortName[32]{};
		wcsncpy_s(shortName, rows[i].name, _TRUNCATE);
		formatName(shortName);
		const int maxNameW = textW - dmgW - HudScalePx(10, sc);
		while (wcslen(shortName) > 2 && SurfTextWFont(font, name) > maxNameW) {
			shortName[wcslen(shortName) - 1] = 0;
			formatName(shortName);
		}
		const COLORREF col = rows[i].local ? RGB(255, 214, 96) : RGB(242, 244, 248);
		SurfTextAt(font, tx, ty, col, name);
		SurfTextAt(font, tx + textW - dmgW, ty, col, dmg);
		const int barY = ty + fontPx + HudScalePx(3, sc);
		const int barW = (int)((long long)textW * rows[i].damage / topDmg);
		if (barW > 0) {
			SurfColor(GetRValue(accent), GetGValue(accent), GetBValue(accent), rows[i].local ? 200 : 140);
			SurfFill(tx, barY, tx + barW, barY + barH);
		}
		ty += rowH;
	}
}

// ---- IgniteHud: zombie_ignited on a Witch or Tank -----------------------------
bool g_igniteHudAbs = false;
int g_igniteHudAlign = 4;
int g_igniteHudOffX = 0;
int g_igniteHudOffY = 300;
int g_igniteHudFont = 18;
int g_igniteHudScale = 100;
static unsigned long g_igniteHudFontId = 0;
static int g_igniteHudFontPx = 0;
struct IgniteEvt {
	int userid;
	int entityid;
	int kind; // 0 unresolved, 1 Witch, 2 Tank
};
struct IgniteHudRow {
	int entityid;
	int kind;
	bool local;
	DWORD until;
	wchar_t igniter[32];
};
static constexpr int kIgniteHudQueue = 16; // power of two
static constexpr int kIgniteHudMaxRows = 3;
static constexpr DWORD kIgniteHudShowMs = 3000u;
static IgniteEvt g_igniteQ[kIgniteHudQueue]{};
static std::atomic<unsigned> g_igniteHead{0};
static std::atomic<unsigned> g_igniteTail{0};
static IgniteHudRow g_igniteRows[kIgniteHudMaxRows]{};

void IgniteHudResetPos() {
	g_igniteHudAlign = 4;
	g_igniteHudOffX = 0;
	g_igniteHudOffY = 300;
	g_igniteHudAbs = false;
}

void IgniteHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_igniteHudAbs, &g_igniteHudOffX, &g_igniteHudOffY,
		g_igniteHudAlign, dx, dy);
}

void IgniteHudClamp() {
	NewHudClampLayout(g_igniteHudAbs, &g_igniteHudAlign, &g_igniteHudOffX,
		&g_igniteHudOffY, &g_igniteHudFont, &g_igniteHudScale);
}

void IgniteHudClear() {
	memset(g_igniteRows, 0, sizeof(g_igniteRows));
	g_igniteTail.store(g_igniteHead.load(std::memory_order_acquire), std::memory_order_relaxed);
}

void IgniteHudInvalidateFont() {
	g_igniteHudFontId = 0;
	g_igniteHudFontPx = 0;
}

static int IgniteVictimKind(const char* victimName) {
	if (!victimName || !victimName[0]) return 0;
	if (!_stricmp(victimName, "Witch")) return 1;
	if (!_stricmp(victimName, "Tank")) return 2;
	return -1; // some other infected: ignore
}

void IgniteHudOnEvent(void* ev) {
	if (!g_optIgniteHud || !ev) return;
	const int kind = IgniteVictimKind(EvStr(ev, "victimname", ""));
	if (kind < 0) return;
	IgniteEvt rec{};
	rec.userid = EvInt(ev, "userid", 0);
	rec.entityid = EvInt(ev, "entityid", 0);
	rec.kind = kind;
	if (kind == 0 && (rec.entityid <= 0 || rec.entityid >= 2048)) return;
	const unsigned head = g_igniteHead.load(std::memory_order_relaxed);
	const unsigned tail = g_igniteTail.load(std::memory_order_acquire);
	// Unresolved records are low priority; never let them crowd out named ones.
	if (head - tail >= (unsigned)(kind == 0 ? kIgniteHudQueue / 2 : kIgniteHudQueue)) return;
	g_igniteQ[head & (kIgniteHudQueue - 1)] = rec;
	g_igniteHead.store(head + 1, std::memory_order_release);
}

// Paint thread: resolves unresolved victims and igniter names, then updates rows.
static void IgniteHudDrain(DWORD now) {
	unsigned tail = g_igniteTail.load(std::memory_order_relaxed);
	const unsigned head = g_igniteHead.load(std::memory_order_acquire);
	int budget = kIgniteHudQueue;
	while (tail != head && budget-- > 0) {
		const IgniteEvt rec = g_igniteQ[tail & (kIgniteHudQueue - 1)];
		++tail;
		int kind = rec.kind;
		if (kind == 0) {
			const char* cn = EntNetClassName(rec.entityid);
			if (cn && (!_stricmp(cn, "Witch") || !_stricmp(cn, "C_Witch"))) kind = 1;
			else if (rec.entityid <= 32 && g_offTeam >= 0 && g_offZombieClass >= 0) {
				void* e = EntGet(rec.entityid);
				int team = 0, zombieClass = 0;
				if (TankTargetHudReadInt(e, g_offTeam, &team) && team == 3
					&& TankTargetHudReadInt(e, g_offZombieClass, &zombieClass) && zombieClass == 8)
					kind = 2;
			}
			if (kind == 0) continue;
		}
		int slot = -1;
		for (int i = 0; i < kIgniteHudMaxRows; ++i) {
			const IgniteHudRow& r = g_igniteRows[i];
			if (r.until && (LONG)(r.until - now) > 0 && r.entityid == rec.entityid && r.kind == kind) {
				slot = i;
				break;
			}
			if (slot < 0 && (!r.until || (LONG)(r.until - now) <= 0)) slot = i;
		}
		if (slot < 0) {
			slot = 0;
			for (int i = 1; i < kIgniteHudMaxRows; ++i)
				if ((LONG)(g_igniteRows[i].until - g_igniteRows[slot].until) < 0) slot = i;
		}
		IgniteHudRow& row = g_igniteRows[slot];
		row = {};
		row.entityid = rec.entityid;
		row.kind = kind;
		row.until = now + kIgniteHudShowMs;
		const int igniterEnt = rec.userid > 0 ? EngPlayerForUserID(rec.userid) : 0;
		NewHudPlayerName(igniterEnt, row.igniter, (int)_countof(row.igniter));
		row.local = igniterEnt > 0 && igniterEnt == EngLocal();
		if (!row.igniter[0])
			wcsncpy_s(row.igniter, UiText(UiId::IgniteHudUnknownSource, L"火焰"), _TRUNCATE);
	}
	g_igniteTail.store(tail, std::memory_order_relaxed);
}

void IgniteHudPaint() {
	const bool preview = HudMenuPage(kMenuPageIgniteHud)
		|| HudLayoutPeer(g_optIgniteHud, kMenuPageIgniteHud);
	if (!HudWantsDraw(g_optIgniteHud, kMenuPageIgniteHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!preview && (!g_engine || !EngInGame() || EngDrawingLoading())) {
		IgniteHudClear();
		return;
	}
	if (!MenuEnsureSurf()) return;
	IgniteHudClamp();
	const DWORD now = GetTickCount();
	IgniteHudRow previewRow{};
	const IgniteHudRow* rows = g_igniteRows;
	int count = 0;
	if (preview) {
		previewRow.kind = 1;
		previewRow.until = now + 1u;
		wcsncpy_s(previewRow.igniter, UiText(UiId::IgniteHudPreviewName, L"Nick"), _TRUNCATE);
		rows = &previewRow;
		count = 1;
	} else {
		IgniteHudDrain(now);
		for (int i = 0; i < kIgniteHudMaxRows; ++i) {
			if (!g_igniteRows[i].until) continue;
			if ((LONG)(g_igniteRows[i].until - now) <= 0) g_igniteRows[i] = {};
			else ++count;
		}
	}
	if (count <= 0) return;

	const int sc = g_igniteHudScale;
	const int fontPx = HudScalePx(g_igniteHudFont, sc);
	const unsigned long font = HudAcquireLayoutFont(fontPx, &g_igniteHudFontId,
		&g_igniteHudFontPx, 800);
	if (!font) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const int padX = HudScalePx(12, sc), padY = HudScalePx(7, sc);
	const int textW = HudScalePx(260, sc);
	const int rowH = fontPx + HudScalePx(6, sc);
	const int panelW = padX * 2 + textW;
	const int panelH = padY * 2 + count * rowH - HudScalePx(2, sc);
	int x = 0, y = 0;
	HudFinalizePos(&g_igniteHudAbs, &g_igniteHudOffX, &g_igniteHudOffY,
		g_igniteHudAlign, sw, sh, panelW, panelH, &x, &y);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudCornerBrackets(x, y, panelW, panelH, 8, RGB(255, 132, 48), preview ? 210 : 165);
	const int tx = x + padX;
	int ty = y + padY;
	const wchar_t* verb = UiText(UiId::IgniteHudVerb, L"点燃了");
	const COLORREF cName = RGB(255, 255, 255);
	const COLORREF cVerb = RGB(255, 214, 96);
	for (int i = 0; i < (preview ? 1 : kIgniteHudMaxRows); ++i) {
		const IgniteHudRow& row = rows[i];
		if (!row.until) continue;
		wchar_t igniter[32]{};
		wcsncpy_s(igniter, row.igniter, _TRUNCATE);
		const wchar_t* target = row.kind == 2 ? L"Tank" : L"Witch";
		auto lineW = [&]() {
			return SurfTextWFont(font, igniter) + SurfTextWFont(font, verb)
				+ SurfTextWFont(font, target);
		};
		while (wcslen(igniter) > 3 && lineW() > textW - HudScalePx(4, sc))
			igniter[wcslen(igniter) - 1] = 0;
		int cx = tx;
		SurfTextAt(font, cx, ty, cName, igniter);
		cx += SurfTextWFont(font, igniter);
		SurfTextAt(font, cx, ty, cVerb, verb);
		cx += SurfTextWFont(font, verb);
		SurfTextAt(font, cx, ty, cName, target);
		ty += rowH;
	}
}

void AmmoHudEnsureFonts() {
	ClampAmmoHud();
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	int sm = g_ammoHudFont * 11 / 18;
	if (sm < 16) sm = 16;
	const int mainWant = HudFontPx(g_ammoHudFont), smWant = HudFontPx(sm);
	if (!g_ammoFontId || g_ammoFontTall != mainWant) {
		unsigned long f = SurfMakeYahei(mainWant, 800, flags);
		if (f) { g_ammoFontId = f; g_ammoFontTall = mainWant; }
	}
	if (!g_ammoFontSmId || g_ammoFontSmTall != smWant) {
		unsigned long f = SurfMakeYahei(smWant, 650, flags);
		if (f) { g_ammoFontSmId = f; g_ammoFontSmTall = smWant; }
	}
}

void AmmoHudPaintHud() {
	const bool preview = HudMenuPage(21);
	if (!preview && IdleSwapHudQuietActive()) return;
	const bool wantPri = g_optAmmoHud || preview || (HudLayoutSubPage() && g_optAmmoHud);
	// Secondary magazine can show alone — not tied to primary ammo HUD.
	if (!wantPri && !g_optAmmoHudSecClip && !(HudLayoutSubPage() && g_optAmmoHudSecClip)) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	AmmoHudEnsureFonts();
	if (!g_ammoFontId) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const int sc = g_ammoHudScale;

	int clip = -1, ammo = -1, secClip = -1;
	int priWepIdx = 0, secWepIdx = 0;
	const int local = EngLocal();
	if (local > 0) {
		void* me = EntGet(local);
		if (EntReadable(me)) {
			// Inventory topology is comparatively slow-changing. Reuse the bounded
			// Team HUD gear cache instead of resolving all 16 weapon handles and
			// network class names every Paint. Active-weapon changes invalidate the
			// local slot immediately; clip/reserve values remain freshly sampled below.
			static int s_ownerEnt = 0;
			static int s_activeHandle = 0;
			int activeHandle = 0;
			if (g_offActiveWeapon >= 0 && g_offActiveWeapon <= 0x4000) {
				int* ph = (int*)((uint8_t*)me + g_offActiveWeapon);
				if (PtrCommitted(ph)) activeHandle = *ph;
			}
			if (s_ownerEnt != local || s_activeHandle != activeHandle) {
				if (local <= 32)
					g_teamHudGearCache[local].nextReadAt = 0;
				s_ownerEnt = local;
				s_activeHandle = activeHandle;
			}
			TeamHudSlot tmp{};
			if (local <= 32)
				TeamHudReadWepsCached(me, local, &tmp, 100u);
			else
				TeamHudReadWeps(me, &tmp);
			const int activeIdx = activeHandle ? EntIndexFromHandle(activeHandle) : 0;
			// Identity stays cached; refresh only the held gun's numbers each Paint.
			if (wantPri && activeIdx > 0 && activeIdx == tmp.priWepIdx
				&& TeamHudIsPrimary(tmp.wep) && TeamHudIsGun(tmp.wep)) {
				TeamHudReadGunAmmo(me, tmp.priWepIdx, tmp.wep, &tmp.clip, &tmp.ammo);
				priWepIdx = tmp.priWepIdx;
				clip = tmp.clip;
				ammo = tmp.ammo;
			} else if (g_optAmmoHudSecClip && activeIdx > 0 && activeIdx == tmp.sideWepIdx
				&& TeamHudIsGun(tmp.side)) {
				TeamHudReadGunAmmo(me, tmp.sideWepIdx, tmp.side, &tmp.sideClip, nullptr);
				secClip = tmp.sideClip;
				secWepIdx = tmp.sideWepIdx;
			}
		}
	}
	if (preview || HudLayoutPeer(g_optAmmoHud, 21) || (HudLayoutSubPage() && g_optAmmoHudSecClip)) {
		if (wantPri && clip < 0) { clip = 32; ammo = 180; priWepIdx = 1; }
		if (g_optAmmoHudSecClip && secClip < 0) secClip = 15;
	}
	const bool drawPri = wantPri && clip >= 0;
	const bool drawSec = g_optAmmoHudSecClip && secClip >= 0;
	if (!drawPri && !drawSec) return;

	wchar_t clipBuf[8]{};
	wchar_t ammoBuf[12]{};
	wchar_t secBuf[16]{};
	if (drawPri) {
		swprintf_s(clipBuf, 8, L"%d", clip);
		if (ammo >= 0 && !g_optAmmoHudClipOnly)
			swprintf_s(ammoBuf, 12, L"/ %d", ammo);
	}
	if (drawSec)
		swprintf_s(secBuf, 16, drawPri ? L"  %d" : L"%d", secClip);

	const int maxClip = drawPri ? WepClipCapResolve(priWepIdx, clip) : 0;
	COLORREF clipCol = AmmoHudClipColor(drawPri ? clip : -1, maxClip);
	COLORREF ammoCol = (ammo == 0) ? RGB(255, 150, 96) : RGB(168, 186, 198);
	const int secMaxClip = drawSec ? (secWepIdx > 0 ? WepClipCapResolve(secWepIdx, secClip) : 15) : 0;
	const COLORREF secCol = AmmoHudClipColor(secClip, secMaxClip, RGB(140, 190, 220));

	const int clipW = (drawPri && clipBuf[0]) ? SurfTextWFont(g_ammoFontId, clipBuf) : 0;
	const int ammoW = (ammoBuf[0] && g_ammoFontSmId) ? SurfTextWFont(g_ammoFontSmId, ammoBuf) : 0;
	const unsigned long secFont = drawPri ? (g_ammoFontSmId ? g_ammoFontSmId : g_ammoFontId)
		: g_ammoFontId;
	const int secW = (secBuf[0] && secFont) ? SurfTextWFont(secFont, secBuf) : 0;

	const int layoutMainPx = HudScalePx(g_ammoHudLayoutFont, sc);
	const int layoutSmPx = HudScalePx(HudSmLayoutBase(g_ammoHudLayoutFont), sc);
	static unsigned long s_ammoLayoutMainId = 0;
	static int s_ammoLayoutMainPx = 0;
	static unsigned long s_ammoLayoutSmId = 0;
	static int s_ammoLayoutSmPx = 0;
	unsigned long layoutMainFont = HudAcquireLayoutFont(layoutMainPx, &s_ammoLayoutMainId, &s_ammoLayoutMainPx, 800);
	unsigned long layoutSmFont = HudAcquireLayoutFont(layoutSmPx, &s_ammoLayoutSmId, &s_ammoLayoutSmPx, 650);
	const int layoutClipW = (drawPri && clipBuf[0] && layoutMainFont) ? SurfTextWFont(layoutMainFont, clipBuf) : 0;
	const int layoutAmmoW = (ammoBuf[0] && layoutSmFont) ? SurfTextWFont(layoutSmFont, ammoBuf) : 0;
	const unsigned long layoutSecFont = drawPri ? (layoutSmFont ? layoutSmFont : layoutMainFont) : layoutMainFont;
	const int layoutSecW = (secBuf[0] && layoutSecFont) ? SurfTextWFont(layoutSecFont, secBuf) : 0;

	const int gap = HudScalePx(8, sc);
	const int panelW = layoutClipW + (layoutAmmoW ? gap + layoutAmmoW : 0) + layoutSecW;
	const int panelH = layoutMainPx + HudScalePx(8, sc);

	if (!g_ammoHudAbs) {
		g_ammoHudOffX = sw / 2 + 48;
		g_ammoHudOffY = sh / 2 + 36;
		g_ammoHudAbs = true;
	}
	int x = 0, y = 0;
	HudPlaceAbs(sw, sh, panelW > 0 ? panelW : 40, panelH, g_ammoHudOffX, g_ammoHudOffY, &x, &y);

	int cx = x;
	if (drawPri && clipBuf[0]) {
		SurfTextAt(g_ammoFontId, cx, y, clipCol, clipBuf);
		cx += clipW;
		if (ammoBuf[0] && g_ammoFontSmId) {
			int ay = y + (g_ammoFontTall - g_ammoFontSmTall);
			if (ay < y) ay = y;
			SurfTextAt(g_ammoFontSmId, cx + gap, ay, ammoCol, ammoBuf);
			cx += gap + ammoW;
		}
		const int uy = y + (g_ammoFontTall > 0 ? g_ammoFontTall : g_ammoHudFont) + 1;
		const int cr = GetRValue(clipCol), cg = GetGValue(clipCol), cb = GetBValue(clipCol);
		SurfColor(cr, cg, cb, 150);
		SurfFill(x, uy, x + clipW, uy + HudScalePx(2, sc));
	}
	if (drawSec && secBuf[0] && secFont) {
		int ay = y;
		if (drawPri && g_ammoFontSmId && secFont == g_ammoFontSmId) {
			ay = y + (g_ammoFontTall - g_ammoFontSmTall);
			if (ay < y) ay = y;
		}
		SurfTextAt(secFont, cx, ay, secCol, secBuf);
	}
	if (preview) {
		SurfColor(80, 230, 255, 180);
		SurfOutline(x - HudScalePx(6, sc), y - HudScalePx(4, sc), x + panelW + HudScalePx(6, sc), y + panelH + HudScalePx(4, sc));
	}
}

static void UpgradeAmmoFillDisc(int cx, int cy, int r, int rr, int gg, int bb, int aa) {
	if (r < 1) return;
	SurfColor(rr, gg, bb, aa);
	for (int y = -r; y <= r; ++y) {
		int span = 0;
		const int y2 = y * y;
		const int r2 = r * r;
		while (span * span + y2 <= r2)
			++span;
		if (span <= 0) continue;
		SurfFill(cx - (span - 1), cy + y, cx + span, cy + y + 1);
	}
}

// Paint-thread. Primary weapon RecvProp only; no entity-table walk.
static void UpgradeAmmoDrawFlame(int cx, int cy, int sc) {
	const int h = HudScalePx(22, sc);
	const int maxW = HudScalePx(12, sc);
	for (int i = 0; i < h; ++i) {
		const int fromTop = i;
		const int w = 2 + (fromTop * (maxW - 2)) / (h > 1 ? h : 1);
		const int y = cy - h / 2 + i;
		const int a = 230 - fromTop * 4;
		if (fromTop < h * 2 / 5)
			SurfColor(255, 210, 70, a > 40 ? a : 40);
		else if (fromTop < h * 3 / 4)
			SurfColor(255, 140, 40, a > 40 ? a : 40);
		else
			SurfColor(255, 70, 36, a > 40 ? a : 40);
		SurfFill(cx - w / 2, y, cx + (w + 1) / 2, y + 1);
	}
	SurfColor(255, 245, 180, 220);
	SurfFill(cx - HudScalePx(2, sc), cy - HudScalePx(4, sc),
		cx + HudScalePx(2, sc), cy + HudScalePx(6, sc));
}

static void UpgradeAmmoDrawBurst(int cx, int cy, int sc) {
	const int core = HudScalePx(6, sc);
	UpgradeAmmoFillDisc(cx, cy, core + HudScalePx(2, sc), 255, 170, 40, 80);
	UpgradeAmmoFillDisc(cx, cy, core, 255, 220, 80, 230);
	UpgradeAmmoFillDisc(cx, cy, HudScalePx(3, sc), 255, 250, 210, 240);
	const int ray = HudScalePx(10, sc);
	const int t = HudScalePx(2, sc) > 1 ? HudScalePx(2, sc) : 1;
	SurfColor(255, 196, 64, 220);
	SurfFill(cx - ray, cy - t / 2, cx + ray + 1, cy + (t + 1) / 2);
	SurfFill(cx - t / 2, cy - ray, cx + (t + 1) / 2, cy + ray + 1);
	const int d = (ray * 70) / 100;
	SurfFill(cx - d, cy - d, cx - d + t + 1, cy + d + 1);
	SurfFill(cx + d - t, cy - d, cx + d + 1, cy + d + 1);
	SurfFill(cx - d, cy - d, cx + d + 1, cy - d + t + 1);
	SurfFill(cx - d, cy + d - t, cx + d + 1, cy + d + 1);
}

static void UpgradeAmmoDrawLaser(int cx, int cy, int sc) {
	const int bodyW = HudScalePx(7, sc);
	const int bodyH = HudScalePx(10, sc);
	const int beam = HudScalePx(12, sc);
	const int x0 = cx - HudScalePx(8, sc);
	SurfColor(70, 88, 104, 230);
	SurfFill(x0, cy - bodyH / 2, x0 + bodyW, cy + (bodyH + 1) / 2);
	SurfColor(160, 190, 210, 220);
	SurfFill(x0 + 1, cy - bodyH / 2 + 1, x0 + bodyW - 1, cy - bodyH / 2 + HudScalePx(2, sc) + 1);
	const int lens = x0 + bodyW;
	UpgradeAmmoFillDisc(lens, cy, HudScalePx(3, sc), 255, 48, 48, 240);
	UpgradeAmmoFillDisc(lens, cy, HudScalePx(1, sc), 255, 210, 210, 255);
	const int t = HudScalePx(2, sc) > 1 ? HudScalePx(2, sc) : 1;
	SurfColor(255, 56, 56, 200);
	SurfFill(lens, cy - t / 2, lens + beam, cy + (t + 1) / 2);
	SurfColor(255, 110, 110, 90);
	SurfFill(lens, cy - t - 1, lens + beam, cy - t / 2);
	SurfFill(lens, cy + (t + 1) / 2, lens + beam, cy + t + 1);
}


// Independent ammo graphic HUDs (style: off/seg/arc/bar). Not tied to AmmoHud number panel.
static void AmmoGfxHealthRgb(float frac, int* rr, int* gg, int* bb) {
	if (frac < 0.f) frac = 0.f;
	if (frac > 1.f) frac = 1.f;
	if (frac >= 0.55f) {
		const float t = (1.f - frac) / 0.45f;
		*rr = 255; *gg = 255; *bb = (int)(255.f * (1.f - t) + 0.5f);
	} else {
		const float t = frac / 0.55f;
		*rr = 255; *gg = (int)(255.f * t + 0.5f); *bb = 0;
	}
}

// Segment cells: stepped thresholds (same yellowTh/redTh as AmmoHudClipColor), healthy = white.
static void AmmoGfxSegRgb(int clip, int maxClip, int* rr, int* gg, int* bb) {
	if (maxClip <= 0) maxClip = clip > 0 ? clip : 1;
	int yellowTh = (maxClip * 20 + 99) / 100;
	int redTh = (maxClip * 10 + 99) / 100;
	if (yellowTh < 1) yellowTh = 1;
	if (redTh < 1) redTh = 1;
	if (clip <= redTh) { *rr = 255; *gg = 72; *bb = 72; }
	else if (clip <= yellowTh) { *rr = 255; *gg = 196; *bb = 72; }
	else { *rr = 255; *gg = 255; *bb = 255; }
}

static bool AmmoGfxSampleClip(int* outClip, int* outMax) {
	*outClip = -1; *outMax = 0;
	const int local = EngLocal();
	if (local <= 0) return false;
	void* me = EntGet(local);
	if (!EntReadable(me)) return false;
	// Same active-weapon invalidation as AmmoHudPaintHud: gear cache is 100ms,
	// but graphic ammo must follow primary↔side swaps immediately.
	static int s_ownerEnt = 0;
	static int s_activeHandle = 0;
	int activeHandle = 0;
	if (g_offActiveWeapon >= 0 && g_offActiveWeapon <= 0x4000) {
		int* ph = (int*)((uint8_t*)me + g_offActiveWeapon);
		if (PtrCommitted(ph)) activeHandle = *ph;
	}
	if (s_ownerEnt != local || s_activeHandle != activeHandle) {
		if (local <= 32)
			g_teamHudGearCache[local].nextReadAt = 0;
		s_ownerEnt = local;
		s_activeHandle = activeHandle;
	}
	TeamHudSlot tmp{};
	if (local <= 32)
		TeamHudReadWepsCached(me, local, &tmp, 100u);
	else
		TeamHudReadWeps(me, &tmp);

	int activeIdx = 0;
	if (activeHandle)
		activeIdx = EntIndexFromHandle(activeHandle);

	int wepIdx = 0;
	int kind = TW_None;
	// Follow active weapon: primary gun vs pistol/magnum. Melee / throwable → no bar.
	if (activeIdx > 0 && activeIdx == tmp.sideWepIdx && TeamHudIsGun(tmp.side)) {
		wepIdx = tmp.sideWepIdx;
		kind = tmp.side;
	} else if (activeIdx > 0 && activeIdx == tmp.priWepIdx && TeamHudIsGun(tmp.wep)) {
		wepIdx = tmp.priWepIdx;
		kind = tmp.wep;
	} else if (activeIdx <= 0 && TeamHudIsGun(tmp.wep) && tmp.priWepIdx > 0) {
		wepIdx = tmp.priWepIdx;
		kind = tmp.wep;
	} else {
		return false;
	}

	int clip = -1;
	TeamHudReadGunAmmo(me, wepIdx, kind, &clip, nullptr);
	*outClip = clip;
	*outMax = WepClipCapResolve(wepIdx, clip);
	if (*outMax < 1) *outMax = 1;
	return clip >= 0;
}

void AmmoSegHudResetPos() {
	g_ammoSegAbs = false;
	g_ammoSegOffX = 0;
	g_ammoSegOffY = 0;
}

void AmmoSegHudNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_ammoSegAbs, &g_ammoSegOffX, &g_ammoSegOffY, 0, dx, dy);
}

static bool AmmoGfxPaintGate(bool preview) {
	if (!preview && IdleSwapHudQuietActive()) return false;
	if (!g_run.load(std::memory_order_relaxed)) return false;
	if (!SkeetoFeaturesOn()) return false;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return false;
	if (!MenuEnsureSurf()) return false;
	return true;
}

void AmmoBarHudPaint() {
	const bool preview = HudMenuPage(kMenuPageAmmoGfxHud);
	if (!preview && g_ammoGfxStyle != 3) return;
	if (preview && g_ammoGfxStyle != 3) return;
	if (!AmmoGfxPaintGate(preview)) return;
	ClampAmmoGfxHud();

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	const int sc = g_ammoGfxScale;

	int clip = -1, maxClip = 0;
	if (!AmmoGfxSampleClip(&clip, &maxClip)) {
		if (preview) { clip = 18; maxClip = 30; }
		else return;
	}
	float frac = maxClip > 0 ? (float)clip / (float)maxClip : 0.f;
	if (frac < 0.f) frac = 0.f;
	if (frac > 1.f) frac = 1.f;
	int fr = 255, fg = 255, fb = 255;
	AmmoGfxHealthRgb(frac, &fr, &fg, &fb);

	const int cx0 = sw / 2, cy0 = sh / 2;
	const int gap = HudScalePx(34, sc);
	const int thick = HudScalePx(7, sc);
	const int barLen = HudScalePx(52, sc);
	const int pos = g_ammoGfxPos;
	const bool vert = (pos == 2 || pos == 3); // left/right = vertical bar

	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	if (pos == 3) { // right: vertical, deplete top->bottom (fill at bottom)
		x0 = cx0 + gap; y0 = cy0 - barLen / 2;
		x1 = x0 + thick; y1 = y0 + barLen;
	} else if (pos == 2) { // left
		x1 = cx0 - gap; y0 = cy0 - barLen / 2;
		x0 = x1 - thick; y1 = y0 + barLen;
	} else if (pos == 0) { // up: horizontal, deplete left->right (fill at right)
		y1 = cy0 - gap; x0 = cx0 - barLen / 2;
		y0 = y1 - thick; x1 = x0 + barLen;
	} else { // down
		y0 = cy0 + gap; x0 = cx0 - barLen / 2;
		y1 = y0 + thick; x1 = x0 + barLen;
	}

	// Dark trough
	SurfColor(40, 46, 56, 230);
	SurfFill(x0, y0, x1, y1);
	SurfColor(22, 26, 32, 240);
	SurfOutline(x0, y0, x1, y1);

	const int fillAmt = (int)(frac * (float)barLen + 0.5f);
	if (fillAmt > 0) {
		SurfColor(fr, fg, fb, 255);
		if (vert) {
			// remaining ammo sits at bottom; empty grows from top
			SurfFill(x0, y1 - fillAmt, x1, y1);
		} else {
			// remaining ammo sits at right; empty grows from left
			SurfFill(x1 - fillAmt, y0, x1, y1);
		}
	}

	// Cursor tick at deplete frontier (slightly longer than thickness).
	SurfColor(frac > 0.001f ? fr : 255, frac > 0.001f ? fg : 64, frac > 0.001f ? fb : 48, 255);
	if (vert) {
		const int cy = (fillAmt > 0) ? (y1 - fillAmt) : y1;
		const int ext = 3;
		SurfLine(x0 - ext, cy, x1 + ext, cy);
		SurfLine(x0 - ext, cy - 1, x1 + ext, cy - 1);
	} else {
		const int cx = (fillAmt > 0) ? (x1 - fillAmt) : x0;
		const int ext = 3;
		SurfLine(cx, y0 - ext, cx, y1 + ext);
		SurfLine(cx - 1, y0 - ext, cx - 1, y1 + ext);
	}

	if (preview) {
		SurfColor(80, 230, 255, 160);
		SurfOutline(cx0 - HudScalePx(70, sc), cy0 - HudScalePx(70, sc),
			cx0 + HudScalePx(70, sc), cy0 + HudScalePx(70, sc));
	}
}

void AmmoSegHudPaint() {
	const bool preview = HudMenuPage(kMenuPageAmmoGfxHud);
	if (!preview && g_ammoGfxStyle != 1) return;
	if (preview && g_ammoGfxStyle != 1) return;
	if (!AmmoGfxPaintGate(preview)) return;
	ClampAmmoGfxHud();

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	const int sc = g_ammoGfxScale;

	int clip = -1, maxClip = 0;
	if (!AmmoGfxSampleClip(&clip, &maxClip)) {
		if (preview) { clip = 18; maxClip = 30; }
		else return;
	}
	int fr = 255, fg = 255, fb = 255;
	AmmoGfxSegRgb(clip, maxClip, &fr, &fg, &fb);

	float frac = maxClip > 0 ? (float)clip / (float)maxClip : 0.f;
	if (frac < 0.f) frac = 0.f;
	if (frac > 1.f) frac = 1.f;

	int cells = maxClip;
	if (cells > 50) cells = 50;
	if (cells < 1) cells = 1;
	int lit;
	if (maxClip > 50)
		lit = (int)(frac * 50.f + 0.5f);
	else
		lit = clip;
	if (lit < 0) lit = 0;
	if (lit > cells) lit = cells;
	const int cols = (cells + 9) / 10;
	const int cell = HudScalePx(7, sc);
	const int gapC = HudScalePx(2, sc);
	const int colGap = HudScalePx(3, sc);
	const int gridW = cols * cell + (cols - 1) * colGap;
	const int rowsFull = 10;
	const int gridH = rowsFull * cell + (rowsFull - 1) * gapC;

	if (!g_ammoSegAbs) {
		// Default: right of crosshair (same visual as former pos==3).
		g_ammoSegOffX = sw / 2 + HudScalePx(34, sc);
		g_ammoSegOffY = sh / 2 - gridH / 2;
		if (g_ammoSegOffX < 0) g_ammoSegOffX = 0;
		if (g_ammoSegOffY < 0) g_ammoSegOffY = 0;
		g_ammoSegAbs = true;
	}
	int ox = 0, oy = 0;
	HudPlaceAbs(sw, sh, gridW, gridH, g_ammoSegOffX, g_ammoSegOffY, &ox, &oy);

	for (int i = 0; i < cells; ++i) {
		const int col = i / 10;
		const int row = i % 10;
		const int x = ox + col * (cell + colGap);
		const int y = oy + row * (cell + gapC);
		if (i < lit) {
			SurfColor(fr, fg, fb, 255);
			SurfFill(x, y, x + cell, y + cell);
		} else {
			SurfColor(40, 48, 58, 160);
			SurfFill(x, y, x + cell, y + cell);
			SurfColor(70, 80, 95, 200);
			SurfOutline(x, y, x + cell, y + cell);
		}
	}

	if (preview) {
		SurfColor(80, 230, 255, 160);
		SurfOutline(ox - 4, oy - 4, ox + gridW + 4, oy + gridH + 4);
	}
}

void AmmoGfxHudPaint() {
	AmmoBarHudPaint();
	AmmoSegHudPaint();
}

void HudOverlayAmmoArcPaint() {
	int offX = 0, offY = 0, scale = 100, side = 3;
	bool preview = false;
	if (!HudOverlayGetAmmoArcConfig(&offX, &offY, &scale, &side, &preview)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	int clip = -1, maxClip = 0;
	if (!AmmoGfxSampleClip(&clip, &maxClip)) {
		if (!preview) return;
		clip = 18;
		maxClip = 30;
	}
	float frac = maxClip > 0 ? (float)clip / (float)maxClip : 0.f;
	if (frac < 0.f) frac = 0.f;
	if (frac > 1.f) frac = 1.f;
	int red = 255, green = 255, blue = 255;
	AmmoGfxHealthRgb(frac, &red, &green, &blue);

	const int radius = HudScalePx(46, scale);
	const int thick = HudScalePx(8, scale);
	const float pi = 3.14159265f;
	// Rotate the whole original right-side layout, including its 76px circle-center
	// offset. Menu offsets are additional translation, not the circle's base anchor.
	const int centerGap = 76;
	offX += side == 2 ? -centerGap : side == 3 ? centerGap : 0;
	offY += side == 1 ? -centerGap : side == 0 ? centerGap : 0;
	const float center = side == 1 ? -pi * 0.5f : side == 2 ? pi : side == 3 ? 0.f : pi * 0.5f;
	const float a0 = center - pi * 0.28f;
	const float a1 = center + pi * 0.28f;
	HudOverlaySetAmmoArc(sw, sh, offX, offY, radius, thick, a0, a1, frac,
		clip, maxClip, red, green, blue, preview);
}

// =============================================================================
// Graphic reload progress HUD — full ring + center rifle bullet (independent of FireGate).
// =============================================================================
void HudOverlayReloadRingPaint() {
	int offX = 0, offY = 0, radius = 44;
	bool preview = false;
	if (!HudOverlayGetReloadRingConfig(&offX, &offY, &radius, &preview)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	static bool s_wasReloading = false;
	static DWORD s_holdUntil = 0;
	bool reloading = false;
	float frac = 0.f;
	const bool sampled = ReloadProgressSample(&reloading, &frac, nullptr);
	const DWORD now = GetTickCount();
	if (sampled && reloading) {
		s_wasReloading = true;
		s_holdUntil = 0;
	} else if (s_wasReloading) {
		s_wasReloading = false;
		s_holdUntil = now + 120;
		frac = 1.f;
	}
	const bool hold = s_holdUntil != 0 && (LONG)(s_holdUntil - now) > 0;
	if (hold) frac = 1.f;
	else if (!reloading) s_holdUntil = 0;
	bool show = (sampled && reloading) || hold;
	if (preview && !show) {
		show = true;
		frac = 0.58f;
	}
	if (!show) return;
	if (frac < 0.f) frac = 0.f;
	if (frac > 1.f) frac = 1.f;
	const int thick = (radius <= 36) ? 3 : (radius <= 48) ? 4 : (radius <= 64) ? 5 : (radius <= 96) ? 6 : 7;
	HudOverlaySetReloadRing(sw, sh, offX, offY, radius, thick, frac, preview);
}


void UpgradeAmmoHudPaint() {
	const bool preview = HudMenuPage(kMenuPageUpgradeAmmoHud);
	if (!HudWantsDraw(g_optUpgradeAmmoHud, kMenuPageUpgradeAmmoHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	if (!preview && IdleSwapHudQuietActive()) return;

	bool inc = false;
	bool exp = false;
	bool laser = false;
	const int local = EngLocal();
	if (local > 0 && g_offUpgradeBitVec >= 0 && g_offUpgradeBitVec <= 0x4000) {
		void* me = EntGet(local);
		if (EntReadable(me)) {
			TeamHudSlot tmp{};
			if (local <= 32)
				TeamHudReadWepsCached(me, local, &tmp, 100u);
			else
				TeamHudReadWeps(me, &tmp);
			const int wepIdx = tmp.priWepIdx;
			if (wepIdx > 0 && wepIdx < 2048 && TeamHudIsGun(tmp.wep)) {
				void* wep = EntGet(wepIdx);
				if (EntReadable(wep) && EntNetworkable(wepIdx)) {
					int* pb = (int*)((uint8_t*)wep + g_offUpgradeBitVec);
					if (PtrCommitted(pb)) {
						const int bits = *pb;
						inc = (bits & 1) != 0;
						exp = (bits & 2) != 0;
						laser = (bits & 4) != 0;
					}
				}
			}
		}
	}
	if (preview || (!inc && !exp && !laser
		&& HudLayoutPeer(g_optUpgradeAmmoHud, kMenuPageUpgradeAmmoHud))) {
		inc = true;
		exp = true;
		laser = true;
	}
	if (!inc && !exp && !laser) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh) || sw < 160 || sh < 120) return;
	const int sc = HudClampScale(g_upgradeAmmoHudScale);
	const int icon = HudScalePx(28, sc);
	const int gap = HudScalePx(8, sc);
	int n = 0;
	if (inc) ++n;
	if (exp) ++n;
	if (laser) ++n;
	const int panelW = n * icon + (n > 1 ? (n - 1) * gap : 0);
	const int panelH = icon;
	if (!g_upgradeAmmoHudAbs) {
		g_upgradeAmmoHudOffX = sw / 2 - panelW - HudScalePx(56, sc);
		g_upgradeAmmoHudOffY = sh / 2 + HudScalePx(36, sc);
		if (g_upgradeAmmoHudOffX < 0) g_upgradeAmmoHudOffX = 0;
		if (g_upgradeAmmoHudOffY < 0) g_upgradeAmmoHudOffY = 0;
		g_upgradeAmmoHudAbs = true;
	}
	int x = 0, y = 0;
	HudPlaceAbs(sw, sh, panelW, panelH, g_upgradeAmmoHudOffX, g_upgradeAmmoHudOffY, &x, &y);
	int cx = x + icon / 2;
	const int cy = y + icon / 2;
	if (inc) {
		UpgradeAmmoDrawFlame(cx, cy, sc);
		cx += icon + gap;
	}
	if (exp) {
		UpgradeAmmoDrawBurst(cx, cy, sc);
		cx += icon + gap;
	}
	if (laser)
		UpgradeAmmoDrawLaser(cx, cy, sc);
}

// Local black-and-white (third-strike) screen badge — same cracked-heart ECG as world HUD.
void BwSelfHudPaint() {
	const bool preview = HudMenuPage(kMenuPageBwSelfHud);
	if (!HudWantsDraw(g_optBwSelfHud, kMenuPageBwSelfHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	if (!preview && IdleSwapHudQuietActive()) return;

	bool show = preview;
	if (!preview) {
		if (TeamHudLocalIsSpectating()) return;
		const int local = EngLocal();
		if (local <= 0) return;
		void* me = EntGet(local);
		if (!EntReadable(me)) return;
		if (g_offLifeState >= 0 && g_offLifeState <= 0x4000) {
			const unsigned char* pl = (const unsigned char*)me + g_offLifeState;
			if (PtrCommitted(pl) && *pl != 0) return;
		}
		if (g_offIsIncapacitated >= 0 && g_offIsIncapacitated <= 0x4000) {
			const unsigned char* pi = (const unsigned char*)me + g_offIsIncapacitated;
			if (PtrCommitted(pi) && *pi != 0) return;
		}
		if (g_offThirdStrike >= 0 && g_offThirdStrike <= 0x4000) {
			const unsigned char* pt = (const unsigned char*)me + g_offThirdStrike;
			if (PtrCommitted(pt)) show = (*pt != 0);
		}
	}
	if (!show && HudLayoutPeer(g_optBwSelfHud, kMenuPageBwSelfHud))
		show = true;
	if (!show) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh) || sw < 160 || sh < 120) return;
	const int sc = HudClampScale(g_bwSelfHudScale);
	// Base geometry is 2× menu scale so 100% matches the previous barely-readable 200%.
	const int drawSc = sc * 2;
	const int halfW = HudScalePx(14, sc) * 2;
	const int halfH = HudScalePx(12, sc) * 2;
	const int panelW = halfW * 2 + HudScalePx(4, sc) * 2;
	const int panelH = halfH * 2 + HudScalePx(4, sc) * 2;
	if (!g_bwSelfHudAbs) {
		g_bwSelfHudOffX = sw / 2 - panelW / 2;
		g_bwSelfHudOffY = sh / 2 + HudScalePx(72, sc);
		if (g_bwSelfHudOffX < 0) g_bwSelfHudOffX = 0;
		if (g_bwSelfHudOffY < 0) g_bwSelfHudOffY = 0;
		g_bwSelfHudAbs = true;
	}
	int x = 0, y = 0;
	HudPlaceAbs(sw, sh, panelW, panelH, g_bwSelfHudOffX, g_bwSelfHudOffY, &x, &y);
	const int cx = x + panelW / 2;
	const int cy = y + panelH / 2;
	const int pulse = 220 + (int)(((GetTickCount() / 110u) % 2u) * 28);
	WorldNoticeDrawBlackWhite(cx, cy, pulse, drawSc);
}

void ItemHudSample() {
	const DWORD now = GetTickCount();
	if (g_itemHudSampleAt && now - g_itemHudSampleAt < 100u) return;
	g_itemHudSampleAt = now;
	g_itemHudNade = g_itemHudKit = g_itemHudPills = TW_None;
	const int local = EngLocal();
	if (local <= 0) return;
	void* me = EntGet(local);
	if (!EntReadable(me) || TeamHudLocalIsSpectating()) return;
	TeamHudSlot tmp{};
	// Same 100 ms cadence as before, but share the local inventory snapshot with
	// Team/Ammo HUD consumers instead of doing a second 16-handle walk.
	if (local <= 32)
		TeamHudReadWepsCached(me, local, &tmp, 100u);
	else
		TeamHudReadWeps(me, &tmp);
	g_itemHudNade = tmp.nade;
	g_itemHudKit = tmp.kit;
	g_itemHudPills = tmp.pills;
}

void ItemHudPaint() {
	const bool preview = HudMenuPage(kMenuPageItemHud);
	if (!preview && IdleSwapHudQuietActive()) return;
	if (!HudWantsDraw(g_optItemHud, kMenuPageItemHud)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading() || !MenuEnsureSurf()) return;
	ItemHudSample();
	int items[3] = { g_itemHudNade, g_itemHudKit, g_itemHudPills };
	if (preview) {
		if (!items[0]) items[0] = TW_Molly;
		if (!items[1]) items[1] = TW_Kit;
		if (!items[2]) items[2] = TW_Pills;
	}
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	const int sc = HudClampScale(g_itemHudScale);
	const int pad = HudScalePx(6, sc);
	const int cell = HudScalePx(42, sc);
	const int gap = HudScalePx(4, sc);
	const int icon = HudScalePx(30, sc);
	const int panelW = pad * 2 + cell;
	const int panelH = pad * 2 + cell * 3 + gap * 2;
	int x = 0, y = 0;
	HudFinalizePos(&g_itemHudAbs, &g_itemHudOffX, &g_itemHudOffY, g_itemHudAlign,
		sw, sh, panelW, panelH, &x, &y);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	const COLORREF accents[3] = { RGB(255, 166, 62), RGB(86, 220, 178), RGB(115, 190, 255) };
	for (int i = 0; i < 3; ++i) {
		const int cy = y + pad + i * (cell + gap);
		const COLORREF c = accents[i];
		SurfColor(255, 255, 255, items[i] ? 18 : 8);
		SurfFill(x + pad, cy, x + pad + cell, cy + cell);
		SurfColor(GetRValue(c), GetGValue(c), GetBValue(c), items[i] ? 210 : 65);
		SurfFill(x + pad, cy, x + pad + HudScalePx(2, sc), cy + cell);
		SurfOutline(x + pad, cy, x + pad + cell, cy + cell);
		if (items[i]) {
			const int ix = x + pad + (cell - icon) / 2;
			const int iy = cy + (cell - icon) / 2;
			TeamHudDrawIconEx(ix, iy, icon, icon, items[i], nullptr, -1, false);
		} else {
			const int mid = cy + cell / 2;
			SurfColor(150, 160, 170, 80);
			SurfFill(x + pad + cell / 3, mid, x + pad + cell * 2 / 3, mid + 1);
		}
	}
	HudCornerBrackets(x, y, panelW, panelH, HudScalePx(8, sc), RGB(110, 220, 205), preview ? 200 : 105);
}

void DirectorHudPaint() {
	const bool preview = HudMenuPage(17);
	if (IdleSwapDeferEntityHud(preview)) return;
	if (!HudWantsDraw(g_optCustomDirectorHud, 17)) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	TeamHudEnsureFonts();
	SurfEnsureSpeedTimerFonts();
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const TeamHudSlot* cols[kTeamHudMax + 1]{};
	int nCols = 0;
	auto pushCol = [&](const TeamHudSlot& sl) {
		if (!sl.present || nCols >= kTeamHudMax + 1) return;
		cols[nCols++] = &sl;
	};
	pushCol(g_teamHudLocal);
	for (int i = 0; i < kTeamHudMax; ++i)
		pushCol(g_teamHudSlots[i]);
	for (int a = 1; a < nCols; ++a) {
		const TeamHudSlot* t = cols[a];
		int j = a;
		while (j > 0 && cols[j - 1]->ent > t->ent) {
			cols[j] = cols[j - 1];
			--j;
		}
		cols[j] = t;
	}
	int n = nCols;
	if (n <= 0) {
		if (!preview && !HudLayoutPeer(g_optCustomDirectorHud, 17)) return;
		n = 4;
		for (int i = 0; i < n; ++i) cols[i] = nullptr;
	}
	const int sc = g_dirHudScale;
	const int colW = HudScalePx(54, sc);
	const int barH = HudScalePx(92, sc);
	const int face = HudScalePx(36, sc);
	const int pad = HudScalePx(12, sc);
	const int headerH = HudScalePx(22, sc);
	const int panelW = n * colW + pad * 2;
	const int panelH = pad + headerH + HudScalePx(6, sc) + barH + HudScalePx(8, sc) + face + HudScalePx(28, sc);
	int x = 0, y = 0;
	HudFinalizePos(&g_dirHudAbs, &g_dirHudX, &g_dirHudY, 4, sw, sh, panelW, panelH, &x, &y);

	const COLORREF accent = RGB(80, 210, 230);
	HudPanelShadow(x, y, panelW, panelH);
	HudGlassBody(x, y, panelW, panelH);
	HudTopAccent(x, y, panelW, accent, 2);
	HudCornerBrackets(x, y, panelW, panelH, HudScalePx(10, sc), accent, preview ? 200 : 110);

	if (g_hudSubFont) {
		int tempo = 0;
		bool haveTempo = DirectorTempoRead(&tempo);
		if (preview && !haveTempo) {
			haveTempo = true;
			tempo = 0;
		}
		const wchar_t* mood = haveTempo ? DirectorTempoName(tempo) : nullptr;
		if (mood && mood[0])
			HudTagPill(x + pad, y + pad + 2, g_hudSubFont, mood, RGB(255, 244, 236), DirectorTempoColor(tempo));
	}

	const int barTop = y + pad + headerH + HudScalePx(6, sc);
	for (int c = 0; c < n; ++c) {
		const TeamHudSlot* sp = cols[c];
		const TeamHudSlot& s = sp ? *sp : g_teamHudSlots[c];
		const int cx = x + pad + c * colW;
		const int barX = cx + HudScalePx(12, sc);
		const int barW = HudScalePx(26, sc);
		const int barY = barTop;
		SurfColor(6, 8, 12, 220);
		SurfFill(barX, barY, barX + barW, barY + barH);
		SurfColor(255, 255, 255, 30);
		SurfOutline(barX, barY, barX + barW, barY + barH);
		const int y50 = barY + barH - barH * 50 / 100;
		const int y85 = barY + barH - barH * 85 / 100;
		SurfColor(255, 255, 255, 40);
		SurfFill(barX, y50, barX + barW, y50 + 1);
		SurfFill(barX, y85, barX + barW, y85 + 1);
		int h = (sp && sp->present) ? (sp->intensity * barH / 100) : (preview ? (40 + c * 12) * barH / 100 : 0);
		if (h < 0) h = 0;
		if (h > barH) h = barH;
		int iv = (sp && sp->present) ? sp->intensity : (preview ? (40 + c * 12) : 0);
		int cr = 80, cg = 220, cb = 120;
		HudIntensityColor(iv, &cr, &cg, &cb);
		if (h > 0) {
			SurfColor(cr, cg, cb, 240);
			SurfFill(barX + 1, barY + barH - h, barX + barW - 1, barY + barH - 1);
			SurfColor(255, 255, 255, 50);
			SurfFill(barX + 2, barY + barH - h, barX + barW - 2, barY + barH - h + 1);
		}
		wchar_t ivBuf[8]{};
		swprintf_s(ivBuf, 8, L"%d", iv);
		if (g_hudSubFont) {
			const int iw = SurfTextWFont(g_hudSubFont, ivBuf);
			SurfTextAt(g_hudSubFont, barX + (barW - iw) / 2, barY - HudScalePx(14, sc), RGB(cr, cg, cb), ivBuf);
		}
		const int fx = cx + (colW - face) / 2;
		const int fy = barY + barH + HudScalePx(8, sc);
		const int idFace = TeamHudBindFace(sp && sp->present ? sp->character : c, sp && sp->present && sp->incap);
		if (idFace >= 0) {
			SurfColor(255, 255, 255, sp && sp->present ? 255 : 90);
			SurfDrawTextured(fx, fy, fx + face, fy + face, idFace);
		} else {
			SurfColor(30, 36, 46, 200);
			SurfFill(fx, fy, fx + face, fy + face);
		}
	}
	if (preview) {
		SurfColor(80, 210, 230, 170);
		SurfOutline(x, y, x + panelW, y + panelH);
	}
}

void WitchStartleHudClear() {
	g_witchStartleUntil = 0;
	g_witchStartleT0 = 0;
	g_witchStartleName[0] = 0;
}

void WitchStartleFillName(int entIdx, void* e, wchar_t* out, int cap) {
	if (!out || cap <= 0) return;
	out[0] = 0;
	int character = -1;
	if (e && g_offSurvivorCharacter >= 0 && g_offSurvivorCharacter <= 0x4000) {
		const int* pc = (const int*)((const uint8_t*)e + g_offSurvivorCharacter);
		if (PtrCommitted(pc) && *pc >= 0 && *pc <= 7)
			character = *pc;
	}
	// L4D1 survivors reuse the raw 0..3 values used by the L4D2 cast. Resolve
	// the generation from this survivor's model/name before selecting a custom
	// BOT name; this is one event entity lookup and does not add a scan.
	character = TeamHudResolveProp(entIdx, e, character);
	PlayerInfo pi{};
	const bool havePi = entIdx > 0 && EngPlayerInfo(entIdx, &pi);
	if (havePi && PlayerInfoIsBot(pi) && g_optClientBotNames
		&& character >= 0 && character < kBotNameCount
		&& g_clientBotNames[character][0]) {
		wcsncpy(out, g_clientBotNames[character], cap - 1);
		out[cap - 1] = 0;
		return;
	}
	if (havePi && pi.name[0]) {
		MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, out, cap);
		out[cap - 1] = 0;
		if (out[0]) return;
	}
	wcsncpy(out, TeamHudCharName(character), cap - 1);
	out[cap - 1] = 0;
	if (!out[0] || (out[0] == L'?' && !out[1]))
		wcsncpy(out, UiText(UiId::Text_d455cf05, L"幸存者"), cap - 1);
	out[cap - 1] = 0;
}

void WitchStartleHudEnsureFonts() {
	ClampWitchStartleHud();
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	const int want = HudFontPx(g_witchStartleHudFont);
	if (!g_witchStartleFontId || g_witchStartleFontTall != want) {
		unsigned long f = SurfMakeYahei(want, 800, flags);
		if (f) { g_witchStartleFontId = f; g_witchStartleFontTall = want; }
	}
}

void WitchStartleHudPaint() {
	bool preview = HudMenuPage(kMenuPageWitchStartleHud);
	const bool peer = HudLayoutPeer(g_optWitchStartleHud, kMenuPageWitchStartleHud);
	if (!HudWantsDraw(g_optWitchStartleHud, kMenuPageWitchStartleHud)
		&& !(g_optWitchStartleHud && g_witchStartleUntil))
		return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	const DWORD now = GetTickCount();
	const bool live = g_optWitchStartleHud && g_witchStartleUntil && now < g_witchStartleUntil;
	if (!preview && !live) {
		if (g_witchStartleUntil)
			WitchStartleHudClear();
		if (!peer) return;
		preview = true;
	}
	if (!MenuEnsureSurf()) return;
	WitchStartleHudEnsureFonts();
	if (!g_witchStartleFontId) return;

	DWORD t0 = preview ? now : g_witchStartleT0;
	DWORD until = preview ? now + 4500u : g_witchStartleUntil;
	if (!t0) t0 = now;
	DWORD dur = until > t0 ? until - t0 : 4500u;
	if (dur < 400) dur = 400;
	int alpha = 255;
	if (!preview) {
		const DWORD elapsed = now - t0;
		if (elapsed < 100)
			alpha = (int)(elapsed * 255 / 100);
		else if (dur > 450 && elapsed > dur - 450)
			alpha = (int)((dur - elapsed) * 255 / 450);
		if (alpha < 12) return;
		if (alpha > 255) alpha = 255;
	}

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh) || sw < 160 || sh < 120) return;
	const int sc = g_witchStartleHudScale;
	wchar_t text[96]{};
	if (preview)
		wcsncpy(text, UiText(UiId::Text_3eb17d28, L"Ellis 惊扰了 Witch"), 95);
	else if (g_witchStartleName[0])
		swprintf_s(text, 96, UiText(UiId::Text_391e1347, L"%s 惊扰了 Witch"), g_witchStartleName);
	else
		wcsncpy(text, UiText(UiId::Text_265aa3d6, L"幸存者 惊扰了 Witch"), 95);
	text[95] = 0;
	const int textW = SurfTextWFont(g_witchStartleFontId, text);
	const int textH = g_witchStartleFontTall > 0 ? g_witchStartleFontTall + 2 : 20;
	const int stripe = HudScalePx(4, sc);
	const int icon = HudScalePx(22, sc);
	const int padX = HudScalePx(12, sc);
	const int padY = HudScalePx(8, sc);
	const int panelW = stripe + padX + icon + padX + textW + padX;
	const int panelH = padY * 2 + (textH > icon ? textH : icon);
	int x = 0, y = 0;
	HudFinalizePos(&g_witchStartleHudAbs, &g_witchStartleHudOffX, &g_witchStartleHudOffY,
		g_witchStartleHudAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 72, 58);
	HudPanelShadow(x, y, panelW, panelH);
	SurfColor(12, 8, 10, alpha * 118 / 255);
	SurfFill(x, y, x + panelW, y + panelH);
	SurfColor(255, 255, 255, alpha * 16 / 255);
	SurfFill(x + stripe + 1, y + 1, x + panelW - 1, y + 2);
	SurfColor(255, 72, 58, alpha * 235 / 255);
	SurfFill(x, y, x + stripe, y + panelH);
	HudCornerBrackets(x, y, panelW, panelH, 8, accent, preview ? alpha * 190 / 255 : alpha * 150 / 255);

	const int icx = x + stripe + padX + icon / 2;
	const int icy = y + panelH / 2;
	const int ih = icon, iHalf = icon / 2;
	for (int iy = 0; iy < ih; ++iy) {
		const int span = (iHalf * iy + ih / 2) / ih;
		SurfColor(255, 56, 48, alpha);
		SurfFill(icx - span, icy - ih / 2 + iy, icx + span + 1, icy - ih / 2 + iy + 1);
	}
	SurfColor(18, 6, 6, alpha);
	for (int iy = 3; iy < ih - 3; ++iy) {
		int span = (iHalf * iy + ih / 2) / ih - 2;
		if (span < 1) continue;
		SurfFill(icx - span, icy - ih / 2 + iy, icx + span + 1, icy - ih / 2 + iy + 1);
	}
	WorldNoticeThickLine(icx - 5, icy - 1, icx + 6, icy + 5, 2, 236, 228, 214, alpha);
	WorldNoticeThickLine(icx - 6, icy + 2, icx + 5, icy + 8, 2, 236, 228, 214, alpha);
	SurfColor(255, 236, 220, alpha);
	SurfFill(icx - 1, icy - ih / 2 + 4, icx + 2, icy - ih / 2 + 10);
	SurfFill(icx - 1, icy - ih / 2 + 12, icx + 2, icy - ih / 2 + 14);

	const int tx = x + stripe + padX + icon + padX;
	const int ty = y + (panelH - textH) / 2;
	SurfTextAt(g_witchStartleFontId, tx, ty, RGB(255, 232, 224), text);
	if (preview) {
		SurfColor(255, 72, 58, alpha * 130 / 255);
		SurfOutline(x, y, x + panelW, y + panelH);
	}
}

void WitchStartleOnEvent(void* ev) {
	if (!g_optWitchStartleHud && !g_optWorldWitchStartle) return;
	if (!ev || !SkeetoFeaturesOn()) return;
	if (!EvBool(ev, "first", true)) return;
	const int ent = WorldNoticeEventPlayer(ev, "userid");
	if (ent <= 0) return;
	void* e = EntGet(ent);
	if (!EntReadable(e) || g_offTeam < 0 || g_offTeam > 0x4000) return;
	const int* team = (const int*)((const uint8_t*)e + g_offTeam);
	if (!PtrCommitted(team) || *team != 2) return;

	wchar_t name[24]{};
	WitchStartleFillName(ent, e, name, 24);
	const DWORD now = GetTickCount();
	if (g_optWitchStartleHud) {
		g_witchStartleT0 = now;
		g_witchStartleUntil = now + 4500u;
		wcsncpy(g_witchStartleName, name[0] ? name : UiText(UiId::Text_d455cf05, L"幸存者"), 23);
		g_witchStartleName[23] = 0;
	}
	if (g_optWorldWitchStartle && WorldNoticeSurvivorEnt(ent)) {
		int slot = -1;
		for (int i = 0; i < kTeamHudMax; ++i) {
			if (g_worldWitchStartleNotice[i].ent == ent) { slot = i; break; }
			if (slot < 0 && !g_worldWitchStartleNotice[i].until) slot = i;
		}
		if (slot < 0) slot = 0;
		g_worldWitchStartleNotice[slot].ent = ent;
		g_worldWitchStartleNotice[slot].born = now;
		g_worldWitchStartleNotice[slot].until = now + 5000u;
		g_worldWitchStartleAny = true;
	}
	Log("witch_harasser_set first=1 ent=%d", ent);
}
