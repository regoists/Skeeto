#pragma once

// SpotlightVision nest API. Tick is Paint-thread only.
// Menu / Save/Load / SkCmd stay in proxy. Do not put Listen/Xhair/HUD here.
// This header is not a toolbox: no Log, SaveSettings, g_paint*, host.h.

extern bool g_optSpotlightVision;
extern int g_spotlightBrightnessPct;
extern int g_spotlightRadius;
extern int g_spotlightColorR;
extern int g_spotlightColorG;
extern int g_spotlightColorB;
extern int g_spotlightMenuSel;

constexpr int kSpotlightBrightnessMin = 20;
constexpr int kSpotlightBrightnessMax = 200;
constexpr int kSpotlightBrightnessStep = 5;
constexpr int kSpotlightRadiusMin = 250;
constexpr int kSpotlightRadiusMax = 1200;
constexpr int kSpotlightRadiusStep = 50;

struct SpotlightVisionConfig {
	float verticalOffset;
	float fov;
	float nearZ;
	float farZ;
	float attenuation;
	bool shadowEnabled;
};
constexpr SpotlightVisionConfig kSpotlightVisionCfg = { 96.f, 110.f, 8.f, 650.f, 1.f, false };
constexpr int kSpotlightVisionKey = 0x534B544C;

bool SpotlightVisionNeedsTick();
void SpotlightVisionTick();
void SpotlightVisionSetEnabled(bool enabled, bool save);
void SpotlightVisionDestroy();
void SpotlightVisionReset();
void SpotlightVisionSetPreset(int preset);
void SpotlightVisionAdjustSelected(int delta);
