// skeeto_proxy.cpp — necola.dll proxy for Necola 1.4
// Loads necola_orig.dll, registers IGameEventListener2 (MSVC vtable).
// Does NOT MinHook or VMT-hook FireEventClientSide (that fights Necola and crashes on map load).
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>
#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "kernel32.lib")
#endif
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <cstdlib>
#include <atomic>
#include "skeeto_style.h"
#include "skeeto_version.generated.h"

using CreateInterfaceFn = void* (*)(const char*, int*);
using DispatchParticleFn = void(__cdecl*)(const char*, float*, float*, int, int, int);
using PrecacheParticleFn = int(__cdecl*)(const char*);
using FindParticleFn = int(__thiscall*)(void*, const char*);
using AddListenerFn = bool(__thiscall*)(void*, void*, const char*, bool);
using ClientCmdFn = void(__thiscall*)(void*, const char*);
using FindCmdBaseFn = void*(__thiscall*)(void*, const char*);
using RemoveFlagsFn = void(__thiscall*)(void*, int);
using AddFlagsFn = void(__thiscall*)(void*, int);
using GetFlagsFn = int(__thiscall*)(void*);

// From Source SDK / Necola CvarsB.h
enum : int {
	kFCvarDevOnly = (1 << 1),
	kFCvarHidden = (1 << 4),
	kFCvarCheat = (1 << 14),
	kFCvarClientCmdCanExecute = (1 << 30),
};

static uintptr_t FindPat(const char* module, const char* pattern);
static int CountPat(const char* module, const char* pattern, uintptr_t* out, int maxOut);

static std::atomic_bool g_ready{false};
// Worker loop flag — cleared on process/DLL detach so we stop touching engine APIs during quit.
static std::atomic_bool g_run{true};
// Last Skeeto subsystem before a crash (sticky). 0=idle.
enum : int {
	kBcIdle = 0, kBcPaint, kBcEvent, kBcHit, kBcLevelInit, kBcXhair, kBcWorker, kBcSettings, kBcMenu
};
static std::atomic<int> g_crashBc{kBcIdle};
static void CrashMark(int bc) { g_crashBc.store(bc, std::memory_order_relaxed); }
static DispatchParticleFn g_dispatch = nullptr;
static PrecacheParticleFn g_precache = nullptr;
static FindParticleFn g_findParticle = nullptr;
static void* g_particleMgr = nullptr;
static void* g_engine = nullptr;
static void* g_events = nullptr;
static void* g_entlist = nullptr;
static void* g_trace = nullptr; // EngineTraceClient003 — CI gun hit TraceRay
static void* g_engineSound = nullptr; // IEngineSoundClient003
static void* g_cvar = nullptr;
static void* g_surf = nullptr; // IMatSystemSurface — menu / HUD / HP gauge
static bool g_overlayCmdUnlocked = false;
static int g_lastGround[65]; // 1=onground (default); 0=airborne — never default to air (false skeets)
static DWORD g_lastAirborneAt[65]{}; // last time Sample saw FL_ONGROUND clear
static int g_offFlags = -1;
static int g_offOrigin = -1;
static int g_offOriginCommon = -1; // Infected/Witch may differ from player origin offset
static int g_offTeam = -1;
static int g_offHealth = -1;
static int g_offHealthCommon = -1; // Infected/Witch m_iHealth
static int g_offLifeState = -1;    // byte: 0=alive (players / SI)
static int g_offLifeStateCommon = -1; // Infected/Witch only — do NOT reuse player offset
static int g_offActiveWeapon = -1; // CTerrorPlayer::m_hActiveWeapon
static int g_offViewOffset = -1;
static int g_offAbsVelocity = -1; // CBaseEntity::m_vecVelocity[0] — throw inherits player vel
static int g_offHideHUD = -1; // C_BasePlayer::m_iHideHUD (nested DT_Local)
static int g_offZombieClass = -1; // CTerrorPlayer::m_zombieClass (8=Tank)
static int g_offIsIncapacitated = -1; // CTerrorPlayer::m_isIncapacitated (FF bleed false-positive gate)
static int g_lastHealth[65]{};
static bool g_healthInit[65]{};
static int g_hitDetectLogLeft = 80;
static DWORD g_hitDetectEnableAt = 0;
static std::atomic_bool g_hitDetectArmed{false};
using RecvVarProxyFn = void(*)(const void* pData, void* pStruct, void* pOut);
static RecvVarProxyFn g_origHealthProxy = nullptr;
static int g_healthProxyHooks = 0;
static FILE* g_log = nullptr;
static HMODULE g_self = nullptr;
static int g_deathLogLeft = 80;
static int g_evtLogLeft = 80;
static int g_skipLogLeft = 40;
static volatile LONG g_fireCalls = 0;
static float g_fxOrigin[3]{};
static float g_fxAngles[3]{0.f, 270.f, 0.f};
static bool g_listening = false;
static int g_localUserId = -1;
// Overlay clear delay (ms). Particles use PCF lifetime; r_screenoverlay has none — we clear manually.
// JSON may override per-event via optional overlay_ms (0 / omitted = these defaults).
static constexpr DWORD kOverlayClearMs = 240;
static constexpr DWORD kOverlayClearMsHit = 110; // hit marks feel sticky if too long
static constexpr DWORD kOverlayClearMsFx = 260;
static DWORD g_overlayClearAt = 0;
static DWORD g_lastHitFeedbackAt = 0;
static int g_feedbackLogLeft = 60;
// Common-hit (bullet_impact TraceRay) must not stomp kill/headshot sounds/overlays.
static DWORD g_suppressCommonHitUntil = 0;
static bool g_pendingCommonHit = false;
static DWORD g_pendingCommonHitAt = 0;
// Stamp from FireGameEvent; MASK_SHOT TraceRay + hit FX run on EngineVGui::Paint
// (not FireGameEvent, not FSN, not the Sleep worker — all three nested under or
// raced L4N's datacache walk; see reference/datacache崩溃排查.md).
static bool g_pendingCiImpactScan = false;
static float g_ciTracePos[3]{};
static DWORD g_lastCiTraceAt = 0;
static DWORD g_lastLocalUidRefreshAt = 0;
static constexpr DWORD kCiTraceMinIntervalMs = 40;
// SI melee hit via m_iHealth: only while recent LMB (not gun impacts — those use bullet_impact).
// Prevents "looking at SI while teammate shoots" after you fired / clicked.
static DWORD g_meleeAttackUntil = 0;
static DWORD g_lastLocalGunImpactAt = 0;
static float g_lastImpactPos[3]{};
static bool g_haveImpactPos = false;
static int g_siStreak = 0;
static int g_commonHitLogLeft = 40;

// Particle name PrecacheParticleSystem: full DIY catalog during connect/loading only.
static constexpr int kPtWarmCap = 256;
static char g_ptWarmName[kPtWarmCap][64]{};
static int g_ptWarmCount = 0;

// Optional Dispatch of selected-style particles on the game thread during loading
// (IBaseClientDLL::LevelInitPreEntity). Overlay/sound files are Precached automatically.
// Never Dispatch / r_screenoverlay / TraceRay from the Sleep worker.
static constexpr int kPtDispWarmCap = 64;
static char g_ptDispWarmName[kPtDispWarmCap][64]{};
static int g_ptDispWarmCount = 0;
// Menu option: Loading-stage Dispatch warm. Default off (people who never hitch
// should not get the screen-particle flash). Takes effect on next map load.
static bool g_optPtDispWarm = false;

// Overlay crosshair (Win32 layered window, Crosshair X-style). Default off = vanilla HUD.
// Each style keeps its own tune; factory defaults below are the Reset target.
static constexpr int kXhairStyleCount = 16;
static constexpr int kXhairColorCount = 10;
struct XhairTune {
	int color, size, length, gap, thick, dot, outline, alpha;
};
static const wchar_t* kXhairStyleNames[kXhairStyleCount] = {
	L"十字", L"十字+点", L"点", L"圆环", L"圆环+点", L"圆环+十字",
	L"T字", L"T字+点", L"方框", L"方框+点", L"菱形", L"人字",
	L"四角", L"双环", L"横线", L"三角"
};
static const wchar_t* kXhairColorNames[kXhairColorCount] = {
	L"白", L"绿", L"青", L"红", L"黄", L"橙", L"粉", L"蓝", L"黄绿", L"水绿"
};
static const COLORREF kXhairColorRgb[kXhairColorCount] = {
	RGB(255, 255, 255), RGB(0, 255, 0), RGB(0, 255, 255), RGB(255, 40, 40), RGB(255, 220, 0),
	RGB(255, 140, 0), RGB(255, 80, 180), RGB(80, 140, 255), RGB(180, 255, 0), RGB(0, 200, 120)
};
// color, size%, length, gap, thick, center-dot, outline, alpha
static const XhairTune kXhairFactory[kXhairStyleCount] = {
	{ 2, 100, 8, 4, 2, 0, 1, 100 }, // 十字
	{ 2, 100, 8, 4, 2, 3, 1, 100 }, // 十字+点
	{ 2, 100, 8, 0, 2, 4, 1, 100 }, // 点
	{ 2, 100, 7, 0, 2, 0, 1, 100 }, // 圆环
	{ 2, 100, 7, 0, 2, 3, 1, 100 }, // 圆环+点
	{ 2, 100, 7, 3, 2, 0, 1, 100 }, // 圆环+十字
	{ 2, 100, 8, 4, 2, 0, 1, 100 }, // T字
	{ 2, 100, 8, 4, 2, 3, 1, 100 }, // T字+点
	{ 2, 100, 6, 2, 2, 0, 1, 100 }, // 方框
	{ 2, 100, 6, 2, 2, 3, 1, 100 }, // 方框+点
	{ 2, 100, 7, 0, 2, 0, 1, 100 }, // 菱形
	{ 2, 100, 8, 2, 2, 0, 1, 100 }, // 人字
	{ 2, 100, 5, 6, 2, 0, 1, 100 }, // 四角
	{ 2, 100, 8, 3, 2, 0, 1, 100 }, // 双环
	{ 2, 100, 10, 4, 2, 0, 1, 100 }, // 横线
	{ 2, 100, 8, 0, 2, 0, 1, 100 }, // 三角
};
static XhairTune g_xhairTune[kXhairStyleCount]{};
static bool g_optXhair = false;
static bool g_optXhairRing = false; // engine HUD circle (spread); independent of static HUD
static int g_xhairRingColor = 1; // palette index; default green (not vanilla pale-white)
static int g_xhairRingMode = 2; // engine cl_crosshair_circle_mode: 0=aim+pellets (large) 2=inside lines
static int g_xhairRingAlpha = 70; // 10-100%; maps to cl_crosshair_circle_alpha (default ~180)
static bool g_optXhairTex = false; // engine HUD VTF crosshair (saved)
static bool g_optXhairTexFull = false; // false=center square, true=fullscreen
static int g_xhairTexSize = 80;    // on-screen pixels when not fullscreen
static char g_xhairTexMat[80]{};   // DrawSetTextureFile name (no materials/, no ext)
enum { kTexXhairMax = 64, kTexXhairSizeMin = 20, kTexXhairSizeMax = 800, kTexXhairSizeStep = 10 };
struct TexXhairTune {
	char mat[80];
	int size;
	int full;
};
static TexXhairTune g_texXhairTune[kTexXhairMax]{};
static int g_texXhairTuneCount = 0;

static bool TexXhairIsVanillaTest(const char* mat) {
	return mat && _stricmp(mat, "vgui/gfx/vgui/crosshair") == 0;
}
static int TexXhairTuneFind(const char* mat) {
	if (!mat || !mat[0]) return -1;
	for (int i = 0; i < g_texXhairTuneCount; ++i) {
		if (_stricmp(g_texXhairTune[i].mat, mat) == 0)
			return i;
	}
	return -1;
}
static void TexXhairRememberCurrent() {
	if (!g_xhairTexMat[0] || TexXhairIsVanillaTest(g_xhairTexMat)) return;
	int i = TexXhairTuneFind(g_xhairTexMat);
	if (i < 0) {
		if (g_texXhairTuneCount >= kTexXhairMax) return;
		i = g_texXhairTuneCount++;
		strncpy(g_texXhairTune[i].mat, g_xhairTexMat, 79);
		g_texXhairTune[i].mat[79] = 0;
	}
	g_texXhairTune[i].size = g_xhairTexSize;
	g_texXhairTune[i].full = g_optXhairTexFull ? 1 : 0;
}
static void TexXhairApplyCurrent() {
	const int i = TexXhairTuneFind(g_xhairTexMat);
	if (i < 0) return;
	g_xhairTexSize = g_texXhairTune[i].size;
	g_optXhairTexFull = g_texXhairTune[i].full != 0;
}
static void TexXhairApplyCurrentOrDefault() {
	if (TexXhairTuneFind(g_xhairTexMat) >= 0) {
		TexXhairApplyCurrent();
		return;
	}
	g_xhairTexSize = 80;
	g_optXhairTexFull = false;
}

static const wchar_t* XhairRingModeName() {
	if (g_xhairRingMode == 0) return L"瞄准误差+散布（偏大）";
	return L"严格贴合散布";
}

static int XhairRingAlphaEngine() {
	int p = g_xhairRingAlpha;
	if (p < 10) p = 10;
	if (p > 100) p = 100;
	return (p * 255 + 50) / 100;
}
static int g_xhairStyle = 1;
static int g_xhairHudMode = -1; // -1 = force re-apply engine cvars
static DWORD g_xhairHudAt = 0;
static bool g_xhairRingApplied = false;
struct XhairUserCvars {
	bool valid;
	int alpha, red, green, blue, dynamic, circleMode, circleAlpha;
};
static XhairUserCvars g_xhairUserCvars{};

static XhairTune& XhairT() {
	if (g_xhairStyle < 0) g_xhairStyle = 0;
	if (g_xhairStyle >= kXhairStyleCount) g_xhairStyle = kXhairStyleCount - 1;
	return g_xhairTune[g_xhairStyle];
}

static void XhairLoadFactoryAll() {
	for (int i = 0; i < kXhairStyleCount; ++i)
		g_xhairTune[i] = kXhairFactory[i];
}

static void XhairResetCurrentStyle() {
	if (g_xhairStyle < 0 || g_xhairStyle >= kXhairStyleCount) g_xhairStyle = 0;
	g_xhairTune[g_xhairStyle] = kXhairFactory[g_xhairStyle];
}

using LevelInitPreFn = void(__fastcall*)(void*, void*, const char*);
using ServerLevelInitFn = bool(__fastcall*)(void*, void*, const char*, const char*, const char*, const char*, bool, bool);
using ServerLevelShutdownFn = void(__fastcall*)(void*, void*);
using EnginePaintFn = void(__fastcall*)(void*, void*, int);
using InKeyEventFn = int(__fastcall*)(void*, void*, int, int, const char*);
static void* g_baseClient = nullptr;
static void* g_engineVgui = nullptr;
static LevelInitPreFn g_origLevelInitPre = nullptr;
static void* g_hookLevelInitPre = nullptr;
static bool g_levelInitHookLogged = false;
static ServerLevelInitFn g_origServerLevelInit = nullptr;
static ServerLevelShutdownFn g_origServerLevelShutdown = nullptr;
static void* g_hookServerLevelInit = nullptr;
static void* g_hookServerLevelShutdown = nullptr;
static bool g_serverLevelHookLogged = false;
static void EnsureServerGameHooks();
static EnginePaintFn g_origEnginePaint = nullptr;
static void* g_hookEnginePaint = nullptr;
static bool g_enginePaintHookLogged = false;
static InKeyEventFn g_origInKeyEvent = nullptr;
// First non-self next pointer ever seen (Necola or client). Survives VMT thrash so we
// never permanently lose the chain (null orig → WASD dead, mouse look still works).
static InKeyEventFn g_origInKeyEventPrimary = nullptr;
static void* g_hookInKeyEvent = nullptr;
static bool g_keyEventHookLogged = false;
static bool g_inKeyEatInstalled = false;
static DWORD g_inKeyEatWaitAt = 0;
// Defer disk save / ClientCmd out of IN_KeyEvent (heavy menu spam can break key chain).
static bool g_inKeyHook = false;
static bool g_settingsDirty = false;
static DWORD g_settingsSaveAt = 0;
static constexpr int kVmtInKeyEvent = 19;
static constexpr int kVmtEngineVGuiPaint = 14;
static constexpr int kPaintInGamePanels = (1 << 1);
static void RunDeferredCiTrace();
static void PumpGameThreadFeedback();
static void EnsureClientLevelHooks();
static void MenuPaintEngine();
static void ElimPaintHud();
static void ClockPaintHud();
static void SpeedPaintHud();
static void TimerPaintHud();
static void TeamHudPaintHud();
static void HudHideTick();
static void HudHideInvalidate();
static void HudHideRestore();
static void RoundTimerOnStart();
static void RoundTimerOnEnd();
static void SurfDrawTexXhair();
static void SurfDrawHudXhair();
static void XhairDrawOverlayGameThread();
static void ClientUxPaintThrowLand();
static void ClientUxPaintDmgNums();
static void ClientUxDmgReset();
static void ClientUxDmgOnSi(void* ent, int objectId, int dmg);
static void ClientUxOnDmgPlayerHurt(void* ev);
static void XhairNoteLoading();
static void ClientUxPaintNoCorpse();
static void ClientUxApplyNoCorpseCvars(bool force);
static void ClientUxApplyDirectorHud(bool force);
static void ClientUxClearNoCorpseTracks();
static void ClientUxOnSiDeath(void* ev);
static void ClientUxOnCiDeath(void* ev);
static void ClientUxOnWitchDeath(void* ev);
static void MenuOnVk(DWORD vk);
static bool MenuVkIsMenuOnly(DWORD vk);
static void MenuPollPaintKeys();
static int __fastcall Hooked_InKeyEvent(void* ecx, void* edx, int eventcode, int keynum, const char* binding);
static void UnhookInKeyEventIfOurs();

static bool g_emitSoundOk = true; // IEngineSound::EmitSound (atten) usable
static int g_emitSoundIdx = 5;    // first EmitSound overload (L4D2 hl2sdk)

// =============================================================================
// In-game menu — Necola-style IEngineVGui::Paint + IMatSystemSurface (no Win32 window).
// Static crosshair is a small click-through layered window (GDI+), same pixels as the
// old overlay — ISurface HUD cannot match that sharpness (game buffer / filter / FXAA).
// =============================================================================
static bool g_optSound = true;
// Independent feedback SFX volume (0–100). Multiplies EmitAmbientSound only —
// does NOT touch engine volume / snd_musicvolume.
static int g_optSfxVol = 100;
static bool g_optIcon = true;
// Hit mode: 0=全部关闭 1=仅特感Hit(默认,关普感) 2=特感+普感
static int g_optHitMode = 1;
// 特感击杀时：画面/音效是否用 [5] 选中的特感包（关=跟当前普感包）。二者独立。
static bool g_optSiVisual = true;
static bool g_optSiSound = true;
static bool g_optKillFx = true;
static bool g_optFf = false; // friendly-fire hit feedback (requires real HP drop)

static bool HitModeCiAllowed() { return g_optHitMode == 2; }
static bool HitModeSiAllowed() { return g_optHitMode != 0; }
// Any feature that needs local gun/melee attribution via HP proxy or impact stamp.
static bool NeedGunHitAttribution() {
	return HitModeSiAllowed() || HitModeCiAllowed() || g_optFf;
}
static bool NeedHealthProxyWork() {
	return HitModeSiAllowed() || g_optFf;
}
// Menu: 0 root, 1 hit-feedback, 2 SI hitch, 3 crosshair, 4 menu style,
// 5 elim HUD, 6 elim lines, 7 other, 8 local-listen, 9 client UX, 10 char pick
static int g_menuPage = 0;
static bool g_menuVisible = false;
static bool g_menuParked = false;
static HWND g_gameHwnd = nullptr;
static HWND g_xhairHwnd = nullptr;
static DWORD g_xhairCreateTid = 0;
static std::atomic_bool g_xhairHideScene{ false }; // set on game thread only
static DWORD g_xhairHideUntil = 0;
static bool g_sawGameWindow = false;
static void MenuShow(bool show);
static void MenuForceClose();
// Design size (logical). Window pixels = design * g_menuScale (capped to fit game client).
static const int kMenuDesignW = 600;
static const int kMenuDesignH = 720;
static const int kMenuMargin = 12;
static int g_menuW = kMenuDesignW;
static int g_menuH = kMenuDesignH;
static float g_menuScale = 1.f;
static constexpr int kSfxVolStep = 10;

// In-game menu layout / style (defaults match the original hardcoded look).
static constexpr int kUiBgCount = 6;
static constexpr int kUiTextCount = 5;
static constexpr int kUiTitleCount = 5;
static int g_uiSizePct = 100;   // 50–100, on top of resolution fit
static int g_uiAlignX = 0;      // 0 left 1 center 2 right
static int g_uiAlignY = 1;      // 0 top 1 center 2 bottom
static int g_uiBgAlpha = 90;    // 30–100 → fill alpha
static int g_uiBg = 0;
static int g_uiText = 0;
static int g_uiTitle = 0;
static int g_uiFont = 20;       // 14–28
static int g_uiOutline = 1;
// Kill-count HUD (net_graph-like). Local eliminations only.
static bool g_optElim = true;
static int g_elimMode = 0;      // 0=chapter reset, keep wipe; 1=persist; 2=chapter+wipe reset
static char g_elimLastMap[64] = {};
static int g_elimAlign = 0;     // legacy anchor; ignored once g_elimAbs
static bool g_elimAbs = false;  // offX/offY are top-left screen pixels
static int g_elimOffX = 16;
static int g_elimOffY = 16;
static int g_elimFont = 22;     // 18–48, Tahoma / YaHei px
static constexpr int kElimFontMin = 18;
static constexpr int kElimFontMax = 48;
static constexpr int kElimFontStep = 2;
static constexpr int kElimLineCount = 6;
static constexpr int kElimShowAll = 63;     // all 6 lines (bit5 = headshot rate)
static constexpr int kElimShowCompact = 3;  // SI + CI
static constexpr int kElimShowLegacyFull = 31; // old 5-line "full"
static std::atomic<int> g_elimSi{0};
static std::atomic<int> g_elimCi{0};
static std::atomic<int> g_elimSiMelee{0};
static std::atomic<int> g_elimSkeet{0};
static std::atomic<int> g_elimMeleeSkeet{0};
static std::atomic<int> g_elimHs{0};    // CI + SI headshot kills (for combined rate)
static int g_elimLang = 0;      // 0 = English, 1 = Chinese
static int g_elimCompact = 0;   // 0 = full preset, 1 = compact preset
static int g_elimShow = kElimShowAll; // bit0 SI, bit1 CI, bit2 melee, bit3 skeet, bit4 melee skeet
// World-clock HUD (local wall clock). Independent of elim HUD.
static bool g_optClock = true;
static int g_clockAlign = 1;    // legacy; ignored once g_clockAbs
static bool g_clockAbs = false;
static int g_clockOffX = 16;
static int g_clockOffY = 16;
static int g_clockFont = 26;    // time px; date is derived
// Extra HUD: speed / round timer / hidehud bits.
static bool g_optHudHide = false;
static bool g_optHudHideTeam = false;
static bool g_optHudHideWep = false;
static bool g_optHudHidePickup = false;
static bool g_optSpeed = true;
static int g_speedAlign = 5;
static bool g_speedAbs = false;
static int g_speedOffX = 0;
static int g_speedOffY = 48;
static int g_speedFont = 36;
static bool g_optTimer = true;
static int g_timerAlign = 4;
static bool g_timerAbs = false;
static int g_timerOffX = 0;
static int g_timerOffY = 10;
static int g_timerFont = 28;
static constexpr int kTeamHudMax = 8;
static bool g_optTeamHud = true;
static DWORD g_teamHudAllowModelAt = 0;
static int g_teamHudFont = 18;
static int g_teamHudSel = 0;
static int g_teamHudX[kTeamHudMax] = { 16, 16, 16, 16, 16, 16, 16, 16 };
static int g_teamHudY[kTeamHudMax] = { 16, 124, 232, 340, 16, 124, 232, 340 };
static int g_offClip1 = -1;
static int g_offAmmoType = -1;
static int g_offMyWeapons = -1;
static int g_offAmmo = -1;
static int g_offHanging = -1;
static int g_offReviveCount = -1;
static int g_offThirdStrike = -1;
static int g_offHealthBuffer = -1;
static DWORD g_roundT0 = 0;
static bool g_roundTiming = false;
static DWORD g_roundFrozenMs = 0;
static bool g_optCrashDialog = true;
// Local listen-host only. 30 / 0 / -1 = vanilla (do not touch the engine).
static int g_optLocalTick = 30;   // 30, 60, 100, 128
static int g_optLocalNb = 0;      // 0=auto/leave 1=0.066 2=0.024 3=0.1 vanilla
static bool g_optLocalAllow0Lerp = false;
static int g_optLocalLerp = -1;   // -1=leave, else milliseconds
static bool g_optLocalIdleSolo = false;    // go_afk_unlock: idle with 1 human
static bool g_optLocalIdleNoDelay = false; // go_afk_unlock: command works every press
static int g_optLocalAa = 10;              // sv_airaccelerate: 10=vanilla, 100/400/1000
// Listen-host play extras (isolated toggles; never touch remote sessions).
static bool g_optLocalCharChange = false;    // survivor model swap (off = no precache / no FPS cost)
// Client UX (works on listen + remote; pure client draw).
static bool g_optClientThrowLand = false;    // throwable landing marker
static std::atomic_bool g_throwLandReset{false};
static bool g_optClientInfectedHp = false;   // SI/Tank/Witch HP bar (follow model; any server)
static bool g_optClientNoCorpseSi = false;   // hide SI/Tank corpses + special ragdolls
static bool g_optClientNoCorpseCi = false;   // hide CI/Witch corpses + generic ragdolls
static bool g_optClientDmgNum = false;       // floating damage numbers (client draw; any server)
static bool g_optClientDirectorHud = false;  // pin director_show_intensity across chapters
static int g_offEffects = -1;               // CBaseEntity::m_fEffects
static int g_offOwnerEntity = -1;           // CBaseEntity::m_hOwnerEntity
static int g_offWeaponOwner = -1;           // CBaseCombatWeapon::m_hOwner
static int g_offWeaponID = -1;              // CWeaponSpawn::m_weaponID
static int g_offModelIndex = -1;            // CBaseEntity::m_nModelIndex
static int g_offRenderMode = -1;            // m_nRenderMode
static int g_offClrRender = -1;             // m_clrRender (color32)
static constexpr int kEfNoDraw = 32;        // EF_NODRAW
static constexpr int kRenderNone = 10;
static unsigned long g_hudFont = 0;
static int g_hudFontTall = 0;
static int g_hudFontLang = -1;
static unsigned long g_clockTimeFont = 0;
static int g_clockTimeFontTall = 0;
static unsigned long g_clockDateFont = 0;
static int g_clockDateFontTall = 0;
static unsigned long g_speedFontId = 0;
static int g_speedFontTall = 0;
static unsigned long g_timerFontId = 0;
static int g_timerFontTall = 0;
static unsigned long g_hudSubFont = 0;
static int g_hudSubFontTall = 0;
static unsigned long g_dmgFont = 0;
static int g_dmgFontTall = 0;
static unsigned long g_menuFont = 0;
static int g_menuFontTall = 0;
static int g_menuFontFlags = -1;
static const wchar_t* kUiAlignXNames[] = { L"靠左", L"居中", L"靠右" };
static const wchar_t* kUiAlignYNames[] = { L"靠上", L"居中", L"靠下" };
static const wchar_t* kUiBgNames[kUiBgCount] = { L"深灰", L"纯黑", L"深蓝", L"墨绿", L"酒红", L"浅灰" };
static const COLORREF kUiBgRgb[kUiBgCount] = {
	RGB(32, 32, 36), RGB(0, 0, 0), RGB(18, 38, 88),
	RGB(14, 56, 28), RGB(80, 20, 28), RGB(86, 86, 94)
};
static const wchar_t* kUiTextNames[kUiTextCount] = { L"浅灰", L"纯白", L"暖白", L"浅青", L"浅黄" };
static const COLORREF kUiTextRgb[kUiTextCount] = {
	RGB(220, 220, 220), RGB(255, 255, 255), RGB(255, 236, 210),
	RGB(180, 230, 255), RGB(255, 230, 140)
};
static const wchar_t* kUiTitleNames[kUiTitleCount] = { L"金黄", L"纯白", L"橙色", L"青色", L"跟随文字" };
static const COLORREF kUiTitleRgb[kUiTitleCount] = {
	RGB(255, 220, 100), RGB(255, 255, 255), RGB(255, 160, 60),
	RGB(80, 220, 255), RGB(220, 220, 220)
};

static COLORREF ScaleRgb(COLORREF c, int pct) {
	if (pct < 0) pct = 0;
	if (pct > 100) pct = 100;
	return RGB(GetRValue(c) * pct / 100, GetGValue(c) * pct / 100, GetBValue(c) * pct / 100);
}
static COLORREF UiBgRgb() {
	int i = (g_uiBg >= 0 && g_uiBg < kUiBgCount) ? g_uiBg : 0;
	return kUiBgRgb[i];
}
static int UiBgA() {
	int a = g_uiBgAlpha * 255 / 100;
	if (a < 40) a = 40;
	if (a > 255) a = 255;
	return a;
}
static COLORREF UiTextRgb() {
	int i = (g_uiText >= 0 && g_uiText < kUiTextCount) ? g_uiText : 0;
	return kUiTextRgb[i];
}
static COLORREF UiTitleRgb() {
	if (g_uiTitle == 4) return UiTextRgb();
	int i = (g_uiTitle >= 0 && g_uiTitle < kUiTitleCount) ? g_uiTitle : 0;
	return kUiTitleRgb[i];
}
static COLORREF UiHintRgb() { return ScaleRgb(UiTextRgb(), 62); }
static COLORREF UiMutedRgb() { return ScaleRgb(UiTextRgb(), 52); }
static COLORREF UiSectionRgb() { return ScaleRgb(UiTitleRgb(), 82); }
static COLORREF UiAccentRgb() { return UiTitleRgb(); }

static void MenuInvalidateFont() {
	g_menuFont = 0;
	g_menuFontTall = 0;
	g_menuFontFlags = -1;
}

static void ClampMenuStyle() {
	if (g_uiSizePct < 50) g_uiSizePct = 50;
	if (g_uiSizePct > 100) g_uiSizePct = 100;
	if (g_uiAlignX < 0) g_uiAlignX = 0;
	if (g_uiAlignX > 2) g_uiAlignX = 2;
	if (g_uiAlignY < 0) g_uiAlignY = 0;
	if (g_uiAlignY > 2) g_uiAlignY = 2;
	if (g_uiBgAlpha < 30) g_uiBgAlpha = 30;
	if (g_uiBgAlpha > 100) g_uiBgAlpha = 100;
	if (g_uiBg < 0) g_uiBg = 0;
	if (g_uiBg >= kUiBgCount) g_uiBg = kUiBgCount - 1;
	if (g_uiText < 0) g_uiText = 0;
	if (g_uiText >= kUiTextCount) g_uiText = kUiTextCount - 1;
	if (g_uiTitle < 0) g_uiTitle = 0;
	if (g_uiTitle >= kUiTitleCount) g_uiTitle = kUiTitleCount - 1;
	if (g_uiFont < 14) g_uiFont = 14;
	if (g_uiFont > 28) g_uiFont = 28;
	g_uiOutline = g_uiOutline ? 1 : 0;
}

static void ClampHudAbsCoord(bool abs, int* ox, int* oy, int oxLo, int oxHi) {
	if (!ox || !oy) return;
	if (abs) {
		if (*ox < 0) *ox = 0;
		if (*oy < 0) *oy = 0;
		if (*ox > 7680) *ox = 7680;
		if (*oy > 4320) *oy = 4320;
		return;
	}
	if (*ox < oxLo) *ox = oxLo;
	if (*ox > oxHi) *ox = oxHi;
	if (*oy < 0) *oy = 0;
	if (*oy > 600) *oy = 600;
}

static void ClampElimHud() {
	if (g_elimMode < 0) g_elimMode = 0;
	if (g_elimMode > 2) g_elimMode = 2;
	if (g_elimAlign < 0) g_elimAlign = 0;
	if (g_elimAlign > 3) g_elimAlign = 3;
	ClampHudAbsCoord(g_elimAbs, &g_elimOffX, &g_elimOffY, 0, 600);
	if (g_elimFont < kElimFontMin) g_elimFont = kElimFontMin;
	if (g_elimFont > kElimFontMax) g_elimFont = kElimFontMax;
	g_elimLang = g_elimLang ? 1 : 0;
	g_elimShow &= kElimShowAll;
	if (g_elimShow == kElimShowCompact) g_elimCompact = 1;
	else if (g_elimShow == kElimShowAll) g_elimCompact = 0;
}

static void ClampClockHud() {
	if (g_clockAlign < 0) g_clockAlign = 0;
	if (g_clockAlign > 3) g_clockAlign = 3;
	ClampHudAbsCoord(g_clockAbs, &g_clockOffX, &g_clockOffY, 0, 600);
	if (g_clockFont < kElimFontMin) g_clockFont = kElimFontMin;
	if (g_clockFont > kElimFontMax) g_clockFont = kElimFontMax;
}

static void ClockInvalidateFont() {
	g_clockTimeFont = 0;
	g_clockTimeFontTall = 0;
	g_clockDateFont = 0;
	g_clockDateFontTall = 0;
}

static void HudPlacePanel(int sw, int sh, int pw, int ph, int align, int ox, int oy, int* x, int* y);
static bool SurfGetScreenSize(int* sw, int* sh);

static void ClockResetPos() {
	g_clockAlign = 1;
	g_clockOffX = 16;
	g_clockOffY = 16;
	g_clockAbs = false;
}

static void SpeedResetPos() {
	g_speedAlign = 5;
	g_speedOffX = 0;
	g_speedOffY = 48;
	g_speedAbs = false;
}
static void TimerResetPos() {
	g_timerAlign = 4;
	g_timerOffX = 0;
	g_timerOffY = 10;
	g_timerAbs = false;
}

static void TeamHudResetPos() {
	for (int i = 0; i < kTeamHudMax; ++i) {
		g_teamHudX[i] = 16;
		g_teamHudY[i] = 16 + (i % 4) * 108;
	}
}

static void HudPlaceAbs(int sw, int sh, int pw, int ph, int ox, int oy, int* x, int* y) {
	int px = ox, py = oy;
	if (px < 0) px = 0;
	if (py < 0) py = 0;
	if (pw > 0 && px + pw > sw) px = sw - pw;
	if (ph > 0 && py + ph > sh) py = sh - ph;
	if (px < 0) px = 0;
	if (py < 0) py = 0;
	*x = px;
	*y = py;
}

static void HudFinalizePos(bool* abs, int* ox, int* oy, int align, int sw, int sh, int pw, int ph, int* x, int* y) {
	if (!abs || !ox || !oy || !x || !y) return;
	if (!*abs) {
		HudPlacePanel(sw, sh, pw, ph, align, *ox, *oy, x, y);
		*ox = *x;
		*oy = *y;
		*abs = true;
		g_settingsDirty = true;
		g_settingsSaveAt = GetTickCount() + 800;
		return;
	}
	HudPlaceAbs(sw, sh, pw, ph, *ox, *oy, x, y);
}

static void HudNudgeAbs(bool* abs, int* ox, int* oy, int align, int dx, int dy) {
	if (!dx && !dy) return;
	if (abs && ox && oy && !*abs) {
		int sw = 0, sh = 0;
		if (SurfGetScreenSize(&sw, &sh)) {
			int x = 0, y = 0;
			HudPlacePanel(sw, sh, 240, 72, align, *ox, *oy, &x, &y);
			*ox = x;
			*oy = y;
			*abs = true;
		}
	}
	if (ox) *ox += dx;
	if (oy) *oy += dy;
	if (ox && *ox < 0) *ox = 0;
	if (oy && *oy < 0) *oy = 0;
	if (ox && *ox > 7680) *ox = 7680;
	if (oy && *oy > 4320) *oy = 4320;
	g_settingsDirty = true;
	g_settingsSaveAt = GetTickCount() + 400;
}

static const wchar_t* HudAlignName6(int a) {
	static const wchar_t* k[] = { L"左上", L"右上", L"左下", L"右下", L"上中", L"下中" };
	if (a < 0 || a > 5) return k[0];
	return k[a];
}

static void ClampHud6(int* align, int* ox, int* oy, int* font, bool abs) {
	if (*align < 0) *align = 0;
	if (*align > 5) *align = 5;
	const bool mid = (*align == 4 || *align == 5);
	ClampHudAbsCoord(abs, ox, oy, mid ? -400 : 0, mid ? 400 : 600);
	if (*font < kElimFontMin) *font = kElimFontMin;
	if (*font > kElimFontMax) *font = kElimFontMax;
}

static void ClampSpeedHud() { ClampHud6(&g_speedAlign, &g_speedOffX, &g_speedOffY, &g_speedFont, g_speedAbs); }
static void ClampTimerHud() { ClampHud6(&g_timerAlign, &g_timerOffX, &g_timerOffY, &g_timerFont, g_timerAbs); }

static void SpeedInvalidateFont() { g_speedFontId = 0; g_speedFontTall = 0; }
static void TimerInvalidateFont() { g_timerFontId = 0; g_timerFontTall = 0; }

static void HudPlacePanel(int sw, int sh, int pw, int ph, int align, int ox, int oy, int* x, int* y) {
	int px = ox, py = oy;
	if (align == 4 || align == 5)
		px = (sw - pw) / 2 + ox;
	else if (align == 1 || align == 3)
		px = sw - pw - ox;
	if (align == 2 || align == 3 || align == 5)
		py = sh - ph - oy;
	if (px < 0) px = 0;
	if (py < 0) py = 0;
	if (px + pw > sw) px = sw - pw;
	if (py + ph > sh) py = sh - ph;
	*x = px;
	*y = py;
}

static void HudNudge6(int align, int* ox, int* oy, int dx, int dy) {
	if (!dx && !dy) return;
	if (align == 1 || align == 3) dx = -dx;
	if (align == 2 || align == 3 || align == 5) dy = -dy;
	const bool mid = (align == 4 || align == 5);
	const int oxLo = mid ? -400 : 0;
	const int oxHi = mid ? 400 : 600;
	int nx = *ox + dx;
	int ny = *oy + dy;
	if (nx < oxLo) nx = oxLo;
	if (nx > oxHi) nx = oxHi;
	if (ny < 0) ny = 0;
	if (ny > 600) ny = 600;
	*ox = nx;
	*oy = ny;
	g_settingsDirty = true;
	g_settingsSaveAt = GetTickCount() + 400;
}

static bool ElimLineOn(int i) {
	return i >= 0 && i < kElimLineCount && (g_elimShow & (1 << i)) != 0;
}

static void ElimSetLine(int i, bool on) {
	if (i < 0 || i >= kElimLineCount) return;
	if (on) g_elimShow |= (1 << i);
	else g_elimShow &= ~(1 << i);
	g_elimShow &= kElimShowAll;
}

static void ElimApplyPreset(bool compact) {
	g_elimCompact = compact ? 1 : 0;
	g_elimShow = g_elimCompact ? kElimShowCompact : kElimShowAll;
}

static const wchar_t* ElimPresetName() {
	if (g_elimShow == kElimShowAll) return L"完整";
	if (g_elimShow == kElimShowCompact) return L"精简";
	return L"自定义";
}

static void ElimInvalidateFont() {
	g_hudFont = 0;
	g_hudFontTall = 0;
	g_hudFontLang = -1;
	g_dmgFont = 0;
	g_dmgFontTall = 0;
}

static void ElimResetCounts() {
	g_elimSi.store(0, std::memory_order_relaxed);
	g_elimCi.store(0, std::memory_order_relaxed);
	g_elimSiMelee.store(0, std::memory_order_relaxed);
	g_elimSkeet.store(0, std::memory_order_relaxed);
	g_elimMeleeSkeet.store(0, std::memory_order_relaxed);
	g_elimHs.store(0, std::memory_order_relaxed);
}

static const wchar_t* ElimModeLabel() {
	if (g_elimMode == 1) return L"章节不重置";
	if (g_elimMode == 2) return L"章节重置、团灭清零";
	return L"章节重置、团灭不清零";
}

static void ElimCycleMode() {
	// Menu order: chapter keep-wipe → chapter+wipe → persist
	if (g_elimMode == 0) g_elimMode = 2;
	else if (g_elimMode == 2) g_elimMode = 1;
	else g_elimMode = 0;
}

static void ElimRememberMap(const char* mapName) {
	if (!mapName || !mapName[0]) {
		g_elimLastMap[0] = 0;
		return;
	}
	strncpy(g_elimLastMap, mapName, sizeof(g_elimLastMap) - 1);
	g_elimLastMap[sizeof(g_elimLastMap) - 1] = 0;
}

// Chapter load / same-map wipe reload. Mode 1 never clears here.
// Mode 0 skips a same-map LevelInit so campaign wipes do not look like a new chapter.
static void ElimOnChapterLoad(const char* mapName) {
	if (g_elimMode == 1) {
		ElimRememberMap(mapName);
		return;
	}
	if (g_elimMode == 2) {
		ElimResetCounts();
		ElimRememberMap(mapName);
		return;
	}
	const bool sameMap = mapName && mapName[0] && g_elimLastMap[0]
		&& _stricmp(g_elimLastMap, mapName) == 0;
	if (!sameMap)
		ElimResetCounts();
	ElimRememberMap(mapName);
}

static void ElimOnMissionLost() {
	if (g_elimMode == 2)
		ElimResetCounts();
}

static void MenuStyleReset() {
	g_uiSizePct = 100;
	g_uiAlignX = 0;
	g_uiAlignY = 1;
	g_uiBgAlpha = 90;
	g_uiBg = 0;
	g_uiText = 0;
	g_uiTitle = 0;
	g_uiFont = 20;
	g_uiOutline = 1;
	MenuInvalidateFont();
}
static char g_gameL4d2Dir[MAX_PATH]{}; // .../Left 4 Dead 2/left4dead2
// First-run: issue bind [ "skeeto_menu" once (existing ini never overwritten).
static bool g_needDefaultMenuBind = false;
static bool g_menuDefaultBindDone = false;

static void Log(const char* fmt, ...) {
	if (!g_log) return;
	va_list ap; va_start(ap, fmt);
	vfprintf(g_log, fmt, ap);
	fputc('\n', g_log);
	// Always flush early boot lines so crashes leave a trail; later throttle.
	static int s_logLines = 0;
	++s_logLines;
	if (s_logLines <= 64 || (s_logLines & 15) == 0)
		fflush(g_log);
	va_end(ap);
}

static void SettingsFilePath(char* out, size_t n) {
	char dir[MAX_PATH]{};
	if (g_self)
		GetModuleFileNameA(g_self, dir, MAX_PATH);
	char* slash = strrchr(dir, '\\');
	if (slash) *slash = 0;
	else dir[0] = 0;
	snprintf(out, n, "%s\\skeeto_setting.ini", dir[0] ? dir : ".");
}

static void ResolveGameDir() {
	g_gameL4d2Dir[0] = 0;
	char mod[MAX_PATH]{};
	if (g_self)
		GetModuleFileNameA(g_self, mod, MAX_PATH);
	// necola.dll lives in game root: .../Left 4 Dead 2/necola.dll
	char* slash = strrchr(mod, '\\');
	if (slash) *slash = 0;
	if (!mod[0]) return;
	snprintf(g_gameL4d2Dir, sizeof(g_gameL4d2Dir), "%s\\left4dead2", mod);
}

static bool SettingsLineIsKnown(const char* line) {
	while (line && (*line == ' ' || *line == '\t')) ++line;
	if (!line || !line[0] || line[0] == '\n' || line[0] == '\r') return true;
	if (line[0] == '#' || line[0] == ';' || (line[0] == '/' && line[1] == '/'))
		return false;
	static const char* k[] = {
		"sound=", "sfx_volume=", "icon=", "hit_mode=", "hit_feedback=",
		"ci_style_id=", "si_dedicated=", "si_sound=", "si_style_id=",
		"ff_feedback=", "ff_style_id=", "kill_fx=", "fx_style_id=",
		"pt_disp_warm=", "menu_", "xhair", "ci_style=", "si_style=",
		"elim=", "elim_", "crash_dialog=",
		"local_tick=", "local_nb=", "local_lerp=", "local_allow0lerp=",
		"local_idle_solo=", "local_idle_nodelay=",
		"local_aa=",
		"local_char_change=",
		"client_throw_land=", "client_infected_hp=", "client_no_corpse_si=", "client_no_corpse_ci=",
		"client_loot_beam=", // removed — drop old ini lines
		"client_dmg_num=",
		// removed / migrated — treat as known so old lines are dropped, not re-appended
		"local_infected_hp=",
		"local_ai_dmg=", "local_melee_unlock=", "local_saferoom_melee=",
		nullptr
	};
	for (int i = 0; k[i]; ++i) {
		if (!_strnicmp(line, k[i], (int)strlen(k[i]))) return true;
	}
	return false;
}

static void SaveSettings() {
	if (g_inKeyHook) {
		g_settingsDirty = true;
		g_settingsSaveAt = GetTickCount() + 400;
		return;
	}
	CrashMark(kBcSettings);
	char path[MAX_PATH]{};
	SettingsFilePath(path, sizeof(path));
	char extra[4096]{};
	{
		FILE* old = fopen(path, "r");
		if (old) {
			char line[192]{};
			size_t used = 0;
			while (fgets(line, sizeof(line), old)) {
				if (SettingsLineIsKnown(line)) continue;
				size_t n = strlen(line);
				if (used + n + 1 >= sizeof(extra)) break;
				memcpy(extra + used, line, n);
				used += n;
				extra[used] = 0;
			}
			fclose(old);
		}
	}
	FILE* f = fopen(path, "w");
	if (!f) {
		Log("settings: save failed (%s)", path);
		return;
	}
	TexXhairRememberCurrent();
	const char* ci = DlcGetSelected("ci");
	const char* si = DlcGetSelected("si");
	const char* ff = DlcGetSelected("ff");
	const char* fx = DlcGetSelected("fx");
	fprintf(f,
		"sound=%d\n"
		"sfx_volume=%d\n"
		"icon=%d\n"
		"hit_mode=%d\n"
		"ci_style_id=%s\n"
		"si_dedicated=%d\n"
		"si_sound=%d\n"
		"si_style_id=%s\n"
		"ff_feedback=%d\n"
		"ff_style_id=%s\n"
		"kill_fx=%d\n"
		"fx_style_id=%s\n"
		"pt_disp_warm=%d\n"
		"menu_default_bind=%d\n"
		"menu_size=%d\n"
		"menu_align_x=%d\n"
		"menu_align_y=%d\n"
		"menu_bg_alpha=%d\n"
		"menu_bg=%d\n"
		"menu_text=%d\n"
		"menu_title=%d\n"
		"menu_font=%d\n"
		"menu_outline=%d\n"
		"elim=%d\n"
		"elim_mode=%d\n"
		"elim_align=%d\n"
		"elim_offset_x=%d\n"
		"elim_offset_y=%d\n"
		"elim_font=%d\n"
		"elim_lang=%d\n"
		"elim_compact=%d\n"
		"elim_show=%d\n"
		"clock=%d\n"
		"clock_align=%d\n"
		"clock_offset_x=%d\n"
		"clock_offset_y=%d\n"
		"clock_font=%d\n"
		"hud_hide=%d\n"
		"hud_hide_team=%d\n"
		"hud_hide_wep=%d\n"
		"hud_hide_pickup=%d\n"
		"speed=%d\n"
		"speed_align=%d\n"
		"speed_offset_x=%d\n"
		"speed_offset_y=%d\n"
		"speed_font=%d\n"
		"timer=%d\n"
		"timer_align=%d\n"
		"timer_offset_x=%d\n"
		"timer_offset_y=%d\n"
		"timer_font=%d\n"
		"crash_dialog=%d\n"
		"local_tick=%d\n"
		"local_nb=%d\n"
		"local_allow0lerp=%d\n"
		"local_lerp=%d\n"
		"local_idle_solo=%d\n"
		"local_idle_nodelay=%d\n"
		"local_aa=%d\n"
		"local_char_change=%d\n"
		"client_throw_land=%d\n"
		"client_infected_hp=%d\n"
		"client_no_corpse_si=%d\n"
		"client_no_corpse_ci=%d\n"
		"client_dmg_num=%d\n"
		"client_director_hud=%d\n"
		"xhair=%d\n"
		"xhair_ring=%d\n"
		"xhair_ring_color=%d\n"
		"xhair_ring_mode=%d\n"
		"xhair_ring_alpha=%d\n"
		"xhair_style=%d\n"
		"xhair_tex=%d\n"
		"xhair_tex_full=%d\n"
		"xhair_tex_size=%d\n"
		"xhair_tex_mat=%s\n",
		g_optSound ? 1 : 0,
		g_optSfxVol,
		g_optIcon ? 1 : 0,
		g_optHitMode,
		(ci && ci[0]) ? ci : "off",
		g_optSiVisual ? 1 : 0,
		g_optSiSound ? 1 : 0,
		(si && si[0]) ? si : "off",
		g_optFf ? 1 : 0,
		(ff && ff[0]) ? ff : "off",
		g_optKillFx ? 1 : 0,
		(fx && fx[0]) ? fx : "off",
		g_optPtDispWarm ? 1 : 0,
		g_menuDefaultBindDone ? 1 : 0,
		g_uiSizePct,
		g_uiAlignX,
		g_uiAlignY,
		g_uiBgAlpha,
		g_uiBg,
		g_uiText,
		g_uiTitle,
		g_uiFont,
		g_uiOutline ? 1 : 0,
		g_optElim ? 1 : 0,
		g_elimMode,
		g_elimAlign,
		g_elimOffX,
		g_elimOffY,
		g_elimFont,
		g_elimLang,
		g_elimCompact,
		g_elimShow,
		g_optClock ? 1 : 0,
		g_clockAlign,
		g_clockOffX,
		g_clockOffY,
		g_clockFont,
		(g_optHudHideTeam || g_optHudHideWep || g_optHudHidePickup) ? 1 : 0,
		g_optHudHideTeam ? 1 : 0,
		g_optHudHideWep ? 1 : 0,
		g_optHudHidePickup ? 1 : 0,
		g_optSpeed ? 1 : 0,
		g_speedAlign,
		g_speedOffX,
		g_speedOffY,
		g_speedFont,
		g_optTimer ? 1 : 0,
		g_timerAlign,
		g_timerOffX,
		g_timerOffY,
		g_timerFont,
		g_optCrashDialog ? 1 : 0,
		g_optLocalTick,
		g_optLocalNb,
		g_optLocalAllow0Lerp ? 1 : 0,
		g_optLocalLerp,
		g_optLocalIdleSolo ? 1 : 0,
		g_optLocalIdleNoDelay ? 1 : 0,
		g_optLocalAa,
		g_optLocalCharChange ? 1 : 0,
		g_optClientThrowLand ? 1 : 0,
		g_optClientInfectedHp ? 1 : 0,
		g_optClientNoCorpseSi ? 1 : 0,
		g_optClientNoCorpseCi ? 1 : 0,
		g_optClientDmgNum ? 1 : 0,
		g_optClientDirectorHud ? 1 : 0,
		g_optXhair ? 1 : 0,
		g_optXhairRing ? 1 : 0,
		g_xhairRingColor,
		g_xhairRingMode,
		g_xhairRingAlpha,
		g_xhairStyle,
		g_optXhairTex ? 1 : 0,
		g_optXhairTexFull ? 1 : 0,
		g_xhairTexSize,
		g_xhairTexMat[0] ? g_xhairTexMat : "");
	fprintf(f,
		"elim_abs=%d\n"
		"clock_abs=%d\n"
		"speed_abs=%d\n"
		"timer_abs=%d\n"
		"teamhud=%d\n"
		"teamhud_font=%d\n"
		"teamhud_sel=%d\n"
		"teamhud_x=%d,%d,%d,%d,%d,%d,%d,%d\n"
		"teamhud_y=%d,%d,%d,%d,%d,%d,%d,%d\n",
		g_elimAbs ? 1 : 0,
		g_clockAbs ? 1 : 0,
		g_speedAbs ? 1 : 0,
		g_timerAbs ? 1 : 0,
		g_optTeamHud ? 1 : 0,
		g_teamHudFont,
		g_teamHudSel,
		g_teamHudX[0], g_teamHudX[1], g_teamHudX[2], g_teamHudX[3],
		g_teamHudX[4], g_teamHudX[5], g_teamHudX[6], g_teamHudX[7],
		g_teamHudY[0], g_teamHudY[1], g_teamHudY[2], g_teamHudY[3],
		g_teamHudY[4], g_teamHudY[5], g_teamHudY[6], g_teamHudY[7]);
	for (int i = 0; i < kXhairStyleCount; ++i) {
		const XhairTune& t = g_xhairTune[i];
		fprintf(f, "xhair_p%d=%d,%d,%d,%d,%d,%d,%d,%d\n",
			i, t.color, t.size, t.length, t.gap, t.thick, t.dot, t.outline, t.alpha);
	}
	for (int i = 0; i < g_texXhairTuneCount; ++i) {
		const TexXhairTune& t = g_texXhairTune[i];
		if (!t.mat[0] || TexXhairIsVanillaTest(t.mat)) continue;
		fprintf(f, "xhair_tex_t%d=%s,%d,%d\n", i, t.mat, t.size, t.full ? 1 : 0);
	}
	if (extra[0])
		fputs(extra, f);
	fclose(f);
	Log("settings: saved ci=%s si=%s ff=%d/%s sound=%d sfxVol=%d icon=%d hitMode=%d siVis=%d siSnd=%d killFx=%d fx=%s ptDispWarm=%d xhair=%d ring=%d",
		(ci && ci[0]) ? ci : "off", (si && si[0]) ? si : "off",
		g_optFf ? 1 : 0, (ff && ff[0]) ? ff : "off",
		g_optSound ? 1 : 0, g_optSfxVol, g_optIcon ? 1 : 0, g_optHitMode,
		g_optSiVisual ? 1 : 0, g_optSiSound ? 1 : 0, g_optKillFx ? 1 : 0,
		(fx && fx[0]) ? fx : "off", g_optPtDispWarm ? 1 : 0, g_optXhair ? 1 : 0, g_optXhairRing ? 1 : 0);
}

static void PumpDeferredSettings() {
	if (!g_settingsDirty) return;
	if (GetTickCount() < g_settingsSaveAt) return;
	g_settingsDirty = false;
	SaveSettings();
}

static void MigrateLegacyStyleInts(int ciStyle, int siStyle) {
	// Old: ci 0=off 1=ow 2=apex 3=cf 4=cod 5=l4d2; si 0=cf 1=valorant
	static const char* kCi[] = { "off", "ci_ow", "ci_apex", "ci_cf", "ci_cod", "ci_l4d2" };
	if (ciStyle >= 0 && ciStyle <= 5)
		DlcSetSelected("ci", kCi[ciStyle]);
	DlcSetSelected("si", siStyle == 0 ? "si_cf" : "si_valorant");
}

static void ClampSfxVol() {
	if (g_optSfxVol < 0) g_optSfxVol = 0;
	if (g_optSfxVol > 100) g_optSfxVol = 100;
}

static void ClampXhairTune(XhairTune& t) {
	if (t.color < 0) t.color = 0;
	if (t.color >= kXhairColorCount) t.color = kXhairColorCount - 1;
	if (t.size < 50) t.size = 50;
	if (t.size > 200) t.size = 200;
	if (t.length < 2) t.length = 2;
	if (t.length > 24) t.length = 24;
	if (t.gap < 0) t.gap = 0;
	if (t.gap > 16) t.gap = 16;
	if (t.thick < 1) t.thick = 1;
	if (t.thick > 8) t.thick = 8;
	if (t.dot < 0) t.dot = 0;
	if (t.dot > 12) t.dot = 12;
	if (t.outline < 0) t.outline = 0;
	if (t.outline > 4) t.outline = 4;
	if (t.alpha < 10) t.alpha = 10;
	if (t.alpha > 100) t.alpha = 100;
}

static void ClampXhair() {
	if (g_xhairStyle < 0) g_xhairStyle = 0;
	if (g_xhairStyle >= kXhairStyleCount) g_xhairStyle = kXhairStyleCount - 1;
	if (g_xhairRingColor < 0) g_xhairRingColor = 0;
	if (g_xhairRingColor >= kXhairColorCount) g_xhairRingColor = kXhairColorCount - 1;
	if (g_xhairRingMode != 0 && g_xhairRingMode != 2)
		g_xhairRingMode = 2;
	if (g_xhairRingAlpha < 10) g_xhairRingAlpha = 10;
	if (g_xhairRingAlpha > 100) g_xhairRingAlpha = 100;
	if (g_xhairTexSize < kTexXhairSizeMin) g_xhairTexSize = kTexXhairSizeMin;
	if (g_xhairTexSize > kTexXhairSizeMax) g_xhairTexSize = kTexXhairSizeMax;
	for (int i = 0; i < g_texXhairTuneCount; ++i) {
		if (g_texXhairTune[i].size < kTexXhairSizeMin) g_texXhairTune[i].size = kTexXhairSizeMin;
		if (g_texXhairTune[i].size > kTexXhairSizeMax) g_texXhairTune[i].size = kTexXhairSizeMax;
	}
	for (int i = 0; i < kXhairStyleCount; ++i)
		ClampXhairTune(g_xhairTune[i]);
}

static int CycleWrap(int v, int lo, int hi) {
	int n = v + 1;
	if (n > hi) n = lo;
	return n;
}

static int CycleStep(int v, int lo, int hi, int step) {
	int n = v + step;
	if (n > hi) n = lo;
	return n;
}

static int ClampAdd(int v, int d, int lo, int hi) {
	v += d;
	if (v < lo) v = lo;
	if (v > hi) v = hi;
	return v;
}

// dx/dy are screen-space (right/down positive).
static void ElimNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_elimAbs, &g_elimOffX, &g_elimOffY, g_elimAlign, dx, dy);
}

static void ClockNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_clockAbs, &g_clockOffX, &g_clockOffY, g_clockAlign, dx, dy);
}

static void SpeedNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_speedAbs, &g_speedOffX, &g_speedOffY, g_speedAlign, dx, dy);
}

static void TimerNudgeScreen(int dx, int dy) {
	HudNudgeAbs(&g_timerAbs, &g_timerOffX, &g_timerOffY, g_timerAlign, dx, dy);
}

static void TeamHudNudgeScreen(int dx, int dy) {
	if (g_teamHudSel < 0) g_teamHudSel = 0;
	if (g_teamHudSel >= kTeamHudMax) g_teamHudSel = kTeamHudMax - 1;
	HudNudgeAbs(nullptr, &g_teamHudX[g_teamHudSel], &g_teamHudY[g_teamHudSel], 0, dx, dy);
}

static void ClampListenOpts() {
	if (g_optLocalTick != 30 && g_optLocalTick != 60 && g_optLocalTick != 100 && g_optLocalTick != 128)
		g_optLocalTick = 30;
	if (g_optLocalNb < 0) g_optLocalNb = 0;
	if (g_optLocalNb > 3) g_optLocalNb = 3;
	if (g_optLocalLerp != -1 && g_optLocalLerp != 0 && g_optLocalLerp != 24
		&& g_optLocalLerp != 40 && g_optLocalLerp != 60 && g_optLocalLerp != 100)
		g_optLocalLerp = -1;
	if (g_optLocalAa != 10 && g_optLocalAa != 100 && g_optLocalAa != 400 && g_optLocalAa != 1000)
		g_optLocalAa = 10;
}

static void LoadSettings() {
	// Switches: first-run / missing-key baseline only (never rewrite over a loaded ini key).
	g_optSound = true;
	g_optSfxVol = 100;
	g_optIcon = true;
	g_optHitMode = 1; // 仅特感 Hit（无 ini 时）
	g_optSiVisual = true;
	g_optSiSound = true;
	g_optKillFx = true;
	g_optFf = false;
	g_optPtDispWarm = false;
	g_optXhair = false;
	g_optXhairRing = false;
	g_optXhairTex = false;
	g_optXhairTexFull = false;
	g_xhairTexSize = 80;
	g_xhairTexMat[0] = 0;
	g_texXhairTuneCount = 0;
	memset(g_texXhairTune, 0, sizeof(g_texXhairTune));
	g_optElim = true;
	g_elimMode = 0;
	g_elimAlign = 0;
	g_elimAbs = false;
	g_elimOffX = 16;
	g_elimOffY = 16;
	g_elimFont = 22;
	g_elimLang = 0;
	g_elimCompact = 0;
	g_elimShow = kElimShowAll;
	g_optClock = true;
	g_clockAlign = 1;
	g_clockAbs = false;
	g_clockOffX = 16;
	g_clockOffY = 16;
	g_clockFont = 26;
	g_optHudHide = false;
	g_optHudHideTeam = false;
	g_optHudHideWep = false;
	g_optHudHidePickup = false;
	g_optSpeed = true;
	g_speedAlign = 5;
	g_speedAbs = false;
	g_speedOffX = 0;
	g_speedOffY = 48;
	g_speedFont = 36;
	g_optTimer = true;
	g_timerAlign = 4;
	g_timerAbs = false;
	g_timerOffX = 0;
	g_timerOffY = 10;
	g_timerFont = 28;
	g_optTeamHud = true;
	g_teamHudFont = 18;
	g_teamHudSel = 0;
	TeamHudResetPos();
	g_optCrashDialog = true;
	g_optLocalTick = 30;
	g_optLocalNb = 0;
	g_optLocalAllow0Lerp = false;
	g_optLocalLerp = -1;
	g_optLocalIdleSolo = false;
	g_optLocalIdleNoDelay = false;
	g_optLocalAa = 10;
	g_optLocalCharChange = false;
	g_optClientThrowLand = false;
	g_optClientInfectedHp = false;
	g_optClientNoCorpseSi = false;
	g_optClientNoCorpseCi = false;
	g_optClientDmgNum = false;
	g_optClientDirectorHud = false;
	g_xhairRingColor = 1;
	g_xhairRingMode = 2;
	g_xhairRingAlpha = 70;
	g_xhairStyle = 1;
	XhairLoadFactoryAll();
	// Styles: no preset pack — empty = off. Never force ci_ow / si_valorant / etc.
	DlcSetSelected("ci", "off");
	DlcSetSelected("si", "off");
	DlcSetSelected("ff", "off");
	DlcSetSelected("fx", "off");

	char path[MAX_PATH]{};
	SettingsFilePath(path, sizeof(path));
	FILE* f = fopen(path, "r");
	bool migratedLegacySettings = false;
	// Migrate legacy Necola DLC / HSTM settings filename once.
	if (!f) {
		char dir[MAX_PATH]{};
		if (g_self) GetModuleFileNameA(g_self, dir, MAX_PATH);
		char* slash = strrchr(dir, '\\');
		if (slash) *slash = 0;
		char legacy[MAX_PATH]{};
		snprintf(legacy, sizeof(legacy), "%s\\hstm_killfeed_settings.ini", dir[0] ? dir : ".");
		f = fopen(legacy, "r");
		if (f) {
			migratedLegacySettings = true;
			Log("settings: migrating from %s", legacy);
		}
	}
	if (!f) {
		Log("settings: no file, styles off -> %s", path);
		g_needDefaultMenuBind = true;
		g_menuDefaultBindDone = false;
		SaveSettings();
		return;
	}
	char line[192]{};
	int legacyCi = -1, legacySi = -1;
	bool hadCiId = false, hadSiId = false;
	bool hadHitMode = false;
	bool hadElim = false;
	bool hadElimShow = false;
	bool hadHudHideParts = false;
	bool hadXhairPer = false;
	bool hadXhairOldShare = false;
	XhairTune xhairOldShare = kXhairFactory[1];
	while (fgets(line, sizeof(line), f)) {
		int v = 0;
		int tSize = 0, tFull = 0;
		char id[64]{};
		char tMat[80]{};
		if (sscanf(line, "sound=%d", &v) == 1) g_optSound = v != 0;
		else if (sscanf(line, "sfx_volume=%d", &v) == 1) {
			g_optSfxVol = v;
			ClampSfxVol();
		}
		else if (sscanf(line, "icon=%d", &v) == 1) g_optIcon = v != 0;
		else if (sscanf(line, "hit_mode=%d", &v) == 1) {
			if (v < 0) v = 0;
			if (v > 2) v = 2;
			g_optHitMode = v;
			hadHitMode = true;
		} else if (!hadHitMode && sscanf(line, "hit_feedback=%d", &v) == 1) {
			g_optHitMode = (v != 0) ? 2 : 0;
		}
		else if (sscanf(line, "ci_style_id=%63s", id) == 1) {
			DlcSetSelected("ci", id);
			hadCiId = true;
		} else if (sscanf(line, "si_style_id=%63s", id) == 1) {
			DlcSetSelected("si", id);
			hadSiId = true;
		} else if (sscanf(line, "ff_style_id=%63s", id) == 1)
			DlcSetSelected("ff", id);
		else if (sscanf(line, "fx_style_id=%63s", id) == 1)
			DlcSetSelected("fx", id);
		else if (sscanf(line, "ci_style=%d", &v) == 1) legacyCi = v;
		else if (sscanf(line, "si_dedicated=%d", &v) == 1) g_optSiVisual = v != 0;
		else if (sscanf(line, "si_sound=%d", &v) == 1) g_optSiSound = v != 0;
		else if (sscanf(line, "si_style=%d", &v) == 1) legacySi = (v == 0) ? 0 : 1;
		else if (sscanf(line, "ff_feedback=%d", &v) == 1) g_optFf = v != 0;
		else if (sscanf(line, "kill_fx=%d", &v) == 1) g_optKillFx = v != 0;
		else if (sscanf(line, "pt_disp_warm=%d", &v) == 1) g_optPtDispWarm = v != 0;
		else if (sscanf(line, "menu_default_bind=%d", &v) == 1)
			g_menuDefaultBindDone = v != 0;
		else if (sscanf(line, "menu_size=%d", &v) == 1) g_uiSizePct = v;
		else if (sscanf(line, "menu_align_x=%d", &v) == 1) g_uiAlignX = v;
		else if (sscanf(line, "menu_align_y=%d", &v) == 1) g_uiAlignY = v;
		else if (sscanf(line, "menu_bg_alpha=%d", &v) == 1) g_uiBgAlpha = v;
		else if (sscanf(line, "menu_bg=%d", &v) == 1) g_uiBg = v;
		else if (sscanf(line, "menu_text=%d", &v) == 1) g_uiText = v;
		else if (sscanf(line, "menu_title=%d", &v) == 1) g_uiTitle = v;
		else if (sscanf(line, "menu_font=%d", &v) == 1) g_uiFont = v;
		else if (sscanf(line, "menu_outline=%d", &v) == 1) g_uiOutline = v != 0;
		else if (sscanf(line, "elim=%d", &v) == 1) { g_optElim = v != 0; hadElim = true; }
		else if (sscanf(line, "elim_mode=%d", &v) == 1) { g_elimMode = v; hadElim = true; }
		else if (sscanf(line, "elim_align=%d", &v) == 1) { g_elimAlign = v; hadElim = true; }
		else if (sscanf(line, "elim_abs=%d", &v) == 1) g_elimAbs = v != 0;
		else if (sscanf(line, "elim_offset_x=%d", &v) == 1) { g_elimOffX = v; hadElim = true; }
		else if (sscanf(line, "elim_offset_y=%d", &v) == 1) { g_elimOffY = v; hadElim = true; }
		else if (sscanf(line, "elim_font=%d", &v) == 1) { g_elimFont = v; hadElim = true; }
		else if (sscanf(line, "elim_lang=%d", &v) == 1) { g_elimLang = v; hadElim = true; }
		else if (sscanf(line, "elim_compact=%d", &v) == 1) { g_elimCompact = v; hadElim = true; }
		else if (sscanf(line, "elim_show=%d", &v) == 1) { g_elimShow = v; hadElim = true; hadElimShow = true; }
		else if (sscanf(line, "clock=%d", &v) == 1) g_optClock = v != 0;
		else if (sscanf(line, "clock_align=%d", &v) == 1) g_clockAlign = v;
		else if (sscanf(line, "clock_abs=%d", &v) == 1) g_clockAbs = v != 0;
		else if (sscanf(line, "clock_offset_x=%d", &v) == 1) g_clockOffX = v;
		else if (sscanf(line, "clock_offset_y=%d", &v) == 1) g_clockOffY = v;
		else if (sscanf(line, "clock_font=%d", &v) == 1) g_clockFont = v;
		else if (sscanf(line, "hud_hide=%d", &v) == 1) g_optHudHide = v != 0;
		else if (sscanf(line, "hud_hide_team=%d", &v) == 1) { g_optHudHideTeam = v != 0; hadHudHideParts = true; }
		else if (sscanf(line, "hud_hide_wep=%d", &v) == 1) { g_optHudHideWep = v != 0; hadHudHideParts = true; }
		else if (sscanf(line, "hud_hide_pickup=%d", &v) == 1) { g_optHudHidePickup = v != 0; hadHudHideParts = true; }
		else if (sscanf(line, "speed=%d", &v) == 1) g_optSpeed = v != 0;
		else if (sscanf(line, "speed_align=%d", &v) == 1) g_speedAlign = v;
		else if (sscanf(line, "speed_abs=%d", &v) == 1) g_speedAbs = v != 0;
		else if (sscanf(line, "speed_offset_x=%d", &v) == 1) g_speedOffX = v;
		else if (sscanf(line, "speed_offset_y=%d", &v) == 1) g_speedOffY = v;
		else if (sscanf(line, "speed_font=%d", &v) == 1) g_speedFont = v;
		else if (sscanf(line, "timer=%d", &v) == 1) g_optTimer = v != 0;
		else if (sscanf(line, "timer_align=%d", &v) == 1) g_timerAlign = v;
		else if (sscanf(line, "timer_abs=%d", &v) == 1) g_timerAbs = v != 0;
		else if (sscanf(line, "timer_offset_x=%d", &v) == 1) g_timerOffX = v;
		else if (sscanf(line, "timer_offset_y=%d", &v) == 1) g_timerOffY = v;
		else if (sscanf(line, "timer_font=%d", &v) == 1) g_timerFont = v;
		else if (sscanf(line, "teamhud=%d", &v) == 1) g_optTeamHud = v != 0;
		else if (sscanf(line, "teamhud_font=%d", &v) == 1) g_teamHudFont = v;
		else if (sscanf(line, "teamhud_sel=%d", &v) == 1) g_teamHudSel = v;
		else if (sscanf(line, "teamhud_x=%d,%d,%d,%d,%d,%d,%d,%d",
			&g_teamHudX[0], &g_teamHudX[1], &g_teamHudX[2], &g_teamHudX[3],
			&g_teamHudX[4], &g_teamHudX[5], &g_teamHudX[6], &g_teamHudX[7]) == 8) {}
		else if (sscanf(line, "teamhud_y=%d,%d,%d,%d,%d,%d,%d,%d",
			&g_teamHudY[0], &g_teamHudY[1], &g_teamHudY[2], &g_teamHudY[3],
			&g_teamHudY[4], &g_teamHudY[5], &g_teamHudY[6], &g_teamHudY[7]) == 8) {}
		else if (sscanf(line, "crash_dialog=%d", &v) == 1) g_optCrashDialog = v != 0;
		else if (sscanf(line, "local_tick=%d", &v) == 1) g_optLocalTick = v;
		else if (sscanf(line, "local_nb=%d", &v) == 1) g_optLocalNb = v;
		else if (sscanf(line, "local_allow0lerp=%d", &v) == 1) g_optLocalAllow0Lerp = v != 0;
		else if (sscanf(line, "local_lerp=%d", &v) == 1) g_optLocalLerp = v;
		else if (sscanf(line, "local_idle_solo=%d", &v) == 1) g_optLocalIdleSolo = v != 0;
		else if (sscanf(line, "local_idle_nodelay=%d", &v) == 1) g_optLocalIdleNoDelay = v != 0;
		else if (sscanf(line, "local_aa=%d", &v) == 1) g_optLocalAa = v;
		else if (sscanf(line, "local_char_change=%d", &v) == 1) g_optLocalCharChange = v != 0;
		else if (sscanf(line, "client_throw_land=%d", &v) == 1) g_optClientThrowLand = v != 0;
		else if (sscanf(line, "client_infected_hp=%d", &v) == 1) g_optClientInfectedHp = v != 0;
		else if (sscanf(line, "local_infected_hp=%d", &v) == 1) g_optClientInfectedHp = v != 0; // migrate
		else if (sscanf(line, "client_no_corpse_si=%d", &v) == 1) g_optClientNoCorpseSi = v != 0;
		else if (sscanf(line, "client_no_corpse_ci=%d", &v) == 1) g_optClientNoCorpseCi = v != 0;
		else if (strncmp(line, "client_loot_beam=", 17) == 0) {}
		else if (sscanf(line, "client_dmg_num=%d", &v) == 1) g_optClientDmgNum = v != 0;
		else if (sscanf(line, "client_director_hud=%d", &v) == 1) g_optClientDirectorHud = v != 0;
		else if (strncmp(line, "local_ai_dmg=", 13) == 0) {}
		else if (strncmp(line, "local_melee_unlock=", 19) == 0) {}
		else if (strncmp(line, "local_saferoom_melee=", 21) == 0) {}
		else if (sscanf(line, "xhair=%d", &v) == 1) g_optXhair = v != 0;
		else if (sscanf(line, "xhair_ring=%d", &v) == 1) g_optXhairRing = v != 0;
		else if (sscanf(line, "xhair_ring_color=%d", &v) == 1) g_xhairRingColor = v;
		else if (sscanf(line, "xhair_ring_mode=%d", &v) == 1) g_xhairRingMode = v;
		else if (sscanf(line, "xhair_ring_alpha=%d", &v) == 1) g_xhairRingAlpha = v;
		else if (sscanf(line, "xhair_style=%d", &v) == 1) g_xhairStyle = v;
		else if (sscanf(line, "xhair_tex=%d", &v) == 1) g_optXhairTex = v != 0;
		else if (sscanf(line, "xhair_tex_full=%d", &v) == 1) g_optXhairTexFull = v != 0;
		else if (sscanf(line, "xhair_tex_size=%d", &v) == 1) g_xhairTexSize = v;
		else if (sscanf(line, "xhair_tex_mat=%79s", g_xhairTexMat) == 1) {}
		else if (sscanf(line, "xhair_tex_t%d=%79[^,],%d,%d", &v, tMat, &tSize, &tFull) == 4) {
			if (tMat[0] && !TexXhairIsVanillaTest(tMat) && g_texXhairTuneCount < kTexXhairMax
				&& TexXhairTuneFind(tMat) < 0) {
				TexXhairTune& t = g_texXhairTune[g_texXhairTuneCount++];
				strncpy(t.mat, tMat, 79);
				t.mat[79] = 0;
				t.size = tSize;
				t.full = tFull ? 1 : 0;
			}
		}
		else if (sscanf(line, "xhair_color=%d", &v) == 1) {
			xhairOldShare.color = v;
			hadXhairOldShare = true;
		} else if (sscanf(line, "xhair_size=%d", &v) == 1) {
			xhairOldShare.size = v;
			hadXhairOldShare = true;
		} else if (sscanf(line, "xhair_length=%d", &v) == 1) {
			xhairOldShare.length = v;
			hadXhairOldShare = true;
		} else if (sscanf(line, "xhair_gap=%d", &v) == 1) {
			xhairOldShare.gap = v;
			hadXhairOldShare = true;
		} else if (sscanf(line, "xhair_thick=%d", &v) == 1) {
			xhairOldShare.thick = v;
			hadXhairOldShare = true;
		} else if (sscanf(line, "xhair_dot=%d", &v) == 1) {
			xhairOldShare.dot = v;
			hadXhairOldShare = true;
		} else if (sscanf(line, "xhair_outline=%d", &v) == 1) {
			xhairOldShare.outline = v;
			hadXhairOldShare = true;
		} else if (sscanf(line, "xhair_alpha=%d", &v) == 1) {
			xhairOldShare.alpha = v;
			hadXhairOldShare = true;
		} else {
			int idx = -1;
			XhairTune pt{};
			if (sscanf(line, "xhair_p%d=%d,%d,%d,%d,%d,%d,%d,%d",
				&idx, &pt.color, &pt.size, &pt.length, &pt.gap,
				&pt.thick, &pt.dot, &pt.outline, &pt.alpha) == 9
				&& idx >= 0 && idx < kXhairStyleCount) {
				g_xhairTune[idx] = pt;
				hadXhairPer = true;
			}
		}
	}
	fclose(f);
	if ((!hadCiId || !hadSiId) && (legacyCi >= 0 || legacySi >= 0))
		MigrateLegacyStyleInts(legacyCi >= 0 ? legacyCi : 1, legacySi >= 0 ? legacySi : 1);
	if (!hadXhairPer && hadXhairOldShare) {
		for (int i = 0; i < kXhairStyleCount; ++i) {
			g_xhairTune[i] = xhairOldShare;
			g_xhairTune[i].dot = kXhairFactory[i].dot;
		}
	}
	if (TexXhairIsVanillaTest(g_xhairTexMat)) {
		g_xhairTexMat[0] = 0;
		g_xhairTexSize = 80;
		g_optXhairTexFull = false;
	} else {
		TexXhairApplyCurrent();
	}
	ClampSfxVol();
	ClampXhair();
	ClampMenuStyle();
	ClampListenOpts();
	if (!hadElimShow) {
		ElimApplyPreset(g_elimCompact != 0);
		if (g_elimFont == 28) g_elimFont = 22;
	} else if (g_elimShow == kElimShowLegacyFull) {
		g_elimShow = kElimShowAll;
	}
	ClampElimHud();
	ClampClockHud();
	ClampSpeedHud();
	ClampTimerHud();
	if (g_teamHudFont < 14) g_teamHudFont = 14;
	if (g_teamHudFont > 28) g_teamHudFont = 28;
	if (g_teamHudSel < 0) g_teamHudSel = 0;
	if (g_teamHudSel >= kTeamHudMax) g_teamHudSel = kTeamHudMax - 1;
	for (int i = 0; i < kTeamHudMax; ++i)
		ClampHudAbsCoord(true, &g_teamHudX[i], &g_teamHudY[i], 0, 7680);
	if (!hadHudHideParts && g_optHudHide) {
		g_optHudHideTeam = true;
		g_optHudHideWep = true;
		g_optHudHidePickup = true;
	}
	g_optHudHide = g_optHudHideTeam || g_optHudHideWep || g_optHudHidePickup;
	if (migratedLegacySettings || !hadElim || !hadElimShow)
		SaveSettings();
	Log("settings: loaded ci=%s si=%s ff=%d sound=%d sfxVol=%d icon=%d hitMode=%d siVis=%d siSnd=%d killFx=%d fx=%s ptDispWarm=%d xhair=%d ring=%d",
		DlcGetSelected("ci")[0] ? DlcGetSelected("ci") : "off",
		DlcGetSelected("si")[0] ? DlcGetSelected("si") : "off",
		g_optFf ? 1 : 0, g_optSound ? 1 : 0, g_optSfxVol, g_optIcon ? 1 : 0, g_optHitMode,
		g_optSiVisual ? 1 : 0, g_optSiSound ? 1 : 0, g_optKillFx ? 1 : 0,
		DlcGetSelected("fx")[0] ? DlcGetSelected("fx") : "off",
		g_optPtDispWarm ? 1 : 0, g_optXhair ? 1 : 0, g_optXhairRing ? 1 : 0);
}

static void* GetIface(const char* mod, const char* name) {
	HMODULE h = GetModuleHandleA(mod);
	if (!h) return nullptr;
	auto fn = (CreateInterfaceFn)GetProcAddress(h, "CreateInterface");
	return fn ? fn(name, nullptr) : nullptr;
}

static void* VGet(void* inst, int idx) { return (*(void***)inst)[idx]; }

static bool IsExec(void* p) {
	if (!p) return false;
	MEMORY_BASIC_INFORMATION mbi{};
	if (!VirtualQuery(p, &mbi, sizeof(mbi))) return false;
	return (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}

static bool PtrCommitted(const void* p) {
	if (!p) return false;
	MEMORY_BASIC_INFORMATION mbi{};
	if (!VirtualQuery(p, &mbi, sizeof(mbi))) return false;
	if (mbi.State != MEM_COMMIT) return false;
	const DWORD prot = mbi.Protect & 0xFF;
	return prot != PAGE_NOACCESS && prot != 0;
}

static bool IfaceAlive(void* inst) {
	if (!PtrCommitted(inst)) return false;
	void** vt = *(void***)inst;
	if (!PtrCommitted(vt)) return false;
	return IsExec(vt[0]);
}

static void* EvVt(void* ev, int idx) { return (*(void***)ev)[idx]; }

static const char* EvName(void* ev) {
	using Fn = const char*(__thiscall*)(void*);
	return ((Fn)EvVt(ev, 1))(ev);
}
static int EvInt(void* ev, const char* k, int def = 0) {
	using Fn = int(__thiscall*)(void*, const char*, int);
	return ((Fn)EvVt(ev, 6))(ev, k, def);
}
static bool EvBool(void* ev, const char* k, bool def = false) {
	using Fn = bool(__thiscall*)(void*, const char*, bool);
	return ((Fn)EvVt(ev, 5))(ev, k, def);
}
static float EvFloat(void* ev, const char* k, float def = 0.f) {
	using Fn = float(__thiscall*)(void*, const char*, float);
	return ((Fn)EvVt(ev, 8))(ev, k, def);
}
static const char* EvStr(void* ev, const char* k, const char* def = "") {
	using Fn = const char*(__thiscall*)(void*, const char*, const char*);
	return ((Fn)EvVt(ev, 9))(ev, k, def);
}

static int EngLocal() {
	if (!g_engine) return 0;
	using Fn = int(__thiscall*)(void*);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!IfaceAlive(g_engine)) return 0;
		s_fn = (Fn)VGet(g_engine, 12);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return 0;
		}
	}
	return s_fn(g_engine);
}
static int EngPlayerForUserID(int id) {
	using Fn = int(__thiscall*)(void*, int);
	return ((Fn)VGet(g_engine, 9))(g_engine, id);
}
static bool EngInGame() {
	if (!g_engine) return false;
	using Fn = bool(__thiscall*)(void*);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!IfaceAlive(g_engine)) return false;
		s_fn = (Fn)VGet(g_engine, 26);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return false;
		}
	}
	return s_fn(g_engine);
}
static bool EngDrawingLoading() {
	if (!g_engine) return false;
	using Fn = bool(__thiscall*)(void*);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!IfaceAlive(g_engine)) return false;
		s_fn = (Fn)VGet(g_engine, 28); // IsDrawingLoadingImage
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return false;
		}
	}
	return s_fn(g_engine);
}
static bool EngConnected() {
	if (!g_engine) return false;
	using Fn = bool(__thiscall*)(void*);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!IfaceAlive(g_engine)) return false;
		s_fn = (Fn)VGet(g_engine, 27);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return false;
		}
	}
	return s_fn(g_engine);
}

// sv_pure 2 = full Steam-only files (no custom content). 0/1 and unreadable = do not touch SKT.
// Fail-open: never suppress because we failed to read the cvar.
static std::atomic_bool g_pureSuppress{ false };

static int ConVarReadIntBounded(void* var, int lo, int hi) {
	if (!var) return lo - 1;
	// L4D2 ConVar: ConCommandBase (flags @ 0x14) + IConVar vptr, then m_pParent / m_Value.m_nValue.
	static const int kParentOff[] = { 0x1C, 0x18 };
	static const int kValueOff[] = { 0x30, 0x2C };
	for (int i = 0; i < 2; ++i) {
		void* parent = *(void**)((uint8_t*)var + kParentOff[i]);
		if (!parent) parent = var;
		MEMORY_BASIC_INFORMATION mbi{};
		if (!VirtualQuery(parent, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT)
			continue;
		int v = *(int*)((uint8_t*)parent + kValueOff[i]);
		if (v >= lo && v <= hi) return v;
	}
	return lo - 1;
}

static int ReadSvPure() {
	if (!g_cvar) return -1;
	using FindVarFn = void*(__thiscall*)(void*, const char*);
	auto fn = (FindVarFn)VGet(g_cvar, 12); // ICvar::FindVar; FindCommandBase is [10]
	if (!fn || !IsExec((void*)fn)) return -1;
	void* var = fn(g_cvar, "sv_pure");
	if (!var) return -1;
	return ConVarReadIntBounded(var, 0, 2);
}

static bool SkeetoFeaturesOn() {
	return !g_pureSuppress.load(std::memory_order_relaxed);
}

static void TickPurePolicy(bool connected) {
	if (!connected) {
		if (g_pureSuppress.exchange(false))
			Log("sv_pure: disconnected — Skeeto features re-enabled");
		return;
	}
	const int pure = ReadSvPure();
	const bool lock = (pure == 2); // only fully-pure servers
	const bool was = g_pureSuppress.exchange(lock);
	if (lock == was) return;
	if (lock) {
		MenuForceClose();
		Log("sv_pure=2 — Skeeto features off on this server (Necola orig unchanged)");
	} else {
		Log("sv_pure=%d — Skeeto features on", pure);
	}
}
static void EngGetViewAngles(float* pitchYawRoll) {
	if (!g_engine || !pitchYawRoll) return;
	using Fn = void(__thiscall*)(void*, float*);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		s_fn = (Fn)VGet(g_engine, 19);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return;
		}
	}
	s_fn(g_engine, pitchYawRoll);
}
// Use ClientCmd (idx 7) only. ClientCmd_Unrestricted idx was wrong/unsafe on L4D2 and crashed mid-load.
// Never run ClientCmd / SaveSettings synchronously inside IN_KeyEvent — that re-enters input and
// can leave WASD dead after heavy menu tweaking (mouse look still works).
static char g_deferCmds[8][192]{};
static int g_deferCmdN = 0;

static void EngClientCmd(const char* cmd) {
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!g_engine || !IfaceAlive(g_engine) || !cmd || !cmd[0]) return;
	if (g_inKeyHook) {
		if (g_deferCmdN < 8) {
			strncpy(g_deferCmds[g_deferCmdN], cmd, 191);
			g_deferCmds[g_deferCmdN][191] = 0;
			++g_deferCmdN;
		}
		return;
	}
	auto fn = (ClientCmdFn)VGet(g_engine, 7);
	if (fn && IsExec((void*)fn))
		fn(g_engine, cmd);
}

static void PumpDeferredClientCmds() {
	const int n = g_deferCmdN;
	g_deferCmdN = 0;
	for (int i = 0; i < n; ++i)
		EngClientCmd(g_deferCmds[i]);
}

// Source latches +forward/+attack until it sees the matching minus. A brief
// focus flash (Win11 XamlExplorerHostIslandWindow, overlay Hide/Show) can drop
// WM_KEYUP / WM_LBUTTONUP and the player walks or fires forever. Resync from
// the physical key state when Valve001 becomes foreground again.
static std::atomic_bool g_unstickButtons{ false };

static void ClientUxUnstickButtons() {
	if (!g_engine || !EngInGame()) return;
	if (g_menuVisible && !g_menuParked) return;
	if (!SkeetoFeaturesOn()) return;
	auto down = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
	char cmd[256]{};
	int n = 0;
	auto add = [&](bool held, const char* on, const char* off) {
		const char* s = held ? on : off;
		const int len = (int)strlen(s);
		if (n + len + 2 >= (int)sizeof(cmd)) return;
		if (n) cmd[n++] = ';';
		memcpy(cmd + n, s, (size_t)len);
		n += len;
		cmd[n] = 0;
	};
	add(down('W') || down(VK_UP), "+forward", "-forward");
	add(down('S') || down(VK_DOWN), "+back", "-back");
	add(down('A') || down(VK_LEFT), "+moveleft", "-moveleft");
	add(down('D') || down(VK_RIGHT), "+moveright", "-moveright");
	add(down(VK_LBUTTON), "+attack", "-attack");
	add(down(VK_RBUTTON), "+attack2", "-attack2");
	add(down(VK_SPACE), "+jump", "-jump");
	add(down(VK_CONTROL), "+duck", "-duck");
	add(down(VK_SHIFT), "+speed", "-speed");
	if (n) {
		EngClientCmd(cmd);
		Log("input unstick %s", cmd);
	}
}

// Same trick as the working SourceMod hit-feedback plugin:
// clear FCVAR_CHEAT on r_screenoverlay, and allow ClientCmd to run it.
static bool UnlockConCommand(const char* name, int clearFlags, int setFlags) {
	if (!g_cvar || !name || !name[0]) return false;
	auto findFn = (FindCmdBaseFn)VGet(g_cvar, 10);
	if (!findFn || !IsExec((void*)findFn)) {
		Log("UnlockCmd(%s): FindCommandBase invalid", name);
		return false;
	}
	void* cmd = findFn(g_cvar, name);
	if (!cmd) {
		Log("UnlockCmd(%s): not found", name);
		return false;
	}
	auto getFlags = (GetFlagsFn)VGet(cmd, 5);
	auto removeFlags = (RemoveFlagsFn)VGet(cmd, 4);
	auto addFlags = (AddFlagsFn)VGet(cmd, 3);
	int before = (getFlags && IsExec((void*)getFlags)) ? getFlags(cmd) : -1;
	if (before >= 0 && !(before & clearFlags) && (!setFlags || (before & setFlags)))
		return true;
	if (clearFlags && removeFlags && IsExec((void*)removeFlags))
		removeFlags(cmd, clearFlags);
	if (setFlags && addFlags && IsExec((void*)addFlags))
		addFlags(cmd, setFlags);
	// Fallback direct flag poke if virtuals look wrong
	if (before >= 0 && getFlags && IsExec((void*)getFlags)) {
		int mid = getFlags(cmd);
		if (clearFlags && (mid & clearFlags)) {
			int* pFlags = (int*)((uint8_t*)cmd + 0x14);
			*pFlags &= ~clearFlags;
		}
		if (setFlags && !(getFlags(cmd) & setFlags)) {
			int* pFlags = (int*)((uint8_t*)cmd + 0x14);
			*pFlags |= setFlags;
		}
	}
	int after = (getFlags && IsExec((void*)getFlags)) ? getFlags(cmd) : -1;
	Log("UnlockCmd(%s): flags 0x%X -> 0x%X", name, before, after);
	return true;
}

static void UnlockOverlayCommand() {
	if (g_overlayCmdUnlocked || !g_cvar) return;
	bool a = UnlockConCommand("r_screenoverlay", kFCvarCheat, kFCvarClientCmdCanExecute);
	bool b = UnlockConCommand("play", kFCvarCheat, kFCvarClientCmdCanExecute);
	UnlockConCommand("playvol", kFCvarCheat, kFCvarClientCmdCanExecute);
	g_overlayCmdUnlocked = a || b;
}

static void UnlockXhairCircleCvars() {
	static bool s_done = false;
	if (s_done || !g_cvar) return;
	s_done = true;
	const int clear = kFCvarCheat | kFCvarHidden | kFCvarDevOnly;
	UnlockConCommand("cl_crosshair_circle_mode", clear, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_crosshair_circle_alpha", clear, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_crosshair_alpha", 0, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_crosshair_red", 0, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_crosshair_green", 0, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_crosshair_blue", 0, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_crosshair_dynamic", 0, kFCvarClientCmdCanExecute);
	UnlockConCommand("crosshair", 0, kFCvarClientCmdCanExecute);
}

static void CvarSetInt(const char* name, int value) {
	if (!name || !name[0]) return;
	char cmd[96]{};
	snprintf(cmd, sizeof(cmd), "%s %d", name, value);
	EngClientCmd(cmd);
}

static void* CvarFind(const char* name) {
	if (!g_cvar || !name || !name[0]) return nullptr;
	using FindVarFn = void*(__thiscall*)(void*, const char*);
	auto fn = (FindVarFn)VGet(g_cvar, 12);
	if (!fn || !IsExec((void*)fn)) return nullptr;
	return fn(g_cvar, name);
}

static int CvarGetIntRange(const char* name, int lo, int hi, int fallback) {
	int v = ConVarReadIntBounded(CvarFind(name), lo, hi);
	if (v < lo || v > hi) return fallback;
	return v;
}

// =============================================================================
// Local listen-host experience (tick / nextbot / lerp / idle unlock).
// Hard gate: THIS process is running the game server (ServerGameDLL LevelInit /
// GameFrame, or gpGlobals tickcount advancing). Loopback is a fallback for solo
// `map`. Do not use "any dotted IP / ping>12ms" — lobby local (Steam relay / LAN)
// looks like a remote server but is still our listen host.
// server.dll leftover on a remote client is not enough. Official / someone else's
// server: never write, restore anything we already wrote.
// =============================================================================
static CRITICAL_SECTION g_listenCs;
static bool g_listenCsInit = false;
static std::atomic_bool g_listenHost{false};
static std::atomic_bool g_listenMapHost{false};
static std::atomic_bool g_listenServerSimLive{false};
static std::atomic<int> g_listenSimPrevTick{-1};
static std::atomic<DWORD> g_listenSimPrevAt{0};
static bool g_listenMutated = false;
static bool g_listenDirty = false;
static int g_idxNetChan = -1;
static float* g_hostTickInterval = nullptr;
static float g_hostTickSaved = 0.f;
static bool g_hostTickSavedOk = false;
static int g_listenAppliedTick = 30;
static int g_aaCmdApplied = -1;

struct ListenCvarSnap {
	const char* name;
	bool saved;
	bool wrote;
	bool hasMaxOk;
	int flags;
	int n;
	float f;
	unsigned char hasMax;
};
static ListenCvarSnap g_listenSnaps[] = {
	{ "sv_minrate", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_maxrate", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_mincmdrate", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_maxcmdrate", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_minupdaterate", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_maxupdaterate", false, false, false, 0, 0, 0.f, 0 },
	{ "net_splitpacket_maxrate", false, false, false, 0, 0, 0.f, 0 },
	{ "net_splitrate", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_client_min_interp_ratio", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_client_max_interp_ratio", false, false, false, 0, 0, 0.f, 0 },
	{ "nb_update_frequency", false, false, false, 0, 0, 0.f, 0 },
	{ "nb_update_framelimit", false, false, false, 0, 0, 0.f, 0 },
	{ "cl_interp", false, false, false, 0, 0, 0.f, 0 },
	{ "cl_interp_ratio", false, false, false, 0, 0, 0.f, 0 },
	{ "cl_updaterate", false, false, false, 0, 0, 0.f, 0 },
	{ "cl_cmdrate", false, false, false, 0, 0, 0.f, 0 },
	{ "rate", false, false, false, 0, 0, 0.f, 0 },
	{ "fps_max", false, false, false, 0, 0, 0.f, 0 },
	// Solo idle: without these the game refuses / immediately cancels when you are the only human.
	{ "sb_all_bot_game", false, false, false, 0, 0, 0.f, 0 },
	{ "allow_all_bot_survivor_team", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_airaccelerate", false, false, false, 0, 0, 0.f, 0 },
	{ "sv_airaccelerate_raycast", false, false, false, 0, 0, 0.f, 0 },
};
static constexpr int kListenSnapCount = (int)(sizeof(g_listenSnaps) / sizeof(g_listenSnaps[0]));

static void ListenCsEnsure() {
	if (g_listenCsInit) return;
	InitializeCriticalSection(&g_listenCs);
	g_listenCsInit = true;
}

static bool ListenAddrIsStrictLoopback(const char* a) {
	if (!a || !a[0]) return false;
	if (_strnicmp(a, "loopback", 8) == 0) {
		const char c = a[8];
		return c == 0 || c == ':' || c == ' ' || c == '\t';
	}
	if (_strnicmp(a, "localhost", 9) == 0) {
		const char c = a[9];
		return c == 0 || c == ':' || c == ' ' || c == '\t';
	}
	return false;
}

static bool ListenAddrIsLoopback(const char* a) {
	if (!a || !a[0]) return false;
	if (ListenAddrIsStrictLoopback(a))
		return true;
	if (!strncmp(a, "127.", 4))
		return true;
	if (_strnicmp(a, "::1", 3) == 0)
		return true;
	return false;
}

static bool ListenAddrLooksRemote(const char* a) {
	if (!a || !a[0]) return false;
	if (ListenAddrIsLoopback(a)) return false;
	if (!strncmp(a, "0.0.0.0", 7)) return false;
	bool dot = false, digit = false;
	for (const char* p = a; *p && *p != ':'; ++p) {
		if (*p == '.') dot = true;
		if (*p >= '0' && *p <= '9') digit = true;
	}
	return dot && digit;
}

static bool ListenPtrLooksObject(void* inst) {
	if (!PtrCommitted(inst)) return false;
	void** vt = *(void***)inst;
	return PtrCommitted(vt);
}

static const char* ListenChanAddr(void* nc) {
	if (!nc || !ListenPtrLooksObject(nc)) return "";
	using AddrFn = const char*(__thiscall*)(void*);
	static const int kAddrSlot[] = { 1, 2, 0 };
	for (int i = 0; i < 3; ++i) {
		auto fn = (AddrFn)VGet(nc, kAddrSlot[i]);
		if (!fn || !IsExec((void*)fn)) continue;
		const char* a = fn(nc);
		if (!a || !a[0] || !PtrCommitted(a)) continue;
		if (ListenAddrIsLoopback(a) || ListenAddrLooksRemote(a) || strchr(a, ':'))
			return a;
	}
	return "";
}

static bool ListenChanCallIsLoopback(void* nc) {
	if (!nc || !ListenPtrLooksObject(nc)) return false;
	using Fn = bool(__thiscall*)(void*);
	auto fn = (Fn)VGet(nc, 6);
	if (!fn || !IsExec((void*)fn)) return false;
	return fn(nc);
}

static float ListenChanLatency(void* nc) {
	if (!nc || !ListenPtrLooksObject(nc)) return -1.f;
	using Fn = float(__thiscall*)(void*, int);
	static const int kSlot[] = { 10, 9, 11 };
	for (int i = 0; i < 3; ++i) {
		auto fn = (Fn)VGet(nc, kSlot[i]);
		if (!fn || !IsExec((void*)fn)) continue;
		const float v = fn(nc, 0);
		if (v >= 0.f && v < 2.f)
			return v;
	}
	return -1.f;
}

static bool ListenChanIsLoopback(void* nc) {
	if (!nc) return false;
	if (ListenAddrIsStrictLoopback(ListenChanAddr(nc)))
		return true;
	return ListenChanCallIsLoopback(nc) && !ListenAddrLooksRemote(ListenChanAddr(nc));
}

static bool ListenServerDllLoaded() {
	if (GetModuleHandleA("server.dll"))
		return true;
	if (!g_gameL4d2Dir[0])
		return false;
	char path[MAX_PATH]{};
	snprintf(path, sizeof(path), "%s\\bin\\server.dll", g_gameL4d2Dir);
	HMODULE h = nullptr;
	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, path, &h) && h)
		return true;
	return false;
}

static void* EngNetChanFromClient() {
	if (!g_engine || !IfaceAlive(g_engine) || !EngConnected())
		return nullptr;
	auto tryIdx = [](int idx) -> void* {
		if (idx < 0) return nullptr;
		using Fn = void*(__thiscall*)(void*);
		auto fn = (Fn)VGet(g_engine, idx);
		if (!fn || !IsExec((void*)fn)) return nullptr;
		void* nc = fn(g_engine);
		if (!nc || !ListenPtrLooksObject(nc)) return nullptr;
		return nc;
	};
	if (g_idxNetChan >= 0) {
		void* nc = tryIdx(g_idxNetChan);
		if (nc) return nc;
		g_idxNetChan = -1;
	}
	// L4D2 VEngineClient013 (Necola header): GetNetChannelInfo = 74.
	// Do not lead with 72/73 — those are SaveAllocMemory / SaveFreeMemory.
	static const int kTry[] = { 74, 72, 75, 76, 71 };
	for (int i = 0; i < (int)(sizeof(kTry) / sizeof(kTry[0])); ++i) {
		void* nc = tryIdx(kTry[i]);
		if (!nc) continue;
		const char* a = ListenChanAddr(nc);
		if (!a[0] && !ListenChanCallIsLoopback(nc))
			continue;
		g_idxNetChan = kTry[i];
		Log("listen: GetNetChannelInfo idx=%d addr='%s' loop=%d lat=%.3f",
			g_idxNetChan, a[0] ? a : "?",
			ListenChanCallIsLoopback(nc) ? 1 : 0,
			ListenChanLatency(nc));
		return nc;
	}
	return nullptr;
}

static void* EngNetChanFromServer() {
	void* es = GetIface("engine.dll", "VEngineServer022");
	if (!es) es = GetIface("engine.dll", "VEngineServer021");
	if (!es || !IfaceAlive(es))
		return nullptr;
	using Fn = void*(__thiscall*)(void*, int);
	static const int kSlot[] = { 21, 20, 22 };
	int idx = EngLocal();
	if (idx < 1) idx = 1;
	const int kWho[] = { idx, 1, idx - 1 };
	for (int s = 0; s < 3; ++s) {
		auto fn = (Fn)VGet(es, kSlot[s]);
		if (!fn || !IsExec((void*)fn)) continue;
		for (int w = 0; w < 3; ++w) {
			if (kWho[w] < 1) continue;
			void* nc = fn(es, kWho[w]);
			if (!nc || !ListenPtrLooksObject(nc)) continue;
			const char* a = ListenChanAddr(nc);
			if (a[0] || ListenChanIsLoopback(nc)) {
				static bool s_logged = false;
				if (!s_logged) {
					s_logged = true;
					Log("listen: GetPlayerNetInfo slot=%d who=%d addr='%s'", kSlot[s], kWho[w], a[0] ? a : "?");
				}
				return nc;
			}
		}
	}
	return nullptr;
}

static void* EngNetChan() {
	void* nc = EngNetChanFromClient();
	if (nc) return nc;
	return EngNetChanFromServer();
}

static void ListenNoteServerSim();

static bool ListenDetectHost() {
	if (!g_run.load(std::memory_order_relaxed))
		return false;
	if (!g_engine || !IfaceAlive(g_engine) || !EngConnected())
		return false;
	ListenNoteServerSim();
	// This process is simulating the listen/dedicated server (lobby local included).
	if (g_listenMapHost.load(std::memory_order_relaxed)
		|| g_listenServerSimLive.load(std::memory_order_relaxed))
		return true;
	// Solo `map` / loopback before gpGlobals has ticked.
	void* nc = EngNetChan();
	if (!nc)
		return false;
	const char* addr = ListenChanAddr(nc);
	if (ListenAddrIsLoopback(addr))
		return true;
	if (ListenChanCallIsLoopback(nc) && !ListenAddrLooksRemote(addr))
		return true;
	return false;
}

static void ListenLogDetect(const char* why) {
	static DWORD s_last = 0;
	const DWORD now = GetTickCount();
	if (now - s_last < 2000 && why && why[0] != '!')
		return;
	s_last = now;
	void* nc = EngNetChan();
	const char* addr = ListenChanAddr(nc);
	Log("listen: %s host=%d server=%d mapHost=%d sim=%d connected=%d ingame=%d nc=%p addr='%s' loop=%d lat=%.3f idx=%d",
		why ? why : "?",
		ListenDetectHost() ? 1 : 0,
		ListenServerDllLoaded() ? 1 : 0,
		g_listenMapHost.load(std::memory_order_relaxed) ? 1 : 0,
		g_listenServerSimLive.load(std::memory_order_relaxed) ? 1 : 0,
		EngConnected() ? 1 : 0,
		EngInGame() ? 1 : 0,
		nc,
		addr && addr[0] ? addr : "-",
		ListenChanIsLoopback(nc) ? 1 : 0,
		ListenChanLatency(nc),
		g_idxNetChan);
}

static void* CvarParent(void* var) {
	if (!var) return nullptr;
	static const int kParentOff[] = { 0x1C, 0x18 };
	for (int i = 0; i < 2; ++i) {
		void* parent = *(void**)((uint8_t*)var + kParentOff[i]);
		if (parent && PtrCommitted(parent))
			return parent;
	}
	return var;
}

static int CvarFlagsOf(void* var) {
	if (!var || !PtrCommitted(var)) return 0;
	if (IfaceAlive(var)) {
		auto getFlags = (GetFlagsFn)VGet(var, 5);
		if (getFlags && IsExec((void*)getFlags))
			return getFlags(var);
	}
	return *(int*)((uint8_t*)var + 0x14);
}

static void CvarWriteFlags(void* var, int flags) {
	if (var && PtrCommitted(var))
		*(int*)((uint8_t*)var + 0x14) = flags;
	void* parent = CvarParent(var);
	if (parent && parent != var && PtrCommitted(parent))
		*(int*)((uint8_t*)parent + 0x14) = flags;
}

static int CvarPickValueLayout(void* parent) {
	if (!parent) return 0;
	static const int kNOff[] = { 0x30, 0x2C };
	static const int kFOff[] = { 0x2C, 0x28 };
	for (int i = 0; i < 2; ++i) {
		if (!PtrCommitted((uint8_t*)parent + kNOff[i]) || !PtrCommitted((uint8_t*)parent + kFOff[i]))
			continue;
		const float f = *(float*)((uint8_t*)parent + kFOff[i]);
		if (f == f && f > -1.0e8f && f < 1.0e8f)
			return i;
	}
	return 0;
}

static bool CvarReadPair(void* var, int* nOut, float* fOut) {
	void* parent = CvarParent(var);
	if (!parent) return false;
	static const int kNOff[] = { 0x30, 0x2C };
	static const int kFOff[] = { 0x2C, 0x28 };
	const int i = CvarPickValueLayout(parent);
	if (!PtrCommitted((uint8_t*)parent + kNOff[i]) || !PtrCommitted((uint8_t*)parent + kFOff[i]))
		return false;
	if (nOut) *nOut = *(int*)((uint8_t*)parent + kNOff[i]);
	if (fOut) *fOut = *(float*)((uint8_t*)parent + kFOff[i]);
	return true;
}

static bool CvarPokePair(void* var, int n, float f) {
	void* parent = CvarParent(var);
	if (!parent) return false;
	static const int kNOff[] = { 0x30, 0x2C };
	static const int kFOff[] = { 0x2C, 0x28 };
	const int i = CvarPickValueLayout(parent);
	uint8_t* pn = (uint8_t*)parent + kNOff[i];
	uint8_t* pf = (uint8_t*)parent + kFOff[i];
	if (!PtrCommitted(pn) || !PtrCommitted(pf))
		return false;
	DWORD old = 0;
	if (VirtualProtect(pn, 4, PAGE_READWRITE, &old)) {
		*(int*)pn = n;
		DWORD tmp = 0;
		VirtualProtect(pn, 4, old, &tmp);
	} else {
		*(int*)pn = n;
	}
	old = 0;
	if (VirtualProtect(pf, 4, PAGE_READWRITE, &old)) {
		*(float*)pf = f;
		DWORD tmp = 0;
		VirtualProtect(pf, 4, old, &tmp);
	} else {
		*(float*)pf = f;
	}
	return true;
}

static ListenCvarSnap* ListenSnapFind(const char* name) {
	for (int i = 0; i < kListenSnapCount; ++i) {
		if (_stricmp(g_listenSnaps[i].name, name) == 0)
			return &g_listenSnaps[i];
	}
	return nullptr;
}

static void ListenSnapCapture(ListenCvarSnap* s) {
	if (!s || s->saved) return;
	void* var = CvarFind(s->name);
	if (!var) return;
	s->flags = CvarFlagsOf(var);
	if (!CvarReadPair(var, &s->n, &s->f))
		return;
	void* parent = CvarParent(var);
	if (parent && PtrCommitted((uint8_t*)parent + 0x3C)) {
		unsigned char b = *((uint8_t*)parent + 0x3C);
		if (b <= 1) {
			s->hasMax = b;
			s->hasMaxOk = true;
		}
	}
	s->saved = true;
}

static bool ListenCvarWriteInt(const char* name, int v) {
	ListenCvarSnap* s = ListenSnapFind(name);
	if (s) ListenSnapCapture(s);
	void* var = CvarFind(name);
	if (!var) return false;
	int flags = CvarFlagsOf(var);
	if (flags & (kFCvarCheat | kFCvarDevOnly | kFCvarHidden))
		CvarWriteFlags(var, flags & ~(kFCvarCheat | kFCvarDevOnly | kFCvarHidden));
	if (s && s->hasMaxOk) {
		void* parent = CvarParent(var);
		if (parent && PtrCommitted((uint8_t*)parent + 0x3C))
			*((uint8_t*)parent + 0x3C) = 0;
	}
	bool ok = CvarPokePair(var, v, (float)v);
	CvarWriteFlags(var, flags); // put CHEAT/HIDDEN back; value stays
	if (s) s->wrote = true;
	g_listenMutated = true;
	return ok;
}

static void ListenPokeFloat(float* slot, float want) {
	if (!slot || !PtrCommitted(slot)) return;
	DWORD old = 0;
	if (VirtualProtect(slot, 4, PAGE_READWRITE, &old)) {
		*slot = want;
		DWORD tmp = 0;
		VirtualProtect(slot, 4, old, &tmp);
	} else {
		*slot = want;
	}
}

// L4D2 ConVar (Necola CvarsB.h): m_fValue +0x2C, m_nValue +0x30.
// Do not scan nearby slots: 0.1f's bits look like a pointer, so a heuristic skip
// leaves GetFloat() stuck at the default (commons T-pose at high tick).
static bool CvarPokeFloatValue(void* var, float v) {
	void* parent = CvarParent(var);
	if (!parent || !PtrCommitted(parent)) return false;
	uint8_t* p = (uint8_t*)parent;
	if (!PtrCommitted(p + 0x30)) return false;
	ListenPokeFloat((float*)(p + 0x2C), v);
	// GetFloat reads +0x2C. Do not smash m_nValue to 0 for 0.066 — only write n for whole numbers.
	if (v == (float)(int)v && PtrCommitted(p + 0x30)) {
		const int ni = (int)v;
		DWORD old = 0;
		if (VirtualProtect(p + 0x30, 4, PAGE_READWRITE, &old)) {
			*(int*)(p + 0x30) = ni;
			DWORD tmp = 0;
			VirtualProtect(p + 0x30, 4, old, &tmp);
		} else {
			*(int*)(p + 0x30) = ni;
		}
	}
	const int slen = *(int*)(p + 0x28);
	char* str = *(char**)(p + 0x24);
	if (slen >= 4 && slen <= 64 && str && PtrCommitted(str)) {
		char tmp[32]{};
		snprintf(tmp, sizeof(tmp), "%g", v);
		const size_t n = strlen(tmp);
		if (n + 1 <= (size_t)slen)
			memcpy(str, tmp, n + 1);
	}
	return true;
}

static bool ListenCvarWriteFloat(const char* name, float v) {
	ListenCvarSnap* s = ListenSnapFind(name);
	if (s) ListenSnapCapture(s);
	void* var = CvarFind(name);
	if (!var) return false;
	int flags = CvarFlagsOf(var);
	if (flags & (kFCvarCheat | kFCvarDevOnly | kFCvarHidden))
		CvarWriteFlags(var, flags & ~(kFCvarCheat | kFCvarDevOnly | kFCvarHidden));
	if (s && s->hasMaxOk) {
		void* parent = CvarParent(var);
		if (parent && PtrCommitted((uint8_t*)parent + 0x3C))
			*((uint8_t*)parent + 0x3C) = 0;
	}
	const bool ok = CvarPokeFloatValue(var, v);
	CvarWriteFlags(var, flags);
	if (s) s->wrote = true;
	g_listenMutated = true;
	void* parent = CvarParent(var);
	if (parent && PtrCommitted(parent)) {
		uint8_t* p = (uint8_t*)parent;
		Log("listen: cvar %s write=%.4f fValue=+2C=%.4f n=+30=%d ok=%d",
			name, v, *(float*)(p + 0x2C), *(int*)(p + 0x30), ok ? 1 : 0);
	}
	return ok;
}

static float* ListenResolveHostTick() {
	if (g_hostTickInterval && PtrCommitted(g_hostTickInterval)) {
		const float v = *g_hostTickInterval;
		if (v >= 0.007f && v <= 0.05f)
			return g_hostTickInterval;
		g_hostTickInterval = nullptr;
	}
	void* es = GetIface("engine.dll", "VEngineServer021");
	if (!es) es = GetIface("engine.dll", "VEngineServer022");
	if (!es || !IfaceAlive(es))
		return nullptr;
	void* fn = VGet(es, 0x50);
	if (!fn || !IsExec(fn))
		return nullptr;
	uint8_t* p = (uint8_t*)fn;
	uintptr_t obj = 0;
	if (p[6] == 0xA1)
		obj = *(uintptr_t*)(p + 7);
	else
		obj = *(uintptr_t*)(p + 7);
	if (!obj || !PtrCommitted((void*)obj))
		return nullptr;
	float* interval = (float*)(obj + 8);
	if (!PtrCommitted(interval))
		return nullptr;
	const float v = *interval;
	if (!(v >= 0.007f && v <= 0.05f))
		return nullptr;
	g_hostTickInterval = interval;
	Log("listen: host interval ptr=%p value=%.6f (tick~%.0f)",
		(void*)interval, v, (v > 0.f) ? (1.f / v) : 0.f);
	return interval;
}

static uint8_t* ListenGlobalsBase() {
	static uint8_t* s_g = nullptr;
	static DWORD s_at = 0;
	const DWORD now = GetTickCount();
	if (s_g && PtrCommitted(s_g + 0x1C) && now - s_at < 250)
		return s_g;
	s_at = now;
	s_g = nullptr;
	void* pim = GetIface("server.dll", "PlayerInfoManager002");
	if (!pim || !IfaceAlive(pim))
		return nullptr;
	using Fn = void*(__thiscall*)(void*);
	auto fn = (Fn)VGet(pim, 1);
	if (!fn || !IsExec((void*)fn))
		return nullptr;
	uint8_t* g = (uint8_t*)fn(pim);
	if (!g || !PtrCommitted(g + 0x1C))
		return nullptr;
	s_g = g;
	return s_g;
}

// CGlobalVarsBase: tickcount at +24, interval_per_tick at +0x1C.
// Advancing tickcount means THIS process is running the server sim (listen host).
static void ListenNoteServerSim() {
	if (!EngConnected()) {
		g_listenServerSimLive.store(false, std::memory_order_relaxed);
		g_listenSimPrevTick.store(-1, std::memory_order_relaxed);
		return;
	}
	uint8_t* g = ListenGlobalsBase();
	if (!g || !PtrCommitted(g + 24)) {
		g_listenServerSimLive.store(false, std::memory_order_relaxed);
		return;
	}
	const int tick = *(int*)(g + 24);
	const int prev = g_listenSimPrevTick.load(std::memory_order_relaxed);
	const DWORD now = GetTickCount();
	if (prev >= 0 && tick != prev)
		g_listenServerSimLive.store(true, std::memory_order_relaxed);
	else {
		const DWORD at = g_listenSimPrevAt.load(std::memory_order_relaxed);
		if (now - at > 1500)
			g_listenServerSimLive.store(false, std::memory_order_relaxed);
	}
	if (tick != prev) {
		g_listenSimPrevTick.store(tick, std::memory_order_relaxed);
		g_listenSimPrevAt.store(now, std::memory_order_relaxed);
	}
}

static float* ListenResolveGlobalsInterval() {
	void* pim = GetIface("server.dll", "PlayerInfoManager002");
	if (!pim || !IfaceAlive(pim))
		return nullptr;
	using Fn = void*(__thiscall*)(void*);
	auto fn = (Fn)VGet(pim, 1);
	if (!fn || !IsExec((void*)fn))
		return nullptr;
	uint8_t* g = (uint8_t*)fn(pim);
	if (!g || !PtrCommitted(g + 0x1C))
		return nullptr;
	float* slot = (float*)(g + 0x1C);
	const float v = *slot;
	if (v != 0.f && !(v >= 0.007f && v <= 0.05f))
		return nullptr;
	return slot;
}

static bool ListenWriteHostTick(int tick, bool alsoGlobals) {
	if (tick < 30) tick = 30;
	if (tick > 128) tick = 128;
	float* slot = ListenResolveHostTick();
	if (!slot) return false;
	const float want = 1.f / (float)tick;
	const float cur = *slot;
	const float d = (cur > want) ? (cur - want) : (want - cur);
	const bool same = (d <= 0.00005f);
	if (!same) {
		if (!g_hostTickSavedOk) {
			g_hostTickSaved = cur;
			g_hostTickSavedOk = true;
		}
		ListenPokeFloat(slot, want);
	}
	if (alsoGlobals) {
		float* gi = ListenResolveGlobalsInterval();
		if (gi) {
			const float gv = *gi;
			const float gd = (gv > want) ? (gv - want) : (want - gv);
			if (gd > 0.00005f) {
				ListenPokeFloat(gi, want);
				Log("listen: gpGlobals interval -> %.6f", want);
			}
		}
	}
	g_listenMutated = true;
	g_listenAppliedTick = tick;
	if (!same)
		Log("listen: host interval -> %.6f (tick %d) globals=%d", want, tick, alsoGlobals ? 1 : 0);
	return true;
}

static void ListenApplyRates(int tick) {
	if (tick < 30) tick = 30;
	ListenCvarWriteInt("sv_minrate", tick * 1000);
	ListenCvarWriteInt("sv_maxrate", tick * 1000);
	ListenCvarWriteInt("sv_mincmdrate", tick);
	ListenCvarWriteInt("sv_maxcmdrate", tick);
	ListenCvarWriteInt("sv_minupdaterate", tick);
	ListenCvarWriteInt("sv_maxupdaterate", tick);
	ListenCvarWriteInt("net_splitpacket_maxrate", (tick / 2) * 1000);
	ListenCvarWriteInt("net_splitrate", 2);
	ListenCvarWriteInt("cl_updaterate", tick);
	ListenCvarWriteInt("cl_cmdrate", tick);
	ListenCvarWriteInt("rate", tick * 1000);
	int fps = ConVarReadIntBounded(CvarFind("fps_max"), 0, 1000);
	if (fps > 0 && fps < tick)
		ListenCvarWriteInt("fps_max", 0);
}

// Auto nextbot interval (seconds). Lower = more often = stronger XSS, more CPU.
// Same curve as 豆瓣酱 tick plugin: 30→0.1, 60→0.024, 61–99 lerp down, ≥100→0.01.
static float ListenNbAutoFreq(int tick) {
	if (tick <= 30) return 0.1f;
	if (tick <= 60) return 0.024f;
	if (tick < 100) return 0.024f - 0.00035f * (float)(tick - 60);
	return 0.01f;
}

static bool ListenNbWantedFreq(float* out) {
	if (!out) return false;
	if (g_optLocalNb == 1) {
		*out = 0.066f;
		return true;
	}
	if (g_optLocalNb == 2) {
		*out = 0.024f;
		return true;
	}
	if (g_optLocalNb == 3) {
		*out = 0.1f; // engine default
		return true;
	}
	if (g_optLocalTick != 30) {
		*out = ListenNbAutoFreq(g_optLocalTick);
		return true;
	}
	return false;
}

static void ListenApplyNb() {
	float freq = 0.f;
	if (!ListenNbWantedFreq(&freq))
		return;
	ListenCvarWriteFloat("nb_update_frequency", freq);
	// ShouldUpdate: framelimit==0 skips the flagged path (Charger/Hunter never start
	// abilities). 15/30ms is the engine default but on listen+high tick commons burn
	// that budget first, so SI slide forever and never Think. Large positive = flagged
	// bots actually run. Frequency still controls how often they are scheduled.
	ListenCvarWriteFloat("nb_update_framelimit", 100.f);
}

static void ListenRestoreNamed(const char* const* names) {
	if (!names) return;
	for (int i = 0; names[i]; ++i) {
		ListenCvarSnap* s = ListenSnapFind(names[i]);
		if (!s || !s->saved || !s->wrote) continue;
		void* var = CvarFind(s->name);
		if (var) {
			CvarPokePair(var, s->n, s->f);
			if (s->hasMaxOk) {
				void* parent = CvarParent(var);
				if (parent && PtrCommitted((uint8_t*)parent + 0x3C))
					*((uint8_t*)parent + 0x3C) = s->hasMax;
			}
			CvarWriteFlags(var, s->flags);
		}
		s->wrote = false;
	}
}

static void ListenApplyAllow0Lerp() {
	ListenCvarWriteInt("sv_client_min_interp_ratio", -1);
	ListenCvarWriteInt("sv_client_max_interp_ratio", 1);
}

static void ListenApplyLerp() {
	if (g_optLocalLerp < 0) return;
	ListenCvarWriteFloat("cl_interp", (float)g_optLocalLerp / 1000.f);
	ListenCvarWriteFloat("cl_interp_ratio", 0.f);
}

static void ListenApplyIdleBotAllow() {
	// Same requirement as solo-idle guides / most SM servers: keep running with only bots.
	const bool a = ListenCvarWriteInt("sb_all_bot_game", 1);
	const bool b = ListenCvarWriteInt("allow_all_bot_survivor_team", 1);
	Log("listen: idle bot allow sb_all_bot_game=%d allow_team=%d", a ? 1 : 0, b ? 1 : 0);
}

static void ListenApplyAirAccel() {
	// Never touch this during server LevelInit / loading — a bad IConVar
	// SetValue call crashed server.dll+F28DF on the loading screen.
	if (!g_engine || !EngInGame() || EngDrawingLoading())
		return;
	int want = 10;
	if (g_optLocalAa == 100 || g_optLocalAa == 400 || g_optLocalAa == 1000)
		want = g_optLocalAa;
	if (g_aaCmdApplied == want)
		return;
	const int clear = kFCvarCheat | kFCvarDevOnly | kFCvarHidden;
	UnlockConCommand("sv_airaccelerate", clear, kFCvarClientCmdCanExecute);
	char cmd[48]{};
	snprintf(cmd, sizeof(cmd), "sv_airaccelerate %d", want);
	EngClientCmd(cmd);
	g_aaCmdApplied = want;
	if (want != 10)
		g_listenMutated = true;
	Log("listen: sv_airaccelerate cmd %s", cmd);
}

static void ListenRestoreAirAccel(const char* why) {
	if (g_aaCmdApplied < 0 || g_aaCmdApplied == 10) {
		g_aaCmdApplied = -1;
		return;
	}
	UnlockConCommand("sv_airaccelerate",
		kFCvarCheat | kFCvarDevOnly | kFCvarHidden, kFCvarClientCmdCanExecute);
	EngClientCmd("sv_airaccelerate 10");
	g_aaCmdApplied = -1;
	Log("listen: sv_airaccelerate restore 10 (%s)", why ? why : "?");
}

// Solo AFK (all-bot) often spams chat with director flow errors — mute the string in-process.
static char* g_flowSpamStr = nullptr;
static char g_flowSpamOrig = 0;
static bool g_flowSpamMuted = false;

static char* FindModCString(const char* module, const char* needle) {
	HMODULE h = GetModuleHandleA(module);
	if (!h || !needle || !needle[0]) return nullptr;
	auto dos = (PIMAGE_DOS_HEADER)h;
	auto nt = (PIMAGE_NT_HEADERS)((uint8_t*)h + dos->e_lfanew);
	auto base = (uint8_t*)h;
	const size_t size = (size_t)nt->OptionalHeader.SizeOfImage;
	const size_t nlen = strlen(needle);
	if (nlen == 0 || nlen >= size) return nullptr;
	for (size_t i = 0; i + nlen < size; ++i) {
		if (base[i] != (uint8_t)needle[0]) continue;
		if (memcmp(base + i, needle, nlen) == 0)
			return (char*)(base + i);
	}
	return nullptr;
}

static void ListenMuteDirectorFlowSpam(bool mute) {
	if (mute == g_flowSpamMuted) return;
	if (mute) {
		if (!g_flowSpamStr) {
			g_flowSpamStr = FindModCString("server.dll", "ERROR: FLOW IS BROKEN");
			if (!g_flowSpamStr)
				g_flowSpamStr = FindModCString("server.dll", "FLOW IS BROKEN");
		}
		if (!g_flowSpamStr || !g_flowSpamStr[0]) {
			Log("listen: flow-spam string not found (ok if unused)");
			return;
		}
		DWORD old = 0;
		if (!VirtualProtect(g_flowSpamStr, 1, PAGE_READWRITE, &old)) return;
		g_flowSpamOrig = g_flowSpamStr[0];
		g_flowSpamStr[0] = '\0';
		VirtualProtect(g_flowSpamStr, 1, old, &old);
		g_flowSpamMuted = true;
		Log("listen: muted director flow-spam chat");
	} else {
		if (g_flowSpamStr && g_flowSpamMuted && g_flowSpamOrig) {
			DWORD old = 0;
			if (VirtualProtect(g_flowSpamStr, 1, PAGE_READWRITE, &old)) {
				g_flowSpamStr[0] = g_flowSpamOrig;
				VirtualProtect(g_flowSpamStr, 1, old, &old);
			}
		}
		g_flowSpamMuted = false;
		g_flowSpamOrig = 0;
		Log("listen: restored director flow-spam string");
	}
}

// go_afk_unlock + direct GoAwayFromKeyboard (left4dhooks-style).
// Vanilla command only sets a timer; PreThink later calls GoAway — flaky alone.
// JMP Input_GoAwayFromKeyboard -> call GoAwayFromKeyboard(this) so bind works every time.
enum {
	kAfkPreComp = 0, // PreThink +309: jcc -> 6xNOP (versus)
	kAfkPreHuman,    // PreThink +428: cmp imm 1->0 (auto-idle when alone)
	kAfkPreHumanJle, // PreThink +429: jle -> NOP
	kAfkSlotCount
};
struct AfkSlot {
	uint8_t* p;
	uint8_t orig[8];
	uint8_t patch[8];
	int n;
	bool applied;
	bool mismatch;
};
static AfkSlot g_afkSlot[kAfkSlotCount]{};
static HMODULE g_afkServerMod = nullptr;
static bool g_afkResolved = false;
static bool g_afkResolveFail = false;
static bool g_afkLoggedFail = false;
static uint8_t* g_afkInputFn = nullptr;
static uint8_t* g_afkGoAwayFn = nullptr;
static uint8_t* g_afkStub = nullptr;
static uint8_t g_afkInputOrig[5]{};
static bool g_afkInputDetoured = false;

static bool AfkWrite(uint8_t* p, const uint8_t* src, int n) {
	if (!p || n <= 0 || n > 8 || !PtrCommitted(p)) return false;
	DWORD old = 0, tmp = 0;
	if (!VirtualProtect(p, (SIZE_T)n, PAGE_EXECUTE_READWRITE, &old))
		return false;
	memcpy(p, src, (size_t)n);
	VirtualProtect(p, (SIZE_T)n, old, &tmp);
	FlushInstructionCache(GetCurrentProcess(), p, (SIZE_T)n);
	return true;
}

static bool AfkMemEq(const uint8_t* p, const uint8_t* b, int n) {
	return p && b && n > 0 && memcmp(p, b, (size_t)n) == 0;
}

static void AfkRemoveInputDetour() {
	if (!g_afkInputDetoured) return;
	if (g_afkInputFn && PtrCommitted(g_afkInputFn)
		&& GetModuleHandleA("server.dll") == g_afkServerMod)
		AfkWrite(g_afkInputFn, g_afkInputOrig, 5);
	g_afkInputDetoured = false;
	Log("listen: afk Input detour removed");
}

static void AfkForgetSlots() {
	AfkRemoveInputDetour();
	for (int i = 0; i < kAfkSlotCount; ++i) {
		AfkSlot& s = g_afkSlot[i];
		s.p = nullptr;
		s.n = 0;
		s.applied = false;
		s.mismatch = false;
		memset(s.orig, 0, sizeof(s.orig));
		memset(s.patch, 0, sizeof(s.patch));
	}
	g_afkResolved = false;
	g_afkInputFn = nullptr;
	g_afkGoAwayFn = nullptr;
}

static bool AfkFillSlot(AfkSlot& s, uint8_t* p, const uint8_t* patch, int n, const uint8_t* expect, int expectN) {
	s.p = nullptr;
	s.n = 0;
	s.mismatch = false;
	if (!p || !PtrCommitted(p) || n <= 0 || n > 8) return false;
	if (expect && expectN > 0 && !AfkMemEq(p, expect, expectN)) {
		s.mismatch = true;
		return false;
	}
	s.p = p;
	s.n = n;
	memcpy(s.patch, patch, (size_t)n);
	memcpy(s.orig, p, (size_t)n);
	return true;
}

static bool AfkEnsureStub() {
	if (g_afkStub) return true;
	g_afkStub = (uint8_t*)VirtualAlloc(nullptr, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!g_afkStub) {
		Log("listen: afk stub VirtualAlloc failed");
		return false;
	}
	return true;
}

static bool AfkInstallInputDetour() {
	if (!g_afkInputFn || !g_afkGoAwayFn || !AfkEnsureStub())
		return false;
	int32_t callRel = (int32_t)(g_afkGoAwayFn - (g_afkStub + 5));
	g_afkStub[0] = 0xE8;
	memcpy(g_afkStub + 1, &callRel, 4);
	g_afkStub[5] = 0xC3;
	FlushInstructionCache(GetCurrentProcess(), g_afkStub, 6);

	uint8_t jmp[5] = { 0xE9, 0, 0, 0, 0 };
	int32_t jmpRel = (int32_t)(g_afkStub - (g_afkInputFn + 5));
	memcpy(jmp + 1, &jmpRel, 4);
	if (g_afkInputDetoured) {
		if (!AfkMemEq(g_afkInputFn, jmp, 5))
			AfkWrite(g_afkInputFn, jmp, 5);
		return true;
	}
	memcpy(g_afkInputOrig, g_afkInputFn, 5);
	if (!AfkWrite(g_afkInputFn, jmp, 5))
		return false;
	g_afkInputDetoured = true;
	g_listenMutated = true;
	Log("listen: afk Input -> GoAway detour ok input=%p goaway=%p stub=%p",
		(void*)g_afkInputFn, (void*)g_afkGoAwayFn, (void*)g_afkStub);
	return true;
}

static bool ListenAfkResolve() {
	HMODULE mod = GetModuleHandleA("server.dll");
	if (!mod) {
		AfkForgetSlots();
		g_afkServerMod = nullptr;
		g_afkResolveFail = false;
		return false;
	}
	if (mod == g_afkServerMod && g_afkResolved)
		return true;
	if (mod == g_afkServerMod && g_afkResolveFail)
		return false;

	AfkForgetSlots();
	g_afkServerMod = mod;
	g_afkResolveFail = false;

	const uintptr_t pre = FindPat("server.dll",
		"55 8B ? 83 ? ? A1 ? ? ? ? 33 ? 89 ? ? 56 57 8B ? E8 ? ? ? ? 8B ? E8");
	const char* inpPat = "57 8B ? E8 ? ? ? ? 84 ? 75 ? 56 6A";
	uintptr_t inpHits[8]{};
	const int inpN = CountPat("server.dll", inpPat, inpHits, 8);
	const uintptr_t inp = (inpN == 1) ? inpHits[0] : 0;
	const uintptr_t go = FindPat("server.dll",
		"55 8B EC 83 EC 08 53 56 57 8B F1 8B 06 8B 90 C8 08 00 00");
	if (inpN != 1) {
		Log("listen: afk Input sig hits=%d (need 1) — skip Input detour", inpN);
		for (int i = 0; i < inpN && i < 8; ++i)
			Log("  input hit[%d]=%p", i, (void*)inpHits[i]);
	}
	if (!pre || !go) {
		g_afkResolveFail = true;
		if (!g_afkLoggedFail) {
			g_afkLoggedFail = true;
			Log("listen: afk sig miss pre=%p input=%p goaway=%p", (void*)pre, (void*)inp, (void*)go);
		}
		return false;
	}

	uint8_t* preP = (uint8_t*)pre;
	g_afkInputFn = inp ? (uint8_t*)inp : nullptr;
	g_afkGoAwayFn = (uint8_t*)go;
	const uint8_t nop6[6] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	const uint8_t nop2[2] = { 0x90, 0x90 };
	const uint8_t expectJcc[1] = { 0x0F };
	const uint8_t human0[1] = { 0x00 };
	const uint8_t expect1[1] = { 0x01 };
	const uint8_t expectJle[1] = { 0x7E };

	bool ok = true;
	ok = AfkFillSlot(g_afkSlot[kAfkPreComp], preP + 309, nop6, 6, expectJcc, 1) && ok;
	ok = AfkFillSlot(g_afkSlot[kAfkPreHuman], preP + 428, human0, 1, expect1, 1) && ok;
	ok = AfkFillSlot(g_afkSlot[kAfkPreHumanJle], preP + 429, nop2, 2, expectJle, 1) && ok;

	if (!ok) {
		g_afkResolveFail = true;
		if (!g_afkLoggedFail) {
			g_afkLoggedFail = true;
			Log("listen: afk PreThink offset mismatch pre+309=%02X pre+428=%02X pre+429=%02X",
				preP[309], preP[428], preP[429]);
		}
		AfkForgetSlots();
		return false;
	}

	g_afkResolved = true;
	Log("listen: afk resolved PreThink=%p Input=%p GoAway=%p", (void*)pre, (void*)inp, (void*)go);
	return true;
}

static bool AfkSlotWanted(int id) {
	return g_optLocalIdleSolo
		&& (id == kAfkPreComp || id == kAfkPreHuman || id == kAfkPreHumanJle);
}

static void ListenAfkRestoreUnlocked(const char* why) {
	AfkRemoveInputDetour();
	bool any = false;
	for (int i = 0; i < kAfkSlotCount; ++i) {
		AfkSlot& s = g_afkSlot[i];
		if (!s.applied) continue;
		if (s.p && PtrCommitted(s.p) && GetModuleHandleA("server.dll") == g_afkServerMod)
			AfkWrite(s.p, s.orig, s.n);
		s.applied = false;
		any = true;
	}
	if (any)
		Log("listen: afk restored (%s)", why ? why : "?");
}

static void ListenAfkRestore(const char* why) {
	ListenCsEnsure();
	EnterCriticalSection(&g_listenCs);
	ListenAfkRestoreUnlocked(why);
	LeaveCriticalSection(&g_listenCs);
}

static void ListenAfkSyncUnlocked() {
	const bool wantAny = g_optLocalIdleSolo || g_optLocalIdleNoDelay;
	if (!wantAny) {
		ListenAfkRestoreUnlocked("opts-off");
		return;
	}
	if (!ListenAfkResolve())
		return;

	int wrote = 0, restored = 0;
	for (int i = 0; i < kAfkSlotCount; ++i) {
		AfkSlot& s = g_afkSlot[i];
		const bool want = AfkSlotWanted(i);
		if (want) {
			if (s.applied && s.p && AfkMemEq(s.p, s.patch, s.n))
				continue;
			if (!s.p || !AfkWrite(s.p, s.patch, s.n)) continue;
			s.applied = true;
			g_listenMutated = true;
			++wrote;
		} else if (s.applied) {
			if (s.p && PtrCommitted(s.p))
				AfkWrite(s.p, s.orig, s.n);
			s.applied = false;
			++restored;
		}
	}

	bool detourOk = true;
	if (g_optLocalIdleNoDelay)
		detourOk = AfkInstallInputDetour();
	else
		AfkRemoveInputDetour();
	Log("listen: afk sync solo=%d nodelay=%d wrote=%d restored=%d detour=%d input0=%02X",
		g_optLocalIdleSolo ? 1 : 0, g_optLocalIdleNoDelay ? 1 : 0, wrote, restored, detourOk ? 1 : 0,
		g_afkInputFn ? g_afkInputFn[0] : 0xFF);
}

static void ListenAfkSync() {
	ListenCsEnsure();
	EnterCriticalSection(&g_listenCs);
	ListenAfkSyncUnlocked();
	LeaveCriticalSection(&g_listenCs);
}

static void ListenAfkSyncUnlocked();
static void LocalPlaySync(const char* why);
static void LocalPlayRestore(const char* why);
static void LocalPlayPaint();
static void LocalPlayOnRoundBoundary(bool start);
static void LocalPlayOnPlayerHurtHp(void* ev);
static void LocalPlayOnWitchSpawn(void* ev);
static void LocalPlayOnWitchHurt(void* ev);
static void LocalPlayOnWitchKilled(void* ev);
static void LocalPlayOnTrackedDeath(void* ev);
static void LocalPlayClearHpTrack();
static bool LocalPlayOptsWanted();
static void LocalPlayPrecacheAllSurvivors(const char* why);
static void LocalPlayNotifySiHp(void* ent, int objectId, int newHp, int userid);
static bool IsLocalAttacker(void* ev);

static void ListenRestoreSnaps() {
	for (int i = 0; i < kListenSnapCount; ++i) {
		ListenCvarSnap& s = g_listenSnaps[i];
		if (!s.saved || !s.wrote) {
			s.saved = false;
			s.wrote = false;
			s.hasMaxOk = false;
			continue;
		}
		void* var = CvarFind(s.name);
		if (var) {
			CvarPokePair(var, s.n, s.f);
			if (s.hasMaxOk) {
				void* parent = CvarParent(var);
				if (parent && PtrCommitted((uint8_t*)parent + 0x3C))
					*((uint8_t*)parent + 0x3C) = s.hasMax;
			}
			CvarWriteFlags(var, s.flags);
		}
		s.saved = false;
		s.wrote = false;
		s.hasMaxOk = false;
	}
}

static void ListenRestoreHostTick() {
	// Never poke interval while a map is running — that launches the player.
	if (EngInGame()) {
		g_listenAppliedTick = 30;
		return;
	}
	const float want = (g_hostTickSavedOk && g_hostTickSaved >= 0.007f && g_hostTickSaved <= 0.05f)
		? g_hostTickSaved : (1.f / 30.f);
	if (g_hostTickInterval && PtrCommitted(g_hostTickInterval))
		ListenPokeFloat(g_hostTickInterval, want);
	g_listenAppliedTick = 30;
}

static void ListenRestore(const char* why) {
	if (!g_listenMutated) return;
	ListenCsEnsure();
	EnterCriticalSection(&g_listenCs);
	if (!g_listenMutated) {
		LeaveCriticalSection(&g_listenCs);
		return;
	}
	ListenAfkRestoreUnlocked(why);
	LocalPlayRestore(why);
	ListenMuteDirectorFlowSpam(false);
	ListenRestoreAirAccel(why);
	ListenRestoreSnaps();
	ListenRestoreHostTick();
	g_listenMutated = false;
	g_listenDirty = false;
	g_hostTickSavedOk = false;
	LeaveCriticalSection(&g_listenCs);
	Log("listen: restored (%s)", why ? why : "?");
}

static bool ListenOptsWanted() {
	return g_optLocalTick != 30 || g_optLocalNb != 0 || g_optLocalAllow0Lerp || g_optLocalLerp >= 0
		|| g_optLocalIdleSolo || g_optLocalIdleNoDelay
		|| g_optLocalAa != 10
		|| g_optLocalCharChange;
}

static bool ListenWhyWritesTick(const char* why) {
	if (why && strcmp(why, "menu") == 0)
		return !EngInGame();
	if (why && (strcmp(why, "levelinit") == 0 || strcmp(why, "preentity") == 0
		|| strcmp(why, "server-levelinit") == 0))
		return true;
	return !EngInGame();
}

static bool ListenWriteTickNow(int tick, const char* why) {
	(void)why;
	return ListenWriteHostTick(tick, true);
}

static void ListenApply(const char* why) {
	if (!ListenDetectHost())
		return;
	ListenCsEnsure();
	EnterCriticalSection(&g_listenCs);
	if (!ListenDetectHost()) {
		LeaveCriticalSection(&g_listenCs);
		return;
	}
	if (!ListenOptsWanted() && !g_listenMutated) {
		g_listenDirty = false;
		LeaveCriticalSection(&g_listenCs);
		return;
	}
	if (g_optLocalTick != 30) {
		if (ListenWhyWritesTick(why)) {
			if (!ListenWriteTickNow(g_optLocalTick, why))
				Log("listen: tick unlock unavailable (cvars only) want=%d", g_optLocalTick);
			ListenApplyRates(g_optLocalTick);
		} else if (g_listenAppliedTick != g_optLocalTick) {
			Log("listen: tick %d queued until map change (now ~%d)", g_optLocalTick, g_listenAppliedTick);
		} else {
			ListenApplyRates(g_optLocalTick);
		}
	} else if (g_listenAppliedTick != 30) {
		if (!ListenWhyWritesTick(why)) {
			Log("listen: tick 30 queued until map change (now ~%d)", g_listenAppliedTick);
		} else {
			ListenWriteTickNow(30, why);
			static const char* kRate[] = {
				"sv_minrate", "sv_maxrate", "sv_mincmdrate", "sv_maxcmdrate",
				"sv_minupdaterate", "sv_maxupdaterate", "net_splitpacket_maxrate",
				"net_splitrate", "cl_updaterate", "cl_cmdrate", "rate", "fps_max",
				nullptr
			};
			for (int i = 0; kRate[i]; ++i) {
				ListenCvarSnap* s = ListenSnapFind(kRate[i]);
				if (!s || !s->saved || !s->wrote) continue;
				void* var = CvarFind(s->name);
				if (var) {
					CvarPokePair(var, s->n, s->f);
					if (s->hasMaxOk) {
						void* parent = CvarParent(var);
						if (parent && PtrCommitted((uint8_t*)parent + 0x3C))
							*((uint8_t*)parent + 0x3C) = s->hasMax;
					}
					CvarWriteFlags(var, s->flags);
				}
				s->wrote = false;
			}
		}
	}
	float nbFreq = 0.f;
	if (ListenNbWantedFreq(&nbFreq))
		ListenApplyNb();
	else {
		static const char* kNb[] = { "nb_update_frequency", "nb_update_framelimit", nullptr };
		ListenRestoreNamed(kNb);
	}
	if (g_optLocalAllow0Lerp)
		ListenApplyAllow0Lerp();
	else {
		static const char* kAllow[] = {
			"sv_client_min_interp_ratio", "sv_client_max_interp_ratio", nullptr
		};
		ListenRestoreNamed(kAllow);
	}
	if (g_optLocalLerp >= 0)
		ListenApplyLerp();
	else {
		static const char* kLerp[] = { "cl_interp", "cl_interp_ratio", nullptr };
		ListenRestoreNamed(kLerp);
	}
	if (g_optLocalIdleSolo)
		ListenApplyIdleBotAllow();
	else {
		static const char* kIdleBot[] = { "sb_all_bot_game", "allow_all_bot_survivor_team", nullptr };
		ListenRestoreNamed(kIdleBot);
	}
	ListenApplyAirAccel();
	ListenMuteDirectorFlowSpam(g_optLocalIdleSolo);
	ListenAfkSyncUnlocked();
	g_listenDirty = false;
	LeaveCriticalSection(&g_listenCs);
	LocalPlaySync(why);
	Log("listen: apply (%s) host=1 tick=%d nb=%d freq=%.4f allow0=%d lerp=%d idle=%d/%d aa=%d mutated=%d",
		why ? why : "?", g_optLocalTick, g_optLocalNb, nbFreq,
		g_optLocalAllow0Lerp ? 1 : 0, g_optLocalLerp,
		g_optLocalIdleSolo ? 1 : 0, g_optLocalIdleNoDelay ? 1 : 0, g_optLocalAa, g_listenMutated ? 1 : 0);
}

// IServerGameDLL::LevelInit runs only in THIS process when it is hosting.
// Do not wait for loopback netchan — that is too late, NextBots already spawned.
static void ListenApplyServerBoot(const char* why) {
	ListenCsEnsure();
	EnterCriticalSection(&g_listenCs);
	if (g_optLocalTick != 30) {
		if (!ListenWriteHostTick(g_optLocalTick, true))
			Log("listen: tick unlock unavailable (cvars only) want=%d", g_optLocalTick);
		ListenApplyRates(g_optLocalTick);
	}
	float nbFreq = 0.f;
	if (ListenNbWantedFreq(&nbFreq))
		ListenApplyNb();
	if (g_optLocalAllow0Lerp)
		ListenApplyAllow0Lerp();
	if (g_optLocalLerp >= 0)
		ListenApplyLerp();
	if (g_optLocalIdleSolo)
		ListenApplyIdleBotAllow();
	ListenAfkSyncUnlocked();
	ListenMuteDirectorFlowSpam(g_optLocalIdleSolo);
	g_listenDirty = false;
	LeaveCriticalSection(&g_listenCs);
	LocalPlaySync(why);
	Log("listen: apply (%s) boot tick=%d nb=%d freq=%.4f allow0=%d lerp=%d idle=%d/%d aa=%d",
		why ? why : "?", g_optLocalTick, g_optLocalNb, nbFreq,
		g_optLocalAllow0Lerp ? 1 : 0, g_optLocalLerp,
		g_optLocalIdleSolo ? 1 : 0, g_optLocalIdleNoDelay ? 1 : 0, g_optLocalAa);
}

static void ListenPump() {
	if (!g_run.load(std::memory_order_relaxed))
		return;
	EnsureServerGameHooks();
	if (!EngConnected()) {
		g_listenMapHost.store(false, std::memory_order_relaxed);
		g_listenServerSimLive.store(false, std::memory_order_relaxed);
		g_listenSimPrevTick.store(-1, std::memory_order_relaxed);
	}
	ListenNoteServerSim();
	const bool host = ListenDetectHost();
	const bool was = g_listenHost.exchange(host, std::memory_order_relaxed);
	if (!host) {
		if (EngConnected()) {
			// On a remote session — tear listen patches down.
			ListenAfkRestore(was ? "left-listen" : "remote");
			LocalPlayRestore(was ? "left-listen" : "remote");
			ListenMuteDirectorFlowSpam(false);
			if (g_listenMutated)
				ListenRestore(was ? "left-listen" : "not-listen");
		} else {
			// Lobby / map load: netchan may briefly look non-host — keep AFK patches.
			if (!(g_optLocalIdleSolo || g_optLocalIdleNoDelay))
				ListenAfkRestore("opts-off-lobby");
			ListenCsEnsure();
			EnterCriticalSection(&g_listenCs);
			if (g_optLocalTick != 30)
				ListenWriteHostTick(g_optLocalTick, false);
			else if (g_listenAppliedTick != 30)
				ListenWriteHostTick(30, false);
			LeaveCriticalSection(&g_listenCs);
		}
		return;
	}
	if (!was)
		g_listenDirty = true;
	if (!was)
		ListenLogDetect("!entered");
	if (g_listenDirty || (ListenOptsWanted() && !g_listenMutated))
		ListenApply(was ? "pump" : "entered-listen");
	if (EngInGame() && !EngDrawingLoading())
		ListenApplyAirAccel();
}

static void ListenOnLevelInit() {
	if (!ListenDetectHost()) {
		ListenRestore("levelinit-remote");
		return;
	}
	g_listenHost.store(true, std::memory_order_relaxed);
	g_listenDirty = true;
	g_aaCmdApplied = -1;
	ListenApply("levelinit");
	// Map-load is the reliable window for L4D1 survivor models on L4D2 campaigns.
	// Only when character-change is enabled — otherwise skips (avoids FPS cost).
	if (g_optLocalCharChange)
		LocalPlayPrecacheAllSurvivors("levelinit");
}

static void ListenMarkDirty() {
	g_listenDirty = true;
	if (ListenDetectHost())
		ListenApply("menu");
}

// =============================================================================
// Local play extras (listen-host only) — SI HP bar / survivor model swap
// =============================================================================
struct RecvTable { void* props; int nProps; void* pad; const char* name; };
struct ClientClass {
	void* c0; void* c1; const char* name; RecvTable* table; ClientClass* next; int id;
};
// Matches Necola SDK player_info_t layout (L4D2)
struct PlayerInfo {
	char pad0[0x8];
	char name[32];
	int userid;
	char pad1[0x150];
};
static int FindOffset(RecvTable* table, const char* propName);
static bool EntReadable(void* e);
static void SurfEnsureHudFont();
static void SurfTextAt(unsigned long font, int x, int y, COLORREF rgb, const wchar_t* text);
static void SurfColor(int r, int g, int b, int a);
static void SurfFill(int x0, int y0, int x1, int y1);
static void SurfLine(int x0, int y0, int x1, int y1);
static void SurfClearTexture();
static void SurfEnsureDmgFont();
static int SurfTextWFont(unsigned long font, const wchar_t* text);
static bool MenuEnsureSurf();
static bool SurfGetScreenSize(int* sw, int* sh);
static void* EntGet(int i);
static const char* EntNetClassName(int i);
static void RefreshLocalUserId();
static bool EngPlayerInfo(int ent, PlayerInfo* out);
static bool VtProtectWrite(void** slot, void* neu, void** prevOut);

static constexpr int kSpawnVmt = 24;        // CBaseEntity::Spawn
static constexpr int kSetModelVmt = 27;

static int g_offMaxHealth = -1;
static int g_offSurvivorCharacter = -1;
static int g_offSurvCharServer = -1;
static constexpr int kGlobalsPEdictsOff = 0x58;
static constexpr int kEdictStride = 0x10;
static constexpr int kEntEdictOff = 0x28;
static constexpr int kEngPrecacheModel = 5;
static constexpr int kEngIsModelPrecached = 9;
static constexpr int kEngServerCommand = 37;
static constexpr int kEngServerExecute = 38;
static constexpr int kEngWorldToScreenMatrix = 37;

static int g_hpPrev[65]{};
static int g_hpMax[65]{};
static int g_hpZc[65]{};
static int g_witchId[8]{};
static int g_witchHp[8]{};
static int g_witchMax[8]{};
static int g_witchCur = 0;

static int g_hpTrackEnt = 0;
static int g_hpTrackUid = 0; // player_death userid match (index alone is unreliable on death)
static bool g_hpTrackWitch = false;
static int g_hpTrackNow = 0;
static int g_hpTrackMax = 1;
static int g_hpTrackZc = 0;
static DWORD g_hpTrackUntil = 0;
static wchar_t g_hpTrackName[32]{};
// Last SI we locally attributed (aim-confirmed HP drop or local player_hurt).
static int g_hpLocalSiEnt = 0;
static DWORD g_hpLocalSiUntil = 0;
// After death, client often reports HP=1 on the corpse — ignore reopen for a short window.
static int g_hpDeadUid = 0;
static int g_hpDeadEnt = 0;
static DWORD g_hpDeadUntil = 0;
static wchar_t g_hpTipLine[160]{};
static DWORD g_hpTipUntil = 0;
// Remember last menu pick so idle→takeover can re-apply (engine restores old character).
static int g_charWantProp = -1;
static char g_charWantModel[96]{};
static wchar_t g_charWantName[24]{};
static DWORD g_charReapplyAt = 0;
static DWORD g_charReapplyAt2 = 0;

static bool LocalPlayOptsWanted() {
	return g_optLocalCharChange;
}

static void LocalPlayHpShow(const wchar_t* text);

static void LocalPlayResolveClientProps() {
	if (g_offMaxHealth >= 0 && g_offSurvivorCharacter >= 0)
		return;
	void* bc = GetIface("client.dll", "VClient016");
	if (!bc) return;
	using Fn = ClientClass*(__thiscall*)(void*);
	auto getCc = (Fn)VGet(bc, 7);
	if (!getCc || !IsExec((void*)getCc)) return;
	for (auto* cc = getCc(bc); cc; cc = cc->next) {
		if (!cc->name || !cc->table) continue;
		if (!strcmp(cc->name, "CTerrorPlayer")) {
			if (g_offMaxHealth < 0) g_offMaxHealth = FindOffset(cc->table, "m_iMaxHealth");
			if (g_offSurvivorCharacter < 0)
				g_offSurvivorCharacter = FindOffset(cc->table, "m_survivorCharacter");
		}
	}
	static bool s_logged = false;
	if (!s_logged) {
		s_logged = true;
		Log("localplay: props maxhp=%d zclass=%d hp=%d survChar=%d",
			g_offMaxHealth, g_offZombieClass, g_offHealth, g_offSurvivorCharacter);
	}
}

static void* LocalPlayGetGlobals() {
	void* pim = GetIface("server.dll", "PlayerInfoManager002");
	if (!pim || !IfaceAlive(pim)) return nullptr;
	using Fn = void*(__thiscall*)(void*);
	auto fn = (Fn)VGet(pim, 1);
	if (!fn || !IsExec((void*)fn)) return nullptr;
	return fn(pim);
}

static void* LocalPlayServerGameEnts() {
	void* sge = GetIface("server.dll", "ServerGameEnts001");
	return (sge && IfaceAlive(sge)) ? sge : nullptr;
}

static void* LocalPlayEdictToEntity(void* edict) {
	if (!edict || !PtrCommitted(edict)) return nullptr;
	void* sge = LocalPlayServerGameEnts();
	if (!sge) return nullptr;
	using Fn = void*(__thiscall*)(void*, void*);
	auto fn = (Fn)VGet(sge, 4); // EdictToBaseEntity
	if (!fn || !IsExec((void*)fn)) return nullptr;
	void* ent = fn(sge, edict);
	return (ent && PtrCommitted(ent)) ? ent : nullptr;
}

static void* LocalPlayEntityToEdict(void* ent) {
	if (!ent || !PtrCommitted(ent)) return nullptr;
	void* sge = LocalPlayServerGameEnts();
	if (!sge) return nullptr;
	using Fn = void*(__thiscall*)(void*, void*);
	auto fn = (Fn)VGet(sge, 3); // BaseEntityToEdict
	if (!fn || !IsExec((void*)fn)) return nullptr;
	void* ed = fn(sge, ent);
	return (ed && PtrCommitted(ed)) ? ed : nullptr;
}

// Listen-host server player only. Never guess stride/offset — that corrupted Charger AI.
static void* LocalPlayServerPlayer(int idx) {
	if (idx < 1 || idx > 64) return nullptr;
	uint8_t* g = (uint8_t*)LocalPlayGetGlobals();
	if (!g || !PtrCommitted(g + kGlobalsPEdictsOff + 4)) return nullptr;
	uint8_t* arr = *(uint8_t**)(g + kGlobalsPEdictsOff);
	if (!arr || !PtrCommitted(arr)) {
		static bool once = false;
		if (!once) { once = true; Log("localplay: pEdicts null @+0x%X", kGlobalsPEdictsOff); }
		return nullptr;
	}
	void* ed = arr + idx * kEdictStride;
	if (!PtrCommitted(ed)) return nullptr;
	void* ent = LocalPlayEdictToEntity(ed);
	if (!ent) return nullptr;
	// Cross-check: entity's edict pointer and BaseEntityToEdict must match.
	void* edFromEnt = nullptr;
	if (PtrCommitted((uint8_t*)ent + kEntEdictOff))
		edFromEnt = *(void**)((uint8_t*)ent + kEntEdictOff);
	void* edRound = LocalPlayEntityToEdict(ent);
	if (edFromEnt != ed && edRound != ed) {
		static int s_bad = 0;
		if (s_bad++ < 5)
			Log("localplay: server player reject idx=%d ed=%p ent=%p fromEnt=%p round=%p",
				idx, ed, ent, edFromEnt, edRound);
		return nullptr;
	}
	void** vt = *(void***)ent;
	if (!vt || !IsExec(vt[kSpawnVmt]) || !IsExec(vt[kSetModelVmt]))
		return nullptr;
	static bool s_ok = false;
	if (!s_ok) {
		s_ok = true;
		Log("localplay: server player ok idx=%d ent=%p pEdicts=%p stride=%d",
			idx, ent, (void*)arr, kEdictStride);
	}
	return ent;
}

static void* LocalPlayEngineServer() {
	void* es = GetIface("engine.dll", "VEngineServer022");
	if (!es) es = GetIface("engine.dll", "VEngineServer021");
	return (es && IfaceAlive(es)) ? es : nullptr;
}

static bool LocalPlayIsModelPrecached(const char* model) {
	if (!model || !model[0]) return false;
	void* es = LocalPlayEngineServer();
	if (!es) return false;
	using Fn = bool(__thiscall*)(void*, const char*);
	auto fn = (Fn)VGet(es, kEngIsModelPrecached);
	if (!fn || !IsExec((void*)fn)) return false;
	return fn(es, model);
}

static bool LocalPlayPrecacheModel(const char* model) {
	if (!model || !model[0]) return false;
	void* es = LocalPlayEngineServer();
	if (!es) return false;
	if (LocalPlayIsModelPrecached(model)) return true;
	using Fn = int(__thiscall*)(void*, const char*, bool);
	auto fn = (Fn)VGet(es, kEngPrecacheModel);
	if (!fn || !IsExec((void*)fn)) return false;
	const int idx = fn(es, model, false);
	Log("localplay: PrecacheModel '%s' -> %d", model, idx);
	return idx > 0 || LocalPlayIsModelPrecached(model);
}

static void LocalPlayPrecacheAllSurvivors(const char* why) {
	if (!g_optLocalCharChange) return;
	static const char* kModels[] = {
		"models/survivors/survivor_gambler.mdl",
		"models/survivors/survivor_producer.mdl",
		"models/survivors/survivor_coach.mdl",
		"models/survivors/survivor_mechanic.mdl",
		"models/survivors/survivor_namvet.mdl",
		"models/survivors/survivor_teenangst.mdl",
		"models/survivors/survivor_biker.mdl",
		"models/survivors/survivor_manager.mdl",
	};
	static bool s_doneThisMap = false;
	static DWORD s_lastTry = 0;
	const DWORD now = GetTickCount();
	// Once per map is enough; IsModelPrecached still costs if we spam every Listen pump.
	if (s_doneThisMap && why && strcmp(why, "force") != 0 && strcmp(why, "char-menu") != 0)
		return;
	if (why && strcmp(why, "levelinit") == 0)
		s_doneThisMap = false;
	if (now - s_lastTry < 500 && why && strcmp(why, "force") != 0)
		return;
	s_lastTry = now;
	ListenCvarWriteInt("precache_all_survivors", 1);
	int ok = 0, did = 0;
	for (int i = 0; i < 8; ++i) {
		if (LocalPlayIsModelPrecached(kModels[i])) { ++ok; continue; }
		if (LocalPlayPrecacheModel(kModels[i])) { ++ok; ++did; }
	}
	if (ok >= 8) s_doneThisMap = true;
	Log("localplay: survivor precache (%s) ok=%d/8 newly=%d", why ? why : "?", ok, did);
}

static void LocalPlayRememberChar(int prop, const char* model, const wchar_t* name) {
	g_charWantProp = prop;
	g_charWantModel[0] = 0;
	g_charWantName[0] = 0;
	if (model && model[0])
		strncpy_s(g_charWantModel, model, _TRUNCATE);
	if (name && name[0])
		wcsncpy_s(g_charWantName, name, _TRUNCATE);
}

static void LocalPlayScheduleCharReapply() {
	const DWORD now = GetTickCount();
	g_charReapplyAt = now + 80;
	g_charReapplyAt2 = now + 450;
}

// Apply to a survivor player index (local human or the idle bot that replaced them).
static bool LocalPlayChangeSurvivorAt(int idx, int prop, const char* model, const wchar_t* name, bool tip) {
	if (!ListenDetectHost()) return false;
	if (!g_optLocalCharChange) {
		if (tip) LocalPlayHpShow(L"换人物未开启（本地服页先打开开关）");
		return false;
	}
	LocalPlayResolveClientProps();
	if (idx < 1) {
		Log("localplay: change survivor bad idx");
		return false;
	}
	void* ce = EntGet(idx);
	if (!EntReadable(ce) || g_offTeam < 0 || *(int*)((uint8_t*)ce + g_offTeam) != 2) {
		Log("localplay: change survivor needs survivor team idx=%d", idx);
		if (tip) LocalPlayHpShow(L"换人物失败：需要生还者");
		return false;
	}
	// Windows: Zoey prop index crashes — use Rochelle prop with Zoey model (plugin default).
	int useProp = prop;
	if (prop == 5) useProp = 1; // ZOEY -> ROCHELLE prop

	void* se = LocalPlayServerPlayer(idx);
	if (!se) {
		Log("localplay: change survivor no server ent idx=%d", idx);
		if (tip) LocalPlayHpShow(L"换人物失败：找不到服务器实体");
		return false;
	}

	// Client RecvProp offset works on CLIENT entity only (arms/icon prediction).
	if (g_offSurvivorCharacter >= 0 && g_offSurvivorCharacter <= 0x4000) {
		int* cslot = (int*)((uint8_t*)ce + g_offSurvivorCharacter);
		if (PtrCommitted(cslot)) {
			const int clientCur = *cslot;
			*cslot = useProp;
			// Find matching field on SERVER entity (same value 0–7).
			if (g_offSurvCharServer < 0 && clientCur >= 0 && clientCur <= 7) {
				int best = -1, bestDist = 0x7fffffff;
				for (int off = 0x800; off < 0x4800; off += 4) {
					int* p = (int*)((uint8_t*)se + off);
					if (!PtrCommitted(p)) continue;
					if (*p != clientCur) continue;
					int d = off - g_offSurvivorCharacter;
					if (d < 0) d = -d;
					if (d < bestDist) { bestDist = d; best = off; }
				}
				if (best >= 0) {
					g_offSurvCharServer = best;
					Log("localplay: server survChar off=%d (clientRecv=%d cur=%d)",
						best, g_offSurvivorCharacter, clientCur);
				} else {
					Log("localplay: server survChar scan miss clientCur=%d", clientCur);
				}
			}
		}
	}
	if (g_offSurvCharServer >= 0) {
		int* sslot = (int*)((uint8_t*)se + g_offSurvCharServer);
		if (PtrCommitted(sslot))
			*sslot = useProp;
	}

	if (model && model[0]) {
		if (!LocalPlayPrecacheModel(model)) {
			Log("localplay: model not precached '%s'", model);
			if (tip) LocalPlayHpShow(L"换人物失败：模型未预缓存（关功能重进图后再开）");
			return false;
		}
		using SetModelFn = void(__thiscall*)(void*, const char*);
		auto sm = (SetModelFn)VGet(se, kSetModelVmt);
		if (sm && IsExec((void*)sm))
			sm(se, model);
		else {
			Log("localplay: SetModel missing vmt=%d", kSetModelVmt);
			if (tip) LocalPlayHpShow(L"换人物失败：SetModel不可用");
			return false;
		}
	}
	Log("localplay: survivor idx=%d -> prop=%d/%d model=%s se=%p soff=%d",
		idx, prop, useProp, model ? model : "?", se, g_offSurvCharServer);
	if (tip && name && name[0]) {
		wchar_t tipBuf[96]{};
		swprintf_s(tipBuf, 96, L"已切换人物：%s", name);
		LocalPlayHpShow(tipBuf);
	}
	return true;
}

static void LocalPlayChangeSurvivor(int prop, const char* model, const wchar_t* name) {
	LocalPlayRememberChar(prop, model, name);
	LocalPlayChangeSurvivorAt(EngLocal(), prop, model, name, true);
}

static void LocalPlayTickCharPersist() {
	if (!g_optLocalCharChange || g_charWantProp < 0 || !g_charWantModel[0]) return;
	if (!g_listenHost.load(std::memory_order_relaxed) || !EngInGame()) return;
	const DWORD now = GetTickCount();
	auto fire = [&](DWORD* slot) {
		if (!*slot || now < *slot) return;
		*slot = 0;
		const int local = EngLocal();
		if (local < 1) return;
		void* ce = EntGet(local);
		// Don't touch while idle/spectating — avoids hitching go_afk / takeover.
		if (!EntReadable(ce) || g_offTeam < 0 || *(int*)((uint8_t*)ce + g_offTeam) != 2)
			return;
		LocalPlayChangeSurvivorAt(local, g_charWantProp, g_charWantModel, nullptr, false);
	};
	fire(&g_charReapplyAt);
	fire(&g_charReapplyAt2);
}

static void LocalPlayOnCharBotSwap(void* ev, bool playerTookBot) {
	if (!ev || !g_optLocalCharChange || g_charWantProp < 0 || !g_charWantModel[0]) return;
	if (!g_listenHost.load(std::memory_order_relaxed)) return;
	RefreshLocalUserId();
	const int playerUid = EvInt(ev, "player");
	if (g_localUserId > 0 && playerUid > 0 && playerUid != g_localUserId)
		return;
	// Idle (player_bot_replace): do NOT SetModel here — that hitch made go_afk feel sluggish.
	// Takeover (bot_player_replace): engine restores stock character — re-apply after settle.
	if (playerTookBot) {
		LocalPlayScheduleCharReapply();
		Log("localplay: bot_player_replace -> reapply char prop=%d", g_charWantProp);
	}
}

// LoadScripts Pre: rewrite mission melee list before the store reads it (same as SM plugin).
static void LocalPlayOnRoundBoundary(bool start) {
	if (start) {
		memset(g_hpMax, 0, sizeof(g_hpMax));
		memset(g_hpPrev, 0, sizeof(g_hpPrev));
		memset(g_hpZc, 0, sizeof(g_hpZc));
		LocalPlayClearHpTrack();
	}
}

static void LocalPlayHpShow(const wchar_t* text) {
	if (!text || !text[0]) return;
	wcsncpy(g_hpTipLine, text, 159);
	g_hpTipLine[159] = 0;
	g_hpTipUntil = GetTickCount() + 2200;
}

static void LocalPlayBuildGauge(int nowHp, int maxHp, const wchar_t* name) {
	if (maxHp < 1) maxHp = 1;
	if (nowHp < 0) nowHp = 0;
	g_hpTrackNow = nowHp;
	g_hpTrackMax = maxHp;
	g_hpTrackUntil = GetTickCount() + 3500;
	wcsncpy(g_hpTrackName, name ? name : L"特感", 31);
	g_hpTrackName[31] = 0;
}

static void LocalPlayClearHpTrack() {
	g_hpTrackUntil = 0;
	g_hpTrackEnt = 0;
	g_hpTrackUid = 0;
	g_hpTrackNow = 0;
	g_hpTrackWitch = false;
}

static void LocalPlayMarkLocalSi(int ent) {
	if (ent <= 0) return;
	g_hpLocalSiEnt = ent;
	g_hpLocalSiUntil = GetTickCount() + 400;
}

static void LocalPlayMarkSiDead(int uid, int ent) {
	if (uid > 0) g_hpDeadUid = uid;
	if (ent > 0) g_hpDeadEnt = ent;
	g_hpDeadUntil = GetTickCount() + 3000;
	LocalPlayClearHpTrack();
}

static bool LocalPlayIsRecentDead(int uid, int ent) {
	if (!g_hpDeadUntil || GetTickCount() > g_hpDeadUntil) return false;
	if (uid > 0 && uid == g_hpDeadUid) return true;
	if (ent > 0 && ent == g_hpDeadEnt) return true;
	return false;
}

static bool LocalPlayEntIsDead(void* ent) {
	if (!ent) return true;
	if (g_offLifeState >= 0 && g_offLifeState <= 0x4000) {
		// m_lifeState: 0=alive, 1=dying, 2=dead (byte on L4D2)
		if (*(uint8_t*)((uint8_t*)ent + g_offLifeState) != 0) return true;
	}
	if (g_offHealth >= 0 && g_offHealth <= 0x4000) {
		const int hp = *(int*)((uint8_t*)ent + g_offHealth);
		if (hp <= 0) return true;
	}
	return false;
}

static float LocalPlayHeadLift(int zc) {
	switch (zc) {
	case 1: return 68.f;
	case 2: return 58.f;
	case 3: return 44.f;
	case 4: return 54.f;
	case 5: return 34.f;
	case 6: return 70.f;
	case 8: return 82.f;
	default: return 60.f;
	}
}

static bool LocalPlayWorldToScreen(const float* world, float* outSx, float* outSy, int sw, int sh) {
	if (!g_engine || !world || !outSx || !outSy || sw < 8 || sh < 8) return false;
	using MatFn = const float*(__thiscall*)(void*);
	static MatFn s_getM = nullptr;
	if (!s_getM) {
		s_getM = (MatFn)VGet(g_engine, kEngWorldToScreenMatrix);
		if (!s_getM || !IsExec((void*)s_getM)) {
			s_getM = nullptr;
			return false;
		}
	}
	const float* m = s_getM(g_engine);
	if (!m) return false;
	const float x = world[0], y = world[1], z = world[2];
	const float w = m[12] * x + m[13] * y + m[14] * z + m[15];
	if (w < 0.001f) return false;
	const float inv = 1.f / w;
	const float nx = (m[0] * x + m[1] * y + m[2] * z + m[3]) * inv;
	const float ny = (m[4] * x + m[5] * y + m[6] * z + m[7]) * inv;
	*outSx = (sw * 0.5f) + (0.5f * nx * sw) + 0.5f;
	*outSy = (sh * 0.5f) - (0.5f * ny * sh) + 0.5f;
	return *outSx > -80.f && *outSx < sw + 80.f && *outSy > -80.f && *outSy < sh + 80.f;
}

static void LocalPlayNotifySiHp(void* ent, int objectId, int newHp, int userid) {
	if (!g_optClientInfectedHp) return;
	if (!ent || objectId < 1 || objectId > 64) return;
	LocalPlayResolveClientProps();
	if (newHp < 0) newHp = 0;
	// Corpse updates often report HP=1 after death — do not reopen the bar.
	if (LocalPlayIsRecentDead(userid, objectId)) {
		g_hpPrev[objectId] = 0;
		return;
	}
	if (newHp <= 0 || LocalPlayEntIsDead(ent)) {
		LocalPlayMarkSiDead(userid > 0 ? userid : g_hpTrackUid, objectId);
		g_hpPrev[objectId] = 0;
		return;
	}
	const int zc = (g_offZombieClass >= 0) ? *(int*)((uint8_t*)ent + g_offZombieClass) : 0;
	if (g_hpZc[objectId] != zc) {
		g_hpZc[objectId] = zc;
		g_hpMax[objectId] = 0;
	}
	int entMax = (g_offMaxHealth >= 0) ? *(int*)((uint8_t*)ent + g_offMaxHealth) : 0;
	if (entMax < 1 || entMax > 10000) entMax = 0;
	if (entMax >= 1)
		g_hpMax[objectId] = entMax;
	else if (newHp > g_hpMax[objectId])
		g_hpMax[objectId] = newHp;
	if (g_hpMax[objectId] < 1)
		g_hpMax[objectId] = newHp > 0 ? newHp : 1;
	if (newHp > g_hpMax[objectId])
		g_hpMax[objectId] = newHp;
	g_hpPrev[objectId] = newHp;
	static const wchar_t* kZcName[] = {
		L"特感", L"Smoker", L"Boomer", L"Hunter", L"Spitter", L"Jockey", L"Charger", L"Witch", L"Tank"
	};
	const wchar_t* name = (zc >= 0 && zc <= 8) ? kZcName[zc] : L"特感";
	g_hpTrackEnt = objectId;
	if (userid > 0) g_hpTrackUid = userid;
	g_hpTrackWitch = false;
	g_hpTrackZc = zc;
	LocalPlayBuildGauge(newHp, g_hpMax[objectId], name);
}

static void LocalPlayOnPlayerHurtHp(void* ev) {
	if (!g_optClientInfectedHp || !ev) return;
	if (!IsLocalAttacker(ev)) return;
	LocalPlayResolveClientProps();

	const int uid = EvInt(ev, "userid");
	const int ve = EngPlayerForUserID(uid);
	if (ve < 1 || ve > 64) return;
	void* e = EntGet(ve);
	if (!EntReadable(e) || g_offTeam < 0) return;
	if (*(int*)((uint8_t*)e + g_offTeam) != 3) return;
	int nowHp = EvInt(ev, "health");
	LocalPlayMarkLocalSi(ve);
	LocalPlayNotifySiHp(e, ve, nowHp, uid);
}

static void LocalPlayOnWitchSpawn(void* ev) {
	if (!g_optClientInfectedHp || !ev) return;
	const int id = EvInt(ev, "witchid");
	if (id <= 0) return;
	int health = 1000;
	void* cv = CvarFind("z_witch_health");
	if (cv) {
		int n = 0; float f = 0.f;
		if (CvarReadPair(cv, &n, &f) && n > 0) health = n;
	}
	g_witchId[g_witchCur] = id;
	g_witchMax[g_witchCur] = health;
	g_witchHp[g_witchCur] = health;
	g_witchCur = (g_witchCur + 1) % 8;
}

static void LocalPlayOnWitchHurt(void* ev) {
	if (!g_optClientInfectedHp || !ev) return;
	if (!IsLocalAttacker(ev)) return;
	const int id = EvInt(ev, "entityid");
	const int amount = EvInt(ev, "amount");
	for (int i = 0; i < 8; ++i) {
		if (g_witchId[i] != id) continue;
		int now = g_witchHp[i] - amount;
		if (now < 0) now = 0;
		g_witchHp[i] = now;
		if (now <= 0) {
			if (g_hpTrackWitch && g_hpTrackEnt == id)
				LocalPlayClearHpTrack();
			break;
		}
		g_hpTrackEnt = id;
		g_hpTrackWitch = true;
		g_hpTrackZc = 7;
		LocalPlayBuildGauge(now, g_witchMax[i] > 0 ? g_witchMax[i] : 1000, L"Witch");
		break;
	}
}

static void LocalPlayOnWitchKilled(void* ev) {
	if (!ev) return;
	const int id = EvInt(ev, "witchid");
	for (int i = 0; i < 8; ++i) {
		if (g_witchId[i] == id) {
			g_witchId[i] = 0;
			g_witchHp[i] = -1;
			g_witchMax[i] = -1;
		}
	}
	if (g_hpTrackWitch && g_hpTrackEnt == id)
		LocalPlayClearHpTrack();
}

// player_death — hide bar at once (HP often sticks at 1, never hits 0).
static void LocalPlayOnTrackedDeath(void* ev) {
	if (!g_optClientInfectedHp || !ev) return;
	if (g_hpTrackWitch) return;
	const char* vn = EvStr(ev, "victimname");
	if (!vn || !vn[0] || !strcmp(vn, "Infected")) return;
	const int uid = EvInt(ev, "userid");
	const int ve = EngPlayerForUserID(uid);
	if (uid > 0 && g_hpTrackUid > 0 && uid == g_hpTrackUid) {
		LocalPlayMarkSiDead(uid, ve > 0 ? ve : g_hpTrackEnt);
		return;
	}
	if (ve > 0 && ve == g_hpTrackEnt) {
		LocalPlayMarkSiDead(uid, ve);
		return;
	}
	// Only one bar at a time: local kill of any SI while a bar is up → hide.
	if (g_hpTrackUntil && IsLocalAttacker(ev))
		LocalPlayMarkSiDead(uid, ve > 0 ? ve : g_hpTrackEnt);
}

static void LocalPlayPaint() {
	if (!g_optClientInfectedHp && !(g_hpTipUntil && g_hpTipLine[0])) return;
	if (g_menuVisible && !g_menuParked) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!g_engine || !EngInGame()) return;
	if (!MenuEnsureSurf()) return;
	SurfEnsureHudFont();
	if (!g_hudFont) return;

	int sw = 0, sh = 0;
	using SzFn = void(__thiscall*)(void*, int*, int*);
	static SzFn s_getSz = nullptr;
	if (!s_getSz) {
		s_getSz = (SzFn)VGet(g_surf, 35);
		if (!s_getSz || !IsExec((void*)s_getSz)) {
			s_getSz = nullptr;
			return;
		}
	}
	s_getSz(g_surf, &sw, &sh);
	if (sw < 80 || sh < 80) return;

	if (g_hpTipUntil && GetTickCount() <= g_hpTipUntil && g_hpTipLine[0]) {
		const int tw = SurfTextWFont(g_hudFont, g_hpTipLine);
		if (tw > 0 && tw < sw)
			SurfTextAt(g_hudFont, (sw - tw) / 2, 56, RGB(180, 255, 180), g_hpTipLine);
	}

	if (!g_optClientInfectedHp) return;
	if (!g_hpTrackUntil || GetTickCount() > g_hpTrackUntil) {
		if (g_hpTrackUntil && GetTickCount() > g_hpTrackUntil)
			LocalPlayClearHpTrack();
		return;
	}

	float world[3]{};
	bool have = false;
	if (g_hpTrackWitch) {
		if (g_hpTrackEnt > 0 && g_offOriginCommon >= 0 && g_offOriginCommon <= 0x4000) {
			void* e = EntGet(g_hpTrackEnt);
			if (e) {
				float* o = (float*)((uint8_t*)e + g_offOriginCommon);
				world[0] = o[0]; world[1] = o[1]; world[2] = o[2] + 64.f;
				have = true;
			}
		}
	} else if (g_hpTrackEnt >= 1 && g_hpTrackEnt <= 64 && g_offOrigin >= 0 && g_offOrigin <= 0x4000) {
		void* e = EntGet(g_hpTrackEnt);
		if (!e) {
			LocalPlayClearHpTrack();
			return;
		}
		if (g_offTeam >= 0 && *(int*)((uint8_t*)e + g_offTeam) != 3) {
			LocalPlayClearHpTrack();
			return;
		}
		if (LocalPlayEntIsDead(e)) {
			LocalPlayClearHpTrack();
			return;
		}
		if (g_offHealth >= 0) {
			int hp = *(int*)((uint8_t*)e + g_offHealth);
			if (hp >= 0 && hp <= 100000) g_hpTrackNow = hp;
			if (hp <= 0) { LocalPlayClearHpTrack(); return; }
		}
		float* o = (float*)((uint8_t*)e + g_offOrigin);
		world[0] = o[0]; world[1] = o[1]; world[2] = o[2];
		float lift = LocalPlayHeadLift(g_hpTrackZc);
		if (g_offViewOffset >= 0 && g_offViewOffset <= 0x4000) {
			float* vo = (float*)((uint8_t*)e + g_offViewOffset);
			if (vo[2] > 20.f && vo[2] < 120.f)
				lift = vo[2] + 2.f;
		}
		world[2] += lift;
		have = true;
	}
	if (!have) return;

	float sx = 0.f, sy = 0.f;
	if (!LocalPlayWorldToScreen(world, &sx, &sy, sw, sh)) return;

	const int barW = 78;
	const int barH = 7;
	const int x0 = (int)(sx - barW * 0.5f);
	const int y0 = (int)(sy - 14.f);
	if (x0 < -40 || y0 < -40 || x0 > sw || y0 > sh) return;

	int maxHp = g_hpTrackMax > 0 ? g_hpTrackMax : 1;
	int nowHp = g_hpTrackNow;
	if (nowHp < 0) nowHp = 0;
	if (nowHp > maxHp) nowHp = maxHp;
	const float t = (float)nowHp / (float)maxHp;
	int fill = (int)(barW * t + 0.5f);
	if (fill < 0) fill = 0;
	if (fill > barW) fill = barW;

	int fr = 70, fg = 210, fb = 90;
	if (t <= 0.25f) { fr = 230; fg = 55; fb = 55; }
	else if (t <= 0.5f) { fr = 240; fg = 190; fb = 50; }

	SurfColor(0, 0, 0, 160);
	SurfFill(x0 - 2, y0 - 2, x0 + barW + 2, y0 + barH + 2);
	SurfColor(35, 35, 35, 220);
	SurfFill(x0, y0, x0 + barW, y0 + barH);
	if (fill > 0) {
		SurfColor(fr, fg, fb, 235);
		SurfFill(x0, y0, x0 + fill, y0 + barH);
	}
	SurfColor(255, 255, 255, 40);
	SurfFill(x0, y0, x0 + barW, y0 + 1);

	wchar_t label[64]{};
	swprintf_s(label, 64, L"%s  %d/%d", g_hpTrackName, nowHp, maxHp);
	const int tw = SurfTextWFont(g_hudFont, label);
	const int tx = (int)(sx - tw * 0.5f);
	const int ty = y0 - (g_hudFontTall > 0 ? g_hudFontTall + 2 : 14);
	SurfColor(0, 0, 0, 180);
	SurfFill(tx - 3, ty - 1, tx + tw + 3, ty + (g_hudFontTall > 0 ? g_hudFontTall : 12) + 1);
	SurfTextAt(g_hudFont, tx, ty, RGB(255, 245, 220), label);
}

static constexpr int kDmgMax = 20;
static constexpr DWORD kDmgLifeMs = 860;
struct DmgNumSlot {
	int ent;
	int dmg;
	float x, y, z;
	float lift;
	float ang; // screen-space angle around the silhouette
	DWORD born;
	unsigned char pal;
};
static DmgNumSlot g_dmg[kDmgMax]{};
static int g_dmgN = 0;
static int g_dmgPalLast = -1;

// Bright HUD colors only — picked once at spawn, paint just reads RGB.
static const unsigned char kDmgPal[][3] = {
	{255, 64, 64},
	{255, 220, 48},
	{255, 150, 40},
	{90, 255, 100},
	{50, 230, 255},
	{255, 90, 200},
	{200, 110, 255},
	{255, 255, 255},
	{40, 255, 190},
	{255, 90, 130},
};
static constexpr int kDmgPalN = (int)(sizeof(kDmgPal) / sizeof(kDmgPal[0]));

static void ClientUxDmgReset() {
	g_dmgN = 0;
	memset(g_dmg, 0, sizeof(g_dmg));
}

static bool ClientUxDmgIsSiOrTank(void* e) {
	if (!e) return false;
	if (g_offTeam >= 0 && g_offTeam <= 0x4000) {
		if (*(int*)((uint8_t*)e + g_offTeam) != 3)
			return false;
	}
	if (g_offZombieClass >= 0 && g_offZombieClass <= 0x4000) {
		const int zc = *(int*)((uint8_t*)e + g_offZombieClass);
		if (zc == 7) return false; // witch
		if (zc < 1 || zc > 8) return false;
	}
	return true;
}

static float ClientUxDmgPickAng(int ent) {
	const DWORD now = GetTickCount();
	const unsigned h = now * 1664525u + (unsigned)ent * 1013904223u + (unsigned)g_dmgN * 747796405u;
	float ang = (float)(h & 1023) * (6.2831853f / 1024.f);
	// Screen y grows downward. 3pi/2 is straight up into the HP bar — skip that cone.
	const float up = 4.712389f;
	float d = ang - up;
	while (d > 3.1415927f) d -= 6.2831853f;
	while (d < -3.1415927f) d += 6.2831853f;
	if (d > -0.72f && d < 0.72f)
		ang = up + (d >= 0.f ? 0.72f : -0.72f);
	return ang;
}

static void ClientUxDmgPush(int ent, int dmg, const float* pos, float lift) {
	if (!g_optClientDmgNum || dmg < 1 || dmg > 9999 || !pos) return;
	const DWORD now = GetTickCount();
	for (int i = 0; i < g_dmgN; ++i) {
		if (g_dmg[i].ent == ent && now - g_dmg[i].born < 90) {
			g_dmg[i].dmg += dmg;
			if (g_dmg[i].dmg > 9999) g_dmg[i].dmg = 9999;
			g_dmg[i].x = pos[0];
			g_dmg[i].y = pos[1];
			g_dmg[i].z = pos[2];
			g_dmg[i].lift = lift;
			return;
		}
	}
	if (g_dmgN >= kDmgMax) {
		memmove(g_dmg, g_dmg + 1, (kDmgMax - 1) * sizeof(DmgNumSlot));
		g_dmgN = kDmgMax - 1;
	}
	g_dmg[g_dmgN].ent = ent;
	g_dmg[g_dmgN].dmg = dmg;
	g_dmg[g_dmgN].x = pos[0];
	g_dmg[g_dmgN].y = pos[1];
	g_dmg[g_dmgN].z = pos[2];
	g_dmg[g_dmgN].lift = lift;
	g_dmg[g_dmgN].ang = ClientUxDmgPickAng(ent);
	g_dmg[g_dmgN].born = now;
	int pick = (int)((now * 1664525u + (DWORD)ent * 17u + (DWORD)g_dmgN * 31u) % (unsigned)kDmgPalN);
	if (pick == g_dmgPalLast)
		pick = (pick + 1 + (int)(now & 3)) % kDmgPalN;
	g_dmgPalLast = pick;
	g_dmg[g_dmgN].pal = (unsigned char)pick;
	++g_dmgN;
}

static bool ClientUxDmgReadHead(int ent, void* known, float* out, float* liftOut) {
	if (!out || ent <= 0) return false;
	void* e = known ? known : EntGet(ent);
	if (!e || g_offOrigin < 0 || g_offOrigin > 0x4000) return false;
	float* o = (float*)((uint8_t*)e + g_offOrigin);
	out[0] = o[0]; out[1] = o[1]; out[2] = o[2];
	int zc = 0;
	if (g_offZombieClass >= 0 && g_offZombieClass <= 0x4000)
		zc = *(int*)((uint8_t*)e + g_offZombieClass);
	float lift = LocalPlayHeadLift(zc);
	if (g_offViewOffset >= 0 && g_offViewOffset <= 0x4000) {
		float* vo = (float*)((uint8_t*)e + g_offViewOffset);
		if (vo[2] > 20.f && vo[2] < 120.f)
			lift = vo[2] + 2.f;
	}
	if (liftOut) *liftOut = lift;
	return true;
}

static void ClientUxDmgOnSi(void* ent, int objectId, int dmg) {
	if (!g_optClientDmgNum || dmg < 1) return;
	if (ent && !ClientUxDmgIsSiOrTank(ent)) return;
	float p[3]{};
	float lift = 0.f;
	if (!ClientUxDmgReadHead(objectId, ent, p, &lift)) return;
	ClientUxDmgPush(objectId, dmg, p, lift);
}

static void ClientUxOnDmgPlayerHurt(void* ev) {
	if (!g_optClientDmgNum || !ev) return;
	if (!IsLocalAttacker(ev)) return;
	int dmg = EvInt(ev, "dmg_health");
	if (dmg < 1) return;
	const int uid = EvInt(ev, "userid");
	const int ve = EngPlayerForUserID(uid);
	if (ve < 1 || ve > 64) return;
	void* e = EntGet(ve);
	if (!ClientUxDmgIsSiOrTank(e)) return;
	float p[3]{};
	float lift = 0.f;
	if (!ClientUxDmgReadHead(ve, e, p, &lift)) return;
	ClientUxDmgPush(ve, dmg, p, lift);
}

static void ClientUxPaintDmgNums() {
	if (!g_optClientDmgNum) {
		if (g_dmgN) ClientUxDmgReset();
		return;
	}
	if (g_menuVisible && !g_menuParked) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame()) return;
	if (!MenuEnsureSurf()) return;
	SurfEnsureDmgFont();
	if (!g_dmgFont) {
		SurfEnsureHudFont();
		if (!g_hudFont) return;
	}
	if (g_dmgN <= 0) return;

	int sw = 0, sh = 0;
	using SzFn = void(__thiscall*)(void*, int*, int*);
	static SzFn s_getSz = nullptr;
	if (!s_getSz) {
		s_getSz = (SzFn)VGet(g_surf, 35);
		if (!s_getSz || !IsExec((void*)s_getSz)) {
			s_getSz = nullptr;
			return;
		}
	}
	s_getSz(g_surf, &sw, &sh);
	if (sw < 80 || sh < 80) return;

	const DWORD now = GetTickCount();
	int w = 0;
	for (int i = 0; i < g_dmgN; ++i) {
		if (now - g_dmg[i].born >= kDmgLifeMs) continue;
		if (g_dmg[i].ent > 0 && g_offOrigin >= 0 && g_offOrigin <= 0x4000) {
			void* e = EntGet(g_dmg[i].ent);
			if (e) {
				float* o = (float*)((uint8_t*)e + g_offOrigin);
				g_dmg[i].x = o[0];
				g_dmg[i].y = o[1];
				g_dmg[i].z = o[2];
			}
		}
		g_dmg[w++] = g_dmg[i];
	}
	g_dmgN = w;
	if (g_dmgN <= 0) return;

	const unsigned long font = g_dmgFont ? g_dmgFont : g_hudFont;
	using FontFn = void(__thiscall*)(void*, unsigned long);
	using ColFn = void(__thiscall*)(void*, int, int, int, int);
	using PosFn = void(__thiscall*)(void*, int, int);
	using PrintFn = void(__thiscall*)(void*, const wchar_t*, int, int);
	auto setFont = (FontFn)VGet(g_surf, 17);
	auto setCol = (ColFn)VGet(g_surf, 19);
	auto setPos = (PosFn)VGet(g_surf, 20);
	auto print = (PrintFn)VGet(g_surf, 22);
	if (!setFont || !setCol || !setPos || !print) return;
	setFont(g_surf, font);

	for (int i = 0; i < g_dmgN; ++i) {
		const float t = (float)(now - g_dmg[i].born) / (float)kDmgLifeMs;
		int a = 255;
		if (t > 0.55f) {
			a = (int)(255.f * (1.f - (t - 0.55f) / 0.45f));
			if (a < 0) a = 0;
		}
		if (a < 18) continue;

		float feet[3] = { g_dmg[i].x, g_dmg[i].y, g_dmg[i].z };
		float head[3] = { g_dmg[i].x, g_dmg[i].y, g_dmg[i].z + g_dmg[i].lift };
		float fsx = 0.f, fsy = 0.f, hsx = 0.f, hsy = 0.f;
		const bool gotF = LocalPlayWorldToScreen(feet, &fsx, &fsy, sw, sh);
		const bool gotH = LocalPlayWorldToScreen(head, &hsx, &hsy, sw, sh);
		if (!gotF && !gotH) continue;
		if (!gotF) { fsx = hsx; fsy = hsy + 36.f; }
		if (!gotH) { hsx = fsx; hsy = fsy - 36.f; }

		const float cx = (fsx + hsx) * 0.5f;
		const float cy = (fsy + hsy) * 0.5f;
		float bh = fsy - hsy;
		if (bh < 28.f) bh = 28.f;
		float bw = bh * 0.42f;
		if (bw < 22.f) bw = 22.f;
		const float ang = g_dmg[i].ang;
		const float rx = bw * 0.5f + 20.f;
		const float ry = bh * 0.5f + 14.f;
		float sx = cx + cosf(ang) * rx;
		float sy = cy + sinf(ang) * ry - 10.f * t;

		wchar_t txt[16]{};
		swprintf_s(txt, 16, L"%d", g_dmg[i].dmg);
		const int len = (int)wcslen(txt);
		const int th = (g_dmgFontTall > 0 ? g_dmgFontTall : 20);
		const int tw = len * (th * 3 / 5);
		int tx = (int)(sx + 0.5f) - tw / 2;
		int ty = (int)(sy + 0.5f);

		if (g_optClientInfectedHp && !g_hpTrackWitch && g_hpTrackEnt == g_dmg[i].ent
			&& g_hpTrackUntil && now <= g_hpTrackUntil) {
			const int barW = 78;
			const int barH = 7;
			const int barX0 = (int)(hsx - barW * 0.5f);
			const int barY0 = (int)(hsy - 14.f);
			const int labH = (g_hudFontTall > 0 ? g_hudFontTall : 12) + 4;
			const int avoidL = barX0 - 6;
			const int avoidR = barX0 + barW + 6;
			const int avoidT = barY0 - labH - 4;
			const int avoidB = barY0 + barH + 4;
			if (tx + tw > avoidL && tx < avoidR && ty + th > avoidT && ty < avoidB) {
				if (cx < hsx)
					tx = avoidL - tw - 2;
				else
					tx = avoidR + 2;
			}
		}

		const int pi = (g_dmg[i].pal < (unsigned char)kDmgPalN) ? (int)g_dmg[i].pal : 0;
		const int fr = kDmgPal[pi][0];
		const int fg = kDmgPal[pi][1];
		const int fb = kDmgPal[pi][2];
		setCol(g_surf, 0, 0, 0, a);
		setPos(g_surf, tx + 1, ty + 1);
		print(g_surf, txt, len, 0);
		setCol(g_surf, fr, fg, fb, a);
		setPos(g_surf, tx, ty);
		print(g_surf, txt, len, 0);
	}
}

static void LocalPlayRestore(const char* why) {
	g_offSurvCharServer = -1;
	// HP bar is client UX now — do not clear it when leaving listen-host localplay.
	g_hpTipUntil = 0;
	g_hpTipLine[0] = 0;
	if (why && why[0])
		Log("localplay: restored (%s)", why);
}

static void LocalPlaySync(const char* why) {
	if (!ListenDetectHost()) {
		LocalPlayRestore(why ? why : "not-host");
		return;
	}
	if (!LocalPlayOptsWanted()) {
		LocalPlayRestore("opts-off");
		return;
	}
	LocalPlayResolveClientProps();
	Log("localplay: sync (%s) char=%d",
		why ? why : "?", g_optLocalCharChange ? 1 : 0);
}

static bool ListenIsHost() {
	return g_listenHost.load(std::memory_order_relaxed);
}

static int ListenEngineTickGuess() {
	if (g_hostTickInterval && PtrCommitted(g_hostTickInterval)) {
		const float v = *g_hostTickInterval;
		if (v >= 0.007f && v <= 0.05f)
			return (int)(1.f / v + 0.5f);
	}
	return 30;
}

static int ListenCycleTick(int cur) {
	if (cur == 30) return 60;
	if (cur == 60) return 100;
	if (cur == 100) return 128;
	return 30;
}

static int ListenCycleLerp(int cur) {
	static const int kMs[] = { -1, 0, 24, 40, 60, 100 };
	int idx = 0;
	for (int i = 0; i < 6; ++i) {
		if (kMs[i] == cur) { idx = i; break; }
	}
	return kMs[(idx + 1) % 6];
}

static int ListenCycleAa(int cur) {
	if (cur == 10) return 100;
	if (cur == 100) return 400;
	if (cur == 400) return 1000;
	return 10;
}

static int ListenReadAirAccel() {
	void* var = CvarFind("sv_airaccelerate");
	if (!var) return -1;
	int n = 0;
	float f = 0.f;
	if (!CvarReadPair(var, &n, &f)) return -1;
	if (f > 0.5f && f < 100000.f) return (int)(f + 0.5f);
	if (n > 0 && n < 100000) return n;
	return -1;
}

static void XhairSnapshotUserCvars() {
	g_xhairUserCvars.alpha = CvarGetIntRange("cl_crosshair_alpha", 0, 255, -1);
	g_xhairUserCvars.red = CvarGetIntRange("cl_crosshair_red", 0, 255, -1);
	g_xhairUserCvars.green = CvarGetIntRange("cl_crosshair_green", 0, 255, -1);
	g_xhairUserCvars.blue = CvarGetIntRange("cl_crosshair_blue", 0, 255, -1);
	g_xhairUserCvars.dynamic = CvarGetIntRange("cl_crosshair_dynamic", 0, 1, -1);
	g_xhairUserCvars.circleMode = CvarGetIntRange("cl_crosshair_circle_mode", 0, 2, -1);
	g_xhairUserCvars.circleAlpha = CvarGetIntRange("cl_crosshair_circle_alpha", 0, 255, -1);
	g_xhairUserCvars.valid = true;
	Log("xhair snap user alpha=%d rgb=%d,%d,%d dyn=%d circle=%d/%d",
		g_xhairUserCvars.alpha, g_xhairUserCvars.red, g_xhairUserCvars.green, g_xhairUserCvars.blue,
		g_xhairUserCvars.dynamic, g_xhairUserCvars.circleMode, g_xhairUserCvars.circleAlpha);
}

static void XhairRestoreUserCvars() {
	if (!g_xhairUserCvars.valid) return;
	if (g_xhairUserCvars.circleAlpha >= 0)
		CvarSetInt("cl_crosshair_circle_alpha", g_xhairUserCvars.circleAlpha);
	if (g_xhairUserCvars.circleMode >= 0)
		CvarSetInt("cl_crosshair_circle_mode", g_xhairUserCvars.circleMode);
	if (g_xhairUserCvars.alpha >= 0)
		CvarSetInt("cl_crosshair_alpha", g_xhairUserCvars.alpha);
	if (g_xhairUserCvars.red >= 0)
		CvarSetInt("cl_crosshair_red", g_xhairUserCvars.red);
	if (g_xhairUserCvars.green >= 0)
		CvarSetInt("cl_crosshair_green", g_xhairUserCvars.green);
	if (g_xhairUserCvars.blue >= 0)
		CvarSetInt("cl_crosshair_blue", g_xhairUserCvars.blue);
	if (g_xhairUserCvars.dynamic >= 0)
		CvarSetInt("cl_crosshair_dynamic", g_xhairUserCvars.dynamic);
	Log("xhair restore user cvars");
}

static void ClearHitOverlay(); // defined later

static const char* NormSoundSample(const char* relativePath) {
	if (!relativePath || !relativePath[0]) return "";
	if (!_strnicmp(relativePath, "sound/", 6))
		return relativePath + 6;
	return relativePath;
}

// IRecipientFilter (MSVC vtable[0]=dtor) — local player only.
struct SkRcptFilter {
	virtual ~SkRcptFilter() {}
	virtual bool IsReliable() { return false; }
	virtual bool IsInitMessage() { return false; }
	virtual int GetRecipientCount() { return 1; }
	virtual int GetRecipientIndex(int) {
		const int local = g_engine ? EngLocal() : 1;
		return local > 0 ? local : 1;
	}
};

// EmitSound(CHAN_STATIC family, ATTN_NONE): same SFX bus as gunfire UI, full loudness at 1.0.
// EmitAmbientSound was much quieter (ambient ducking in L4D2 combat).
static bool EngEmitLocalSfx(const char* sample, float vol) {
	if (!g_emitSoundOk || !g_engineSound || !sample || !sample[0]) return false;
	using Fn = void(__thiscall*)(void*, void*, int, int, const char*,
		float, float, int, int,
		const float*, const float*, void*, bool, float, int);
	auto fn = (Fn)VGet(g_engineSound, g_emitSoundIdx);
	if (!fn || !IsExec((void*)fn)) {
		g_emitSoundOk = false;
		return false;
	}
	if (vol < 0.f) vol = 0.f;
	if (vol > 1.f) vol = 1.f;

	SkRcptFilter filter;
	// Rotate user channels so rapid clips don't stomp each other / melee BODY channel.
	static unsigned s_rot = 0;
	const int chan = 136 + (int)(s_rot++ & 3); // CHAN_USER_BASE .. +3

	fn(g_engineSound, &filter,
		-1,   // SOUND_FROM_LOCAL_PLAYER
		chan,
		sample,
		vol,
		0.0f, // ATTN_NONE
		0,
		100,  // PITCH_NORM
		nullptr, nullptr, nullptr,
		true,
		0.0f,
		-1);
	return true;
}

static void PtWarmEnqueue(const char* name, bool /*worldSpace*/) {
	if (!name || !name[0] || g_ptWarmCount >= kPtWarmCap) return;
	for (int i = 0; i < g_ptWarmCount; ++i) {
		if (!_stricmp(g_ptWarmName[i], name))
			return;
	}
	strncpy(g_ptWarmName[g_ptWarmCount], name, sizeof(g_ptWarmName[0]) - 1);
	g_ptWarmName[g_ptWarmCount][sizeof(g_ptWarmName[0]) - 1] = 0;
	++g_ptWarmCount;
}

static void PtWarmEnqueueFx(const DlcFx* fx) {
	if (!fx || !fx->used) return;
	if (fx->particle[0])
		PtWarmEnqueue(fx->particle, fx->worldSpace);
	for (int i = 0; i < fx->particleExtraCount; ++i) {
		if (fx->particleExtra[i][0])
			PtWarmEnqueue(fx->particleExtra[i], fx->worldSpace);
	}
}

static void PtWarmEnqueueStyle(const DlcStyle* st) {
	if (!st) return;
	PtWarmEnqueueFx(&st->hit);
	PtWarmEnqueueFx(&st->kill);
	PtWarmEnqueueFx(&st->headshot);
	PtWarmEnqueueFx(&st->melee);
	for (int i = 1; i <= 11; ++i) {
		if (st->streakSlot[i])
			PtWarmEnqueueFx(&st->streak[i]);
	}
	for (int i = 0; i < st->namedCount; ++i)
		PtWarmEnqueueFx(&st->named[i].fx);
}

// Full DIY catalog → PrecacheParticleSystem. Call on the game thread
// (LevelInitPreEntity). Do not require FindParticleSystem first: DIY names
// often are not in the mgr yet, and skipping left combat Dispatch as a no-op
// unless the optional loading Dispatch warm had already registered them.
static void PrecacheAllCatalogParticles() {
	if (!g_precache || !IsExec((void*)g_precache)) {
		Log("particle precache skip: PrecacheParticleSystem unavailable");
		return;
	}

	g_ptWarmCount = 0;
	const DlcCatalog* cat = DlcGetCatalog();
	if (cat) {
		for (int i = 0; i < cat->count; ++i)
			PtWarmEnqueueStyle(&cat->styles[i]);
	}

	int done = 0;
	int known = 0;
	for (int i = 0; i < g_ptWarmCount; ++i) {
		const char* name = g_ptWarmName[i];
		if (!name[0]) continue;
		// FindParticleSystem != usable this map. Necola still calls
		// PrecacheParticleSystemOffset after a successful find; skipping it
		// made combat Dispatch a no-op (queued=N precached=0 already-in-mgr=N).
		if (g_findParticle && g_particleMgr && IsExec((void*)g_findParticle)
			&& g_findParticle(g_particleMgr, name))
			++known;
		g_precache(name);
		++done;
	}
	Log("particle precache (all catalog): queued=%d precached=%d already-in-mgr=%d styles=%d",
		g_ptWarmCount, done, known, cat ? cat->count : 0);
}

static void PtDispWarmEnqueue(const char* name) {
	if (!name || !name[0] || g_ptDispWarmCount >= kPtDispWarmCap) return;
	for (int i = 0; i < g_ptDispWarmCount; ++i) {
		if (!_stricmp(g_ptDispWarmName[i], name))
			return;
	}
	strncpy(g_ptDispWarmName[g_ptDispWarmCount], name, sizeof(g_ptDispWarmName[0]) - 1);
	g_ptDispWarmName[g_ptDispWarmCount][sizeof(g_ptDispWarmName[0]) - 1] = 0;
	++g_ptDispWarmCount;
}

static void PtDispWarmEnqueueFx(const DlcFx* fx) {
	if (!fx || !fx->used) return;
	if (fx->particle[0])
		PtDispWarmEnqueue(fx->particle);
	for (int i = 0; i < fx->particleExtraCount; ++i) {
		if (fx->particleExtra[i][0])
			PtDispWarmEnqueue(fx->particleExtra[i]);
	}
}

static void PtDispWarmEnqueueStyle(const DlcStyle* st) {
	if (!st) return;
	PtDispWarmEnqueueFx(&st->hit);
	PtDispWarmEnqueueFx(&st->kill);
	PtDispWarmEnqueueFx(&st->headshot);
	PtDispWarmEnqueueFx(&st->melee);
	for (int i = 1; i <= 11; ++i) {
		if (st->streakSlot[i])
			PtDispWarmEnqueueFx(&st->streak[i]);
	}
	for (int i = 0; i < st->namedCount; ++i)
		PtDispWarmEnqueueFx(&st->named[i].fx);
}

// Build selected-style particle name list (ci/si/ff/fx + built-in kill-fx names).
static void PtDispWarmBuildSelected() {
	g_ptDispWarmCount = 0;

	static const char* kCh[] = { "ci", "si", "ff", "fx" };
	for (int c = 0; c < 4; ++c) {
		const char* id = DlcGetSelected(kCh[c]);
		if (!id || !id[0] || !_stricmp(id, "off"))
			continue;
		PtDispWarmEnqueueStyle(DlcFindStyle(id));
	}

	if (g_optKillFx) {
		static const char* kKillFx[] = {
			"skeet", "melee_skeet", "headshot_skeet",
			"crown", "perfect_crown", "melee_crown",
			"crit_text", "level", "tongue_cut",
			nullptr
		};
		for (int i = 0; kKillFx[i]; ++i)
			PtDispWarmEnqueue(kKillFx[i]);
	}
}

static bool g_loadWarmDone = false;

static void PtDispWarmFlushAtLoading(const char* mapName) {
	if (!SkeetoFeaturesOn()) {
		Log("particle load-time skip (sv_pure=2) map=%s", mapName ? mapName : "?");
		return;
	}
	if (!g_optPtDispWarm) {
		Log("particle disp warm skipped (menu off) map=%s", mapName ? mapName : "?");
		return;
	}
	if (!g_dispatch || !IsExec((void*)g_dispatch)) return;
	PtDispWarmBuildSelected();
	if (g_ptDispWarmCount <= 0) {
		Log("particle disp warm at LevelInit: nothing queued map=%s", mapName ? mapName : "?");
		return;
	}

	int done = 0;
	for (int i = 0; i < g_ptDispWarmCount; ++i) {
		const char* name = g_ptDispWarmName[i];
		if (!name[0]) continue;
		if (g_precache && IsExec((void*)g_precache))
			g_precache(name);
		g_fxOrigin[0] = -8192.f;
		g_fxOrigin[1] = -8192.f;
		g_fxOrigin[2] = -8192.f;
		g_fxAngles[0] = 0.f;
		g_fxAngles[1] = 0.f;
		g_fxAngles[2] = 0.f;
		g_dispatch(name, g_fxOrigin, g_fxAngles, 2, 0, 0);
		++done;
	}
	Log("particle disp warm at LevelInitPreEntity map=%s count=%d",
		mapName ? mapName : "?", done);
}

static bool VtProtectWrite(void** slot, void* neu, void** prevOut) {
	if (!slot) return false;
	DWORD old = 0;
	if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &old))
		return false;
	if (prevOut) *prevOut = *slot;
	*slot = neu;
	DWORD tmp = 0;
	VirtualProtect(slot, sizeof(void*), old, &tmp);
	return true;
}

// Do NOT hook LevelShutdown (exit dialog). Catalog is loaded once on the game thread
// (Paint / first Ensure). LevelInit only registers particle *names* via
// PrecacheParticleSystem (same as Necola) — FindParticleSystem is not enough.
// Overlay / VTF: no warmup; r_screenoverlay at combat time is enough.
static bool __fastcall Hooked_ServerLevelInit(void* ecx, void* edx,
	const char* map, const char* ents, const char* oldLevel, const char* landmark,
	bool loadGame, bool background) {
	g_listenMapHost.store(true, std::memory_order_relaxed);
	Log("listen: ServerGameDLL LevelInit map=%s (this process is host)", map ? map : "?");
	if (g_run.load(std::memory_order_relaxed) && ListenOptsWanted())
		ListenApplyServerBoot("server-levelinit");
	if (g_origServerLevelInit)
		return g_origServerLevelInit(ecx, edx, map, ents, oldLevel, landmark, loadGame, background);
	return true;
}

static void __fastcall Hooked_ServerLevelShutdown(void* ecx, void* edx) {
	if (g_run.load(std::memory_order_relaxed)) {
		ListenCsEnsure();
		EnterCriticalSection(&g_listenCs);
		ListenWriteHostTick(g_optLocalTick, false);
		LeaveCriticalSection(&g_listenCs);
	}
	if (g_origServerLevelShutdown)
		g_origServerLevelShutdown(ecx, edx);
}

static void EnsureServerGameHooks() {
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!GetModuleHandleA("server.dll")) return;
	if (!g_hookServerLevelInit)
		g_hookServerLevelInit = (void*)&Hooked_ServerLevelInit;
	if (!g_hookServerLevelShutdown)
		g_hookServerLevelShutdown = (void*)&Hooked_ServerLevelShutdown;
	void* sg = GetIface("server.dll", "ServerGameDLL005");
	if (!sg || !IfaceAlive(sg)) return;
	void** vt = *(void***)sg;
	if (!vt) return;
	if (vt[2] != g_hookServerLevelInit) {
		void* prev = nullptr;
		if (VtProtectWrite(&vt[2], g_hookServerLevelInit, &prev)) {
			if (prev && prev != (void*)g_origServerLevelInit)
				g_origServerLevelInit = (ServerLevelInitFn)prev;
		}
	}
	if (vt[6] != g_hookServerLevelShutdown) {
		void* prev = nullptr;
		if (VtProtectWrite(&vt[6], g_hookServerLevelShutdown, &prev)) {
			if (prev && prev != (void*)g_origServerLevelShutdown)
				g_origServerLevelShutdown = (ServerLevelShutdownFn)prev;
		}
	}
	if (!g_serverLevelHookLogged && g_origServerLevelInit) {
		g_serverLevelHookLogged = true;
		Log("ServerGameDLL005 hooked LevelInit orig=%p LevelShutdown orig=%p",
			(void*)g_origServerLevelInit, (void*)g_origServerLevelShutdown);
	}
}

static void __fastcall Hooked_LevelInitPreEntity(void* ecx, void* edx, const char* mapName) {
	if (g_origLevelInitPre)
		g_origLevelInitPre(ecx, edx, mapName);

	if (!g_run.load(std::memory_order_relaxed) || !g_ready.load(std::memory_order_relaxed))
		return;
	ListenOnLevelInit();
	if (!GetModuleHandleA("client.dll") || !GetModuleHandleA("engine.dll"))
		return;

	CrashMark(kBcLevelInit);
	ElimOnChapterLoad(mapName);

	if (SkeetoFeaturesOn()) {
		const DWORD t0 = GetTickCount();
		DlcEnsureCatalog(g_gameL4d2Dir, false);
		PrecacheAllCatalogParticles();
		g_loadWarmDone = true;
		Log("LevelInit catalog+precache dt=%ums styles=%d loose=%d GAME=%d catalogMs=%d",
			GetTickCount() - t0,
			DlcGetCatalog()->count,
			DlcLastLooseJsonCount(),
			DlcLastEngineJsonCount(),
			DlcLastReloadMs());
		PtDispWarmFlushAtLoading(mapName);
	}
	g_xhairHudMode = -1; // hidden circle cvars reset on map change
	if (g_optClientNoCorpseSi || g_optClientNoCorpseCi)
		ClientUxApplyNoCorpseCvars(true);
	ClientUxApplyDirectorHud(true);
	ClientUxDmgReset();
	HudHideInvalidate();
	g_teamHudAllowModelAt = GetTickCount() + 20000;
	RoundTimerOnStart();
	g_xhairHideUntil = GetTickCount() + 400;
	g_xhairHideScene.store(true, std::memory_order_relaxed);
}

// Source ButtonCode_t (const.h): KEY_NONE=0, KEY_0=1 ... KEY_ESCAPE=70
static DWORD ButtonCodeToVk(int keynum) {
	if (keynum >= 1 && keynum <= 10) return (DWORD)('0' + (keynum - 1));
	if (keynum >= 11 && keynum <= 36) return (DWORD)('A' + (keynum - 11));
	if (keynum >= 37 && keynum <= 46) return (DWORD)(VK_NUMPAD0 + (keynum - 37));
	if (keynum == 49) return VK_SUBTRACT;
	if (keynum == 50) return VK_ADD;
	if (keynum == 53) return VK_OEM_4;     // [
	if (keynum == 62) return VK_OEM_MINUS; // -
	if (keynum == 63) return VK_OEM_PLUS;  // =
	if (keynum == 70) return VK_ESCAPE;
	return 0;
}

static void InstallMenuKeyEatOnce() {
	if (g_inKeyEatInstalled)
		return;
	if (!g_run.load(std::memory_order_relaxed))
		return;
	void* bc = GetIface("client.dll", "VClient016");
	if (!bc)
		return;
	void** vt = *(void***)bc;
	if (!vt)
		return;
	void* const ours = (void*)&Hooked_InKeyEvent;
	void* slot = vt[kVmtInKeyEvent];
	if (slot == ours) {
		g_inKeyEatInstalled = true;
		return;
	}
	if (!slot || !IsExec(slot))
		return;

	// NCL MinHooks CHLClient::IN_KeyEvent (VMT pointer unchanged, first byte becomes E9).
	// We must chain AFTER that, and we must NOT MinHook the same address (relative JMP
	// in a second trampoline would jump to the wrong place).
	HMODULE ncl = GetModuleHandleA("necola_orig.dll");
	if (ncl) {
		const uint8_t op = *(const uint8_t*)slot;
		if (op != 0xE9 && op != 0xEB && op != 0xFF) {
			if (!g_inKeyEatWaitAt)
				g_inKeyEatWaitAt = GetTickCount();
			if (GetTickCount() - g_inKeyEatWaitAt < 5000)
				return;
			Log("IN_KeyEvent: Necola jmp not seen (op=%02X) — chain anyway", op);
		}
	}

	void* prev = nullptr;
	if (!VtProtectWrite(&vt[kVmtInKeyEvent], ours, &prev))
		return;
	if (!prev || prev == ours) {
		Log("IN_KeyEvent chain abort orig=%p — restore", prev);
		if (prev != ours)
			VtProtectWrite(&vt[kVmtInKeyEvent], prev, nullptr);
		return;
	}
	g_origInKeyEvent = (InKeyEventFn)prev;
	if (!g_origInKeyEventPrimary || (void*)g_origInKeyEventPrimary == ours)
		g_origInKeyEventPrimary = (InKeyEventFn)prev;
	g_hookInKeyEvent = ours;
	g_inKeyEatInstalled = true;
	g_keyEventHookLogged = true;
	Log("IN_KeyEvent chained once orig=%p ncl=%d (eat menu keys only while menu open)",
		prev, ncl ? 1 : 0);
}

// Hide stock HUD in pieces. Never set WEAPONSELECTION / NEEDSUIT / PLAYERDEAD /
// INVEHICLE / HIDEHUD_ALL — those make slot1–5 stop working.
// Team: hidehud HEALTH+MISC. Weapon ammo / itempickup radar: VGUI SetVisible
// on named panels (does not trip ShouldDraw / SelectSlot).
static constexpr int kHideHudMask =
	(1 << 3) |  // HEALTH
	(1 << 6);   // MISCSTATUS
static constexpr int kHideHudStrip =
	(1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) |
	(1 << 9) | (1 << 10) | (1 << 11);
static int g_hideHudSaved = 0;
static bool g_hideHudPinned = false;
static bool g_hideHudDidStrip = false;
static bool g_hideHudLogged = false;
static void* g_hideHudVar = nullptr;
static void* g_ipanel = nullptr;
static constexpr int kIPanelSetPos = 2;
static constexpr int kIPanelGetPos = 3;
static constexpr int kIPanelGetSize = 5;
static constexpr int kIPanelSetVisible = 14;
static constexpr int kIPanelGetChildCount = 17;
static constexpr int kIPanelGetChild = 18;
static constexpr int kIPanelGetParent = 20;
static constexpr int kIPanelGetName = 36;
static constexpr int kIPanelGetClassName = 37;
static constexpr int kSurfGetEmbeddedPanel = 6;
static constexpr int kVmtEngineGetPanel = 1;
static constexpr int kPanelClientDll = 2;
static constexpr int kHudVguiBagMax = 8;
static constexpr int kHudVguiKindTeam = 1;
static constexpr int kHudVguiKindWep = 2;
static constexpr int kHudVguiKindPickup = 3;

static void HudHidePokeCvar(int v) {
	if (!g_hideHudVar)
		g_hideHudVar = CvarFind("hidehud");
	void* var = g_hideHudVar;
	if (!var) return;
	const int flags = CvarFlagsOf(var);
	if (flags & (kFCvarCheat | kFCvarDevOnly | kFCvarHidden))
		CvarWriteFlags(var, flags & ~(kFCvarCheat | kFCvarDevOnly | kFCvarHidden));
	if (!CvarPokeFloatValue(var, (float)v))
		CvarPokePair(var, v, (float)v);
}

static void HudHideStripLocalBits() {
	if (g_offHideHUD < 0 || g_offHideHUD > 0x4000) return;
	if (!g_entlist || !g_engine) return;
	const int local = EngLocal();
	if (local <= 0) return;
	void* me = EntGet(local);
	if (!EntReadable(me)) return;
	int* p = (int*)((uint8_t*)me + g_offHideHUD);
	if (!PtrCommitted(p)) return;
	const int cur = *p;
	const int next = cur & ~kHideHudStrip;
	if (next == cur) return;
	*p = next;
	if (!g_hideHudLogged)
		Log("hud hide strip m_iHideHUD %d->%d off=%d", cur, next, g_offHideHUD);
}

static void HudHideStripOnce() {
	if (g_hideHudDidStrip) return;
	HudHideStripLocalBits();
	g_hideHudDidStrip = true;
}

static void HudHideRestore() {
	if (g_hideHudPinned) {
		HudHidePokeCvar(g_hideHudSaved);
		g_hideHudPinned = false;
	}
}

static void HudHideSyncMaster() {
	g_optHudHide = g_optHudHideTeam || g_optHudHideWep || g_optHudHidePickup;
}

struct HudVguiBag {
	unsigned int pan[kHudVguiBagMax];
	int n;
	int x[kHudVguiBagMax], y[kHudVguiBagMax], w[kHudVguiBagMax], h[kHudVguiBagMax];
	bool have[kHudVguiBagMax];
};
static HudVguiBag g_hvTeam{}, g_hvWep{}, g_hvPickup{};
static DWORD g_hvSearchAt = 0;
static bool g_hvLoggedKids = false;
static bool g_hvLoggedFound = false;

static bool HudVguiReady() {
	if (!g_ipanel)
		g_ipanel = GetIface("vgui2.dll", "VGUI_Panel009");
	return g_ipanel && IfaceAlive(g_ipanel);
}

static unsigned int HudVguiEnginePanel(int type) {
	void* ev = g_engineVgui;
	if (!ev || !IfaceAlive(ev)) return 0;
	using Fn = unsigned int(__thiscall*)(void*, int);
	auto fn = (Fn)VGet(ev, kVmtEngineGetPanel);
	if (!fn || !IsExec((void*)fn)) return 0;
	return fn(ev, type);
}

static unsigned int HudVguiEmbedded() {
	if (!g_surf)
		g_surf = GetIface("vguimatsurface.dll", "VGUI_Surface031");
	if (!g_surf || !IfaceAlive(g_surf)) return 0;
	using Fn = unsigned int(__thiscall*)(void*);
	auto fn = (Fn)VGet(g_surf, kSurfGetEmbeddedPanel);
	if (!fn || !IsExec((void*)fn)) return 0;
	return fn(g_surf);
}

static const char* HudVguiSafeStr(const char* n) {
	if (!n || !PtrCommitted(n)) return "";
	if ((unsigned char)n[0] < 32 && n[0] != 0) return "";
	return n;
}

static const char* HudVguiName(unsigned int p) {
	if (!p || !g_ipanel) return "";
	using Fn = const char*(__thiscall*)(void*, unsigned int);
	auto fn = (Fn)VGet(g_ipanel, kIPanelGetName);
	if (!fn || !IsExec((void*)fn)) return "";
	return HudVguiSafeStr(fn(g_ipanel, p));
}

static const char* HudVguiClass(unsigned int p) {
	if (!p || !g_ipanel) return "";
	using Fn = const char*(__thiscall*)(void*, unsigned int);
	auto fn = (Fn)VGet(g_ipanel, kIPanelGetClassName);
	if (!fn || !IsExec((void*)fn)) return "";
	return HudVguiSafeStr(fn(g_ipanel, p));
}

static int HudVguiChildCount(unsigned int p) {
	if (!p || !g_ipanel) return 0;
	using Fn = int(__thiscall*)(void*, unsigned int);
	auto fn = (Fn)VGet(g_ipanel, kIPanelGetChildCount);
	if (!fn || !IsExec((void*)fn)) return 0;
	const int n = fn(g_ipanel, p);
	if (n < 0 || n > 1024) return 0;
	return n;
}

static unsigned int HudVguiChild(unsigned int p, int i) {
	if (!p || !g_ipanel || i < 0) return 0;
	using Fn = unsigned int(__thiscall*)(void*, unsigned int, int);
	auto fn = (Fn)VGet(g_ipanel, kIPanelGetChild);
	if (!fn || !IsExec((void*)fn)) return 0;
	return fn(g_ipanel, p, i);
}

static int HudVguiFillKids(unsigned int root, unsigned int* out, int maxOut) {
	if (!root || !out || maxOut <= 0) return 0;
	const int c = HudVguiChildCount(root);
	if (c <= 0) return 0;
	const int n = c > maxOut ? maxOut : c;
	int got = 0;
	for (int i = 0; i < n; ++i) {
		const unsigned int ch = HudVguiChild(root, i);
		if (!ch) break;
		out[got++] = ch;
	}
	return got;
}

static unsigned int HudVguiParent(unsigned int p) {
	if (!p || !g_ipanel) return 0;
	using Fn = unsigned int(__thiscall*)(void*, unsigned int);
	auto fn = (Fn)VGet(g_ipanel, kIPanelGetParent);
	if (!fn || !IsExec((void*)fn)) return 0;
	return fn(g_ipanel, p);
}

static void HudVguiSetVisible(unsigned int p, bool vis) {
	if (!p || !g_ipanel) return;
	using Fn = void(__thiscall*)(void*, unsigned int, bool);
	auto fn = (Fn)VGet(g_ipanel, kIPanelSetVisible);
	if (fn && IsExec((void*)fn))
		fn(g_ipanel, p, vis);
}

static void HudVguiGetPos(unsigned int p, int* x, int* y) {
	if (!p || !g_ipanel || !x || !y) return;
	using Fn = void(__thiscall*)(void*, unsigned int, int&, int&);
	auto fn = (Fn)VGet(g_ipanel, kIPanelGetPos);
	if (fn && IsExec((void*)fn))
		fn(g_ipanel, p, *x, *y);
}

static void HudVguiSetPos(unsigned int p, int x, int y) {
	if (!p || !g_ipanel) return;
	using Fn = void(__thiscall*)(void*, unsigned int, int, int);
	auto fn = (Fn)VGet(g_ipanel, kIPanelSetPos);
	if (fn && IsExec((void*)fn))
		fn(g_ipanel, p, x, y);
}

static void HudVguiGetSize(unsigned int p, int* w, int* h) {
	if (!p || !g_ipanel || !w || !h) return;
	using Fn = void(__thiscall*)(void*, unsigned int, int&, int&);
	auto fn = (Fn)VGet(g_ipanel, kIPanelGetSize);
	if (fn && IsExec((void*)fn))
		fn(g_ipanel, p, *w, *h);
}

static bool HudVguiSkip(const char* n) {
	if (!n || !n[0]) return false;
	return strstr(n, "GameUI") || strstr(n, "FocusNav") || strstr(n, "SteamOverlay");
}

static int HudVguiKind(const char* n) {
	if (!n || !n[0]) return 0;
	if (!_stricmp(n, "CHudTeamDisplay") || !_stricmp(n, "CHudLocalPlayerDisplay")
		|| !_stricmp(n, "HudTeamDisplay") || !_stricmp(n, "CHudSurvivorTeamStatus"))
		return kHudVguiKindTeam;
	if (!_stricmp(n, "HudWeaponSelection") || !_stricmp(n, "CHudWeaponSelection")
		|| !_stricmp(n, "HudWeapon") || !_stricmp(n, "CHudWeapon")
		|| strstr(n, "WeaponSelection"))
		return kHudVguiKindWep;
	if (!_stricmp(n, "CItemPickupPanel") || !_stricmp(n, "ItemPickup")
		|| !_stricmp(n, "HudItemPickup") || !_stricmp(n, "speedmeter")
		|| !_stricmp(n, "ramka") || !_stricmp(n, "Arc-ai") || !_stricmp(n, "ARC-AI")
		|| !_stricmp(n, "arrow") || strstr(n, "ItemPickup"))
		return kHudVguiKindPickup;
	if (!_strnicmp(n, "time_", 5) || !_strnicmp(n, "digits", 6) || !_strnicmp(n, "r_", 2))
		return kHudVguiKindPickup;
	return 0;
}

static void HudVguiBagClear(HudVguiBag* b) {
	if (!b) return;
	memset(b, 0, sizeof(*b));
}

static void HudVguiBagAdd(HudVguiBag* b, unsigned int p) {
	if (!b || !p || b->n >= kHudVguiBagMax) return;
	for (int i = 0; i < b->n; ++i)
		if (b->pan[i] == p) return;
	b->pan[b->n++] = p;
}

static void HudVguiConsider(unsigned int p) {
	if (!p) return;
	const char* n = HudVguiName(p);
	const char* cls = HudVguiClass(p);
	int kind = HudVguiKind(n);
	if (!kind) kind = HudVguiKind(cls);
	if (!kind) return;
	if (kind == kHudVguiKindPickup) {
		unsigned int par = HudVguiParent(p);
		if (par && par != p) {
			const char* pn = HudVguiName(par);
			const char* pc = HudVguiClass(par);
			if (HudVguiKind(pn) == kHudVguiKindPickup || HudVguiKind(pc) == kHudVguiKindPickup
				|| !_stricmp(pn, "CItemPickupPanel")
				|| !_strnicmp(n, "r_", 2) || !_strnicmp(n, "time_", 5) || !_strnicmp(n, "digits", 6)
				|| !_stricmp(n, "speedmeter") || !_stricmp(n, "ramka") || !_stricmp(n, "arrow"))
				p = par;
		}
		HudVguiBagAdd(&g_hvPickup, p);
		return;
	}
	if (kind == kHudVguiKindTeam)
		HudVguiBagAdd(&g_hvTeam, p);
	else
		HudVguiBagAdd(&g_hvWep, p);
}

static void HudVguiWalkKids(unsigned int root, int maxKids) {
	if (!root || maxKids <= 0) return;
	const char* n = HudVguiName(root);
	if (HudVguiSkip(n)) return;
	HudVguiConsider(root);
	unsigned int kids[96];
	const int cap = maxKids > 96 ? 96 : maxKids;
	const int got = HudVguiFillKids(root, kids, cap);
	if (!g_hvLoggedKids)
		Log("hud vgui walk name=%s reported=%d filled=%d", n[0] ? n : "?", HudVguiChildCount(root), got);
	for (int i = 0; i < got; ++i)
		HudVguiConsider(kids[i]);
}

static void HudVguiLogKids(unsigned int root, const char* tag) {
	if (!root) return;
	unsigned int kids[32];
	const int got = HudVguiFillKids(root, kids, 32);
	Log("hud vgui %s=%u children=%d filled=%d name=%s", tag, root, HudVguiChildCount(root), got, HudVguiName(root));
	const int lim = got > 24 ? 24 : got;
	for (int i = 0; i < lim; ++i)
		Log("  %s[%d] name=%s class=%s kids=%d", tag, i, HudVguiName(kids[i]), HudVguiClass(kids[i]),
			HudVguiChildCount(kids[i]));
}

static void HudVguiSearch() {
	if (!HudVguiReady()) return;
	const unsigned int client = HudVguiEnginePanel(kPanelClientDll);
	const unsigned int embedded = HudVguiEmbedded();
	const unsigned int ingame = HudVguiEnginePanel(4);
	if (!g_hvLoggedKids) {
		HudVguiLogKids(client, "client");
		if (embedded && embedded != client)
			HudVguiLogKids(embedded, "embedded");
		if (ingame && ingame != client && ingame != embedded)
			HudVguiLogKids(ingame, "ingame");
	}
	if (client) {
		HudVguiWalkKids(client, 80);
		const int c = HudVguiChildCount(client);
		const int lim = c > 8 ? 8 : (c > 0 ? c : 4);
		for (int i = 0; i < lim; ++i) {
			const unsigned int ch = HudVguiChild(client, i);
			if (!ch) break;
			HudVguiWalkKids(ch, 80);
			if (g_hvPickup.n <= 0 || g_hvWep.n <= 0) {
				const int n2 = HudVguiChildCount(ch);
				const int inner = (n2 > 0 && n2 < 80) ? n2 : 40;
				for (int j = 0; j < inner && g_hvPickup.n < kHudVguiBagMax; ++j) {
					const unsigned int gch = HudVguiChild(ch, j);
					if (!gch) break;
					HudVguiWalkKids(gch, 40);
				}
			}
		}
	}
	if (embedded && embedded != client)
		HudVguiWalkKids(embedded, 48);
	if (ingame && ingame != client && ingame != embedded)
		HudVguiWalkKids(ingame, 48);
	if (!g_hvLoggedKids) {
		g_hvLoggedKids = true;
		const int c = HudVguiChildCount(client);
		const int lim = c > 4 ? 4 : c;
		for (int i = 0; i < lim; ++i) {
			const unsigned int ch = HudVguiChild(client, i);
			if (ch) HudVguiLogKids(ch, "viewport");
		}
		if (embedded) {
			const unsigned int st = HudVguiChild(embedded, 0);
			if (st) HudVguiLogKids(st, "static");
		}
	}
	if (!g_hvLoggedFound) {
		g_hvLoggedFound = true;
		Log("hud vgui found team=%d wep=%d pickup=%d", g_hvTeam.n, g_hvWep.n, g_hvPickup.n);
		for (int i = 0; i < g_hvTeam.n; ++i)
			Log("  team pan name=%s class=%s", HudVguiName(g_hvTeam.pan[i]), HudVguiClass(g_hvTeam.pan[i]));
		for (int i = 0; i < g_hvWep.n; ++i)
			Log("  wep pan name=%s class=%s", HudVguiName(g_hvWep.pan[i]), HudVguiClass(g_hvWep.pan[i]));
		for (int i = 0; i < g_hvPickup.n; ++i)
			Log("  pickup pan name=%s class=%s", HudVguiName(g_hvPickup.pan[i]), HudVguiClass(g_hvPickup.pan[i]));
	}
}

static void HudVguiInvalidate() {
	HudVguiBagClear(&g_hvTeam);
	HudVguiBagClear(&g_hvWep);
	HudVguiBagClear(&g_hvPickup);
	g_hvSearchAt = 0;
	g_hvLoggedFound = false;
	g_hvLoggedKids = false;
}

static void HudHideInvalidate() {
	HudVguiInvalidate();
}

static void HudVguiApplyBag(HudVguiBag* b, bool hide, bool keepShown) {
	if (!b) return;
	for (int i = 0; i < b->n; ++i) {
		const unsigned int p = b->pan[i];
		if (!p) continue;
		if (hide) {
			if (!b->have[i]) {
				HudVguiGetPos(p, &b->x[i], &b->y[i]);
				HudVguiGetSize(p, &b->w[i], &b->h[i]);
				if (b->x[i] > -2000)
					b->have[i] = true;
			}
			if (!keepShown)
				HudVguiSetVisible(p, false);
			HudVguiSetPos(p, -5000, -5000);
		} else {
			if (b->have[i]) {
				HudVguiSetPos(p, b->x[i], b->y[i]);
				b->have[i] = false;
			}
			HudVguiSetVisible(p, true);
		}
	}
}

static void HudVguiApplyNow() {
	const bool live = g_run.load(std::memory_order_relaxed) && SkeetoFeaturesOn();
	const bool team = live && g_optHudHideTeam;
	const bool wep = live && g_optHudHideWep;
	const bool pickup = live && g_optHudHidePickup;
	HudVguiApplyBag(&g_hvTeam, team, false);
	HudVguiApplyBag(&g_hvWep, wep, false);
	HudVguiApplyBag(&g_hvPickup, pickup, false);
}

static void HudHideTick() {
	const bool live = g_run.load(std::memory_order_relaxed) && SkeetoFeaturesOn();
	const bool team = live && g_optHudHideTeam;
	const bool wep = live && g_optHudHideWep;
	const bool pickup = live && g_optHudHidePickup;
	if (!team)
		HudHideRestore();
	if (!g_engine || !EngInGame() || EngDrawingLoading()) {
		if (!live || (!team && !wep && !pickup))
			HudVguiApplyNow();
		return;
	}

	HudHideStripOnce();
	if (team) {
		if (!g_hideHudVar)
			g_hideHudVar = CvarFind("hidehud");
		if (g_hideHudVar && !g_hideHudPinned) {
			int cur = 0;
			float f = 0.f;
			if (!CvarReadPair(g_hideHudVar, &cur, &f))
				cur = ConVarReadIntBounded(g_hideHudVar, 0, 4095);
			if (cur < 0) cur = 0;
			g_hideHudSaved = cur & ~kHideHudStrip;
			g_hideHudPinned = true;
			if (!g_hideHudLogged) {
				g_hideHudLogged = true;
				Log("hud hide pin hidehud %d->%d (health+misc only)",
					cur, g_hideHudSaved | kHideHudMask);
			}
		}
		HudHidePokeCvar(g_hideHudSaved | kHideHudMask);
	}

	const bool needSearch = (team && g_hvTeam.n <= 0) || (wep && g_hvWep.n <= 0)
		|| (pickup && g_hvPickup.n <= 0);
	if (needSearch) {
		const DWORD now = GetTickCount();
		if (!g_hvSearchAt || now - g_hvSearchAt >= 1500) {
			g_hvSearchAt = now;
			HudVguiSearch();
		}
	}
	HudVguiApplyNow();
}

static void RoundTimerOnStart() {
	g_roundT0 = GetTickCount();
	g_roundTiming = true;
	g_roundFrozenMs = 0;
}

static void RoundTimerOnEnd() {
	if (g_roundTiming) {
		g_roundFrozenMs = GetTickCount() - g_roundT0;
		g_roundTiming = false;
	}
}

static void __fastcall Hooked_EnginePaint(void* ecx, void* edx, int mode) {
	if (ecx) g_engineVgui = ecx;
	HudHideTick();
	if (g_origEnginePaint)
		g_origEnginePaint(ecx, edx, mode);
	HudVguiApplyNow();
	if (!g_run.load(std::memory_order_relaxed))
		return;
	InstallMenuKeyEatOnce();
	if (g_optXhair || g_optXhairRing || g_optXhairTex)
		XhairNoteLoading();
	CrashMark(kBcPaint);
	PumpDeferredClientCmds();
	PumpDeferredSettings();
	ListenPump();
	if (DlcGetCatalog()->count <= 0) {
		const bool allowEngineGlob = g_engine && !EngConnected();
		DlcEnsureCatalog(g_gameL4d2Dir, allowEngineGlob);
		if (DlcLastReloadMs() > 0 && DlcGetCatalog()->count > 0)
			Log("catalog ready (Paint) styles=%d loose=%d GAME=%d ms=%d glob=%d",
				DlcGetCatalog()->count, DlcLastLooseJsonCount(),
				DlcLastEngineJsonCount(), DlcLastReloadMs(), allowEngineGlob ? 1 : 0);
	}
	if (mode != kPaintInGamePanels)
		return;
	if (g_unstickButtons.exchange(false, std::memory_order_relaxed))
		ClientUxUnstickButtons();
	// Same game-thread Surface path as the menu. Do NOT hook PaintTraverseEx
	// (that VMT slot crashed on MinGW before the first paint log flushed).
	SurfDrawTexXhair();
	SurfDrawHudXhair();
	ElimPaintHud();
	ClockPaintHud();
	SpeedPaintHud();
	TimerPaintHud();
	TeamHudPaintHud();
	ClientUxPaintThrowLand();
	ClientUxPaintNoCorpse();
	LocalPlayTickCharPersist();
	LocalPlayPaint();
	ClientUxPaintDmgNums();
	MenuPollPaintKeys();
	MenuPaintEngine();
	// After HUD paint: FSN (and L4N's datacache walk) has already returned this frame.
	PumpGameThreadFeedback();
}

static int __fastcall Hooked_InKeyEvent(void* ecx, void* edx, int eventcode, int keynum, const char* binding) {
	(void)edx;
	// Same policy as NCL: eat ONLY while our menu is open, then return 0 so the
	// engine never sees 0-9 / Esc / page shortcuts. Everything else goes to orig
	// (NCL's MinHook trampoline / CHLClient). Never return 0 for the whole keyboard.
	g_inKeyHook = true;
	const bool menu = g_menuVisible && !g_menuParked
		&& g_run.load(std::memory_order_relaxed) && SkeetoFeaturesOn();
	const DWORD vk = ButtonCodeToVk(keynum);
	if (menu && vk && MenuVkIsMenuOnly(vk)) {
		if (eventcode == 1)
			MenuOnVk(vk);
		g_inKeyHook = false;
		return 0;
	}
	InKeyEventFn next = g_origInKeyEvent;
	if (next == (InKeyEventFn)&Hooked_InKeyEvent)
		next = g_origInKeyEventPrimary;
	int r = 1;
	if (next && next != (InKeyEventFn)&Hooked_InKeyEvent)
		r = next(ecx, edx, eventcode, keynum, binding);
	g_inKeyHook = false;
	return r;
}

static void UnhookInKeyEventIfOurs() {
	void* bc = GetIface("client.dll", "VClient016");
	if (!bc) return;
	void** vt = *(void***)bc;
	if (!vt) return;

	void* const ours = (void*)&Hooked_InKeyEvent;
	void* slot = vt[kVmtInKeyEvent];
	if (slot != ours && (!g_hookInKeyEvent || slot != g_hookInKeyEvent))
		return;

	void* rest = nullptr;
	if (g_origInKeyEventPrimary && (void*)g_origInKeyEventPrimary != ours)
		rest = (void*)g_origInKeyEventPrimary;
	else if (g_origInKeyEvent && (void*)g_origInKeyEvent != ours)
		rest = (void*)g_origInKeyEvent;
	if (!rest) {
		static bool s_once = false;
		if (!s_once) {
			s_once = true;
			Log("IN_KeyEvent slot is ours but no orig to restore");
		}
		return;
	}
	void* old = nullptr;
	if (VtProtectWrite(&vt[kVmtInKeyEvent], rest, &old)) {
		Log("IN_KeyEvent unhooked restore=%p (was %p)", rest, old);
		g_hookInKeyEvent = nullptr;
	}
}

static void EnsureClientLevelHooks() {
	if (!g_run.load(std::memory_order_relaxed)) return;
	EnsureServerGameHooks();
	if (!g_hookLevelInitPre)
		g_hookLevelInitPre = (void*)&Hooked_LevelInitPreEntity;
	if (!g_hookEnginePaint)
		g_hookEnginePaint = (void*)&Hooked_EnginePaint;
	// Do not UnhookInKeyEventIfOurs here — worker/Paint used to fight NCL's MinHook.

	void* bc = GetIface("client.dll", "VClient016");
	if (!bc) return;
	g_baseClient = bc;
	void** vt = *(void***)bc;
	if (!vt) return;

	if (vt[4] != g_hookLevelInitPre) {
		void* prev = nullptr;
		if (VtProtectWrite(&vt[4], g_hookLevelInitPre, &prev)) {
			if (prev != (void*)g_origLevelInitPre)
				g_origLevelInitPre = (LevelInitPreFn)prev;
			if (!g_levelInitHookLogged) {
				g_levelInitHookLogged = true;
				Log("LevelInitPreEntity hooked (VMT[4]) orig=%p", (void*)g_origLevelInitPre);
			}
		}
	}

	if (!g_engineVgui)
		g_engineVgui = GetIface("engine.dll", "VEngineVGui001");
	if (g_engineVgui && IfaceAlive(g_engineVgui)) {
		void** evt = *(void***)g_engineVgui;
		if (evt && evt[kVmtEngineVGuiPaint] != g_hookEnginePaint) {
			void* prev = nullptr;
			if (VtProtectWrite(&evt[kVmtEngineVGuiPaint], g_hookEnginePaint, &prev)) {
				if (prev != (void*)g_origEnginePaint)
					g_origEnginePaint = (EnginePaintFn)prev;
				if (!g_enginePaintHookLogged) {
					g_enginePaintHookLogged = true;
					Log("EngineVGui::Paint hooked (VMT[%d]) orig=%p", kVmtEngineVGuiPaint, (void*)g_origEnginePaint);
				}
			}
		}
	}
}

// IMatSystemSurface (VGUI_Surface031). Menu + textrial share this; no extra VMT hook.
// L4D2 binary: slot 10 is DrawSetColor(Color) ret 4; slot 11 is (r,g,b,a) ret 10h.
// Necola's header lists them swapped; calling slot 10 with 4 ints leaves args on the
// stack and the next ret jumps into VirtualQuery's MBI.State (0x1000 = MEM_COMMIT).
static constexpr int kVmtSurfDrawSetColor = 11;
static constexpr int kVmtSurfDrawFilledRect = 12;
static constexpr int kVmtSurfDrawOutlinedRect = 14;
static constexpr int kVmtSurfDrawLine = 15;
static constexpr int kVmtSurfDrawSetTextFont = 17;
static constexpr int kVmtSurfDrawSetTextColor = 19; // slot 18 = Color (1 dword)
static constexpr int kVmtSurfDrawSetTextPos = 20;
static constexpr int kVmtSurfDrawPrintText = 22;
// Texture slots sit between DrawFlushText and GetScreenSize (35 verified).
// No Color/rgba overload swap in this range (unlike DrawSetColor 10/11).
static constexpr int kVmtSurfDrawSetTextureFile = 27;
static constexpr int kVmtSurfDrawSetTextureRGBA = 28;
static constexpr int kVmtSurfDrawSetTexture = 29;
static constexpr int kVmtSurfDrawGetTextureSize = 30;
static constexpr int kVmtSurfDrawTexturedRect = 31;
static constexpr int kVmtSurfCreateNewTextureID = 34;
static constexpr int kVmtSurfGetScreenSize = 35;
static constexpr int kVmtSurfCreateFont = 63;
static constexpr int kVmtSurfSetFontGlyphSet = 64;
static constexpr int kVmtSurfGetTextSize = 72;
static constexpr int kFontFlagOutline = 0x200;
static int g_menuOx = 10;
static int g_menuOy = 10;

static HWND FindGameWindow() {
	HWND w = FindWindowA("Valve001", nullptr);
	if (w && IsWindow(w) && w != g_xhairHwnd)
		return w;
	return nullptr;
}

static bool MenuEnsureSurf() {
	if (g_surf) return true;
	g_surf = GetIface("vguimatsurface.dll", "VGUI_Surface031");
	return g_surf && IfaceAlive(g_surf);
}

static bool SurfGetScreenSize(int* sw, int* sh) {
	if (!g_surf || !sw || !sh) return false;
	using SzFn = void(__thiscall*)(void*, int*, int*);
	static SzFn s_fn = nullptr;
	if (!s_fn) {
		s_fn = (SzFn)VGet(g_surf, kVmtSurfGetScreenSize);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return false;
		}
	}
	s_fn(g_surf, sw, sh);
	return *sw >= 80 && *sh >= 80;
}

static void SurfColor(int r, int g, int b, int a) {
	using Fn = void(__thiscall*)(void*, int, int, int, int);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!g_surf) return;
		s_fn = (Fn)VGet(g_surf, kVmtSurfDrawSetColor);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return;
		}
	}
	s_fn(g_surf, r, g, b, a);
}

static void SurfClearTexture() {
	using Fn = void(__thiscall*)(void*, int);
	auto fn = (Fn)VGet(g_surf, kVmtSurfDrawSetTexture);
	if (fn && IsExec((void*)fn))
		fn(g_surf, 0);
}

static void SurfFill(int x0, int y0, int x1, int y1) {
	using Fn = void(__thiscall*)(void*, int, int, int, int);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!g_surf) return;
		s_fn = (Fn)VGet(g_surf, kVmtSurfDrawFilledRect);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return;
		}
	}
	s_fn(g_surf, x0, y0, x1, y1);
}

static void SurfColorSwatch(int x, int y, int w, int h, COLORREF fill) {
	if (w < 4 || h < 4) return;
	SurfColor(220, 220, 220, 255);
	SurfFill(x - 1, y - 1, x + w + 1, y + h + 1);
	SurfColor(GetRValue(fill), GetGValue(fill), GetBValue(fill), 255);
	SurfFill(x, y, x + w, y + h);
}

// Engine HUD texture crosshair. Does not touch itempickup.res / r_screenoverlay.
static char g_texXhairList[kTexXhairMax][80];
static int g_texXhairCount = 0;
static int g_texXhairId = -1;
static char g_texXhairBound[80]{};

static void TexXhairAdd(const char* mat) {
	if (!mat || !mat[0] || TexXhairIsVanillaTest(mat)) return;
	if (g_texXhairCount >= kTexXhairMax) {
		static bool s_cap = false;
		if (!s_cap) {
			s_cap = true;
			Log("xhairTex list capped at %d", kTexXhairMax);
		}
		return;
	}
	for (int i = 0; i < g_texXhairCount; ++i) {
		if (_stricmp(g_texXhairList[i], mat) == 0)
			return;
	}
	strncpy(g_texXhairList[g_texXhairCount], mat, 79);
	g_texXhairList[g_texXhairCount][79] = 0;
	++g_texXhairCount;
}

static void TexXhairAddVmtFile(const char* filename, void*) {
	if (!filename || !filename[0]) return;
	const size_t n = strlen(filename);
	if (n < 5 || _stricmp(filename + n - 4, ".vmt") != 0) return;
	char stem[64]{};
	const size_t stemLen = n - 4;
	if (stemLen >= sizeof(stem)) return;
	memcpy(stem, filename, stemLen);
	stem[stemLen] = 0;
	char mat[80]{};
	snprintf(mat, sizeof(mat), "skeeto/xhair/%s", stem);
	TexXhairAdd(mat);
}

static void TexXhairRefreshList() {
	g_texXhairCount = 0;
	if (g_gameL4d2Dir[0]) {
		char pat[MAX_PATH]{};
		snprintf(pat, sizeof(pat), "%s\\materials\\skeeto\\xhair\\*.vmt", g_gameL4d2Dir);
		WIN32_FIND_DATAA fd{};
		HANDLE h = FindFirstFileA(pat, &fd);
		if (h != INVALID_HANDLE_VALUE) {
			do {
				if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
				TexXhairAddVmtFile(fd.cFileName, nullptr);
			} while (FindNextFileA(h, &fd));
			FindClose(h);
		}
	}
	// Addon VPK is invisible to Win32; list via engine GAME (menu key / first pick only).
	DlcForEachGameGlob("materials/skeeto/xhair/*.vmt", &TexXhairAddVmtFile, nullptr);
	static const char* kWorkshop[] = {
		"vgui/crosshair",
		"vgui/hud/crosshair",
	};
	for (const char* m : kWorkshop) {
		char vmt[96]{};
		snprintf(vmt, sizeof(vmt), "materials/%s.vmt", m);
		if (DlcGameFileExists(vmt))
			TexXhairAdd(m);
	}
	Log("xhairTex list n=%d", g_texXhairCount);
}

static int TexXhairIndexOf(const char* mat) {
	if (!mat || !mat[0]) return -1;
	for (int i = 0; i < g_texXhairCount; ++i) {
		if (_stricmp(g_texXhairList[i], mat) == 0)
			return i;
	}
	return -1;
}

static const char* TexXhairPickCurrent() {
	if (g_texXhairCount <= 0)
		TexXhairRefreshList();
	if (g_texXhairCount <= 0) return nullptr;
	int idx = TexXhairIndexOf(g_xhairTexMat);
	if (idx < 0) idx = 0;
	strncpy(g_xhairTexMat, g_texXhairList[idx], sizeof(g_xhairTexMat) - 1);
	g_xhairTexMat[sizeof(g_xhairTexMat) - 1] = 0;
	TexXhairApplyCurrent();
	return g_xhairTexMat;
}

static bool SurfEnsureTexXhair() {
	if (!MenuEnsureSurf()) return false;
	const char* mat = g_xhairTexMat[0] ? g_xhairTexMat : TexXhairPickCurrent();
	if (mat && TexXhairIsVanillaTest(mat)) {
		g_xhairTexMat[0] = 0;
		mat = TexXhairPickCurrent();
	}
	if (!mat || !mat[0]) {
		mat = TexXhairPickCurrent();
		if (!mat) return false;
	}
	if (g_texXhairId >= 0 && _stricmp(g_texXhairBound, mat) == 0)
		return true;

	using CreateIdFn = int(__thiscall*)(void*, bool);
	using SetFileFn = void(__thiscall*)(void*, int, const char*, int, bool);
	using GetSizeFn = void(__thiscall*)(void*, int, int*, int*);
	auto createId = (CreateIdFn)VGet(g_surf, kVmtSurfCreateNewTextureID);
	auto setFile = (SetFileFn)VGet(g_surf, kVmtSurfDrawSetTextureFile);
	auto getSize = (GetSizeFn)VGet(g_surf, kVmtSurfDrawGetTextureSize);
	if (!createId || !setFile || !IsExec((void*)createId) || !IsExec((void*)setFile)) {
		Log("xhairTex missing CreateNewTextureID/DrawSetTextureFile");
		return false;
	}

	char vmt[96]{};
	snprintf(vmt, sizeof(vmt), "materials/%s.vmt", mat);
	if (!DlcGameFileExists(vmt)) {
		const char* picked = TexXhairPickCurrent();
		if (!picked) {
			Log("xhairTex no GAME vmt for %s", mat);
			return false;
		}
		mat = picked;
		snprintf(vmt, sizeof(vmt), "materials/%s.vmt", mat);
	}

	const int id = createId(g_surf, false);
	if (id < 0) {
		Log("xhairTex CreateNewTextureID=%d", id);
		return false;
	}
	setFile(g_surf, id, mat, 0, false);

	int tw = 0, th = 0;
	if (getSize && IsExec((void*)getSize))
		getSize(g_surf, id, &tw, &th);

	g_texXhairId = id;
	strncpy(g_texXhairBound, mat, sizeof(g_texXhairBound) - 1);
	g_texXhairBound[sizeof(g_texXhairBound) - 1] = 0;
	if (_stricmp(g_xhairTexMat, mat) != 0) {
		strncpy(g_xhairTexMat, mat, sizeof(g_xhairTexMat) - 1);
		g_xhairTexMat[sizeof(g_xhairTexMat) - 1] = 0;
	}
	Log("xhairTex id=%d mat=%s vtf=%dx%d draw=%d", id, mat, tw, th, g_xhairTexSize);
	return true;
}

static void SurfDrawTextured(int x0, int y0, int x1, int y1, int id) {
	using SetTexFn = void(__thiscall*)(void*, int);
	using RectFn = void(__thiscall*)(void*, int, int, int, int);
	static SetTexFn s_setTex = nullptr;
	static RectFn s_rect = nullptr;
	if (!s_setTex || !s_rect) {
		if (!g_surf) return;
		s_setTex = (SetTexFn)VGet(g_surf, kVmtSurfDrawSetTexture);
		s_rect = (RectFn)VGet(g_surf, kVmtSurfDrawTexturedRect);
		if (!s_setTex || !s_rect || !IsExec((void*)s_setTex) || !IsExec((void*)s_rect)) {
			s_setTex = nullptr;
			s_rect = nullptr;
			return;
		}
	}
	SurfColor(255, 255, 255, 255);
	s_setTex(g_surf, id);
	s_rect(g_surf, x0, y0, x1, y1);
}

// hardwareFilter=0 → point sample so GDI+ diagonals/circles stay sharp.
// Restore ESP: L4D2 may pop 4 args even if the Necola header lists 6.
static void __declspec(noinline) SurfSetTextureRGBA(int id, const unsigned char* rgba, int wide, int tall) {
	void* surf = g_surf;
	void* fn = (surf && id >= 0 && rgba && wide > 0 && tall > 0)
		? VGet(surf, kVmtSurfDrawSetTextureRGBA) : nullptr;
	if (!fn || !IsExec(fn))
		return;
	__asm {
		mov eax, fn
		mov ecx, surf
		push ebx
		mov ebx, esp
		push 1
		push 0
		push tall
		push wide
		push rgba
		push id
		call eax
		mov esp, ebx
		pop ebx
	}
}

static void SurfDrawTexXhair() {
	if (!g_optXhairTex) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!g_engine || !EngInGame()) return;
	if (g_xhairHideScene.load(std::memory_order_relaxed)) return;
	if (EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	if (!SurfEnsureTexXhair()) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	if (g_optXhairTexFull) {
		SurfDrawTextured(0, 0, sw, sh, g_texXhairId);
		return;
	}
	int box = g_xhairTexSize;
	if (box < kTexXhairSizeMin) box = kTexXhairSizeMin;
	if (box > kTexXhairSizeMax) box = kTexXhairSizeMax;
	const int cap = (sw < sh) ? sw : sh;
	if (box > cap) box = cap;
	const int x0 = sw / 2 - box / 2;
	const int y0 = sh / 2 - box / 2;
	SurfDrawTextured(x0, y0, x0 + box, y0 + box, g_texXhairId);
}

static void SurfOutline(int x0, int y0, int x1, int y1) {
	using Fn = void(__thiscall*)(void*, int, int, int, int);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!g_surf) return;
		s_fn = (Fn)VGet(g_surf, kVmtSurfDrawOutlinedRect);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return;
		}
	}
	s_fn(g_surf, x0, y0, x1, y1);
}

static void SurfLine(int x0, int y0, int x1, int y1) {
	using Fn = void(__thiscall*)(void*, int, int, int, int);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		if (!g_surf) return;
		s_fn = (Fn)VGet(g_surf, kVmtSurfDrawLine);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return;
		}
	}
	s_fn(g_surf, x0, y0, x1, y1);
}

static void SurfEnsureFont(int tall) {
	if (tall < 12) tall = 12;
	if (tall > 32) tall = 32;
	const int flags = g_uiOutline ? kFontFlagOutline : 0x010; // ANTIALIAS when no outline
	if (g_menuFont && g_menuFontTall == tall && g_menuFontFlags == flags)
		return;
	using CreateFn = unsigned long(__thiscall*)(void*);
	using GlyphFn = bool(__thiscall*)(void*, unsigned long, const char*, int, int, int, int, int, int, int);
	auto create = (CreateFn)VGet(g_surf, kVmtSurfCreateFont);
	auto glyph = (GlyphFn)VGet(g_surf, kVmtSurfSetFontGlyphSet);
	if (!create || !IsExec((void*)create) || !glyph || !IsExec((void*)glyph))
		return;
	unsigned long f = create(g_surf);
	if (!f) return;
	if (!glyph(g_surf, f, "Microsoft YaHei", tall, 700, 0, 0, flags, 0, 0))
		return;
	g_menuFont = f;
	g_menuFontTall = tall;
	g_menuFontFlags = flags;
}

static void SurfTextAt(unsigned long font, int x, int y, COLORREF rgb, const wchar_t* text) {
	if (!text || !font) return;
	using FontFn = void(__thiscall*)(void*, unsigned long);
	using ColFn = void(__thiscall*)(void*, int, int, int, int);
	using PosFn = void(__thiscall*)(void*, int, int);
	using PrintFn = void(__thiscall*)(void*, const wchar_t*, int, int);
	auto setFont = (FontFn)VGet(g_surf, kVmtSurfDrawSetTextFont);
	auto setCol = (ColFn)VGet(g_surf, kVmtSurfDrawSetTextColor);
	auto setPos = (PosFn)VGet(g_surf, kVmtSurfDrawSetTextPos);
	auto print = (PrintFn)VGet(g_surf, kVmtSurfDrawPrintText);
	if (!setFont || !setCol || !setPos || !print) return;
	setFont(g_surf, font);
	setCol(g_surf, GetRValue(rgb), GetGValue(rgb), GetBValue(rgb), 255);
	setPos(g_surf, x, y);
	print(g_surf, text, (int)wcslen(text), 0);
}

static void SurfText(int x, int y, COLORREF rgb, const wchar_t* text) {
	SurfTextAt(g_menuFont, x, y, rgb, text);
}

static int SurfTextWFont(unsigned long font, const wchar_t* text) {
	if (!text || !font) return 0;
	using Fn = void(__thiscall*)(void*, unsigned long, const wchar_t*, int*, int*);
	static Fn s_fn = nullptr;
	if (!s_fn) {
		const int em = (g_menuFontTall > 0) ? g_menuFontTall : 16;
		if (!g_surf) return (int)wcslen(text) * em;
		s_fn = (Fn)VGet(g_surf, kVmtSurfGetTextSize);
		if (!s_fn || !IsExec((void*)s_fn)) {
			s_fn = nullptr;
			return (int)wcslen(text) * em;
		}
	}
	int w = 0, h = 0;
	s_fn(g_surf, font, text, &w, &h);
	return w;
}

static int SurfTextW(const wchar_t* text) {
	return SurfTextWFont(g_menuFont, text);
}

static int MenuSx(int x) { return g_menuOx + (int)(x * g_menuScale + 0.5f); }
static int MenuSy(int y) { return g_menuOy + (int)(y * g_menuScale + 0.5f); }

static int SurfWrapDesign(int designY, COLORREF rgb, const wchar_t* text, int afterPad = 6) {
	if (!text || !text[0]) return designY;
	const int x0 = MenuSx(14);
	const int rightPad = g_uiOutline ? 26 : 22;
	int maxW = MenuSx(kMenuDesignW - rightPad) - x0;
	if (maxW < 80) maxW = 80;
	int indentN = 0;
	while (text[indentN] == L' ' && indentN < 8)
		++indentN;
	int y = MenuSy(designY);
	const int lineH = (g_menuFontTall > 0 ? g_menuFontTall : 18) + 4;
	wchar_t line[256]{};
	int n = 0;
	auto emit = [&]() {
		if (n <= 0) return;
		line[n] = 0;
		SurfText(x0, y, rgb, line);
		y += lineH;
		n = 0;
	};
	auto startCont = [&]() {
		n = 0;
		if (indentN <= 0) return;
		for (int i = 0; i < indentN && i < 250; ++i)
			line[n++] = L' ';
	};
	for (const wchar_t* p = text; *p; ++p) {
		if (n >= 250) {
			emit();
			startCont();
		}
		wchar_t trial[256]{};
		memcpy(trial, line, n * sizeof(wchar_t));
		trial[n] = *p;
		trial[n + 1] = 0;
		if (n > indentN && SurfTextW(trial) > maxW) {
			emit();
			startCont();
			line[n++] = *p;
		} else {
			line[n++] = *p;
		}
	}
	emit();
	if (afterPad < 0) afterPad = 0;
	return (int)((y - g_menuOy) / (g_menuScale > 0.01f ? g_menuScale : 1.f) + 0.5f) + afterPad;
}

// Fit menu into the game framebuffer. Never larger than design size.
static void MenuUpdateScale() {
	int cw = 0, ch = 0;
	if (MenuEnsureSurf())
		SurfGetScreenSize(&cw, &ch);
	if (cw < 80 || ch < 80) {
		g_gameHwnd = FindGameWindow();
		RECT crc{};
		if (g_gameHwnd && GetClientRect(g_gameHwnd, &crc)) {
			cw = crc.right - crc.left;
			ch = crc.bottom - crc.top;
		}
	}
	if (cw < 80 || ch < 80) {
		g_menuScale = 1.f;
		g_menuW = kMenuDesignW;
		g_menuH = kMenuDesignH;
		g_menuOx = 10;
		g_menuOy = 10;
		return;
	}
	const float sx = (float)(cw - 2 * kMenuMargin) / (float)kMenuDesignW;
	const float sy = (float)(ch - 2 * kMenuMargin) / (float)kMenuDesignH;
	float s = (sx < sy) ? sx : sy;
	if (s > 1.f) s = 1.f;
	if (s < 0.50f) s = 0.50f;
	s *= (float)g_uiSizePct / 100.f;
	if (s < 0.40f) s = 0.40f;
	g_menuScale = s;
	g_menuW = (int)(kMenuDesignW * s + 0.5f);
	g_menuH = (int)(kMenuDesignH * s + 0.5f);
	if (g_menuW > cw - 2 * kMenuMargin) g_menuW = cw - 2 * kMenuMargin;
	if (g_menuH > ch - 2 * kMenuMargin) g_menuH = ch - 2 * kMenuMargin;
	if (g_menuW < 200) g_menuW = 200;
	if (g_menuH < 160) g_menuH = 160;
	if (g_uiAlignX <= 0) g_menuOx = kMenuMargin;
	else if (g_uiAlignX >= 2) g_menuOx = cw - g_menuW - kMenuMargin;
	else g_menuOx = (cw - g_menuW) / 2;
	if (g_uiAlignY <= 0) g_menuOy = kMenuMargin;
	else if (g_uiAlignY >= 2) g_menuOy = ch - g_menuH - kMenuMargin;
	else g_menuOy = (ch - g_menuH) / 2;
	if (g_menuOx < kMenuMargin) g_menuOx = kMenuMargin;
	if (g_menuOy < kMenuMargin) g_menuOy = kMenuMargin;
}

static constexpr int kFontFlagDropShadow = 0x080;
static constexpr int kFontFlagAntiAlias = 0x010;

static const wchar_t* ElimLabel(int which) {
	static const wchar_t* kEn[] = {
		L"Special Infected Kills",
		L"Common Infected Kills",
		L"SI Melee Kills",
		L"Skeet Kills",
		L"Melee Skeets",
		L"Headshot rate"
	};
	static const wchar_t* kZh[] = {
		L"特感击杀",
		L"普感击杀",
		L"特感刀杀",
		L"空爆数量",
		L"刀爆数量",
		L"爆头率"
	};
	if (which < 0 || which >= kElimLineCount) return L"";
	return g_elimLang ? kZh[which] : kEn[which];
}

static int ElimCountAt(int which) {
	switch (which) {
	case 0: return g_elimSi.load(std::memory_order_relaxed);
	case 1: return g_elimCi.load(std::memory_order_relaxed);
	case 2: return g_elimSiMelee.load(std::memory_order_relaxed);
	case 3: return g_elimSkeet.load(std::memory_order_relaxed);
	case 4: return g_elimMeleeSkeet.load(std::memory_order_relaxed);
	default: return 0;
	}
}

static void ElimFormatLine(int which, wchar_t* buf, int cap) {
	if (!buf || cap < 8) return;
	if (which == 5) {
		const int tot = g_elimSi.load(std::memory_order_relaxed) + g_elimCi.load(std::memory_order_relaxed);
		const int hs = g_elimHs.load(std::memory_order_relaxed);
		if (tot <= 0)
			swprintf_s(buf, cap, L"%s: --", ElimLabel(5));
		else
			swprintf_s(buf, cap, L"%s: %d%%", ElimLabel(5), (hs * 100 + tot / 2) / tot);
		return;
	}
	swprintf_s(buf, cap, L"%s: %d", ElimLabel(which), ElimCountAt(which));
}

static void ElimFormatValue(int which, wchar_t* buf, int cap) {
	if (!buf || cap < 4) return;
	if (which == 5) {
		const int tot = g_elimSi.load(std::memory_order_relaxed) + g_elimCi.load(std::memory_order_relaxed);
		const int hs = g_elimHs.load(std::memory_order_relaxed);
		if (tot <= 0) {
			swprintf_s(buf, cap, L"--");
			return;
		}
		swprintf_s(buf, cap, L"%d%%", (hs * 100 + tot / 2) / tot);
		return;
	}
	swprintf_s(buf, cap, L"%d", ElimCountAt(which));
}

static COLORREF ElimLineColor(int which) {
	switch (which) {
	case 0: return RGB(255, 118, 88);
	case 1: return RGB(255, 214, 96);
	case 2: return RGB(186, 214, 255);
	case 3: return RGB(64, 220, 255);
	case 4: return RGB(130, 255, 168);
	default: return RGB(255, 186, 110);
	}
}

static void SurfEnsureHudFont() {
	ClampElimHud();
	if (g_hudFont && g_hudFontTall == g_elimFont && g_hudFontLang == g_elimLang) return;
	if (!g_surf) return;
	using CreateFn = unsigned long(__thiscall*)(void*);
	using GlyphFn = bool(__thiscall*)(void*, unsigned long, const char*, int, int, int, int, int, int, int);
	auto create = (CreateFn)VGet(g_surf, kVmtSurfCreateFont);
	auto glyph = (GlyphFn)VGet(g_surf, kVmtSurfSetFontGlyphSet);
	if (!create || !IsExec((void*)create) || !glyph || !IsExec((void*)glyph)) return;
	unsigned long f = create(g_surf);
	if (!f) return;
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow;
	const char* face = (g_elimLang == 1) ? "Microsoft YaHei" : "Tahoma";
	if (!glyph(g_surf, f, face, g_elimFont, 700, 0, 0, flags, 0, 0)) {
		if (g_elimLang != 1) return;
		if (!glyph(g_surf, f, "Tahoma", g_elimFont, 700, 0, 0, flags, 0, 0))
			return;
	}
	g_hudFont = f;
	g_hudFontTall = g_elimFont;
	g_hudFontLang = g_elimLang;
}

static int ClockDateTall() {
	int d = g_clockFont * 11 / 20;
	if (d < 14) d = 14;
	if (g_clockFont > 18 && d > g_clockFont - 4) d = g_clockFont - 4;
	return d;
}

static void SurfEnsureClockFonts() {
	ClampClockHud();
	const int dateTall = ClockDateTall();
	if (g_clockTimeFont && g_clockTimeFontTall == g_clockFont
		&& g_clockDateFont && g_clockDateFontTall == dateTall)
		return;
	if (!g_surf) return;
	using CreateFn = unsigned long(__thiscall*)(void*);
	using GlyphFn = bool(__thiscall*)(void*, unsigned long, const char*, int, int, int, int, int, int, int);
	auto create = (CreateFn)VGet(g_surf, kVmtSurfCreateFont);
	auto glyph = (GlyphFn)VGet(g_surf, kVmtSurfSetFontGlyphSet);
	if (!create || !IsExec((void*)create) || !glyph || !IsExec((void*)glyph)) return;

	auto make = [&](int tall, int weight, int flags) -> unsigned long {
		unsigned long f = create(g_surf);
		if (!f) return 0;
		if (glyph(g_surf, f, "Microsoft YaHei", tall, weight, 0, 0, flags, 0, 0))
			return f;
		if (glyph(g_surf, f, "Tahoma", tall, weight, 0, 0, flags, 0, 0))
			return f;
		return 0;
	};

	const int timeFlags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	const int dateFlags = kFontFlagAntiAlias | kFontFlagDropShadow;
	unsigned long tf = make(g_clockFont, 800, timeFlags);
	unsigned long df = make(dateTall, 600, dateFlags);
	if (!tf || !df) return;
	g_clockTimeFont = tf;
	g_clockTimeFontTall = g_clockFont;
	g_clockDateFont = df;
	g_clockDateFontTall = dateTall;
}

static unsigned long SurfMakeYahei(int tall, int weight, int flags) {
	if (!g_surf) return 0;
	using CreateFn = unsigned long(__thiscall*)(void*);
	using GlyphFn = bool(__thiscall*)(void*, unsigned long, const char*, int, int, int, int, int, int, int);
	auto create = (CreateFn)VGet(g_surf, kVmtSurfCreateFont);
	auto glyph = (GlyphFn)VGet(g_surf, kVmtSurfSetFontGlyphSet);
	if (!create || !IsExec((void*)create) || !glyph || !IsExec((void*)glyph)) return 0;
	unsigned long f = create(g_surf);
	if (!f) return 0;
	if (glyph(g_surf, f, "Microsoft YaHei", tall, weight, 0, 0, flags, 0, 0))
		return f;
	if (glyph(g_surf, f, "Tahoma", tall, weight, 0, 0, flags, 0, 0))
		return f;
	return 0;
}

static void SurfEnsureSpeedTimerFonts() {
	ClampSpeedHud();
	ClampTimerHud();
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	const int subFlags = kFontFlagAntiAlias | kFontFlagDropShadow;
	if (!g_speedFontId || g_speedFontTall != g_speedFont) {
		unsigned long f = SurfMakeYahei(g_speedFont, 800, flags);
		if (f) { g_speedFontId = f; g_speedFontTall = g_speedFont; }
	}
	if (!g_timerFontId || g_timerFontTall != g_timerFont) {
		unsigned long f = SurfMakeYahei(g_timerFont, 800, flags);
		if (f) { g_timerFontId = f; g_timerFontTall = g_timerFont; }
	}
	if (!g_hudSubFont) {
		unsigned long f = SurfMakeYahei(14, 600, subFlags);
		if (f) { g_hudSubFont = f; g_hudSubFontTall = 14; }
	}
}

static void SurfEnsureDmgFont() {
	if (g_dmgFont) return;
	if (!g_surf) return;
	using CreateFn = unsigned long(__thiscall*)(void*);
	using GlyphFn = bool(__thiscall*)(void*, unsigned long, const char*, int, int, int, int, int, int, int);
	auto create = (CreateFn)VGet(g_surf, kVmtSurfCreateFont);
	auto glyph = (GlyphFn)VGet(g_surf, kVmtSurfSetFontGlyphSet);
	if (!create || !IsExec((void*)create) || !glyph || !IsExec((void*)glyph)) return;
	unsigned long f = create(g_surf);
	if (!f) return;
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow | kFontFlagOutline;
	if (!glyph(g_surf, f, "Microsoft YaHei", 24, 800, 0, 0, flags, 0, 0)) {
		if (!glyph(g_surf, f, "Tahoma", 24, 800, 0, 0, flags, 0, 0))
			return;
	}
	g_dmgFont = f;
	g_dmgFontTall = 24;
}

static void ElimPaintHud() {
	const bool preview = g_menuVisible && !g_menuParked && g_menuPage == 5;
	if (!g_optElim && !preview) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame()) return;
	if (!MenuEnsureSurf()) return;
	SurfEnsureHudFont();
	if (!g_hudFont) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	int ids[kElimLineCount]{};
	wchar_t vals[kElimLineCount][16]{};
	int nLines = 0;
	int labW = 0;
	int valW = 0;
	for (int i = 0; i < kElimLineCount; ++i) {
		if (!ElimLineOn(i)) continue;
		ids[nLines] = i;
		ElimFormatValue(i, vals[nLines], 16);
		const int lw = SurfTextWFont(g_hudFont, ElimLabel(i));
		const int vw = SurfTextWFont(g_hudFont, vals[nLines]);
		if (lw > labW) labW = lw;
		if (vw > valW) valW = vw;
		++nLines;
	}
	if (nLines <= 0 && !preview) return;

	const wchar_t* title = g_elimLang ? L"击杀统计" : L"ELIMS";
	const int titleW = SurfTextWFont(g_hudFont, title);
	const int lh = g_hudFontTall > 0 ? g_hudFontTall + 3 : 16;
	const int titleH = lh + 2;
	const int gap = 14;
	const int stripe = 3;
	const int padX = 10;
	const int padY = 8;
	int inner = labW + gap + valW;
	if (titleW > inner) inner = titleW;
	if (inner < 72) inner = 72;
	const int panelW = stripe + padX * 2 + inner;
	const int panelH = padY * 2 + titleH + (nLines > 0 ? nLines * lh : 0);

	int x = 0, y = 0;
	HudFinalizePos(&g_elimAbs, &g_elimOffX, &g_elimOffY, g_elimAlign, sw, sh, panelW, panelH, &x, &y);

	SurfColor(0, 0, 0, 62);
	SurfFill(x, y, x + panelW, y + panelH);
	SurfColor(16, 18, 24, 88);
	SurfFill(x + 1, y + 1, x + panelW - 1, y + panelH - 1);
	SurfColor(255, 118, 88, 210);
	SurfFill(x, y, x + stripe, y + panelH);
	SurfColor(255, 255, 255, 36);
	SurfFill(x + stripe, y, x + panelW, y + 1);
	SurfColor(255, 118, 88, 38);
	SurfFill(x + stripe, y + 1, x + panelW - 1, y + padY + titleH);
	if (preview) {
		SurfColor(255, 180, 80, 170);
		SurfOutline(x, y, x + panelW, y + panelH);
	}

	const int tx = x + stripe + padX;
	int ty = y + padY;
	SurfTextAt(g_hudFont, tx, ty, RGB(255, 236, 214), title);
	ty += titleH;
	SurfColor(255, 255, 255, 40);
	SurfFill(tx, ty - 3, x + panelW - padX, ty - 2);
	for (int i = 0; i < nLines; ++i) {
		const int ry0 = ty;
		const int ry1 = ty + lh;
		if (i & 1)
			SurfColor(255, 255, 255, 22);
		else
			SurfColor(0, 8, 18, 28);
		SurfFill(x + stripe, ry0, x + panelW - 1, ry1);
		SurfTextAt(g_hudFont, tx, ty, RGB(206, 206, 200), ElimLabel(ids[i]));
		const int vw = SurfTextWFont(g_hudFont, vals[i]);
		SurfTextAt(g_hudFont, x + panelW - padX - vw, ty, ElimLineColor(ids[i]), vals[i]);
		ty += lh;
	}
}

static void ClockPaintHud() {
	const bool preview = g_menuVisible && !g_menuParked && g_menuPage == 12;
	if (!g_optClock && !preview) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame()) return;
	if (!MenuEnsureSurf()) return;
	SurfEnsureClockFonts();
	if (!g_clockTimeFont || !g_clockDateFont) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	SYSTEMTIME st{};
	GetLocalTime(&st);
	static const wchar_t* kWeek[] = { L"日", L"一", L"二", L"三", L"四", L"五", L"六" };
	const wchar_t* week = kWeek[st.wDayOfWeek <= 6 ? st.wDayOfWeek : 0];

	wchar_t timeBuf[16]{};
	swprintf_s(timeBuf, 16, L"%02u:%02u:%02u", st.wHour, st.wMinute, st.wSecond);
	wchar_t dateBuf[40]{};
	swprintf_s(dateBuf, 40, L"%u月%u日  周%s", st.wMonth, st.wDay, week);

	const int timeW = SurfTextWFont(g_clockTimeFont, timeBuf);
	const int dateW = SurfTextWFont(g_clockDateFont, dateBuf);
	const int timeH = g_clockTimeFontTall > 0 ? g_clockTimeFontTall + 2 : 18;
	const int dateH = g_clockDateFontTall > 0 ? g_clockDateFontTall + 2 : 14;
	const int stripe = 3;
	const int padX = 12;
	const int padY = 8;
	int inner = timeW;
	if (dateW > inner) inner = dateW;
	if (inner < 96) inner = 96;
	const int panelW = stripe + padX * 2 + inner;
	const int panelH = padY * 2 + timeH + 6 + dateH;

	int x = 0, y = 0;
	HudFinalizePos(&g_clockAbs, &g_clockOffX, &g_clockOffY, g_clockAlign, sw, sh, panelW, panelH, &x, &y);

	SurfColor(0, 0, 0, 62);
	SurfFill(x, y, x + panelW, y + panelH);
	SurfColor(8, 22, 28, 92);
	SurfFill(x + 1, y + 1, x + panelW - 1, y + panelH - 1);
	SurfColor(64, 214, 232, 220);
	SurfFill(x, y, x + stripe, y + panelH);
	SurfColor(180, 250, 255, 42);
	SurfFill(x + stripe, y, x + panelW, y + 1);
	SurfColor(64, 214, 232, 36);
	SurfFill(x + stripe, y + 1, x + panelW - 1, y + padY + timeH + 2);
	if (preview) {
		SurfColor(80, 230, 255, 180);
		SurfOutline(x, y, x + panelW, y + panelH);
	}

	const int tx = x + stripe + padX;
	int ty = y + padY;
	const int timeX = x + panelW - padX - timeW;
	SurfTextAt(g_clockTimeFont, timeX, ty, RGB(236, 252, 255), timeBuf);
	ty += timeH + 2;
	SurfColor(180, 250, 255, 48);
	SurfFill(tx, ty, x + panelW - padX, ty + 1);
	ty += 4;
	const int dateX = x + panelW - padX - dateW;
	SurfTextAt(g_clockDateFont, dateX, ty, RGB(164, 216, 228), dateBuf);
}

static void HudGlassPanel(int x, int y, int w, int h, int stripe, COLORREF accent, bool preview) {
	SurfColor(0, 0, 0, 62);
	SurfFill(x, y, x + w, y + h);
	SurfColor(10, 12, 18, 94);
	SurfFill(x + 1, y + 1, x + w - 1, y + h - 1);
	SurfColor(GetRValue(accent), GetGValue(accent), GetBValue(accent), 220);
	SurfFill(x, y, x + stripe, y + h);
	SurfColor(255, 255, 255, 40);
	SurfFill(x + stripe, y, x + w, y + 1);
	SurfColor(GetRValue(accent), GetGValue(accent), GetBValue(accent), 36);
	SurfFill(x + stripe, y + 1, x + w - 1, y + 22);
	if (preview) {
		SurfColor(GetRValue(accent), GetGValue(accent), GetBValue(accent), 180);
		SurfOutline(x, y, x + w, y + h);
	}
}

static float LocalHorizSpeed() {
	if (!g_entlist || g_offAbsVelocity < 0 || g_offAbsVelocity > 0x4000) return 0.f;
	const int local = EngLocal();
	if (local <= 0) return 0.f;
	void* me = EntGet(local);
	if (!EntReadable(me)) return 0.f;
	float* pv = (float*)((uint8_t*)me + g_offAbsVelocity);
	if (!PtrCommitted(pv)) return 0.f;
	return sqrtf(pv[0] * pv[0] + pv[1] * pv[1]);
}

static void SpeedPaintHud() {
	const bool preview = g_menuVisible && !g_menuParked && g_menuPage == 13;
	if (!g_optSpeed && !preview) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame()) return;
	if (!MenuEnsureSurf()) return;
	SurfEnsureSpeedTimerFonts();
	if (!g_speedFontId) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	const int spd = (int)(LocalHorizSpeed() + 0.5f);
	wchar_t num[16]{};
	swprintf_s(num, 16, L"%d", spd);
	const wchar_t* lab = L"速度";
	const int numW = SurfTextWFont(g_speedFontId, num);
	const int labW = g_hudSubFont ? SurfTextWFont(g_hudSubFont, lab) : 32;
	const int numH = g_speedFontTall > 0 ? g_speedFontTall + 2 : 28;
	const int labH = g_hudSubFontTall > 0 ? g_hudSubFontTall + 2 : 14;
	const int stripe = 3;
	const int padX = 12;
	const int padY = 8;
	const int barH = 5;
	int inner = numW;
	if (labW > inner) inner = labW;
	if (inner < 88) inner = 88;
	const int panelW = stripe + padX * 2 + inner;
	const int panelH = padY * 2 + labH + numH + 8 + barH;

	int x = 0, y = 0;
	HudFinalizePos(&g_speedAbs, &g_speedOffX, &g_speedOffY, g_speedAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(255, 186, 72);
	HudGlassPanel(x, y, panelW, panelH, stripe, accent, preview);

	COLORREF numCol = RGB(255, 236, 210);
	if (spd >= 300) numCol = RGB(255, 96, 88);
	else if (spd >= 240) numCol = RGB(255, 214, 96);
	else if (spd >= 180) numCol = RGB(130, 255, 168);

	const int tx = x + stripe + padX;
	int ty = y + padY;
	if (g_hudSubFont)
		SurfTextAt(g_hudSubFont, tx, ty, RGB(255, 210, 150), lab);
	ty += labH;
	const int nx = x + panelW - padX - numW;
	SurfTextAt(g_speedFontId, nx, ty, numCol, num);
	ty += numH + 4;
	const int barW = inner;
	const int fill = spd * barW / 400;
	int fw = fill;
	if (fw < 0) fw = 0;
	if (fw > barW) fw = barW;
	SurfColor(255, 255, 255, 28);
	SurfFill(tx, ty, tx + barW, ty + barH);
	SurfColor(GetRValue(numCol), GetGValue(numCol), GetBValue(numCol), 210);
	if (fw > 0)
		SurfFill(tx, ty, tx + fw, ty + barH);
}

static DWORD RoundElapsedMs() {
	if (g_roundTiming) {
		DWORD now = GetTickCount();
		if (now >= g_roundT0) return now - g_roundT0;
		return 0;
	}
	return g_roundFrozenMs;
}

static void TimerPaintHud() {
	const bool preview = g_menuVisible && !g_menuParked && g_menuPage == 14;
	if (!g_optTimer && !preview) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame()) return;
	if (!MenuEnsureSurf()) return;
	SurfEnsureSpeedTimerFonts();
	if (!g_timerFontId) return;

	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;

	DWORD ms = RoundElapsedMs();
	if (preview && ms == 0 && !g_roundTiming)
		ms = 0;
	const int totalSec = (int)(ms / 1000);
	const int h = totalSec / 3600;
	const int m = (totalSec / 60) % 60;
	const int s = totalSec % 60;
	wchar_t num[20]{};
	if (h > 0)
		swprintf_s(num, 20, L"%d:%02d:%02d", h, m, s);
	else
		swprintf_s(num, 20, L"%02d:%02d", m, s);
	const wchar_t* lab = L"回合时间";
	const int numW = SurfTextWFont(g_timerFontId, num);
	const int labW = g_hudSubFont ? SurfTextWFont(g_hudSubFont, lab) : 48;
	const int numH = g_timerFontTall > 0 ? g_timerFontTall + 2 : 24;
	const int labH = g_hudSubFontTall > 0 ? g_hudSubFontTall + 2 : 14;
	const int stripe = 3;
	const int padX = 14;
	const int padY = 8;
	int inner = numW;
	if (labW > inner) inner = labW;
	if (inner < 96) inner = 96;
	const int panelW = stripe + padX * 2 + inner;
	const int panelH = padY * 2 + labH + numH;

	int x = 0, y = 0;
	HudFinalizePos(&g_timerAbs, &g_timerOffX, &g_timerOffY, g_timerAlign, sw, sh, panelW, panelH, &x, &y);
	const COLORREF accent = RGB(120, 230, 150);
	HudGlassPanel(x, y, panelW, panelH, stripe, accent, preview);

	const int tx = x + stripe + padX;
	int ty = y + padY;
	if (g_hudSubFont)
		SurfTextAt(g_hudSubFont, tx, ty, RGB(176, 230, 190), lab);
	ty += labH;
	const int nx = x + panelW - padX - numW;
	SurfTextAt(g_timerFontId, nx, ty, RGB(230, 255, 236), num);
}

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

static unsigned long g_teamFont = 0;
static int g_teamFontPx = 0;
static unsigned long g_teamFontSm = 0;
static int g_teamFontSmPx = 0;

struct TeamHudSlot {
	bool present;
	bool isLocal;
	int ent;
	int character;
	int hp;
	int maxHp;
	int tempHp;
	bool dead;
	bool incap;
	bool hanging;
	bool third;
	int wep;
	int side;
	int clip;
	int ammo;
	int nade;
	int kit;
	int pills;
	char melee[24];
	wchar_t name[24];
};

static TeamHudSlot g_teamHudSlots[kTeamHudMax]{};
static DWORD g_teamHudNameAt = 0;

static void* EntFromHandle(int h) {
	if (h == 0 || h == -1) return nullptr;
	int idx = h & 0xFFF;
	if (idx <= 0) idx = h & 0x7FF;
	if (idx <= 0 || idx > 4096) return nullptr;
	return EntGet(idx);
}

static int EntIndexFromHandle(int h) {
	if (h == 0 || h == -1) return 0;
	int idx = h & 0xFFF;
	if (idx <= 0) idx = h & 0x7FF;
	if (idx <= 0 || idx > 4096) return 0;
	return idx;
}

static bool TeamHudHas(const char* cn, const char* s) {
	return cn && s && strstr(cn, s) != nullptr;
}

static int TeamHudWepKind(const char* cn) {
	if (!cn || !cn[0]) return TW_None;
	if (TeamHudHas(cn, "AK47") || TeamHudHas(cn, "Ak47")) return TW_Ak;
	if (TeamHudHas(cn, "SG552") || TeamHudHas(cn, "Sg552")) return TW_Sg;
	if (TeamHudHas(cn, "Desert") || TeamHudHas(cn, "SCAR") || TeamHudHas(cn, "Scar")) return TW_Scar;
	if (TeamHudHas(cn, "M60") || TeamHudHas(cn, "m60")) return TW_M60;
	if (TeamHudHas(cn, "GrenadeLauncher") || TeamHudHas(cn, "grenade_launcher")) return TW_GL;
	if (TeamHudHas(cn, "Sniper_AWP") || TeamHudHas(cn, "AWP") || TeamHudHas(cn, "awp")) return TW_Awp;
	if (TeamHudHas(cn, "Scout") || TeamHudHas(cn, "scout")) return TW_Scout;
	if (TeamHudHas(cn, "Military") || TeamHudHas(cn, "sniper_military")) return TW_Mil;
	if (TeamHudHas(cn, "Hunting") || TeamHudHas(cn, "hunting_rifle")) return TW_Hunt;
	if (TeamHudHas(cn, "MP5") || TeamHudHas(cn, "Mp5") || TeamHudHas(cn, "mp5")) return TW_Mp5;
	if (TeamHudHas(cn, "Silenced") || TeamHudHas(cn, "smg_silenced")) return TW_SmgS;
	if (TeamHudHas(cn, "Spas") || TeamHudHas(cn, "SPAS") || TeamHudHas(cn, "shotgun_spas")) return TW_Spas;
	if (TeamHudHas(cn, "Chrome") || TeamHudHas(cn, "shotgun_chrome")) return TW_Chrome;
	if (TeamHudHas(cn, "AutoShotgun") || TeamHudHas(cn, "autoshotgun") || TeamHudHas(cn, "Autoshotgun")) return TW_Auto;
	if (TeamHudHas(cn, "Pump") || TeamHudHas(cn, "pumpshotgun")) return TW_Pump;
	if (TeamHudHas(cn, "Magnum") || TeamHudHas(cn, "Pistol_Magnum") || TeamHudHas(cn, "pistol_magnum")) return TW_Magnum;
	if (TeamHudHas(cn, "Chainsaw") || TeamHudHas(cn, "chainsaw")) return TW_Saw;
	if (TeamHudHas(cn, "Melee") || TeamHudHas(cn, "melee")) return TW_Melee;
	if (TeamHudHas(cn, "Molotov") || TeamHudHas(cn, "molotov")) return TW_Molly;
	if (TeamHudHas(cn, "PipeBomb") || TeamHudHas(cn, "Pipebomb") || TeamHudHas(cn, "pipe_bomb")) return TW_Pipe;
	if (TeamHudHas(cn, "Vomit") || TeamHudHas(cn, "vomitjar") || TeamHudHas(cn, "Bile")) return TW_Bile;
	if (TeamHudHas(cn, "Defib") || TeamHudHas(cn, "defibrillator")) return TW_Defib;
	if (TeamHudHas(cn, "FirstAid") || TeamHudHas(cn, "first_aid") || TeamHudHas(cn, "Medkit")) return TW_Kit;
	if (TeamHudHas(cn, "Incendiary") || TeamHudHas(cn, "upgradepack_incendiary")) return TW_Inc;
	if (TeamHudHas(cn, "Explosive") || TeamHudHas(cn, "upgradepack_explosive")) return TW_Exp;
	if (TeamHudHas(cn, "Adrenaline") || TeamHudHas(cn, "adrenaline")) return TW_Adren;
	if (TeamHudHas(cn, "PainPills") || TeamHudHas(cn, "pain_pills") || TeamHudHas(cn, "Pills")) return TW_Pills;
	if (TeamHudHas(cn, "SubMachine") || TeamHudHas(cn, "SMG") || TeamHudHas(cn, "Smg") || TeamHudHas(cn, "smg")) return TW_Smg;
	if (TeamHudHas(cn, "Rifle") || TeamHudHas(cn, "rifle") || TeamHudHas(cn, "M16") || TeamHudHas(cn, "m16")) return TW_M16;
	if (TeamHudHas(cn, "Pistol") || TeamHudHas(cn, "pistol")) return TW_Pistol;
	if (TeamHudHas(cn, "GasCan") || TeamHudHas(cn, "Gnome") || TeamHudHas(cn, "Cola")
		|| TeamHudHas(cn, "Firework") || TeamHudHas(cn, "Oxygen") || TeamHudHas(cn, "Propane"))
		return TW_Carry;
	return TW_None;
}

static bool TeamHudIsGun(int k) {
	return k >= TW_Pistol && k <= TW_GL;
}
static bool TeamHudIsSide(int k) {
	return k == TW_Pistol || k == TW_Magnum || k == TW_Melee || k == TW_Saw;
}
static bool TeamHudIsPrimary(int k) {
	return k >= TW_Smg && k <= TW_GL;
}
static bool TeamHudIsNade(int k) { return k == TW_Molly || k == TW_Pipe || k == TW_Bile; }
static bool TeamHudIsKit(int k) { return k == TW_Kit || k == TW_Defib || k == TW_Exp || k == TW_Inc; }
static bool TeamHudIsPills(int k) { return k == TW_Pills || k == TW_Adren; }

static const wchar_t* TeamHudWepLabel(int k) {
	switch (k) {
	case TW_Pistol: return L"手枪";
	case TW_Magnum: return L"马格南";
	case TW_Melee: return L"近战";
	case TW_Saw: return L"电锯";
	case TW_Smg: return L"冲锋枪";
	case TW_SmgS: return L"消音冲锋";
	case TW_Mp5: return L"MP5";
	case TW_Pump: return L"木喷";
	case TW_Chrome: return L"铁喷";
	case TW_Auto: return L"连喷";
	case TW_Spas: return L"SPAS";
	case TW_M16: return L"M16";
	case TW_Ak: return L"AK47";
	case TW_Scar: return L"SCAR";
	case TW_Sg: return L"SG552";
	case TW_M60: return L"M60";
	case TW_Hunt: return L"猎枪";
	case TW_Mil: return L"军用狙击";
	case TW_Scout: return L"Scout";
	case TW_Awp: return L"AWP";
	case TW_GL: return L"榴弹";
	case TW_Molly: return L"燃烧瓶";
	case TW_Pipe: return L"土制";
	case TW_Bile: return L"胆汁";
	case TW_Kit: return L"医疗包";
	case TW_Defib: return L"电击器";
	case TW_Exp: return L"高爆包";
	case TW_Inc: return L"燃烧包";
	case TW_Pills: return L"止痛药";
	case TW_Adren: return L"肾上腺素";
	case TW_Carry: return L"携带物";
	default: return L"";
	}
}

static int TeamHudWepMatList(int k, const char** out, int maxOut) {
	if (!out || maxOut <= 0) return 0;
	const char* a = nullptr;
	const char* b = nullptr;
	const char* c = nullptr;
	switch (k) {
	case TW_Pistol: a = "vgui/hud/icon_pistol"; b = "vgui/terror/l4d360ui/hud/inventory_secondary"; break;
	case TW_Magnum: a = "vgui/hud/icon_pistol_magnum"; b = "vgui/hud/icon_pistol"; c = "vgui/terror/l4d360ui/hud/inventory_secondary"; break;
	case TW_Melee: a = "vgui/hud/icon_melee"; break;
	case TW_Saw: a = "vgui/hud/icon_chainsaw"; b = "vgui/terror/l4d360ui/hud/inventory_secondary"; break;
	case TW_Smg: a = "vgui/hud/icon_smg"; break;
	case TW_SmgS: a = "vgui/hud/icon_smg_silenced"; break;
	case TW_Mp5: a = "vgui/hud/icon_smg_mp5"; break;
	case TW_Pump: a = "vgui/hud/icon_pumpshotgun"; break;
	case TW_Chrome: a = "vgui/hud/icon_shotgun_chrome"; break;
	case TW_Auto: a = "vgui/hud/icon_autoshotgun"; break;
	case TW_Spas: a = "vgui/hud/icon_shotgun_spas"; break;
	case TW_M16: a = "vgui/hud/icon_rifle"; break;
	case TW_Ak: a = "vgui/hud/icon_rifle_ak47"; break;
	case TW_Scar: a = "vgui/hud/icon_rifle_desert"; break;
	case TW_Sg: a = "vgui/hud/icon_rifle_sg552"; break;
	case TW_M60: a = "vgui/hud/icon_rifle_m60"; break;
	case TW_Hunt: a = "vgui/hud/icon_hunting_rifle"; break;
	case TW_Mil: a = "vgui/hud/icon_sniper_military"; break;
	case TW_Scout: a = "vgui/hud/icon_sniper_scout"; break;
	case TW_Awp: a = "vgui/hud/icon_sniper_awp"; break;
	case TW_GL: a = "vgui/hud/icon_grenade_launcher"; break;
	case TW_Molly: a = "vgui/hud/icon_molotov"; b = "vgui/terror/molotov_icon"; break;
	case TW_Pipe: a = "vgui/hud/icon_pipebomb"; b = "vgui/terror/pipebomb_icon"; break;
	case TW_Bile: a = "vgui/hud/icon_vomitjar"; b = "vgui/hud/icon_bile_flask"; break;
	case TW_Kit: a = "vgui/hud/icon_medkit"; b = "vgui/terror/medkit_icon"; c = "vgui/terror/l4d360ui/hud/inventory_firstaidkit"; break;
	case TW_Defib: a = "vgui/hud/icon_defibrillator"; b = "vgui/hud/icon_defib"; break;
	case TW_Exp: a = "vgui/hud/icon_upgradepack_explosive"; break;
	case TW_Inc: a = "vgui/hud/icon_upgradepack_incendiary"; break;
	case TW_Pills: a = "vgui/hud/icon_pain_pills"; b = "vgui/terror/painkillers_icon"; c = "vgui/terror/l4d360ui/hud/inventory_painpills"; break;
	case TW_Adren: a = "vgui/hud/icon_adrenaline"; b = "vgui/terror/painkillers_icon"; break;
	default: return 0;
	}
	int n = 0;
	if (a && n < maxOut) out[n++] = a;
	if (b && n < maxOut) out[n++] = b;
	if (c && n < maxOut) out[n++] = c;
	return n;
}

static const char* TeamHudFaceMat(int ch, bool incap) {
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

static const wchar_t* TeamHudCharName(int ch) {
	static const wchar_t* k[] = {
		L"Nick", L"Rochelle", L"Coach", L"Ellis",
		L"Bill", L"Zoey", L"Francis", L"Louis"
	};
	if (ch < 0 || ch >= 8) return L"?";
	return k[ch];
}

struct HudMatCache { char name[72]; int id; };
static HudMatCache g_hudMat[96]{};
static int g_hudMatN = 0;

static int SurfHudMat(const char* mat) {
	if (!mat || !mat[0] || !g_surf) return -1;
	for (int i = 0; i < g_hudMatN; ++i) {
		if (!_stricmp(g_hudMat[i].name, mat))
			return g_hudMat[i].id;
	}
	if (g_hudMatN >= 96) return -1;
	char vmt[96]{};
	snprintf(vmt, sizeof(vmt), "materials/%s.vmt", mat);
	int id = -1;
	if (DlcGameFileExists(vmt)) {
		using CreateIdFn = int(__thiscall*)(void*, bool);
		using SetFileFn = void(__thiscall*)(void*, int, const char*, int, bool);
		auto createId = (CreateIdFn)VGet(g_surf, kVmtSurfCreateNewTextureID);
		auto setFile = (SetFileFn)VGet(g_surf, kVmtSurfDrawSetTextureFile);
		if (createId && setFile && IsExec((void*)createId) && IsExec((void*)setFile)) {
			const int created = createId(g_surf, false);
			if (created >= 0) {
				setFile(g_surf, created, mat, 0, false);
				id = created;
			}
		}
	} else {
		static int s_missLog = 8;
		if (s_missLog > 0) {
			--s_missLog;
			Log("teamhud mat miss %s", mat);
		}
	}
	HudMatCache& c = g_hudMat[g_hudMatN++];
	strncpy(c.name, mat, 71);
	c.name[71] = 0;
	c.id = id;
	return id;
}

static int TeamHudBindWep(int kind, const char* meleeStem) {
	if (kind == TW_Melee && meleeStem && meleeStem[0]) {
		char mat[80]{};
		snprintf(mat, sizeof(mat), "vgui/hud/icon_%s", meleeStem);
		const int id = SurfHudMat(mat);
		if (id >= 0) return id;
	}
	const char* mats[4]{};
	const int n = TeamHudWepMatList(kind, mats, 4);
	for (int i = 0; i < n; ++i) {
		const int id = SurfHudMat(mats[i]);
		if (id >= 0) return id;
	}
	return -1;
}

// L4D1 campaigns still report m_survivorCharacter 0–3 (same as Nick…).
// survivor_set=1 → remap to Bill/Zoey/Francis/Louis (4–7). Memory-only; no modelinfo.
static int TeamHudSurvivorSet() {
	static int s_set = -1;
	static DWORD s_at = 0;
	const DWORD now = GetTickCount();
	if (s_set >= 0 && now - s_at < 2000)
		return s_set;
	const int v = ConVarReadIntBounded(CvarFind("survivor_set"), 0, 2);
	if (v < 0)
		return (s_set >= 0) ? s_set : 2;
	s_set = v;
	s_at = now;
	return s_set;
}

static int TeamHudFaceFromProp(int prop) {
	if (prop < 0 || prop > 7) return -1;
	if (prop <= 3 && TeamHudSurvivorSet() == 1)
		return prop + 4;
	return prop;
}

static int TeamHudBindFace(int ch, bool incap) {
	int id = SurfHudMat(TeamHudFaceMat(ch, incap));
	if (id >= 0) return id;
	if (incap) return SurfHudMat(TeamHudFaceMat(ch, false));
	return -1;
}

static void TeamHudEnsureFonts() {
	if (g_teamHudFont < 14) g_teamHudFont = 14;
	if (g_teamHudFont > 28) g_teamHudFont = 28;
	if (g_teamFont && g_teamFontPx == g_teamHudFont && g_teamFontSm) return;
	if (!g_surf) return;
	using CreateFn = unsigned long(__thiscall*)(void*);
	using GlyphFn = bool(__thiscall*)(void*, unsigned long, const char*, int, int, int, int, int, int, int);
	auto create = (CreateFn)VGet(g_surf, kVmtSurfCreateFont);
	auto glyph = (GlyphFn)VGet(g_surf, kVmtSurfSetFontGlyphSet);
	if (!create || !glyph || !IsExec((void*)create) || !IsExec((void*)glyph)) return;
	const int flags = kFontFlagAntiAlias | kFontFlagDropShadow;
	unsigned long f = create(g_surf);
	if (!f) return;
	if (!glyph(g_surf, f, "Microsoft YaHei", g_teamHudFont, 700, 0, 0, flags, 0, 0))
		glyph(g_surf, f, "Tahoma", g_teamHudFont, 700, 0, 0, flags, 0, 0);
	g_teamFont = f;
	g_teamFontPx = g_teamHudFont;
	const int sm = g_teamHudFont * 3 / 4;
	unsigned long fs = create(g_surf);
	if (fs) {
		if (!glyph(g_surf, fs, "Microsoft YaHei", sm, 600, 0, 0, flags, 0, 0))
			glyph(g_surf, fs, "Tahoma", sm, 600, 0, 0, flags, 0, 0);
		g_teamFontSm = fs;
		g_teamFontSmPx = sm;
	}
}

static const wchar_t* TeamHudMeleeLabel(const char* stem) {
	if (!stem || !stem[0]) return L"近战";
	if (!_stricmp(stem, "katana")) return L"武士刀";
	if (!_stricmp(stem, "fireaxe")) return L"斧头";
	if (!_stricmp(stem, "machete")) return L"砍刀";
	if (!_stricmp(stem, "crowbar")) return L"撬棍";
	if (!_stricmp(stem, "cricket_bat")) return L"球棒";
	if (!_stricmp(stem, "baseball_bat")) return L"球棒";
	if (!_stricmp(stem, "electric_guitar")) return L"吉他";
	if (!_stricmp(stem, "frying_pan")) return L"平底锅";
	if (!_stricmp(stem, "golfclub") || !_stricmp(stem, "golf_club")) return L"球杆";
	if (!_stricmp(stem, "pitchfork")) return L"叉子";
	if (!_stricmp(stem, "shovel")) return L"铲子";
	if (!_stricmp(stem, "tonfa")) return L"警棍";
	if (!_stricmp(stem, "knife") || !_stricmp(stem, "knife_t")) return L"小刀";
	if (!_stricmp(stem, "riotshield") || !_stricmp(stem, "riot_shield")) return L"盾牌";
	return L"近战";
}

static const char* TeamHudModelName(int modelIndex) {
	if (modelIndex <= 0) return nullptr;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return nullptr;
	if (g_teamHudAllowModelAt && GetTickCount() < g_teamHudAllowModelAt) return nullptr;
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
	auto tryPair = [&](int gm, int gn) -> const char* {
		auto getModel = (GetModelFn)VGet(s_mi, gm);
		auto getName = (GetNameFn)VGet(s_mi, gn);
		if (!getModel || !getName || !IsExec((void*)getModel) || !IsExec((void*)getName))
			return nullptr;
		void* mdl = getModel(s_mi, modelIndex);
		if (!mdl || !PtrCommitted(mdl)) return nullptr;
		const char* n = getName(s_mi, mdl);
		if (!n || !PtrCommitted(n)) return nullptr;
		if (!n[0]) return nullptr;
		if (!strstr(n, ".mdl") && !strstr(n, ".MDL"))
			return nullptr;
		return n;
	};
	const char* n = tryPair(1, 3);
	if (n) return n;
	return tryPair(0, 2);
}

static bool TeamHudMeleeStemFromModel(const char* model, char* out, int outN) {
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
	return true;
}

static void TeamHudFillMeleeStem(int wepIdx, char* out, int outN) {
	if (!out || outN <= 0) return;
	out[0] = 0;
	if (wepIdx <= 0 || g_offModelIndex < 0 || g_offModelIndex > 0x4000) return;
	void* wep = EntGet(wepIdx);
	if (!wep) return;
	int* pm = (int*)((uint8_t*)wep + g_offModelIndex);
	if (!PtrCommitted(pm) || *pm <= 0) return;
	const char* model = TeamHudModelName(*pm);
	if (!model) return;
	TeamHudMeleeStemFromModel(model, out, outN);
}

static void TeamHudDrawIcon(int x, int y, int w, int h, int kind, const char* meleeStem) {
	const int id = TeamHudBindWep(kind, meleeStem);
	if (id >= 0) {
		SurfColor(255, 255, 255, 255);
		SurfDrawTextured(x, y, x + w, y + h, id);
		return;
	}
	const wchar_t* lab = (kind == TW_Melee) ? TeamHudMeleeLabel(meleeStem) : TeamHudWepLabel(kind);
	if (!lab || !lab[0] || !g_teamFontSm) return;
	SurfColor(20, 24, 32, 180);
	SurfFill(x, y, x + w, y + h);
	SurfTextAt(g_teamFontSm, x + 2, y + (h > 16 ? 4 : 1), RGB(230, 230, 220), lab);
}

static void TeamHudReadGunAmmo(void* owner, int wepIdx, int kind, int* clip, int* ammo) {
	if (!owner || wepIdx <= 0 || !TeamHudIsGun(kind) || !clip || !ammo) return;
	void* wep = EntGet(wepIdx);
	if (!wep) return;
	if (g_offClip1 >= 0 && g_offClip1 <= 0x4000) {
		int* pc = (int*)((uint8_t*)wep + g_offClip1);
		if (PtrCommitted(pc) && *pc >= 0 && *pc <= 255)
			*clip = *pc;
	}
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

static void TeamHudReadWeps(void* e, TeamHudSlot* s) {
	s->wep = TW_None;
	s->side = TW_None;
	s->clip = -1;
	s->ammo = -1;
	s->nade = TW_None;
	s->kit = TW_None;
	s->pills = TW_None;
	s->melee[0] = 0;
	if (!e) return;
	int pri = TW_None, side = TW_None;
	int priIdx = 0, sideIdx = 0;
	if (g_offMyWeapons >= 0 && g_offMyWeapons <= 0x4000) {
		for (int i = 0; i < 8; ++i) {
			int* ph = (int*)((uint8_t*)e + g_offMyWeapons + i * 4);
			if (!PtrCommitted(ph)) break;
			const int idx = EntIndexFromHandle(*ph);
			if (idx <= 0) continue;
			const int k = TeamHudWepKind(EntNetClassName(idx));
			if (k == TW_None) continue;
			if (TeamHudIsNade(k)) s->nade = k;
			else if (TeamHudIsKit(k)) s->kit = k;
			else if (TeamHudIsPills(k)) s->pills = k;
			else if (TeamHudIsSide(k)) {
				side = k;
				sideIdx = idx;
				if (k == TW_Melee)
					TeamHudFillMeleeStem(idx, s->melee, 24);
			}
			else if (TeamHudIsPrimary(k) || TeamHudIsGun(k)) { pri = k; priIdx = idx; }
		}
	}
	if (pri != TW_None)
		TeamHudReadGunAmmo(e, priIdx, pri, &s->clip, &s->ammo);
	else if (side != TW_None)
		TeamHudReadGunAmmo(e, sideIdx, side, &s->clip, &s->ammo);
	s->wep = pri;
	s->side = side;
}

static void TeamHudGather() {
	for (int i = 0; i < kTeamHudMax; ++i)
		g_teamHudSlots[i].present = false;
	if (!g_entlist) return;
	const int local = EngLocal();
	const DWORD now = GetTickCount();
	const bool refreshName = (g_teamHudNameAt == 0) || (now - g_teamHudNameAt > 400);
	if (refreshName) g_teamHudNameAt = now;
	for (int i = 1; i <= 32; ++i) {
		void* e = EntGet(i);
		if (!EntReadable(e)) continue;
		if (g_offTeam >= 0 && g_offTeam <= 0x4000) {
			int* pt = (int*)((uint8_t*)e + g_offTeam);
			if (!PtrCommitted(pt) || *pt != 2) continue;
		}
		if (g_offZombieClass >= 0 && g_offZombieClass <= 0x4000) {
			int* pz = (int*)((uint8_t*)e + g_offZombieClass);
			if (PtrCommitted(pz) && *pz >= 1 && *pz <= 8) continue;
		}
		int prop = -1;
		if (g_offSurvivorCharacter >= 0 && g_offSurvivorCharacter <= 0x4000) {
			int* pc = (int*)((uint8_t*)e + g_offSurvivorCharacter);
			if (PtrCommitted(pc)) prop = *pc;
		}
		const int faceCh = TeamHudFaceFromProp(prop);
		int ch = faceCh;
		if (ch < 0 || ch >= kTeamHudMax) {
			for (int s = 0; s < kTeamHudMax; ++s) {
				if (!g_teamHudSlots[s].present) { ch = s; break; }
			}
		}
		if (ch < 0 || ch >= kTeamHudMax) continue;
		if (g_teamHudSlots[ch].present) {
			int alt = -1;
			for (int s = 0; s < kTeamHudMax; ++s) {
				if (!g_teamHudSlots[s].present) { alt = s; break; }
			}
			if (alt < 0) continue;
			ch = alt;
		}
		TeamHudSlot& s = g_teamHudSlots[ch];
		s.present = true;
		s.ent = i;
		s.isLocal = (i == local);
		s.character = (faceCh >= 0 && faceCh < kTeamHudMax) ? faceCh : ch;
		s.hp = 0;
		s.maxHp = 100;
		s.tempHp = 0;
		s.dead = false;
		s.incap = false;
		s.hanging = false;
		s.third = false;
		if (g_offHealth >= 0 && g_offHealth <= 0x4000) {
			int* ph = (int*)((uint8_t*)e + g_offHealth);
			if (PtrCommitted(ph)) s.hp = *ph;
		}
		if (g_offMaxHealth >= 0 && g_offMaxHealth <= 0x4000) {
			int* pm = (int*)((uint8_t*)e + g_offMaxHealth);
			if (PtrCommitted(pm) && *pm > 0 && *pm <= 1000) s.maxHp = *pm;
		}
		if (g_offHealthBuffer >= 0 && g_offHealthBuffer <= 0x4000) {
			float* pb = (float*)((uint8_t*)e + g_offHealthBuffer);
			if (PtrCommitted(pb) && *pb > 0.f && *pb < 200.f)
				s.tempHp = (int)(*pb + 0.5f);
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
		if (g_offLifeState >= 0 && g_offLifeState <= 0x4000) {
			unsigned char* pl = (unsigned char*)e + g_offLifeState;
			if (PtrCommitted(pl)) s.dead = (*pl != 0) && !s.incap;
		}
		if (s.hp <= 0 && !s.incap) s.dead = true;
		TeamHudReadWeps(e, &s);
		if (refreshName) {
			PlayerInfo pi{};
			if (EngPlayerInfo(i, &pi) && pi.name[0]) {
				MultiByteToWideChar(CP_UTF8, 0, pi.name, -1, s.name, 24);
				s.name[23] = 0;
			} else {
				wcsncpy(s.name, TeamHudCharName(s.character), 23);
				s.name[23] = 0;
			}
		} else if (!s.name[0]) {
			wcsncpy(s.name, TeamHudCharName(s.character), 23);
			s.name[23] = 0;
		}
	}
}

static void TeamHudPaintCard(int slot, int x, int y, int w, int h, bool preview) {
	const TeamHudSlot& s = g_teamHudSlots[slot];
	const bool empty = !s.present;
	COLORREF accent = s.isLocal ? RGB(255, 196, 92) : RGB(80, 210, 230);
	if (s.dead) accent = RGB(140, 140, 148);
	else if (s.incap || s.hanging) accent = RGB(255, 140, 64);
	else if (s.third) accent = RGB(230, 230, 230);
	SurfColor(0, 0, 0, empty ? 40 : 70);
	SurfFill(x, y, x + w, y + h);
	SurfColor(12, 16, 22, empty ? 70 : 96);
	SurfFill(x + 1, y + 1, x + w - 1, y + h - 1);
	SurfColor(GetRValue(accent), GetGValue(accent), GetBValue(accent), empty ? 90 : 220);
	SurfFill(x, y, x + 3, y + h);
	if (preview) {
		SurfColor(GetRValue(accent), GetGValue(accent), GetBValue(accent), 200);
		SurfOutline(x, y, x + w, y + h);
	}
	const int faceCap = 64;
	const int face = (h - 16) < faceCap ? (h - 16) : faceCap;
	const int fx = x + 8;
	const int fy = y + (h - face) / 2;
	const int faceCh = empty ? slot : ((s.character >= 0 && s.character < 8) ? s.character : slot);
	const int idFace = TeamHudBindFace(faceCh, s.incap && !empty);
	if (idFace >= 0) {
		SurfColor(255, 255, 255, empty ? 70 : 255);
		SurfDrawTextured(fx, fy, fx + face, fy + face, idFace);
	} else {
		SurfColor(30, 36, 46, 200);
		SurfFill(fx, fy, fx + face, fy + face);
		if (g_teamFontSm)
			SurfTextAt(g_teamFontSm, fx + 4, fy + face / 2 - 8, RGB(200, 200, 200), TeamHudCharName(faceCh));
	}
	const int tx = fx + face + 10;
	if (!g_teamFont) return;
	if (empty) {
		SurfTextAt(g_teamFont, tx, y + 10, RGB(140, 148, 160), TeamHudCharName(faceCh));
		if (g_teamFontSm)
			SurfTextAt(g_teamFontSm, tx, y + 34, RGB(110, 118, 128), L"不在局内");
		return;
	}
	SurfTextAt(g_teamFont, tx, y + 6, RGB(236, 240, 244), s.name);
	const wchar_t* st = nullptr;
	COLORREF stc = RGB(200, 200, 200);
	if (s.dead) { st = L"死亡"; stc = RGB(180, 180, 186); }
	else if (s.hanging) { st = L"挂边"; stc = RGB(255, 170, 80); }
	else if (s.incap) { st = L"倒地"; stc = RGB(255, 140, 64); }
	else if (s.third) { st = L"黑白"; stc = RGB(230, 230, 230); }
	if (st && g_teamFontSm)
		SurfTextAt(g_teamFontSm, x + w - 52, y + 6, stc, st);

	const int barX = tx;
	const int barY = y + h - 14;
	const int barW = w - (barX - x) - 10;
	const int barH = 8;
	int cap = s.incap ? 300 : (s.maxHp > 0 ? s.maxHp : 100);
	if (cap < 1) cap = 1;
	int hp = s.hp;
	if (hp < 0) hp = 0;
	if (hp > cap) hp = cap;
	int fill = hp * barW / cap;
	COLORREF hc = RGB(80, 220, 120);
	if (s.dead) hc = RGB(90, 90, 96);
	else if (s.incap) hc = RGB(255, 150, 70);
	else if (hp <= 24) hc = RGB(255, 72, 72);
	else if (hp <= 39) hc = RGB(255, 186, 64);
	SurfColor(255, 255, 255, 28);
	SurfFill(barX, barY, barX + barW, barY + barH);
	SurfColor(GetRValue(hc), GetGValue(hc), GetBValue(hc), 220);
	if (fill > 0)
		SurfFill(barX, barY, barX + fill, barY + barH);
	if (!s.incap && s.tempHp > 0) {
		int tw = s.tempHp * barW / cap;
		if (tw > barW - fill) tw = barW - fill;
		if (tw > 0) {
			SurfColor(255, 220, 90, 180);
			SurfFill(barX + fill, barY, barX + fill + tw, barY + barH);
		}
	}
	wchar_t hpBuf[16]{};
	swprintf_s(hpBuf, 16, L"%d", s.hp);
	SurfTextAt(g_teamFont, tx, y + 26, hc, hpBuf);

	const int item = 28;
	int ix = x + w - 8 - item;
	const int iy = y + h - 44;
	if (s.pills) { TeamHudDrawIcon(ix, iy, item, item, s.pills, nullptr); ix -= item + 4; }
	if (s.kit) { TeamHudDrawIcon(ix, iy, item, item, s.kit, nullptr); ix -= item + 4; }
	if (s.nade) { TeamHudDrawIcon(ix, iy, item, item, s.nade, nullptr); ix -= item + 4; }

	const int priKind = s.wep ? s.wep : s.side;
	const int secKind = s.wep ? s.side : TW_None;
	int wx = tx;
	const int wy = y + h - 42;
	if (priKind) {
		TeamHudDrawIcon(wx, wy, 64, 28, priKind, (priKind == TW_Melee) ? s.melee : nullptr);
		wx += 68;
		wchar_t ammoBuf[24]{};
		if (TeamHudIsGun(priKind) && s.clip >= 0) {
			if (s.ammo >= 0)
				swprintf_s(ammoBuf, 24, L"%d / %d", s.clip, s.ammo);
			else
				swprintf_s(ammoBuf, 24, L"%d", s.clip);
		} else if (priKind) {
			swprintf_s(ammoBuf, 24, L"%s", TeamHudWepLabel(priKind));
		}
		if (ammoBuf[0]) {
			unsigned long ammoFont = g_teamFont ? g_teamFont : g_teamFontSm;
			SurfTextAt(ammoFont, wx, wy + 6, RGB(220, 226, 232), ammoBuf);
			wx += SurfTextWFont(ammoFont, ammoBuf) + 12;
		}
	}
	if (secKind && wx + 52 < ix) {
		TeamHudDrawIcon(wx, wy + 4, 48, 24, secKind, (secKind == TW_Melee) ? s.melee : nullptr);
	}
}

static void TeamHudPaintHud() {
	const bool preview = g_menuVisible && !g_menuParked && g_menuPage == 15;
	if (!g_optTeamHud && !preview) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (!MenuEnsureSurf()) return;
	TeamHudEnsureFonts();
	if (!g_teamFont) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	TeamHudGather();
	const int cardW = 368;
	const int cardH = 96;
	for (int i = 0; i < kTeamHudMax; ++i) {
		if (!g_teamHudSlots[i].present && !(preview && i == g_teamHudSel))
			continue;
		int x = 0, y = 0;
		HudPlaceAbs(sw, sh, cardW, cardH, g_teamHudX[i], g_teamHudY[i], &x, &y);
		TeamHudPaintCard(i, x, y, cardW, cardH, preview && i == g_teamHudSel);
	}
}

static void MenuPaintEngine() {
	if (!g_menuVisible || g_menuParked) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!MenuEnsureSurf()) return;
	CrashMark(kBcMenu);

	MenuUpdateScale();
	SurfEnsureFont((int)(g_uiFont * g_menuScale + 0.5f));

	const COLORREF bg = UiBgRgb();
	SurfColor(GetRValue(bg), GetGValue(bg), GetBValue(bg), UiBgA());
	SurfFill(g_menuOx, g_menuOy, g_menuOx + g_menuW, g_menuOy + g_menuH);
	static bool s_fillOk = false;
	if (!s_fillOk) {
		s_fillOk = true;
		Log("menu: engine fill ok %dx%d at %d,%d", g_menuW, g_menuH, g_menuOx, g_menuOy);
	}
	const COLORREF cTitle = UiTitleRgb();
	const COLORREF cText = UiTextRgb();
	const COLORREF cHint = UiHintRgb();
	const COLORREF cMuted = UiMutedRgb();
	const COLORREF cAccent = UiAccentRgb();
	const COLORREF cSection = UiSectionRgb();
	SurfColor(GetRValue(cText), GetGValue(cText), GetBValue(cText), 150);
	SurfOutline(g_menuOx, g_menuOy, g_menuOx + g_menuW, g_menuOy + g_menuH);

	auto line = [&](int y, COLORREF color, const wchar_t* text) {
		SurfText(MenuSx(14), MenuSy(y), color, text);
	};
	auto rule = [&]() {
		SurfColor(GetRValue(cText), GetGValue(cText), GetBValue(cText), 180);
		SurfLine(MenuSx(10), MenuSy(36), MenuSx(kMenuDesignW - 10), MenuSy(36));
	};

	auto onoff = [](bool v) { return v ? L"【开】" : L"【关】"; };
	auto col = [](bool v) { return v ? RGB(50, 255, 50) : RGB(255, 50, 50); };
	wchar_t buf[192]{};

	if (g_menuPage == 0) {
		swprintf_s(buf, 192, L"Skeeto  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(52, cText, L"[1] 命中反馈");
		line(78, cText, L"[2] 切换准星");
		line(104, cText, L"[3] 自定义 HUD");
		line(130, cText, L"[4] 客户端体验优化");
		line(156, cText, L"[5] 本地服体验优化");
		line(182, cText, L"[6] 菜单样式");
		line(208, cText, L"[7] 其他选项");
		line(250, cHint, L"默认绑定 [ 打开菜单 · [/Esc/0 关闭");
		line(290, cAccent, L"[0] 关闭");
	} else if (g_menuPage == 2) {
		swprintf_s(buf, 192, L"解决特感击杀卡顿  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();

		swprintf_s(buf, 192, L"[1] Loading阶段Dispatch粒子进行预热 %s", onoff(g_optPtDispWarm));
		line(52, col(g_optPtDispWarm), buf);

		auto wrap = [&](int y, COLORREF color, const wchar_t* text) -> int {
			return SurfWrapDesign(y, color, text);
		};
		int y = 88;
		y = wrap(y, RGB(255, 70, 70),
			L"如果每局第一次击杀特感会卡顿，开启此选项会大幅改善。");
		y = wrap(y, RGB(255, 160, 50),
			L"原本就不会卡顿的，不建议开启此选项，开启后对于屏幕粒子会在加载完一瞬间大量触发，代理无法改变屏幕粒子位置，但是落雷等世界粒子不会出现此现象，SKT会把它在地图之外进行Dispatch，不会出现在你脸上。");
		y = wrap(y, cMuted,
			L"当前版本不开这个选项，一般也不会再出现特感击杀卡顿。开关要等下次进图才生效。");

		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 7) {
		swprintf_s(buf, 192, L"其他选项  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(52, cText, L"[1] 解决特感击杀卡顿问题");
		line(76, cHint, L"    杂项入口，以后别的零散选项也会放这里");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 9) {
		swprintf_s(buf, 192, L"客户端体验优化  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"此页选项服务器内也会生效");

		int y = 88;
		swprintf_s(buf, 192, L"[1] 投掷物落点预览 %s", onoff(g_optClientThrowLand));
		line(y, col(g_optClientThrowLand), buf);
		y += 20;
		y = SurfWrapDesign(y, cHint, L"    手持燃烧瓶/土制炸弹/胆汁时显示预计落点与轨迹", 10);

		swprintf_s(buf, 192, L"[2] 关闭特感布偶/尸体 %s", onoff(g_optClientNoCorpseSi));
		line(y, col(g_optClientNoCorpseSi), buf);
		y += 20;
		y = SurfWrapDesign(y, cHint, L"    特感/坦克死后立刻消失", 2);
		y = SurfWrapDesign(y, cHint, L"    关闭此选项时，需要同步关闭选项[4]才会生效。", 10);

		swprintf_s(buf, 192, L"[3] 关闭普感布偶/尸体 %s", onoff(g_optClientNoCorpseCi));
		line(y, col(g_optClientNoCorpseCi), buf);
		y += 20;
		y = SurfWrapDesign(y, cHint, L"    小僵尸/女巫死后立刻消失", 10);

		swprintf_s(buf, 192, L"[4] 特感血条 %s", onoff(g_optClientInfectedHp));
		line(y, col(g_optClientInfectedHp), buf);
		y += 20;
		y = SurfWrapDesign(y, cHint, L"    此血条绑定特感模型，且受到伤害后才会显示，死亡立即消失", 10);

		swprintf_s(buf, 192, L"[5] 伤害数字 %s", onoff(g_optClientDmgNum));
		line(y, col(g_optClientDmgNum), buf);
		y += 20;
		y = SurfWrapDesign(y, cHint, L"    只对特感/Tank，数字在模型外侧随机出现，不挡身体和血条", 10);

		swprintf_s(buf, 192, L"[6] 常驻导演系统HUD %s", onoff(g_optClientDirectorHud));
		line(y, col(g_optClientDirectorHud), buf);
		y += 20;
		SurfWrapDesign(y, cHint,
			L"    此HUD对绝境玩家作用比较大。开启此选项之后，导演HUD会常驻，过关也不会重置");

		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 8) {
		swprintf_s(buf, 192, L"本地服体验优化  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		const bool host = ListenDetectHost();
		const COLORREF lockCol = host ? col(true) : RGB(255, 80, 80);
		line(48, lockCol, host ? L"状态 【本地听服主机 · 可改】" : L"状态 【未在本地主机 · 已锁定】");
		line(68, cHint, L"自己开的本地服可用（含大厅本地联机）。进别人的服会锁定。");

		line(100, cSection, L"—— Tick / 网络 ——");
		const wchar_t* tickName = L"30（原版）";
		if (g_optLocalTick == 60) tickName = L"60";
		else if (g_optLocalTick == 100) tickName = L"100";
		else if (g_optLocalTick == 128) tickName = L"128";
		swprintf_s(buf, 192, L"[1] Tick 【%s】", tickName);
		line(122, host ? cText : cMuted, buf);
		line(142, host ? cHint : cMuted, L"    调完此选项之后要经过一次加载才会生效。");
		{
			const int eng = ListenEngineTickGuess();
			if (g_optLocalTick == 30)
				swprintf_s(buf, 192, L"    档位 30/60/100/128");
			else if (eng != g_optLocalTick)
				swprintf_s(buf, 192, L"    目标 %d · 当前约 %d · 过关后生效", g_optLocalTick, eng);
			else
				swprintf_s(buf, 192, L"    当前约 %d。128 很吃 CPU", eng);
			line(162, host ? cHint : cMuted, buf);
		}
		line(182, cMuted, L"    不要加启动项 -tickrate；也不要和 SourceMod / l4dtoolz 叠用");

		if (g_optLocalNb == 1)
			swprintf_s(buf, 192, L"[2] Nextbot刷新率 【0.066 省CPU】");
		else if (g_optLocalNb == 2)
			swprintf_s(buf, 192, L"[2] Nextbot刷新率 【0.024 更猛】");
		else if (g_optLocalNb == 3)
			swprintf_s(buf, 192, L"[2] Nextbot刷新率 【0.1 原版】");
		else if (g_optLocalTick != 30)
			swprintf_s(buf, 192, L"[2] Nextbot刷新率 【自动 %.3f】", ListenNbAutoFreq(g_optLocalTick));
		else
			swprintf_s(buf, 192, L"[2] Nextbot刷新率 【自动】");
		line(214, host ? cText : cMuted, buf);
		line(234, host ? cHint : cMuted, L"    如果发现特感莫名其妙变智障，多半和这个有关，调整参数后重进游戏即可。");
		line(254, host ? cHint : cMuted, L"    数字越小敌人的反应与决策频率越高，同时也越吃CPU性能。");

		swprintf_s(buf, 192, L"[3] 允许 0 lerp %s", onoff(g_optLocalAllow0Lerp));
		line(286, host ? col(g_optLocalAllow0Lerp) : cMuted, buf);
		line(306, host ? cHint : cMuted, L"    允许客户端把 lerp 调到 0");

		if (g_optLocalLerp < 0)
			swprintf_s(buf, 192, L"[4] Lerp 【不改】");
		else
			swprintf_s(buf, 192, L"[4] Lerp 【%dms】", g_optLocalLerp);
		line(338, host ? cText : cMuted, buf);
		if (g_optLocalLerp == 0 && !g_optLocalAllow0Lerp)
			line(358, host ? RGB(255, 180, 80) : cMuted, L"    选 0 请先开上面的「允许 0 lerp」");
		else
			line(358, host ? cHint : cMuted, L"    不改 / 0 / 24 / 40 / 60 / 100");

		line(390, cSection, L"—— 本地玩法 ——");
		swprintf_s(buf, 192, L"[5] 单人也可闲置 %s", onoff(g_optLocalIdleSolo));
		line(410, host ? col(g_optLocalIdleSolo) : cMuted, buf);
		line(430, host ? cHint : cMuted, L"    移除闲置条件的限制，并静音导演报错字符串");

		swprintf_s(buf, 192, L"[6] 去掉闲置延迟 %s", onoff(g_optLocalIdleNoDelay));
		line(462, host ? col(g_optLocalIdleNoDelay) : cMuted, buf);
		line(482, host ? cHint : cMuted, L"    原版闲置响应不灵敏，开启此选项会修复这一点，按即闲置");

		swprintf_s(buf, 192, L"[7] 换人物功能 %s", onoff(g_optLocalCharChange));
		line(514, host ? col(g_optLocalCharChange) : cMuted, buf);
		line(534, host ? cHint : cMuted, L"    等价于!csm，开启会对所有生还者模型进行precache，可能会影响帧率。默认关闭。");

		line(566, (host && g_optLocalCharChange) ? cText : cMuted, L"[8] 选人物…");
		line(586, (host && g_optLocalCharChange) ? cHint : cMuted,
			g_optLocalCharChange ? L"    进入人物列表" : L"    请先打开上面的「换人物功能」");

		if (g_optLocalAa == 100)
			swprintf_s(buf, 192, L"[9] 空中加速 【100】");
		else if (g_optLocalAa == 400)
			swprintf_s(buf, 192, L"[9] 空中加速 【400】");
		else if (g_optLocalAa == 1000)
			swprintf_s(buf, 192, L"[9] 空中加速 【1000】");
		else
			swprintf_s(buf, 192, L"[9] 空中加速 【原版 10】");
		line(618, host ? cText : cMuted, buf);
		line(638, host ? cHint : cMuted, L"    即sv_airaccelerate，影响空中转向速度");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 10) {
		swprintf_s(buf, 192, L"选人物  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		const bool host = ListenDetectHost();
		const bool ok = host && g_optLocalCharChange;
		line(48, ok ? col(true) : RGB(255, 80, 80),
			!host ? L"状态 【未在本地主机 · 已锁定】"
			: (g_optLocalCharChange ? L"状态 【可换】" : L"状态 【功能未开】"));
		line(68, cHint, L"需在生还者队。一代/二代图都会预缓存全套模型");
		line(100, ok ? cText : cMuted, L"[1] Nick 尼克");
		line(124, ok ? cText : cMuted, L"[2] Rochelle 黑妹");
		line(148, ok ? cText : cMuted, L"[3] Coach 教练");
		line(172, ok ? cText : cMuted, L"[4] Ellis 艾利斯");
		line(196, ok ? cText : cMuted, L"[5] Bill 比尔");
		line(220, ok ? cText : cMuted, L"[6] Zoey 佐伊");
		line(244, ok ? cText : cMuted, L"[7] Francis 弗朗西斯");
		line(268, ok ? cText : cMuted, L"[8] Louis 路易斯");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 3) {
		swprintf_s(buf, 192, L"切换准星  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();

		int y = 48;
		line(y, cSection, L"—— 静态准星（屏幕叠加）——");
		y += 16;
		swprintf_s(buf, 192, L"[1] 静态准星 %s", onoff(g_optXhair));
		line(y, col(g_optXhair), buf);
		y += 16;
		y = SurfWrapDesign(y, cHint, L"    外置叠加层，用来瞄准敌人；可与动态同时开", 2);
		y = SurfWrapDesign(y, RGB(255, 72, 72), L"    如果遇到键盘失灵现象，很有可能和这个有关，可以先关闭试试。", 2);
		y = SurfWrapDesign(y, RGB(255, 72, 72), L"    目前还没有查清具体的原因。", 6);

		const XhairTune& xt = XhairT();
		const wchar_t* st = kXhairStyleNames[g_xhairStyle];
		const wchar_t* cn = kXhairColorNames[xt.color];
		swprintf_s(buf, 192, L"[3] 样式 【%s】", st);
		line(y, cText, buf);
		y += 16;
		swprintf_s(buf, 192, L"[4] 颜色 【%s】", cn);
		line(y, kXhairColorRgb[xt.color], buf);
		y += 16;

		swprintf_s(buf, 192, L"[5] 整体大小 【%d%%】", xt.size);
		line(y, RGB(180, 220, 255), buf);
		y += 16;
		y = SurfWrapDesign(y, cHint, L"    会根据游戏分辨率自适应大小", 6);

		swprintf_s(buf, 192, L"[6] 长度 【%d】", xt.length);
		line(y, cText, buf);
		y += 16;
		swprintf_s(buf, 192, L"[7] 间隙 【%d】", xt.gap);
		line(y, cText, buf);
		y += 16;
		swprintf_s(buf, 192, L"[8] 粗细 【%d】", xt.thick);
		line(y, cText, buf);
		y += 16;
		swprintf_s(buf, 192, L"[9] 中心点 【%s%d】", xt.dot <= 0 ? L"关 " : L"", xt.dot);
		line(y, cText, buf);
		y += 16;
		swprintf_s(buf, 192, L"[O] 描边 【%s%d】", xt.outline <= 0 ? L"关 " : L"", xt.outline);
		line(y, cText, buf);
		y += 16;

		swprintf_s(buf, 192, L"静态透明度 【%d%%】  按 - / = 调节", xt.alpha);
		line(y, RGB(180, 220, 255), buf);
		y += 16;
		line(y, cText, L"[R] 重置为默认参数（仅当前静态样式）");
		y += 18;

		line(y, cSection, L"—— 动态准星（引擎散布圈）——");
		y += 16;
		swprintf_s(buf, 192, L"[2] 动态准星 %s", onoff(g_optXhairRing));
		line(y, col(g_optXhairRing), buf);
		y += 16;
		y = SurfWrapDesign(y, cHint, L"    内置引擎层，用来判断散布", 2);
		y = SurfWrapDesign(y, cHint, L"    建议把原版十字线透明度调到 0（控制台 cl_crosshair_alpha 0）", 6);
		{
			const int rc = (g_xhairRingColor >= 0 && g_xhairRingColor < kXhairColorCount) ? g_xhairRingColor : 1;
			swprintf_s(buf, 192, L"[C] 颜色 【%s】", kXhairColorNames[rc]);
			line(y, kXhairColorRgb[rc], buf);
		}
		y += 16;
		swprintf_s(buf, 192, L"[M] 范围 【%s】", XhairRingModeName());
		line(y, cText, buf);
		y += 16;
		swprintf_s(buf, 192, L"[T] 透明度 【%d%%】", g_xhairRingAlpha);
		line(y, RGB(180, 220, 255), buf);
		y += 18;

		line(y, cSection, L"—— 贴图准星（VTF贴图）——");
		y += 16;
		y = SurfWrapDesign(y, cHint, L"    与工坊现有所有准星mod都兼容，也可自行DIY新增", 4);
		swprintf_s(buf, 192, L"[X] 贴图准星 %s", onoff(g_optXhairTex));
		line(y, col(g_optXhairTex), buf);
		y += 16;
		{
			wchar_t matW[64]{};
			const char* m = g_xhairTexMat[0] ? g_xhairTexMat : "(未选)";
			const char* slash = strrchr(m, '/');
			MultiByteToWideChar(CP_UTF8, 0, slash ? slash + 1 : m, -1, matW, 64);
			swprintf_s(buf, 192, L"[V] 材质 【%s】", matW);
			line(y, g_optXhairTex ? RGB(50, 255, 200) : cMuted, buf);
		}
		y += 16;
		swprintf_s(buf, 192, L"[F] 铺满屏幕 %s", onoff(g_optXhairTexFull));
		line(y, g_optXhairTex ? col(g_optXhairTexFull) : cMuted, buf);
		y += 16;
		if (g_optXhairTexFull)
			swprintf_s(buf, 192, L"[B] 大小 【全屏时无效】");
		else
			swprintf_s(buf, 192, L"[B] 大小 【%d】", g_xhairTexSize);
		line(y, (g_optXhairTex && !g_optXhairTexFull) ? RGB(180, 220, 255) : cMuted, buf);
		y += 16;
		SurfWrapDesign(y, cMuted,
			L"    不依赖HUD的res文件，只需要把vmt/vtf放到 materials/skeeto/xhair/（松散或 VPK 均可）。同时开静态准星时，静态会画在贴图上面。");

		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 4) {
		swprintf_s(buf, 192, L"菜单样式  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();

		line(48, cSection, L"—— 布局 ——");
		swprintf_s(buf, 192, L"[1] 整体大小 【%d%%】", g_uiSizePct);
		line(72, RGB(180, 220, 255), buf);
		line(92, cHint, L"    在分辨率自适应之后再缩放");

		swprintf_s(buf, 192, L"[2] 左右位置 【%s】", kUiAlignXNames[g_uiAlignX]);
		line(116, cText, buf);
		swprintf_s(buf, 192, L"[3] 上下位置 【%s】", kUiAlignYNames[g_uiAlignY]);
		line(136, cText, buf);

		line(168, cSection, L"—— 样式 ——");
		swprintf_s(buf, 192, L"[4] 背景透明度 【%d%%】  也可按 - / = 调节", g_uiBgAlpha);
		line(192, RGB(180, 220, 255), buf);
		swprintf_s(buf, 192, L"[5] 背景颜色 【%s】", kUiBgNames[g_uiBg]);
		line(212, cText, buf);
		{
			const int tw = SurfTextW(buf);
			const int fh = g_menuFontTall > 0 ? g_menuFontTall : 18;
			const int box = (int)(28 * g_menuScale + 0.5f);
			const int boxH = fh > 8 ? fh - 4 : fh;
			SurfColorSwatch(
				MenuSx(14) + tw + (int)(10 * g_menuScale + 0.5f),
				MenuSy(212) + 2,
				box, boxH, UiBgRgb());
		}
		swprintf_s(buf, 192, L"[6] 文字颜色 【%s】", kUiTextNames[g_uiText]);
		line(232, cText, buf);
		swprintf_s(buf, 192, L"[7] 标题颜色 【%s】", kUiTitleNames[g_uiTitle]);
		line(252, cTitle, buf);
		swprintf_s(buf, 192, L"[8] 字号 【%d】", g_uiFont);
		line(272, cText, buf);
		swprintf_s(buf, 192, L"[9] 文字描边 %s", onoff(g_uiOutline != 0));
		line(292, col(g_uiOutline != 0), buf);

		line(328, cText, L"[R] 恢复默认菜单样式");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 5) {
		swprintf_s(buf, 192, L"击杀统计 HUD  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		swprintf_s(buf, 192, L"[1] 显示击杀 HUD %s", onoff(g_optElim));
		line(52, col(g_optElim), buf);
		swprintf_s(buf, 192, L"[2] 统计模式 【%s】", ElimModeLabel());
		line(76, cText, buf);
		line(96, cHint, L"    过图清零；团灭按此项决定是否清零；不重置则回大厅才清");
		line(128, RGB(180, 220, 255), L"    WASD 全屏挪位置（按住连移，Shift 大步）");
		swprintf_s(buf, 192, L"[3] 字号 【%d】  也可按 - / = 调节", g_elimFont);
		line(156, RGB(180, 220, 255), buf);
		swprintf_s(buf, 192, L"[4] 语言 【%s】", g_elimLang ? L"中文" : L"English");
		line(188, cText, buf);
		swprintf_s(buf, 192, L"[5] 显示 【%s】", ElimPresetName());
		line(216, cText, buf);
		line(244, cText, L"[6] 单独开关");
		line(264, cHint, L"    精简/完整是快捷预设；单独开关可只留某几项");
		line(300, cText, L"[R] 清零当前计数");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 6) {
		swprintf_s(buf, 192, L"击杀 HUD 单独开关  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"关掉的项不画在 HUD 上，计数仍累计");
		for (int i = 0; i < kElimLineCount; ++i) {
			const bool on = ElimLineOn(i);
			wchar_t row[96]{};
			ElimFormatLine(i, row, 96);
			swprintf_s(buf, 192, L"[%d] %s  %s", i + 1, row, onoff(on));
			line(72 + i * 32, col(on), buf);
		}
		line(276, cText, L"[A] 全部显示");
		line(300, cText, L"[S] 仅特感 / 普感");
		line(332, cMuted, L"也可在上一页用 [8] 在完整和精简之间切换");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 11) {
		swprintf_s(buf, 192, L"自定义 HUD  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"屏幕叠加信息，和命中反馈、准星分开");
		line(76, cText, L"[1] 击杀统计 HUD");
		line(96, cMuted, L"    本地击杀计数，可调位置和显示项");
		line(124, cText, L"[2] 世界时间");
		line(144, cMuted, L"    显示本机当前时间，可调位置");
		line(172, cText, L"[3] 速度表");
		line(192, cMuted, L"    本地水平速度，可调位置");
		line(220, cText, L"[4] 局内计时器");
		line(240, cMuted, L"    从回合开始计时，可调位置");
		line(268, cText, L"[5] 队友血条");
		line(288, cMuted, L"    最多8人，每人可单独挪位置");
		swprintf_s(buf, 192, L"[6] 隐藏原版 HUD %s", onoff(g_optHudHide));
		line(316, col(g_optHudHide), buf);
		line(336, cHint, L"    队友框、武器弹药、雷达/速度/计时可单独开关");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 12) {
		swprintf_s(buf, 192, L"世界时间  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"本机本地时间，不是游戏内时间");
		swprintf_s(buf, 192, L"[1] 显示世界时间 %s", onoff(g_optClock));
		line(80, col(g_optClock), buf);
		line(112, RGB(180, 220, 255), L"    WASD 全屏挪位置（按住连移，Shift 大步）");
		swprintf_s(buf, 192, L"[2] 字号 【%d】  也可按 - / = 调节", g_clockFont);
		line(144, RGB(180, 220, 255), buf);
		line(188, cText, L"[R] 复位位置");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 13) {
		swprintf_s(buf, 192, L"速度表  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"本机水平速度，不是垂直速度");
		swprintf_s(buf, 192, L"[1] 显示速度表 %s", onoff(g_optSpeed));
		line(80, col(g_optSpeed), buf);
		line(112, RGB(180, 220, 255), L"    WASD 全屏挪位置（按住连移，Shift 大步）");
		swprintf_s(buf, 192, L"[2] 字号 【%d】  也可按 - / = 调节", g_speedFont);
		line(144, RGB(180, 220, 255), buf);
		line(188, cText, L"[R] 复位位置");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 14) {
		swprintf_s(buf, 192, L"局内计时器  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"从回合开始计时，不是本机时钟");
		swprintf_s(buf, 192, L"[1] 显示计时器 %s", onoff(g_optTimer));
		line(80, col(g_optTimer), buf);
		line(112, RGB(180, 220, 255), L"    WASD 全屏挪位置（按住连移，Shift 大步）");
		swprintf_s(buf, 192, L"[2] 字号 【%d】  也可按 - / = 调节", g_timerFont);
		line(144, RGB(180, 220, 255), buf);
		line(188, cText, L"[R] 复位位置");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 15) {
		swprintf_s(buf, 192, L"队友血条  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"生还者血量、倒地、主副武器弹药和道具槽，最多8人");
		swprintf_s(buf, 192, L"[1] 显示队友血条 %s", onoff(g_optTeamHud));
		line(80, col(g_optTeamHud), buf);
		{
			const TeamHudSlot& sl = g_teamHudSlots[g_teamHudSel];
			swprintf_s(buf, 192, L"[2] 正在移动 【%s】  %d/8",
				TeamHudCharName(g_teamHudSel), g_teamHudSel + 1);
			line(112, RGB(180, 220, 255), buf);
			if (sl.present && sl.name[0])
				swprintf_s(buf, 192, L"    局内：%s%s", sl.name, sl.isLocal ? L"（你）" : L"");
			else
				swprintf_s(buf, 192, L"    这一栏局内暂时没人，仍可先摆位置");
			line(132, cMuted, buf);
		}
		line(156, RGB(180, 220, 255), L"    WASD 挪这一栏（按住连移，Shift 大步）");
		swprintf_s(buf, 192, L"[3] 字号 【%d】  也可按 - / = 调节", g_teamHudFont);
		line(188, RGB(180, 220, 255), buf);
		line(228, cText, L"[R] 复位全部位置");
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else if (g_menuPage == 16) {
		swprintf_s(buf, 192, L"隐藏原版 HUD  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();
		line(48, cHint, L"按块藏，互不影响。关某一项只把那一块还原");
		swprintf_s(buf, 192, L"[1] 队友框 / 血条 %s", onoff(g_optHudHideTeam));
		line(80, col(g_optHudHideTeam), buf);
		line(100, cMuted, L"    原版左下角自己和队友的血条框");
		swprintf_s(buf, 192, L"[2] 武器图标和弹药 %s", onoff(g_optHudHideWep));
		line(128, col(g_optHudHideWep), buf);
		line(148, cMuted, L"    只藏画面，数字键切枪 / 用道具不受影响");
		swprintf_s(buf, 192, L"[3] 雷达 / 速度表 / 计时器 %s", onoff(g_optHudHidePickup));
		line(176, col(g_optHudHidePickup), buf);
		line(196, cMuted, L"    藏 itempickup 里那一套（拾取提示会一起藏）");
		{
			const bool allOn = g_optHudHideTeam && g_optHudHideWep && g_optHudHidePickup;
			swprintf_s(buf, 192, L"[4] 全部%s", allOn ? L"关闭" : L"开启");
			line(228, col(allOn), buf);
		}
		line(kMenuDesignH - 32, cAccent, L"[0] 返回    [/Esc 关闭");
	} else {
		swprintf_s(buf, 192, L"命中反馈  v%s", SKEETO_VERSION_W);
		line(10, cTitle, buf);
		rule();

		auto srcLabel = [](bool useSiPack) {
			return useSiPack ? L"【使用特感专用素材】" : L"【使用当前普感素材】";
		};
		auto srcCol = [](bool useSiPack, bool siOn) -> COLORREF {
			if (!siOn) return RGB(120, 120, 120);
			return useSiPack ? RGB(50, 255, 50) : RGB(255, 200, 50);
		};

		const char* ciId = DlcGetSelected("ci");
		const int ciIdx = DlcIndexOfSelected("ci");
		const char* ciNameA = (ciIdx >= 0) ? DlcChannelNameAt("ci", ciIdx) : "关";
		wchar_t ciNameW[64]{};
		MultiByteToWideChar(CP_UTF8, 0, ciNameA ? ciNameA : "关", -1, ciNameW, 64);

		const int siIdx = DlcIndexOfSelected("si");
		const bool siOn = siIdx >= 0;
		const char* siNameA = siOn ? DlcChannelNameAt("si", siIdx) : "关";
		wchar_t siNameW[64]{};
		MultiByteToWideChar(CP_UTF8, 0, siNameA ? siNameA : "关", -1, siNameW, 64);

		line(42, cSection, L"—— 1. 总开关 ——");

		swprintf_s(buf, 192, L"[1] 总音效开关 %s", onoff(g_optSound));
		line(64, col(g_optSound), buf);
		swprintf_s(buf, 192, L"    反馈音量 【%d%%】  按 - / = 调节", g_optSfxVol);
		line(84, g_optSound ? RGB(180, 220, 255) : RGB(120, 120, 120), buf);
		line(102, cHint, L"    只调命中反馈音效，不影响游戏音量/音乐音量");

		swprintf_s(buf, 192, L"[2] 总画面开关 %s", onoff(g_optIcon));
		line(126, col(g_optIcon), buf);
		line(146, cHint, L"    隐藏全部图标与粒子");

		const wchar_t* hitLabel = L"【特感+普感】";
		COLORREF hitCol = RGB(50, 255, 50);
		if (g_optHitMode == 0) {
			hitLabel = L"【全部关闭】";
			hitCol = RGB(255, 50, 50);
		} else if (g_optHitMode == 1) {
			hitLabel = L"【仅特感】";
			hitCol = RGB(255, 200, 50);
		}
		swprintf_s(buf, 192, L"[3] 击中反馈(hit)单独开关 %s", hitLabel);
		line(170, hitCol, buf);
		line(190, cHint, L"    循环：仅特感(默认) → 特感+普感 → 全关");

		line(238, cSection, L"—— 2. 选素材包 ——");

		swprintf_s(buf, 192, L"[4] 普感素材包 【%s】", ciNameW);
		line(260, (ciId && ciId[0] && _stricmp(ciId, "off") != 0) ? RGB(50, 255, 50) : RGB(255, 50, 50), buf);
		line(280, cHint, L"    普感击杀/爆头/Hit 用哪一套");
		line(300, RGB(120, 180, 220), L"    可自行DIY新增，JSON文件可配置所有路径与规则");

		swprintf_s(buf, 192, L"[5] 特感素材包 【%s】", siNameW);
		line(328, siOn ? RGB(80, 200, 255) : RGB(255, 50, 50), buf);
		line(348, cHint, L"    关=特感击杀整套跟普感；可选瓦/CF 等");
		line(368, RGB(120, 180, 220), L"    可自行DIY新增，JSON文件可配置所有路径与规则");

		line(396, cSection, L"—— 3. 特感击杀怎么播 ——");
		line(416, cHint,
			siOn ? L"    （画面与音效可分开选，互不影响）"
			     : L"    （先在 [5] 选特感素材包，下面两项才生效）");

		swprintf_s(buf, 192, L"[6] 特感画面 %s", srcLabel(g_optSiVisual));
		line(440, srcCol(g_optSiVisual, siOn), buf);
		line(460, cHint, L"    图标+粒子来源");

		swprintf_s(buf, 192, L"[7] 特感音效 %s", srcLabel(g_optSiSound));
		line(484, srcCol(g_optSiSound, siOn), buf);
		line(504, cHint, L"    声音来源");

		line(532, cSection, L"—— 4. 额外 ——");

		swprintf_s(buf, 192, L"[8] 特殊击杀反馈 %s", onoff(g_optKillFx));
		line(554, col(g_optKillFx), buf);
		line(574, cHint, L"    空爆 / 断舌 / Crown 等");
		line(594, RGB(120, 180, 220), L"    可自行DIY新增，JSON文件可配置所有路径与规则");

		swprintf_s(buf, 192, L"[9] 友伤反馈 %s", onoff(g_optFf));
		line(622, col(g_optFf), buf);
		line(642, cHint, L"    仅真实掉血触发");
		line(662, RGB(120, 180, 220), L"    可自行DIY新增，JSON文件可配置所有路径与规则");

		line(688, cAccent, L"[0] 返回    [/Esc 关闭");
	}
}

static void MenuToggle();
static void MenuProcessKey(int digit);
static void MenuAdjustSfxVol(int delta);
static void MenuAdjustXhairAlpha(int delta);
static void MenuAdjustUiBgAlpha(int delta);
static void MenuAdjustElimOffset(int delta);
static void MenuAdjustElimFont(int delta);
static void MenuAdjustClockFont(int delta);
static void MenuAdjustSpeedFont(int delta);
static void MenuAdjustTimerFont(int delta);
static void XhairTick();
static void XhairNotifyChanged();
static void XhairShutdown();

static DWORD g_menuIgnoreOpenUntil = 0;
static DWORD g_menuCmdStamp = 0;

static bool MenuVkIsMenuOnly(DWORD vk) {
	if (vk >= '0' && vk <= '9') return true;
	if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return true;
	if (vk == VK_ESCAPE || vk == VK_OEM_4) return true; // Esc / [
	if (vk == VK_OEM_MINUS || vk == VK_OEM_PLUS) return true;
	if (vk == VK_SUBTRACT || vk == VK_ADD) return true;
	// Letter shortcuts only on pages that use them — don't steal binds (e.g. C=crouch) elsewhere.
	if (g_menuPage == 3) {
		if (vk == 'R' || vk == 'O' || vk == 'C' || vk == 'M' || vk == 'T') return true;
		if (vk == 'X' || vk == 'V' || vk == 'B' || vk == 'F') return true;
	}
	if (g_menuPage == 4 && vk == 'R') return true;
	if (g_menuPage == 5 && (vk == 'R' || vk == 'W' || vk == 'A' || vk == 'S' || vk == 'D')) return true;
	if (g_menuPage == 12 && (vk == 'R' || vk == 'W' || vk == 'A' || vk == 'S' || vk == 'D')) return true;
	if ((g_menuPage == 13 || g_menuPage == 14 || g_menuPage == 15) && (vk == 'R' || vk == 'W' || vk == 'A' || vk == 'S' || vk == 'D')) return true;
	if (g_menuPage == 6 && (vk == 'A' || vk == 'S')) return true;
	return false;
}

static void MenuOnVk(DWORD vk) {
	if (!g_menuVisible || g_menuParked) return;
	if (vk == VK_ESCAPE) {
		MenuToggle();
		return;
	}
	if (vk == VK_OEM_4) {
		if (GetTickCount() < g_menuIgnoreOpenUntil) return;
		MenuToggle();
		return;
	}
	if (vk == VK_OEM_MINUS || vk == VK_SUBTRACT) {
		if (g_menuPage == 3) MenuAdjustXhairAlpha(-kSfxVolStep);
		else if (g_menuPage == 4) MenuAdjustUiBgAlpha(-kSfxVolStep);
		else if (g_menuPage == 5) MenuAdjustElimFont(-kElimFontStep);
		else if (g_menuPage == 12) MenuAdjustClockFont(-kElimFontStep);
		else if (g_menuPage == 13) MenuAdjustSpeedFont(-kElimFontStep);
		else if (g_menuPage == 14) MenuAdjustTimerFont(-kElimFontStep);
		else if (g_menuPage == 15) {
			g_teamHudFont = ClampAdd(g_teamHudFont, -2, 14, 28);
			g_teamFont = 0;
			g_teamFontSm = 0;
			SaveSettings();
		}
		else MenuAdjustSfxVol(-kSfxVolStep);
		return;
	}
	if (vk == VK_OEM_PLUS || vk == VK_ADD) {
		if (g_menuPage == 3) MenuAdjustXhairAlpha(+kSfxVolStep);
		else if (g_menuPage == 4) MenuAdjustUiBgAlpha(+kSfxVolStep);
		else if (g_menuPage == 5) MenuAdjustElimFont(+kElimFontStep);
		else if (g_menuPage == 12) MenuAdjustClockFont(+kElimFontStep);
		else if (g_menuPage == 13) MenuAdjustSpeedFont(+kElimFontStep);
		else if (g_menuPage == 14) MenuAdjustTimerFont(+kElimFontStep);
		else if (g_menuPage == 15) {
			g_teamHudFont = ClampAdd(g_teamHudFont, 2, 14, 28);
			g_teamFont = 0;
			g_teamFontSm = 0;
			SaveSettings();
		}
		else MenuAdjustSfxVol(+kSfxVolStep);
		return;
	}
	if (g_menuPage == 3 && (vk == 'C' || vk == 'c')) {
		g_xhairRingColor = CycleWrap(g_xhairRingColor, 0, kXhairColorCount - 1);
		g_xhairHudMode = -1;
		ClampXhair();
		SaveSettings();
		XhairTick();
		Log("menu: xhairRingColor=%d", g_xhairRingColor);
		EngClientCmd("play buttons/button14");
		
		return;
	}
	if (g_menuPage == 3 && (vk == 'M' || vk == 'm')) {
		g_xhairRingMode = (g_xhairRingMode == 0) ? 2 : 0;
		g_xhairHudMode = -1;
		SaveSettings();
		XhairTick();
		Log("menu: xhairRingMode=%d", g_xhairRingMode);
		EngClientCmd("play buttons/button14");
		
		return;
	}
	if (g_menuPage == 3 && (vk == 'T' || vk == 't')) {
		g_xhairRingAlpha = CycleStep(g_xhairRingAlpha, 10, 100, 10);
		g_xhairHudMode = -1;
		ClampXhair();
		SaveSettings();
		XhairTick();
		Log("menu: xhairRingAlpha=%d", g_xhairRingAlpha);
		EngClientCmd("play buttons/button14");
		
		return;
	}
	if (g_menuPage == 3 && (vk == 'X' || vk == 'x')) {
		g_optXhairTex = !g_optXhairTex;
		if (g_optXhairTex) {
			TexXhairRefreshList();
			TexXhairPickCurrent();
		}
		SaveSettings();
		Log("menu: xhairTex=%d mat=%s size=%d", g_optXhairTex ? 1 : 0,
			g_xhairTexMat[0] ? g_xhairTexMat : "-", g_xhairTexSize);
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 3 && (vk == 'V' || vk == 'v')) {
		TexXhairRememberCurrent();
		TexXhairRefreshList();
		if (g_texXhairCount <= 0) {
			Log("menu: xhairTex no materials (drop vmt into materials/skeeto/xhair/)");
			EngClientCmd("play buttons/button10");
			return;
		}
		int idx = TexXhairIndexOf(g_xhairTexMat);
		idx = (idx < 0) ? 0 : ((idx + 1) % g_texXhairCount);
		strncpy(g_xhairTexMat, g_texXhairList[idx], sizeof(g_xhairTexMat) - 1);
		g_xhairTexMat[sizeof(g_xhairTexMat) - 1] = 0;
		g_texXhairBound[0] = 0;
		TexXhairApplyCurrentOrDefault();
		ClampXhair();
		SaveSettings();
		Log("menu: xhairTexMat=%s (%d/%d) size=%d full=%d", g_xhairTexMat, idx + 1, g_texXhairCount,
			g_xhairTexSize, g_optXhairTexFull ? 1 : 0);
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 3 && (vk == 'F' || vk == 'f')) {
		g_optXhairTexFull = !g_optXhairTexFull;
		SaveSettings();
		Log("menu: xhairTexFull=%d", g_optXhairTexFull ? 1 : 0);
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 3 && (vk == 'B' || vk == 'b')) {
		g_xhairTexSize = CycleStep(g_xhairTexSize, kTexXhairSizeMin, kTexXhairSizeMax, kTexXhairSizeStep);
		ClampXhair();
		SaveSettings();
		Log("menu: xhairTexSize=%d", g_xhairTexSize);
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 3 && (vk == 'O' || vk == 'o')) {
		XhairT().outline = CycleWrap(XhairT().outline, 0, 4);
		ClampXhair();
		SaveSettings();
		XhairNotifyChanged();
		Log("menu: xhairOutline=%d", XhairT().outline);
		EngClientCmd("play buttons/button14");
		
		return;
	}
	if (g_menuPage == 4 && (vk == 'R' || vk == 'r')) {
		MenuStyleReset();
		ClampMenuStyle();
		SaveSettings();
		Log("menu: ui reset");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 5 && (vk == 'R' || vk == 'r')) {
		ElimResetCounts();
		Log("menu: elim reset");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 12 && (vk == 'R' || vk == 'r')) {
		ClockResetPos();
		ClampClockHud();
		SaveSettings();
		Log("menu: clock pos reset");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 13 && (vk == 'R' || vk == 'r')) {
		SpeedResetPos();
		ClampSpeedHud();
		SaveSettings();
		Log("menu: speed pos reset");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 14 && (vk == 'R' || vk == 'r')) {
		TimerResetPos();
		ClampTimerHud();
		SaveSettings();
		Log("menu: timer pos reset");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 15 && (vk == 'R' || vk == 'r')) {
		TeamHudResetPos();
		SaveSettings();
		Log("menu: teamhud pos reset");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 6 && (vk == 'A' || vk == 'a')) {
		ElimApplyPreset(false);
		ClampElimHud();
		SaveSettings();
		Log("menu: elimShow all");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 6 && (vk == 'S' || vk == 's')) {
		ElimApplyPreset(true);
		ClampElimHud();
		SaveSettings();
		Log("menu: elimShow compact");
		EngClientCmd("play buttons/button14");
		return;
	}
	if (g_menuPage == 3 && (vk == 'R' || vk == 'r')) {
		XhairResetCurrentStyle();
		ClampXhair();
		SaveSettings();
		XhairNotifyChanged();
		Log("menu: xhair reset style=%d", g_xhairStyle);
		EngClientCmd("play buttons/button14");
		
		return;
	}
	if (vk >= '0' && vk <= '9')
		MenuProcessKey((int)(vk - '0'));
	else if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
		MenuProcessKey((int)(vk - VK_NUMPAD0));
}

static void MenuShow(bool show) {
	g_menuParked = false;
	if (show)
		Log("menu engine HUD on");
}

static void MenuForceClose() {
	if (!g_menuVisible && !g_menuParked)
		return;
	g_menuVisible = false;
	g_menuParked = false;
	g_menuPage = 0;
	g_menuIgnoreOpenUntil = GetTickCount() + 300;
	MenuShow(false);
	Log("necola-dlc menu force-close (left game / main menu)");
}

static void MenuToggle() {
	g_menuCmdStamp = GetTickCount();
	if (g_menuVisible) {
		g_menuVisible = false;
		g_menuIgnoreOpenUntil = GetTickCount() + 300;
		Log("necola-dlc menu close page=%d", g_menuPage);
		if (g_engine && EngInGame())
			EngClientCmd("play buttons/button11");
		MenuShow(false);
		return;
	}
	if (!g_engine || !EngConnected() || !EngInGame()) return;
	if (!SkeetoFeaturesOn()) return;
	if (GetTickCount() < g_menuIgnoreOpenUntil) return;
	g_menuVisible = true;
	g_menuPage = 0;
	g_menuIgnoreOpenUntil = GetTickCount() + 200; // ignore still-held [
	Log("necola-dlc menu open page=%d", g_menuPage);
	EngClientCmd("play buttons/button9");
	MenuShow(true);
}

// Engine ConCommand callback (same ABI Necola uses for necola_menu).
static void* g_skeetoMenuCmd = nullptr;
static uintptr_t g_conCmdCtor = 0;

static int CmdArgC(const int* cmd) {
	if (!cmd) return 0;
	int n = cmd[0];
	if (n < 0 || n > 64) return 0;
	return n;
}
static const char* CmdArg(const int* cmd, int i) {
	if (!cmd || i < 0 || i >= CmdArgC(cmd)) return "";
	const char** argv = (const char**)((const char*)cmd + 8 + 512 + 512);
	return argv[i] ? argv[i] : "";
}
static bool CmdParseInt(const int* cmd, int* out) {
	if (!out || CmdArgC(cmd) < 2) return false;
	const char* a = CmdArg(cmd, 1);
	if (!a || !a[0]) return false;
	*out = atoi(a);
	return true;
}
static void ConEcho(const char* s) {
	if (!s || !s[0]) return;
	char cmd[320]{};
	snprintf(cmd, sizeof(cmd), "echo %s", s);
	EngClientCmd(cmd);
}

static void* RegisterEngineCommand(const char* name, void* cb, const char* help) {
	if (!g_cvar || !name || !cb) return nullptr;
	using FindCmdBaseFn = void*(__thiscall*)(void*, const char*);
	auto findFn = (FindCmdBaseFn)VGet(g_cvar, 10);
	if (findFn && IsExec((void*)findFn)) {
		void* existing = findFn(g_cvar, name);
		if (existing) {
			Log("cmd '%s' already registered %p", name, existing);
			return existing;
		}
	}
	if (!g_conCmdCtor) {
		const char* ctorPat =
			"55 8B EC 8B 45 0C 53 33 DB 56 8B F1 8B 4D 18 80 4E 20 02 89 46 18 8A 46";
		g_conCmdCtor = FindPat("client.dll", ctorPat);
	}
	if (!g_conCmdCtor || !IsExec((void*)g_conCmdCtor)) {
		Log("WARN: ConCommand ctor not found — '%s' skipped", name);
		return nullptr;
	}
	void* mem = malloc(96);
	if (!mem) return nullptr;
	memset(mem, 0, 96);
	using CtorFn = void(__thiscall*)(void*, const char*, void*, const char*, int, int);
	((CtorFn)g_conCmdCtor)(mem, name, cb, help, kFCvarClientCmdCanExecute, 0);
	Log("cmd '%s' registered ctor=%p cmd=%p", name, (void*)g_conCmdCtor, mem);
	return mem;
}

static void SkCmd_SkeetoMenu(int*) {
	if (GetTickCount() < g_menuIgnoreOpenUntil) return;
	g_menuCmdStamp = GetTickCount();
	MenuToggle();
}

static void SkCmd_Elim(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_optElim = v != 0;
		SaveSettings();
	}
	char b[80]{};
	snprintf(b, sizeof(b), "skeeto_elim %d  (0=hide 1=show)", g_optElim ? 1 : 0);
	ConEcho(b);
}

static void SkCmd_ElimMode(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_elimMode = v;
		ClampElimHud();
		SaveSettings();
	}
	char b[128]{};
	snprintf(b, sizeof(b),
		"skeeto_elim_mode %d  (0=chapter keep-wipe 1=persist 2=chapter+wipe reset)",
		g_elimMode);
	ConEcho(b);
}

static void SkCmd_ElimAlign(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_elimAlign = v;
		g_elimAbs = false;
		ClampElimHud();
		SaveSettings();
	}
	char b[80]{};
	snprintf(b, sizeof(b), "skeeto_elim_align %d  (0=TL 1=TR 2=BL 3=BR)", g_elimAlign);
	ConEcho(b);
}

static void SkCmd_ElimOffX(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_elimOffX = v;
		ClampElimHud();
		SaveSettings();
	}
	char b[64]{};
	snprintf(b, sizeof(b), "skeeto_elim_offset_x %d", g_elimOffX);
	ConEcho(b);
}

static void SkCmd_ElimOffY(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_elimOffY = v;
		ClampElimHud();
		SaveSettings();
	}
	char b[64]{};
	snprintf(b, sizeof(b), "skeeto_elim_offset_y %d", g_elimOffY);
	ConEcho(b);
}

static void SkCmd_ElimFont(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_elimFont = v;
		ClampElimHud();
		ElimInvalidateFont();
		SaveSettings();
	}
	char b[72]{};
	snprintf(b, sizeof(b), "skeeto_elim_font %d  (%d-%d)", g_elimFont, kElimFontMin, kElimFontMax);
	ConEcho(b);
}

static void SkCmd_ElimSi(int*) {
	char b[80]{};
	snprintf(b, sizeof(b), "Special Infected Kills: %d", g_elimSi.load(std::memory_order_relaxed));
	ConEcho(b);
}

static void SkCmd_ElimCi(int*) {
	char b[80]{};
	snprintf(b, sizeof(b), "Common Infected Kills: %d", g_elimCi.load(std::memory_order_relaxed));
	ConEcho(b);
}

static void SkCmd_ElimSiMelee(int*) {
	char b[80]{};
	snprintf(b, sizeof(b), "SI Melee Kills: %d", g_elimSiMelee.load(std::memory_order_relaxed));
	ConEcho(b);
}

static void SkCmd_ElimSkeet(int*) {
	char b[80]{};
	snprintf(b, sizeof(b), "Skeet Kills: %d", g_elimSkeet.load(std::memory_order_relaxed));
	ConEcho(b);
}

static void SkCmd_ElimMeleeSkeet(int*) {
	char b[80]{};
	snprintf(b, sizeof(b), "Melee Skeets: %d", g_elimMeleeSkeet.load(std::memory_order_relaxed));
	ConEcho(b);
}

static void SkCmd_ElimHs(int*) {
	const int tot = g_elimSi.load(std::memory_order_relaxed) + g_elimCi.load(std::memory_order_relaxed);
	const int hs = g_elimHs.load(std::memory_order_relaxed);
	char b[80]{};
	if (tot <= 0)
		snprintf(b, sizeof(b), "Headshot rate: --  (%d/%d)", hs, tot);
	else
		snprintf(b, sizeof(b), "Headshot rate: %d%%  (%d/%d)", (hs * 100 + tot / 2) / tot, hs, tot);
	ConEcho(b);
}

static void SkCmd_ElimLang(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_elimLang = v ? 1 : 0;
		ElimInvalidateFont();
		SaveSettings();
	}
	char b[80]{};
	snprintf(b, sizeof(b), "skeeto_elim_lang %d  (0=English 1=Chinese)", g_elimLang);
	ConEcho(b);
}

static void SkCmd_ElimCompact(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		ElimApplyPreset(v != 0);
		ClampElimHud();
		SaveSettings();
	}
	char b[88]{};
	snprintf(b, sizeof(b), "skeeto_elim_compact %d  (0=full 1=SI+CI only)", g_elimCompact);
	ConEcho(b);
}

static void SkCmd_ElimShow(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_elimShow = v;
		ClampElimHud();
		SaveSettings();
	}
	char b[96]{};
	snprintf(b, sizeof(b), "skeeto_elim_show %d  (bits: SI CI melee skeet melee_skeet hs_rate)", g_elimShow);
	ConEcho(b);
}

static void SkCmd_ElimReset(int*) {
	ElimResetCounts();
	ConEcho("skeeto_elim_reset: counts cleared");
}

static void SkCmd_CrashDialog(int* cmd) {
	int v = 0;
	if (CmdParseInt(cmd, &v)) {
		g_optCrashDialog = v != 0;
		SaveSettings();
	}
	char b[80]{};
	snprintf(b, sizeof(b), "skeeto_crash_dialog %d", g_optCrashDialog ? 1 : 0);
	ConEcho(b);
}

static void RegisterSkeetoMenuCommand() {
	if (g_skeetoMenuCmd) return;
	if (!g_cvar) {
		Log("skeeto_menu: skip (no ICvar)");
		return;
	}
	g_skeetoMenuCmd = RegisterEngineCommand("skeeto_menu", (void*)&SkCmd_SkeetoMenu,
		"Toggle Skeeto menu (bind <key> skeeto_menu)");
	RegisterEngineCommand("skeeto_elim", (void*)&SkCmd_Elim,
		"Show kill HUD. skeeto_elim 0/1");
	RegisterEngineCommand("skeeto_elim_mode", (void*)&SkCmd_ElimMode,
		"Kill HUD reset: 0=chapter keep-wipe 1=persist 2=chapter+wipe reset");
	RegisterEngineCommand("skeeto_elim_align", (void*)&SkCmd_ElimAlign,
		"Kill HUD corner: 0=top-left 1=top-right 2=bottom-left 3=bottom-right");
	RegisterEngineCommand("skeeto_elim_offset_x", (void*)&SkCmd_ElimOffX,
		"Kill HUD X offset from corner (pixels)");
	RegisterEngineCommand("skeeto_elim_offset_y", (void*)&SkCmd_ElimOffY,
		"Kill HUD Y offset from corner (pixels)");
	RegisterEngineCommand("skeeto_elim_font", (void*)&SkCmd_ElimFont,
		"Kill HUD font size (18-48)");
	RegisterEngineCommand("skeeto_elim_lang", (void*)&SkCmd_ElimLang,
		"Kill HUD language: 0=English 1=Chinese");
	RegisterEngineCommand("skeeto_elim_compact", (void*)&SkCmd_ElimCompact,
		"Kill HUD: 0=full 1=compact (SI+CI only)");
	RegisterEngineCommand("skeeto_elim_show", (void*)&SkCmd_ElimShow,
		"Kill HUD line bits: 1=SI 2=CI 4=melee 8=skeet 16=melee_skeet 32=hs_rate");
	RegisterEngineCommand("skeeto_elim_si", (void*)&SkCmd_ElimSi,
		"Print Special Infected Kills count");
	RegisterEngineCommand("skeeto_elim_ci", (void*)&SkCmd_ElimCi,
		"Print Common Infected Kills count");
	RegisterEngineCommand("skeeto_elim_si_melee", (void*)&SkCmd_ElimSiMelee,
		"Print SI Melee Kills count");
	RegisterEngineCommand("skeeto_elim_skeet", (void*)&SkCmd_ElimSkeet,
		"Print Skeet Kills count");
	RegisterEngineCommand("skeeto_elim_melee_skeet", (void*)&SkCmd_ElimMeleeSkeet,
		"Print Melee Skeets count");
	RegisterEngineCommand("skeeto_elim_hs", (void*)&SkCmd_ElimHs,
		"Print combined CI+SI Headshot rate");
	RegisterEngineCommand("skeeto_elim_reset", (void*)&SkCmd_ElimReset,
		"Clear kill HUD counts");
	RegisterEngineCommand("skeeto_crash_dialog", (void*)&SkCmd_CrashDialog,
		"Skeeto crash popup when the fault is in this DLL. 0/1");
}

static void EnsureDefaultMenuBind() {
	if (!g_needDefaultMenuBind || g_menuDefaultBindDone) return;
	if (!g_engine) return;
	EngClientCmd("bind [ \"skeeto_menu\"");
	g_menuDefaultBindDone = true;
	g_needDefaultMenuBind = false;
	SaveSettings();
	Log("menu: first-run bind [ \"skeeto_menu\"");
}

static void MenuProcessKey(int digit) {
	if (!g_menuVisible) return;

	if (g_menuPage == 0) {
		if (digit == 0) {
			MenuToggle();
			return;
		}
		if (digit == 1) {
			g_menuPage = 1;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 2) {
			g_menuPage = 3;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 3) {
			g_menuPage = 11;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 4) {
			g_menuPage = 9;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 5) {
			g_menuPage = 8;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 6) {
			g_menuPage = 4;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 7) {
			g_menuPage = 7;
			EngClientCmd("play buttons/button14");
			return;
		}
		return;
	}

	if (g_menuPage == 9) {
		if (digit == 0) {
			g_menuPage = 0;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optClientThrowLand = !g_optClientThrowLand;
			if (!g_optClientThrowLand)
				g_throwLandReset.store(true, std::memory_order_relaxed);
			Log("menu: clientThrowLand=%d", g_optClientThrowLand ? 1 : 0);
			SaveSettings();
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 2) {
			g_optClientNoCorpseSi = !g_optClientNoCorpseSi;
			Log("menu: clientNoCorpseSi=%d", g_optClientNoCorpseSi ? 1 : 0);
			ClientUxApplyNoCorpseCvars(true);
			if (!g_optClientNoCorpseSi && !g_optClientNoCorpseCi)
				ClientUxClearNoCorpseTracks();
			SaveSettings();
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 3) {
			g_optClientNoCorpseCi = !g_optClientNoCorpseCi;
			Log("menu: clientNoCorpseCi=%d", g_optClientNoCorpseCi ? 1 : 0);
			ClientUxApplyNoCorpseCvars(true);
			if (!g_optClientNoCorpseSi && !g_optClientNoCorpseCi)
				ClientUxClearNoCorpseTracks();
			SaveSettings();
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 4) {
			g_optClientInfectedHp = !g_optClientInfectedHp;
			if (!g_optClientInfectedHp)
				LocalPlayClearHpTrack();
			Log("menu: clientInfectedHp=%d", g_optClientInfectedHp ? 1 : 0);
			SaveSettings();
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 5) {
			g_optClientDmgNum = !g_optClientDmgNum;
			if (!g_optClientDmgNum)
				ClientUxDmgReset();
			Log("menu: clientDmgNum=%d", g_optClientDmgNum ? 1 : 0);
			SaveSettings();
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 6) {
			g_optClientDirectorHud = !g_optClientDirectorHud;
			Log("menu: clientDirectorHud=%d", g_optClientDirectorHud ? 1 : 0);
			ClientUxApplyDirectorHud(true);
			SaveSettings();
			EngClientCmd("play buttons/button14");
			return;
		}
		return;
	}

	if (g_menuPage == 7) {
		if (digit == 0) {
			g_menuPage = 0;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_menuPage = 2;
			EngClientCmd("play buttons/button14");
			return;
		}
		return;
	}

	if (g_menuPage == 8) {
		if (digit == 0) {
			g_menuPage = 0;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (!ListenDetectHost()) {
			Log("menu: local options locked (not listen host)");
			EngClientCmd("play buttons/button10");
			return;
		}
		if (digit == 1) {
			g_optLocalTick = ListenCycleTick(g_optLocalTick);
			Log("menu: localTick=%d (map change)", g_optLocalTick);
		} else if (digit == 2) {
			g_optLocalNb = CycleWrap(g_optLocalNb, 0, 3);
			Log("menu: localNb=%d", g_optLocalNb);
		} else if (digit == 3) {
			g_optLocalAllow0Lerp = !g_optLocalAllow0Lerp;
			Log("menu: localAllow0Lerp=%d", g_optLocalAllow0Lerp ? 1 : 0);
		} else if (digit == 4) {
			g_optLocalLerp = ListenCycleLerp(g_optLocalLerp);
			Log("menu: localLerp=%d", g_optLocalLerp);
		} else if (digit == 5) {
			g_optLocalIdleSolo = !g_optLocalIdleSolo;
			Log("menu: localIdleSolo=%d", g_optLocalIdleSolo ? 1 : 0);
		} else if (digit == 6) {
			g_optLocalIdleNoDelay = !g_optLocalIdleNoDelay;
			Log("menu: localIdleNoDelay=%d", g_optLocalIdleNoDelay ? 1 : 0);
		} else if (digit == 7) {
			g_optLocalCharChange = !g_optLocalCharChange;
			Log("menu: localCharChange=%d", g_optLocalCharChange ? 1 : 0);
			if (g_optLocalCharChange && EngInGame())
				LocalPlayPrecacheAllSurvivors("char-menu");
			if (!g_optLocalCharChange) {
				g_charWantProp = -1;
				g_charWantModel[0] = 0;
				g_charWantName[0] = 0;
				g_charReapplyAt = 0;
				g_charReapplyAt2 = 0;
			}
		} else if (digit == 8) {
			if (!g_optLocalCharChange) {
				Log("menu: char change disabled — enable [7] first");
				EngClientCmd("play buttons/button10");
				return;
			}
			g_menuPage = 10;
			LocalPlayPrecacheAllSurvivors("char-menu");
			EngClientCmd("play buttons/button14");
			return;
		} else if (digit == 9) {
			g_optLocalAa = ListenCycleAa(g_optLocalAa);
			Log("menu: localAa=%d", g_optLocalAa);
		} else
			return;
		ClampListenOpts();
		SaveSettings();
		ListenMarkDirty();
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 10) {
		if (digit == 0) {
			g_menuPage = 8;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (!ListenDetectHost() || !g_optLocalCharChange) {
			EngClientCmd("play buttons/button10");
			return;
		}
		struct CharPick { int prop; const char* model; const wchar_t* name; };
		static const CharPick kChars[] = {
			{ 0, "models/survivors/survivor_gambler.mdl", L"Nick" },
			{ 1, "models/survivors/survivor_producer.mdl", L"Rochelle" },
			{ 2, "models/survivors/survivor_coach.mdl", L"Coach" },
			{ 3, "models/survivors/survivor_mechanic.mdl", L"Ellis" },
			{ 4, "models/survivors/survivor_namvet.mdl", L"Bill" },
			{ 5, "models/survivors/survivor_teenangst.mdl", L"Zoey" },
			{ 6, "models/survivors/survivor_biker.mdl", L"Francis" },
			{ 7, "models/survivors/survivor_manager.mdl", L"Louis" },
		};
		if (digit >= 1 && digit <= 8) {
			const CharPick& c = kChars[digit - 1];
			LocalPlayChangeSurvivor(c.prop, c.model, c.name);
			EngClientCmd("play buttons/button14");
		}
		return;
	}

	if (g_menuPage == 2) {
		if (digit == 0) {
			g_menuPage = 7;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optPtDispWarm = !g_optPtDispWarm;
			SaveSettings();
			Log("menu: ptDispWarm=%d (next map load)", g_optPtDispWarm ? 1 : 0);
			EngClientCmd("play buttons/button14");
			
		}
		return;
	}

	if (g_menuPage == 3) {
		if (digit == 0) {
			g_menuPage = 0;
			EngClientCmd("play buttons/button11");
			
			return;
		}
		if (digit == 1) {
			g_optXhair = !g_optXhair;
			g_xhairHudMode = -1;
			Log("menu: xhair=%d", g_optXhair ? 1 : 0);
		} else if (digit == 2) {
			g_optXhairRing = !g_optXhairRing;
			g_xhairHudMode = -1;
			Log("menu: xhairRing=%d", g_optXhairRing ? 1 : 0);
		} else if (digit == 3) {
			g_xhairStyle = CycleWrap(g_xhairStyle, 0, kXhairStyleCount - 1);
			Log("menu: xhairStyle=%d", g_xhairStyle);
		} else if (digit == 4) {
			XhairT().color = CycleWrap(XhairT().color, 0, kXhairColorCount - 1);
			Log("menu: xhairColor=%d", XhairT().color);
		} else if (digit == 5) {
			XhairT().size = CycleStep(XhairT().size, 50, 200, 10);
			Log("menu: xhairSize=%d", XhairT().size);
		} else if (digit == 6) {
			XhairT().length = CycleWrap(XhairT().length, 2, 24);
			Log("menu: xhairLength=%d", XhairT().length);
		} else if (digit == 7) {
			XhairT().gap = CycleWrap(XhairT().gap, 0, 16);
			Log("menu: xhairGap=%d", XhairT().gap);
		} else if (digit == 8) {
			XhairT().thick = CycleWrap(XhairT().thick, 1, 8);
			Log("menu: xhairThick=%d", XhairT().thick);
		} else if (digit == 9) {
			XhairT().dot = CycleWrap(XhairT().dot, 0, 12);
			Log("menu: xhairDot=%d", XhairT().dot);
		} else {
			return;
		}
		ClampXhair();
		SaveSettings();
		XhairNotifyChanged();
		XhairTick();
		EngClientCmd("play buttons/button14");
		
		return;
	}

	if (g_menuPage == 4) {
		if (digit == 0) {
			g_menuPage = 0;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_uiSizePct = CycleStep(g_uiSizePct, 50, 100, 10);
		} else if (digit == 2) {
			g_uiAlignX = CycleWrap(g_uiAlignX, 0, 2);
		} else if (digit == 3) {
			g_uiAlignY = CycleWrap(g_uiAlignY, 0, 2);
		} else if (digit == 4) {
			g_uiBgAlpha = CycleStep(g_uiBgAlpha, 30, 100, 10);
		} else if (digit == 5) {
			g_uiBg = CycleWrap(g_uiBg, 0, kUiBgCount - 1);
		} else if (digit == 6) {
			g_uiText = CycleWrap(g_uiText, 0, kUiTextCount - 1);
		} else if (digit == 7) {
			g_uiTitle = CycleWrap(g_uiTitle, 0, kUiTitleCount - 1);
		} else if (digit == 8) {
			g_uiFont = CycleStep(g_uiFont, 14, 28, 2);
			MenuInvalidateFont();
		} else if (digit == 9) {
			g_uiOutline = g_uiOutline ? 0 : 1;
			MenuInvalidateFont();
		} else {
			return;
		}
		ClampMenuStyle();
		SaveSettings();
		Log("menu: ui size=%d ax=%d ay=%d a=%d bg=%d text=%d title=%d font=%d ol=%d",
			g_uiSizePct, g_uiAlignX, g_uiAlignY, g_uiBgAlpha, g_uiBg, g_uiText, g_uiTitle,
			g_uiFont, g_uiOutline);
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 5) {
		if (digit == 0) {
			g_menuPage = 11;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optElim = !g_optElim;
		} else if (digit == 2) {
			ElimCycleMode();
		} else if (digit == 3) {
			g_elimFont = CycleStep(g_elimFont, kElimFontMin, kElimFontMax, kElimFontStep);
			ElimInvalidateFont();
		} else if (digit == 4) {
			g_elimLang = g_elimLang ? 0 : 1;
			ElimInvalidateFont();
		} else if (digit == 5) {
			ElimApplyPreset(g_elimShow == kElimShowAll);
		} else if (digit == 6) {
			g_menuPage = 6;
			EngClientCmd("play buttons/button14");
			return;
		} else {
			return;
		}
		ClampElimHud();
		SaveSettings();
		Log("menu: elim=%d mode=%d align=%d ox=%d oy=%d font=%d lang=%d show=%d",
			g_optElim ? 1 : 0, g_elimMode, g_elimAlign, g_elimOffX, g_elimOffY, g_elimFont,
			g_elimLang, g_elimShow);
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 6) {
		if (digit == 0) {
			g_menuPage = 5;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit >= 1 && digit <= kElimLineCount) {
			ElimSetLine(digit - 1, !ElimLineOn(digit - 1));
		} else {
			return;
		}
		ClampElimHud();
		SaveSettings();
		Log("menu: elimShow=%d", g_elimShow);
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 11) {
		if (digit == 0) {
			g_menuPage = 0;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_menuPage = 5;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 2) {
			g_menuPage = 12;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 3) {
			g_menuPage = 13;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 4) {
			g_menuPage = 14;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 5) {
			g_menuPage = 15;
			EngClientCmd("play buttons/button14");
			return;
		}
		if (digit == 6) {
			g_menuPage = 16;
			EngClientCmd("play buttons/button14");
			return;
		}
		return;
	}

	if (g_menuPage == 12) {
		if (digit == 0) {
			g_menuPage = 11;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optClock = !g_optClock;
		} else if (digit == 2) {
			g_clockFont = CycleStep(g_clockFont, kElimFontMin, kElimFontMax, kElimFontStep);
			ClockInvalidateFont();
		} else {
			return;
		}
		ClampClockHud();
		SaveSettings();
		Log("menu: clock=%d align=%d ox=%d oy=%d font=%d",
			g_optClock ? 1 : 0, g_clockAlign, g_clockOffX, g_clockOffY, g_clockFont);
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 13) {
		if (digit == 0) {
			g_menuPage = 11;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optSpeed = !g_optSpeed;
		} else if (digit == 2) {
			g_speedFont = CycleStep(g_speedFont, kElimFontMin, kElimFontMax, kElimFontStep);
			SpeedInvalidateFont();
		} else {
			return;
		}
		ClampSpeedHud();
		SaveSettings();
		Log("menu: speed=%d align=%d ox=%d oy=%d font=%d",
			g_optSpeed ? 1 : 0, g_speedAlign, g_speedOffX, g_speedOffY, g_speedFont);
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 14) {
		if (digit == 0) {
			g_menuPage = 11;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optTimer = !g_optTimer;
		} else if (digit == 2) {
			g_timerFont = CycleStep(g_timerFont, kElimFontMin, kElimFontMax, kElimFontStep);
			TimerInvalidateFont();
		} else {
			return;
		}
		ClampTimerHud();
		SaveSettings();
		Log("menu: timer=%d align=%d ox=%d oy=%d font=%d",
			g_optTimer ? 1 : 0, g_timerAlign, g_timerOffX, g_timerOffY, g_timerFont);
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 15) {
		if (digit == 0) {
			g_menuPage = 11;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optTeamHud = !g_optTeamHud;
		} else if (digit == 2) {
			g_teamHudSel = CycleWrap(g_teamHudSel, 0, kTeamHudMax - 1);
		} else if (digit == 3) {
			g_teamHudFont = CycleStep(g_teamHudFont, 14, 28, 2);
			g_teamFont = 0;
			g_teamFontSm = 0;
		} else {
			return;
		}
		if (g_teamHudSel < 0) g_teamHudSel = 0;
		if (g_teamHudSel >= kTeamHudMax) g_teamHudSel = kTeamHudMax - 1;
		SaveSettings();
		Log("menu: teamhud=%d sel=%d font=%d", g_optTeamHud ? 1 : 0, g_teamHudSel, g_teamHudFont);
		EngClientCmd("play buttons/button14");
		return;
	}

	if (g_menuPage == 16) {
		if (digit == 0) {
			g_menuPage = 11;
			EngClientCmd("play buttons/button11");
			return;
		}
		if (digit == 1) {
			g_optHudHideTeam = !g_optHudHideTeam;
			if (!g_optHudHideTeam)
				HudHideRestore();
		} else if (digit == 2) {
			g_optHudHideWep = !g_optHudHideWep;
		} else if (digit == 3) {
			g_optHudHidePickup = !g_optHudHidePickup;
		} else if (digit == 4) {
			const bool allOn = g_optHudHideTeam && g_optHudHideWep && g_optHudHidePickup;
			const bool next = !allOn;
			g_optHudHideTeam = next;
			g_optHudHideWep = next;
			g_optHudHidePickup = next;
			if (!next)
				HudHideRestore();
		} else {
			return;
		}
		HudHideSyncMaster();
		SaveSettings();
		Log("menu: hudHide team=%d wep=%d pickup=%d",
			g_optHudHideTeam ? 1 : 0, g_optHudHideWep ? 1 : 0, g_optHudHidePickup ? 1 : 0);
		EngClientCmd("play buttons/button14");
		return;
	}

	// Hit-feedback submenu
	// [1]总音效 [2]总画面 [3]Hit [4]普感包 [5]特感包 [6]特感画面 [7]特感音效 [8]特殊击杀反馈 [9]友伤
	if (digit == 0) {
		g_menuPage = 0;
		EngClientCmd("play buttons/button11");
		
		return;
	}
	if (digit == 1) {
		g_optSound = !g_optSound;
		Log("menu: sound=%d", g_optSound ? 1 : 0);
	} else if (digit == 2) {
		g_optIcon = !g_optIcon;
		Log("menu: icon=%d", g_optIcon ? 1 : 0);
	} else if (digit == 3) {
		if (g_optHitMode == 1) g_optHitMode = 2;
		else if (g_optHitMode == 2) g_optHitMode = 0;
		else g_optHitMode = 1;
		Log("menu: hitMode=%d", g_optHitMode);
	} else if (digit == 4) {
		const char* id = DlcCycleChannel("ci");
		Log("menu: ciStyle=%s", id);
	} else if (digit == 5) {
		const char* id = DlcCycleChannel("si");
		g_siStreak = 0;
		Log("menu: siStyle=%s", id);
	} else if (digit == 6) {
		g_optSiVisual = !g_optSiVisual;
		Log("menu: siVisual=%d", g_optSiVisual ? 1 : 0);
	} else if (digit == 7) {
		g_optSiSound = !g_optSiSound;
		Log("menu: siSound=%d", g_optSiSound ? 1 : 0);
	} else if (digit == 8) {
		g_optKillFx = !g_optKillFx;
		Log("menu: killFx=%d", g_optKillFx ? 1 : 0);
	} else if (digit == 9) {
		g_optFf = !g_optFf;
		Log("menu: ff=%d", g_optFf ? 1 : 0);
	} else {
		return;
	}
	SaveSettings();
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustSfxVol(int delta) {
	if (!g_menuVisible || g_menuPage != 1) return;
	g_optSfxVol += delta;
	ClampSfxVol();
	SaveSettings();
	Log("menu: sfxVol=%d", g_optSfxVol);
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustXhairAlpha(int delta) {
	if (!g_menuVisible || g_menuPage != 3) return;
	XhairT().alpha += delta;
	ClampXhair();
	SaveSettings();
	XhairNotifyChanged();
	Log("menu: xhairAlpha=%d", XhairT().alpha);
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustUiBgAlpha(int delta) {
	if (!g_menuVisible || g_menuPage != 4) return;
	g_uiBgAlpha += delta;
	ClampMenuStyle();
	SaveSettings();
	Log("menu: uiBgAlpha=%d", g_uiBgAlpha);
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustElimOffset(int delta) {
	if (!g_menuVisible || g_menuPage != 5) return;
	g_elimOffX += delta;
	ClampElimHud();
	SaveSettings();
	Log("menu: elimOffX=%d", g_elimOffX);
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustElimFont(int delta) {
	if (!g_menuVisible || g_menuPage != 5) return;
	g_elimFont += delta;
	ClampElimHud();
	ElimInvalidateFont();
	SaveSettings();
	Log("menu: elimFont=%d", g_elimFont);
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustClockFont(int delta) {
	if (!g_menuVisible || g_menuPage != 12) return;
	g_clockFont += delta;
	ClampClockHud();
	ClockInvalidateFont();
	SaveSettings();
	Log("menu: clockFont=%d", g_clockFont);
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustSpeedFont(int delta) {
	if (!g_menuVisible || g_menuPage != 13) return;
	g_speedFont += delta;
	ClampSpeedHud();
	SpeedInvalidateFont();
	SaveSettings();
	Log("menu: speedFont=%d", g_speedFont);
	EngClientCmd("play buttons/button14");
}

static void MenuAdjustTimerFont(int delta) {
	if (!g_menuVisible || g_menuPage != 14) return;
	g_timerFont += delta;
	ClampTimerHud();
	TimerInvalidateFont();
	SaveSettings();
	Log("menu: timerFont=%d", g_timerFont);
	EngClientCmd("play buttons/button14");
}

static bool MenuKeyEdge(int vk, bool* prev) {
	const bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
	const bool fire = down && !*prev;
	*prev = down;
	return fire;
}

// Menu keys: NCL eats 0-9 in IN_KeyEvent while its menu is visible (return 0).
// We do the same for our menu via a one-shot VMT chain. Paint polling is only a
// fallback for `[` if the engine bind did not fire. Do not poll digits here
// when the hook is installed — that would double-fire and also leak into the game.
static void MenuPollPaintKeys() {
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!g_engine || !EngInGame() || !SkeetoFeaturesOn()) return;
	if (!g_gameHwnd)
		g_gameHwnd = FindGameWindow();
	HWND fg = GetForegroundWindow();
	if (!fg || !g_gameHwnd || fg != g_gameHwnd)
		return;

	static bool pOem4 = false, pEsc = false;
	static bool pDigit[10]{}, pNum[10]{};
	static bool pMinus = false, pPlus = false, pSub = false, pAdd = false;
	static bool pLetter[26]{};

	if (g_inKeyEatInstalled) {
		if (!g_menuVisible && MenuKeyEdge(VK_OEM_4, &pOem4)
			&& GetTickCount() >= g_menuIgnoreOpenUntil
			&& GetTickCount() - g_menuCmdStamp >= 120)
			MenuToggle();
		return;
	}

	if (MenuKeyEdge(VK_OEM_4, &pOem4) && GetTickCount() >= g_menuIgnoreOpenUntil) {
		if (GetTickCount() - g_menuCmdStamp >= 120)
			MenuToggle();
	}
	if (!g_menuVisible || g_menuParked)
		return;

	if (MenuKeyEdge(VK_ESCAPE, &pEsc)) MenuOnVk(VK_ESCAPE);
	if (MenuKeyEdge(VK_OEM_MINUS, &pMinus)) MenuOnVk(VK_OEM_MINUS);
	if (MenuKeyEdge(VK_OEM_PLUS, &pPlus)) MenuOnVk(VK_OEM_PLUS);
	if (MenuKeyEdge(VK_SUBTRACT, &pSub)) MenuOnVk(VK_SUBTRACT);
	if (MenuKeyEdge(VK_ADD, &pAdd)) MenuOnVk(VK_ADD);
	for (int d = 0; d < 10; ++d) {
		if (MenuKeyEdge('0' + d, &pDigit[d])) MenuOnVk((DWORD)('0' + d));
		if (MenuKeyEdge(VK_NUMPAD0 + d, &pNum[d])) MenuOnVk((DWORD)(VK_NUMPAD0 + d));
	}
	static const char kLetters[] = "ROTC MXVBFAS";
	for (int i = 0; kLetters[i]; ++i) {
		if (kLetters[i] == ' ') continue;
		const int vk = kLetters[i];
		if (vk >= 'A' && vk <= 'Z' && MenuKeyEdge(vk, &pLetter[vk - 'A']))
			MenuOnVk((DWORD)vk);
	}
}

static void MenuTickElimNudge() {
	static DWORD start = 0;
	static DWORD last = 0;
	const bool elim = g_menuPage == 5;
	const bool clock = g_menuPage == 12;
	const bool speed = g_menuPage == 13;
	const bool timer = g_menuPage == 14;
	const bool team = g_menuPage == 15;
	if (!g_menuVisible || g_menuParked || (!elim && !clock && !speed && !timer && !team)) {
		start = 0;
		return;
	}
	int dx = 0, dy = 0;
	if (GetAsyncKeyState('W') & 0x8000) dy -= 1;
	if (GetAsyncKeyState('S') & 0x8000) dy += 1;
	if (GetAsyncKeyState('A') & 0x8000) dx -= 1;
	if (GetAsyncKeyState('D') & 0x8000) dx += 1;
	if (!dx && !dy) {
		start = 0;
		return;
	}
	const int step = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 32 : 8;
	const DWORD now = GetTickCount();
	auto apply = [&]() {
		if (clock) ClockNudgeScreen(dx * step, dy * step);
		else if (speed) SpeedNudgeScreen(dx * step, dy * step);
		else if (timer) TimerNudgeScreen(dx * step, dy * step);
		else if (team) TeamHudNudgeScreen(dx * step, dy * step);
		else ElimNudgeScreen(dx * step, dy * step);
	};
	if (!start) {
		start = now;
		last = now;
		apply();
		return;
	}
	if (now - start < 220) return;
	if (now - last < 40) return;
	last = now;
	apply();
}

static void MenuTick() {
	if (!g_run.load(std::memory_order_relaxed)) {
		MenuForceClose();
		return;
	}
	if (!g_engine || !EngInGame()) {
		MenuForceClose();
		return;
	}
	if (!SkeetoFeaturesOn()) {
		MenuForceClose();
		return;
	}
	if (!g_menuVisible)
		return;

	g_gameHwnd = FindGameWindow();
	HWND fg = GetForegroundWindow();
	// Overlay hwnd is not "the game". Treating it as in-game focus left keys on the UI thread.
	const bool otherApp = !fg || !g_gameHwnd || fg != g_gameHwnd;
	if (otherApp) {
		if (!g_menuParked) {
			g_menuParked = true;
			Log("menu parked (other app focused)");
		}
	} else if (g_menuParked) {
		g_menuParked = false;
		Log("menu unparked");
	}
	if (g_menuVisible && !g_menuParked)
		MenuTickElimNudge();
}

// =============================================================================
// Static crosshair: GDI+ layered window at window-client pixels (DWM), not ISurface.
// Create/update on the game thread (no owner, no UI thread). Park offscreen without
// Show/Hide — Hide/Show + cross-thread owner was the input-queue hazard.
// =============================================================================
static const wchar_t* kXhairClass = L"SkeetoXhairWnd";
static constexpr UINT WM_XHAIR_SYNC = WM_APP + 21;
static constexpr UINT WM_XHAIR_DESTROY = WM_APP + 22;
static bool g_xhairParked = false;
static int g_xhairWin = 64;
static int g_xhairDrawH = 0;
static ULONG_PTR g_gdiplusToken = 0;
static bool g_gdiplusOk = false;
static HANDLE g_xhairUiTh = nullptr;
static DWORD g_xhairUiTid = 0;
static HANDLE g_xhairUiReady = nullptr;
static std::atomic_bool g_xhairUiRun{ false };
static std::atomic_bool g_xhairWantShown{ false };
static std::atomic_bool g_xhairRedraw{ false };
static bool g_xhairOsShown = false;       // UI thread: first SHOWNOACTIVATE done
static bool g_xhairParkedOffscreen = false;
static int g_xhairLastWin = 0;
static int g_xhairLastX = 0x7fffffff;
static int g_xhairLastY = 0x7fffffff;
static int g_xhairLastSig = 0;

static void XhairGdiplusInit() {
	if (g_gdiplusOk) return;
	Gdiplus::GdiplusStartupInput in;
	g_gdiplusOk = (Gdiplus::GdiplusStartup(&g_gdiplusToken, &in, nullptr) == Gdiplus::Ok);
	Log("xhair gdiplus=%d", g_gdiplusOk ? 1 : 0);
}

static float XhairHudMul() {
	int ch = g_xhairDrawH;
	if (ch < 80) {
		RECT crc{};
		ch = 1080;
		if (g_gameHwnd && GetClientRect(g_gameHwnd, &crc)) {
			const int h = crc.bottom - crc.top;
			if (h >= 80) ch = h;
		}
	}
	return ((float)ch / 1080.f) * ((float)XhairT().size / 100.f);
}

static float XhairScaleF(int logical, float minv) {
	float x = (float)logical * XhairHudMul();
	if (x < minv) x = minv;
	return x;
}

static int XhairWantWin() {
	const XhairTune& t = XhairT();
	const float length = XhairScaleF(t.length, 1.f);
	const float gap = (t.gap <= 0) ? 0.f : XhairScaleF(t.gap, 0.f);
	const float thick = XhairScaleF(t.thick, 1.f);
	const float outline = (t.outline <= 0) ? 0.f : XhairScaleF(t.outline, 1.f);
	const float dot = (t.dot <= 0) ? 0.f : XhairScaleF(t.dot, 1.f);
	// Circle/diamond radius is length+gap+thick; keep padding for AA + outline.
	float reach = length + gap + thick * 2.f + outline * 2.f + dot + 16.f;
	if (reach < 28.f) reach = 28.f;
	int win = (int)ceilf(reach * 2.f + 8.f);
	if (win > 512) win = 512;
	if (win < 32) win = 32;
	return win;
}

static Gdiplus::Color XhairArgb(COLORREF c, BYTE a) {
	return Gdiplus::Color(a, GetRValue(c), GetGValue(c), GetBValue(c));
}

static void XhairStrokeLine(Gdiplus::Graphics& g, float x1, float y1, float x2, float y2,
	float thick, const Gdiplus::Color& col) {
	if (thick <= 0.f) return;
	Gdiplus::Pen pen(col, thick);
	pen.SetLineCap(Gdiplus::LineCapFlat, Gdiplus::LineCapFlat, Gdiplus::DashCapFlat);
	pen.SetLineJoin(Gdiplus::LineJoinRound);
	g.DrawLine(&pen, x1, y1, x2, y2);
}

static void XhairArmAA(Gdiplus::Graphics& g, float cx, float cy, float dirX, float dirY,
	float length, float thick, float gap, const Gdiplus::Color& col, float outline,
	const Gdiplus::Color& oc) {
	if (length <= 0.f || thick <= 0.f) return;
	float x1, y1, x2, y2;
	if (dirY == 0.f) {
		y1 = y2 = cy;
		if (dirX > 0.f) { x1 = cx + gap; x2 = cx + gap + length; }
		else { x1 = cx - gap; x2 = cx - gap - length; }
	} else {
		x1 = x2 = cx;
		if (dirY > 0.f) { y1 = cy + gap; y2 = cy + gap + length; }
		else { y1 = cy - gap; y2 = cy - gap - length; }
	}
	if (outline > 0.f)
		XhairStrokeLine(g, x1, y1, x2, y2, thick + outline * 2.f, oc);
	XhairStrokeLine(g, x1, y1, x2, y2, thick, col);
}

static void XhairDotAA(Gdiplus::Graphics& g, float cx, float cy, float sz,
	const Gdiplus::Color& col, float outline, const Gdiplus::Color& oc) {
	if (sz <= 0.f) return;
	const float x = cx - sz * 0.5f;
	const float y = cy - sz * 0.5f;
	if (outline > 0.f) {
		Gdiplus::SolidBrush ob(oc);
		g.FillEllipse(&ob, x - outline, y - outline, sz + outline * 2.f, sz + outline * 2.f);
	}
	Gdiplus::SolidBrush br(col);
	g.FillEllipse(&br, x, y, sz, sz);
}

static void XhairCircleAA(Gdiplus::Graphics& g, float cx, float cy, float radius, float thick,
	const Gdiplus::Color& col, float outline, const Gdiplus::Color& oc) {
	if (radius <= 0.f || thick <= 0.f) return;
	const float x = cx - radius;
	const float y = cy - radius;
	const float d = radius * 2.f;
	if (outline > 0.f) {
		Gdiplus::Pen op(oc, thick + outline * 2.f);
		g.DrawEllipse(&op, x, y, d, d);
	}
	Gdiplus::Pen pen(col, thick);
	g.DrawEllipse(&pen, x, y, d, d);
}

static void XhairRectAA(Gdiplus::Graphics& g, float l, float t, float w, float h, float thick,
	const Gdiplus::Color& col, float outline, const Gdiplus::Color& oc) {
	if (thick <= 0.f) return;
	if (outline > 0.f) {
		Gdiplus::Pen op(oc, thick + outline * 2.f);
		g.DrawRectangle(&op, l, t, w, h);
	}
	Gdiplus::Pen pen(col, thick);
	g.DrawRectangle(&pen, l, t, w, h);
}

static void XhairDrawAll(Gdiplus::Graphics& g, float cx, float cy) {
	const XhairTune& t = XhairT();
	int a = (t.alpha * 255) / 100;
	if (a < 16) a = 16;
	if (a > 255) a = 255;
	const COLORREF cr = kXhairColorRgb[(t.color >= 0 && t.color < kXhairColorCount) ? t.color : 2];
	const Gdiplus::Color col = XhairArgb(cr, (BYTE)a);
	const Gdiplus::Color oc = XhairArgb(RGB(0, 0, 0), (BYTE)a);
	const float length = XhairScaleF(t.length, 1.f);
	const float gap = (t.gap <= 0) ? 0.f : XhairScaleF(t.gap, 0.f);
	const float thick = XhairScaleF(t.thick, 1.f);
	const float outline = (t.outline <= 0) ? 0.f : XhairScaleF(t.outline, 1.f);
	float dot = (t.dot <= 0) ? 0.f : XhairScaleF(t.dot, 1.f);
	const int style = g_xhairStyle;

	auto cross = [&]() {
		XhairArmAA(g, cx, cy, 1.f, 0.f, length, thick, gap, col, outline, oc);
		XhairArmAA(g, cx, cy, -1.f, 0.f, length, thick, gap, col, outline, oc);
		XhairArmAA(g, cx, cy, 0.f, 1.f, length, thick, gap, col, outline, oc);
		XhairArmAA(g, cx, cy, 0.f, -1.f, length, thick, gap, col, outline, oc);
	};
	auto tee = [&]() {
		XhairArmAA(g, cx, cy, 1.f, 0.f, length, thick, gap, col, outline, oc);
		XhairArmAA(g, cx, cy, -1.f, 0.f, length, thick, gap, col, outline, oc);
		XhairArmAA(g, cx, cy, 0.f, 1.f, length, thick, gap, col, outline, oc);
	};
	auto maybeDot = [&]() {
		if (dot > 0.f)
			XhairDotAA(g, cx, cy, dot, col, outline, oc);
	};

	switch (style) {
	case 0: // 十字
		cross();
		maybeDot();
		break;
	case 1: // 十字+点
		cross();
		maybeDot();
		break;
	case 2: // 点
		if (dot <= 0.f) dot = XhairScaleF(4, 1.f);
		XhairDotAA(g, cx, cy, dot, col, outline, oc);
		break;
	case 3: // 圆环
		XhairCircleAA(g, cx, cy, length + gap, thick, col, outline, oc);
		maybeDot();
		break;
	case 4: // 圆环+点
		XhairCircleAA(g, cx, cy, length + gap, thick, col, outline, oc);
		maybeDot();
		break;
	case 5: // 圆环+十字
		XhairCircleAA(g, cx, cy, length + gap + thick, thick, col, outline, oc);
		cross();
		maybeDot();
		break;
	case 6: // T字
		tee();
		maybeDot();
		break;
	case 7: // T字+点
		tee();
		maybeDot();
		break;
	case 8: { // 方框
		const float r = length + gap;
		XhairRectAA(g, cx - r, cy - r, r * 2.f, r * 2.f, thick, col, outline, oc);
		maybeDot();
		break;
	}
	case 9: { // 方框+点
		const float r = length + gap;
		XhairRectAA(g, cx - r, cy - r, r * 2.f, r * 2.f, thick, col, outline, oc);
		maybeDot();
		break;
	}
	case 10: { // 菱形
		const float r = length + gap;
		if (outline > 0.f) {
			XhairStrokeLine(g, cx, cy - r, cx + r, cy, thick + outline * 2.f, oc);
			XhairStrokeLine(g, cx + r, cy, cx, cy + r, thick + outline * 2.f, oc);
			XhairStrokeLine(g, cx, cy + r, cx - r, cy, thick + outline * 2.f, oc);
			XhairStrokeLine(g, cx - r, cy, cx, cy - r, thick + outline * 2.f, oc);
		}
		XhairStrokeLine(g, cx, cy - r, cx + r, cy, thick, col);
		XhairStrokeLine(g, cx + r, cy, cx, cy + r, thick, col);
		XhairStrokeLine(g, cx, cy + r, cx - r, cy, thick, col);
		XhairStrokeLine(g, cx - r, cy, cx, cy - r, thick, col);
		maybeDot();
		break;
	}
	case 11: { // 人字
		const float r = length + gap;
		if (outline > 0.f) {
			XhairStrokeLine(g, cx, cy - r, cx - r, cy + r * 0.5f, thick + outline * 2.f, oc);
			XhairStrokeLine(g, cx, cy - r, cx + r, cy + r * 0.5f, thick + outline * 2.f, oc);
		}
		XhairStrokeLine(g, cx, cy - r, cx - r, cy + r * 0.5f, thick, col);
		XhairStrokeLine(g, cx, cy - r, cx + r, cy + r * 0.5f, thick, col);
		maybeDot();
		break;
	}
	case 12: { // 四角
		const float inn = gap;
		const float arm = length;
		auto corner = [&](float ix, float iy, float hx, float vy) {
			XhairArmAA(g, ix, iy, hx, 0.f, arm, thick, 0.f, col, outline, oc);
			XhairArmAA(g, ix, iy, 0.f, vy, arm, thick, 0.f, col, outline, oc);
		};
		corner(cx - inn, cy - inn, -1.f, -1.f);
		corner(cx + inn, cy - inn, 1.f, -1.f);
		corner(cx - inn, cy + inn, -1.f, 1.f);
		corner(cx + inn, cy + inn, 1.f, 1.f);
		maybeDot();
		break;
	}
	case 13: { // 双环
		const float outer = length + gap;
		float inner = outer - thick * 3.f - 2.f;
		if (inner < thick + 1.f) inner = outer * 0.55f;
		XhairCircleAA(g, cx, cy, outer, thick, col, outline, oc);
		XhairCircleAA(g, cx, cy, inner, thick, col, outline, oc);
		maybeDot();
		break;
	}
	case 14: // 横线
		XhairArmAA(g, cx, cy, 1.f, 0.f, length, thick, gap, col, outline, oc);
		XhairArmAA(g, cx, cy, -1.f, 0.f, length, thick, gap, col, outline, oc);
		maybeDot();
		break;
	case 15: { // 三角
		const float r = length + gap;
		const float x1 = cx, y1 = cy - r;
		const float x2 = cx - r * 0.86f, y2 = cy + r * 0.5f;
		const float x3 = cx + r * 0.86f, y3 = cy + r * 0.5f;
		if (outline > 0.f) {
			const float ot = thick + outline * 2.f;
			XhairStrokeLine(g, x1, y1, x2, y2, ot, oc);
			XhairStrokeLine(g, x2, y2, x3, y3, ot, oc);
			XhairStrokeLine(g, x3, y3, x1, y1, ot, oc);
		}
		XhairStrokeLine(g, x1, y1, x2, y2, thick, col);
		XhairStrokeLine(g, x2, y2, x3, y3, thick, col);
		XhairStrokeLine(g, x3, y3, x1, y1, thick, col);
		maybeDot();
		break;
	}
	default:
		cross();
		maybeDot();
		break;
	}
}

static int XhairStyleSig() {
	const XhairTune& t = XhairT();
	int s = g_xhairStyle + 1;
	s = s * 131 + t.color;
	s = s * 131 + t.size;
	s = s * 131 + t.length;
	s = s * 131 + t.gap;
	s = s * 131 + t.thick;
	s = s * 131 + t.dot;
	s = s * 131 + t.outline;
	s = s * 131 + t.alpha;
	return s;
}

static int XhairPotSize(int win) {
	int pot = 32;
	while (pot < win)
		pot <<= 1;
	if (pot > 512)
		pot = 512;
	return pot;
}

static int g_hudXhairId = -1;
static int g_hudXhairAlloc = 0;
static int g_hudXhairWin = 0;
static int g_hudXhairSig = 0;
static int g_hudXhairDrawH = 0;
static bool g_hudXhairOk = false;
static bool g_hudXhairLoggedFail = false;
static int g_hudWhiteId = -1;
// Procedural textures cannot be resized in-place (vguimatsurface crash + UV wrap
// looking like the crosshair jumped to a corner). Always bake/upload/draw this.
static constexpr int kHudXhairCanvas = 512;

static void XhairDrawHudFallback(int cx, int cy) {
	const XhairTune& t = XhairT();
	int a = (t.alpha * 255) / 100;
	if (a < 16) a = 16;
	if (a > 255) a = 255;
	const COLORREF rgb = kXhairColorRgb[(t.color >= 0 && t.color < kXhairColorCount) ? t.color : 2];
	int length = (int)(XhairScaleF(t.length, 1.f) + 0.5f);
	int gap = (t.gap <= 0) ? 0 : (int)(XhairScaleF(t.gap, 0.f) + 0.5f);
	int thick = (int)(XhairScaleF(t.thick, 1.f) + 0.5f);
	int outline = (t.outline <= 0) ? 0 : (int)(XhairScaleF(t.outline, 1.f) + 0.5f);
	int dot = (t.dot <= 0) ? 0 : (int)(XhairScaleF(t.dot, 1.f) + 0.5f);
	if (length < 1) length = 1;
	if (thick < 1) thick = 1;
	const int ht = thick / 2;
	auto arm = [&](int x0, int y0, int x1, int y1) {
		if (x1 < x0) { int tmp = x0; x0 = x1; x1 = tmp; }
		if (y1 < y0) { int tmp = y0; y0 = y1; y1 = tmp; }
		if (x1 <= x0) x1 = x0 + 1;
		if (y1 <= y0) y1 = y0 + 1;
		if (outline > 0) {
			SurfColor(0, 0, 0, a);
			SurfFill(x0 - outline, y0 - outline, x1 + outline, y1 + outline);
		}
		SurfColor(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb), a);
		SurfFill(x0, y0, x1, y1);
	};
	arm(cx + gap, cy - ht, cx + gap + length, cy - ht + thick);
	arm(cx - gap - length, cy - ht, cx - gap, cy - ht + thick);
	arm(cx - ht, cy + gap, cx - ht + thick, cy + gap + length);
	arm(cx - ht, cy - gap - length, cx - ht + thick, cy - gap);
	if (dot > 0) {
		if (outline > 0) {
			SurfColor(0, 0, 0, a);
			SurfFill(cx - dot / 2 - outline, cy - dot / 2 - outline,
				cx - dot / 2 + dot + outline, cy - dot / 2 + dot + outline);
		}
		SurfColor(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb), a);
		SurfFill(cx - dot / 2, cy - dot / 2, cx - dot / 2 + dot, cy - dot / 2 + dot);
	}
}

static bool __declspec(noinline) XhairBakeHudTexture() {
	const int pot = kHudXhairCanvas;
	XhairGdiplusInit();
	if (!g_gdiplusOk || !MenuEnsureSurf())
		return false;

	using CreateIdFn = int(__thiscall*)(void*, bool);
	auto createId = (CreateIdFn)VGet(g_surf, kVmtSurfCreateNewTextureID);
	if (!createId || !IsExec((void*)createId)) {
		if (!g_hudXhairLoggedFail) {
			g_hudXhairLoggedFail = true;
			Log("xhairHUD missing CreateNewTextureID");
		}
		return false;
	}

	Gdiplus::Bitmap bmp(pot, pot, PixelFormat32bppPARGB);
	if (bmp.GetLastStatus() != Gdiplus::Ok)
		return false;
	Gdiplus::Graphics gfx(&bmp);
	if (gfx.GetLastStatus() != Gdiplus::Ok)
		return false;
	gfx.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	gfx.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	gfx.Clear(Gdiplus::Color(0, 0, 0, 0));
	XhairDrawAll(gfx, (float)pot * 0.5f, (float)pot * 0.5f);

	Gdiplus::BitmapData bd{};
	Gdiplus::Rect lockRc(0, 0, pot, pot);
	if (bmp.LockBits(&lockRc, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bd) != Gdiplus::Ok)
		return false;

	static unsigned char s_rgba[512 * 512 * 4];
	int maxA = 0;
	for (int y = 0; y < pot; ++y) {
		const unsigned char* src = (const unsigned char*)bd.Scan0 + y * bd.Stride;
		unsigned char* dst = s_rgba + (size_t)y * (size_t)pot * 4u;
		for (int x = 0; x < pot; ++x) {
			const unsigned char b = src[x * 4 + 0];
			const unsigned char g = src[x * 4 + 1];
			const unsigned char r = src[x * 4 + 2];
			const unsigned char a = src[x * 4 + 3];
			if ((int)a > maxA) maxA = (int)a;
			if (a == 0) {
				dst[x * 4 + 0] = 0;
				dst[x * 4 + 1] = 0;
				dst[x * 4 + 2] = 0;
				dst[x * 4 + 3] = 0;
			} else {
				dst[x * 4 + 0] = (unsigned char)((r * 255) / a);
				dst[x * 4 + 1] = (unsigned char)((g * 255) / a);
				dst[x * 4 + 2] = (unsigned char)((b * 255) / a);
				dst[x * 4 + 3] = a;
			}
		}
	}
	bmp.UnlockBits(&bd);
	if (maxA <= 0)
		return false;

	if (g_hudXhairId < 0 || g_hudXhairAlloc != pot) {
		g_hudXhairId = createId(g_surf, true);
		g_hudXhairAlloc = 0;
		if (g_hudXhairId < 0) {
			if (!g_hudXhairLoggedFail) {
				g_hudXhairLoggedFail = true;
				Log("xhairHUD CreateNewTextureID=%d", g_hudXhairId);
			}
			return false;
		}
	}
	SurfSetTextureRGBA(g_hudXhairId, s_rgba, pot, pot);
	g_hudXhairAlloc = pot;
	g_hudXhairWin = pot;
	static bool s_logged = false;
	if (!s_logged) {
		s_logged = true;
		Log("xhairHUD baked id=%d canvas=%d maxA=%d sh=%d",
			g_hudXhairId, pot, maxA, g_xhairDrawH);
	}
	return true;
}

static bool HudEnsureWhiteTex() {
	if (g_hudWhiteId >= 0) return true;
	if (!MenuEnsureSurf()) return false;
	using CreateIdFn = int(__thiscall*)(void*, bool);
	auto createId = (CreateIdFn)VGet(g_surf, kVmtSurfCreateNewTextureID);
	if (!createId || !IsExec((void*)createId)) return false;
	const int id = createId(g_surf, true);
	if (id < 0) return false;
	unsigned char px[16 * 16 * 4];
	for (int i = 0; i < 16 * 16; ++i) {
		px[i * 4 + 0] = 255;
		px[i * 4 + 1] = 255;
		px[i * 4 + 2] = 255;
		px[i * 4 + 3] = 255;
	}
	SurfSetTextureRGBA(id, px, 16, 16);
	g_hudWhiteId = id;
	Log("xhairHUD whiteTex id=%d (elim-style fill)", id);
	return true;
}

static void HudCol(COLORREF c, int a) {
	SurfColor(GetRValue(c), GetGValue(c), GetBValue(c), a);
}

// Same primitive family as the elim HUD panel (solid quads), but tint a 16x16
// all-white procedural texture instead of vgui/white — that material's bilinear
// edge makes 1–2px crosshair arms look muddy while large panels still look fine.
static void HudBox(int x0, int y0, int x1, int y1) {
	if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
	if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
	if (x1 <= x0) x1 = x0 + 1;
	if (y1 <= y0) y1 = y0 + 1;
	if (g_hudWhiteId >= 0) {
		using SetTexFn = void(__thiscall*)(void*, int);
		using RectFn = void(__thiscall*)(void*, int, int, int, int);
		static SetTexFn s_setTex = nullptr;
		static RectFn s_rect = nullptr;
		if (!s_setTex || !s_rect) {
			s_setTex = (SetTexFn)VGet(g_surf, kVmtSurfDrawSetTexture);
			s_rect = (RectFn)VGet(g_surf, kVmtSurfDrawTexturedRect);
			if (!s_setTex || !s_rect || !IsExec((void*)s_setTex) || !IsExec((void*)s_rect)) {
				s_setTex = nullptr;
				s_rect = nullptr;
			}
		}
		if (s_setTex && s_rect) {
			s_setTex(g_surf, g_hudWhiteId);
			s_rect(g_surf, x0, y0, x1, y1);
			return;
		}
	}
	SurfFill(x0, y0, x1, y1);
}

static void HudStroke(float x1, float y1, float x2, float y2, float thick, COLORREF rgb, int a) {
	HudCol(rgb, a);
	int t = (int)(thick + 0.5f);
	if (t < 1) t = 1;
	const int ix1 = (int)floorf(x1 + 0.5f);
	const int iy1 = (int)floorf(y1 + 0.5f);
	const int ix2 = (int)floorf(x2 + 0.5f);
	const int iy2 = (int)floorf(y2 + 0.5f);
	if (iy1 == iy2) {
		HudBox(ix1, iy1 - t / 2, ix2, iy1 - t / 2 + t);
		return;
	}
	if (ix1 == ix2) {
		HudBox(ix1 - t / 2, iy1, ix1 - t / 2 + t, iy2);
		return;
	}
	const float fx0 = (float)ix1, fy0 = (float)iy1, fx1 = (float)ix2, fy1 = (float)iy2;
	float dx = fx1 - fx0, dy = fy1 - fy0;
	const float len = sqrtf(dx * dx + dy * dy);
	if (len < 0.5f) {
		HudBox(ix1 - t / 2, iy1 - t / 2, ix1 - t / 2 + t, iy1 - t / 2 + t);
		return;
	}
	const float hx = -dy / len * (float)t * 0.5f;
	const float hy = dx / len * (float)t * 0.5f;
	const float cx[4] = { fx0 + hx, fx0 - hx, fx1 - hx, fx1 + hx };
	const float cy[4] = { fy0 + hy, fy0 - hy, fy1 - hy, fy1 + hy };
	float minY = cy[0], maxY = cy[0];
	for (int i = 1; i < 4; ++i) {
		if (cy[i] < minY) minY = cy[i];
		if (cy[i] > maxY) maxY = cy[i];
	}
	const int y0s = (int)floorf(minY);
	const int y1s = (int)ceilf(maxY);
	for (int y = y0s; y < y1s; ++y) {
		const float ys = (float)y + 0.5f;
		float xa = 1e9f, xb = -1e9f;
		int hits = 0;
		for (int i = 0; i < 4; ++i) {
			const int j = (i + 1) & 3;
			const float ya = cy[i], yb = cy[j];
			if ((ya <= ys && yb > ys) || (yb <= ys && ya > ys)) {
				const float u = (ys - ya) / (yb - ya);
				const float x = cx[i] + u * (cx[j] - cx[i]);
				if (x < xa) xa = x;
				if (x > xb) xb = x;
				++hits;
			}
		}
		if (hits < 2 || xb <= xa) continue;
		HudBox((int)floorf(xa + 0.5f), y, (int)floorf(xb + 0.5f), y + 1);
	}
}

static void HudArm(float cx, float cy, float dirX, float dirY, float length, float thick, float gap,
	COLORREF rgb, int a, float outline, COLORREF oc) {
	if (length <= 0.f || thick <= 0.f) return;
	float x1, y1, x2, y2;
	if (dirY == 0.f) {
		y1 = y2 = cy;
		if (dirX > 0.f) { x1 = cx + gap; x2 = cx + gap + length; }
		else { x1 = cx - gap; x2 = cx - gap - length; }
	} else {
		x1 = x2 = cx;
		if (dirY > 0.f) { y1 = cy + gap; y2 = cy + gap + length; }
		else { y1 = cy - gap; y2 = cy - gap - length; }
	}
	if (outline > 0.f)
		HudStroke(x1, y1, x2, y2, thick + outline * 2.f, oc, 255);
	HudStroke(x1, y1, x2, y2, thick, rgb, a);
}

static void HudDot(float cx, float cy, float sz, COLORREF rgb, int a, float outline, COLORREF oc) {
	if (sz <= 0.f) return;
	int r = (int)(sz * 0.5f + 0.5f);
	if (r < 1) r = 1;
	auto disc = [&](int rr, COLORREF c, int aa) {
		HudCol(c, aa);
		const int px = (int)floorf(cx + 0.5f);
		const int py = (int)floorf(cy + 0.5f);
		for (int y = -rr; y <= rr; ++y) {
			int w2 = rr * rr - y * y;
			if (w2 < 0) continue;
			int w = (int)(sqrtf((float)w2) + 0.5f);
			HudBox(px - w, py + y, px + w + 1, py + y + 1);
		}
	};
	if (outline > 0.f)
		disc(r + (int)(outline + 0.5f), oc, 255);
	disc(r, rgb, a);
}

static void HudCircle(float cx, float cy, float radius, float thick, COLORREF rgb, int a,
	float outline, COLORREF oc) {
	if (radius <= 0.f || thick <= 0.f) return;
	const int px = (int)floorf(cx + 0.5f);
	const int py = (int)floorf(cy + 0.5f);
	auto ring = [&](float r, float th, COLORREF c, int aa) {
		int ro = (int)floorf(r + th * 0.5f + 0.5f);
		int ri = (int)floorf(r - th * 0.5f + 0.5f);
		if (ro < 1) ro = 1;
		if (ri < 0) ri = 0;
		HudCol(c, aa);
		for (int y = -ro; y <= ro; ++y) {
			const int y2 = y * y;
			const int xo2 = ro * ro - y2;
			if (xo2 < 0) continue;
			const int xo = (int)(sqrtf((float)xo2) + 0.5f);
			const int xi2 = (ri > 0) ? (ri * ri - y2) : -1;
			if (xi2 <= 0) {
				HudBox(px - xo, py + y, px + xo + 1, py + y + 1);
				continue;
			}
			const int xi = (int)(sqrtf((float)xi2) + 0.5f);
			if (xo <= xi) continue;
			HudBox(px - xo, py + y, px - xi, py + y + 1);
			HudBox(px + xi + 1, py + y, px + xo + 1, py + y + 1);
		}
	};
	if (outline > 0.f)
		ring(radius, thick + outline * 2.f, oc, 255);
	ring(radius, thick, rgb, a);
}

static void HudRectStroke(float x, float y, float w, float h, float thick, COLORREF rgb, int a,
	float outline, COLORREF oc) {
	const float x2 = x + w, y2 = y + h;
	if (outline > 0.f) {
		const float ot = thick + outline * 2.f;
		HudStroke(x, y, x2, y, ot, oc, 255);
		HudStroke(x2, y, x2, y2, ot, oc, 255);
		HudStroke(x2, y2, x, y2, ot, oc, 255);
		HudStroke(x, y2, x, y, ot, oc, 255);
	}
	HudStroke(x, y, x2, y, thick, rgb, a);
	HudStroke(x2, y, x2, y2, thick, rgb, a);
	HudStroke(x2, y2, x, y2, thick, rgb, a);
	HudStroke(x, y2, x, y, thick, rgb, a);
}

static void XhairDrawHudGeom(int cx, int cy) {
	const XhairTune& t = XhairT();
	int a = (t.alpha * 255) / 100;
	if (a < 16) a = 16;
	if (a > 255) a = 255;
	const COLORREF rgb = kXhairColorRgb[(t.color >= 0 && t.color < kXhairColorCount) ? t.color : 2];
	const COLORREF oc = RGB(0, 0, 0);
	const float fcx = (float)cx, fcy = (float)cy;
	auto px = [](int logical, float minv) -> float {
		float v = XhairScaleF(logical, minv);
		float r = floorf(v + 0.5f);
		if (r < minv) r = minv;
		return r;
	};
	const float length = px(t.length, 1.f);
	const float gap = (t.gap <= 0) ? 0.f : px(t.gap, 0.f);
	const float thick = px(t.thick, 1.f);
	const float outline = (t.outline <= 0) ? 0.f : px(t.outline, 1.f);
	float dot = (t.dot <= 0) ? 0.f : px(t.dot, 1.f);
	const int style = g_xhairStyle;
	auto cross = [&]() {
		HudArm(fcx, fcy, 1.f, 0.f, length, thick, gap, rgb, a, outline, oc);
		HudArm(fcx, fcy, -1.f, 0.f, length, thick, gap, rgb, a, outline, oc);
		HudArm(fcx, fcy, 0.f, 1.f, length, thick, gap, rgb, a, outline, oc);
		HudArm(fcx, fcy, 0.f, -1.f, length, thick, gap, rgb, a, outline, oc);
	};
	auto tee = [&]() {
		HudArm(fcx, fcy, 1.f, 0.f, length, thick, gap, rgb, a, outline, oc);
		HudArm(fcx, fcy, -1.f, 0.f, length, thick, gap, rgb, a, outline, oc);
		HudArm(fcx, fcy, 0.f, 1.f, length, thick, gap, rgb, a, outline, oc);
	};
	auto maybeDot = [&]() {
		if (dot > 0.f)
			HudDot(fcx, fcy, dot, rgb, a, outline, oc);
	};
	switch (style) {
	case 0: case 1:
		cross();
		maybeDot();
		break;
	case 2:
		if (dot <= 0.f) dot = px(4, 1.f);
		HudDot(fcx, fcy, dot, rgb, a, outline, oc);
		break;
	case 3: case 4:
		HudCircle(fcx, fcy, length + gap, thick, rgb, a, outline, oc);
		maybeDot();
		break;
	case 5:
		HudCircle(fcx, fcy, length + gap + thick, thick, rgb, a, outline, oc);
		cross();
		maybeDot();
		break;
	case 6: case 7:
		tee();
		maybeDot();
		break;
	case 8: case 9: {
		const float r = length + gap;
		HudRectStroke(fcx - r, fcy - r, r * 2.f, r * 2.f, thick, rgb, a, outline, oc);
		maybeDot();
		break;
	}
	case 10: {
		const float r = length + gap;
		if (outline > 0.f) {
			const float ot = thick + outline * 2.f;
			HudStroke(fcx, fcy - r, fcx + r, fcy, ot, oc, 255);
			HudStroke(fcx + r, fcy, fcx, fcy + r, ot, oc, 255);
			HudStroke(fcx, fcy + r, fcx - r, fcy, ot, oc, 255);
			HudStroke(fcx - r, fcy, fcx, fcy - r, ot, oc, 255);
		}
		HudStroke(fcx, fcy - r, fcx + r, fcy, thick, rgb, a);
		HudStroke(fcx + r, fcy, fcx, fcy + r, thick, rgb, a);
		HudStroke(fcx, fcy + r, fcx - r, fcy, thick, rgb, a);
		HudStroke(fcx - r, fcy, fcx, fcy - r, thick, rgb, a);
		maybeDot();
		break;
	}
	case 11: {
		const float r = length + gap;
		if (outline > 0.f) {
			const float ot = thick + outline * 2.f;
			HudStroke(fcx, fcy - r, fcx - r, fcy + r * 0.5f, ot, oc, 255);
			HudStroke(fcx, fcy - r, fcx + r, fcy + r * 0.5f, ot, oc, 255);
		}
		HudStroke(fcx, fcy - r, fcx - r, fcy + r * 0.5f, thick, rgb, a);
		HudStroke(fcx, fcy - r, fcx + r, fcy + r * 0.5f, thick, rgb, a);
		maybeDot();
		break;
	}
	case 12: {
		const float inn = gap;
		const float arm = length;
		auto corner = [&](float ix, float iy, float hx, float vy) {
			HudArm(ix, iy, hx, 0.f, arm, thick, 0.f, rgb, a, outline, oc);
			HudArm(ix, iy, 0.f, vy, arm, thick, 0.f, rgb, a, outline, oc);
		};
		corner(fcx - inn, fcy - inn, -1.f, -1.f);
		corner(fcx + inn, fcy - inn, 1.f, -1.f);
		corner(fcx - inn, fcy + inn, -1.f, 1.f);
		corner(fcx + inn, fcy + inn, 1.f, 1.f);
		maybeDot();
		break;
	}
	case 13: {
		const float outer = length + gap;
		float inner = outer - thick * 3.f - 2.f;
		if (inner < thick + 1.f) inner = outer * 0.55f;
		HudCircle(fcx, fcy, outer, thick, rgb, a, outline, oc);
		HudCircle(fcx, fcy, inner, thick, rgb, a, outline, oc);
		maybeDot();
		break;
	}
	case 14:
		HudArm(fcx, fcy, 1.f, 0.f, length, thick, gap, rgb, a, outline, oc);
		HudArm(fcx, fcy, -1.f, 0.f, length, thick, gap, rgb, a, outline, oc);
		maybeDot();
		break;
	case 15: {
		const float r = length + gap;
		const float x1 = fcx, y1 = fcy - r;
		const float x2 = fcx - r * 0.86f, y2 = fcy + r * 0.5f;
		const float x3 = fcx + r * 0.86f, y3 = fcy + r * 0.5f;
		if (outline > 0.f) {
			const float ot = thick + outline * 2.f;
			HudStroke(x1, y1, x2, y2, ot, oc, 255);
			HudStroke(x2, y2, x3, y3, ot, oc, 255);
			HudStroke(x3, y3, x1, y1, ot, oc, 255);
		}
		HudStroke(x1, y1, x2, y2, thick, rgb, a);
		HudStroke(x2, y2, x3, y3, thick, rgb, a);
		HudStroke(x3, y3, x1, y1, thick, rgb, a);
		maybeDot();
		break;
	}
	default:
		cross();
		maybeDot();
		break;
	}
}

static void SurfDrawHudXhair() {
	XhairDrawOverlayGameThread();
}

// Overlay: GDI+ → UpdateLayeredWindow at window-client pixels.
static bool XhairPushLayer(bool force) {
	if (!g_xhairHwnd || !g_gdiplusOk) return false;
	g_gameHwnd = FindGameWindow();
	if (!g_gameHwnd) return false;
	RECT crc{};
	if (!GetClientRect(g_gameHwnd, &crc)) return false;
	const int cw = crc.right - crc.left;
	const int ch = crc.bottom - crc.top;
	if (cw < 80 || ch < 80) return false;
	g_xhairDrawH = ch;

	const int win = XhairWantWin();
	int x = (cw - win) / 2;
	int y = (ch - win) / 2;
	POINT pt{ x, y };
	ClientToScreen(g_gameHwnd, &pt);
	const int sig = XhairStyleSig();
	if (!force && !g_xhairParkedOffscreen && g_xhairLastWin == win
		&& g_xhairLastX == pt.x && g_xhairLastY == pt.y && g_xhairLastSig == sig)
		return true;

	Gdiplus::Bitmap bmp(win, win, PixelFormat32bppPARGB);
	Gdiplus::Graphics gfx(&bmp);
	gfx.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
	gfx.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
	gfx.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	gfx.Clear(Gdiplus::Color(0, 0, 0, 0));
	XhairDrawAll(gfx, (float)win * 0.5f, (float)win * 0.5f);

	Gdiplus::BitmapData bd{};
	Gdiplus::Rect lockRc(0, 0, win, win);
	if (bmp.LockBits(&lockRc, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bd) != Gdiplus::Ok)
		return false;

	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = win;
	bmi.bmiHeader.biHeight = -win;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* bits = nullptr;
	HDC screen = GetDC(nullptr);
	HDC mem = CreateCompatibleDC(screen);
	HBITMAP dib = CreateDIBSection(mem, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
	HGDIOBJ old = SelectObject(mem, dib);
	if (bits && bd.Scan0) {
		const int dstStride = win * 4;
		for (int y = 0; y < win; ++y)
			memcpy((BYTE*)bits + y * dstStride, (const BYTE*)bd.Scan0 + y * bd.Stride, dstStride);
	}
	bmp.UnlockBits(&bd);

	SIZE sz{ win, win };
	POINT src{ 0, 0 };
	BLENDFUNCTION bf{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	// ULW_ALPHA: DWM hit-tests non-zero alpha pixels. HTTRANSPARENT is the backstop
	// (see XhairWndProc). Click-through failure → stuck +attack, docs §13.
	UpdateLayeredWindow(g_xhairHwnd, screen, &pt, &sz, mem, &src, 0, &bf, ULW_ALPHA);
	g_xhairWin = win;
	g_xhairLastWin = win;
	g_xhairLastX = pt.x;
	g_xhairLastY = pt.y;
	g_xhairLastSig = sig;
	g_xhairParkedOffscreen = false;

	SelectObject(mem, old);
	DeleteObject(dib);
	DeleteDC(mem);
	ReleaseDC(nullptr, screen);
	return true;
}

static void XhairLogFocus(const char* why);
static void XhairForwardToGame(UINT msg, WPARAM wp, LPARAM lp) {
	if (g_gameHwnd && IsWindow(g_gameHwnd))
		PostMessageW(g_gameHwnd, msg, wp, lp);
}

static LRESULT CALLBACK XhairWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_PAINT: {
		PAINTSTRUCT ps{};
		BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_ERASEBKGND:
		return 1;
	case WM_NCHITTEST:
		return HTTRANSPARENT;
	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP: {
		// ULW_ALPHA hit-tests opaque/fringe pixels. HTTRANSPARENT should make
		// this unreachable; if it fires, the game can miss -attack and keep firing.
		static int s_mouseHit = 0;
		if (s_mouseHit < 16) {
			++s_mouseHit;
			Log("xhair WndProc mouse=0x%X (click-through FAILED) shown=%d",
				msg, g_xhairOsShown ? 1 : 0);
			XhairLogFocus("mouse-hit");
		}
		if (msg == WM_LBUTTONUP || msg == WM_RBUTTONUP
			|| msg == WM_MBUTTONUP || msg == WM_XBUTTONUP)
			XhairForwardToGame(msg, wp, 0);
		return 0;
	}
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP: {
		static int s_keyHit = 0;
		if (s_keyHit < 16) {
			++s_keyHit;
			Log("xhair WndProc key=0x%X vk=%u (click-through FAILED) shown=%d",
				msg, (unsigned)wp, g_xhairOsShown ? 1 : 0);
			XhairLogFocus("key-hit");
		}
		XhairForwardToGame(msg, wp, lp);
		return 0;
	}
	case WM_CLOSE:
		Log("xhair WndProc WM_CLOSE — overlay got Alt+F4; forwarding to game");
		XhairLogFocus("close");
		XhairForwardToGame(WM_CLOSE, 0, 0);
		return 0;
	case WM_XHAIR_DESTROY:
		DestroyWindow(hwnd);
		return 0;
	case WM_SYSCOMMAND:
		if ((wp & 0xFFF0) == SC_CLOSE) {
			Log("xhair WndProc SC_CLOSE — overlay got Alt+F4; forwarding to game");
			XhairLogFocus("sysclose");
			XhairForwardToGame(WM_SYSCOMMAND, wp, lp);
			return 0;
		}
		break;
	case WM_ACTIVATE:
	case WM_NCACTIVATE:
		if (LOWORD(wp) != WA_INACTIVE) {
			static int s_act = 0;
			if (s_act < 8) {
				++s_act;
				Log("xhair WndProc ACTIVATE msg=0x%X wp=%p", msg, (void*)wp);
				XhairLogFocus("activate");
			}
			return 0;
		}
		break;
	case WM_SETFOCUS: {
		static int s_sf = 0;
		if (s_sf < 8) {
			++s_sf;
			Log("xhair WndProc WM_SETFOCUS");
			XhairLogFocus("setfocus");
		}
		return 0;
	}
	default:
		return DefWindowProcW(hwnd, msg, wp, lp);
	}
	return DefWindowProcW(hwnd, msg, wp, lp);
}

static void XhairUiDisableIme() {
	HMODULE imm = GetModuleHandleW(L"imm32.dll");
	if (!imm)
		imm = LoadLibraryW(L"imm32.dll");
	if (!imm)
		return;
	using Fn = BOOL(WINAPI*)(DWORD);
	auto fn = (Fn)GetProcAddress(imm, "ImmDisableIME");
	// 0 = this thread only. Do NOT pass -1 (that disables IME for the game too).
	if (fn)
		fn(0);
}

static void XhairLogFocus(const char* why) {
	HWND fg = GetForegroundWindow();
	char fgClass[64]{}, focClass[64]{}, capClass[64]{};
	if (fg)
		GetClassNameA(fg, fgClass, (int)sizeof(fgClass));
	GUITHREADINFO gi{};
	gi.cbSize = sizeof(gi);
	HWND focus = nullptr, cap = nullptr;
	DWORD flags = 0;
	if (g_gameHwnd) {
		const DWORD tid = GetWindowThreadProcessId(g_gameHwnd, nullptr);
		if (tid && GetGUIThreadInfo(tid, &gi)) {
			focus = gi.hwndFocus;
			cap = gi.hwndCapture;
			flags = gi.flags;
		}
	}
	if (focus)
		GetClassNameA(focus, focClass, (int)sizeof(focClass));
	if (cap)
		GetClassNameA(cap, capClass, (int)sizeof(capClass));
	Log("input-focus %s fg=%p/%s focus=%p/%s cap=%p/%s xhwnd=%p xpark=%d shown=%d flags=0x%X",
		why ? why : "?",
		(void*)fg, fgClass[0] ? fgClass : "-",
		(void*)focus, focClass[0] ? focClass : "-",
		(void*)cap, capClass[0] ? capClass : "-",
		(void*)g_xhairHwnd, g_xhairParked ? 1 : 0, g_xhairOsShown ? 1 : 0, flags);
}

static void XhairEnsureWindow() {
	XhairGdiplusInit();
	if (g_xhairHwnd && IsWindow(g_xhairHwnd)) return;
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = XhairWndProc;
	wc.hInstance = GetModuleHandleW(nullptr);
	wc.lpszClassName = kXhairClass;
	wc.hCursor = nullptr;
	RegisterClassExW(&wc);
	g_xhairWin = XhairWantWin();
	if (!g_gameHwnd)
		g_gameHwnd = FindGameWindow();
	// hwndParent MUST be null. Owner=Valve001 on a different thread is the
	// "look works, keys/buttons dead" bug (Windows attaches the input queues).
	g_xhairHwnd = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
		kXhairClass, L"",
		WS_POPUP,
		-32000, -32000, g_xhairWin, g_xhairWin,
		nullptr, nullptr, wc.hInstance, nullptr);
	if (!g_xhairHwnd) {
		Log("xhair ERROR: CreateWindowEx failed err=%lu", GetLastError());
		return;
	}
	g_xhairCreateTid = GetCurrentThreadId();
	SetWindowPos(g_xhairHwnd, HWND_TOPMOST, -32000, -32000, g_xhairWin, g_xhairWin,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	g_xhairOsShown = true;
	g_xhairParkedOffscreen = true;
	Log("xhair window hwnd=%p tid=%lu (game thread, no owner, no hide)",
		(void*)g_xhairHwnd, (unsigned long)g_xhairCreateTid);
}

static void XhairUiDestroyWindow(const char* why);
static void XhairParkOffscreen() {
	if (!g_xhairHwnd || !IsWindow(g_xhairHwnd))
		return;
	if (g_xhairParkedOffscreen)
		return;
	SetWindowPos(g_xhairHwnd, nullptr, -32000, -32000, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
	g_xhairParkedOffscreen = true;
	g_xhairLastX = 0x7fffffff;
	g_xhairLastY = 0x7fffffff;
}

static void XhairDrawOverlayGameThread() {
	if (!g_optXhair) {
		if (g_xhairHwnd)
			XhairUiDestroyWindow("overlay off");
		return;
	}
	const bool want = g_run.load(std::memory_order_relaxed) && SkeetoFeaturesOn()
		&& g_engine && EngInGame()
		&& !g_xhairHideScene.load(std::memory_order_relaxed)
		&& !EngDrawingLoading();
	g_xhairWantShown.store(want, std::memory_order_relaxed);
	if (!want) {
		XhairParkOffscreen();
		return;
	}
	XhairEnsureWindow();
	if (!g_xhairHwnd)
		return;
	const bool force = g_xhairRedraw.exchange(false, std::memory_order_relaxed);
	if (!XhairPushLayer(force || g_xhairParkedOffscreen))
		return;
	static bool s_logged = false;
	if (!s_logged) {
		s_logged = true;
		Log("xhair overlay game-thread hwnd=%p clientH=%d", (void*)g_xhairHwnd, g_xhairDrawH);
	}
}

static void XhairUiHide() {
	XhairParkOffscreen();
}

static void XhairUiDestroyWindow(const char* why) {
	HWND hwnd = g_xhairHwnd;
	if (!hwnd || !IsWindow(hwnd)) {
		g_xhairHwnd = nullptr;
		g_xhairCreateTid = 0;
		return;
	}
	if (g_xhairCreateTid && GetCurrentThreadId() != g_xhairCreateTid) {
		if (why && why[0])
			Log("xhair window destroy posted (%s)", why);
		PostMessageW(hwnd, WM_XHAIR_DESTROY, 0, 0);
		g_xhairHwnd = nullptr;
		g_xhairOsShown = false;
		g_xhairParkedOffscreen = false;
		g_xhairLastX = 0x7fffffff;
		g_xhairLastY = 0x7fffffff;
		return;
	}
	g_xhairHwnd = nullptr;
	g_xhairCreateTid = 0;
	g_xhairOsShown = false;
	g_xhairParkedOffscreen = false;
	g_xhairLastX = 0x7fffffff;
	g_xhairLastY = 0x7fffffff;
	DestroyWindow(hwnd);
	if (why && why[0])
		Log("xhair window destroyed (%s)", why);
}

static void XhairUiApply() {
	if (!g_xhairUiRun.load(std::memory_order_relaxed))
		return;
	if (!g_optXhair) {
		XhairUiDestroyWindow("overlay off");
		return;
	}

	HWND fg = GetForegroundWindow();
	if (g_xhairHwnd && fg == g_xhairHwnd) {
		static DWORD s_lastSteal = 0;
		const DWORD now = GetTickCount();
		if (now - s_lastSteal > 400) {
			s_lastSteal = now;
			Log("xhair stole focus hwnd=%p game=%p — parking",
				(void*)g_xhairHwnd, (void*)g_gameHwnd);
			XhairLogFocus("stole");
		}
		XhairParkOffscreen();
		return;
	}

	const bool want = g_xhairWantShown.load(std::memory_order_relaxed);
	const bool force = g_xhairRedraw.exchange(false, std::memory_order_relaxed);
	if (!want) {
		XhairUiHide();
		return;
	}

	XhairEnsureWindow();
	if (!g_xhairHwnd)
		return;
	if (!XhairPushLayer(force || !g_xhairOsShown))
		return;
	if (!g_xhairOsShown) {
		SetWindowPos(g_xhairHwnd, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
		g_xhairOsShown = true;
	}
}

static DWORD WINAPI XhairUiThread(void*) {
	MSG boot{};
	PeekMessageW(&boot, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
	XhairUiDisableIme();
	XhairGdiplusInit();
	if (g_xhairUiReady)
		SetEvent(g_xhairUiReady);
	Log("xhair ui thread start tid=%lu", GetCurrentThreadId());
	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
		if (msg.message == WM_XHAIR_SYNC) {
			XhairUiApply();
			continue;
		}
		// No TranslateMessage: that starts IME / WM_CHAR on this thread.
		DispatchMessageW(&msg);
	}
	if (g_xhairHwnd && IsWindow(g_xhairHwnd))
		XhairUiDestroyWindow("ui thread exit");
	else {
		g_xhairHwnd = nullptr;
		g_xhairOsShown = false;
		g_xhairParkedOffscreen = false;
	}
	Log("xhair ui thread exit");
	return 0;
}

static void XhairPostSync() {
	if (g_xhairUiTid)
		PostThreadMessageW(g_xhairUiTid, WM_XHAIR_SYNC, 0, 0);
}

static bool XhairEnsureUiThread() {
	if (g_xhairUiTh && g_xhairUiRun.load(std::memory_order_relaxed))
		return true;
	g_xhairUiRun.store(true, std::memory_order_relaxed);
	if (!g_xhairUiReady)
		g_xhairUiReady = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (g_xhairUiReady)
		ResetEvent(g_xhairUiReady);
	g_xhairUiTh = CreateThread(nullptr, 0, XhairUiThread, nullptr, 0, &g_xhairUiTid);
	if (!g_xhairUiTh) {
		g_xhairUiRun.store(false, std::memory_order_relaxed);
		g_xhairUiTid = 0;
		Log("xhair ERROR: ui thread create failed err=%lu", GetLastError());
		return false;
	}
	if (g_xhairUiReady)
		WaitForSingleObject(g_xhairUiReady, 2000);
	return true;
}

// Overlay = static aim. Ring = engine hidden circle that follows real spread.
// Never write the `crosshair` cvar. When the ring is off, do not touch the
// player's vanilla cl_crosshair_* either — only snapshot/restore around the ring.
static void XhairSyncEngineHud(bool wantRing) {
	if (!g_run.load(std::memory_order_relaxed) || !g_engine)
		return;
	CrashMark(kBcXhair);
	const DWORD now = GetTickCount();
	const int mode = (wantRing ? 1 : 0)
		| (g_xhairRingColor << 4) | ((g_xhairRingMode & 3) << 12)
		| ((g_xhairRingAlpha & 127) << 16);
	if (mode == g_xhairHudMode && (!wantRing || now - g_xhairHudAt < 1500))
		return;
	if (!IfaceAlive(g_engine))
		return;
	g_xhairHudMode = mode;
	g_xhairHudAt = now;

	UnlockXhairCircleCvars();
	if (!wantRing) {
		if (g_xhairRingApplied) {
			XhairRestoreUserCvars();
			g_xhairRingApplied = false;
		}
		return;
	}
	if (!g_xhairRingApplied) {
		XhairSnapshotUserCvars();
		g_xhairRingApplied = true;
	}
	const int ci = (g_xhairRingColor >= 0 && g_xhairRingColor < kXhairColorCount)
		? g_xhairRingColor : 1;
	const COLORREF rgb = kXhairColorRgb[ci];
	CvarSetInt("cl_crosshair_dynamic", 1);
	CvarSetInt("cl_crosshair_circle_mode", g_xhairRingMode);
	CvarSetInt("cl_crosshair_circle_alpha", XhairRingAlphaEngine());
	CvarSetInt("cl_crosshair_alpha", 0);
	CvarSetInt("cl_crosshair_red", (int)GetRValue(rgb));
	CvarSetInt("cl_crosshair_green", (int)GetGValue(rgb));
	CvarSetInt("cl_crosshair_blue", (int)GetBValue(rgb));
}

static void XhairNotifyChanged() {
	g_hudXhairOk = false;
	g_xhairRedraw.store(true, std::memory_order_relaxed);
}

static void XhairHideNow() {
	g_xhairHideScene.store(true, std::memory_order_relaxed);
	const DWORD until = GetTickCount() + 2800;
	if (until > g_xhairHideUntil)
		g_xhairHideUntil = until;
}

static void XhairKeepHiddenWhileLoading() {
	g_xhairHideScene.store(true, std::memory_order_relaxed);
	const DWORD until = GetTickCount() + 500;
	if (until > g_xhairHideUntil)
		g_xhairHideUntil = until;
}

// Game thread backup. Loading plaque often skips EngineVGui::Paint — worker must hide.
static void XhairNoteLoading() {
	if (!g_engine) return;
	const DWORD now = GetTickCount();
	if (EngDrawingLoading()) {
		XhairKeepHiddenWhileLoading();
		return;
	}
	if (g_xhairHideUntil && now < g_xhairHideUntil) {
		g_xhairHideScene.store(true, std::memory_order_relaxed);
		return;
	}
	g_xhairHideUntil = 0;
	g_xhairHideScene.store(false, std::memory_order_relaxed);
}

static void InputFocusTick() {
	if (!g_gameHwnd)
		g_gameHwnd = FindGameWindow();
	HWND fg = GetForegroundWindow();
	const bool gameFg = g_gameHwnd && fg && fg == g_gameHwnd;
	static bool s_wasGameFg = false;
	static bool s_loggedLeave = false;
	if (gameFg && !s_wasGameFg) {
		g_unstickButtons.store(true, std::memory_order_relaxed);
		Log("input focus returned to game - unstick armed");
		s_loggedLeave = false;
	} else if (!gameFg && s_wasGameFg && !s_loggedLeave) {
		s_loggedLeave = true;
		char cls[64]{};
		if (fg)
			GetClassNameA(fg, cls, (int)sizeof(cls));
		Log("input fg left game -> %p/%s", (void*)fg, cls[0] ? cls : "-");
	}
	s_wasGameFg = gameFg;
}

static void XhairStopOverlayUi() {
	g_xhairWantShown.store(false, std::memory_order_relaxed);
	if (!g_xhairUiTid && !g_xhairUiTh)
		return;
	g_xhairUiRun.store(false, std::memory_order_relaxed);
	if (g_xhairUiTid)
		PostThreadMessageW(g_xhairUiTid, WM_QUIT, 0, 0);
	if (g_xhairUiTh) {
		WaitForSingleObject(g_xhairUiTh, 1000);
		CloseHandle(g_xhairUiTh);
		g_xhairUiTh = nullptr;
	}
	g_xhairUiTid = 0;
	g_xhairHwnd = nullptr;
	g_xhairParked = false;
	g_xhairOsShown = false;
	g_xhairParkedOffscreen = false;
	Log("xhair overlay UI stopped (HUD draw)");
}

static void XhairTick() {
	if (!g_run.load(std::memory_order_relaxed)) return;
	if (!g_optXhair && !g_optXhairRing && !g_optXhairTex) {
		g_xhairWantShown.store(false, std::memory_order_relaxed);
		XhairUiDestroyWindow("all xhair off");
		XhairSyncEngineHud(false);
		return;
	}
	if (!g_optXhair)
		XhairUiDestroyWindow("overlay off");
	g_gameHwnd = FindGameWindow();
	const bool loading = g_engine && EngDrawingLoading();
	if (loading)
		XhairKeepHiddenWhileLoading();
	const bool ingame = g_engine && EngInGame();
	const bool feat = SkeetoFeaturesOn();
	const bool hideScene = g_xhairHideScene.load(std::memory_order_relaxed);
	const bool wantStatic = feat && g_optXhair && ingame && !hideScene && !loading;
	g_xhairWantShown.store(wantStatic, std::memory_order_relaxed);
	if (!wantStatic)
		XhairParkOffscreen();
	const bool wantRing = feat && g_optXhairRing && ingame && !hideScene && !loading;
	XhairSyncEngineHud(wantRing);
}

static void XhairShutdown() {
	g_xhairHudMode = -1;
	g_xhairWantShown.store(false, std::memory_order_relaxed);
	g_xhairUiRun.store(false, std::memory_order_relaxed);
	if (g_xhairUiTid)
		PostThreadMessageW(g_xhairUiTid, WM_QUIT, 0, 0);
	if (g_xhairUiTh) {
		WaitForSingleObject(g_xhairUiTh, 1000);
		CloseHandle(g_xhairUiTh);
		g_xhairUiTh = nullptr;
	}
	g_xhairUiTid = 0;
	XhairParkOffscreen();
	XhairUiDestroyWindow("shutdown");
	g_xhairParked = false;
	g_xhairOsShown = false;
	g_xhairParkedOffscreen = false;
	if (g_xhairUiReady) {
		CloseHandle(g_xhairUiReady);
		g_xhairUiReady = nullptr;
	}
	if (g_gdiplusOk) {
		Gdiplus::GdiplusShutdown(g_gdiplusToken);
		g_gdiplusOk = false;
		g_gdiplusToken = 0;
	}
}
static void PlayHitSound(const char* relativePath) {
	if (!SkeetoFeaturesOn()) return;
	if (!g_optSound) return;
	if (g_optSfxVol <= 0) return;
	if (!relativePath || !relativePath[0]) return;

	const char* sample = NormSoundSample(relativePath);
	if (!sample[0]) return;

	const float vol = (float)g_optSfxVol / 100.f;

	// EmitSound (SFX bus, ATTN_NONE) ≈ old console "play" loudness + volume knob.
	// (EmitAmbientSound was ducked/quiet in L4D2 combat.)
	if (EngEmitLocalSfx(sample, vol))
		return;

	UnlockOverlayCommand();
	char cmd[176]{};
	snprintf(cmd, sizeof(cmd), "playvol %s %.2f", sample, vol);
	EngClientCmd(cmd);
}
static void* EntGet(int i) {
	using Fn = void*(__thiscall*)(void*, int);
	return ((Fn)VGet(g_entlist, 3))(g_entlist, i);
}
static void* EntNetworkable(int i) {
	using Fn = void*(__thiscall*)(void*, int);
	return ((Fn)VGet(g_entlist, 0))(g_entlist, i);
}
static int EntHighestIndex() {
	using Fn = int(__thiscall*)(void*);
	return ((Fn)VGet(g_entlist, 6))(g_entlist);
}

// Forward decls — defined later
static uintptr_t FindPat(const char* module, const char* pattern);
static void FeedbackCi(const char* kind, bool throttle, bool meleeKill, bool ignoreHitModeGate = false,
	float wx = 0.f, float wy = 0.f, float wz = 0.f);
static void FeedbackSiHit(float wx = 0.f, float wy = 0.f, float wz = 0.f);
static void FeedbackFf(int dmg, float wx = 0.f, float wy = 0.f, float wz = 0.f);
static void ClearHitOverlay();
static bool CiChannelOn();
static bool EntReadable(void* e);
static const char* EntNetClassName(int i);
static bool IsCommonInfectedClass(const char* name);
static bool LocalEyePos(float* outEye);
static void AngleVectors(float pitch, float yaw, float* forward);

static bool EngPlayerInfo(int ent, PlayerInfo* out) {
	using Fn = bool(__thiscall*)(void*, int, PlayerInfo*);
	return ((Fn)VGet(g_engine, 8))(g_engine, ent, out);
}

static void RefreshLocalUserId() {
	DWORD now = GetTickCount();
	// EngPlayerInfo is expensive — shotgun fires 8–10 bullet_impact/hurt per shot.
	if (g_localUserId > 0 && g_lastLocalUidRefreshAt && now - g_lastLocalUidRefreshAt < 1000)
		return;
	g_lastLocalUidRefreshAt = now;
	int local = EngLocal();
	if (local <= 0) return;
	PlayerInfo pi{};
	if (EngPlayerInfo(local, &pi) && pi.userid > 0) {
		if (pi.userid != g_localUserId) {
			Log("localUserId %d -> %d (ent=%d name=%.32s)", g_localUserId, pi.userid, local, pi.name);
			g_localUserId = pi.userid;
		}
	}
}

static uintptr_t FindPat(const char* module, const char* pattern) {
	HMODULE h = GetModuleHandleA(module);
	if (!h) return 0;
	auto dos = (PIMAGE_DOS_HEADER)h;
	auto nt = (PIMAGE_NT_HEADERS)((uint8_t*)h + dos->e_lfanew);
	auto base = (uintptr_t)h + nt->OptionalHeader.BaseOfCode;
	auto size = nt->OptionalHeader.SizeOfCode;

	uint8_t bytes[128]; bool wild[128]; int n = 0;
	for (const char* p = pattern; *p && n < 128; ) {
		while (*p == ' ') ++p;
		if (!*p) break;
		if (*p == '?') { bytes[n] = 0; wild[n] = true; p += (p[1] == '?' ? 2 : 1); }
		else { unsigned v = 0; sscanf(p, "%02x", &v); bytes[n] = (uint8_t)v; wild[n] = false; p += 2; }
		++n;
	}
	for (size_t i = 0; i + (size_t)n <= size; ++i) {
		bool ok = true;
		for (int j = 0; j < n; ++j)
			if (!wild[j] && *(uint8_t*)(base + i + j) != bytes[j]) { ok = false; break; }
		if (ok) return base + i;
	}
	return 0;
}

static int CountPat(const char* module, const char* pattern, uintptr_t* out, int maxOut) {
	HMODULE h = GetModuleHandleA(module);
	if (!h) return 0;
	auto dos = (PIMAGE_DOS_HEADER)h;
	auto nt = (PIMAGE_NT_HEADERS)((uint8_t*)h + dos->e_lfanew);
	auto base = (uintptr_t)h + nt->OptionalHeader.BaseOfCode;
	auto size = nt->OptionalHeader.SizeOfCode;
	uint8_t bytes[128]; bool wild[128]; int n = 0;
	for (const char* p = pattern; *p && n < 128; ) {
		while (*p == ' ') ++p;
		if (!*p) break;
		if (*p == '?') { bytes[n] = 0; wild[n] = true; p += (p[1] == '?' ? 2 : 1); }
		else { unsigned v = 0; sscanf(p, "%02x", &v); bytes[n] = (uint8_t)v; wild[n] = false; p += 2; }
		++n;
	}
	int hits = 0;
	for (size_t i = 0; i + (size_t)n <= size; ++i) {
		bool ok = true;
		for (int j = 0; j < n; ++j)
			if (!wild[j] && *(uint8_t*)(base + i + j) != bytes[j]) { ok = false; break; }
		if (!ok) continue;
		if (hits < maxOut) out[hits] = base + i;
		++hits;
	}
	return hits;
}

static int FindOffset(RecvTable* table, const char* propName) {
	if (!table || !table->props) return -1;
	auto* props = (uint8_t*)table->props;
	for (int i = 0; i < table->nProps; ++i) {
		uint8_t* prop = props + i * 0x3C;
		const char* vn = *(const char**)prop;
		RecvTable* child = *(RecvTable**)(prop + 0x28);
		int off = *(int*)(prop + 0x2C);
		if (vn && !strcmp(vn, propName)) return off;
		if (child) {
			int c = FindOffset(child, propName);
			if (c >= 0) return off + c;
		}
	}
	return -1;
}

// RecvProp layout (MSVC, size 0x3C): m_pVarName@0, m_pDataTable@0x28, m_Offset@0x2C, m_ProxyFn@0x20
static uint8_t* FindRecvProp(RecvTable* table, const char* propName) {
	if (!table || !table->props) return nullptr;
	auto* props = (uint8_t*)table->props;
	for (int i = 0; i < table->nProps; ++i) {
		uint8_t* prop = props + i * 0x3C;
		const char* vn = *(const char**)prop;
		RecvTable* child = *(RecvTable**)(prop + 0x28);
		if (vn && !strcmp(vn, propName)) return prop;
		if (child) {
			uint8_t* c = FindRecvProp(child, propName);
			if (c) return c;
		}
	}
	return nullptr;
}

// CRecvProxyData: m_Value.m_Int @+4, m_ObjectID @+24
struct RecvProxyDataPod {
	const void* recvProp;
	int valueInt;
	int pad0[2];
	int valueType;
	int element;
	int objectId;
};

static void OnSiHealthDecreased(void* ent, int objectId, int oldHp, int newHp, float maxDist, float minDot);
static void OnSurvivorHealthDecreased(void* ent, int objectId, int oldHp, int newHp, bool requireMeleeAim);

static void HookedHealthProxy(const void* pData, void* pStruct, void* pOut) {
	// Read old HP without VirtualQuery — this runs on every networked HP update.
	int oldHp = 0;
	if (pStruct && g_offHealth >= 0 && g_offHealth <= 0x4000)
		oldHp = *(int*)((uint8_t*)pStruct + g_offHealth);

	if (g_origHealthProxy)
		g_origHealthProxy(pData, pStruct, pOut);
	else if (pOut && pData)
		*(int*)pOut = ((const RecvProxyDataPod*)pData)->valueInt;

	if (!SkeetoFeaturesOn()) return;

	if (!pData || !pStruct) return;
	if (!g_ready.load()) return;
	if (!g_hitDetectArmed.load() && !g_optClientInfectedHp && !g_optClientDmgNum) return;
	// Hit 全关且友伤关：只转发原 proxy，不做开火窗口/瞄准等额外工作。
	if (!NeedHealthProxyWork() && !g_optClientInfectedHp && !g_optClientDmgNum) return;

	DWORD now = GetTickCount();
	const bool recentGun = g_lastLocalGunImpactAt && (now - g_lastLocalGunImpactAt < 180);
	const bool recentMelee = now <= g_meleeAttackUntil;
	// Most HP traffic is not from local fire — bail before any aim/team work.
	if (!recentGun && !recentMelee) return;
	if (!g_engine || !EngInGame()) return;

	int newHp = ((const RecvProxyDataPod*)pData)->valueInt;
	int objectId = ((const RecvProxyDataPod*)pData)->objectId;
	if (newHp >= oldHp || oldHp <= 0 || newHp < 0) return;
	if (newHp > 100000 || oldHp > 100000) return;
	if (objectId <= 0 || objectId > 64) return;

	const int team = (g_offTeam >= 0 && g_offTeam <= 0x4000)
		? *(int*)((uint8_t*)pStruct + g_offTeam) : -1;

	if (g_optFf && team == 2) {
		int local = EngLocal();
		if (local > 0 && objectId != local) {
			OnSurvivorHealthDecreased(pStruct, objectId, oldHp, newHp, !recentGun);
			return;
		}
	}

	if (newHp >= 0 && team == 3 && (HitModeSiAllowed() || g_optClientInfectedHp || g_optClientDmgNum)) {
		OnSiHealthDecreased(pStruct, objectId, oldHp, newHp,
			recentGun ? 5000.f : 110.f,
			recentGun ? 0.93f : 0.90f);
	}
}

static bool HookHealthProp(RecvTable* table, const char* className) {
	uint8_t* prop = FindRecvProp(table, "m_iHealth");
	if (!prop) return false;
	auto* pProxy = (RecvVarProxyFn*)(prop + 0x20);
	if (!IsExec((void*)*pProxy)) return false;
	if (*pProxy == &HookedHealthProxy) return true;
	if (!g_origHealthProxy)
		g_origHealthProxy = *pProxy;
	*pProxy = &HookedHealthProxy;
	++g_healthProxyHooks;
	Log("hooked m_iHealth proxy on %s", className ? className : "?");
	return true;
}

static void ResolveNetvars() {
	void* bc = GetIface("client.dll", "VClient016");
	if (!bc) return;
	using Fn = ClientClass*(__thiscall*)(void*);
	g_healthProxyHooks = 0;
	g_origHealthProxy = nullptr;
	for (auto* cc = ((Fn)VGet(bc, 7))(bc); cc; cc = cc->next) {
		if (!cc->name || !cc->table) continue;
		if (g_offClip1 < 0) {
			int c = FindOffset(cc->table, "m_iClip1");
			if (c >= 0 && c <= 0x1000) g_offClip1 = c;
		}
		if (g_offAmmoType < 0) {
			int c = FindOffset(cc->table, "m_iPrimaryAmmoType");
			if (c >= 0 && c <= 0x1000) g_offAmmoType = c;
		}
		if (!strcmp(cc->name, "CBasePlayer") || !strcmp(cc->name, "CTerrorPlayer")) {
			if (g_offFlags < 0) g_offFlags = FindOffset(cc->table, "m_fFlags");
			if (g_offOrigin < 0) g_offOrigin = FindOffset(cc->table, "m_vecOrigin");
			if (g_offViewOffset < 0) g_offViewOffset = FindOffset(cc->table, "m_vecViewOffset[0]");
			if (!strcmp(cc->name, "CTerrorPlayer") && g_offZombieClass < 0)
				g_offZombieClass = FindOffset(cc->table, "m_zombieClass");
			if (!strcmp(cc->name, "CTerrorPlayer") && g_offIsIncapacitated < 0)
				g_offIsIncapacitated = FindOffset(cc->table, "m_isIncapacitated");
			if (!strcmp(cc->name, "CTerrorPlayer") && g_offActiveWeapon < 0)
				g_offActiveWeapon = FindOffset(cc->table, "m_hActiveWeapon");
			if (g_offAbsVelocity < 0)
				g_offAbsVelocity = FindOffset(cc->table, "m_vecVelocity[0]");
			if (g_offHideHUD < 0)
				g_offHideHUD = FindOffset(cc->table, "m_iHideHUD");
			if (g_offSurvivorCharacter < 0)
				g_offSurvivorCharacter = FindOffset(cc->table, "m_survivorCharacter");
			if (g_offMaxHealth < 0)
				g_offMaxHealth = FindOffset(cc->table, "m_iMaxHealth");
			if (g_offMyWeapons < 0) {
				g_offMyWeapons = FindOffset(cc->table, "m_hMyWeapons[0]");
				if (g_offMyWeapons < 0)
					g_offMyWeapons = FindOffset(cc->table, "m_hMyWeapons");
			}
			if (g_offAmmo < 0) {
				g_offAmmo = FindOffset(cc->table, "m_iAmmo[0]");
				if (g_offAmmo < 0)
					g_offAmmo = FindOffset(cc->table, "m_iAmmo");
			}
			if (g_offHanging < 0)
				g_offHanging = FindOffset(cc->table, "m_isHangingFromLedge");
			if (g_offReviveCount < 0)
				g_offReviveCount = FindOffset(cc->table, "m_currentReviveCount");
			if (g_offThirdStrike < 0)
				g_offThirdStrike = FindOffset(cc->table, "m_bIsOnThirdStrike");
			if (g_offHealthBuffer < 0)
				g_offHealthBuffer = FindOffset(cc->table, "m_healthBuffer");
			// ONLY hook player HP — never Infected/Witch (shotgun would storm this proxy).
			HookHealthProp(cc->table, cc->name);
			if (g_offHealth < 0) g_offHealth = FindOffset(cc->table, "m_iHealth");
			if (g_offLifeState < 0) g_offLifeState = FindOffset(cc->table, "m_lifeState");
		}
		if (!strcmp(cc->name, "CBaseEntity") || !strcmp(cc->name, "CBasePlayer") || !strcmp(cc->name, "CTerrorPlayer")) {
			if (g_offTeam < 0) g_offTeam = FindOffset(cc->table, "m_iTeamNum");
			if (g_offHealth < 0) g_offHealth = FindOffset(cc->table, "m_iHealth");
			if (g_offLifeState < 0) g_offLifeState = FindOffset(cc->table, "m_lifeState");
			if (g_offAbsVelocity < 0) g_offAbsVelocity = FindOffset(cc->table, "m_vecVelocity[0]");
			if (g_offEffects < 0) g_offEffects = FindOffset(cc->table, "m_fEffects");
			if (g_offRenderMode < 0) g_offRenderMode = FindOffset(cc->table, "m_nRenderMode");
			if (g_offClrRender < 0) g_offClrRender = FindOffset(cc->table, "m_clrRender");
			if (g_offOwnerEntity < 0) g_offOwnerEntity = FindOffset(cc->table, "m_hOwnerEntity");
			if (g_offModelIndex < 0) g_offModelIndex = FindOffset(cc->table, "m_nModelIndex");
		}
		if (!strcmp(cc->name, "CTerrorWeapon") || !strcmp(cc->name, "CBaseCombatWeapon")
			|| !strcmp(cc->name, "CWeaponCSBase") || !strcmp(cc->name, "CPistol")) {
			if (g_offWeaponOwner < 0) g_offWeaponOwner = FindOffset(cc->table, "m_hOwner");
			if (g_offClip1 < 0) g_offClip1 = FindOffset(cc->table, "m_iClip1");
			if (g_offAmmoType < 0) g_offAmmoType = FindOffset(cc->table, "m_iPrimaryAmmoType");
		}
		if (!strcmp(cc->name, "CWeaponSpawn")) {
			if (g_offWeaponID < 0) g_offWeaponID = FindOffset(cc->table, "m_weaponID");
		}
		if (!strcmp(cc->name, "Infected") || !strcmp(cc->name, "Witch")) {
			int hpOff = FindOffset(cc->table, "m_iHealth");
			int orgOff = FindOffset(cc->table, "m_vecOrigin");
			int lifeOff = FindOffset(cc->table, "m_lifeState");
			Log("class %s m_iHealth off=%d m_vecOrigin off=%d m_lifeState off=%d hasProxyProp=%d",
				cc->name, hpOff, orgOff, lifeOff, FindRecvProp(cc->table, "m_iHealth") ? 1 : 0);
			if (orgOff >= 0 && orgOff <= 0x4000 && g_offOriginCommon < 0)
				g_offOriginCommon = orgOff;
			if (hpOff >= 0 && hpOff <= 0x4000 && g_offHealthCommon < 0)
				g_offHealthCommon = hpOff;
			// Always prefer Infected's own lifeState — player offset is wrong on CI and blocked all hits.
			if (lifeOff >= 0 && lifeOff <= 0x4000)
				g_offLifeStateCommon = lifeOff;
		}
	}
	Log("health proxy hooks total=%d lifeState=%d lifeStateCI=%d hpCommon=%d",
		g_healthProxyHooks, g_offLifeState, g_offLifeStateCommon, g_offHealthCommon);
}

// Corpse filter for gun-hit. Never apply player m_lifeState to Infected (wrong offset → no CI hits).
static bool EntReadable(void* e) {
	if (!e) return false;
	MEMORY_BASIC_INFORMATION mbi{};
	return VirtualQuery(e, &mbi, sizeof(mbi)) && mbi.State == MEM_COMMIT
		&& !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD));
}

static const char* EntNetClassName(int i) {
	void* net = EntNetworkable(i);
	if (!net) return nullptr;
	using Fn = ClientClass*(__thiscall*)(void*);
	ClientClass* cc = ((Fn)VGet(net, 1))(net);
	return (cc && cc->name) ? cc->name : nullptr;
}

static bool IsCommonInfectedClass(const char* name) {
	if (!name || !name[0]) return false;
	if (!_stricmp(name, "Infected") || !_stricmp(name, "Witch"))
		return true;
	if (!_stricmp(name, "C_Infected") || !_stricmp(name, "C_Witch"))
		return true;
	return false;
}

static bool IsWitchClass(const char* name) {
	if (!name || !name[0]) return false;
	return !_stricmp(name, "Witch") || !_stricmp(name, "C_Witch");
}

static bool LocalEyePos(float* outEye) {
	if (!outEye || !g_entlist || g_offOrigin < 0 || g_offOrigin > 0x4000) return false;
	int local = EngLocal();
	if (local <= 0) return false;
	void* me = EntGet(local);
	if (!EntReadable(me)) return false;
	float* origin = (float*)((uint8_t*)me + g_offOrigin);
	outEye[0] = origin[0]; outEye[1] = origin[1]; outEye[2] = origin[2];
	if (g_offViewOffset >= 0 && g_offViewOffset <= 0x4000) {
		float* vo = (float*)((uint8_t*)me + g_offViewOffset);
		outEye[0] += vo[0]; outEye[1] += vo[1]; outEye[2] += vo[2];
	} else {
		outEye[2] += 62.f;
	}
	return true;
}

// Gun hit:
//   SI → m_iHealth proxy (recv thread; still a residual datacache risk — see tracker).
//   CI → local bullet_impact stamp → EngineVGui::Paint: one MASK_SHOT TraceRay then FX.
// Do not TraceRay/Dispatch from FireGameEvent, FSN, or the Sleep worker.
static void QueueHitFeedback() {
	DWORD now = GetTickCount();
	if (now < g_suppressCommonHitUntil)
		return;
	g_pendingCommonHit = true;
	g_pendingCommonHitAt = now + 45;
}

static void FlushPendingCommonHit() {
	if (!g_pendingCommonHit) return;
	DWORD now = GetTickCount();
	if (now < g_pendingCommonHitAt) return;
	g_pendingCommonHit = false;
	if (now < g_suppressCommonHitUntil) return;
	if (!HitModeCiAllowed()) return;
	if (g_haveImpactPos)
		FeedbackCi("hit", true, false, false, g_lastImpactPos[0], g_lastImpactPos[1], g_lastImpactPos[2]);
	else
		FeedbackCi("hit", true, false);
}

// Source CONTENTS_* bits used by MASK_SHOT (const.h / Necola SDK).
enum : unsigned {
	kContentsSolid = 0x1,
	kContentsWindow = 0x2,
	kContentsMoveable = 0x4000,
	kContentsMonster = 0x2000000,
	kContentsDebris = 0x4000000,
	kContentsHitbox = 0x40000000,
	kMaskShot = kContentsSolid | kContentsMoveable | kContentsMonster
		| kContentsWindow | kContentsDebris | kContentsHitbox,
};

struct alignas(16) TraceVec3A {
	float x, y, z, w;
};

struct TraceRayPod {
	TraceVec3A start;
	TraceVec3A delta;
	TraceVec3A startOffset;
	TraceVec3A extents;
	const void* worldAxisTransform;
	bool isRay;
	bool isSwept;
};

struct TraceResultPod {
	float startpos[3];
	float endpos[3];
	float planeNormal[3];
	float planeDist;
	unsigned char planeType;
	unsigned char planeSignbits;
	unsigned char planePad[2];
	float fraction;
	int contents;
	unsigned short dispFlags;
	bool allsolid;
	bool startsolid;
	float fractionleftsolid;
	const char* surfaceName;
	short surfaceProps;
	unsigned short surfaceFlags;
	int hitgroup;
	short physicsbone;
	unsigned short worldSurfaceIndex;
	void* ent;
	int hitbox;
};

struct TraceFilterSkip {
	virtual bool ShouldHitEntity(void* ent, int) {
		return ent != skip;
	}
	virtual int GetTraceType() const {
		return 0; // TRACE_EVERYTHING
	}
	void* skip;
};

struct TraceFilterSkip2 {
	virtual bool ShouldHitEntity(void* ent, int) {
		return ent && ent != skip0 && ent != skip1;
	}
	virtual int GetTraceType() const {
		return 0;
	}
	void* skip0;
	void* skip1;
};

// Throw preview: world brushes only — never touch entity collision (safer on listen host).
struct TraceFilterWorldOnly {
	virtual bool ShouldHitEntity(void*, int) {
		return false;
	}
	virtual int GetTraceType() const {
		return 1; // TRACE_WORLD_ONLY
	}
};

static bool EngTraceLine(const float* start, const float* end, unsigned mask, void* skip, TraceResultPod* out) {
	if (!g_trace || !start || !end || !out) return false;
	TraceRayPod ray{};
	ray.delta.x = end[0] - start[0];
	ray.delta.y = end[1] - start[1];
	ray.delta.z = end[2] - start[2];
	ray.delta.w = 0.f;
	ray.startOffset.x = ray.startOffset.y = ray.startOffset.z = ray.startOffset.w = 0.f;
	ray.extents.x = ray.extents.y = ray.extents.z = ray.extents.w = 0.f;
	ray.isSwept = (ray.delta.x || ray.delta.y || ray.delta.z);
	ray.isRay = true;
	ray.worldAxisTransform = nullptr;
	ray.start.x = start[0]; ray.start.y = start[1]; ray.start.z = start[2]; ray.start.w = 0.f;
	TraceFilterSkip filter{};
	filter.skip = skip;
	using TraceFn = void(__thiscall*)(void*, const TraceRayPod*, unsigned int, TraceFilterSkip*, void*);
	auto fn = (TraceFn)VGet(g_trace, 5);
	if (!fn || !IsExec((void*)fn)) return false;
	// Oversized out-buffer: undersized CGameTrace poisons the engine on listen hosts.
	alignas(16) unsigned char raw[512]{};
	fn(g_trace, &ray, mask, &filter, raw);
	memcpy(out, raw, sizeof(*out));
	return true;
}

static bool EngTraceLineSkip2(const float* start, const float* end, unsigned mask,
	void* skip0, void* skip1, TraceResultPod* out) {
	if (!g_trace || !start || !end || !out) return false;
	TraceRayPod ray{};
	ray.delta.x = end[0] - start[0];
	ray.delta.y = end[1] - start[1];
	ray.delta.z = end[2] - start[2];
	ray.delta.w = 0.f;
	ray.startOffset.x = ray.startOffset.y = ray.startOffset.z = ray.startOffset.w = 0.f;
	ray.extents.x = ray.extents.y = ray.extents.z = ray.extents.w = 0.f;
	ray.isSwept = (ray.delta.x || ray.delta.y || ray.delta.z);
	ray.isRay = true;
	ray.worldAxisTransform = nullptr;
	ray.start.x = start[0]; ray.start.y = start[1]; ray.start.z = start[2]; ray.start.w = 0.f;
	TraceFilterSkip2 filter{};
	filter.skip0 = skip0;
	filter.skip1 = skip1;
	using TraceFn = void(__thiscall*)(void*, const TraceRayPod*, unsigned int, TraceFilterSkip2*, void*);
	auto fn = (TraceFn)VGet(g_trace, 5);
	if (!fn || !IsExec((void*)fn)) return false;
	alignas(16) unsigned char raw[512]{};
	fn(g_trace, &ray, mask, &filter, raw);
	memcpy(out, raw, sizeof(*out));
	return true;
}

static bool EngTraceLineWorld(const float* start, const float* end, unsigned mask, TraceResultPod* out) {
	if (!g_trace || !start || !end || !out) return false;
	TraceRayPod ray{};
	ray.delta.x = end[0] - start[0];
	ray.delta.y = end[1] - start[1];
	ray.delta.z = end[2] - start[2];
	ray.delta.w = 0.f;
	ray.startOffset.x = ray.startOffset.y = ray.startOffset.z = ray.startOffset.w = 0.f;
	ray.extents.x = ray.extents.y = ray.extents.z = ray.extents.w = 0.f;
	ray.isSwept = (ray.delta.x || ray.delta.y || ray.delta.z);
	ray.isRay = true;
	ray.worldAxisTransform = nullptr;
	ray.start.x = start[0]; ray.start.y = start[1]; ray.start.z = start[2]; ray.start.w = 0.f;
	TraceFilterWorldOnly filter{};
	using TraceFn = void(__thiscall*)(void*, const TraceRayPod*, unsigned int, TraceFilterWorldOnly*, void*);
	auto fn = (TraceFn)VGet(g_trace, 5);
	if (!fn || !IsExec((void*)fn)) return false;
	alignas(16) unsigned char raw[512]{};
	fn(g_trace, &ray, mask, &filter, raw);
	memcpy(out, raw, sizeof(*out));
	return true;
}

static int EntIndexOf(void* ent) {
	if (!ent || !g_entlist) return -1;
	for (int i = 1; i <= 64; ++i) {
		if (EntGet(i) == ent) return i;
	}
	int hi = EntHighestIndex();
	if (hi > 2048) hi = 2048;
	for (int i = 65; i <= hi; ++i) {
		if (EntGet(i) == ent) return i;
	}
	return -1;
}

static bool TryCiHitTrace(const float* impact) {
	if (!impact || !g_trace || !g_entlist || !g_engine || !EngInGame())
		return false;
	if (!HitModeCiAllowed())
		return false;

	DWORD now = GetTickCount();
	if (now - g_lastCiTraceAt < kCiTraceMinIntervalMs)
		return false;
	g_lastCiTraceAt = now;

	float eye[3]{};
	if (!LocalEyePos(eye)) return false;

	float dx = impact[0] - eye[0], dy = impact[1] - eye[1], dz = impact[2] - eye[2];
	float len2 = dx * dx + dy * dy + dz * dz;
	if (len2 < 1.f) return false;
	float inv = 2.f / sqrtf(len2);
	float endx = impact[0] + dx * inv, endy = impact[1] + dy * inv, endz = impact[2] + dz * inv;

	float end[3] = { endx, endy, endz };
	int local = EngLocal();
	void* skip = (local > 0) ? EntGet(local) : nullptr;
	TraceResultPod tr{};
	if (!EngTraceLine(eye, end, kMaskShot, skip, &tr))
		return false;

	if (!tr.ent) return false;
	int idx = EntIndexOf(tr.ent);
	if (idx <= 0) return false;

	const char* cn = EntNetClassName(idx);
	if (!IsCommonInfectedClass(cn))
		return false;

	if (g_commonHitLogLeft > 0) {
		--g_commonHitLogLeft;
		Log("common-hit ent=%d hg=%d (trace)", idx, tr.hitgroup);
	}
	return true;
}

// =============================================================================
// Client UX — throwable landing preview (molotov / pipe / bile).
// Pure client draw. Throttled TraceRay steps; caches while aim barely changes.
// =============================================================================
enum : unsigned {
	kContentsGrate = 0x8,
	// World geometry only — skip players/CI hitboxes (cheaper + better "floor" hit).
	kMaskThrowLand = kContentsSolid | kContentsMoveable | kContentsWindow | kContentsGrate,
};

static int ClientUxLocalThrowableKind() {
	// 0=none 1=molotov/bile (impact) 2=pipe (bounces)
	// Cache by weapon handle — never EntIndexOf (that scans the whole entlist every paint).
	static int s_lastHandle = 0;
	static int s_lastKind = 0;
	if (g_offActiveWeapon < 0 || g_offActiveWeapon > 0x4000) return 0;
	int local = EngLocal();
	if (local <= 0) return 0;
	void* me = EntGet(local);
	if (!me) return 0;
	int h = *(int*)((uint8_t*)me + g_offActiveWeapon);
	if (h == s_lastHandle) return s_lastKind;
	s_lastHandle = h;
	s_lastKind = 0;
	if (h == -1 || h == 0) return 0;
	int idx = h & 0xFFF;
	if (idx <= 0) idx = h & 0x7FF;
	if (idx <= 0) return 0;
	void* wep = EntGet(idx);
	if (!wep) return 0;
	const char* cn = EntNetClassName(idx);
	if (!cn || !cn[0]) return 0;
	if (strstr(cn, "Molotov") || strstr(cn, "VomitJar") || strstr(cn, "Vomitjar"))
		s_lastKind = 1;
	else if (strstr(cn, "PipeBomb") || strstr(cn, "Pipebomb"))
		s_lastKind = 2;
	return s_lastKind;
}

static float ClientUxSvGravity() {
	void* cv = CvarFind("sv_gravity");
	if (!cv) return 800.f;
	int n = 0; float f = 0.f;
	if (!CvarReadPair(cv, &n, &f)) return 800.f;
	if (f > 1.f && f < 4000.f) return f;
	if (n > 1 && n < 4000) return (float)n;
	return 800.f;
}

static constexpr int kThrowPathMax = 28;
static constexpr DWORD kThrowLingerMs = 4000;
// CS/L4D grenade projectiles use MOVETYPE_FLYGRAVITY with ~0.4 scale — full sv_gravity
// makes predicted arcs drop too early (land marker too close).
static constexpr float kThrowGravityScale = 0.4f;
// weapon_fire is unreliable for L4D2 throwables; kept as a weak backup pulse only.
static std::atomic_bool g_throwLockPulse{false};

static void ClientUxOnLocalThrowableFire(void* ev) {
	if (!g_optClientThrowLand || !ev) return;
	RefreshLocalUserId();
	const int uid = EvInt(ev, "userid");
	if (g_localUserId > 0 && uid > 0 && uid != g_localUserId) return;
	const char* wpn = EvStr(ev, "weapon");
	if (!wpn || !wpn[0]) return;
	if (!strstr(wpn, "molotov") && !strstr(wpn, "pipe_bomb") && !strstr(wpn, "pipebomb")
		&& !strstr(wpn, "vomitjar") && !strstr(wpn, "bile"))
		return;
	g_throwLockPulse.store(true, std::memory_order_relaxed);
}

static bool ClientUxPredictThrowLand(int kind, float* outLand, float path[][3], int* pathN) {
	if (!outLand || kind < 1) return false;
	if (pathN) *pathN = 0;
	float eye[3]{};
	if (!LocalEyePos(eye)) return false;
	float ang[3]{};
	EngGetViewAngles(ang);

	// CBaseCSGrenade-style launch (L4D2 throwables inherit this path):
	// remap pitch, speed = (90 - throwPitch) * 6 capped at 750, then + player velocity.
	float throwPitch = ang[0];
	if (throwPitch < 0.f)
		throwPitch = -10.f + throwPitch * ((90.f - 10.f) / 90.f);
	else
		throwPitch = -10.f + throwPitch * ((90.f + 10.f) / 90.f);
	float flVel = (90.f - throwPitch) * 6.f;
	if (flVel > 750.f) flVel = 750.f;
	if (flVel < 50.f) flVel = 50.f;

	float forward[3]{};
	AngleVectors(throwPitch, ang[1], forward);

	float pos[3] = {
		eye[0] + forward[0] * 16.f,
		eye[1] + forward[1] * 16.f,
		eye[2] + forward[2] * 16.f
	};
	float vel[3] = {
		forward[0] * flVel,
		forward[1] * flVel,
		forward[2] * flVel
	};
	int local = EngLocal();
	void* me = (local > 0) ? EntGet(local) : nullptr;
	if (me && EntReadable(me) && g_offAbsVelocity >= 0 && g_offAbsVelocity <= 0x4000) {
		float* pv = (float*)((uint8_t*)me + g_offAbsVelocity);
		if (PtrCommitted(pv)) {
			const float sp2 = pv[0] * pv[0] + pv[1] * pv[1] + pv[2] * pv[2];
			if (sp2 < 450.f * 450.f) {
				vel[0] += pv[0];
				vel[1] += pv[1];
				vel[2] += pv[2];
			}
		}
	}

	auto pushPath = [&](const float* p) {
		if (!path || !pathN || *pathN >= kThrowPathMax) return;
		path[*pathN][0] = p[0];
		path[*pathN][1] = p[1];
		path[*pathN][2] = p[2];
		++(*pathN);
	};
	pushPath(pos);

	const float gravity = ClientUxSvGravity() * kThrowGravityScale;
	const float dt = 0.06f;
	const int maxSteps = 36;
	const int maxBounces = (kind == 2) ? 3 : 0;
	int bounces = 0;

	for (int step = 0; step < maxSteps; ++step) {
		vel[2] -= gravity * dt;
		float next[3] = {
			pos[0] + vel[0] * dt,
			pos[1] + vel[1] * dt,
			pos[2] + vel[2] * dt
		};

		TraceResultPod tr{};
		if (!EngTraceLineWorld(pos, next, kMaskThrowLand, &tr))
			return false;

		if (tr.fraction < 1.f || tr.startsolid || tr.allsolid) {
			outLand[0] = tr.endpos[0];
			outLand[1] = tr.endpos[1];
			outLand[2] = tr.endpos[2];
			pushPath(outLand);
			if (kind != 2 || bounces >= maxBounces)
				return true;
			float nx = tr.planeNormal[0], ny = tr.planeNormal[1], nz = tr.planeNormal[2];
			float nd = nx * nx + ny * ny + nz * nz;
			if (nd < 0.01f) return true;
			float inv = 1.f / sqrtf(nd);
			nx *= inv; ny *= inv; nz *= inv;
			float dot = vel[0] * nx + vel[1] * ny + vel[2] * nz;
			// Pipe bomb loses most energy on bounce; 0.55 made 2nd arc ~2× too long;
			// 0.28/0.18 then sat a bit short — nudge toward mid.
			float rest = (bounces == 0) ? 0.36f : 0.22f;
			vel[0] = (vel[0] - 2.f * dot * nx) * rest;
			vel[1] = (vel[1] - 2.f * dot * ny) * rest;
			vel[2] = (vel[2] - 2.f * dot * nz) * rest;
			pos[0] = tr.endpos[0] + nx * 2.f;
			pos[1] = tr.endpos[1] + ny * 2.f;
			pos[2] = tr.endpos[2] + nz * 2.f;
			++bounces;
			float sp2 = vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2];
			if (sp2 < 45.f * 45.f)
				return true;
			continue;
		}

		pos[0] = next[0]; pos[1] = next[1]; pos[2] = next[2];
		if ((step & 1) == 0)
			pushPath(pos);
	}
	outLand[0] = pos[0]; outLand[1] = pos[1]; outLand[2] = pos[2];
	pushPath(outLand);
	return true;
}

static void ClientUxDrawThrowOverlay(const float land[3], const float path[][3], int pathN,
	bool locked, int sw, int sh) {
	if (pathN >= 2) {
		float sx0 = 0.f, sy0 = 0.f;
		bool have0 = LocalPlayWorldToScreen(path[0], &sx0, &sy0, sw, sh);
		SurfColor(locked ? 120 : 90, locked ? 210 : 190, 255, locked ? 190 : 210);
		for (int i = 1; i < pathN; ++i) {
			float sx1 = 0.f, sy1 = 0.f;
			const bool have1 = LocalPlayWorldToScreen(path[i], &sx1, &sy1, sw, sh);
			if (have0 && have1)
				SurfLine((int)(sx0 + 0.5f), (int)(sy0 + 0.5f), (int)(sx1 + 0.5f), (int)(sy1 + 0.5f));
			sx0 = sx1; sy0 = sy1; have0 = have1;
		}
	}
	float sx = 0.f, sy = 0.f;
	if (!LocalPlayWorldToScreen(land, &sx, &sy, sw, sh)) return;
	const int cx = (int)(sx + 0.5f);
	const int cy = (int)(sy + 0.5f);
	if (cx < -20 || cy < -20 || cx > sw + 20 || cy > sh + 20) return;
	const int r = locked ? 9 : 7;
	SurfColor(0, 0, 0, 160);
	SurfFill(cx - r - 1, cy - 1, cx + r + 1, cy + 2);
	SurfFill(cx - 1, cy - r - 1, cx + 2, cy + r + 1);
	SurfColor(255, locked ? 180 : 220, locked ? 40 : 60, 230);
	SurfFill(cx - r, cy, cx + r, cy + 1);
	SurfFill(cx, cy - r, cx + 1, cy + r);
	SurfColor(255, locked ? 40 : 80, locked ? 40 : 60, 220);
	SurfFill(cx - 2, cy - 2, cx + 3, cy + 3);
}

// Enemy corpse clear: event-driven only (never scan the whole entlist each paint —
// that dropped 250fps→50). -lv death anims are NOT ragdolls; hide the dead ent
// itself. Ragdoll caps are a light assist when not using -lv.
static struct {
	bool have;
	int special;
	int generic;
	int boss;
	float lvFade;
} g_ragdollSnap = { false, 8, 8, 8, 0.5f };

static constexpr int kNoCorpseTrackMax = 64;
static int g_noCorpseEnt[kNoCorpseTrackMax]{};
static DWORD g_noCorpseUntil[kNoCorpseTrackMax]{};
static bool g_noCorpseIsSi[kNoCorpseTrackMax]{};
static int g_noCorpseN = 0;
static int g_noCorpseRagLeft = 0;
static DWORD g_noCorpseRagNextAt = 0;

static void ClientUxClearNoCorpseTracks() {
	g_noCorpseN = 0;
	g_noCorpseRagLeft = 0;
	g_noCorpseRagNextAt = 0;
	memset(g_noCorpseEnt, 0, sizeof(g_noCorpseEnt));
	memset(g_noCorpseUntil, 0, sizeof(g_noCorpseUntil));
	memset(g_noCorpseIsSi, 0, sizeof(g_noCorpseIsSi));
}

static void ClientUxUnlockRagdollCvars() {
	static bool s_done = false;
	if (s_done || !g_cvar) return;
	s_done = true;
	const int clear = kFCvarCheat | kFCvarHidden | kFCvarDevOnly;
	UnlockConCommand("cl_ragdoll_maxcount_special", clear, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_ragdoll_maxcount_generic", clear, kFCvarClientCmdCanExecute);
	UnlockConCommand("cl_ragdoll_maxcount_boss", clear, kFCvarClientCmdCanExecute);
	UnlockConCommand("g_ragdoll_lvfadespeed", clear, kFCvarClientCmdCanExecute);
}

static void ClientUxCvarWriteInt(const char* name, int v) {
	void* var = CvarFind(name);
	if (var && CvarPokeFloatValue(var, (float)v))
		return;
	CvarSetInt(name, v);
}

static void ClientUxCvarWriteFloat(const char* name, float v) {
	const int clear = kFCvarCheat | kFCvarDevOnly | kFCvarHidden;
	UnlockConCommand(name, clear, kFCvarClientCmdCanExecute);
	char cmd[64]{};
	snprintf(cmd, sizeof(cmd), "%s %.3f", name, v);
	EngClientCmd(cmd);
}

static void ClientUxRagdollSnapOnce() {
	if (g_ragdollSnap.have) return;
	g_ragdollSnap.special = CvarGetIntRange("cl_ragdoll_maxcount_special", 0, 64, 8);
	g_ragdollSnap.generic = CvarGetIntRange("cl_ragdoll_maxcount_generic", 0, 64, 8);
	g_ragdollSnap.boss = CvarGetIntRange("cl_ragdoll_maxcount_boss", 0, 64, 8);
	g_ragdollSnap.lvFade = 0.5f;
	g_ragdollSnap.have = true;
}

static void ClientUxApplyNoCorpseCvars(bool force) {
	(void)force;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_cvar) return;
	ClientUxUnlockRagdollCvars();
	ClientUxRagdollSnapOnce();

	if (g_optClientNoCorpseSi) {
		ClientUxCvarWriteInt("cl_ragdoll_maxcount_special", 0);
		ClientUxCvarWriteInt("cl_ragdoll_maxcount_boss", 0);
	} else if (g_ragdollSnap.have) {
		ClientUxCvarWriteInt("cl_ragdoll_maxcount_special", g_ragdollSnap.special);
		ClientUxCvarWriteInt("cl_ragdoll_maxcount_boss", g_ragdollSnap.boss);
	}

	if (g_optClientNoCorpseCi) {
		ClientUxCvarWriteInt("cl_ragdoll_maxcount_generic", 0);
		ClientUxCvarWriteFloat("g_ragdoll_lvfadespeed", 10000.f);
	} else if (g_ragdollSnap.have) {
		ClientUxCvarWriteInt("cl_ragdoll_maxcount_generic", g_ragdollSnap.generic);
		ClientUxCvarWriteFloat("g_ragdoll_lvfadespeed", g_ragdollSnap.lvFade);
	}
}

// Pin vanilla director_show_intensity. Chapter change / sv_cheats 0 restores CHEAT
// cvars; ESC menu edits die with the map. Same idea as the spread ring, but only
// written on menu toggle / LevelInit / round_start — never from Paint.
static void ClientUxApplyDirectorHud(bool force) {
	(void)force;
	if (!g_run.load(std::memory_order_relaxed) || !g_cvar) return;
	static bool s_pinned = false;
	static bool s_missingLogged = false;
	static bool s_onLogged = false;
	const bool on = g_optClientDirectorHud && SkeetoFeaturesOn();
	if (!on) {
		if (!s_pinned)
			return;
		void* var = CvarFind("director_show_intensity");
		if (var)
			CvarPokeFloatValue(var, 0.f);
		s_pinned = false;
		s_onLogged = false;
		return;
	}

	void* var = CvarFind("director_show_intensity");
	if (!var) {
		if (!s_missingLogged) {
			s_missingLogged = true;
			Log("director_show_intensity not found — director HUD skipped");
		}
		return;
	}

	const int flags = CvarFlagsOf(var);
	if (flags & (kFCvarCheat | kFCvarDevOnly | kFCvarHidden))
		CvarWriteFlags(var, flags & ~(kFCvarCheat | kFCvarDevOnly | kFCvarHidden));
	if (!CvarPokeFloatValue(var, 1.f))
		CvarSetInt("director_show_intensity", 1);
	s_pinned = true;
	if (!s_onLogged) {
		s_onLogged = true;
		Log("director HUD pinned director_show_intensity=1");
	}
}

static void ClientUxHideEntHard(void* e) {
	if (!e) return;
	// Draw flags only — do not move origin (that fought prediction and could not be undone).
	if (g_offEffects >= 0 && g_offEffects <= 0x4000)
		*(int*)((uint8_t*)e + g_offEffects) |= kEfNoDraw;
	if (g_offRenderMode >= 0 && g_offRenderMode <= 0x4000)
		*((uint8_t*)e + g_offRenderMode) = (uint8_t)kRenderNone;
	if (g_offClrRender >= 0 && g_offClrRender <= 0x4000)
		*(uint32_t*)((uint8_t*)e + g_offClrRender) = 0;
}

static bool ClientUxIsClientRagdollClass(const char* name) {
	if (!name || !name[0]) return false;
	if (!_stricmp(name, "C_ClientRagdoll") || !_stricmp(name, "C_ClientRagdoll2"))
		return true;
	if (!_stricmp(name, "client_ragdoll") || !_stricmp(name, "client_ragdoll2"))
		return true;
	return false;
}

static void ClientUxTrackCorpse(int ent, DWORD holdMs, bool si) {
	if (ent <= 0) return;
	if (si && !g_optClientNoCorpseSi) return;
	if (!si && !g_optClientNoCorpseCi) return;
	const DWORD until = GetTickCount() + holdMs;
	for (int i = 0; i < g_noCorpseN; ++i) {
		if (g_noCorpseEnt[i] == ent) {
			if (until > g_noCorpseUntil[i])
				g_noCorpseUntil[i] = until;
			g_noCorpseIsSi[i] = si;
			return;
		}
	}
	if (g_noCorpseN >= kNoCorpseTrackMax) {
		// Drop oldest slot
		memmove(g_noCorpseEnt, g_noCorpseEnt + 1, (kNoCorpseTrackMax - 1) * sizeof(int));
		memmove(g_noCorpseUntil, g_noCorpseUntil + 1, (kNoCorpseTrackMax - 1) * sizeof(DWORD));
		memmove(g_noCorpseIsSi, g_noCorpseIsSi + 1, (kNoCorpseTrackMax - 1) * sizeof(bool));
		g_noCorpseN = kNoCorpseTrackMax - 1;
	}
	g_noCorpseEnt[g_noCorpseN] = ent;
	g_noCorpseUntil[g_noCorpseN] = until;
	g_noCorpseIsSi[g_noCorpseN] = si;
	++g_noCorpseN;

	void* e = EntGet(ent);
	if (e) ClientUxHideEntHard(e);
	// Ragdolls spawn a tick later at high indices — 3 short pulses, not every Paint.
	g_noCorpseRagLeft = 3;
	g_noCorpseRagNextAt = GetTickCount();
}

static void ClientUxOnSiDeath(void* ev) {
	if (!g_optClientNoCorpseSi || !ev) return;
	const char* vn = EvStr(ev, "victimname");
	if (!vn || !vn[0] || !strcmp(vn, "Infected")) return;
	// Survivors also fire player_death — only infected team.
	int ve = EngPlayerForUserID(EvInt(ev, "userid"));
	if (ve <= 0)
		ve = EvInt(ev, "entityid");
	if (ve <= 0) return;
	void* e = EntGet(ve);
	if (EntReadable(e) && g_offTeam >= 0) {
		const int team = *(int*)((uint8_t*)e + g_offTeam);
		if (team != 3) return;
	}
	ClientUxTrackCorpse(ve, 5000, true);
}

static void ClientUxOnCiDeath(void* ev) {
	if (!g_optClientNoCorpseCi || !ev) return;
	int ent = EvInt(ev, "infected_id");
	if (ent <= 0)
		ent = EvInt(ev, "entityid");
	if (ent > 0)
		ClientUxTrackCorpse(ent, 5000, false);
}

static void ClientUxOnWitchDeath(void* ev) {
	if (!g_optClientNoCorpseCi || !ev) return;
	const int id = EvInt(ev, "witchid");
	if (id > 0)
		ClientUxTrackCorpse(id, 5000, false);
}

static void ClientUxPaintNoCorpse() {
	if (!g_optClientNoCorpseSi && !g_optClientNoCorpseCi) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || !g_entlist) return;

	const DWORD now = GetTickCount();

	// Re-apply hide only on tracked corpses (tiny list). Net updates fight us each tick.
	int w = 0;
	for (int i = 0; i < g_noCorpseN; ++i) {
		if (now > g_noCorpseUntil[i]) continue;
		const bool si = g_noCorpseIsSi[i];
		if (si && !g_optClientNoCorpseSi) continue;
		if (!si && !g_optClientNoCorpseCi) continue;
		const int ent = g_noCorpseEnt[i];
		void* e = EntGet(ent);
		if (e) ClientUxHideEntHard(e);
		g_noCorpseEnt[w] = ent;
		g_noCorpseUntil[w] = g_noCorpseUntil[i];
		g_noCorpseIsSi[w] = si;
		++w;
	}
	g_noCorpseN = w;

	// After a death: at most 3 pulses, newest 48 slots. Hide C_ClientRagdoll in
	// that window only (bounded, not a full entlist scan).
	if (g_noCorpseRagLeft > 0 && now >= g_noCorpseRagNextAt) {
		--g_noCorpseRagLeft;
		g_noCorpseRagNextAt = now + 90;
		int hi = EntHighestIndex();
		if (hi > 0) {
			const int lo = (hi > 48) ? (hi - 48) : 1;
			const bool tagSi = g_optClientNoCorpseSi && !g_optClientNoCorpseCi;
			for (int i = lo; i <= hi; ++i) {
				const char* cn = EntNetClassName(i);
				if (!ClientUxIsClientRagdollClass(cn)) continue;
				void* e = EntGet(i);
				if (!e) continue;
				ClientUxHideEntHard(e);
				ClientUxTrackCorpse(i, 3000, tagSi);
			}
		}
	}
}

static void ClientUxPaintThrowLand() {
	if (!g_optClientThrowLand) return;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn()) return;
	if (!g_engine || !EngInGame() || EngDrawingLoading()) return;
	if (g_menuVisible && !g_menuParked) return;
	if (!g_trace || !g_entlist) return;

	// Live preview while idle-aiming. Lock on -attack using the last snapshot.
	// NEVER TraceRay on the throw frame and NEVER while +attack is held: listen-host
	// Paint shares the game thread with the server tick. A 36-step sim there causes
	// slow-mo throws, molotovs with no fire, pipe bombs that never leave, and
	// WASD/mouse-btn stalls (look still works via raw input). GetAsyncKeyState is
	// poll-only — it does not eat +attack/-attack.
	static DWORD s_lastSimAt = 0;
	static float s_land[3]{};
	static float s_path[kThrowPathMax][3]{};
	static int s_pathN = 0;
	static bool s_have = false;
	static int s_kindCached = 0;
	static int s_yawQ = 0, s_pitchQ = 0;
	static float s_simEye[3]{};
	static float s_lockLand[3]{};
	static float s_lockPath[kThrowPathMax][3]{};
	static int s_lockPathN = 0;
	static bool s_lockHave = false;
	static DWORD s_lockUntil = 0;
	static bool s_prevLmb = false;
	static bool s_heldThrow = false;

	const DWORD now = GetTickCount();
	if (g_throwLandReset.exchange(false, std::memory_order_relaxed)) {
		s_have = false;
		s_pathN = 0;
		s_kindCached = 0;
		s_lockHave = false;
		s_lockPathN = 0;
		s_lockUntil = 0;
		s_prevLmb = false;
		s_heldThrow = false;
		g_throwLockPulse.store(false, std::memory_order_relaxed);
	}

	const bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	const bool released = !lmb && s_prevLmb;
	s_prevLmb = lmb;

	float ang[3]{};
	EngGetViewAngles(ang);
	const int yawQ = (int)(ang[1] * 0.67f);
	const int pitchQ = (int)(ang[0] * 0.67f);
	const int kind = ClientUxLocalThrowableKind();

	auto walkShift = [&](float land[3], float path[][3], int n) {
		if (n < 1) return;
		float eye[3]{};
		if (!LocalEyePos(eye)) return;
		const float dx = eye[0] - s_simEye[0];
		const float dy = eye[1] - s_simEye[1];
		const float dz = eye[2] - s_simEye[2];
		if (dx * dx + dy * dy + dz * dz < 0.25f) return;
		land[0] += dx; land[1] += dy; land[2] += dz;
		for (int i = 0; i < n; ++i) {
			path[i][0] += dx;
			path[i][1] += dy;
			path[i][2] += dz;
		}
	};

	auto commitLockFromPreview = [&](const char* why) {
		if (!s_have || s_pathN < 1) return;
		if (s_lockHave && now <= s_lockUntil) return;
		s_lockLand[0] = s_land[0]; s_lockLand[1] = s_land[1]; s_lockLand[2] = s_land[2];
		s_lockPathN = s_pathN;
		for (int i = 0; i < s_pathN; ++i) {
			s_lockPath[i][0] = s_path[i][0];
			s_lockPath[i][1] = s_path[i][1];
			s_lockPath[i][2] = s_path[i][2];
		}
		walkShift(s_lockLand, s_lockPath, s_lockPathN);
		s_lockHave = true;
		s_lockUntil = now + kThrowLingerMs;
		Log("throw-lock (%s) pathN=%d", why ? why : "?", s_lockPathN);
	};

	auto refreshPreview = [&]() {
		if (kind < 1) return;
		const bool aimMoved = (yawQ != s_yawQ) || (pitchQ != s_pitchQ) || (kind != s_kindCached);
		const DWORD gap = aimMoved ? 80u : 125u;
		if (s_have && now - s_lastSimAt < gap)
			return;
		float land[3]{};
		float path[kThrowPathMax][3]{};
		int pathN = 0;
		if (!ClientUxPredictThrowLand(kind, land, path, &pathN))
			return;
		s_land[0] = land[0]; s_land[1] = land[1]; s_land[2] = land[2];
		s_pathN = pathN;
		for (int i = 0; i < pathN; ++i) {
			s_path[i][0] = path[i][0];
			s_path[i][1] = path[i][1];
			s_path[i][2] = path[i][2];
		}
		s_have = true;
		s_kindCached = kind;
		s_yawQ = yawQ;
		s_pitchQ = pitchQ;
		s_lastSimAt = now;
		if (!LocalEyePos(s_simEye)) {
			s_simEye[0] = s_simEye[1] = s_simEye[2] = 0.f;
		}
	};

	if (s_lockHave && now > s_lockUntil) {
		s_lockHave = false;
		s_lockPathN = 0;
	}

	if (kind >= 1 && lmb)
		s_heldThrow = true;

	// Pin-pull / throw frame: snapshot only. No TraceRay.
	if (released) {
		if (s_heldThrow)
			commitLockFromPreview("-attack");
		s_heldThrow = false;
	}
	if (g_throwLockPulse.exchange(false, std::memory_order_relaxed) && s_have && !lmb)
		commitLockFromPreview("weapon_fire");

	if (s_lockHave && now <= s_lockUntil) {
		if (!MenuEnsureSurf()) return;
		int sw = 0, sh = 0;
		if (!SurfGetScreenSize(&sw, &sh)) return;
		ClientUxDrawThrowOverlay(s_lockLand, s_lockPath, s_lockPathN, true, sw, sh);
		return;
	}

	if (kind < 1) {
		if (!s_heldThrow) {
			s_have = false;
			s_kindCached = 0;
			s_pathN = 0;
		}
		return;
	}

	// Holding +attack: translate last arc with eye delta (walk). Do not resimulate.
	if (!lmb)
		refreshPreview();

	if (!s_have) return;
	if (!MenuEnsureSurf()) return;
	int sw = 0, sh = 0;
	if (!SurfGetScreenSize(&sw, &sh)) return;
	float dLand[3] = { s_land[0], s_land[1], s_land[2] };
	float dPath[kThrowPathMax][3]{};
	for (int i = 0; i < s_pathN; ++i) {
		dPath[i][0] = s_path[i][0];
		dPath[i][1] = s_path[i][1];
		dPath[i][2] = s_path[i][2];
	}
	if (lmb)
		walkShift(dLand, dPath, s_pathN);
	ClientUxDrawThrowOverlay(dLand, dPath, s_pathN, false, sw, sh);
}

static void RunDeferredCiTrace() {
	if (!g_pendingCiImpactScan) return;
	g_pendingCiImpactScan = false;
	if (!g_run.load(std::memory_order_relaxed) || !SkeetoFeaturesOn())
		return;
	if (!g_hitDetectArmed.load() || !HitModeCiAllowed())
		return;
	if (GetTickCount() < g_suppressCommonHitUntil)
		return;
	if (!TryCiHitTrace(g_ciTracePos))
		return;
	QueueHitFeedback();
}

// Game-thread pump: CI TraceRay + delayed hit FX + overlay clear.
// Called from EngineVGui::Paint AFTER orig (FSN/L4N datacache walk already finished).
// Never call this from the Sleep worker.
static void PumpGameThreadFeedback() {
	if (!g_run.load(std::memory_order_relaxed))
		return;
	CrashMark(kBcHit);
	static bool s_pumpLogged = false;
	if (!s_pumpLogged) {
		s_pumpLogged = true;
		Log("ci-fx pump on EngineVGui::Paint (not worker/FSN)");
	}
	if (SkeetoFeaturesOn()) {
		RunDeferredCiTrace();
		FlushPendingCommonHit();
	}
	DWORD now = GetTickCount();
	if (g_overlayClearAt && now >= g_overlayClearAt)
		ClearHitOverlay();
}

static void AngleVectors(float pitch, float yaw, float* forward) {
	const float deg = 0.01745329251f;
	float p = pitch * deg, y = yaw * deg;
	float cp = cosf(p), sp = sinf(p), cy = cosf(y), sy = sinf(y);
	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;
}

// SI hit from HP drop: O(1) aim check against the damaged ent only.
// Caller already verified team/recent-fire; skip redundant EntGet/VirtualQuery here.
static void OnSiHealthDecreased(void* ent, int objectId, int oldHp, int newHp, float maxDist, float minDot) {
	if (!ent || g_offOrigin < 0 || g_offOrigin > 0x4000) return;

	const DWORD now = GetTickCount();
	const int dmgAmt = oldHp - newHp;
	auto showBar = [&]() {
		if (!g_optClientInfectedHp) return;
		int uid = 0;
		PlayerInfo pi{};
		if (EngPlayerInfo(objectId, &pi) && pi.userid > 0)
			uid = pi.userid;
		LocalPlayMarkLocalSi(objectId);
		LocalPlayNotifySiHp(ent, objectId, newHp, uid);
	};

	// Already attributed this SI (your shot / shotgun follow-up): update bar, skip aim.
	// Still play Hit — locking the bar used to return here and mute full-auto.
	if (g_hpLocalSiEnt == objectId && g_hpLocalSiUntil && now <= g_hpLocalSiUntil) {
		showBar();
		ClientUxDmgOnSi(ent, objectId, dmgAmt);
		if (HitModeSiAllowed()) {
			float* o = (float*)((uint8_t*)ent + g_offOrigin);
			FeedbackSiHit(o[0], o[1], o[2] + 40.f);
		}
		return;
	}
	// Shotgun coalesce: do not open a bar for some other SI (teammate damage).
	// Damage numbers still go through the aim check below.
	if (now - g_lastHitFeedbackAt < 70 && !g_optClientDmgNum)
		return;

	int local = EngLocal();
	if (local <= 0 || objectId == local) return;
	if (objectId <= 0 || objectId > 64) return;

	float eye[3];
	if (!LocalEyePos(eye)) return;

	float angles[3]{};
	EngGetViewAngles(angles);
	float forward[3];
	AngleVectors(angles[0], angles[1], forward);

	float* eo = (float*)((uint8_t*)ent + g_offOrigin);
	float dx = eo[0] - eye[0], dy = eo[1] - eye[1], dz = (eo[2] + 40.f) - eye[2];
	float len = sqrtf(dx * dx + dy * dy + dz * dz);
	if (len < 1.f || len > maxDist) return;
	dx /= len; dy /= len; dz /= len;
	float dot = dx * forward[0] + dy * forward[1] + dz * forward[2];
	if (dot < minDot) return;

	if (g_hitDetectLogLeft > 0) {
		--g_hitDetectLogLeft;
		Log("si-hit hp id=%d dot=%.2f dist=%.0f", objectId, dot, len);
	}
	showBar();
	ClientUxDmgOnSi(ent, objectId, dmgAmt);
	if (HitModeSiAllowed()) {
		float* o = (float*)((uint8_t*)ent + g_offOrigin);
		FeedbackSiHit(o[0], o[1], o[2] + 40.f);
	}
}

// FF: real m_iHealth drop required; gun/melee both need aim when requireMeleeAim, gun path passes false after aim at caller... 
// requireMeleeAim=true → close range; false → still aim but longer range.
static void OnSurvivorHealthDecreased(void* ent, int objectId, int oldHp, int newHp, bool requireMeleeAim) {
	int dmg = oldHp - newHp;
	if (dmg <= 0) return;

	if (!EntReadable(ent) || g_offOrigin < 0 || g_offOrigin > 0x4000) return;
	// Incap bleed ticks also decrease m_iHealth. Aim-cone alone would false-trigger while
	// shooting near a downed teammate. Skip FF feedback entirely while incapacitated
	// (also suppresses rare true FF-on-incap; prefer no bleed false positives).
	if (g_offIsIncapacitated >= 0 && g_offIsIncapacitated <= 0x4000
		&& (*(unsigned char*)((uint8_t*)ent + g_offIsIncapacitated) != 0))
		return;

	float eye[3];
	if (!LocalEyePos(eye)) return;
	float angles[3]{};
	EngGetViewAngles(angles);
	float forward[3];
	AngleVectors(angles[0], angles[1], forward);
	float* eo = (float*)((uint8_t*)ent + g_offOrigin);
	float dx = eo[0] - eye[0], dy = eo[1] - eye[1], dz = (eo[2] + 40.f) - eye[2];
	float len = sqrtf(dx * dx + dy * dy + dz * dz);
	const float maxDist = requireMeleeAim ? 110.f : 3000.f;
	const float minDot = requireMeleeAim ? 0.90f : 0.93f;
	if (len < 1.f || len > maxDist) return;
	dx /= len; dy /= len; dz /= len;
	float dot = dx * forward[0] + dy * forward[1] + dz * forward[2];
	if (dot < minDot) return;

	if (g_hitDetectLogLeft > 0) {
		--g_hitDetectLogLeft;
		Log("ff-hit ent=%d dmg=%d meleeAim=%d", objectId, dmg, requireMeleeAim ? 1 : 0);
	}
	FeedbackFf(dmg, eo[0], eo[1], eo[2] + 40.f);
}

// Cache FL_ONGROUND for skeet; arm melee window on LMB edge (in-game only).
// Skip work when nothing that consumes these signals is enabled.
static void Sample() {
	const bool needMeleeWindow = NeedHealthProxyWork() || g_optClientInfectedHp || g_optClientDmgNum;
	// Ground scan for special-kill FX and kill-HUD skeet counts. Style selected is not
	// enough — menu kill_fx=off must skip the poll unless the elim HUD is on.
	const char* fxId = DlcGetSelected("fx");
	const bool needGroundScan =
		(g_optKillFx && fxId && fxId[0] && _stricmp(fxId, "off") != 0)
		|| g_optElim;
	if (!needMeleeWindow && !needGroundScan)
		return;
	if (!SkeetoFeaturesOn())
		return;

	if (needMeleeWindow) {
		static bool s_prevLmb = false;
		const bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		if (lmb && !s_prevLmb && g_engine && EngInGame())
			g_meleeAttackUntil = GetTickCount() + 400; // melee swing window
		s_prevLmb = lmb;
	}

	if (!needGroundScan) return;
	if (!g_entlist || g_offFlags < 0 || g_offFlags > 0x4000) return;
	static DWORD s_lastGroundAt = 0;
	static int s_groundPhase = 0;
	DWORD now = GetTickCount();
	if (s_lastGroundAt && now - s_lastGroundAt < 100) return;
	s_lastGroundAt = now;
	const int phase = s_groundPhase++ & 1;
	for (int i = 1 + phase; i <= 64; i += 2) {
		void* e = EntGet(i);
		if (!e) { g_lastGround[i] = 1; continue; }
		int flags = *(int*)((uint8_t*)e + g_offFlags);
		const bool onGround = (flags & 1) != 0;
		g_lastGround[i] = onGround ? 1 : 0;
		if (!onGround)
			g_lastAirborneAt[i] = now;
	}
}

static void ResetHealthCache() {
	memset(g_lastHealth, 0, sizeof(g_lastHealth));
	memset(g_healthInit, 0, sizeof(g_healthInit));
	for (int i = 0; i < 65; ++i) {
		g_lastGround[i] = 1; // assume grounded until Sample proves otherwise
		g_lastAirborneAt[i] = 0;
	}
	g_hitDetectArmed = false;
	g_hitDetectEnableAt = GetTickCount() + 4000;
	g_suppressCommonHitUntil = 0;
	g_pendingCommonHit = false;
	g_pendingCommonHitAt = 0;
	g_pendingCiImpactScan = false;
	g_siStreak = 0;
	Log("recv-hit arm in 4s (hooks=%d orig=%p)", g_healthProxyHooks, (void*)g_origHealthProxy);
}

static void FireParticle(const char* effectName, float x, float y, float z, bool worldSpace) {
	if (!SkeetoFeaturesOn()) return;
	if (!effectName || !effectName[0]) return;
	if (!g_dispatch || !IsExec((void*)g_dispatch)) {
		if (g_feedbackLogLeft > 0) {
			--g_feedbackLogLeft;
			Log("particle SKIP dispatch=null name=%s", effectName);
		}
		return;
	}
	// Combat Dispatch: do not PrecacheParticleSystem here (MDL/datacache). Names were
	// precached at connect/loading.
	if (worldSpace) {
		g_fxOrigin[0] = x; g_fxOrigin[1] = y; g_fxOrigin[2] = z;
		g_fxAngles[0] = 0.f; g_fxAngles[1] = 270.f; g_fxAngles[2] = 0.f;
	} else {
		g_fxOrigin[0] = 0.f; g_fxOrigin[1] = 0.f; g_fxOrigin[2] = 0.f;
		g_fxAngles[0] = 0.f; g_fxAngles[1] = 0.f; g_fxAngles[2] = 0.f;
	}
	g_dispatch(effectName, g_fxOrigin, g_fxAngles, 2, 0, 0);
	if (g_feedbackLogLeft > 0) {
		--g_feedbackLogLeft;
		Log("particle '%s' world=%d at %.0f %.0f %.0f", effectName, worldSpace ? 1 : 0, x, y, z);
	}
}

static void ApplyDlcFx(const DlcFx* fx, bool throttle, float wx, float wy, float wz,
	DWORD defaultOverlayMs = kOverlayClearMs) {
	if (!SkeetoFeaturesOn()) return;
	if (!fx || !fx->used) return;
	if (!g_optSound && !g_optIcon) return;
	DWORD now = GetTickCount();
	if (throttle && now - g_lastHitFeedbackAt < 70)
		return;
	g_lastHitFeedbackAt = now;

	UnlockOverlayCommand();
	// overlay is always fullscreen (r_screenoverlay). world:true only affects particle
	// spawn coords — do NOT gate overlay on worldSpace (SI hit can have both).
	if (g_optIcon && fx->overlay[0]) {
		char cmd[160]{};
		snprintf(cmd, sizeof(cmd), "r_screenoverlay %s", fx->overlay);
		EngClientCmd(cmd);
		DWORD clearMs = (fx->overlayMs > 0) ? (DWORD)fx->overlayMs : defaultOverlayMs;
		if (clearMs < 40) clearMs = 40;
		if (clearMs > 5000) clearMs = 5000;
		g_overlayClearAt = now + clearMs;
	}
	if (g_optIcon && fx->particle[0])
		FireParticle(fx->particle, wx, wy, wz, fx->worldSpace);
	if (g_optIcon) {
		for (int i = 0; i < fx->particleExtraCount; ++i) {
			if (fx->particleExtra[i][0])
				FireParticle(fx->particleExtra[i], wx, wy, wz, fx->worldSpace);
		}
	}
	char snd[96]{};
	const bool haveSnd = g_optSound && DlcPickSound(fx, snd, sizeof(snd));
	if (haveSnd)
		PlayHitSound(snd);

	if (g_feedbackLogLeft > 0) {
		--g_feedbackLogLeft;
		Log("dlc-fx ov=%s pt=%s extras=%d snd=%s sndPool=%d pri=%d world=%d ovMs=%d",
			fx->overlay, fx->particle, fx->particleExtraCount,
			haveSnd ? snd : "-", fx->soundExtraCount, fx->priority,
			fx->worldSpace ? 1 : 0,
			fx->overlayMs > 0 ? fx->overlayMs : (int)defaultOverlayMs);
	}
}

static void ClearHitOverlay() {
	UnlockOverlayCommand();
	EngClientCmd("r_screenoverlay \"\"");
	g_overlayClearAt = 0;
}

static bool IsMeleeWeaponName(const char* w) {
	if (!w || !w[0]) return false;
	if (!_stricmp(w, "melee") || !_stricmp(w, "weapon_melee") || !_stricmp(w, "chainsaw"))
		return true;
	if (strstr(w, "melee")) return true;
	static const char* kMelee[] = {
		"katana", "fireaxe", "machete", "crowbar", "cricket_bat", "baseball_bat",
		"electric_guitar", "frying_pan", "golfclub", "pitchfork", "shovel",
		"tonfa", "knife", "riotshield", "guitar", "pan", "bat",
		nullptr
	};
	for (int i = 0; kMelee[i]; ++i) {
		if (!_stricmp(w, kMelee[i]))
			return true;
	}
	return false;
}

static bool WeaponIsMelee(const char* wpn) {
	return wpn && wpn[0] && (!strcmp(wpn, "melee") || IsMeleeWeaponName(wpn));
}

static bool VictimWasAirborne(int ve) {
	DWORD now = GetTickCount();
	int groundCached = (ve > 0 && ve < 65) ? g_lastGround[ve] : 1;
	int groundLive = 1;
	if (ve > 0 && g_offFlags >= 0 && g_offFlags <= 0x4000) {
		void* victim = EntGet(ve);
		if (victim) {
			MEMORY_BASIC_INFORMATION mbi{};
			if (VirtualQuery(victim, &mbi, sizeof(mbi)) && mbi.State == MEM_COMMIT) {
				int flags = *(int*)((uint8_t*)victim + g_offFlags);
				groundLive = (flags & 1) ? 1 : 0;
			}
		}
	}
	const bool recentAir = (ve > 0 && ve < 65 && g_lastAirborneAt[ve]
		&& (now - g_lastAirborneAt[ve] < 350));
	return (groundCached == 0) || (groundLive == 0) || recentAir;
}

static void FireFX(const char* name, float x, float y, float z) {
	if (!SkeetoFeaturesOn()) return;
	// Special SI kill FX: DIY dual channel.
	// - particle → always world-space (victim origin)
	// - overlay → screen overlay only if JSON sets a real overlay path
	//   (leave empty when material is SpriteCard particle sheet)
	if (!g_optKillFx) return;
	if (!name || !name[0]) return;

	DlcFx fx{};
	const char* effect = name;
	const char* overlay = nullptr;
	char snd[96]{};
	bool haveSnd = false;
	if (DlcResolveNamed("fx", name, &fx)) {
		if (fx.particle[0])
			effect = fx.particle;
		else
			effect = nullptr; // JSON explicitly disabled particle
		if (fx.overlay[0])
			overlay = fx.overlay;
		haveSnd = g_optSound && DlcPickSound(&fx, snd, sizeof(snd));
	}

	if (effect && effect[0])
		FireParticle(effect, x, y, z, true);
	// Optional extras from JSON "particles" (same world origin as primary).
	for (int i = 0; i < fx.particleExtraCount; ++i) {
		if (fx.particleExtra[i][0])
			FireParticle(fx.particleExtra[i], x, y, z, true);
	}

	if (overlay && overlay[0] && g_optIcon) {
		UnlockOverlayCommand();
		char cmd[160]{};
		snprintf(cmd, sizeof(cmd), "r_screenoverlay %s", overlay);
		EngClientCmd(cmd);
		DWORD clearMs = (fx.overlayMs > 0) ? (DWORD)fx.overlayMs : kOverlayClearMsFx;
		if (clearMs < 40) clearMs = 40;
		if (clearMs > 5000) clearMs = 5000;
		g_overlayClearAt = GetTickCount() + clearMs;
	}

	if (haveSnd)
		PlayHitSound(snd);

	if (g_feedbackLogLeft > 0) {
		--g_feedbackLogLeft;
		Log("FireFX name=%s pt=%s ov=%s", name, effect ? effect : "", overlay ? overlay : "");
	}
}

static bool IsShotgun(const char* w) {
	return w && (!strcmp(w, "shotgun_chrome") || !strcmp(w, "pumpshotgun")
		|| !strcmp(w, "autoshotgun") || !strcmp(w, "shotgun_spas"));
}

static bool IsLocalAttacker(void* ev) {
	if (!ev) return false;
	if (g_localUserId <= 0)
		RefreshLocalUserId();
	int local = EngLocal();
	int atkUid = EvInt(ev, "attacker");
	int atkEntId = EvInt(ev, "attackerentid");

	if (atkUid > 0 && g_localUserId > 0 && atkUid == g_localUserId)
		return true;
	if (local > 0 && atkUid > 0 && EngPlayerForUserID(atkUid) == local)
		return true;
	if (local > 0 && atkEntId > 0 && atkEntId == local)
		return true;
	return false;
}

static bool CiChannelOn() {
	const char* id = DlcGetSelected("ci");
	return id && id[0] && _stricmp(id, "off") != 0;
}

static bool SiChannelOn() {
	const char* id = DlcGetSelected("si");
	return id && id[0] && _stricmp(id, "off") != 0;
}

// 特感击杀：画面来源与音效来源独立（用特感包 / 跟普感）。
static void AssembleSiFx(const DlcFx* si, bool haveSi, const DlcFx* ci, bool haveCi, DlcFx* out) {
	memset(out, 0, sizeof(*out));

	const bool wantSiVisual = g_optSiVisual && haveSi;
	if (wantSiVisual) *out = *si;
	else if (haveCi) *out = *ci;
	else if (haveSi) *out = *si;
	else return;

	out->sound[0] = 0;
	const bool wantSiSound = g_optSiSound && haveSi;
	if (wantSiSound) {
		if (si->sound[0]) strncpy(out->sound, si->sound, sizeof(out->sound) - 1);
		else if (haveCi && ci->sound[0]) strncpy(out->sound, ci->sound, sizeof(out->sound) - 1);
	} else {
		if (haveCi && ci->sound[0]) strncpy(out->sound, ci->sound, sizeof(out->sound) - 1);
		else if (haveSi && si->sound[0]) strncpy(out->sound, si->sound, sizeof(out->sound) - 1);
	}
	out->sound[sizeof(out->sound) - 1] = 0;
	if (!out->used) out->used = true;
}

static void FeedbackCi(const char* kind, bool throttle, bool meleeKill, bool ignoreHitModeGate,
	float wx, float wy, float wz) {
	if (!kind || !kind[0] || !g_engine || !EngInGame()) return;
	if (!CiChannelOn()) return;
	if (!g_optSound && !g_optIcon) return;

	const bool isHit = !_stricmp(kind, "hit");
	if (isHit && !ignoreHitModeGate && !HitModeCiAllowed()) return;
	// Shotgun pellet storms: skip resolve work when hit FX already played.
	if (throttle && isHit) {
		DWORD now = GetTickCount();
		if (now - g_lastHitFeedbackAt < 70)
			return;
	}
	const bool isHs = !_stricmp(kind, "headshot");
	DlcFx fx{};
	if (!DlcResolve("ci", isHit ? "hit" : "kill", isHs, meleeKill && !isHit && !isHs, 0, &fx))
		return;
	ApplyDlcFx(&fx, throttle, wx, wy, wz, isHit ? kOverlayClearMsHit : kOverlayClearMs);
}

static void FeedbackFf(int dmg, float wx, float wy, float wz) {
	if (!g_optFf || !g_engine || !EngInGame()) return;
	if (!g_optSound && !g_optIcon) return;
	DlcFx fx{};
	if (!DlcResolve("ff", dmg >= 15 ? "kill" : "hit", false, false, 0, &fx))
		return;
	const bool isHit = (dmg < 15);
	ApplyDlcFx(&fx, true, wx, wy, wz, isHit ? kOverlayClearMsHit : kOverlayClearMs);
}

static void FeedbackSiHit(float wx, float wy, float wz) {
	if (!HitModeSiAllowed()) return;
	// Shotgun: player_hurt / HP proxy can fire per pellet — bail BEFORE DlcResolve.
	DWORD now = GetTickCount();
	if (now - g_lastHitFeedbackAt < 70)
		return;

	DlcFx si{}, ci{};
	const bool haveSi = SiChannelOn() && DlcResolve("si", "hit", false, false, 0, &si);
	const bool haveCi = CiChannelOn() && DlcResolve("ci", "hit", false, false, 0, &ci);
	if (!haveSi && !haveCi) return;

	DlcFx out{};
	AssembleSiFx(&si, haveSi, &ci, haveCi, &out);
	if (!out.overlay[0] && !out.particle[0] && !out.sound[0] && out.particleExtraCount <= 0) return;
	ApplyDlcFx(&out, true, wx, wy, wz, kOverlayClearMsHit);
	if (g_feedbackLogLeft > 0) {
		--g_feedbackLogLeft;
		Log("si-hit siVis=%d siSnd=%d ov=%s pt=%s snd=%s",
			g_optSiVisual ? 1 : 0, g_optSiSound ? 1 : 0,
			out.overlay[0] ? out.overlay : "-",
			out.particle[0] ? out.particle : "-",
			out.sound[0] ? out.sound : "-");
	}
}

static void FeedbackSiKill(void* ev) {
	if (!g_engine || !EngInGame() || !ev) return;
	if (!g_optSound && !g_optIcon) return;
	g_suppressCommonHitUntil = GetTickCount() + 400;

	const char* wpn = EvStr(ev, "weapon");
	bool hs = EvBool(ev, "headshot") || EvInt(ev, "hitgroup", 0) == 1;
	bool melee = IsMeleeWeaponName(wpn);
	++g_siStreak;

	DlcFx si{}, ci{};
	const bool haveSi = SiChannelOn() && DlcResolve("si", "kill", hs, melee, g_siStreak, &si);
	const bool haveCi = CiChannelOn() && DlcResolve("ci", "kill", hs, melee && !hs, 0, &ci);
	if (!haveSi && !haveCi) {
		Log("si-kill: no si/ci resolve (si=%s ci=%s)",
			DlcGetSelected("si")[0] ? DlcGetSelected("si") : "off",
			DlcGetSelected("ci")[0] ? DlcGetSelected("ci") : "off");
		return;
	}

	float wx = EvFloat(ev, "victim_x"), wy = EvFloat(ev, "victim_y"), wz = EvFloat(ev, "victim_z");
	if (wx == 0.f && wy == 0.f && wz == 0.f && g_haveImpactPos) {
		wx = g_lastImpactPos[0]; wy = g_lastImpactPos[1]; wz = g_lastImpactPos[2];
	}

	DlcFx out{};
	AssembleSiFx(&si, haveSi, &ci, haveCi, &out);
	ApplyDlcFx(&out, false, wx, wy, wz);
	Log("feedback si kill streak=%d hs=%d melee=%d siVis=%d siSnd=%d ov=%s pt=%s snd=%s",
		g_siStreak, hs ? 1 : 0, melee ? 1 : 0,
		g_optSiVisual ? 1 : 0, g_optSiSound ? 1 : 0,
		out.overlay[0] ? out.overlay : "-",
		out.particle[0] ? out.particle : "-",
		out.sound[0] ? out.sound : "-");
}

static void OnBulletImpact(void* ev) {
	// Local gun impacts: stamp coords. CI TraceRay is deferred to EngineVGui::Paint
	// (same MASK_SHOT eye→impact as before — not run inside FireGameEvent / FSN).
	if (g_localUserId <= 0)
		RefreshLocalUserId();
	int uid = EvInt(ev, "userid");
	if (uid <= 0 || g_localUserId <= 0 || uid != g_localUserId)
		return;
	if (!NeedGunHitAttribution() && !g_optClientInfectedHp && !g_optClientDmgNum)
		return;

	DWORD now = GetTickCount();
	if (HitModeSiAllowed() || g_optFf || g_optClientInfectedHp || g_optClientDmgNum)
		g_lastLocalGunImpactAt = now;

	g_lastImpactPos[0] = EvFloat(ev, "x");
	g_lastImpactPos[1] = EvFloat(ev, "y");
	g_lastImpactPos[2] = EvFloat(ev, "z");
	g_haveImpactPos = true;

	if (!HitModeCiAllowed())
		return;
	if (!g_hitDetectArmed.load())
		return;
	if (now < g_suppressCommonHitUntil)
		return;
	if (g_pendingCommonHit)
		return;
	// First pellet of a burst keeps the trace origin (shotgun storm).
	if (g_pendingCiImpactScan)
		return;
	g_ciTracePos[0] = g_lastImpactPos[0];
	g_ciTracePos[1] = g_lastImpactPos[1];
	g_ciTracePos[2] = g_lastImpactPos[2];
	g_pendingCiImpactScan = true;
}

static void OnHurt(void* ev, bool commonInfected) {
	// SI hit → m_iHealth proxy. Regular CI hit → bullet_impact TraceRay.
	// Witch seated/crawling hitboxes often miss MASK_SHOT, so local infected_hurt
	// ONLY for Witch (same as before).
	if (!commonInfected || !ev) return;
	if (!HitModeCiAllowed() || !g_hitDetectArmed.load()) return;
	if (!g_run.load(std::memory_order_relaxed)) return;
	DWORD now = GetTickCount();
	if (now < g_suppressCommonHitUntil) return;
	if (g_pendingCommonHit) return;

	if (g_localUserId <= 0)
		RefreshLocalUserId();
	const int atk = EvInt(ev, "attacker");
	if (atk <= 0 || g_localUserId <= 0 || atk != g_localUserId)
		return;

	const int ent = EvInt(ev, "entityid");
	if (ent <= 0) return;
	if (!IsWitchClass(EntNetClassName(ent)))
		return;

	if (g_offOriginCommon >= 0 && g_offOriginCommon <= 0x4000) {
		void* e = EntGet(ent);
		if (EntReadable(e)) {
			float* o = (float*)((uint8_t*)e + g_offOriginCommon);
			g_lastImpactPos[0] = o[0];
			g_lastImpactPos[1] = o[1];
			g_lastImpactPos[2] = o[2] + 40.f;
			g_haveImpactPos = true;
		}
	}
	if (g_commonHitLogLeft > 0) {
		--g_commonHitLogLeft;
		Log("witch-hit ent=%d (infected_hurt queue)", ent);
	}
	QueueHitFeedback();
}

static void OnKillFeedback(void* ev, bool specialInfected) {
	if (!IsLocalAttacker(ev)) return;
	g_pendingCommonHit = false;
	g_pendingCommonHitAt = 0;
	g_pendingCiImpactScan = false;
	g_suppressCommonHitUntil = GetTickCount() + 400;
	if (specialInfected)
		FeedbackSiKill(ev);
	else {
		if (!CiChannelOn())
			return;
		bool hs = EvBool(ev, "headshot") || EvInt(ev, "hitgroup", 0) == 1;
		bool melee = IsMeleeWeaponName(EvStr(ev, "weapon"));
		float wx = EvFloat(ev, "victim_x"), wy = EvFloat(ev, "victim_y"), wz = EvFloat(ev, "victim_z");
		if (wx == 0.f && wy == 0.f && wz == 0.f && g_haveImpactPos) {
			wx = g_lastImpactPos[0]; wy = g_lastImpactPos[1]; wz = g_lastImpactPos[2];
		}
		if (hs)
			FeedbackCi("headshot", false, false, false, wx, wy, wz);
		else
			FeedbackCi("kill", false, melee, false, wx, wy, wz);
	}
}

static void OnDeath(void* ev) {
	RefreshLocalUserId();
	const char* vn = EvStr(ev, "victimname");
	int atk = EvInt(ev, "attacker");
	int atkEntId = EvInt(ev, "attackerentid");
	int local = EngLocal();
	int atkEnt = atk > 0 ? EngPlayerForUserID(atk) : -1;

	if (g_deathLogLeft > 0) {
		--g_deathLogLeft;
		Log("player_death vn=%s atkUid=%d atkEnt=%d local=%d localUid=%d atkEntId=%d hs=%d wpn=%s",
			vn ? vn : "?", atk, atkEnt, local, g_localUserId, atkEntId,
			EvBool(ev, "headshot") ? 1 : 0,
			EvStr(ev, "weapon"));
	}

	if (!vn || !vn[0] || !strcmp(vn, "Infected")) return;

	if (!IsLocalAttacker(ev)) {
		if (g_skipLogLeft > 0 && vn[0]) {
			--g_skipLogLeft;
			Log("skip not-local vn=%s atkUid=%d localUid=%d atkEnt=%d local=%d atkEntId=%d",
				vn, atk, g_localUserId, atkEnt, local, atkEntId);
		}
		return;
	}

	g_elimSi.fetch_add(1, std::memory_order_relaxed);
	if (EvBool(ev, "headshot") || EvInt(ev, "hitgroup", 0) == 1)
		g_elimHs.fetch_add(1, std::memory_order_relaxed);

	const char* wpn = EvStr(ev, "weapon");
	const bool melee = WeaponIsMelee(wpn);
	if (melee)
		g_elimSiMelee.fetch_add(1, std::memory_order_relaxed);

	const bool hunterOrJockey = !strcmp(vn, "Hunter") || !strcmp(vn, "Jockey");
	if (hunterOrJockey || g_optKillFx)
		Sample();
	int ve = EngPlayerForUserID(EvInt(ev, "userid"));
	const bool airborne = hunterOrJockey ? VictimWasAirborne(ve) : false;
	if (hunterOrJockey && airborne) {
		if (melee)
			g_elimMeleeSkeet.fetch_add(1, std::memory_order_relaxed);
		else
			g_elimSkeet.fetch_add(1, std::memory_order_relaxed);
	}

	OnKillFeedback(ev, true); // special infected — HUD kill/headshot (ci/si)
	if (!g_optKillFx) return;

	float x = EvFloat(ev, "victim_x"), y = EvFloat(ev, "victim_y"), z = EvFloat(ev, "victim_z");
	bool hs = EvBool(ev, "headshot");

	if (!strcmp(vn, "Charger")) {
		if (hs) FireFX(melee ? "level" : "crit_text", x, y, z);
		return;
	}
	if (hunterOrJockey) {
		// Only skeet / melee_skeet when actually airborne — ground melee must NOT count.
		if (airborne) {
			if (melee) FireFX(hs ? "headshot_skeet" : "melee_skeet", x, y, z);
			else if (hs) FireFX("headshot_skeet", x, y, z);
			else FireFX("skeet", x, y, z);
		} else if (hs) {
			FireFX("crit_text", x, y, z);
		}
		return;
	}
	if (!strcmp(vn, "Witch")) {
		if (melee) FireFX("melee_crown", x, y, z);
		else if (IsShotgun(wpn)) FireFX(hs ? "perfect_crown" : "crown", x, y, z);
		return;
	}
	if (hs) FireFX("crit_text", x, y, z);
}

static void OnInfectedDeath(void* ev) {
	if (IsLocalAttacker(ev)) {
		g_elimCi.fetch_add(1, std::memory_order_relaxed);
		if (EvBool(ev, "headshot") || EvInt(ev, "hitgroup", 0) == 1)
			g_elimHs.fetch_add(1, std::memory_order_relaxed);
	}
	OnKillFeedback(ev, false); // common infected — keep fullscreen kill/headshot overlays
}

static void OnTongue(void* ev) {
	if (!g_optKillFx) return;
	RefreshLocalUserId();
	if (EvInt(ev, "release_type") != 4) return;
	if (EvInt(ev, "userid") != EvInt(ev, "victim")) return;
	int cutUid = EvInt(ev, "userid");
	if (g_localUserId > 0) {
		if (cutUid != g_localUserId) return;
	} else if (EngPlayerForUserID(cutUid) != EngLocal()) {
		return;
	}
	int se = EngPlayerForUserID(EvInt(ev, "smoker"));
	void* e = (se > 0) ? EntGet(se) : nullptr;
	if (!e || g_offOrigin < 0 || g_offOrigin > 0x4000) return;
	float* o = (float*)((uint8_t*)e + g_offOrigin);
	FireFX("tongue_cut", o[0], o[1], o[2] + 64.f);
}

static void HandleEvent(void* ev) {
	if (!ev || !g_ready.load() || !g_run.load(std::memory_order_relaxed)) return;
	if (!SkeetoFeaturesOn()) return;
	CrashMark(kBcEvent);
	const char* n = EvName(ev);
	if (!n) return;
	InterlockedIncrement(&g_fireCalls);
	if (g_evtLogLeft > 0) {
		--g_evtLogLeft;
		Log("evt #%ld '%s'", (long)g_fireCalls, n);
	}
	if (!strcmp(n, "player_death")) {
		LocalPlayOnTrackedDeath(ev);
		ClientUxOnSiDeath(ev);
		OnDeath(ev);
	}
	else if (!strcmp(n, "infected_death")) {
		ClientUxOnCiDeath(ev);
		OnInfectedDeath(ev);
	}
	else if (!strcmp(n, "player_hurt")) {
		OnHurt(ev, false);
		LocalPlayOnPlayerHurtHp(ev);
		ClientUxOnDmgPlayerHurt(ev);
	}
	else if (!strcmp(n, "infected_hurt")) {
		OnHurt(ev, true);
		LocalPlayOnWitchHurt(ev);
	}
	else if (!strcmp(n, "bullet_impact")) OnBulletImpact(ev);
	else if (!strcmp(n, "tongue_pull_stopped")) OnTongue(ev);
	else if (!strcmp(n, "witch_spawn")) LocalPlayOnWitchSpawn(ev);
	else if (!strcmp(n, "witch_killed")) {
		ClientUxOnWitchDeath(ev);
		LocalPlayOnWitchKilled(ev);
	}
	else if (!strcmp(n, "round_start")) {
		LocalPlayOnRoundBoundary(true);
		ClientUxDmgReset();
		ClientUxApplyDirectorHud(true);
		RoundTimerOnStart();
	}
	else if (!strcmp(n, "round_end")) {
		LocalPlayOnRoundBoundary(false);
		XhairHideNow();
		RoundTimerOnEnd();
	}
	else if (!strcmp(n, "map_transition")) XhairHideNow();
	else if (!strcmp(n, "mission_lost")) ElimOnMissionLost();
	else if (!strcmp(n, "bot_player_replace")) LocalPlayOnCharBotSwap(ev, true);
	else if (!strcmp(n, "player_bot_replace")) LocalPlayOnCharBotSwap(ev, false);
	else if (!strcmp(n, "weapon_fire")) ClientUxOnLocalThrowableFire(ev);
}

// IGameEventListener2 (MSVC vtable[0]=dtor, [1]=FireGameEvent, [2]=GetEventDebugID=42)
struct SkeetoEventListener {
	virtual ~SkeetoEventListener() {}
	virtual void FireGameEvent(void* ev) { HandleEvent(ev); }
	virtual int GetEventDebugID() { return 42; }
};
static SkeetoEventListener g_pod;

static bool AddOne(const char* name) {
	if (!g_events) return false;
	auto fn = (AddListenerFn)VGet(g_events, 3);
	if (!fn || !IsExec((void*)fn)) {
		Log("AddListener fn invalid");
		return false;
	}
	bool ok = fn(g_events, &g_pod, name, false);
	Log("AddListener('%s')=%d", name, ok ? 1 : 0);
	return ok;
}

static void EnsureListeners() {
	if (!g_events || !g_ready.load()) return;
	// Re-register each time — map change may Reset the manager
	bool a = AddOne("player_death");
	bool b = AddOne("tongue_pull_stopped");
	bool c = AddOne("player_hurt");
	bool d = AddOne("infected_hurt");
	bool e = AddOne("infected_death");
	bool f = AddOne("bullet_impact"); // local impact stamp + deferred CI TraceRay
	bool g = false;
	bool h = AddOne("witch_spawn");
	bool i = AddOne("witch_killed");
	bool j = AddOne("round_start");
	bool k = AddOne("round_end");
	bool l = AddOne("bot_player_replace");
	bool m = AddOne("player_bot_replace");
	bool nEvt = AddOne("weapon_fire");
	bool oEvt = AddOne("map_transition");
	bool pEvt = AddOne("mission_lost");
	g_listening = a || b || c || d || e || f || g || h || i || j || k || l || m || nEvt || oEvt || pEvt;
}

static bool CmdLineHasFlag(const char* flag) {
	if (!flag || !flag[0]) return false;
	const char* cmd = GetCommandLineA();
	if (!cmd || !cmd[0]) return false;
	const size_t n = strlen(flag);
	for (const char* p = cmd; *p; ++p) {
		if (_strnicmp(p, flag, n) != 0) continue;
		const char next = p[n];
		const bool boundary =
			next == 0 || next == ' ' || next == '\t' || next == '\r' || next == '\n';
		if (!boundary) continue;
		if (p > cmd) {
			const char prev = p[-1];
			if (prev != ' ' && prev != '\t' && prev != '"')
				continue;
		}
		return true;
	}
	return false;
}

static LPTOP_LEVEL_EXCEPTION_FILTER g_prevCrashFilter = nullptr;

static bool PtrInModule(void* addr, HMODULE mod) {
	if (!addr || !mod) return false;
	const auto base = (uintptr_t)mod;
	auto dos = (IMAGE_DOS_HEADER*)base;
	if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
	auto nt = (IMAGE_NT_HEADERS*)(base + (DWORD)dos->e_lfanew);
	if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
	const auto p = (uintptr_t)addr;
	return p >= base && p < base + nt->OptionalHeader.SizeOfImage;
}

static bool CrashLooksLikeOurs(EXCEPTION_POINTERS* ep) {
	if (!ep || !ep->ExceptionRecord || !g_self) return false;
	if (PtrInModule(ep->ExceptionRecord->ExceptionAddress, g_self))
		return true;
	CONTEXT* ctx = ep->ContextRecord;
	if (!ctx) return false;
	if (PtrInModule((void*)(uintptr_t)ctx->Eip, g_self))
		return true;
	uintptr_t ebp = (uintptr_t)ctx->Ebp;
	for (int i = 0; i < 32; ++i) {
		if (ebp < 0x10000 || (ebp & 3)) break;
		MEMORY_BASIC_INFORMATION mbi{};
		if (!VirtualQuery((void*)ebp, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT)
			break;
		uintptr_t ret = *(uintptr_t*)(ebp + 4);
		if (PtrInModule((void*)ret, g_self))
			return true;
		uintptr_t next = *(uintptr_t*)ebp;
		if (next <= ebp) break;
		ebp = next;
	}
	return false;
}

static const char* CrashBcName(int bc) {
	switch (bc) {
	case kBcPaint: return "HUD paint";
	case kBcEvent: return "game event";
	case kBcHit: return "hit feedback";
	case kBcLevelInit: return "map load";
	case kBcXhair: return "crosshair";
	case kBcWorker: return "worker tick";
	case kBcSettings: return "settings";
	case kBcMenu: return "menu";
	default: return "idle";
	}
}

static LONG WINAPI SkeetoCrashFilter(EXCEPTION_POINTERS* ep) {
	static volatile LONG s_in = 0;
	if (InterlockedCompareExchange(&s_in, 1, 0) != 0)
		return EXCEPTION_CONTINUE_SEARCH;
	const DWORD code = (ep && ep->ExceptionRecord) ? ep->ExceptionRecord->ExceptionCode : 0;
	const bool skip = (code == EXCEPTION_BREAKPOINT || code == EXCEPTION_SINGLE_STEP
		|| code == 0x40010006UL || code == 0x406D1388UL);
	if (!skip && g_optCrashDialog && g_run.load(std::memory_order_relaxed)
		&& CrashLooksLikeOurs(ep)) {
		void* at = ep && ep->ExceptionRecord ? ep->ExceptionRecord->ExceptionAddress : nullptr;
		const uintptr_t base = (uintptr_t)g_self;
		const uintptr_t rva = (at && base && (uintptr_t)at >= base) ? ((uintptr_t)at - base) : 0;
		wchar_t msg[768]{};
		swprintf_s(msg, 768,
			L"Skeeto 判断这次闪退发生在本 DLL 内。\n\n"
			L"版本: %s\n"
			L"异常: 0x%08X\n"
			L"地址: %p  (RVA +0x%X)\n"
			L"当时在做: %hs\n\n"
			L"点确定后可能还会弹出 L4N 的崩溃窗，这是正常的。\n"
			L"请把两边弹窗内容和游戏目录下的 mdmp 一起发给作者。\n"
			L"若不想再看到本窗：控制台 skeeto_crash_dialog 0",
			SKEETO_VERSION_W,
			code,
			at,
			(unsigned)rva,
			CrashBcName(g_crashBc.load(std::memory_order_relaxed)));
		MessageBoxW(nullptr, msg, L"Skeeto",
			MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST | MB_SYSTEMMODAL);
	}
	if (g_prevCrashFilter)
		return g_prevCrashFilter(ep);
	return EXCEPTION_CONTINUE_SEARCH;
}

static void CrashInstallFilter() {
	static bool s_done = false;
	if (s_done) return;
	s_done = true;
	g_prevCrashFilter = SetUnhandledExceptionFilter(&SkeetoCrashFilter);
	Log("crash filter installed prev=%p", (void*)g_prevCrashFilter);
}

static DWORD WINAPI MainThread(LPVOID) {
	char dir[MAX_PATH]{};
	GetModuleFileNameA(g_self, dir, MAX_PATH);
	char* slash = strrchr(dir, '\\');
	if (slash) *slash = 0;

	char logp[MAX_PATH]{};
	snprintf(logp, sizeof(logp), "%s\\skeeto.log", dir);
	g_log = fopen(logp, "a");
	Log("==== Skeeto proxy boot (DIY styles) ====");

	const bool hideSkeeto = CmdLineHasFlag("-hide_skeeto");
	if (hideSkeeto)
		Log("cmdline: -hide_skeeto present — will disable Skeeto after loading necola_orig");

	// Dual-track: always try necola_orig when present (Necola inject path).
	// Standalone inject (skeeto.dll via left4dead2_skeeto.exe) may have no Necola — that is OK.
	char orig[MAX_PATH]{};
	snprintf(orig, sizeof(orig), "%s\\necola_orig.dll", dir);
	HMODULE hOrig = LoadLibraryA(orig);
	if (hOrig) {
		Log("necola_orig.dll loaded (Necola track)");
	} else {
		Log("necola_orig.dll not loaded (%lu)%s", GetLastError(),
			hideSkeeto ? " — hide mode still active (Skeeto off)" : " — Skeeto-only / standalone");
	}

	if (hideSkeeto) {
		Log("Skeeto disabled by -hide_skeeto (no menu/hooks/feedback). Necola_orig load attempted above.");
		return 0;
	}

	for (int i = 0; i < 65; ++i) {
		g_lastGround[i] = 1;
		g_lastAirborneAt[i] = 0;
	}
	ResolveGameDir();
	LoadSettings();
	CrashInstallFilter();
	Log("gameDir=%s (catalog waits for IFileSystem GAME / LevelInit)", g_gameL4d2Dir);

	for (int i = 0; i < 180; ++i) {
		if (GetModuleHandleA("serverbrowser.dll") && GetModuleHandleA("client.dll") && GetModuleHandleA("engine.dll"))
			break;
		Sleep(500);
	}
	Sleep(2500);

	g_engine = GetIface("engine.dll", "VEngineClient013");
	g_events = GetIface("engine.dll", "GAMEEVENTSMANAGER002");
	g_entlist = GetIface("client.dll", "VClientEntityList003");
	g_trace = GetIface("engine.dll", "EngineTraceClient003");
	g_engineSound = GetIface("engine.dll", "IEngineSoundClient003");
	g_cvar = GetIface("vstdlib.dll", "VEngineCvar007");
	Log("engine=%p events=%p entlist=%p trace=%p engSound=%p cvar=%p",
		g_engine, g_events, g_entlist, g_trace, g_engineSound, g_cvar);
	{
		void* surfProbe = GetIface("vguimatsurface.dll", "VGUI_Surface031");
		Log("MatSystemSurface probe=%p (textrial uses EngineVGui::Paint, no Surface VMT)", surfProbe);
	}

	UnlockOverlayCommand();
	UnlockXhairCircleCvars();
	RegisterSkeetoMenuCommand();
	ListenCsEnsure();
	EnsureDefaultMenuBind();
	Log("menu: engine HUD (IEngineVGui::Paint); [ key or: bind <key> skeeto_menu");

	const char* dispPat =
		"55 8B EC 8B 45 08 50 E8 ?? ?? ?? ?? 8B 4D 1C 8B 55 18 51 8B 4D 14 52 8B";
	uintptr_t dispHits[8]{};
	int dispCount = CountPat("client.dll", dispPat, dispHits, 8);
	Log("DispatchParticleEffect pattern hits=%d", dispCount);
	for (int i = 0; i < dispCount && i < 8; ++i)
		Log("  hit[%d]=%p", i, (void*)dispHits[i]);
	uintptr_t disp = dispCount > 0 ? dispHits[0] : 0;
	if (disp && IsExec((void*)disp)) {
		g_dispatch = (DispatchParticleFn)disp;
		Log("DispatchParticleEffect=%p (using first hit)", (void*)disp);
	} else {
		Log("ERROR: DispatchParticleEffect not found");
	}

	uintptr_t prec = FindPat("client.dll",
		"55 8B EC 8B 0D ?? ?? ?? ?? 85 C9 75 07 B8");
	if (prec && IsExec((void*)prec)) {
		g_precache = (PrecacheParticleFn)prec;
		Log("PrecacheParticleSystem=%p", (void*)prec);
	} else {
		Log("ERROR: PrecacheParticleSystem not found");
	}

	uintptr_t findPs = FindPat("client.dll",
		"55 8B EC 51 53 8B 5D 08 56 8B B1 8C");
	if (findPs && IsExec((void*)findPs)) {
		g_findParticle = (FindParticleFn)findPs;
		Log("FindParticleSystem=%p", (void*)findPs);
	} else {
		Log("WARN: FindParticleSystem not found");
	}

	// mov ecx, [CParticleSystemMgr]; from Necola Offsets.cpp
	uintptr_t mgrRef = FindPat("client.dll", "0C 8B 0D ?? ?? ?? ?? 52 50 E8");
	if (mgrRef) {
		uint32_t absPtr = *(uint32_t*)(mgrRef + 3);
		if (absPtr) {
			g_particleMgr = *(void**)absPtr;
			Log("ParticleSystemMgr=%p (via %p)", g_particleMgr, (void*)(uintptr_t)absPtr);
		}
	} else {
		Log("WARN: ParticleSystemMgr pointer not found");
	}

	ResolveNetvars();
	Log("m_fFlags=%d m_vecOrigin=%d m_vecOriginCommon=%d m_iTeamNum=%d m_iHealth=%d m_lifeState=%d m_vecViewOffset=%d m_isIncapacitated=%d owner=%d wepOwner=%d wepId=%d modelIdx=%d",
		g_offFlags, g_offOrigin, g_offOriginCommon, g_offTeam, g_offHealth, g_offLifeState, g_offViewOffset,
		g_offIsIncapacitated, g_offOwnerEntity, g_offWeaponOwner, g_offWeaponID, g_offModelIndex);

	g_ready = true;
	EnsureListeners();
	EnsureClientLevelHooks();
	Log("ready listening=%d", g_listening ? 1 : 0);

	bool wasInGame = false;
	bool wasConnected = false;
	int tick = 0;
	while (g_run.load(std::memory_order_relaxed)) {
		CrashMark(kBcWorker);
		// Quitting / module teardown: stop touching engine interfaces (L4N otherwise
		// reports a "crash" dialog on intentional exit).
		if (!GetModuleHandleA("engine.dll") || !GetModuleHandleA("client.dll"))
			break;
		{
			HWND gw = FindGameWindow();
			if (gw)
				g_sawGameWindow = true;
			else if (g_sawGameWindow)
				break; // Valve001 gone = Host_Shutdown; GetModuleHandle still succeeds
		}
		if (g_engine && !IfaceAlive(g_engine))
			break;

		bool connected = g_engine && EngConnected();
		bool ingame = g_engine && EngInGame();
		TickPurePolicy(connected);
		ListenPump();

		// Catalog + Precache run on LevelInit (game thread, IFileSystem GAME).

		if (ingame && !wasInGame) {
			// No Sleep / catalog reload / particle Precache here — avoid hitch right after spawn.
			g_localUserId = -1;
			MenuForceClose();
			g_siStreak = 0;
			RefreshLocalUserId();
			ResetHealthCache();
			EnsureListeners();
			// round_start often fires before listeners — schedule saferoom after map enter.
			g_offSurvCharServer = -1;
			Log("map enter: listeners only styles=%d ci=%s si=%s localUid=%d",
				DlcGetCatalog()->count,
				DlcGetSelected("ci")[0] ? DlcGetSelected("ci") : "off",
				DlcGetSelected("si")[0] ? DlcGetSelected("si") : "off",
				g_localUserId);
		}
		if (!ingame && wasInGame) {
			g_hitDetectArmed = false;
			g_hitDetectEnableAt = 0;
			g_siStreak = 0;
			MenuForceClose();
			g_loadWarmDone = false;
			XhairHideNow();
			// Saferoom / chapter change often stays connected — next LevelInit warms again.
		}
		if (!connected && wasConnected) {
			ElimResetCounts();
			g_elimLastMap[0] = 0;
		}
		wasInGame = ingame;
		wasConnected = connected;
		// Always tick menu: when leaving a map MenuTick used to stop, so an open overlay stuck forever.
		MenuTick();
		InputFocusTick();
		XhairTick();
		if (ingame && SkeetoFeaturesOn()) {
			if (g_hitDetectEnableAt && GetTickCount() >= g_hitDetectEnableAt && !g_hitDetectArmed.load()) {
				g_hitDetectArmed = true;
				g_hitDetectEnableAt = 0;
				Log("recv-hit ARMED");
			}
			Sample();
			if ((++tick % 40) == 0) RefreshLocalUserId();
			if ((tick % 200) == 0) {
				HWND fgHb = GetForegroundWindow();
				Log("heartbeat calls=%ld local=%d localUid=%d hitArmed=%d streak=%d ci=%s si=%s v=%s pureOff=%d listen=%d tick=%d menu=%d park=%d xpark=%d fgXhair=%d",
					(long)g_fireCalls, EngLocal(), g_localUserId, g_hitDetectArmed.load() ? 1 : 0,
					g_siStreak,
					DlcGetSelected("ci")[0] ? DlcGetSelected("ci") : "off",
					DlcGetSelected("si")[0] ? DlcGetSelected("si") : "off",
					SKEETO_VERSION, SkeetoFeaturesOn() ? 0 : 1,
					ListenIsHost() ? 1 : 0, g_optLocalTick,
					g_menuVisible ? 1 : 0, g_menuParked ? 1 : 0,
					g_xhairParked ? 1 : 0,
					(g_xhairHwnd && fgHb == g_xhairHwnd) ? 1 : 0);
				XhairLogFocus("hb");
			}
		} else {
			++tick;
			// First install only. Never rewrite input VMTs from this worker.
			if (!g_enginePaintHookLogged || !g_levelInitHookLogged)
				EnsureClientLevelHooks();
		}
		Sleep(50);
	}

	g_ready = false;
	MenuForceClose();
	XhairShutdown();
	Log("worker exit (clean shutdown)");
	if (g_log) {
		fflush(g_log);
		fclose(g_log);
		g_log = nullptr;
	}
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		g_self = hModule;
		DisableThreadLibraryCalls(hModule);
		g_run = true;
		if (HANDLE t = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr))
			CloseHandle(t);
	} else if (reason == DLL_PROCESS_DETACH) {
		// Stop worker before CRT/engine teardown. Do not Wait here (DllMain deadlock risk).
		g_run = false;
		g_ready = false;
		(void)lpReserved;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void necola_export() {}
