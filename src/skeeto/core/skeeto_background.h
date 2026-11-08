#pragma once

#include <Windows.h>

enum SkeetoBackgroundType : int {
	SKEETO_BG_IMAGE = 1,
	SKEETO_BG_VIDEO_MP4 = 2,
	SKEETO_BG_VIDEO_BIK = 3,
};

enum SkeetoBackgroundFit : int {
	SKEETO_BG_COVER = 0,
	SKEETO_BG_CONTAIN = 1,
	SKEETO_BG_STRETCH = 2,
};

struct SkeetoBackgroundFrame {
	// Media Foundation/WIC native 32-bit order: B, G, R, A in memory.
	const unsigned char* bgra;
	int width;
	int height;
	unsigned int serial;
	int fit;
};

struct SkeetoBackgroundNativeVideo {
	// Absolute loose/cache path consumed by the game's native VBik001 player.
	char path[520];
	unsigned int generation;
	int fit;
};

using SkeetoBackgroundLogFn = void(*)(const char* text);

// All public control functions except SignalStop/Shutdown run on Skeeto's Paint
// thread. Source IFileSystem access therefore never escapes to the decoder worker.
void SkeetoBackgroundInitialize(const char* gameLeft4Dead2Dir, SkeetoBackgroundLogFn logFn);
void SkeetoBackgroundSignalStop();
void SkeetoBackgroundShutdown();

void SkeetoBackgroundSetEnabled(bool enabled);
bool SkeetoBackgroundEnabled();
void SkeetoBackgroundSetMenuActive(bool active);

// Startup-only loose-file preload.  It avoids showing the stock BIK while a
// selected MP4 is first discovered/decoded.  No Source filesystem calls.
bool SkeetoBackgroundPreloadLoose();
bool SkeetoBackgroundScan();
bool SkeetoBackgroundReload();
bool SkeetoBackgroundNext();
int SkeetoBackgroundCount();
int SkeetoBackgroundCurrentType();
const char* SkeetoBackgroundCurrentId();
const char* SkeetoBackgroundCurrentName();
void SkeetoBackgroundSelect(const char* id);
void SkeetoBackgroundSetRandomStart(bool enabled);
bool SkeetoBackgroundRandomStart();

void SkeetoBackgroundSetFit(int fit);
int SkeetoBackgroundFitMode();
void SkeetoBackgroundSetVolume(int volumePercent);
int SkeetoBackgroundVolume();

struct SkeetoBackgroundPlayback {
	bool available = false, paused = false, seekable = false;
	LONGLONG position = 0, duration = 0; // source time, 100 ns
};
SkeetoBackgroundPlayback SkeetoBackgroundGetPlayback();
bool SkeetoBackgroundSetPaused(bool paused);
bool SkeetoBackgroundSeek(LONGLONG position);

// Returns the renderer-owned latest frame. The pointer remains valid on the
// Paint thread until the next AcquireFrame/Shutdown call.
bool SkeetoBackgroundAcquireFrame(SkeetoBackgroundFrame* out);
bool SkeetoBackgroundAcquireNativeVideo(SkeetoBackgroundNativeVideo* out);
bool SkeetoBackgroundHasFrame();
bool SkeetoBackgroundLoadFailed();
// Paint-thread diagnostics, updated at most once per second. FPS is multiplied
// by ten so callers do not need floating-point synchronization.
int SkeetoBackgroundPresentedFps10();
int SkeetoBackgroundPlaybackRatePercent();
int SkeetoBackgroundQueueDepth();
int SkeetoBackgroundQueueCapacity();
int SkeetoBackgroundQueueMinimum();
int SkeetoBackgroundQueueUnderruns();

struct SkeetoSafeMusicCatalog {
	unsigned revision;
	int count;
	bool truncated, failed;
	char names[128][256];
};

// Configure/catalog access run on Paint. Lifecycle/Shutdown run on MainThread.
// QueuePlay/Cancel publish bounded requests and never call Source interfaces.
void SkeetoSafeMusicConfigure(const char* gameLeft4Dead2Dir, bool enabled,
	int volumePercent, bool random, const char* selectedUtf8);
void SkeetoSafeMusicLifecyclePump();
// Paint only: GAME/VPK enumeration (16 entries) and selected-file extraction
// (256 KiB) per call. Disable engine work while loading; cancellation still drains.
void SkeetoSafeMusicResourcePump(bool allowEngineAccess = true);
void SkeetoSafeMusicReload();
bool SkeetoSafeMusicGetCatalog(SkeetoSafeMusicCatalog* out);
bool SkeetoSafeMusicQueuePlay();
void SkeetoSafeMusicCancel();
void SkeetoSafeMusicShutdown();

// Fixed Last Man track. Source extraction on Paint; MF/waveOut on its worker.
void SkeetoLastMusicConfigure(bool enabled, int volumePercent);
void SkeetoLastMusicSetActive(bool active);
void SkeetoLastMusicResourcePump(bool allowEngineAccess);
void SkeetoLastMusicReload();
int SkeetoLastMusicState(); // 0 preparing, 1 ready, -1 unavailable
unsigned SkeetoLastMusicStartedGeneration(); // Current request's first submitted PCM, or 0.
void SkeetoLastMusicLifecyclePump(); // MainThread, never Paint
void SkeetoLastMusicShutdown();
