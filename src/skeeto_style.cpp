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
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cctype>
#include "skeeto_style.h"
#include "skeeto_filesystem.h"

using CreateInterfaceFn = void* (*)(const char*, int*);

static DlcCatalog g_cat{};
static IFileSystem* g_fs = nullptr;
static int g_lastEngineJson = 0;
static int g_lastLooseJson = 0;
static int g_lastReloadMs = 0;
static bool g_catalogLoaded = false;
static bool g_engineGlobTried = false;

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
	if (fx->overlay[0] || fx->particle[0] || fx->sound[0]
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

static IFileSystem* FsGet() {
	if (!g_fs)
		g_fs = (IFileSystem*)GetIface("filesystem_stdio.dll", "VFileSystem018");
	return g_fs;
}

// Necola InGameMenu::LoadConfigOptions: Open / Read / Close on path ID GAME.
static bool FsReadGAME(const char* relPath, char** outBuf, int* outLen) {
	*outBuf = nullptr;
	*outLen = 0;
	IFileSystem* fs = FsGet();
	if (!fs || !relPath || !relPath[0]) return false;
	FileHandle_t f = fs->Open(relPath, "rb", "GAME");
	if (!f || f == SKEETO_FS_INVALID_HANDLE) return false;
	unsigned sz = fs->Size(f);
	if (sz == 0 || sz > 8u * 1024u * 1024u) {
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
	// Same as Necola InGameMenu::LoadConfigOptions (FindFirst, not FindFirstEx).
	FileFindHandle_t handle{};
	const char* name = fs->FindFirst(glob, &handle);
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

static bool ReadWholeFileDisk(const char* path, char** outBuf, int* outLen) {

	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) return false;
	DWORD sz = GetFileSize(h, nullptr);
	if (sz == INVALID_FILE_SIZE || sz > 8 * 1024 * 1024) {
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
	if (g_cat.count >= kDlcMaxStyles) return;
	DlcStyle st{};
	if (!ParseStyleJson(json, &st)) return;
	// de-dupe by id
	for (int i = 0; i < g_cat.count; ++i) {
		if (!_stricmp(g_cat.styles[i].id, st.id)) {
			g_cat.styles[i] = st;
			return;
		}
	}
	g_cat.styles[g_cat.count++] = st;
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
	const DWORD t0 = GetTickCount();
	char ci[48]{}, si[48]{}, ff[48]{}, fx[48]{};
	strncpy(ci, g_cat.ciId, sizeof(ci) - 1);
	strncpy(si, g_cat.siId, sizeof(si) - 1);
	strncpy(ff, g_cat.ffId, sizeof(ff) - 1);
	strncpy(fx, g_cat.fxId, sizeof(fx) - 1);

	memset(&g_cat, 0, sizeof(g_cat));
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

	if (allowEngineGlob) {
		g_lastEngineJson += ScanEngineGlob("skeeto/skeeto_*.json", "skeeto/");
		if (g_lastEngineJson <= 0 && g_lastLooseJson <= 0)
			g_lastEngineJson += ScanEngineGlob("particles/skeeto_*.json", "particles/");
		g_engineGlobTried = true;
	}

	if (g_cat.count > 1)
		qsort(g_cat.styles, (size_t)g_cat.count, sizeof(DlcStyle), CmpStyle);
	g_catalogLoaded = true;
	g_lastReloadMs = (int)(GetTickCount() - t0);
}

void DlcEnsureCatalog(const char* gameLeft4Dead2Dir, bool allowEngineGlob) {
	if (g_cat.count > 0) return;
	if (g_catalogLoaded && (!allowEngineGlob || g_engineGlobTried)) return;
	DlcReloadCatalog(gameLeft4Dead2Dir, allowEngineGlob);
}

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
		DlcSetSelected(channel, "off");
		return "off";
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
	if (!cand->overlay[0] && !cand->particle[0] && !cand->sound[0] && cand->particleExtraCount <= 0)
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
	*outData = nullptr;
	*outSize = 0;
	if (!relPath || !relPath[0]) return false;

	char norm[260]{};
	strncpy(norm, relPath, sizeof(norm) - 1);
	for (char* c = norm; *c; ++c)
		if (*c == '\\') *c = '/';
	while (norm[0] == '/')
		memmove(norm, norm + 1, strlen(norm));

	char* buf = nullptr;
	int len = 0;
	if (FsReadGAME(norm, &buf, &len) && buf && len > 0) {
		*outData = buf;
		*outSize = len;
		return true;
	}

	if (gameLeft4Dead2Dir && gameLeft4Dead2Dir[0]) {
		char disk[MAX_PATH]{};
		snprintf(disk, sizeof(disk), "%s\\%s", gameLeft4Dead2Dir, norm);
		for (char* c = disk; *c; ++c)
			if (*c == '/') *c = '\\';
		if (ReadWholeFileDisk(disk, &buf, &len) && buf && len > 0) {
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
	// FindFirst (not FindFirstEx): same as style JSON. Caller must not use this
	// during map load / per-frame — xhair list refresh is menu-key only.
	FileFindHandle_t handle{};
	const char* name = fs->FindFirst(glob, &handle);
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
