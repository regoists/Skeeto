// skeeto_style.cpp — load/resolve DIY kill-feedback styles
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
#include <shlobj.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cctype>
#include <new>
#include "skeeto_style.h"
#include "skeeto_filesystem.h"

using CreateInterfaceFn = void* (*)(const char*, int*);

static DlcCatalog g_cat{};
static int g_styleLimit = 64;
static bool g_styleLimitPending = false;

int DlcStyleLimit() { return g_styleLimit; }
bool DlcStyleLimitPending() { return g_styleLimitPending; }
void DlcSetStyleLimit(int limit) {
	if (limit != 64 && limit != 96 && limit != 128) limit = 64;
	if (g_styleLimit == limit) return;
	g_styleLimit = limit;
	g_styleLimitPending = true;
}
static IFileSystem* g_fs = nullptr;
static IFileSystem* FsGet();
static int g_lastEngineJson = 0;
static int g_lastLooseJson = 0;
static int g_lastReloadMs = 0;
static bool g_catalogLoaded = false;
static bool g_engineGlobTried = false;
static DWORD g_engineGlobRetryAt = 0;
static int g_engineGlobEmptyTries = 0;

// Addon VPKs join the GAME path after the menu thinks loading finished.
// Two identical samples, or eight changes, then a scan is allowed to stick.
static bool DlcGamePathSettled() {
	IFileSystem* fs = FsGet();
	if (!fs) return false;
	static char paths[4096];
	paths[0] = 0;
	fs->GetSearchPath("GAME", true, paths, (int)sizeof(paths) - 1);
	paths[sizeof(paths) - 1] = 0;
	unsigned hash = 2166136261u;
	for (const char* p = paths; *p; ++p) {
		hash ^= (unsigned char)*p;
		hash *= 16777619u;
	}
	if (!hash) hash = 1;
	static unsigned last = 0;
	static int stable = 0;
	static int waits = 0;
	if (hash == last) {
		if (stable < 2) ++stable;
	} else {
		last = hash;
		stable = 0;
	}
	if (stable >= 2) {
		waits = 0;
		return true;
	}
	if (++waits >= 8) {
		waits = 0;
		stable = 2;
		return true;
	}
	return false;
}

static void* GetIface(const char* mod, const char* name) {
	HMODULE h = GetModuleHandleA(mod);
	if (!h) h = LoadLibraryA(mod);
	if (!h) return nullptr;
	auto fn = (CreateInterfaceFn)GetProcAddress(h, "CreateInterface");
	return fn ? fn(name, nullptr) : nullptr;
}

// ---- tiny JSON helpers (controlled schema only) ----
static const char* SkipWs(const char* p) {
	while (*p && (unsigned char)*p <= 32) ++p;
	return p;
}

static bool MatchKey(const char* p, const char* key, const char** after) {
	p = SkipWs(p);
	if (*p != '"') return false;
	++p;
	size_t n = strlen(key);
	if (strncmp(p, key, n) != 0 || p[n] != '"') return false;
	p = SkipWs(p + n + 1);
	if (*p != ':') return false;
	*after = SkipWs(p + 1);
	return true;
}

static bool ParseString(const char* p, char* out, size_t outN, const char** after) {
	p = SkipWs(p);
	if (*p != '"') return false;
	++p;
	size_t i = 0;
	while (*p && *p != '"') {
		if (*p == '\\' && p[1]) ++p;
		if (i + 1 < outN) out[i++] = *p;
		++p;
	}
	if (*p != '"') return false;
	out[i] = 0;
	*after = p + 1;
	return true;
}

static bool ParseInt(const char* p, int* out, const char** after) {
	p = SkipWs(p);
	char* end = nullptr;
	long v = strtol(p, &end, 10);
	if (end == p) return false;
	*out = (int)v;
	*after = end;
	return true;
}

static bool ParseBool(const char* p, bool* out, const char** after) {
	p = SkipWs(p);
	if (!strncmp(p, "true", 4)) { *out = true; *after = p + 4; return true; }
	if (!strncmp(p, "false", 5)) { *out = false; *after = p + 5; return true; }
	int v = 0;
	if (ParseInt(p, &v, after)) { *out = v != 0; return true; }
	return false;
}

static const char* FindObjectEnd(const char* p) {
	p = SkipWs(p);
	if (*p != '{') return nullptr;
	int depth = 0;
	bool inStr = false;
	for (const char* q = p; *q; ++q) {
		if (inStr) {
			if (*q == '\\' && q[1]) { ++q; continue; }
			if (*q == '"') inStr = false;
			continue;
		}
		if (*q == '"') { inStr = true; continue; }
		if (*q == '{') ++depth;
		else if (*q == '}') {
			if (--depth == 0) return q + 1;
		}
	}
	return nullptr;
}

// Optional JSON string array: ["a","b"]. Caps at maxN. elemCap includes trailing NUL budget.
static bool ParseStringArraySized(const char* p, char* base, int stride, int maxN, int elemCap,
	int* count, const char** after) {
	p = SkipWs(p);
	if (*p != '[') return false;
	++p;
	*count = 0;
	while (*p) {
		p = SkipWs(p);
		if (*p == ']') { ++p; break; }
		if (*p == ',') { ++p; continue; }
		char tmp[128]{};
		const char* next = nullptr;
		if (!ParseString(p, tmp, sizeof(tmp), &next)) {
			++p;
			continue;
		}
		if (*count < maxN && tmp[0] && elemCap > 1) {
			char* slot = base + (*count) * stride;
			strncpy(slot, tmp, (size_t)elemCap - 1);
			slot[elemCap - 1] = 0;
			++(*count);
		}
		p = next;
	}
	*after = p;
	return true;
}

static bool ParseStringArray(const char* p, char out[][64], int maxN, int* count, const char** after) {
	return ParseStringArraySized(p, &out[0][0], 64, maxN, 64, count, after);
}

static bool ExtractFx(const char* objStart, DlcFx* fx) {
	memset(fx, 0, sizeof(*fx));
	const char* end = FindObjectEnd(objStart);
	if (!end) return false;
	const char* p = objStart;
	while (p < end) {
		const char* after = nullptr;
		char tmp[96]{};
		int iv = 0;
		bool bv = false;
		if (MatchKey(p, "overlay", &after) && ParseString(after, tmp, sizeof(tmp), &p)) {
			strncpy(fx->overlay, tmp, sizeof(fx->overlay) - 1);
			fx->used = true;
			continue;
		}
		if (MatchKey(p, "overlay_sequence", &after)
			&& ParseStringArraySized(after, &fx->overlaySequence[0][0], 96,
				kDlcMaxOverlaySequence, 96, &fx->overlaySequenceCount, &p)) {
			if (fx->overlaySequenceCount > 0) fx->used = true;
			continue;
		}
		if (MatchKey(p, "overlay_step_ms", &after) && ParseInt(after, &iv, &p)) {
			if (iv < 0) iv = 0;
			if (iv > 1000) iv = 1000;
			fx->overlayStepMs = iv;
			continue;
		}
		if (MatchKey(p, "particles", &after)
			&& ParseStringArray(after, fx->particleExtra, kDlcMaxParticleExtra, &fx->particleExtraCount, &p)) {
			if (fx->particleExtraCount > 0) fx->used = true;
			continue;
		}
		if (MatchKey(p, "particle", &after) && ParseString(after, tmp, sizeof(tmp), &p)) {
			strncpy(fx->particle, tmp, sizeof(fx->particle) - 1);
			fx->used = true;
			continue;
		}
		// Optional random pool (check before "sound").
		if (MatchKey(p, "sounds", &after)
			&& ParseStringArraySized(after, &fx->soundExtra[0][0], 96, kDlcMaxSoundExtra, 96,
				&fx->soundExtraCount, &p)) {
			if (fx->soundExtraCount > 0) fx->used = true;
			continue;
		}
		if (MatchKey(p, "sound", &after) && ParseString(after, tmp, sizeof(tmp), &p)) {
			strncpy(fx->sound, tmp, sizeof(fx->sound) - 1);
			fx->used = true;
			continue;
		}
		if (MatchKey(p, "priority", &after) && ParseInt(after, &iv, &p)) {
			fx->priority = iv;
			continue;
		}
		// Optional overlay lifetime (ms). Alias: duration / overlay_duration.
		// Omitted or 0 → DLL default (hit shorter; kill/etc. keep legacy ~240ms).
		if ((MatchKey(p, "overlay_ms", &after) || MatchKey(p, "duration", &after)
				|| MatchKey(p, "overlay_duration", &after))
			&& ParseInt(after, &iv, &p)) {
			if (iv < 0) iv = 0;
			if (iv > 5000) iv = 5000;
			fx->overlayMs = iv;
			continue;
		}
		if (MatchKey(p, "world", &after) && ParseBool(after, &bv, &p)) {
			fx->worldSpace = bv;
			continue;
		}
		++p;
	}
	if (fx->overlay[0] || fx->overlaySequenceCount > 0 || fx->particle[0] || fx->sound[0]
		|| fx->particleExtraCount > 0 || fx->soundExtraCount > 0)
		fx->used = true;
	return fx->used;
}

static bool IsReservedStyleKey(const char* key) {
	return !_stricmp(key, "id") || !_stricmp(key, "name") || !_stricmp(key, "channel")
		|| !_stricmp(key, "sort") || !_stricmp(key, "streak") || !_stricmp(key, "note")
		|| !_stricmp(key, "hit") || !_stricmp(key, "kill") || !_stricmp(key, "headshot")
		|| !_stricmp(key, "melee") || !_stricmp(key, "streak_default")
		|| !_strnicmp(key, "streak_", 7);
}

static bool ParseStyleJson(const char* json, DlcStyle* st) {
	memset(st, 0, sizeof(*st));
	const char* p = json;
	const char* after = nullptr;
	char tmp[96]{};
	int iv = 0;
	bool bv = false;

	// top-level fields
	const char* q = json;
	while (*q) {
		if (MatchKey(q, "id", &after) && ParseString(after, tmp, sizeof(tmp), &q))
			strncpy(st->id, tmp, sizeof(st->id) - 1);
		else if (MatchKey(q, "name", &after) && ParseString(after, tmp, sizeof(tmp), &q))
			strncpy(st->name, tmp, sizeof(st->name) - 1);
		else if (MatchKey(q, "channel", &after) && ParseString(after, tmp, sizeof(tmp), &q))
			strncpy(st->channel, tmp, sizeof(st->channel) - 1);
		else if (MatchKey(q, "sort", &after) && ParseInt(after, &iv, &q))
			st->sort = iv;
		else
			++q;
	}

	// streak block
	q = json;
	while (*q) {
		if (MatchKey(q, "streak", &after)) {
			after = SkipWs(after);
			const char* end = FindObjectEnd(after);
			if (!end) break;
			const char* s = after;
			while (s < end) {
				if (MatchKey(s, "enabled", &after) && ParseBool(after, &bv, &s))
					st->streakEnabled = bv;
				else if (MatchKey(s, "wrap", &after) && ParseInt(after, &iv, &s))
					st->streakWrap = iv;
				else if (MatchKey(s, "priority", &after) && ParseInt(after, &iv, &s))
					st->streakPriority = iv;
				else
					++s;
			}
			break;
		}
		++q;
	}

	auto grabEvent = [&](const char* key, DlcFx* dst) {
		const char* t = json;
		while (*t) {
			if (MatchKey(t, key, &after)) {
				after = SkipWs(after);
				if (*after == '{')
					ExtractFx(after, dst);
				return;
			}
			++t;
		}
	};
	grabEvent("hit", &st->hit);
	grabEvent("kill", &st->kill);
	grabEvent("headshot", &st->headshot);
	grabEvent("melee", &st->melee);

	for (int i = 1; i <= 10; ++i) {
		char key[16];
		snprintf(key, sizeof(key), "streak_%d", i);
		DlcFx fx{};
		grabEvent(key, &fx);
		if (fx.used) {
			st->streak[i] = fx;
			if (!st->streak[i].priority)
				st->streak[i].priority = st->streakPriority ? st->streakPriority : 10;
			st->streakSlot[i] = true;
		}
	}
	{
		DlcFx fx{};
		grabEvent("streak_default", &fx);
		if (fx.used) {
			st->streak[11] = fx;
			st->streakSlot[11] = true;
		}
	}

	// Extra named events (skeet/crown/etc.) — any other top-level object keys.
	q = json;
	while (*q && st->namedCount < kDlcMaxNamed) {
		q = SkipWs(q);
		if (*q != '"') { ++q; continue; }
		char key[40]{};
		const char* afterKey = nullptr;
		if (!ParseString(q, key, sizeof(key), &afterKey)) { ++q; continue; }
		afterKey = SkipWs(afterKey);
		if (*afterKey != ':') { q = afterKey; continue; }
		afterKey = SkipWs(afterKey + 1);
		if (*afterKey != '{') { q = afterKey; continue; }
		if (IsReservedStyleKey(key)) {
			q = FindObjectEnd(afterKey);
			if (!q) break;
			continue;
		}
		DlcFx fx{};
		if (ExtractFx(afterKey, &fx) && fx.used) {
			strncpy(st->named[st->namedCount].key, key, sizeof(st->named[0].key) - 1);
			st->named[st->namedCount].fx = fx;
			++st->namedCount;
		}
		q = FindObjectEnd(afterKey);
		if (!q) break;
	}

	if (!st->id[0] || !st->channel[0])
		return false;
	if (!st->name[0])
		strncpy(st->name, st->id, sizeof(st->name) - 1);
	return true;
}

static bool ParseStyleJsonGuarded(const char* json, DlcStyle* st) {
	if (!json || !st) return false;
#ifdef _MSC_VER
	// Style JSON is user/workshop supplied and parsed only while rebuilding the
	// catalog.  A malformed buffer must disable that one style, never take down
	// the client during startup.
	__try {
		return ParseStyleJson(json, st);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		memset(st, 0, sizeof(*st));
		return false;
	}
#else
	return ParseStyleJson(json, st);
#endif
}

static IFileSystem* FsGet() {
	if (!g_fs)
		g_fs = (IFileSystem*)GetIface("filesystem_stdio.dll", "VFileSystem018");
	return g_fs;
}

// Necola InGameMenu::LoadConfigOptions: Open / Read / Close on path ID GAME.
static bool FsReadGAME(const char* relPath, char** outBuf, int* outLen,
	unsigned int maxBytes = 8u * 1024u * 1024u) {
	*outBuf = nullptr;
	*outLen = 0;
	IFileSystem* fs = FsGet();
	if (!fs || !relPath || !relPath[0]) return false;
	FileHandle_t f = fs->Open(relPath, "rb", "GAME");
	if (!f || f == SKEETO_FS_INVALID_HANDLE) return false;
	unsigned sz = fs->Size(f);
	if (sz == 0 || sz > maxBytes || sz > 0x7fffffffu) {
		fs->Close(f);
		return false;
	}
	char* buf = (char*)malloc(sz + 1);
	if (!buf) {
		fs->Close(f);
		return false;
	}
	int rd = fs->Read(buf, (int)sz, f);
	fs->Close(f);
	if (rd <= 0) {
		free(buf);
		return false;
	}
	buf[rd] = 0;
	*outBuf = buf;
	*outLen = rd;
	return true;
}

static bool LooksLikeStyleJsonName(const char* n) {
	if (!n || _strnicmp(n, "skeeto_", 7) != 0) return false;
	const size_t len = strlen(n);
	return len > 5 && !_stricmp(n + len - 5, ".json");
}

static void AddStyleFromJson(const char* json);

static int ScanEngineGlob(const char* glob, const char* dirPrefix) {
	IFileSystem* fs = FsGet();
	if (!fs) return 0;
	// Keep enumeration in the same Source search-path domain used below by
	// FsReadGAME.  Plain FindFirst relies on the filesystem's implicit/default
	// path and can omit a subscribed workshop VPK even though Open(..., "GAME")
	// would read it once named.  FindFirstEx does not add a scan; it only makes
	// this one bounded scan enumerate the intended GAME mounts.
	FileFindHandle_t handle{};
	const char* name = fs->FindFirstEx(glob, "GAME", &handle);
	if (!name) return 0;
	int n = 0;
	while (name) {
		if (LooksLikeStyleJsonName(name)) {
			char full[260]{};
			snprintf(full, sizeof(full), "%s%s", dirPrefix, name);
			char* buf = nullptr;
			int len = 0;
			if (FsReadGAME(full, &buf, &len) && buf) {
				AddStyleFromJson(buf);
				free(buf);
				++n;
			}
		}
		name = fs->FindNext(handle);
	}
	fs->FindClose(handle);
	return n;
}

static bool ReadWholeFileDisk(const char* path, char** outBuf, int* outLen,
	unsigned int maxBytes = 8u * 1024u * 1024u) {

	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) return false;
	DWORD sz = GetFileSize(h, nullptr);
	if (sz == INVALID_FILE_SIZE || sz == 0 || sz > maxBytes || sz > 0x7fffffffu) {
		CloseHandle(h);
		return false;
	}
	char* buf = (char*)malloc(sz + 1);
	if (!buf) { CloseHandle(h); return false; }
	DWORD rd = 0;
	if (!ReadFile(h, buf, sz, &rd, nullptr) || rd != sz) {
		free(buf);
		CloseHandle(h);
		return false;
	}
	CloseHandle(h);
	buf[sz] = 0;
	*outBuf = buf;
	*outLen = (int)sz;
	return true;
}

static void AddStyleFromJson(const char* json) {
	DlcStyle st{};
	if (!ParseStyleJsonGuarded(json, &st)) return;
	// de-dupe by id
	for (int i = 0; i < g_cat.count; ++i) {
		if (!_stricmp(g_cat.styles[i].id, st.id)) {
			g_cat.styles[i] = st;
			return;
		}
	}
	if (g_cat.count < g_styleLimit) g_cat.styles[g_cat.count++] = st;
}

static void ScanLooseDir(const char* dir) {
	char pattern[MAX_PATH]{};
	snprintf(pattern, sizeof(pattern), "%s\\skeeto_*.json", dir);
	WIN32_FIND_DATAA fd{};
	HANDLE h = FindFirstFileA(pattern, &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		char path[MAX_PATH]{};
		snprintf(path, sizeof(path), "%s\\%s", dir, fd.cFileName);
		char* buf = nullptr;
		int len = 0;
		if (ReadWholeFileDisk(path, &buf, &len) && buf) {
			AddStyleFromJson(buf);
			free(buf);
		}
	} while (FindNextFileA(h, &fd));
	FindClose(h);
}

static int CmpStyle(const void* a, const void* b) {
	const DlcStyle* x = (const DlcStyle*)a;
	const DlcStyle* y = (const DlcStyle*)b;
	if (x->sort != y->sort) return x->sort - y->sort; // lower sort first
	return _stricmp(x->name, y->name);
}

void DlcReloadCatalog(const char* gameLeft4Dead2Dir, bool allowEngineGlob) {
	if (allowEngineGlob && g_cat.count == 0 && !DlcGamePathSettled()) {
		g_engineGlobRetryAt = GetTickCount() + 500;
		return;
	}
	const DWORD t0 = GetTickCount();
	char ci[48]{}, si[48]{}, ff[48]{}, fx[48]{};
	strncpy(ci, g_cat.ciId, sizeof(ci) - 1);
	strncpy(si, g_cat.siId, sizeof(si) - 1);
	strncpy(ff, g_cat.ffId, sizeof(ff) - 1);
	strncpy(fx, g_cat.fxId, sizeof(fx) - 1);

	// Entries past count are inaccessible; do not touch unused capacity pages.
	g_cat.count = 0;
	strncpy(g_cat.ciId, ci, sizeof(g_cat.ciId) - 1);
	strncpy(g_cat.siId, si, sizeof(g_cat.siId) - 1);
	strncpy(g_cat.ffId, ff, sizeof(g_cat.ffId) - 1);
	strncpy(g_cat.fxId, fx, sizeof(g_cat.fxId) - 1);

	g_lastEngineJson = 0;
	g_lastLooseJson = 0;

	// Loose disk first (cheap). Never glob GAME during map load — FindFirst on a
	// huge addon/VPK search path (e.g. 终极特感训练) can stall the loading bar for seconds
	// per call, and Paint used to call this every frame.
	if (gameLeft4Dead2Dir && gameLeft4Dead2Dir[0]) {
		char skeetoDir[MAX_PATH]{};
		snprintf(skeetoDir, sizeof(skeetoDir), "%s\\skeeto", gameLeft4Dead2Dir);
		const int before = g_cat.count;
		ScanLooseDir(skeetoDir);
		g_lastLooseJson = g_cat.count - before;
	}

	if (allowEngineGlob && FsGet()) {
		g_lastEngineJson += ScanEngineGlob("skeeto/skeeto_*.json", "skeeto/");
		if (g_lastEngineJson <= 0 && g_lastLooseJson <= 0)
			g_lastEngineJson += ScanEngineGlob("particles/skeeto_*.json", "particles/");
		if (g_lastEngineJson > 0) {
			g_engineGlobTried = true;
			g_engineGlobEmptyTries = 0;
		} else if (++g_engineGlobEmptyTries >= 4) {
			g_engineGlobTried = true;
		} else {
			g_engineGlobRetryAt = GetTickCount() + 500;
		}
	}

	if (g_cat.count > 1)
		qsort(g_cat.styles, (size_t)g_cat.count, sizeof(DlcStyle), CmpStyle);
	g_catalogLoaded = true;
	g_styleLimitPending = false;
	g_lastReloadMs = (int)(GetTickCount() - t0);
}

bool DlcEnsureCatalog(const char* gameLeft4Dead2Dir, bool allowEngineGlob) {
	if (allowEngineGlob) {
		// A successful loose scan is not proof that GAME/VPK styles have been
		// discovered.  Keep the full enumeration as a separate one-time state.
		if (g_engineGlobTried && !g_styleLimitPending) return false;
		const DWORD now = GetTickCount();
		if (g_engineGlobRetryAt && (LONG)(now - g_engineGlobRetryAt) < 0)
			return false;
		if (!FsGet()) {
			// Engine startup can expose Paint before filesystem_stdio is ready.
			// Do not turn that transient condition into a per-Paint interface probe.
			g_engineGlobRetryAt = now + 500;
			return false;
		}
		g_engineGlobRetryAt = 0;
		DlcReloadCatalog(gameLeft4Dead2Dir, true);
		return true;
	}
	if (g_catalogLoaded) return false;
	DlcReloadCatalog(gameLeft4Dead2Dir, allowEngineGlob);
	return true;
}

bool DlcEngineGlobTried() { return g_engineGlobTried; }
bool DlcEngineCatalogReady() { return FsGet() != nullptr; }
int DlcLastEngineJsonCount() { return g_lastEngineJson; }
int DlcLastLooseJsonCount() { return g_lastLooseJson; }
int DlcLastReloadMs() { return g_lastReloadMs; }

const DlcCatalog* DlcGetCatalog() { return &g_cat; }

const DlcStyle* DlcFindStyle(const char* id) {
	if (!id || !id[0] || !_stricmp(id, "off")) return nullptr;
	for (int i = 0; i < g_cat.count; ++i)
		if (!_stricmp(g_cat.styles[i].id, id))
			return &g_cat.styles[i];
	return nullptr;
}

int DlcCountChannel(const char* channel) {
	int n = 0;
	for (int i = 0; i < g_cat.count; ++i)
		if (!_stricmp(g_cat.styles[i].channel, channel))
			++n;
	return n;
}

const char* DlcChannelIdAt(const char* channel, int index) {
	int n = 0;
	for (int i = 0; i < g_cat.count; ++i) {
		if (_stricmp(g_cat.styles[i].channel, channel)) continue;
		if (n == index) return g_cat.styles[i].id;
		++n;
	}
	return nullptr;
}

const char* DlcChannelNameAt(const char* channel, int index) {
	int n = 0;
	for (int i = 0; i < g_cat.count; ++i) {
		if (_stricmp(g_cat.styles[i].channel, channel)) continue;
		if (n == index) return g_cat.styles[i].name;
		++n;
	}
	return nullptr;
}

const DlcStyle* DlcStyleByChannelIndex(const char* channel, int index) {
	const char* id = DlcChannelIdAt(channel, index);
	return id ? DlcFindStyle(id) : nullptr;
}

void DlcSetSelected(const char* channel, const char* id) {
	if (!channel) return;
	char* dst = nullptr;
	if (!_stricmp(channel, "ci")) dst = g_cat.ciId;
	else if (!_stricmp(channel, "si")) dst = g_cat.siId;
	else if (!_stricmp(channel, "ff")) dst = g_cat.ffId;
	else if (!_stricmp(channel, "fx")) dst = g_cat.fxId;
	if (!dst) return;
	if (!id || !_stricmp(id, "off")) dst[0] = 0;
	else {
		strncpy(dst, id, 47);
		dst[47] = 0;
	}
}

const char* DlcGetSelected(const char* channel) {
	if (!channel) return "";
	if (!_stricmp(channel, "ci")) return g_cat.ciId;
	if (!_stricmp(channel, "si")) return g_cat.siId;
	if (!_stricmp(channel, "ff")) return g_cat.ffId;
	if (!_stricmp(channel, "fx")) return g_cat.fxId;
	return "";
}

int DlcIndexOfSelected(const char* channel) {
	const char* sel = DlcGetSelected(channel);
	if (!sel || !sel[0]) return -1;
	const int n = DlcCountChannel(channel);
	for (int i = 0; i < n; ++i) {
		const char* id = DlcChannelIdAt(channel, i);
		if (id && !_stricmp(id, sel)) return i;
	}
	return -1;
}

const char* DlcCycleChannel(const char* channel) {
	const int n = DlcCountChannel(channel);
	if (n <= 0) {
		// A late/disabled workshop pack is not a request to erase the saved ID.
		const char* selected = DlcGetSelected(channel);
		return selected[0] ? selected : "off";
	}
	int idx = DlcIndexOfSelected(channel);
	if (idx < 0) {
		const char* id = DlcChannelIdAt(channel, 0);
		DlcSetSelected(channel, id);
		return id ? id : "off";
	}
	if (idx + 1 >= n) {
		DlcSetSelected(channel, "off");
		return "off";
	}
	const char* id = DlcChannelIdAt(channel, idx + 1);
	DlcSetSelected(channel, id);
	return id ? id : "off";
}

static void Consider(DlcFx* best, const DlcFx* cand) {
	if (!cand || !cand->used) return;
	if (!cand->overlay[0] && cand->overlaySequenceCount <= 0
		&& !cand->particle[0] && !cand->sound[0] && cand->particleExtraCount <= 0 && cand->soundExtraCount <= 0)
		return;
	if (!best->used || cand->priority > best->priority)
		*best = *cand;
}

bool DlcResolve(const char* channel, const char* kindHint, bool headshot, bool melee,
	int streakCount, DlcFx* out) {
	memset(out, 0, sizeof(*out));
	const char* sel = DlcGetSelected(channel);
	const DlcStyle* st = DlcFindStyle(sel);
	if (!st) return false;

	DlcFx best{};
	const bool wantHit = kindHint && !_stricmp(kindHint, "hit");

	if (wantHit) {
		Consider(&best, &st->hit);
		if (!best.used) return false;
		*out = best;
		return true;
	}

	// Kill path: shared streak is the base (gun/HS/melee all inherit one counter).
	// melee/headshot only override when their priority is STRICTLY higher (CF knifed/HS).
	if (st->streakEnabled) {
		int n = streakCount;
		if (st->streakWrap > 0) {
			if (n <= 0) n = 1;
			while (n > st->streakWrap) n -= st->streakWrap;
		}
		if (n >= 1 && n <= 10 && st->streakSlot[n])
			Consider(&best, &st->streak[n]);
		else if (st->streakSlot[11])
			Consider(&best, &st->streak[11]);
		else
			Consider(&best, &st->kill);
	} else {
		Consider(&best, &st->kill);
	}

	if (melee)
		Consider(&best, &st->melee);
	if (headshot)
		Consider(&best, &st->headshot);

	if (!best.used) return false;
	*out = best;
	return true;
}

bool DlcResolveNamed(const char* channel, const char* eventKey, DlcFx* out) {
	memset(out, 0, sizeof(*out));
	if (!eventKey || !eventKey[0]) return false;
	const DlcStyle* st = DlcFindStyle(DlcGetSelected(channel));
	if (!st) return false;
	for (int i = 0; i < st->namedCount; ++i) {
		if (!_stricmp(st->named[i].key, eventKey) && st->named[i].fx.used) {
			*out = st->named[i].fx;
			return true;
		}
	}
	return false;
}

const char* DlcPickSound(const DlcFx* fx, char* out, size_t outN) {
	if (!out || outN == 0) return nullptr;
	out[0] = 0;
	if (!fx) return nullptr;

	const char* cands[1 + kDlcMaxSoundExtra];
	int n = 0;
	if (fx->soundExtraCount > 0) {
		for (int i = 0; i < fx->soundExtraCount; ++i) {
			if (fx->soundExtra[i][0])
				cands[n++] = fx->soundExtra[i];
		}
		// Include singular "sound" in the pool when both are set.
		if (fx->sound[0])
			cands[n++] = fx->sound;
	} else if (fx->sound[0]) {
		strncpy(out, fx->sound, outN - 1);
		out[outN - 1] = 0;
		return out;
	}
	if (n <= 0) return nullptr;
	if (n == 1) {
		strncpy(out, cands[0], outN - 1);
		out[outN - 1] = 0;
		return out;
	}
	static unsigned s_rng = 0xA5A5u;
	s_rng = s_rng * 1664525u + 1013904223u;
	const unsigned idx = s_rng % (unsigned)n;
	strncpy(out, cands[idx], outN - 1);
	out[outN - 1] = 0;
	return out;
}

bool DlcReadGameBinary(const char* gameLeft4Dead2Dir, const char* relPath, void** outData, int* outSize) {
	return DlcReadGameBinaryLimited(gameLeft4Dead2Dir, relPath, outData, outSize,
		8u * 1024u * 1024u);
}

bool DlcReadGameBinaryLimited(const char* gameLeft4Dead2Dir, const char* relPath,
	void** outData, int* outSize, unsigned int maxBytes) {
	*outData = nullptr;
	*outSize = 0;
	if (!relPath || !relPath[0] || maxBytes == 0 || maxBytes > 0x7fffffffu) return false;

	char norm[260]{};
	strncpy(norm, relPath, sizeof(norm) - 1);
	for (char* c = norm; *c; ++c)
		if (*c == '\\') *c = '/';
	while (norm[0] == '/')
		memmove(norm, norm + 1, strlen(norm));

	char* buf = nullptr;
	int len = 0;
	if (FsReadGAME(norm, &buf, &len, maxBytes) && buf && len > 0) {
		*outData = buf;
		*outSize = len;
		return true;
	}

	if (gameLeft4Dead2Dir && gameLeft4Dead2Dir[0]) {
		char disk[MAX_PATH]{};
		snprintf(disk, sizeof(disk), "%s\\%s", gameLeft4Dead2Dir, norm);
		for (char* c = disk; *c; ++c)
			if (*c == '/') *c = '\\';
		if (ReadWholeFileDisk(disk, &buf, &len, maxBytes) && buf && len > 0) {
			*outData = buf;
			*outSize = len;
			return true;
		}
	}
	return false;
}

bool DlcGameFileExists(const char* relPath) {
	if (!relPath || !relPath[0]) return false;
	IFileSystem* fs = FsGet();
	if (!fs) return false;
	FileHandle_t f = fs->Open(relPath, "rb", "GAME");
	if (!f || f == SKEETO_FS_INVALID_HANDLE) return false;
	fs->Close(f);
	return true;
}

int DlcForEachGameGlob(const char* glob, void (*fn)(const char* filename, void* ctx), void* ctx) {
	if (!glob || !glob[0] || !fn) return 0;
	IFileSystem* fs = FsGet();
	if (!fs) return 0;
	// FindFirstEx GAME matches the style JSON scan. Caller must not use this
	// during map load / per-frame — xhair list refresh is menu-key only.
	FileFindHandle_t handle{};
	const char* name = fs->FindFirstEx(glob, "GAME", &handle);
	if (!name) return 0;
	int n = 0;
	while (name) {
		if (!fs->FindIsDirectory(handle) && name[0] && name[0] != '.') {
			fn(name, ctx);
			++n;
		}
		name = fs->FindNext(handle);
	}
	fs->FindClose(handle);
	return n;
}

int DlcForEachGameGlobLimited(const char* glob, int maxFiles,
	void (*fn)(const char* filename, void* ctx), void* ctx) {
	if (!glob || !glob[0] || !fn || maxFiles <= 0) return 0;
	IFileSystem* fs = FsGet();
	if (!fs) return 0;
	FileFindHandle_t handle{};
	const char* name = fs->FindFirstEx(glob, "GAME", &handle);
	if (!name) return 0;
	int n = 0;
	while (name && n < maxFiles) {
		if (!fs->FindIsDirectory(handle) && name[0] && name[0] != '.') {
			fn(name, ctx);
			++n;
		}
		if (n >= maxFiles) break;
		name = fs->FindNext(handle);
	}
	fs->FindClose(handle);
	return n;
}

struct DlcGameGlobScan {
	IFileSystem* fs = nullptr;
	FileFindHandle_t handle{};
	char current[260]{};
	int visited = 0;
	bool open = false;
};

void DlcGameGlobScanCancel(DlcGameGlobScan* scan) {
	if (!scan) return;
	if (scan->open && scan->fs) scan->fs->FindClose(scan->handle);
	delete scan;
}

void DlcGameGlobScanAbandon(DlcGameGlobScan* scan) {
	if (!scan) return;
	scan->open = false;
	DlcGameGlobScanCancel(scan);
}

DlcGameGlobScan* DlcGameGlobScanBegin(const char* glob) {
	if (!glob || !glob[0]) return nullptr;
	IFileSystem* fs = FsGet();
	if (!fs) return nullptr;
	DlcGameGlobScan* scan = new(std::nothrow) DlcGameGlobScan();
	if (!scan) return nullptr;
	scan->fs = fs;
	const char* name = fs->FindFirstEx(glob, "GAME", &scan->handle);
	if (name) {
		scan->open = true;
		strncpy_s(scan->current, name, _TRUNCATE);
	}
	return scan;
}

int DlcGameGlobScanPump(DlcGameGlobScan* scan, int entryBudget,
	void (*fn)(const char* filename, void* ctx), void* ctx) {
	if (!scan || !fn || entryBudget < 1) return -1;
	if (entryBudget > 64) entryBudget = 64;
	for (int entry = 0; entry < entryBudget && scan->current[0]; ++entry) {
		if (scan->visited >= 4096) return 2;
		++scan->visited;
		if (!scan->fs->FindIsDirectory(scan->handle)) fn(scan->current, ctx);
		const char* next = scan->fs->FindNext(scan->handle);
		scan->current[0] = 0;
		if (next) strncpy_s(scan->current, next, _TRUNCATE);
	}
	return scan->current[0] ? 0 : 1;
}

struct DlcCatalogRecovery {
	DlcGameGlobScan* scan = nullptr;
	int json = 0;
	int elapsedMs = 0;
	bool legacy = false;
};
static DlcCatalogRecovery g_catalogRecovery;

void DlcCatalogRecoveryCancel() {
	DlcGameGlobScanCancel(g_catalogRecovery.scan);
	g_catalogRecovery = DlcCatalogRecovery{};
}

bool DlcCatalogRecoveryBegin() {
	const DWORD t0 = GetTickCount();
	// Keep a capacity change pending until the new scan finishes. If cancelled,
	// the ordinary menu path can still rebuild it; never erase the live catalog
	// or acknowledge the setting merely because recovery started.
	if (g_cat.count > g_styleLimit) g_cat.count = g_styleLimit;
	g_catalogRecovery = DlcCatalogRecovery{};
	g_catalogRecovery.scan = DlcGameGlobScanBegin("skeeto/skeeto_*.json");
	g_catalogRecovery.elapsedMs = (int)(GetTickCount() - t0);
	return g_catalogRecovery.scan != nullptr;
}

static void DlcCatalogRecoveryRead(const char* name, void*) {
	if (!LooksLikeStyleJsonName(name)) return;
	char path[260]{};
	snprintf(path, sizeof(path), "%s%s", g_catalogRecovery.legacy ? "particles/" : "skeeto/", name);
	char* json = nullptr;
	int length = 0;
	if (FsReadGAME(path, &json, &length) && json) {
		AddStyleFromJson(json);
		free(json);
		++g_catalogRecovery.json;
	}
}

int DlcCatalogRecoveryPump() {
	const DWORD t0 = GetTickCount();
	int result = DlcGameGlobScanPump(g_catalogRecovery.scan, 1, &DlcCatalogRecoveryRead, nullptr);
	if (result == 1 && !g_catalogRecovery.legacy && !g_catalogRecovery.json && !g_cat.count) {
		DlcGameGlobScanCancel(g_catalogRecovery.scan);
		g_catalogRecovery.legacy = true;
		g_catalogRecovery.scan = DlcGameGlobScanBegin("particles/skeeto_*.json");
		result = g_catalogRecovery.scan ? 0 : -1;
	}
	if (result != 0) {
		DlcGameGlobScanCancel(g_catalogRecovery.scan);
		g_catalogRecovery.scan = nullptr;
		if (g_cat.count > 1)
			qsort(g_cat.styles, (size_t)g_cat.count, sizeof(DlcStyle), CmpStyle);
		if (result == 1) {
			if (g_catalogRecovery.json > 0) {
				g_engineGlobTried = true;
				g_engineGlobEmptyTries = 0;
			} else if (++g_engineGlobEmptyTries >= 4) {
				g_engineGlobTried = true;
			} else {
				g_engineGlobRetryAt = GetTickCount() + 500;
			}
			g_catalogLoaded = true;
			g_styleLimitPending = false;
		}
		g_lastEngineJson = g_catalogRecovery.json;
	}
	g_catalogRecovery.elapsedMs += (int)(GetTickCount() - t0);
	if (result != 0) g_lastReloadMs = g_catalogRecovery.elapsedMs;
	return result == 0 ? 0 : result == 1 ? 1 : -1;
}

struct DlcCacheExtract {
	IFileSystem* fs = nullptr;
	FileHandle_t source = nullptr;
	HANDLE dest = INVALID_HANDLE_VALUE;
	unsigned char* buffer = nullptr;
	unsigned int size = 0;
	unsigned int copied = 0;
	wchar_t finalPath[520]{};
	wchar_t tempPath[520]{};
	bool tempActive = false;
	bool ready = false;
	bool failed = false;
};

void DlcCacheExtractCancel(DlcCacheExtract* extract) {
	if (!extract) return;
	if (extract->source && extract->fs) extract->fs->Close(extract->source);
	if (extract->dest != INVALID_HANDLE_VALUE) CloseHandle(extract->dest);
	free(extract->buffer);
	if (extract->tempActive && extract->tempPath[0]) DeleteFileW(extract->tempPath);
	delete extract;
}

void DlcCacheExtractAbandon(DlcCacheExtract* extract) {
	if (!extract) return;
	// Never cross threads into VFileSystem018 during shutdown. The source handle
	// is intentionally left to engine/process teardown; all Skeeto-owned state is
	// still released and an incomplete private temp file is removed.
	extract->source = nullptr;
	extract->fs = nullptr;
	DlcCacheExtractCancel(extract);
}

DlcCacheExtract* DlcCacheExtractBegin(const char* relPath, const wchar_t* extension,
	unsigned int maxBytes) {
	if (!relPath || !relPath[0] || !extension || extension[0] != L'.' || !maxBytes)
		return nullptr;
	char norm[260]{};
	strncpy(norm, relPath, sizeof(norm) - 1);
	for (char* c = norm; *c; ++c) if (*c == '\\') *c = '/';
	while (norm[0] == '/') memmove(norm, norm + 1, strlen(norm));
	IFileSystem* fs = FsGet();
	if (!fs) return nullptr;
	FileHandle_t source = fs->Open(norm, "rb", "GAME");
	if (!source || source == SKEETO_FS_INVALID_HANDLE) return nullptr;
	const unsigned int size = fs->Size(source);
	const long fileTime = fs->GetFileTime(norm, "GAME");
	if (!size || size > maxBytes) {
		fs->Close(source);
		return nullptr;
	}
	DlcCacheExtract* extract = new(std::nothrow) DlcCacheExtract();
	if (!extract) { fs->Close(source); return nullptr; }
	extract->fs = fs;
	extract->source = source;
	extract->size = size;
	unsigned long long hash = 1469598103934665603ull;
	for (const unsigned char* p = (const unsigned char*)norm; *p; ++p) {
		hash ^= *p;
		hash *= 1099511628211ull;
	}
	for (int shift = 0; shift < 32; shift += 8) {
		hash ^= (unsigned char)(size >> shift);
		hash *= 1099511628211ull;
		hash ^= (unsigned char)((unsigned long)fileTime >> shift);
		hash *= 1099511628211ull;
	}
	wchar_t local[MAX_PATH]{};
	if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, local) != S_OK) {
		DlcCacheExtractCancel(extract);
		return nullptr;
	}
	wchar_t dir[MAX_PATH]{};
	_snwprintf(dir, MAX_PATH - 1, L"%s\\Skeeto", local);
	dir[MAX_PATH - 1] = 0;
	CreateDirectoryW(dir, nullptr);
	_snwprintf(dir, MAX_PATH - 1, L"%s\\Skeeto\\background_cache", local);
	dir[MAX_PATH - 1] = 0;
	CreateDirectoryW(dir, nullptr);
	_snwprintf(extract->finalPath, sizeof(extract->finalPath) / sizeof(extract->finalPath[0]) - 1,
		L"%s\\%016llx%s", dir, hash, extension);
	extract->finalPath[sizeof(extract->finalPath) / sizeof(extract->finalPath[0]) - 1] = 0;
	WIN32_FILE_ATTRIBUTE_DATA ad{};
	if (GetFileAttributesExW(extract->finalPath, GetFileExInfoStandard, &ad)
		&& ad.nFileSizeHigh == 0 && ad.nFileSizeLow == size) {
		fs->Close(source);
		extract->source = nullptr;
		extract->ready = true;
		return extract;
	}
	_snwprintf(extract->tempPath, sizeof(extract->tempPath) / sizeof(extract->tempPath[0]) - 1,
		L"%s.tmp", extract->finalPath);
	extract->tempPath[sizeof(extract->tempPath) / sizeof(extract->tempPath[0]) - 1] = 0;
	extract->dest = CreateFileW(extract->tempPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (extract->dest == INVALID_HANDLE_VALUE) {
		DlcCacheExtractCancel(extract);
		return nullptr;
	}
	extract->tempActive = true;
	extract->buffer = (unsigned char*)malloc(1024u * 1024u);
	if (!extract->buffer) {
		DlcCacheExtractCancel(extract);
		return nullptr;
	}
	return extract;
}

int DlcCacheExtractPump(DlcCacheExtract* extract, unsigned int byteBudget,
	wchar_t* outPath, size_t outCount) {
	if (!extract || !outPath || outCount < 2) return -1;
	outPath[0] = 0;
	if (extract->failed) return -1;
	if (extract->ready) {
		wcsncpy(outPath, extract->finalPath, outCount - 1);
		outPath[outCount - 1] = 0;
		return 1;
	}
	if (byteBudget < 64u * 1024u) byteBudget = 64u * 1024u;
	if (byteBudget > 1024u * 1024u) byteBudget = 1024u * 1024u;
	unsigned int spent = 0;
	while (extract->copied < extract->size && spent < byteBudget) {
		unsigned int want = extract->size - extract->copied;
		if (want > byteBudget - spent) want = byteBudget - spent;
		if (want > 1024u * 1024u) want = 1024u * 1024u;
		const int got = extract->fs->Read(extract->buffer, (int)want, extract->source);
		DWORD wrote = 0;
		const bool ok = got == (int)want
			&& WriteFile(extract->dest, extract->buffer, want, &wrote, nullptr) && wrote == want;
		if (!ok) { extract->failed = true; return -1; }
		extract->copied += want;
		spent += want;
	}
	if (extract->copied < extract->size) return 0;
	extract->fs->Close(extract->source);
	extract->source = nullptr;
	bool ok = FlushFileBuffers(extract->dest) != FALSE;
	CloseHandle(extract->dest);
	extract->dest = INVALID_HANDLE_VALUE;
	if (ok) ok = MoveFileExW(extract->tempPath, extract->finalPath,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
	if (!ok) {
		extract->failed = true;
		return -1;
	}
	extract->tempActive = false;
	extract->ready = true;
	wcsncpy(outPath, extract->finalPath, outCount - 1);
	outPath[outCount - 1] = 0;
	return 1;
}

// -----------------------------------------------------------------------------
// Keyboard options "Edit keys" list (scripts/kb_act.lst): Skeeto section.
// No hook: a generated kb_act.lst lives in a Skeeto-owned loose directory that is
// added as the head GAME search path at runtime (never persisted).
// Split by thread: the engine-filesystem helpers below run on the game thread
// (only on state edges); the Win32 file writes/deletes run on the existing worker
// thread (KbActFile*), so Paint never touches the disk directly.
// -----------------------------------------------------------------------------
static const char kKbActRel[] = "scripts/kb_act.lst";
static const char kKbActMarker[] = "\"blank\"\t\t\"Skeeto\"";

static void KbActPathNorm(const char* in, char* out, int outN) {
	int o = 0;
	for (const char* p = in; p && *p && o < outN - 1; ++p) {
		char c = *p;
		if (c == '\\') c = '/';
		if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
		out[o++] = c;
	}
	while (o > 0 && out[o - 1] == '/') --o;
	out[o] = 0;
}

bool FsKbActSectionPresent(const char* ownDir) {
	IFileSystem* fs = FsGet();
	if (!fs || !ownDir || !ownDir[0]) return false;
	static char s_paths[8192];
	s_paths[0] = 0;
	fs->GetSearchPath("GAME", false, s_paths, (int)sizeof(s_paths));
	s_paths[sizeof(s_paths) - 1] = 0;
	char want[MAX_PATH]{};
	KbActPathNorm(ownDir, want, MAX_PATH);
	if (!want[0]) return false;
	char one[MAX_PATH]{};
	const char* p = s_paths;
	while (*p) {
		const char* e = strchr(p, ';');
		const size_t n = e ? (size_t)(e - p) : strlen(p);
		if (n > 0 && n < MAX_PATH) {
			char raw[MAX_PATH]{};
			memcpy(raw, p, n);
			raw[n] = 0;
			KbActPathNorm(raw, one, MAX_PATH);
			if (!strcmp(one, want)) return true;
		}
		if (!e) break;
		p = e + 1;
	}
	return false;
}

bool FsKbActSectionAtHead(const char* ownDir) {
	IFileSystem* fs = FsGet();
	if (!fs || !ownDir || !ownDir[0]) return false;
	char paths[MAX_PATH + 1]{};
	// Include pack paths: a later addon may precede our still-present loose path.
	fs->GetSearchPath("GAME", true, paths, static_cast<int>(sizeof(paths)));
	paths[sizeof(paths) - 1] = 0;
	const char* end = strchr(paths, ';');
	const size_t firstLen = end ? static_cast<size_t>(end - paths) : strlen(paths);
	if (!firstLen || firstLen >= MAX_PATH) return false;
	char first[MAX_PATH]{}, want[MAX_PATH]{};
	memcpy(first, paths, firstLen);
	char normalized[MAX_PATH]{};
	KbActPathNorm(first, normalized, MAX_PATH);
	KbActPathNorm(ownDir, want, MAX_PATH);
	return want[0] && !strcmp(normalized, want);
}

void FsKbActAddPath(const char* ownDir) {
	IFileSystem* fs = FsGet();
	if (fs && ownDir && ownDir[0]) {
		// Existing entries can be deduplicated without moving to the requested head.
		// Called only on state edges; detach our own path before restoring priority.
		fs->RemoveSearchPath(ownDir, "GAME");
		fs->AddSearchPath(ownDir, "GAME", PATH_ADD_TO_HEAD, 0);
	}
}

void FsKbActRemovePath(const char* ownDir) {
	IFileSystem* fs = FsGet();
	if (fs && ownDir && ownDir[0]) fs->RemoveSearchPath(ownDir, "GAME");
}

int FsKbActReadOriginal(const char* ownDir, char** outBuf, int* outLen) {
	*outBuf = nullptr;
	*outLen = 0;
	if (!FsGet() || !ownDir || !ownDir[0]) return 0;
	// Never read our own generated file back as the "original" list. Open/Read
	// (not ReadFile) so a ReadFile detour (Left 4 Neko) does not inject here.
	FsKbActRemovePath(ownDir);
	char* orig = nullptr;
	int origLen = 0;
	// Match GameUI's null path ID. Require a complete read before composing.
	IFileSystem* fs = FsGet();
	FileHandle_t file = fs->Open(kKbActRel, "rb", nullptr);
	if (!file || file == SKEETO_FS_INVALID_HANDLE) return 0;
	const unsigned size = fs->Size(file);
	if (!size || size > 256u * 1024u) { fs->Close(file); return 0; }
	orig = static_cast<char*>(malloc(size + 1));
	if (!orig) { fs->Close(file); return 0; }
	origLen = fs->Read(orig, static_cast<int>(size), file);
	fs->Close(file);
	if (origLen != static_cast<int>(size)) { free(orig); return 0; }
	orig[origLen] = 0;
	const unsigned char* ub = (const unsigned char*)orig;
	if (origLen >= 2 && ((ub[0] == 0xFF && ub[1] == 0xFE) || (ub[0] == 0xFE && ub[1] == 0xFF))) {
		free(orig);
		return -1; // UTF-16 list: do not mix encodings
	}
	if (strstr(orig, kKbActMarker)) {
		free(orig);
		return -1; // some other copy already carries a Skeeto section
	}
	*outBuf = orig;
	*outLen = origLen;
	return 1;
}

bool FsKbActVerify(const char* expected, int len) {
	IFileSystem* fs = FsGet();
	if (!fs || !expected || len <= 0) return false;
	FileHandle_t file = fs->Open(kKbActRel, "rb", nullptr);
	if (!file || file == SKEETO_FS_INVALID_HANDLE) return false;
	char prefix[256];
	const int count = len < static_cast<int>(sizeof(prefix)) ? len : static_cast<int>(sizeof(prefix));
	const bool sameSize = fs->Size(file) == static_cast<unsigned>(len);
	const int read = sameSize ? fs->Read(prefix, count, file) : 0;
	fs->Close(file);
	return sameSize && read == count && !memcmp(prefix, expected, count);
}

// ---- worker thread (Win32 only) ----------------------------------------------
static void KbActFilePaths(const char* ownDir, char* scripts, char* file, char* tmp) {
	snprintf(scripts, MAX_PATH, "%s\\scripts", ownDir);
	snprintf(file, MAX_PATH, "%s\\kb_act.lst", scripts);
	snprintf(tmp, MAX_PATH, "%s\\kb_act.lst.tmp", scripts);
}

bool KbActFileExists(const char* ownDir) {
	if (!ownDir || !ownDir[0]) return false;
	char scripts[MAX_PATH]{}, file[MAX_PATH]{}, tmp[MAX_PATH]{};
	KbActFilePaths(ownDir, scripts, file, tmp);
	return GetFileAttributesA(file) != INVALID_FILE_ATTRIBUTES
		|| GetFileAttributesA(ownDir) != INVALID_FILE_ATTRIBUTES;
}

int KbActFileSync(const char* ownDir, const char* data, int len) {
	if (!ownDir || !ownDir[0] || !data || len <= 0) return 0;
	char scripts[MAX_PATH]{}, file[MAX_PATH]{}, tmp[MAX_PATH]{};
	KbActFilePaths(ownDir, scripts, file, tmp);
	// Same bytes already on disk: no write at all (the common startup case).
	{
		HANDLE h = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h != INVALID_HANDLE_VALUE) {
			bool same = false;
			LARGE_INTEGER sz{};
			if (GetFileSizeEx(h, &sz) && sz.QuadPart == len) {
				static char s_cur[65536];
				DWORD rd = 0;
				if (len <= (int)sizeof(s_cur) && ::ReadFile(h, s_cur, (DWORD)len, &rd, nullptr) && (int)rd == len)
					same = memcmp(s_cur, data, (size_t)len) == 0;
			}
			CloseHandle(h);
			if (same) return 2;
		}
	}
	CreateDirectoryA(ownDir, nullptr);
	CreateDirectoryA(scripts, nullptr);
	FILE* f = fopen(tmp, "wb");
	if (!f) return 0;
	bool ok = fwrite(data, 1, (size_t)len, f) == (size_t)len;
	if (fclose(f) != 0) ok = false;
	if (!ok || !MoveFileExA(tmp, file, MOVEFILE_REPLACE_EXISTING)) {
		DeleteFileA(tmp);
		return 0;
	}
	return 1;
}

void KbActFileDelete(const char* ownDir) {
	if (!ownDir || !ownDir[0]) return;
	char scripts[MAX_PATH]{}, file[MAX_PATH]{}, tmp[MAX_PATH]{};
	KbActFilePaths(ownDir, scripts, file, tmp);
	DeleteFileA(file);
	DeleteFileA(tmp);
	RemoveDirectoryA(scripts); // only succeeds when empty
	RemoveDirectoryA(ownDir);
}
