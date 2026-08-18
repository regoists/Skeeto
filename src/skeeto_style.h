// skeeto_style.h — DIY kill-feedback style catalog (VPK / loose)
#pragma once
#include <cstdint>
#include <cstddef>

enum { kDlcMaxStyles = 32, kDlcMaxNamed = 24, kDlcMaxParticleExtra = 8, kDlcMaxSoundExtra = 8 };

struct DlcFx {
	char overlay[96];  // r_screenoverlay path (no materials/, no ext)
	char particle[64]; // DispatchParticleEffect name (empty = skip)
	char particleExtra[kDlcMaxParticleExtra][64]; // optional extras from JSON "particles"
	int particleExtraCount;
	char sound[96];    // play path under sound/
	char soundExtra[kDlcMaxSoundExtra][96]; // optional pool from JSON "sounds" (random pick one)
	int soundExtraCount;
	int priority;
	int overlayMs;     // optional; 0 = use DLL default for that event (hit shorter, etc.)
	bool used;
	bool worldSpace;   // true = spawn at world xyz; false = screen-space (0,0,0)
};

struct DlcNamedFx {
	char key[40];
	DlcFx fx;
};

struct DlcStyle {
	char id[48];
	char name[64];
	char channel[8]; // ci | si | ff | fx
	int sort;
	bool streakEnabled;
	int streakWrap;     // 0 = no wrap
	int streakPriority; // base priority for streak_* events
	DlcFx hit{};
	DlcFx kill{};
	DlcFx headshot{};
	DlcFx melee{};
	DlcFx streak[12]{}; // 1..10 used; [0] unused; [11]=default overflow
	bool streakSlot[12]{};
	DlcNamedFx named[kDlcMaxNamed]{};
	int namedCount;
};

struct DlcCatalog {
	DlcStyle styles[kDlcMaxStyles];
	int count;
	char ciId[48];
	char siId[48];
	char ffId[48];
	char fxId[48];
};

// Load style JSON. Win32 scan of left4dead2\skeeto\ first (cheap).
// Engine FindFirst("skeeto/skeeto_*.json") only when allowEngineGlob (main menu);
// never glob GAME during map load. Legacy particles/skeeto_*.json if both empty.
void DlcReloadCatalog(const char* gameLeft4Dead2Dir, bool allowEngineGlob);
void DlcEnsureCatalog(const char* gameLeft4Dead2Dir, bool allowEngineGlob);
int DlcLastEngineJsonCount();
int DlcLastLooseJsonCount();
int DlcLastReloadMs();
const DlcCatalog* DlcGetCatalog();

const DlcStyle* DlcFindStyle(const char* id);
const DlcStyle* DlcStyleByChannelIndex(const char* channel, int index);
int DlcCountChannel(const char* channel);
const char* DlcChannelNameAt(const char* channel, int index);
const char* DlcChannelIdAt(const char* channel, int index);
int DlcIndexOfSelected(const char* channel); // -1 if off/missing

// Resolve feedback for hit/kill path. kindHint: "hit"|"kill".
bool DlcResolve(const char* channel, const char* kindHint, bool headshot, bool melee,
	int streakCount, DlcFx* out);

// Resolve a named event key (e.g. "skeet", "crown") from channel style.
bool DlcResolveNamed(const char* channel, const char* eventKey, DlcFx* out);

void DlcSetSelected(const char* channel, const char* id);
const char* DlcGetSelected(const char* channel);

// Cycle: off -> style0 -> style1 ... -> off. Returns new id or "off".
const char* DlcCycleChannel(const char* channel);

// Resolve which sound path to play for this FX block.
// No "sounds" → sound (legacy). With "sounds" → random among sounds (+ sound if set).
// Writes into out (size outN); returns out on hit, nullptr if none.
const char* DlcPickSound(const DlcFx* fx, char* out, size_t outN);

// Read a game-relative file via IFileSystem GAME (loose or addon VPK).
// Caller must free(*outData). Returns false on miss.
bool DlcReadGameBinary(const char* gameLeft4Dead2Dir, const char* relPath, void** outData, int* outSize);

// True if Open(relPath, "rb", "GAME") succeeds (loose or VPK). No glob.
bool DlcGameFileExists(const char* relPath);

// Enumerate filenames matching glob via IFileSystem FindFirst (loose + VPK).
// Filename only (e.g. "foo.vmt"). Do not call during map load / per-frame.
int DlcForEachGameGlob(const char* glob, void (*fn)(const char* filename, void* ctx), void* ctx);
