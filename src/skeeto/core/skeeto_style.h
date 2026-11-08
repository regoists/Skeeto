// skeeto_style.h — DIY kill-feedback style catalog (VPK / loose)
#pragma once
#include <cstdint>
#include <cstddef>

enum {
	kDlcMaxStyles = 128,
	kDlcMaxNamed = 24,
	kDlcMaxParticleExtra = 8,
	kDlcMaxSoundExtra = 8,
	kDlcMaxOverlaySequence = 30
};

struct DlcFx {
	char overlay[96];  // r_screenoverlay path (no materials/, no ext)
	// Optional deterministic sequence. Screen overlays share one engine slot,
	// so these materials are displayed in order rather than layered.
	char overlaySequence[kDlcMaxOverlaySequence][96];
	int overlaySequenceCount;
	int overlayStepMs; // interval between entries; 0 = 40 ms
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
// Synchronous GAME enumeration only when allowEngineGlob (disconnected menu).
// Optional late-mount recovery uses the separate bounded Pump below; never
// enumerate GAME during loading or automatically on every chapter.
void DlcReloadCatalog(const char* gameLeft4Dead2Dir, bool allowEngineGlob);
// Configured admission limit: 64 (default), 96, 128. Changes wait for a safe reload.
int DlcStyleLimit();
void DlcSetStyleLimit(int limit);
bool DlcStyleLimitPending();
// Returns true only if this call actually rebuilt the catalog.
bool DlcEnsureCatalog(const char* gameLeft4Dead2Dir, bool allowEngineGlob);
// True only after a real IFileSystem GAME enumeration was attempted. A loose
// catalog being non-empty does not imply that VPK styles have been discovered.
bool DlcEngineGlobTried();
// Query only; does not enumerate files. Used to avoid consuming the one-shot
// recovery before the Source filesystem interface is available.
bool DlcEngineCatalogReady();
int DlcLastEngineJsonCount();
int DlcLastLooseJsonCount();
int DlcLastReloadMs();
// Optional late-mount recovery on the game/Paint thread. Merges into the current
// catalog, preserving selected IDs. Begin opens one GAME enumeration; each Pump
// visits at most one entry / reads one JSON (existing 8 MiB cap). 4096 entries per
// glob, at most two legacy-compatible globs. Pump: 0 pending, 1 done, -1 failed.
bool DlcCatalogRecoveryBegin();
int DlcCatalogRecoveryPump();
void DlcCatalogRecoveryCancel();
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

// Same as DlcReadGameBinary with an explicit upper bound. Background media uses
// a larger one-shot limit than small feedback JSON/assets. Call only from the
// game/Paint thread because the Source filesystem interface is not worker-safe.
bool DlcReadGameBinaryLimited(const char* gameLeft4Dead2Dir, const char* relPath,
	void** outData, int* outSize, unsigned int maxBytes);

// True if Open(relPath, "rb", "GAME") succeeds (loose or VPK). No glob.
bool DlcGameFileExists(const char* relPath);

// Enumerate filenames matching glob via IFileSystem FindFirst (loose + VPK).
// Filename only (e.g. "foo.vmt"). Do not call during map load / per-frame.
int DlcForEachGameGlob(const char* glob, void (*fn)(const char* filename, void* ctx), void* ctx);

// Same as DlcForEachGameGlob, with an explicit upper bound. Use this for
// engine-startup registration paths so an unexpected asset pack cannot turn a
// precise prefix glob into unbounded work.
int DlcForEachGameGlobLimited(const char* glob, int maxFiles,
	void (*fn)(const char* filename, void* ctx), void* ctx);

// Paint-owned incremental GAME enumeration, including mounted addon VPKs.
// Pump visits at most entryBudget entries; 0=more, 1=end, 2=4096-entry limit.
struct DlcGameGlobScan;
DlcGameGlobScan* DlcGameGlobScanBegin(const char* glob);
int DlcGameGlobScanPump(DlcGameGlobScan* scan, int entryBudget,
	void (*fn)(const char* filename, void* ctx), void* ctx);
void DlcGameGlobScanCancel(DlcGameGlobScan* scan);
void DlcGameGlobScanAbandon(DlcGameGlobScan* scan);

// Incrementally stream one GAME/VPK file into Skeeto's private LocalAppData
// cache without a whole-file allocation or a long single-Paint copy. The cache
// key uses normalized path, size and Source file time. Game thread only.
struct DlcCacheExtract;
DlcCacheExtract* DlcCacheExtractBegin(const char* relPath, const wchar_t* extension,
	unsigned int maxBytes);
// 1=ready and outPath filled, 0=more bounded work remains, -1=failed.
int DlcCacheExtractPump(DlcCacheExtract* extract, unsigned int byteBudget,
	wchar_t* outPath, size_t outCount);
void DlcCacheExtractCancel(DlcCacheExtract* extract);
// Process/worker teardown fallback: releases Win32/heap state without calling
// Source IFileSystem from the wrong thread. The engine owns the abandoned file
// handle and will reclaim it during process teardown.
void DlcCacheExtractAbandon(DlcCacheExtract* extract);

// Edit-keys list section (scripts/kb_act.lst) through a Skeeto-owned head GAME
// search path. Fs*: game thread only, called on state edges (engine filesystem).
// ReadOriginal: 1 ok (caller free()), 0 retry later, -1 permanent (UTF-16 source or
// a Skeeto section already present). KbActFile*: Win32 only, for the worker thread.
int FsKbActReadOriginal(const char* ownDir, char** outBuf, int* outLen);
bool FsKbActSectionPresent(const char* ownDir);
bool FsKbActSectionAtHead(const char* ownDir);
bool FsKbActVerify(const char* expected, int len);
void FsKbActAddPath(const char* ownDir);
void FsKbActRemovePath(const char* ownDir);
bool KbActFileExists(const char* ownDir);
int KbActFileSync(const char* ownDir, const char* data, int len); // 2 unchanged, 1 written, 0 fail
void KbActFileDelete(const char* ownDir);
