#include "skeeto_host.h"
#include "skeeto_spotlight.h"
#include <atomic>

// SpotlightVision nest. Paint-thread Tick; dlight pointer dies on map change (Reset).
// Do not SaveSettings/ClientCmd from Tick. Menu/SkCmd stay in proxy.

void Log(const char* fmt, ...);
void SaveSettings();
extern std::atomic<bool> g_paintInGame;
extern std::atomic<bool> g_paintLoading;
float SpotlightClampedOffset(const float* eye);

struct SpotlightColorRGBExp32 { uint8_t r, g, b; int8_t exponent; };
struct SpotlightDlight {
	int flags;
	float origin[3];
	float radius;
	SpotlightColorRGBExp32 color;
	float die;
	float decay;
	float minlight;
	int key;
	int style;
	float direction[3];
	float innerAngle;
	float outerAngle;
};
static_assert(sizeof(SpotlightDlight) == 64, "Source dlight_t layout mismatch");
using SpotlightAllocDlightFn = SpotlightDlight*(__thiscall*)(void*, int);

static void* g_spotlightEffects = nullptr;
static SpotlightAllocDlightFn g_spotlightAllocFn = nullptr;
static bool g_spotlightBackendTried = false;
static bool g_spotlightBackendUnavailableLogged = false;
static bool g_spotlightLightLive = false;
static DWORD g_spotlightRetryAt = 0;
static int g_spotlightFaults = 0;
static constexpr float kSpotlightRefreshInterval = 0.033f;

static SpotlightAllocDlightFn SpotlightVisionAllocFn() {
	if (g_spotlightAllocFn) return g_spotlightAllocFn;
	const DWORD now = GetTickCount();
	if (g_spotlightRetryAt && (LONG)(now - g_spotlightRetryAt) < 0) return nullptr;
	g_spotlightRetryAt = 0;
	if (!g_spotlightBackendTried) {
		g_spotlightBackendTried = true;
		g_spotlightEffects = GetIface("engine.dll", "VEngineEffects001");
		if (g_spotlightEffects && IfaceAlive(g_spotlightEffects)) {
			auto fn = (SpotlightAllocDlightFn)VGet(g_spotlightEffects, 4);
			if (fn && IsExec((void*)fn)) {
				g_spotlightAllocFn = fn;
				Log("Spotlight backend initialized (VEngineEffects001 keyed dlight)");
				return fn;
			}
		}
		g_spotlightEffects = nullptr;
		g_spotlightRetryAt = now + 5000;
		if (!g_spotlightBackendUnavailableLogged) {
			g_spotlightBackendUnavailableLogged = true;
			Log("Spotlight backend unavailable");
		}
		return nullptr;
	}
	if (!g_spotlightEffects || !IfaceAlive(g_spotlightEffects)) {
		g_spotlightEffects = nullptr;
		g_spotlightBackendTried = false;
		g_spotlightRetryAt = now + 5000;
		return nullptr;
	}
	auto fn = (SpotlightAllocDlightFn)VGet(g_spotlightEffects, 4);
	if (fn && IsExec((void*)fn)) {
		g_spotlightAllocFn = fn;
		return fn;
	}
	return nullptr;
}

static SpotlightDlight* SpotlightVisionAlloc() {
	auto fn = SpotlightVisionAllocFn();
	if (!fn) return nullptr;
	SpotlightDlight* light = nullptr;
	__try {
		light = fn(g_spotlightEffects, kSpotlightVisionKey);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		++g_spotlightFaults;
		g_spotlightAllocFn = nullptr;
		g_spotlightEffects = nullptr;
		g_spotlightBackendTried = false;
		g_spotlightRetryAt = GetTickCount() + 2000;
		Log("Spotlight alloc fault #%d — retrying", g_spotlightFaults);
	}
	if (!light || !PtrCommitted(light) || !PtrCommitted((uint8_t*)light + sizeof(*light) - 1)) return nullptr;
	return light;
}

void SpotlightVisionDestroy() {
	if (!g_spotlightLightLive) return;
	SpotlightDlight* light = SpotlightVisionAlloc();
	if (light) {
		light->radius = 0.f;
		light->decay = 0.f;
		light->die = EngClientCurTime() - 0.01f;
	}
	g_spotlightLightLive = false;
	Log("Spotlight disabled");
}

void SpotlightVisionReset() {
	// Map transitions clear renderer-owned dlights. Never carry a returned pointer.
	g_spotlightLightLive = false;
}

void SpotlightVisionSetEnabled(bool enabled, bool save) {
	if (g_optSpotlightVision == enabled) {
		if (save) SaveSettings();
		return;
	}
	g_optSpotlightVision = enabled;
	if (!enabled)
		SpotlightVisionDestroy();
	else
		Log("Spotlight enabled");
	if (save) SaveSettings();
}

void SpotlightVisionSetPreset(int preset) {
	switch (preset) {
	case 0: g_spotlightColorR = 235; g_spotlightColorG = 240; g_spotlightColorB = 230; break; // natural
	case 1: g_spotlightColorR = 255; g_spotlightColorG = 218; g_spotlightColorB = 170; break; // warm
	case 2: g_spotlightColorR = 190; g_spotlightColorG = 225; g_spotlightColorB = 255; break; // cool
	default: return;
	}
	SaveSettings();
}

void SpotlightVisionAdjustSelected(int delta) {
	if (delta == 0) return;
	if (g_spotlightMenuSel == 0) {
		g_spotlightBrightnessPct += delta > 0 ? kSpotlightBrightnessStep : -kSpotlightBrightnessStep;
		if (g_spotlightBrightnessPct < kSpotlightBrightnessMin) g_spotlightBrightnessPct = kSpotlightBrightnessMin;
		if (g_spotlightBrightnessPct > kSpotlightBrightnessMax) g_spotlightBrightnessPct = kSpotlightBrightnessMax;
	} else if (g_spotlightMenuSel == 1) {
		g_spotlightRadius += delta > 0 ? kSpotlightRadiusStep : -kSpotlightRadiusStep;
		if (g_spotlightRadius < kSpotlightRadiusMin) g_spotlightRadius = kSpotlightRadiusMin;
		if (g_spotlightRadius > kSpotlightRadiusMax) g_spotlightRadius = kSpotlightRadiusMax;
	} else {
		int* channel = g_spotlightMenuSel == 2 ? &g_spotlightColorR
			: (g_spotlightMenuSel == 3 ? &g_spotlightColorG : &g_spotlightColorB);
		*channel += delta > 0 ? 5 : -5;
		if (*channel < 0) *channel = 0;
		if (*channel > 255) *channel = 255;
	}
	SaveSettings();
}

// Edge-only diagnostics: when the light silently disappears in the field, a
// single "paused" line per reason change in skeeto.log is the cheapest way to
// tell why. Same reason never logs twice in a row; success resets the edge.
static void SpotlightVisionPauseLog(const char* reason) {
	static const char* s_last = nullptr;
	if (s_last == reason) return;
	s_last = reason;
	if (reason) Log("Spotlight paused: %s", reason);
}


bool SpotlightVisionNeedsTick() {
	return g_optSpotlightVision || g_spotlightLightLive;
}

void SpotlightVisionTick() {
	if (!g_optSpotlightVision) {
		SpotlightVisionPauseLog(nullptr);
		SpotlightVisionDestroy();
		return;
	}
	if (!SkeetoFeaturesOn()
		|| !g_paintInGame.load(std::memory_order_relaxed)
		|| g_paintLoading.load(std::memory_order_relaxed)) {
		SpotlightVisionPauseLog("not-in-game");
		SpotlightVisionDestroy();
		return;
	}
	static DWORD s_nextRefreshMs = 0;
	const DWORD nowMs = GetTickCount();
	if (g_spotlightLightLive && s_nextRefreshMs && (LONG)(nowMs - s_nextRefreshMs) < 0) return;
	s_nextRefreshMs = nowMs + (DWORD)(kSpotlightRefreshInterval * 1000.f);
	const int local = EngLocal();
	if (local <= 0 || local > 64 || g_offLifeState < 0 || g_offLifeState > 0x4000
		|| g_offTeam < 0 || g_offTeam > 0x4000) {
		SpotlightVisionPauseLog("player-entity");
		SpotlightVisionDestroy();
		return;
	}
	void* me = EntGet(local);
	if (!EntReadable(me)) {
		SpotlightVisionPauseLog("player-entity");
		SpotlightVisionDestroy();
		return;
	}
	const uint8_t* base = (const uint8_t*)me;
	const int life = *(const uint8_t*)(base + g_offLifeState);
	const int team = *(const int*)(base + g_offTeam);
	int observer = 0;
	if (g_offObserverMode >= 0 && g_offObserverMode <= 0x4000)
		observer = *(const int*)(base + g_offObserverMode);
	if (life != 0 || team != 2 || observer != 0) {
		SpotlightVisionPauseLog("dead-or-spectator");
		SpotlightVisionDestroy();
		return;
	}
	float pos[3]{};
	if (!LocalEyePos(pos)) {
		SpotlightVisionPauseLog("no-eye-position");
		SpotlightVisionDestroy();
		return;
	}
	pos[2] += SpotlightClampedOffset(pos);
	SpotlightDlight* light = SpotlightVisionAlloc();
	if (!light) {
		SpotlightVisionPauseLog("dlight-unavailable");
		return;
	}
	float brightnessScale = (float)g_spotlightBrightnessPct / 100.f;
	int8_t brightnessExponent = 0;
	if (brightnessScale > 1.f) {
		brightnessScale *= 0.5f;
		brightnessExponent = 1;
	}
	const auto scale = [brightnessScale](int c) -> uint8_t {
		float v = (float)c * brightnessScale;
		if (v < 0.f) v = 0.f;
		if (v > 255.f) v = 255.f;
		return (uint8_t)(v + 0.5f);
	};
	light->flags = 0;
	light->origin[0] = pos[0]; light->origin[1] = pos[1]; light->origin[2] = pos[2];
	light->radius = (float)g_spotlightRadius;
	light->color.r = scale(g_spotlightColorR);
	light->color.g = scale(g_spotlightColorG);
	light->color.b = scale(g_spotlightColorB);
	light->color.exponent = brightnessExponent;
	light->die = EngClientCurTime() + 0.20f;
	light->decay = 0.f;
	light->minlight = 0.f;
	light->key = kSpotlightVisionKey;
	light->style = 0;
	light->direction[0] = 0.f; light->direction[1] = 0.f; light->direction[2] = -1.f;
	light->innerAngle = 0.f;
	light->outerAngle = 0.f;
	g_spotlightLightLive = true;
	SpotlightVisionPauseLog(nullptr);
}
