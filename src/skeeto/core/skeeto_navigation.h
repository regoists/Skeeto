#pragma once

// All calls are from Skeeto's existing game/Paint thread. No L4N runtime is used.
namespace skeeto_nav {
constexpr int kSettingCount = 52;
// Supply radar option set: [0,15) general, [15,54) per-label enable,
// [54,210) per-label layout (custom,x,y,scale). Mirrors supplyradar::options (static_assert in runtime).
constexpr int kRadarLabelCount = 39;
constexpr int kRadarItemBase = 15;
constexpr int kRadarLayoutBase = 54;
constexpr int kRadarSettingCount = 210;
bool RadarEnabled();
int RadarGet(int index);
const char* RadarKey(int index);
int RadarMinimum(int index);
int RadarMaximum(int index);
void RadarSet(int index, int value);
void RadarResetDefaults();
bool RadarItemSupported(unsigned label);
const wchar_t* RadarItemName(unsigned label);
void RadarBeginEdit(unsigned label);
bool RadarEditing();
void RadarEndEdit();
void RadarEditKey(unsigned key, bool down);
const wchar_t* RadarStatus();
// One-line diagnostic (state, work/draw cost, counters) for the console command.
unsigned RadarStats(char* out, unsigned capacity);
// Shared NAV lifetime for the minimap, world-route guidance and supply radar.
bool Active();
// Visible map/world/radar consumers; progress sampling alone needs no Surface.
bool PresentationActive();
// The existing master setting controls minimap visibility only.
bool Enabled();
// Paint-owned optional consumer, sharing the NAV and current reference route.
void SetProgressEnabled(bool enabled);
// Listen-host NAV connectivity suspects. Off does no parse, scan, or drawing.
void SetBugScan(bool enabled, bool boxes);
bool BugBoxesWanted();
const wchar_t* BugStatus();
bool BugNearestPoint(float* x, float* y, float* z);
void PaintBug();
int ProgressNotice(); // 0 when inactive, otherwise the crossed threshold
int ProgressPercent(); // cached 0..100, -1 without a valid playable route
bool TakeProgressSound(); // one real threshold crossing, never HUD previews
const wchar_t* ProgressStatus();
unsigned ProgressStats(char* out, unsigned capacity); // on-demand console output
void ProgressRoundBoundary(bool ended); // Event publishes a reset; Paint consumes it
int Get(int index);
const char* Key(int index);
int Minimum(int index);
int Maximum(int index);
void Set(int index, int value);
void ResetDefaults();
void Pump(bool inGame, bool gameUiVisible, bool menuVisible);
// Diagnostic pause: Pump skips all per-frame work but keeps the session loaded (no unload).
void SetPaused(bool paused);
void Paint(bool showMap, bool showWorld, bool showRadar);
// Supply-radar artwork. Game/Paint thread only. False until the icon texture exists.
bool DrawSupplyIcon(unsigned label, int x, int y, int w, int h);
// Same pixel size the supply radar uses for this label at the current screen scale.
bool SupplyIconSize(unsigned label, int screenW, int screenH, float hudScalePercent, int* outW, int* outH);
void Reload();
void InvalidateFonts();
void Shutdown();
unsigned GoalCount();
const wchar_t* Status();
bool Editing();
bool MinimapEditing();
bool WorldHudEditing();
void BeginEdit();
void BeginWorldHudEdit();
void EndEdit();
void EditKey(unsigned key, bool down);
bool ConsumeSettingsChanged();
}
