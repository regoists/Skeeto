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
#include <wincodec.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <mmsystem.h>
#include <cstdio>
#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <climits>
#include <atomic>
#include <new>
#include <emmintrin.h>
#include <utility>
#include <vector>

#include "skeeto_background.h"
#include "skeeto_style.h"

#pragma comment(lib, "winmm.lib")

namespace {

constexpr int kAssetMax = 256;
constexpr unsigned int kImageMax = 64u * 1024u * 1024u;
constexpr unsigned int kVideoMax = 256u * 1024u * 1024u;
constexpr unsigned int kBikMax = 1024u * 1024u * 1024u;
constexpr unsigned int kPixelMax = 3840u * 2160u;

struct Asset {
	char id[64];
	char source[260];
	int type;
};

struct LoadJob {
	unsigned char* bytes;
	int size;
	wchar_t videoPath[520];
	Asset asset;
	unsigned int generation;
};

struct SharedFrame {
	std::vector<unsigned char> pixels;
	int width = 0;
	int height = 0;
	unsigned int serial = 0;
	unsigned int generation = 0;
	LONGLONG mediaTime = 0;
	LONGLONG sourceTime = 0;
};

struct SharedNativeVideo {
	char path[520]{};
	unsigned int generation = 0;
	bool ready = false;
};

// Keep only a very small amount of decoded video ahead of the renderer.  This
// separates variable H.264 decode/conversion time from presentation cadence
// without allowing background playback to grow memory without bound.
constexpr int kVideoQueueDefault = 12;
constexpr int kVideoQueueMax = 16;
constexpr int kVideoQueue4kWarmup = 12;
constexpr LONGLONG kVideoMinFrameStep = 150000; // cap sources above ~60 FPS

static Asset g_assets[kAssetMax]{};
static int g_assetCount = 0;
static int g_current = -1;
static char g_wantedId[64]{};
static bool g_randomStart = false;
static bool g_startupRandomApplied = false;
static bool g_fullScanComplete = false;
static char g_gameDir[MAX_PATH]{};
static SkeetoBackgroundLogFn g_log = nullptr;
static bool g_initialized = false;
static bool g_enabled = false;
static bool g_scanned = false;
static std::atomic_bool g_loadQueued{ false };
static std::atomic_bool g_loadFailed{ false };
static int g_fitOverride = -1;
static std::atomic_int g_volumeOverride{ -1 };
static std::atomic_bool g_stop{ false };
static std::atomic_bool g_menuActive{ false };
static std::atomic_bool g_haveFrame{ false };
static std::atomic_bool g_workerRunning{ false };
static HANDLE g_worker = nullptr;
static HANDLE g_wake = nullptr;
static HANDLE g_audioWake = nullptr;
static CRITICAL_SECTION g_lock{};
static bool g_lockReady = false;
static LoadJob g_job{};
static std::atomic_uint g_generation{ 1 };
static SharedFrame g_shared{};
static SharedFrame g_render{};
static bool g_sharedReady = false;
static SharedFrame g_videoQueue[kVideoQueueMax]{};
static SharedNativeVideo g_nativeVideo{};
static DlcCacheExtract* g_bikExtract = nullptr;
static unsigned int g_bikExtractGeneration = 0;
static int g_videoQueueHead = 0;
static int g_videoQueueCount = 0;
static int g_videoWarmupTarget = 1;
static LONGLONG g_videoWarmupStartedAt = 0;
static bool g_videoQueueStarved = false;
static LONGLONG g_videoClockBase = 0;
static std::atomic_bool g_playbackPaused{ false };
static LONGLONG g_playbackPausedAt = 0;
// Metadata and requests share the existing short frame lock. MF/waveOut stay
// worker-owned; an epoch prevents an in-flight pre-seek frame being published.
static LONGLONG g_playbackDuration = 0, g_seekTarget = 0;
static bool g_playbackSeekable = false;
static unsigned int g_seekEpoch = 0;
static std::atomic_int g_videoPresentedFps10{ 0 };
static std::atomic_int g_videoPlaybackRatePercent{ 0 };
static std::atomic_int g_videoQueueDepth{ 0 };
static std::atomic_int g_videoQueueCapacity{ kVideoQueueDefault };
static std::atomic_int g_videoQueueMinimum{ 0 };
static std::atomic_int g_videoQueueUnderruns{ 0 };
static LONGLONG g_videoStatsStartedAt = 0;
static LONGLONG g_videoStatsStartedMediaTime = 0;
static int g_videoStatsPresented = 0;
static int g_videoStatsMinDepth = kVideoQueueDefault;
static LONGLONG g_videoQpcFrequency = 0;

static void BgLog(const char* fmt, ...);
static DWORD WINAPI DecoderThread(void*);

static LONGLONG VideoClock100ns() {
	LARGE_INTEGER now{};
	if (g_videoQpcFrequency > 0 && QueryPerformanceCounter(&now)) {
		// Split quotient/remainder so long system uptimes cannot overflow before
		// division. Media timestamps and this result are both in 100 ns units.
		return (now.QuadPart / g_videoQpcFrequency) * 10000000
			+ (now.QuadPart % g_videoQpcFrequency) * 10000000 / g_videoQpcFrequency;
	}
	return (LONGLONG)GetTickCount64() * 10000;
}

static bool EnsureWorker() {
	if (!g_initialized || g_stop.load(std::memory_order_relaxed)) return false;
	if (g_worker && g_wake && g_audioWake) return true;
	g_wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	g_audioWake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (!g_wake || !g_audioWake) {
		BgLog("Background load failed: wake event initialization");
		if (g_wake) CloseHandle(g_wake);
		if (g_audioWake) CloseHandle(g_audioWake);
		g_wake = nullptr;
		g_audioWake = nullptr;
		return false;
	}
	g_worker = CreateThread(nullptr, 0, DecoderThread, nullptr, 0, nullptr);
	if (!g_worker) {
		BgLog("Background load failed: worker initialization");
		CloseHandle(g_wake);
		g_wake = nullptr;
		CloseHandle(g_audioWake);
		g_audioWake = nullptr;
		return false;
	}
	return true;
}

static bool LoosePath(const char* relPath, char* out, size_t outN) {
	if (!relPath || !relPath[0] || !out || outN < 2 || !g_gameDir[0]) return false;
	const int wrote = snprintf(out, outN, "%s\\%s", g_gameDir, relPath);
	if (wrote <= 0 || (size_t)wrote >= outN) return false;
	for (char* c = out; *c; ++c)
		if (*c == '/') *c = '\\';
	return true;
}

static bool LooseGameRootPath(const char* relPath, char* out, size_t outN) {
	if (!relPath || !relPath[0] || !out || outN < 2 || !g_gameDir[0]) return false;
	char root[520]{};
	strncpy(root, g_gameDir, sizeof(root) - 1);
	char* slash = strrchr(root, '\\');
	if (!slash) slash = strrchr(root, '/');
	if (!slash) return false;
	*slash = 0;
	const int wrote = snprintf(out, outN, "%s\\%s", root, relPath);
	if (wrote <= 0 || (size_t)wrote >= outN) return false;
	for (char* c = out; *c; ++c)
		if (*c == '/') *c = '\\';
	return true;
}

static bool LooseFileInfo(const char* relPath, char* disk, size_t diskN, DWORD* sizeOut) {
	if (!LoosePath(relPath, disk, diskN)) return false;
	WIN32_FILE_ATTRIBUTE_DATA ad{};
	if (!GetFileAttributesExA(disk, GetFileExInfoStandard, &ad)
		|| (ad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || ad.nFileSizeHigh != 0) {
		// Also accept Left 4 Dead 2\skeeto\backgrounds for users who place
		// loose resources beside the proxy DLL instead of below left4dead2\.
		if (!LooseGameRootPath(relPath, disk, diskN)
			|| !GetFileAttributesExA(disk, GetFileExInfoStandard, &ad)
			|| (ad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || ad.nFileSizeHigh != 0)
			return false;
	}
	if (sizeOut) *sizeOut = ad.nFileSizeLow;
	return true;
}

static bool ReadLooseBinaryLimited(const char* relPath, void** outData, int* outSize,
	unsigned int maxBytes) {
	if (!outData || !outSize) return false;
	*outData = nullptr;
	*outSize = 0;
	char disk[520]{};
	DWORD size = 0;
	if (!LooseFileInfo(relPath, disk, sizeof(disk), &size) || !size || size > maxBytes)
		return false;
	HANDLE file = CreateFileA(disk, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
	if (file == INVALID_HANDLE_VALUE) return false;
	unsigned char* bytes = (unsigned char*)malloc((size_t)size + 1u);
	DWORD read = 0;
	const bool ok = bytes && ReadFile(file, bytes, size, &read, nullptr) && read == size;
	CloseHandle(file);
	if (!ok) { free(bytes); return false; }
	bytes[size] = 0;
	*outData = bytes;
	*outSize = (int)size;
	return true;
}

static bool LooseVideoPath(const char* relPath, wchar_t* out, size_t outN) {
	char disk[520]{};
	DWORD size = 0;
	if (!out || outN < 2 || !LooseFileInfo(relPath, disk, sizeof(disk), &size)
		|| !size)
		return false;
	if (!MultiByteToWideChar(CP_ACP, 0, disk, -1, out, (int)outN)) return false;
	out[outN - 1] = 0;
	return true;
}

static bool LooseNativePath(const char* relPath, char* out, size_t outN) {
	DWORD size = 0;
	return out && outN >= 2 && LooseFileInfo(relPath, out, outN, &size)
		&& size > 0 && size <= kBikMax;
}

static void BgLog(const char* fmt, ...) {
	if (!g_log) return;
	char text[512]{};
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(text, sizeof(text), fmt, ap);
	va_end(ap);
	text[sizeof(text) - 1] = 0;
	g_log(text);
}

static const char* BaseName(const char* path) {
	if (!path) return "";
	const char* b = path;
	for (const char* p = path; *p; ++p)
		if (*p == '/' || *p == '\\') b = p + 1;
	return b;
}

static bool SafeId(const char* s) {
	if (!s || !s[0] || strlen(s) >= 64) return false;
	for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
		// UTF-8 bytes and ordinary filename punctuation are safe here. Reject
		// path separators, control bytes and config delimiters only.
		if (*p < 0x20 || *p == '/' || *p == '\\' || *p == '=' || *p == ';') return false;
	}
	return true;
}

static const char* Ext(const char* s) {
	const char* dot = s ? strrchr(s, '.') : nullptr;
	return dot ? dot : "";
}

static int TypeFromSource(const char* source) {
	const char* e = Ext(source);
	if (!_stricmp(e, ".png") || !_stricmp(e, ".jpg") || !_stricmp(e, ".jpeg")) return SKEETO_BG_IMAGE;
	if (!_stricmp(e, ".mp4")) return SKEETO_BG_VIDEO_MP4;
	if (!_stricmp(e, ".bik")) return SKEETO_BG_VIDEO_BIK;
	return 0;
}

static const char* TypeName(int type) {
	if (type == SKEETO_BG_IMAGE) return "image";
	if (type == SKEETO_BG_VIDEO_BIK) return "native-bik";
	return "mp4";
}

static bool BuildFlat(const char* rawName, Asset* out, bool looseOnly) {
	if (!rawName || !out) return false;
	const char* leaf = BaseName(rawName);
	const int type = TypeFromSource(leaf);
	if (!type) return false;
	char id[64]{};
	const char* dot = strrchr(leaf, '.');
	const size_t idLen = dot ? (size_t)(dot - leaf) : strlen(leaf);
	if (!idLen || idLen >= sizeof(id)) return false;
	memcpy(id, leaf, idLen);
	id[idLen] = 0;
	if (!SafeId(id)) return false;
	char source[260]{};
	snprintf(source, sizeof(source), "skeeto/backgrounds/%s", leaf);
	char disk[520]{};
	if (!LooseFileInfo(source, disk, sizeof(disk), nullptr)) {
		if (looseOnly || !DlcGameFileExists(source)) return false;
	}
	Asset a{};
	strncpy(a.id, id, sizeof(a.id) - 1);
	strncpy(a.source, source, sizeof(a.source) - 1);
	a.type = type;
	*out = a;
	return true;
}

static void AddFlatFile(const char* rawName, void* context) {
	const bool looseOnly = context != nullptr;
	if (g_assetCount >= kAssetMax) return;
	Asset a{};
	if (!BuildFlat(rawName, &a, looseOnly)) return;
	for (int i = 0; i < g_assetCount; ++i)
		if (!_stricmp(g_assets[i].id, a.id)) return;
	g_assets[g_assetCount++] = a;
	BgLog("Found background: %s (%s)", a.id, TypeName(a.type));
}

static int ScanLooseRootFiles() {
	int found = 0;
	for (int root = 0; root < 2; ++root) {
		char glob[520]{};
		const bool pathOk = root == 0
			? LoosePath("skeeto/backgrounds/*", glob, sizeof(glob))
			: LooseGameRootPath("skeeto/backgrounds/*", glob, sizeof(glob));
		if (!pathOk) continue;
		WIN32_FIND_DATAA fd{};
		HANDLE find = FindFirstFileA(glob, &fd);
		if (find == INVALID_HANDLE_VALUE) continue;
		do {
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == '.') continue;
			const int before = g_assetCount;
			AddFlatFile(fd.cFileName, (void*)1);
			if (g_assetCount > before) ++found;
		} while (FindNextFileA(find, &fd));
		FindClose(find);
	}
	return found;
}

static unsigned long long HashBytes(const unsigned char* p, int n) {
	unsigned long long h = 1469598103934665603ull;
	for (int i = 0; i < n; ++i) {
		h ^= p[i];
		h *= 1099511628211ull;
	}
	return h;
}

static bool EnsureCachePath(const unsigned char* bytes, int size, const wchar_t* extension,
	wchar_t* out, size_t outN) {
	if (!bytes || size <= 0 || !extension || !extension[0] || !out || outN < 2)
		return false;
	wchar_t local[MAX_PATH]{};
	if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, local) != S_OK)
		return false;
	wchar_t dir[MAX_PATH]{};
	_snwprintf(dir, MAX_PATH - 1, L"%s\\Skeeto", local);
	CreateDirectoryW(dir, nullptr);
	_snwprintf(dir, MAX_PATH - 1, L"%s\\Skeeto\\background_cache", local);
	CreateDirectoryW(dir, nullptr);
	const unsigned long long hash = HashBytes(bytes, size);
	_snwprintf(out, outN - 1, L"%s\\%016llx%s", dir, hash, extension);
	out[outN - 1] = 0;
	WIN32_FILE_ATTRIBUTE_DATA ad{};
	if (GetFileAttributesExW(out, GetFileExInfoStandard, &ad)
		&& ad.nFileSizeHigh == 0 && ad.nFileSizeLow == (DWORD)size)
		return true;
	HANDLE f = CreateFileW(out, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	DWORD wrote = 0;
	const bool ok = WriteFile(f, bytes, (DWORD)size, &wrote, nullptr) && wrote == (DWORD)size;
	CloseHandle(f);
	if (!ok) DeleteFileW(out);
	return ok;
}

static bool WidePathToAnsi(const wchar_t* wide, char* out, size_t outN) {
	if (!wide || !wide[0] || !out || outN < 2) return false;
	wchar_t shortPath[520]{};
	const wchar_t* source = wide;
	const DWORD shortN = GetShortPathNameW(wide, shortPath,
		(DWORD)(sizeof(shortPath) / sizeof(shortPath[0])));
	if (shortN > 0 && shortN < sizeof(shortPath) / sizeof(shortPath[0])) source = shortPath;
	if (!WideCharToMultiByte(CP_ACP, 0, source, -1, out, (int)outN, nullptr, nullptr))
		return false;
	out[outN - 1] = 0;
	return out[0] != 0;
}

static void PublishNativeVideo(const char* path, unsigned int generation) {
	if (!path || !path[0] || generation != g_generation.load(std::memory_order_acquire)
		|| g_stop.load(std::memory_order_relaxed)) return;
	EnterCriticalSection(&g_lock);
	strncpy(g_nativeVideo.path, path, sizeof(g_nativeVideo.path) - 1);
	g_nativeVideo.path[sizeof(g_nativeVideo.path) - 1] = 0;
	g_nativeVideo.generation = generation;
	g_nativeVideo.ready = true;
	g_loadFailed = false;
	g_loadQueued = true;
	LeaveCriticalSection(&g_lock);
	BgLog("Background native BIK ready: %s", path);
}

static void PublishFrame(std::vector<unsigned char>& pixels, int width, int height,
	unsigned int generation, unsigned int* serial) {
	if (generation != g_generation.load(std::memory_order_acquire)
		|| g_stop.load(std::memory_order_relaxed)) return;
	const bool first = !g_haveFrame.load(std::memory_order_acquire);
	EnterCriticalSection(&g_lock);
	g_shared.pixels.swap(pixels);
	g_shared.width = width;
	g_shared.height = height;
	g_shared.generation = generation;
	g_shared.serial = ++(*serial);
	g_shared.mediaTime = 0;
	g_sharedReady = true;
	g_loadFailed = false;
	g_haveFrame.store(true, std::memory_order_release);
	LeaveCriticalSection(&g_lock);
	if (first) BgLog("Background first frame ready: %dx%d", width, height);
}

static bool QueueVideoFrame(std::vector<unsigned char>& pixels, int width, int height,
	LONGLONG mediaTime, LONGLONG sourceTime, unsigned int epoch, unsigned int generation, unsigned int* serial,
	bool allowInactive, int queueCapacity) {
	if (queueCapacity < 1) queueCapacity = 1;
	if (queueCapacity > kVideoQueueMax) queueCapacity = kVideoQueueMax;
	while (!g_stop.load(std::memory_order_relaxed)
		&& generation == g_generation.load(std::memory_order_acquire)) {
		if (!allowInactive && !g_menuActive.load(std::memory_order_acquire))
			return false;
		bool queued = false;
		bool first = false;
		EnterCriticalSection(&g_lock);
		if (epoch != g_seekEpoch || generation != g_generation.load(std::memory_order_acquire)) {
			LeaveCriticalSection(&g_lock);
			return false;
		}
		if (g_videoQueueCount < queueCapacity) {
			const int tail = (g_videoQueueHead + g_videoQueueCount) % kVideoQueueMax;
			SharedFrame& frame = g_videoQueue[tail];
			frame.pixels.swap(pixels);
			frame.width = width;
			frame.height = height;
			frame.generation = generation;
			frame.serial = ++(*serial);
			frame.mediaTime = mediaTime;
			frame.sourceTime = sourceTime;
			++g_videoQueueCount;
			g_videoQueueDepth.store(g_videoQueueCount, std::memory_order_relaxed);
			g_videoQueueStarved = false;
			first = !g_haveFrame.load(std::memory_order_relaxed);
			g_haveFrame.store(true, std::memory_order_release);
			g_loadFailed = false;
			queued = true;
		}
		LeaveCriticalSection(&g_lock);
		if (queued) {
			if (first) BgLog("Background first frame ready: %dx%d", width, height);
			return true;
		}
		// AcquireFrame signals this event after consuming queue entries.  The
		// short bounded wait also guarantees cancellation cannot strand shutdown.
		WaitForSingleObject(g_wake, g_playbackPaused.load(std::memory_order_acquire) ? 50 : 8);
	}
	return false;
}

static bool DecodeImage(const LoadJob& job, std::vector<unsigned char>& pixels,
	int* outW, int* outH) {
	IWICImagingFactory* factory = nullptr;
	IWICStream* stream = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* conv = nullptr;
	UINT w = 0, h = 0;
	bool ok = false;
	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&factory)))) goto done;
	if (FAILED(factory->CreateStream(&stream))) goto done;
	if (FAILED(stream->InitializeFromMemory(job.bytes, (DWORD)job.size))) goto done;
	if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad, &decoder))) goto done;
	if (FAILED(decoder->GetFrame(0, &frame))) goto done;
	if (FAILED(frame->GetSize(&w, &h)) || !w || !h || w > 4096 || h > 4096
		|| (unsigned long long)w * h > kPixelMax) goto done;
	if (FAILED(factory->CreateFormatConverter(&conv))) goto done;
	if (FAILED(conv->Initialize(frame, GUID_WICPixelFormat32bppBGRA,
		WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom))) goto done;
	pixels.resize((size_t)w * h * 4u);
	if (FAILED(conv->CopyPixels(nullptr, w * 4u, (UINT)pixels.size(), pixels.data()))) goto done;
	*outW = (int)w;
	*outH = (int)h;
	ok = true;
done:
	if (conv) conv->Release();
	if (frame) frame->Release();
	if (decoder) decoder->Release();
	if (stream) stream->Release();
	if (factory) factory->Release();
	return ok;
}

static bool MediaTypeSize(IMFSourceReader* reader, int* outW, int* outH) {
	IMFMediaType* type = nullptr;
	UINT32 w = 0, h = 0;
	const HRESULT hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &type);
	if (SUCCEEDED(hr)) MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &w, &h);
	if (type) type->Release();
	if (!w || !h || w > 4096 || h > 4096 || (unsigned long long)w * h > kPixelMax) return false;
	*outW = (int)w;
	*outH = (int)h;
	return true;
}

static bool OpenVideoReader(const wchar_t* source, bool advanced,
	IMFSourceReader** outReader, int* sourceW, int* sourceH, int* outputW, int* outputH) {
	if (!source || !outReader || !sourceW || !sourceH || !outputW || !outputH)
		return false;
	*outReader = nullptr;
	IMFAttributes* attrs = nullptr;
	IMFSourceReader* reader = nullptr;
	IMFMediaType* native = nullptr;
	IMFMediaType* output = nullptr;
	UINT32 nativeW = 0, nativeH = 0;
	int targetW = 0, targetH = 0;
	bool ok = false;
	if (FAILED(MFCreateAttributes(&attrs, 4))) goto done;
	attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
	attrs->SetUINT32(MF_LOW_LATENCY, TRUE);
	if (advanced)
		attrs->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
	else
		attrs->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
	if (FAILED(MFCreateSourceReaderFromURL(source, attrs, &reader)) || !reader) goto done;
	reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
	reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
	if (FAILED(reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &native))
		|| FAILED(MFGetAttributeSize(native, MF_MT_FRAME_SIZE, &nativeW, &nativeH))
		|| !nativeW || !nativeH || nativeW > 4096 || nativeH > 4096
		|| (unsigned long long)nativeW * nativeH > kPixelMax)
		goto done;
	targetW = (int)nativeW;
	targetH = (int)nativeH;
	if ((unsigned long long)targetW * targetH > 1920ull * 1080ull) {
		double scale = 1920.0 / (double)targetW;
		if ((double)targetH * scale > 1080.0) scale = 1080.0 / (double)targetH;
		targetW = ((int)(targetW * scale + 0.5)) & ~1;
		targetH = ((int)(targetH * scale + 0.5)) & ~1;
	}
	if (FAILED(MFCreateMediaType(&output))) goto done;
	output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	output->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	// The advanced processor can perform resize and YUV->RGB conversion in its
	// playback-oriented path.  The Windows 7 compatibility path deliberately
	// omits resize and keeps the existing bounded CPU downsample fallback.
	if (advanced)
		MFSetAttributeSize(output, MF_MT_FRAME_SIZE, (UINT32)targetW, (UINT32)targetH);
	if (FAILED(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, output)))
		goto done;
	if (!MediaTypeSize(reader, outputW, outputH)) goto done;
	*sourceW = (int)nativeW;
	*sourceH = (int)nativeH;
	*outReader = reader;
	reader = nullptr;
	ok = true;
done:
	if (output) output->Release();
	if (native) native->Release();
	if (reader) reader->Release();
	if (attrs) attrs->Release();
	return ok;
}

static bool CopyVideoFrame(IMFSample* sample, int width, int height, int outWidth, int outHeight,
	std::vector<unsigned char>& bgra) {
	IMFMediaBuffer* buffer = nullptr;
	IMF2DBuffer* two = nullptr;
	BYTE* scan = nullptr;
	LONG pitch = 0;
	DWORD maxLength = 0, currentLength = 0;
	bool locked2D = false;
	bool lockedLinear = false;
	bool ok = false;
	bool exactHalf = false;
	if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) goto done;
	// Hardware and software Media Foundation transforms do not agree on the
	// concrete buffer interface.  IMF2DBuffer is preferable because it exposes
	// the real stride, but ordinary IMFMediaBuffer is a valid and common RGB32
	// output as well (notably for some 4K H.264 decoder paths).
	if (SUCCEEDED(buffer->QueryInterface(IID_PPV_ARGS(&two)))
		&& SUCCEEDED(two->Lock2D(&scan, &pitch)) && scan && pitch) {
		locked2D = true;
	} else {
		if (two) { two->Release(); two = nullptr; }
		scan = nullptr;
		if (FAILED(buffer->Lock(&scan, &maxLength, &currentLength)) || !scan)
			goto done;
		lockedLinear = true;
		// A contiguous RGB32 buffer can still pad each row without exposing
		// IMF2DBuffer.  Derive the real row pitch when possible.
		pitch = width * 4;
		if (height > 0 && currentLength % (DWORD)height == 0) {
			const DWORD candidate = currentLength / (DWORD)height;
			if (candidate >= (DWORD)(width * 4)) pitch = (LONG)candidate;
		}
		const unsigned long long need = (unsigned long long)pitch * (unsigned int)height;
		if ((unsigned long long)currentLength < need) goto done;
	}
	if (outWidth <= 0 || outHeight <= 0 || outWidth > width || outHeight > height)
		goto done;
	bgra.resize((size_t)outWidth * outHeight * 4u);
	exactHalf = width == outWidth * 2 && height == outHeight * 2;
	if (outWidth == width && outHeight == height) {
		// MFVideoFormat_RGB32 is B,G,R,X in memory on little-endian Windows.
		// Preserve that native layout so ordinary <=1080p frames are row copies,
		// not millions of per-pixel channel swaps.
		for (int y = 0; y < outHeight; ++y) {
			const BYTE* src = scan + (ptrdiff_t)y * pitch;
			BYTE* dst = bgra.data() + (size_t)y * outWidth * 4u;
			memcpy(dst, src, (size_t)outWidth * 4u);
		}
		ok = true;
		goto done;
	}
	for (int y = 0; y < outHeight; ++y) {
		const int sy = exactHalf ? y * 2 : (int)((long long)y * height / outHeight);
		const BYTE* src0 = scan + (ptrdiff_t)sy * pitch;
		const BYTE* src1 = exactHalf ? src0 + pitch : src0;
		BYTE* dst = bgra.data() + (size_t)y * outWidth * 4u;
		int x = 0;
		if (exactHalf) {
			// Two 4K source rows -> one 1080p row. SSE2 handles two output
			// pixels per iteration and keeps BGRA byte order intact.
			for (; x + 1 < outWidth; x += 2) {
				const __m128i top = _mm_loadu_si128((const __m128i*)(src0 + (size_t)x * 8u));
				const __m128i bottom = _mm_loadu_si128((const __m128i*)(src1 + (size_t)x * 8u));
				const __m128i vertical = _mm_avg_epu8(top, bottom);
				const __m128i horizontal = _mm_avg_epu8(vertical, _mm_srli_si128(vertical, 4));
				const uint32_t a = (uint32_t)_mm_cvtsi128_si32(horizontal);
				const uint32_t b = (uint32_t)_mm_cvtsi128_si32(_mm_srli_si128(horizontal, 8));
				memcpy(dst + (size_t)x * 4u, &a, sizeof(a));
				memcpy(dst + (size_t)(x + 1) * 4u, &b, sizeof(b));
			}
		}
		for (; x < outWidth; ++x) {
			const int sx = exactHalf ? x * 2 : (int)((long long)x * width / outWidth);
			const BYTE* a = src0 + (size_t)sx * 4u;
			if (exactHalf) {
				const BYTE* b = a + 4;
				const BYTE* c = src1 + (size_t)sx * 4u;
				const BYTE* d = c + 4;
				dst[x * 4 + 0] = (BYTE)(((unsigned)a[0] + b[0] + c[0] + d[0] + 2u) >> 2);
				dst[x * 4 + 1] = (BYTE)(((unsigned)a[1] + b[1] + c[1] + d[1] + 2u) >> 2);
				dst[x * 4 + 2] = (BYTE)(((unsigned)a[2] + b[2] + c[2] + d[2] + 2u) >> 2);
			} else {
				dst[x * 4 + 0] = a[0];
				dst[x * 4 + 1] = a[1];
				dst[x * 4 + 2] = a[2];
			}
			dst[x * 4 + 3] = 255;
		}
	}
	ok = true;
done:
	if (locked2D && two) two->Unlock2D();
	if (lockedLinear && buffer) buffer->Unlock();
	if (two) two->Release();
	if (buffer) buffer->Release();
	return ok;
}

constexpr int kAudioBlockCount = 16;
constexpr unsigned int kAudioSampleMax = 2u * 1024u * 1024u;

struct AudioThreadContext {
	wchar_t source[520]{};
	unsigned int generation = 0;
	std::atomic_bool stop{ false };
};

struct AudioBlock {
	WAVEHDR header{};
	std::vector<unsigned char> bytes;
	bool prepared = false;
};

static bool SeekReaderTime(IMFSourceReader* reader, LONGLONG time) {
	if (!reader) return false;
	PROPVARIANT pos;
	PropVariantInit(&pos);
	pos.vt = VT_I8;
	pos.hVal.QuadPart = time;
	const HRESULT hr = reader->SetCurrentPosition(GUID_NULL, pos);
	PropVariantClear(&pos);
	return SUCCEEDED(hr);
}
static bool SeekReaderStart(IMFSourceReader* reader) { return SeekReaderTime(reader, 0); }

static unsigned int PlaybackSeekRequest(LONGLONG* target) {
	EnterCriticalSection(&g_lock);
	const unsigned int epoch = g_seekEpoch;
	*target = g_seekTarget;
	LeaveCriticalSection(&g_lock);
	return epoch;
}

static bool OpenAudioReader(const wchar_t* source, IMFSourceReader** outReader,
	WAVEFORMATEX* outFormat) {
	if (!source || !outReader || !outFormat) return false;
	*outReader = nullptr;
	memset(outFormat, 0, sizeof(*outFormat));
	IMFAttributes* attrs = nullptr;
	IMFSourceReader* reader = nullptr;
	IMFMediaType* native = nullptr;
	IMFMediaType* output = nullptr;
	UINT32 channels = 0, sampleRate = 0;
	bool ok = false;
	if (FAILED(MFCreateAttributes(&attrs, 2))) goto done;
	attrs->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
	attrs->SetUINT32(MF_LOW_LATENCY, TRUE);
	if (FAILED(MFCreateSourceReaderFromURL(source, attrs, &reader)) || !reader) goto done;
	reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
	if (FAILED(reader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE))) goto done;
	if (FAILED(reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &native))) goto done;
	native->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
	native->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
	if (!channels || channels > 8 || sampleRate < 8000 || sampleRate > 192000) goto done;
	// Background packs are normally stereo AAC. Request bounded 16-bit PCM so
	// waveOut receives a fixed-size, mixer-friendly format. Downmix multichannel
	// sources to stereo instead of exposing an invalid legacy WAVEFORMATEX.
	if (channels > 2) channels = 2;
	if (FAILED(MFCreateMediaType(&output))) goto done;
	output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	output->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	output->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, channels);
	output->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sampleRate);
	output->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
	output->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, channels * 2u);
	output->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, sampleRate * channels * 2u);
	output->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
	if (FAILED(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, output)))
		goto done;
	outFormat->wFormatTag = WAVE_FORMAT_PCM;
	outFormat->nChannels = (WORD)channels;
	outFormat->nSamplesPerSec = sampleRate;
	outFormat->wBitsPerSample = 16;
	outFormat->nBlockAlign = (WORD)(channels * 2u);
	outFormat->nAvgBytesPerSec = sampleRate * outFormat->nBlockAlign;
	outFormat->cbSize = 0;
	*outReader = reader;
	reader = nullptr;
	ok = true;
done:
	if (output) output->Release();
	if (native) native->Release();
	if (reader) reader->Release();
	if (attrs) attrs->Release();
	return ok;
}

static int EffectiveAudioVolume() {
	int volume = g_volumeOverride.load(std::memory_order_relaxed);
	if (volume < 0) volume = 100;
	if (volume > 100) volume = 100;
	return volume;
}

static void ScalePcm16(std::vector<unsigned char>& bytes, int volume) {
	if (volume >= 100 || bytes.empty()) return;
	if (volume <= 0) {
		memset(bytes.data(), 0, bytes.size());
		return;
	}
	short* samples = (short*)bytes.data();
	const size_t count = bytes.size() / sizeof(short);
	for (size_t i = 0; i < count; ++i)
		samples[i] = (short)((int)samples[i] * volume / 100);
}

static bool VideoPresentationStarted(unsigned int generation) {
	if (!g_lockReady) return false;
	bool started = false;
	EnterCriticalSection(&g_lock);
	started = g_videoClockBase != 0 && g_render.generation == generation
		&& !g_render.pixels.empty();
	LeaveCriticalSection(&g_lock);
	return started;
}

static void ReleaseAudioBlocks(HWAVEOUT output, AudioBlock* blocks) {
	if (!output || !blocks) return;
	waveOutReset(output);
	for (int i = 0; i < kAudioBlockCount; ++i) {
		if (blocks[i].prepared) {
			waveOutUnprepareHeader(output, &blocks[i].header, sizeof(WAVEHDR));
			blocks[i].prepared = false;
		}
		blocks[i].header = {};
		blocks[i].bytes.clear();
	}
}

static DWORD WINAPI AudioThread(void* rawContext) {
	AudioThreadContext* context = (AudioThreadContext*)rawContext;
	if (!context) return 0;
	const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	IMFSourceReader* reader = nullptr;
	WAVEFORMATEX format{};
	HWAVEOUT output = nullptr;
	AudioBlock blocks[kAudioBlockCount]{};
	bool activeBefore = false;
	bool started = false;
	bool eof = false;
	bool userPaused = false;
	unsigned int seekEpoch = 0;
	LONGLONG discardBefore = 0;
	if (FAILED(com) || !OpenAudioReader(context->source, &reader, &format)) {
		BgLog("Background MP4 has no supported audio stream; continuing video only");
		goto done;
	}
	if (waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR
		|| !output) {
		BgLog("Background MP4 audio output unavailable; continuing video only");
		goto done;
	}
	waveOutPause(output);
	BgLog("Background MP4 audio ready: %u Hz, %u channel(s)",
		(unsigned)format.nSamplesPerSec, (unsigned)format.nChannels);
	while (!context->stop.load(std::memory_order_relaxed)
		&& !g_stop.load(std::memory_order_relaxed)
		&& context->generation == g_generation.load(std::memory_order_acquire)) {
		const bool active = g_menuActive.load(std::memory_order_acquire);
		if (!active) {
			if (activeBefore) {
				ReleaseAudioBlocks(output, blocks);
				SeekReaderStart(reader);
				waveOutPause(output);
				started = false;
				eof = false;
			}
			activeBefore = false;
			if (g_audioWake) WaitForSingleObject(g_audioWake, INFINITE);
			else Sleep(50);
			continue;
		}
		if (!activeBefore) {
			SeekReaderStart(reader);
			activeBefore = true;
		}
		LONGLONG seekTarget = 0;
		const unsigned int requested = PlaybackSeekRequest(&seekTarget);
		if (requested != seekEpoch) {
			ReleaseAudioBlocks(output, blocks);
			waveOutPause(output);
			if (!SeekReaderTime(reader, seekTarget)) break;
			discardBefore = seekTarget;
			seekEpoch = requested;
			started = false; eof = false; userPaused = false;
		}
		if (g_playbackPaused.load(std::memory_order_acquire)) {
			if (!userPaused) { waveOutPause(output); userPaused = true; }
			if (g_audioWake) WaitForSingleObject(g_audioWake, 50);
			continue;
		}
		if (userPaused) { userPaused = false; started = false; }
		int occupied = 0;
		for (int i = 0; i < kAudioBlockCount; ++i) {
			AudioBlock& block = blocks[i];
			if (block.prepared && (block.header.dwFlags & WHDR_DONE)) {
				waveOutUnprepareHeader(output, &block.header, sizeof(WAVEHDR));
				block.prepared = false;
				block.header = {};
				block.bytes.clear();
			}
			if (block.prepared) ++occupied;
		}
		if (!started && VideoPresentationStarted(context->generation)) {
			waveOutRestart(output);
			started = true;
		}
		if (eof) {
			if (!occupied) break;
			Sleep(10);
			continue;
		}
		int freeIndex = -1;
		for (int i = 0; i < kAudioBlockCount; ++i)
			if (!blocks[i].prepared) { freeIndex = i; break; }
		if (freeIndex < 0) {
			Sleep(10);
			continue;
		}
		DWORD stream = 0, flags = 0;
		LONGLONG timestamp = 0;
		IMFSample* sample = nullptr;
		const HRESULT hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0,
			&stream, &flags, &timestamp, &sample);
		if (FAILED(hr)) { if (sample) sample->Release(); break; }
		if (sample) {
			IMFMediaBuffer* buffer = nullptr;
			BYTE* bytes = nullptr;
			DWORD maxLength = 0, currentLength = 0;
			if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)) && buffer
				&& SUCCEEDED(buffer->Lock(&bytes, &maxLength, &currentLength)) && bytes
				&& currentLength > 0 && currentLength <= kAudioSampleMax
				&& currentLength % format.nBlockAlign == 0) {
				AudioBlock& block = blocks[freeIndex];
				// MF seeks to a preceding sample: trim PCM before the requested time.
				DWORD skip = 0;
				if (timestamp < discardBefore) {
					const LONGLONG delta = discardBefore - (timestamp > 0 ? timestamp : 0);
					const double frames = (double)delta * format.nSamplesPerSec / 10000000.0;
					skip = frames >= currentLength / format.nBlockAlign ? currentLength
						: (DWORD)frames * format.nBlockAlign;
				}
				if (skip == currentLength) {
					buffer->Unlock(); buffer->Release(); sample->Release(); continue;
				}
				block.bytes.assign(bytes + skip, bytes + currentLength);
				// Scale only Skeeto's PCM. waveOutSetVolume can map to a shared
				// endpoint on legacy drivers and must not change the game's volume.
				ScalePcm16(block.bytes, EffectiveAudioVolume());
				buffer->Unlock();
				memset(&block.header, 0, sizeof(block.header));
				block.header.lpData = (LPSTR)block.bytes.data();
				block.header.dwBufferLength = currentLength - skip;
				if (waveOutPrepareHeader(output, &block.header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
					block.prepared = true;
					if (waveOutWrite(output, &block.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
						waveOutUnprepareHeader(output, &block.header, sizeof(WAVEHDR));
						block.prepared = false;
					}
				}
			} else if (buffer && bytes) {
				buffer->Unlock();
			}
			if (buffer) buffer->Release();
			sample->Release();
		}
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
			discardBefore = 0;
			if (SeekReaderStart(reader)) continue;
			eof = true;
		}
	}
done:
	if (output) {
		ReleaseAudioBlocks(output, blocks);
		waveOutClose(output);
	}
	if (reader) reader->Release();
	if (SUCCEEDED(com)) CoUninitialize();
	return 0;
}

static bool DecodeVideo(const LoadJob& job, unsigned int* serial) {
	wchar_t cache[MAX_PATH]{};
	const wchar_t* source = job.videoPath[0] ? job.videoPath : cache;
	IMFSourceReader* reader = nullptr;
	HRESULT hr = E_FAIL;
	int sourceWidth = 0, sourceHeight = 0;
	int width = 0, height = 0;
	int presentWidth = 0, presentHeight = 0;
	bool highResolutionSource = false;
	int queueCapacity = kVideoQueueDefault;
	std::vector<unsigned char> pixels;
	LONGLONG timelineOffset = 0;
	LONGLONG loopFirstMediaTime = -1;
	LONGLONG lastSourceMediaTime = -1;
	LONGLONG sourceFrameStep = 333333;
	LONGLONG lastQueuedMediaTime = -1;
	bool publishedAny = false;
	bool previewPaused = false;
	AudioThreadContext* audioContext = nullptr;
	HANDLE audioThread = nullptr;
	unsigned int seekEpoch = 0;
	LONGLONG discardBefore = 0;
	if (!job.videoPath[0] && !EnsureCachePath(job.bytes, job.size, L".mp4", cache, MAX_PATH)) return false;
	bool advanced = OpenVideoReader(source, true, &reader,
		&sourceWidth, &sourceHeight, &width, &height);
	if (!advanced) {
		BgLog("Background advanced video processing unavailable; using software compatibility path");
		if (!OpenVideoReader(source, false, &reader,
			&sourceWidth, &sourceHeight, &width, &height)) goto done;
	}
	{
		PROPVARIANT duration, characteristics;
		PropVariantInit(&duration); PropVariantInit(&characteristics);
		reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &duration);
		reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS, &characteristics);
		EnterCriticalSection(&g_lock);
		if (job.generation == g_generation.load(std::memory_order_acquire)) {
			g_playbackDuration = duration.vt == VT_UI8 && duration.uhVal.QuadPart <= LLONG_MAX
				? (LONGLONG)duration.uhVal.QuadPart : 0;
			g_playbackSeekable = characteristics.vt == VT_UI4 && (characteristics.ulVal & MFMEDIASOURCE_CAN_SEEK) != 0;
		}
		LeaveCriticalSection(&g_lock);
		PropVariantClear(&duration); PropVariantClear(&characteristics);
	}
	audioContext = new (std::nothrow) AudioThreadContext();
	if (audioContext) {
		wcsncpy(audioContext->source, source,
			(sizeof(audioContext->source) / sizeof(audioContext->source[0])) - 1);
		audioContext->generation = job.generation;
		audioThread = CreateThread(nullptr, 0, AudioThread, audioContext, 0, nullptr);
		if (!audioThread) {
			delete audioContext;
			audioContext = nullptr;
			BgLog("Background MP4 audio worker unavailable; continuing video only");
		}
	}
	presentWidth = width;
	presentHeight = height;
	highResolutionSource = (unsigned long long)sourceWidth * sourceHeight > 1920ull * 1080ull;
	queueCapacity = highResolutionSource ? kVideoQueueMax : kVideoQueueDefault;
	EnterCriticalSection(&g_lock);
	g_videoWarmupTarget = highResolutionSource ? kVideoQueue4kWarmup : 1;
	g_videoWarmupStartedAt = 0;
	g_videoQueueCapacity.store(queueCapacity, std::memory_order_relaxed);
	g_videoStatsMinDepth = queueCapacity;
	LeaveCriticalSection(&g_lock);
	if (!advanced && (unsigned long long)width * height > 1920ull * 1080ull) {
		double scale = 1920.0 / (double)width;
		if ((double)height * scale > 1080.0) scale = 1080.0 / (double)height;
		presentWidth = ((int)(width * scale + 0.5)) & ~1;
		presentHeight = ((int)(height * scale + 0.5)) & ~1;
	}
	BgLog("Loading video background: %s (source=%dx%d output=%dx%d up-to-60fps queue=%d %s)",
		job.asset.id, sourceWidth, sourceHeight, presentWidth, presentHeight,
		queueCapacity, advanced ? "advanced-video-processing" : "software-compatibility");
	while (!g_stop.load(std::memory_order_relaxed)
		&& job.generation == g_generation.load(std::memory_order_acquire)) {
		const bool active = g_menuActive.load(std::memory_order_relaxed);
		if (!active && publishedAny) {
			previewPaused = true;
			WaitForSingleObject(g_wake, INFINITE);
			continue;
		}
		if (active && previewPaused) {
			PROPVARIANT pos;
			PropVariantInit(&pos);
			pos.vt = VT_I8;
			pos.hVal.QuadPart = 0;
			reader->SetCurrentPosition(GUID_NULL, pos);
			PropVariantClear(&pos);
			timelineOffset = 0;
			loopFirstMediaTime = -1;
			lastSourceMediaTime = -1;
			sourceFrameStep = 333333;
			lastQueuedMediaTime = -1;
			previewPaused = false;
		}
		LONGLONG seekTarget = 0;
		const unsigned int requested = PlaybackSeekRequest(&seekTarget);
		if (requested != seekEpoch) {
			if (!SeekReaderTime(reader, seekTarget)) break;
			seekEpoch = requested; discardBefore = seekTarget;
			timelineOffset = 0; loopFirstMediaTime = -1;
			lastSourceMediaTime = -1; lastQueuedMediaTime = -1;
			publishedAny = false; // allow one paused seek preview
		}
		if (g_playbackPaused.load(std::memory_order_acquire) && publishedAny) {
			WaitForSingleObject(g_wake, 50);
			continue;
		}
		DWORD stream = 0, flags = 0;
		LONGLONG timestamp = 0;
		IMFSample* sample = nullptr;
		hr = reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0,
			&stream, &flags, &timestamp, &sample);
		if (FAILED(hr)) { if (sample) sample->Release(); break; }
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
			discardBefore = 0;
			if (lastSourceMediaTime >= 0 && loopFirstMediaTime >= 0)
				timelineOffset += lastSourceMediaTime - loopFirstMediaTime + sourceFrameStep;
			PROPVARIANT pos;
			PropVariantInit(&pos);
			pos.vt = VT_I8;
			pos.hVal.QuadPart = 0;
			reader->SetCurrentPosition(GUID_NULL, pos);
			PropVariantClear(&pos);
			loopFirstMediaTime = -1;
			lastSourceMediaTime = -1;
			if (sample) sample->Release();
			continue;
		}
		if (!sample) continue;
		const LONGLONG mediaTime = timestamp > 0 ? timestamp : 0;
		if (mediaTime < discardBefore) { sample->Release(); continue; }
		if (loopFirstMediaTime < 0) loopFirstMediaTime = mediaTime;
		if (lastSourceMediaTime >= 0 && mediaTime > lastSourceMediaTime) {
			const LONGLONG delta = mediaTime - lastSourceMediaTime;
			if (delta > 0 && delta < 2000000) sourceFrameStep = delta;
		}
		lastSourceMediaTime = mediaTime;
		const LONGLONG presentationTime = timelineOffset + mediaTime - loopFirstMediaTime;
		// Preserve source cadence up to 60 FPS. Presentation remains timestamp based;
		// never let a wall clock that the decoder cannot catch permanently starve the
		// queue, as happened in the withdrawn real-time catch-up experiment.
		const bool selected = lastQueuedMediaTime < 0
			|| presentationTime - lastQueuedMediaTime >= kVideoMinFrameStep;
		if ((active || !publishedAny) && selected
			&& CopyVideoFrame(sample, width, height, presentWidth, presentHeight, pixels)) {
			if (QueueVideoFrame(pixels, presentWidth, presentHeight, presentationTime, mediaTime, seekEpoch,
				job.generation, serial, !publishedAny, queueCapacity)) {
				publishedAny = true;
				lastQueuedMediaTime = presentationTime;
			}
		}
		sample->Release();
	}
done:
	if (reader) reader->Release();
	if (audioContext) {
		audioContext->stop.store(true, std::memory_order_release);
		if (audioThread) {
			if (g_audioWake) SetEvent(g_audioWake);
			WaitForSingleObject(audioThread, INFINITE);
			CloseHandle(audioThread);
		}
		delete audioContext;
	}
	// Suspension, asset switching and DLL shutdown are expected cancellation
	// paths, not decoder failures.  The caller only reports a real failure while
	// this generation remains current and the engine is still running.
	return publishedAny || g_stop.load(std::memory_order_relaxed)
		|| job.generation != g_generation.load(std::memory_order_acquire);
}

static bool TakeJob(LoadJob* out) {
	bool have = false;
	EnterCriticalSection(&g_lock);
	// Loose MP4 jobs intentionally carry only videoPath and no in-memory bytes.
	// generation is assigned for every queued job, so it is the common validity
	// marker for images, cached videos and direct-path videos alike.
	if (g_job.generation != 0) {
		*out = g_job;
		g_job = {};
		have = true;
	}
	LeaveCriticalSection(&g_lock);
	return have;
}

static DWORD WINAPI DecoderThread(void*) {
	g_workerRunning.store(true, std::memory_order_release);
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const HRESULT mf = MFStartup(MF_VERSION, MFSTARTUP_LITE);
	BgLog(SUCCEEDED(mf) ? "Media Foundation initialized" : "Background load failed: Media Foundation init");
	unsigned int serial = 0;
	while (!g_stop.load(std::memory_order_relaxed)) {
		WaitForSingleObject(g_wake, INFINITE);
		LoadJob job{};
		if (!TakeJob(&job)) continue;
		bool ok = false;
		if (job.asset.type == SKEETO_BG_IMAGE) {
			BgLog("Loading image background: %s", job.asset.id);
			std::vector<unsigned char> pixels;
			int w = 0, h = 0;
			ok = DecodeImage(job, pixels, &w, &h);
			if (ok) PublishFrame(pixels, w, h, job.generation, &serial);
		} else if (SUCCEEDED(mf)) {
			ok = DecodeVideo(job, &serial);
		}
		free(job.bytes);
		if (!ok && !g_stop.load(std::memory_order_relaxed)
			&& job.generation == g_generation.load(std::memory_order_acquire)) {
			g_loadFailed = true;
			g_loadQueued = false;
			BgLog("Background load failed: decoder rejected %s", job.asset.source);
		}
	}
	LoadJob tail{};
	if (TakeJob(&tail)) free(tail.bytes);
	if (SUCCEEDED(mf)) MFShutdown();
	CoUninitialize();
	g_workerRunning.store(false, std::memory_order_release);
	return 0;
}

static void ClearFrames(bool clearPublishedStats = true) {
	if (g_bikExtract) {
		DlcCacheExtractCancel(g_bikExtract);
		g_bikExtract = nullptr;
		g_bikExtractGeneration = 0;
	}
	EnterCriticalSection(&g_lock);
	g_shared = {};
	g_render = {};
	if (clearPublishedStats) g_nativeVideo = {};
	g_sharedReady = false;
	for (int i = 0; i < kVideoQueueMax; ++i) g_videoQueue[i] = {};
	g_videoQueueHead = 0;
	g_videoQueueCount = 0;
	g_videoWarmupTarget = 1;
	g_videoWarmupStartedAt = 0;
	g_videoQueueStarved = false;
	g_videoClockBase = 0;
	g_playbackPaused = false;
	g_playbackPausedAt = 0;
	g_seekTarget = 0;
	// Menu suspension keeps the same decoder generation and its local epoch.
	// Publish a fresh reset request so the first seek after reentry cannot alias it.
	if (clearPublishedStats) g_seekEpoch = 0;
	else ++g_seekEpoch;
	if (clearPublishedStats) { g_playbackDuration = 0; g_playbackSeekable = false; }
	if (clearPublishedStats) {
		g_videoPresentedFps10.store(0, std::memory_order_relaxed);
		g_videoPlaybackRatePercent.store(0, std::memory_order_relaxed);
		g_videoQueueCapacity.store(kVideoQueueDefault, std::memory_order_relaxed);
		g_videoQueueMinimum.store(0, std::memory_order_relaxed);
		g_videoQueueUnderruns.store(0, std::memory_order_relaxed);
	}
	g_videoQueueDepth.store(0, std::memory_order_relaxed);
	g_videoStatsStartedAt = 0;
	g_videoStatsStartedMediaTime = 0;
	g_videoStatsPresented = 0;
	g_videoStatsMinDepth = g_videoQueueCapacity.load(std::memory_order_relaxed);
	g_haveFrame.store(false, std::memory_order_release);
	LeaveCriticalSection(&g_lock);
	if (g_wake) SetEvent(g_wake);
	if (g_audioWake) SetEvent(g_audioWake);
}

static bool QueueCurrent(bool allowInactive = false, bool looseOnly = false) {
	if (!g_initialized || !g_enabled
		|| (!allowInactive && !g_menuActive.load(std::memory_order_acquire))
		|| g_current < 0 || g_current >= g_assetCount) return false;
	const Asset& a = g_assets[g_current];
	void* data = nullptr;
	int size = 0;
	LoadJob next{};
	char nativePath[520]{};
	if (a.type == SKEETO_BG_VIDEO_BIK
		&& LooseNativePath(a.source, nativePath, sizeof(nativePath))) {
		++g_generation;
		if (!g_generation) ++g_generation;
		ClearFrames();
		PublishNativeVideo(nativePath, g_generation.load(std::memory_order_acquire));
		return true;
	}
	if (a.type == SKEETO_BG_VIDEO_BIK) {
		if (looseOnly) {
			g_loadFailed = true;
			BgLog("Background load failed: loose BIK missing %s", a.source);
			return false;
		}
		++g_generation;
		if (!g_generation) ++g_generation;
		ClearFrames();
		g_bikExtract = DlcCacheExtractBegin(a.source, L".bik", kBikMax);
		if (!g_bikExtract) {
			g_loadFailed = true;
			BgLog("Background load failed: BIK extraction rejected %s", a.source);
			return false;
		}
		g_bikExtractGeneration = g_generation.load(std::memory_order_acquire);
		g_loadQueued = true;
		g_loadFailed = false;
		BgLog("Preparing VPK BIK cache incrementally: %s", a.source);
		return true;
	}
	if (a.type == SKEETO_BG_VIDEO_MP4 && LooseVideoPath(a.source, next.videoPath,
		sizeof(next.videoPath) / sizeof(next.videoPath[0]))) {
		// Media Foundation opens the loose MP4 directly: no full-file read/hash/cache copy.
	} else {
		const unsigned int limit = a.type == SKEETO_BG_IMAGE ? kImageMax : kVideoMax;
		const bool read = ReadLooseBinaryLimited(a.source, &data, &size, limit)
			|| (!looseOnly && DlcReadGameBinaryLimited(g_gameDir, a.source, &data, &size, limit));
		if (!read) {
			g_loadFailed = true;
			BgLog("Background load failed: missing/oversize %s", a.source);
			return false;
		}
	}
	if (!EnsureWorker()) {
		free(data);
		return false;
	}
	++g_generation;
	if (!g_generation) ++g_generation;
	ClearFrames();
	EnterCriticalSection(&g_lock);
	if (g_job.bytes) free(g_job.bytes);
	next.bytes = (unsigned char*)data;
	next.size = size;
	next.asset = a;
	next.generation = g_generation.load(std::memory_order_acquire);
	g_job = next;
	LeaveCriticalSection(&g_lock);
	g_loadQueued = true;
	g_loadFailed = false;
	SetEvent(g_wake);
	return true;
}

static void ChooseCurrent() {
	g_current = -1;
	if (g_wantedId[0]) {
		for (int i = 0; i < g_assetCount; ++i) {
			if (!_stricmp(g_assets[i].id, g_wantedId)) {
				g_current = i;
				break;
			}
		}
	}
	if (g_current < 0 && g_assetCount > 0) g_current = 0;
	if (g_current >= 0)
		strncpy(g_wantedId, g_assets[g_current].id, sizeof(g_wantedId) - 1);
}

static bool ApplyStartupRandom() {
	if (!g_randomStart || g_startupRandomApplied || !g_fullScanComplete || g_assetCount <= 0)
		return false;
	LARGE_INTEGER qpc{};
	QueryPerformanceCounter(&qpc);
	unsigned int seed = (unsigned int)qpc.LowPart ^ (unsigned int)qpc.HighPart
		^ GetTickCount() ^ GetCurrentProcessId();
	seed ^= seed << 13;
	seed ^= seed >> 17;
	seed ^= seed << 5;
	g_current = (int)(seed % (unsigned int)g_assetCount);
	strncpy(g_wantedId, g_assets[g_current].id, sizeof(g_wantedId) - 1);
	g_wantedId[sizeof(g_wantedId) - 1] = 0;
	g_startupRandomApplied = true;
	BgLog("Background startup random selected: %s", g_assets[g_current].id);
	return true;
}

} // namespace

void SkeetoBackgroundInitialize(const char* gameLeft4Dead2Dir, SkeetoBackgroundLogFn logFn) {
	if (g_initialized) return;
	if (gameLeft4Dead2Dir) strncpy(g_gameDir, gameLeft4Dead2Dir, sizeof(g_gameDir) - 1);
	g_log = logFn;
	InitializeCriticalSection(&g_lock);
	g_lockReady = true;
	LARGE_INTEGER qpcFrequency{};
	g_videoQpcFrequency = QueryPerformanceFrequency(&qpcFrequency)
		? qpcFrequency.QuadPart : 0;
	g_stop.store(false, std::memory_order_relaxed);
	g_initialized = true;
	BgLog("Background engine initialized");
	BgLog("Background filesystem ready");
}

void SkeetoBackgroundSignalStop() {
	g_stop.store(true, std::memory_order_release);
	if (g_wake) SetEvent(g_wake);
	if (g_audioWake) SetEvent(g_audioWake);
}

void SkeetoBackgroundShutdown() {
	if (!g_lockReady) return;
	SkeetoBackgroundSignalStop();
	if (g_worker) {
		if (WaitForSingleObject(g_worker, 3000) != WAIT_OBJECT_0) {
			BgLog("Background worker shutdown exceeded 3 seconds; waiting for safe teardown");
			WaitForSingleObject(g_worker, INFINITE);
		}
		CloseHandle(g_worker);
		g_worker = nullptr;
	}
	if (g_wake) { CloseHandle(g_wake); g_wake = nullptr; }
	if (g_audioWake) { CloseHandle(g_audioWake); g_audioWake = nullptr; }
	if (g_bikExtract) {
		DlcCacheExtractAbandon(g_bikExtract);
		g_bikExtract = nullptr;
		g_bikExtractGeneration = 0;
	}
	EnterCriticalSection(&g_lock);
	if (g_job.bytes) free(g_job.bytes);
	g_job = {};
	g_shared = {};
	g_render = {};
	g_nativeVideo = {};
	for (int i = 0; i < kVideoQueueMax; ++i) g_videoQueue[i] = {};
	g_videoQueueHead = 0;
	g_videoQueueCount = 0;
	g_videoWarmupTarget = 1;
	g_videoWarmupStartedAt = 0;
	g_videoQueueStarved = false;
	g_videoClockBase = 0;
	g_videoPresentedFps10.store(0, std::memory_order_relaxed);
	g_videoPlaybackRatePercent.store(0, std::memory_order_relaxed);
	g_videoQueueDepth.store(0, std::memory_order_relaxed);
	g_videoQueueCapacity.store(kVideoQueueDefault, std::memory_order_relaxed);
	g_videoQueueMinimum.store(0, std::memory_order_relaxed);
	g_videoQueueUnderruns.store(0, std::memory_order_relaxed);
	g_videoStatsStartedAt = 0;
	g_videoStatsStartedMediaTime = 0;
	g_videoStatsPresented = 0;
	g_videoStatsMinDepth = kVideoQueueDefault;
	g_videoQpcFrequency = 0;
	LeaveCriticalSection(&g_lock);
	DeleteCriticalSection(&g_lock);
	g_lockReady = false;
	g_initialized = false;
	g_enabled = false;
	g_scanned = false;
	g_fullScanComplete = false;
	g_startupRandomApplied = false;
	g_randomStart = false;
	g_assetCount = 0;
	g_current = -1;
	g_wantedId[0] = 0;
	g_gameDir[0] = 0;
	g_loadQueued.store(false, std::memory_order_release);
	g_loadFailed.store(false, std::memory_order_release);
	g_menuActive.store(false, std::memory_order_release);
	g_haveFrame.store(false, std::memory_order_release);
	BgLog("Background engine shutdown");
	g_log = nullptr;
}

void SkeetoBackgroundSetEnabled(bool enabled) {
	if (g_enabled == enabled) return;
	g_enabled = enabled;
	if (!enabled) {
		++g_generation;
		g_loadQueued = false;
		g_loadFailed = false;
		if (g_lockReady) ClearFrames();
		BgLog("Background engine suspended");
	} else {
		BgLog("Background engine resumed");
		if (g_menuActive.load(std::memory_order_acquire)) EnsureWorker();
		if (g_scanned && g_current >= 0 && g_menuActive.load(std::memory_order_acquire)) QueueCurrent();
	}
	if (g_wake) SetEvent(g_wake);
	if (g_audioWake) SetEvent(g_audioWake);
}

bool SkeetoBackgroundEnabled() { return g_enabled; }

SkeetoBackgroundPlayback SkeetoBackgroundGetPlayback() {
	SkeetoBackgroundPlayback result;
	if (!g_lockReady || !g_enabled || !g_menuActive.load(std::memory_order_acquire)
		|| SkeetoBackgroundCurrentType() != SKEETO_BG_VIDEO_MP4) return result;
	EnterCriticalSection(&g_lock);
	result.available = g_playbackDuration > 0 && !g_loadFailed.load(std::memory_order_acquire);
	result.paused = g_playbackPaused.load(std::memory_order_acquire);
	result.seekable = result.available && g_playbackSeekable;
	result.duration = g_playbackDuration;
	result.position = g_render.sourceTime;
	if (result.position > result.duration) result.position = result.duration;
	LeaveCriticalSection(&g_lock);
	return result;
}

bool SkeetoBackgroundSetPaused(bool paused) {
	if (!SkeetoBackgroundGetPlayback().available) return false;
	EnterCriticalSection(&g_lock);
	if (paused != g_playbackPaused.load(std::memory_order_relaxed)) {
		const LONGLONG now = VideoClock100ns();
		if (paused) g_playbackPausedAt = now;
		else if (g_videoClockBase && g_playbackPausedAt) g_videoClockBase += now - g_playbackPausedAt;
		g_playbackPausedAt = paused ? now : 0;
		g_playbackPaused.store(paused, std::memory_order_release);
		g_videoStatsStartedAt = 0;
	}
	LeaveCriticalSection(&g_lock);
	if (g_wake) SetEvent(g_wake);
	if (g_audioWake) SetEvent(g_audioWake);
	return true;
}

bool SkeetoBackgroundSeek(LONGLONG position) {
	const auto state = SkeetoBackgroundGetPlayback();
	if (!state.seekable) return false;
	if (position < 0) position = 0;
	if (position >= state.duration) position = state.duration > 1000000 ? state.duration - 1000000 : 0;
	EnterCriticalSection(&g_lock);
	g_seekTarget = position;
	++g_seekEpoch;
	g_videoQueueHead = g_videoQueueCount = 0;
	g_videoQueueDepth = 0;
	g_videoClockBase = 0;
	g_videoStatsStartedAt = 0;
	g_playbackPausedAt = g_playbackPaused.load(std::memory_order_relaxed) ? VideoClock100ns() : 0;
	LeaveCriticalSection(&g_lock);
	if (g_wake) SetEvent(g_wake);
	if (g_audioWake) SetEvent(g_audioWake);
	return true;
}

void SkeetoBackgroundSetMenuActive(bool active) {
	const bool was = g_menuActive.exchange(active, std::memory_order_acq_rel);
	if (was == active) return;
	if (active) {
		BgLog("Background engine resumed");
		if (g_enabled && g_scanned && !g_loadQueued && !g_haveFrame.load(std::memory_order_acquire))
			QueueCurrent();
	} else {
		// Preserve the last one-second playback summary so the in-game menu can
		// report how the previous main-menu session actually performed.
		ClearFrames(false);
		if (g_current >= 0 && g_current < g_assetCount
			&& g_assets[g_current].type == SKEETO_BG_VIDEO_BIK)
			g_loadQueued = false;
		BgLog("Background engine suspended");
	}
	if (g_wake) SetEvent(g_wake);
	if (g_audioWake) SetEvent(g_audioWake);
}

bool SkeetoBackgroundScan() {
	if (!g_initialized) return false;
	g_assetCount = 0;
	ScanLooseRootFiles();
	DlcForEachGameGlob("skeeto/backgrounds/*", &AddFlatFile, nullptr);
	g_scanned = true;
	g_fullScanComplete = true;
	ChooseCurrent();
	ApplyStartupRandom();
	if (g_enabled && g_current >= 0 && g_menuActive.load(std::memory_order_acquire)) QueueCurrent();
	return g_assetCount > 0;
}

bool SkeetoBackgroundPreloadLoose() {
	if (!g_initialized || !g_enabled) return false;
	// Startup random needs the complete loose + GAME/VPK pool. Do not lock it to
	// an incomplete early loose-only catalog merely to show a frame sooner.
	if (g_randomStart && !g_startupRandomApplied) return false;
	g_assetCount = 0;
	ScanLooseRootFiles();
	g_scanned = true;
	g_fullScanComplete = false;
	ChooseCurrent();
	return g_current >= 0 && QueueCurrent(true, true);
}

bool SkeetoBackgroundReload() {
	g_loadQueued = false;
	g_loadFailed = false;
	return SkeetoBackgroundScan();
}

bool SkeetoBackgroundNext() {
	if (!g_scanned) SkeetoBackgroundScan();
	if (g_assetCount <= 0) return false;
	g_current = (g_current < 0) ? 0 : ((g_current + 1) % g_assetCount);
	strncpy(g_wantedId, g_assets[g_current].id, sizeof(g_wantedId) - 1);
	if (g_enabled && g_menuActive.load(std::memory_order_acquire)) QueueCurrent();
	return true;
}

int SkeetoBackgroundCount() { return g_assetCount; }
int SkeetoBackgroundCurrentType() {
	return g_current >= 0 && g_current < g_assetCount ? g_assets[g_current].type : 0;
}
const char* SkeetoBackgroundCurrentId() { return g_current >= 0 ? g_assets[g_current].id : g_wantedId; }
const char* SkeetoBackgroundCurrentName() { return g_current >= 0 ? g_assets[g_current].id : "未发现背景"; }

void SkeetoBackgroundSelect(const char* id) {
	if (!id) id = "";
	strncpy(g_wantedId, id, sizeof(g_wantedId) - 1);
	g_wantedId[sizeof(g_wantedId) - 1] = 0;
	if (g_scanned) {
		ChooseCurrent();
		if (g_enabled && g_current >= 0 && g_menuActive.load(std::memory_order_acquire)) QueueCurrent();
	}
}

void SkeetoBackgroundSetRandomStart(bool enabled) {
	if (g_randomStart == enabled) return;
	g_randomStart = enabled;
	if (!enabled) return;
	if (ApplyStartupRandom() && g_enabled && g_menuActive.load(std::memory_order_acquire))
		QueueCurrent();
}

bool SkeetoBackgroundRandomStart() { return g_randomStart; }

void SkeetoBackgroundSetFit(int fit) {
	if (fit < -1 || fit > SKEETO_BG_STRETCH) fit = -1;
	g_fitOverride = fit;
}

int SkeetoBackgroundFitMode() {
	if (g_fitOverride >= SKEETO_BG_COVER && g_fitOverride <= SKEETO_BG_STRETCH) return g_fitOverride;
	return SKEETO_BG_COVER;
}

void SkeetoBackgroundSetVolume(int volumePercent) {
	if (volumePercent < 0) volumePercent = 0;
	if (volumePercent > 100) volumePercent = 100;
	g_volumeOverride.store(volumePercent, std::memory_order_relaxed);
}

int SkeetoBackgroundVolume() {
	const int volumeOverride = g_volumeOverride.load(std::memory_order_relaxed);
	return volumeOverride >= 0 ? volumeOverride : 100;
}

bool SkeetoBackgroundAcquireFrame(SkeetoBackgroundFrame* out) {
	if (!out || !g_lockReady || !g_haveFrame.load(std::memory_order_acquire)) return false;
	bool consumedVideo = false;
	EnterCriticalSection(&g_lock);
	if (g_sharedReady) {
		g_shared.pixels.swap(g_render.pixels), std::swap(g_shared.width, g_render.width),
		std::swap(g_shared.height, g_render.height), std::swap(g_shared.serial, g_render.serial),
		std::swap(g_shared.generation, g_render.generation);
		g_sharedReady = false;
	}
	if (g_videoQueueCount > 0 && (!g_playbackPaused.load(std::memory_order_acquire) || !g_videoClockBase)) {
		const LONGLONG now = VideoClock100ns();
		if (g_render.pixels.empty() && g_videoWarmupTarget > 1 && !g_videoWarmupStartedAt)
			g_videoWarmupStartedAt = now;
		const bool warming = g_render.pixels.empty() && g_videoWarmupTarget > 1
			&& g_videoQueueCount < g_videoWarmupTarget
			&& now - g_videoWarmupStartedAt < 5000000; // never delay first display beyond 500 ms
		for (; !warming;) {
			SharedFrame& next = g_videoQueue[g_videoQueueHead];
			if (!g_videoClockBase)
				g_videoClockBase = now - next.mediaTime;
			if (!g_render.pixels.empty() && now < g_videoClockBase + next.mediaTime)
				break;
			std::swap(g_render, next);
			next.width = next.height = 0;
			next.serial = next.generation = 0;
			next.mediaTime = 0;
			g_videoQueueHead = (g_videoQueueHead + 1) % kVideoQueueMax;
			--g_videoQueueCount;
			g_videoQueueDepth.store(g_videoQueueCount, std::memory_order_relaxed);
			consumedVideo = true;
			if (g_playbackPaused.load(std::memory_order_acquire)) {
				g_playbackPausedAt = now;
				break;
			}
			if (g_videoQueueCount <= 0) break;
		}
		if (consumedVideo && g_videoQueueCount == 0 && !g_videoQueueStarved) {
			g_videoQueueStarved = true;
			g_videoQueueUnderruns.fetch_add(1, std::memory_order_relaxed);
		}
	}
	if (g_videoClockBase && !g_playbackPaused.load(std::memory_order_acquire)) {
		const LONGLONG statNow = VideoClock100ns();
		if (!g_videoStatsStartedAt) {
			g_videoStatsStartedAt = statNow;
			g_videoStatsStartedMediaTime = g_render.mediaTime;
			g_videoStatsPresented = 0;
		}
		if (consumedVideo) ++g_videoStatsPresented;
		if (g_videoQueueCount < g_videoStatsMinDepth)
			g_videoStatsMinDepth = g_videoQueueCount;
		const LONGLONG elapsed = statNow - g_videoStatsStartedAt;
		if (elapsed >= 10000000) {
			const int fps10 = (int)(((LONGLONG)g_videoStatsPresented * 100000000
				+ elapsed / 2) / elapsed);
			const LONGLONG mediaElapsed = g_render.mediaTime - g_videoStatsStartedMediaTime;
			int rate = mediaElapsed > 0
				? (int)((mediaElapsed * 100 + elapsed / 2) / elapsed) : 0;
			if (rate < 0) rate = 0;
			if (rate > 200) rate = 200;
			g_videoPresentedFps10.store(fps10, std::memory_order_relaxed);
			g_videoPlaybackRatePercent.store(rate, std::memory_order_relaxed);
			g_videoQueueMinimum.store(g_videoStatsMinDepth, std::memory_order_relaxed);
			g_videoStatsStartedAt = statNow;
			g_videoStatsStartedMediaTime = g_render.mediaTime;
			g_videoStatsPresented = 0;
			g_videoStatsMinDepth = g_videoQueueCapacity.load(std::memory_order_relaxed);
		}
	}
	g_haveFrame.store(!g_render.pixels.empty() || g_sharedReady || g_videoQueueCount > 0,
		std::memory_order_release);
	out->bgra = g_render.pixels.empty() ? nullptr : g_render.pixels.data();
	out->width = g_render.width;
	out->height = g_render.height;
	out->serial = g_render.serial;
	out->fit = SkeetoBackgroundFitMode();
	LeaveCriticalSection(&g_lock);
	if (consumedVideo && g_wake) SetEvent(g_wake);
	return out->bgra && out->width > 0 && out->height > 0;
}

bool SkeetoBackgroundAcquireNativeVideo(SkeetoBackgroundNativeVideo* out) {
	if (!out || !g_lockReady) return false;
	if (g_bikExtract) {
		if (g_bikExtractGeneration != g_generation.load(std::memory_order_acquire)) {
			DlcCacheExtractCancel(g_bikExtract);
			g_bikExtract = nullptr;
			g_bikExtractGeneration = 0;
		} else {
			wchar_t cached[520]{};
			const int state = DlcCacheExtractPump(g_bikExtract, 1024u * 1024u,
				cached, sizeof(cached) / sizeof(cached[0]));
			if (state != 0) {
				DlcCacheExtract* finished = g_bikExtract;
				const unsigned int generation = g_bikExtractGeneration;
				g_bikExtract = nullptr;
				g_bikExtractGeneration = 0;
				char nativePath[520]{};
				const bool ok = state > 0
					&& WidePathToAnsi(cached, nativePath, sizeof(nativePath));
				DlcCacheExtractCancel(finished);
				if (ok) PublishNativeVideo(nativePath, generation);
				else {
					g_loadFailed = true;
					g_loadQueued = false;
					BgLog("Background load failed: VPK BIK cache extraction");
				}
			}
		}
	}
	bool ok = false;
	EnterCriticalSection(&g_lock);
	if (g_nativeVideo.ready
		&& g_nativeVideo.generation == g_generation.load(std::memory_order_acquire)) {
		memset(out, 0, sizeof(*out));
		strncpy(out->path, g_nativeVideo.path, sizeof(out->path) - 1);
		out->generation = g_nativeVideo.generation;
		out->fit = SkeetoBackgroundFitMode();
		ok = out->path[0] != 0;
	}
	LeaveCriticalSection(&g_lock);
	return ok;
}

bool SkeetoBackgroundHasFrame() { return g_haveFrame.load(std::memory_order_acquire); }
bool SkeetoBackgroundLoadFailed() { return g_loadFailed.load(std::memory_order_acquire); }
int SkeetoBackgroundPresentedFps10() { return g_videoPresentedFps10.load(std::memory_order_relaxed); }
int SkeetoBackgroundPlaybackRatePercent() { return g_videoPlaybackRatePercent.load(std::memory_order_relaxed); }
int SkeetoBackgroundQueueDepth() { return g_videoQueueDepth.load(std::memory_order_relaxed); }
int SkeetoBackgroundQueueCapacity() { return g_videoQueueCapacity.load(std::memory_order_relaxed); }
int SkeetoBackgroundQueueMinimum() { return g_videoQueueMinimum.load(std::memory_order_relaxed); }
int SkeetoBackgroundQueueUnderruns() { return g_videoQueueUnderruns.load(std::memory_order_relaxed); }

namespace {
struct SafeMusicConfig {
	char directory[520]{};
	char selected[256]{};
	bool enabled = false, random = false;
};
static SRWLOCK g_safeLock = SRWLOCK_INIT;
static SafeMusicConfig g_safeConfig;
static SkeetoSafeMusicCatalog g_safeCatalog{};
static std::atomic_bool g_safeEnabled{false}, g_safeStop{false}, g_safeScan{false};
static std::atomic_int g_safeVolume{100}, g_safeWakeUsers{0};
static std::atomic_uint g_safeGeneration{1}, g_safeReady{0}, g_safeRequest{0}, g_safeClaimed{0};
static std::atomic<HANDLE> g_safeWake{nullptr};
static HANDLE g_safeWorker = nullptr; // MainThread ownership only.

static void SafeMusicWake() {
	g_safeWakeUsers.fetch_add(1, std::memory_order_acq_rel);
	HANDLE wake = g_safeWake.load(std::memory_order_acquire);
	if (wake) SetEvent(wake);
	g_safeWakeUsers.fetch_sub(1, std::memory_order_release);
}

static SafeMusicConfig SafeMusicSnapshot() {
	AcquireSRWLockShared(&g_safeLock);
	SafeMusicConfig config = g_safeConfig;
	ReleaseSRWLockShared(&g_safeLock);
	return config;
}

static SRWLOCK g_safeResourceLock = SRWLOCK_INIT;
static std::atomic_bool g_safeCatalogReady{false};
static DlcGameGlobScan* g_safeGlob = nullptr;
static DlcCacheExtract* g_safeExtract = nullptr;
static SkeetoSafeMusicCatalog g_safeScanCatalog{};
static unsigned g_safeResourceGeneration = 0, g_safeExtractId = 0, g_safeExtractDoneId = 0;
static int g_safeScanPhase = -1;
// Source request/result fields use g_safeLock; workers receive only native paths.
static char g_safeSourceName[256]{};
static wchar_t g_safeSourcePath[520]{};
static unsigned g_safeSourceId = 0, g_safeSourceGeneration = 0, g_safeSourcePublishedId = 0;

static void SafeMusicCatalogAddName(const char* name, void* rawCatalog) {
	if (!name || !rawCatalog || !name[0] || strpbrk(name, "/\\:\r\n")) return;
	const size_t length = strlen(name);
	if (length >= 256 || length + sizeof("sound/skeeto/safe/") > 260) return;
	const char* extension = strrchr(name, '.');
	if (!extension || (_stricmp(extension, ".wav") && _stricmp(extension, ".mp3"))) return;
	wchar_t validated[256]{};
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, validated, 256)) return;
	SkeetoSafeMusicCatalog* catalog = (SkeetoSafeMusicCatalog*)rawCatalog;
	for (int i = 0; i < catalog->count; ++i)
		if (!_stricmp(catalog->names[i], name)) return;
	if (catalog->count >= 128) { catalog->truncated = true; return; }
	strcpy_s(catalog->names[catalog->count++], name);
}

static void SafeMusicPublishCatalog() {
	qsort(g_safeScanCatalog.names, (size_t)g_safeScanCatalog.count, sizeof(g_safeScanCatalog.names[0]),
		[](const void* a, const void* b) -> int { return _stricmp((const char*)a, (const char*)b); });
	AcquireSRWLockExclusive(&g_safeLock);
	g_safeScanCatalog.revision = g_safeCatalog.revision + 1;
	if (!g_safeScanCatalog.revision) g_safeScanCatalog.revision = 1;
	g_safeCatalog = g_safeScanCatalog;
	ReleaseSRWLockExclusive(&g_safeLock);
	g_safeCatalogReady.store(true, std::memory_order_release);
	SafeMusicWake();
}

// The reader, output and all WAVEHDR storage belong solely to this worker.
// A ready token is published only after an output and first PCM block exist.
static DWORD WINAPI SafeMusicThread(void*) {
	const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	const HRESULT mf = SUCCEEDED(com) ? MFStartup(MF_VERSION, MFSTARTUP_LITE) : E_FAIL;
	unsigned generation = 0;
	int previous = -1;
	unsigned seed = GetTickCount() ^ GetCurrentProcessId();
	IMFSourceReader* reader = nullptr;
	HWAVEOUT output = nullptr;
	WAVEFORMATEX format{};
	AudioBlock blocks[kAudioBlockCount]{};
	bool first = false, playing = false, eof = false, attempted = false, awaitingSource = false;
	unsigned sourceId = 0;
	while (!g_safeStop.load(std::memory_order_acquire)) {
		const unsigned current = g_safeGeneration.load(std::memory_order_acquire);
		if (generation != current) {
			g_safeReady.store(0, std::memory_order_release);
			if (output) { ReleaseAudioBlocks(output, blocks); waveOutClose(output); output = nullptr; }
			if (reader) { reader->Release(); reader = nullptr; }
			first = playing = eof = attempted = awaitingSource = false;
			generation = current;
		}
		if (!g_safeEnabled.load(std::memory_order_acquire) || FAILED(mf)) {
			WaitForSingleObject(g_safeWake.load(), 100);
			continue;
		}
		if (!reader && !attempted && g_safeCatalogReady.load(std::memory_order_acquire)) {
			attempted = true;
			const SafeMusicConfig config = SafeMusicSnapshot();
			char name[256]{};
			AcquireSRWLockShared(&g_safeLock);
			const int count = g_safeCatalog.count;
			int chosen = -1;
			if (count && config.random) {
				seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
				chosen = (int)(seed % (unsigned)count);
				if (count > 1 && chosen == previous) chosen = (chosen + 1) % count;
			} else {
				for (int i = 0; i < count; ++i)
					if (!strcmp(config.selected, g_safeCatalog.names[i])) { chosen = i; break; }
				if (chosen < 0 && !config.selected[0] && count) chosen = 0;
			}
			if (chosen >= 0) strcpy_s(name, g_safeCatalog.names[chosen]);
			ReleaseSRWLockShared(&g_safeLock);
			if (name[0]) {
				AcquireSRWLockExclusive(&g_safeLock);
				strcpy_s(g_safeSourceName, name);
				g_safeSourceGeneration = generation;
				sourceId = ++g_safeSourceId;
				if (!sourceId) sourceId = ++g_safeSourceId;
				ReleaseSRWLockExclusive(&g_safeLock);
				awaitingSource = true;
				previous = chosen;
			}
		}
		if (awaitingSource) {
			wchar_t nativePath[520]{};
			AcquireSRWLockShared(&g_safeLock);
			const bool published = g_safeSourcePublishedId == sourceId;
			if (published) wcscpy_s(nativePath, g_safeSourcePath);
			ReleaseSRWLockShared(&g_safeLock);
			if (published) {
				awaitingSource = false;
				if (nativePath[0] && generation == g_safeGeneration.load(std::memory_order_acquire)
					&& OpenAudioReader(nativePath, &reader, &format)) {
					if (waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
						output = nullptr; reader->Release(); reader = nullptr;
					} else waveOutPause(output);
				}
			}
		}
		if (!reader || !output) { WaitForSingleObject(g_safeWake.load(), 100); continue; }
		int occupied = 0, freeIndex = -1;
		for (int i = 0; i < kAudioBlockCount; ++i) {
			AudioBlock& block = blocks[i];
			if (block.prepared && (block.header.dwFlags & WHDR_DONE)) {
				waveOutUnprepareHeader(output, &block.header, sizeof(WAVEHDR));
				block.prepared = false; block.header = {}; block.bytes.clear();
			}
			if (block.prepared) ++occupied;
			else if (freeIndex < 0) freeIndex = i;
		}
		if (first && !playing) {
			const unsigned request = g_safeRequest.exchange(0, std::memory_order_acq_rel);
			if (request == generation && generation == g_safeGeneration.load(std::memory_order_acquire)) {
				AudioBlock& block = blocks[0];
				ScalePcm16(block.bytes, g_safeVolume.load(std::memory_order_relaxed));
				block.header.lpData = (LPSTR)block.bytes.data();
				block.header.dwBufferLength = (DWORD)block.bytes.size();
				if (waveOutPrepareHeader(output, &block.header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
					block.prepared = true;
					if (waveOutWrite(output, &block.header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR
						&& waveOutRestart(output) == MMSYSERR_NOERROR) playing = true;
				}
				if (!playing) g_safeGeneration.fetch_add(1, std::memory_order_acq_rel);
				continue;
			}
			WaitForSingleObject(g_safeWake.load(), 100);
			continue;
		}
		if (eof) {
			if (!occupied) {
				g_safeClaimed.store(0, std::memory_order_release);
				ReleaseAudioBlocks(output, blocks); waveOutClose(output); output = nullptr;
				reader->Release(); reader = nullptr;
				first = playing = eof = attempted = false;
			} else WaitForSingleObject(g_safeWake.load(), 10);
			continue;
		}
		if (freeIndex < 0) { WaitForSingleObject(g_safeWake.load(), 10); continue; }
		DWORD stream = 0, flags = 0;
		LONGLONG timestamp = 0;
		IMFSample* sample = nullptr;
		HRESULT hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0,
			&stream, &flags, &timestamp, &sample);
		if (FAILED(hr)) eof = true;
		if (sample) {
			IMFMediaBuffer* buffer = nullptr;
			BYTE* bytes = nullptr; DWORD maximum = 0, length = 0;
			bool locked = false;
			if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)) && buffer
				&& SUCCEEDED(buffer->Lock(&bytes, &maximum, &length))) {
				locked = true;
				if (bytes && length && length <= kAudioSampleMax && length % format.nBlockAlign == 0) {
					AudioBlock& block = blocks[freeIndex];
					bool copied = false;
					try { block.bytes.assign(bytes, bytes + length); copied = true; }
					catch (...) { eof = true; g_safeReady.store(0, std::memory_order_release); }
					if (copied && !playing) {
						first = true;
						if (generation == g_safeGeneration.load(std::memory_order_acquire)
							&& g_safeEnabled.load()) g_safeReady.store(generation, std::memory_order_release);
					} else if (copied) {
						ScalePcm16(block.bytes, g_safeVolume.load(std::memory_order_relaxed));
						block.header.lpData = (LPSTR)block.bytes.data(); block.header.dwBufferLength = length;
						if (waveOutPrepareHeader(output, &block.header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
							block.prepared = true;
							if (waveOutWrite(output, &block.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
								g_safeGeneration.fetch_add(1, std::memory_order_acq_rel);
						} else g_safeGeneration.fetch_add(1, std::memory_order_acq_rel);
					}
				} else eof = true;
			} else eof = true;
			if (locked) buffer->Unlock();
			if (buffer) buffer->Release();
			sample->Release();
		}
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) eof = true;
		if (!sample && !eof) WaitForSingleObject(g_safeWake.load(), 10);
		if (eof && !playing && !first) {
			ReleaseAudioBlocks(output, blocks); waveOutClose(output); output = nullptr;
			reader->Release(); reader = nullptr;
		}
	}
	g_safeReady.store(0, std::memory_order_release);
	if (output) { ReleaseAudioBlocks(output, blocks); waveOutClose(output); }
	if (reader) reader->Release();
	if (SUCCEEDED(mf)) MFShutdown();
	if (SUCCEEDED(com)) CoUninitialize();
	return 0;
}
} // namespace

void SkeetoSafeMusicCancel() {
	g_safeGeneration.fetch_add(1, std::memory_order_acq_rel);
	g_safeReady.store(0, std::memory_order_release);
	g_safeRequest.store(0, std::memory_order_release);
	g_safeClaimed.store(0, std::memory_order_release);
	SafeMusicWake();
}

void SkeetoSafeMusicConfigure(const char* directory, bool enabled, int volume, bool random, const char* selected) {
	SafeMusicConfig config{};
	wchar_t wideDirectory[520]{};
	char utf8Directory[520]{};
	if (directory && directory[0]
		&& MultiByteToWideChar(CP_ACP, 0, directory, -1, wideDirectory, (int)_countof(wideDirectory))
		&& WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wideDirectory, -1,
			utf8Directory, (int)sizeof(utf8Directory), nullptr, nullptr)
		&& strlen(utf8Directory) + sizeof("\\sound\\skeeto\\safe") <= sizeof(config.directory))
		sprintf_s(config.directory, "%s\\sound\\skeeto\\safe", utf8Directory);
	if (selected) strncpy_s(config.selected, selected, _TRUNCATE);
	config.enabled = enabled; config.random = random;
	AcquireSRWLockExclusive(&g_safeLock);
	const bool scan = enabled && (!g_safeConfig.enabled || strcmp(config.directory, g_safeConfig.directory));
	const bool changed = config.enabled != g_safeConfig.enabled || config.random != g_safeConfig.random
		|| strcmp(config.directory, g_safeConfig.directory) || strcmp(config.selected, g_safeConfig.selected);
	if (changed) g_safeConfig = config;
	ReleaseSRWLockExclusive(&g_safeLock);
	const int boundedVolume = volume < 0 ? 0 : volume > 100 ? 100 : volume;
	if (boundedVolume != g_safeVolume.load(std::memory_order_relaxed))
		g_safeVolume.store(boundedVolume, std::memory_order_relaxed);
	if (changed) g_safeEnabled.store(enabled, std::memory_order_release);
	if (scan) {
		g_safeCatalogReady.store(false, std::memory_order_release);
		g_safeScan.store(true, std::memory_order_release);
	}
	if (changed) SkeetoSafeMusicCancel();
}

void SkeetoSafeMusicResourcePump(bool allowEngineAccess) {
	// Paint owns all Source handles. MainThread can only abandon them on shutdown.
	AcquireSRWLockExclusive(&g_safeResourceLock);
	const unsigned generation = g_safeGeneration.load(std::memory_order_acquire);
	const bool enabled = g_safeEnabled.load(std::memory_order_acquire);
	if (generation != g_safeResourceGeneration || !enabled) {
		if (g_safeScanPhase >= 0 && enabled) g_safeScan.store(true, std::memory_order_release);
		DlcGameGlobScanCancel(g_safeGlob); g_safeGlob = nullptr;
		DlcCacheExtractCancel(g_safeExtract); g_safeExtract = nullptr;
		g_safeScanPhase = -1;
		g_safeExtractId = g_safeExtractDoneId = 0;
		g_safeResourceGeneration = generation;
	}
	if (!enabled) {
		g_safeScan.store(false, std::memory_order_release);
		g_safeCatalogReady.store(false, std::memory_order_release);
		ReleaseSRWLockExclusive(&g_safeResourceLock);
		return;
	}
	if (!allowEngineAccess) { ReleaseSRWLockExclusive(&g_safeResourceLock); return; }
	if (g_safeScan.exchange(false, std::memory_order_acq_rel)) {
		DlcGameGlobScanCancel(g_safeGlob); g_safeGlob = nullptr;
		g_safeScanCatalog = {};
		g_safeScanPhase = 0;
		g_safeCatalogReady.store(false, std::memory_order_release);
	}
	if (g_safeScanPhase >= 0) {
		const char* glob = g_safeScanPhase == 0 ? "sound/skeeto/safe/*.wav" : "sound/skeeto/safe/*.mp3";
		if (!g_safeGlob) g_safeGlob = DlcGameGlobScanBegin(glob);
		const int result = g_safeGlob
			? DlcGameGlobScanPump(g_safeGlob, 16, SafeMusicCatalogAddName, &g_safeScanCatalog) : -1;
		if (result != 0 || g_safeScanCatalog.truncated) {
			if (result < 0) g_safeScanCatalog.failed = true;
			if (result == 2) g_safeScanCatalog.truncated = true;
			DlcGameGlobScanCancel(g_safeGlob); g_safeGlob = nullptr;
			if (++g_safeScanPhase >= 2 || g_safeScanCatalog.truncated) {
				g_safeScanPhase = -1;
				if (generation == g_safeGeneration.load(std::memory_order_acquire)) SafeMusicPublishCatalog();
				else g_safeScan.store(true, std::memory_order_release);
			}
		}
		ReleaseSRWLockExclusive(&g_safeResourceLock);
		return;
	}
	char name[256]{};
	unsigned id = 0, sourceGeneration = 0;
	AcquireSRWLockShared(&g_safeLock);
	id = g_safeSourceId; sourceGeneration = g_safeSourceGeneration;
	if (id && sourceGeneration == generation) strcpy_s(name, g_safeSourceName);
	ReleaseSRWLockShared(&g_safeLock);
	if (id && id != g_safeExtractDoneId && sourceGeneration == generation) {
		if (id != g_safeExtractId) {
			DlcCacheExtractCancel(g_safeExtract); g_safeExtract = nullptr;
			g_safeExtractId = id;
			char relative[260]{};
			const char* extension = strrchr(name, '.');
			if (name[0] && !strpbrk(name, "/\\:\r\n") && extension
				&& (!_stricmp(extension, ".wav") || !_stricmp(extension, ".mp3")))
				g_safeExtract = (sprintf_s(relative, "sound/skeeto/safe/%s", name) > 0)
					? DlcCacheExtractBegin(relative, !_stricmp(extension, ".wav") ? L".wav" : L".mp3",
						128u * 1024u * 1024u) : nullptr;
		}
		wchar_t nativePath[520]{};
		const int result = g_safeExtract ? DlcCacheExtractPump(g_safeExtract, 256u * 1024u,
			nativePath, _countof(nativePath)) : -1;
		if (result != 0) {
			DlcCacheExtractCancel(g_safeExtract); g_safeExtract = nullptr;
			g_safeExtractDoneId = id;
			AcquireSRWLockExclusive(&g_safeLock);
			if (generation == g_safeGeneration.load(std::memory_order_acquire)
				&& id == g_safeSourceId && generation == g_safeSourceGeneration) {
				if (result > 0) wcscpy_s(g_safeSourcePath, nativePath);
				else g_safeSourcePath[0] = 0;
				g_safeSourcePublishedId = id;
			}
			ReleaseSRWLockExclusive(&g_safeLock);
			SafeMusicWake();
		}
	}
	ReleaseSRWLockExclusive(&g_safeResourceLock);
}

void SkeetoSafeMusicReload() {
	if (!g_safeEnabled.load(std::memory_order_acquire)) return;
	g_safeCatalogReady.store(false, std::memory_order_release);
	g_safeScan.store(true, std::memory_order_release);
	SkeetoSafeMusicCancel();
}

bool SkeetoSafeMusicGetCatalog(SkeetoSafeMusicCatalog* out) {
	if (!out) return false;
	AcquireSRWLockShared(&g_safeLock);
	const bool changed = out->revision != g_safeCatalog.revision;
	if (changed) *out = g_safeCatalog;
	ReleaseSRWLockShared(&g_safeLock);
	return changed;
}

bool SkeetoSafeMusicQueuePlay() {
	if (!g_safeEnabled.load(std::memory_order_acquire) || g_safeStop.load(std::memory_order_acquire)) return false;
	const unsigned generation = g_safeGeneration.load(std::memory_order_acquire);
	if (g_safeClaimed.load(std::memory_order_acquire) == generation) return true;
	unsigned ready = g_safeReady.load(std::memory_order_acquire);
	if (!ready || ready != g_safeGeneration.load(std::memory_order_acquire)) return false;
	if (!g_safeReady.compare_exchange_strong(ready, 0, std::memory_order_acq_rel)) return false;
	g_safeClaimed.store(ready, std::memory_order_release);
	g_safeRequest.store(ready, std::memory_order_release);
	SafeMusicWake();
	return ready == g_safeGeneration.load(std::memory_order_acquire) && g_safeEnabled.load(std::memory_order_acquire);
}

static void SafeMusicStopWorker(bool abandonResources) {
	g_safeStop.store(true, std::memory_order_release);
	SkeetoSafeMusicCancel();
	if (g_safeWorker) {
		WaitForSingleObject(g_safeWorker, INFINITE);
		CloseHandle(g_safeWorker); g_safeWorker = nullptr;
	}
	if (abandonResources) {
		AcquireSRWLockExclusive(&g_safeResourceLock);
		DlcGameGlobScanAbandon(g_safeGlob); g_safeGlob = nullptr;
		DlcCacheExtractAbandon(g_safeExtract); g_safeExtract = nullptr;
		g_safeScanPhase = -1;
		g_safeResourceGeneration = 0;
		g_safeCatalogReady.store(false, std::memory_order_release);
		ReleaseSRWLockExclusive(&g_safeResourceLock);
	}
	HANDLE wake = g_safeWake.exchange(nullptr, std::memory_order_acq_rel);
	while (g_safeWakeUsers.load(std::memory_order_acquire)) Sleep(1);
	if (wake) CloseHandle(wake);
}

void SkeetoSafeMusicShutdown() { SafeMusicStopWorker(true); }

void SkeetoSafeMusicLifecyclePump() {
	if (!g_safeEnabled.load(std::memory_order_acquire)) {
		if (g_safeWorker || g_safeWake.load()) SafeMusicStopWorker(false);
		return;
	}
	if (g_safeWorker) return;
	g_safeStop.store(false, std::memory_order_release);
	HANDLE wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (!wake) return;
	g_safeWake.store(wake, std::memory_order_release);
	g_safeWorker = CreateThread(nullptr, 0, SafeMusicThread, nullptr, 0, nullptr);
	if (!g_safeWorker) {
		wake = g_safeWake.exchange(nullptr, std::memory_order_acq_rel);
		while (g_safeWakeUsers.load(std::memory_order_acquire)) Sleep(1);
		CloseHandle(wake);
	}
}


namespace {
static constexpr const char* kLastMusicPath = "sound/skeeto/the last man/Rainie_Love.mp3";
static SRWLOCK g_lastMusicLock = SRWLOCK_INIT;
static std::atomic_bool g_lastMusicEnabled{false}, g_lastMusicActive{false}, g_lastMusicStop{false};
static std::atomic_int g_lastMusicVolume{100}, g_lastMusicState{0};
static std::atomic_uint g_lastMusicGeneration{1};
static std::atomic_uint g_lastMusicFinished{0};
static std::atomic_uint g_lastMusicStarted{0};
static wchar_t g_lastMusicPath[520]{};
static HANDLE g_lastMusicWorker = nullptr, g_lastMusicWake = nullptr;
// Extraction is Paint-owned; teardown may abandon it after Paint has stopped.
static DlcCacheExtract* g_lastMusicExtract = nullptr;
static bool g_lastMusicScanAttempted = false;
static std::atomic_bool g_lastMusicReload{true};

static void LastMusicWake() {
 AcquireSRWLockShared(&g_lastMusicLock);
 if (g_lastMusicWake) SetEvent(g_lastMusicWake);
 ReleaseSRWLockShared(&g_lastMusicLock);
}
static DWORD WINAPI LastMusicThread(void*) {
 const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
 const HRESULT mf = SUCCEEDED(com) ? MFStartup(MF_VERSION, MFSTARTUP_LITE) : E_FAIL;
 IMFSourceReader* reader = nullptr; HWAVEOUT output = nullptr;
 WAVEFORMATEX format{}; AudioBlock blocks[kAudioBlockCount]{};
 unsigned generation = 0; bool attempted = false, eof = false, started = false;
 while (!g_lastMusicStop.load(std::memory_order_acquire)) {
  const unsigned current = g_lastMusicGeneration.load(std::memory_order_acquire);
  const bool active = g_lastMusicEnabled.load(std::memory_order_acquire) && g_lastMusicActive.load(std::memory_order_acquire);
  if (generation != current || !active) {
   if (output) { ReleaseAudioBlocks(output, blocks); waveOutClose(output); output = nullptr; }
   if (reader) { reader->Release(); reader = nullptr; }
   generation = current; attempted = false; eof = false; started = false;
  }
  if (!active || FAILED(mf)) {
   if (active && FAILED(mf)) g_lastMusicState.store(-1, std::memory_order_release);
   WaitForSingleObject(g_lastMusicWake, 100); continue;
  }
  if (!reader && !attempted && g_lastMusicState.load(std::memory_order_acquire) == 1) {
   wchar_t path[520]{}; AcquireSRWLockShared(&g_lastMusicLock); wcscpy_s(path, g_lastMusicPath); ReleaseSRWLockShared(&g_lastMusicLock);
   attempted = true;
   if (!OpenAudioReader(path, &reader, &format) || waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
    if (reader) { reader->Release(); reader = nullptr; } output = nullptr;
    if (generation == g_lastMusicGeneration.load(std::memory_order_acquire)) g_lastMusicState.store(-1, std::memory_order_release);
   }
  }
  if (!reader || !output) { WaitForSingleObject(g_lastMusicWake, 100); continue; }
  int freeIndex = -1, occupied = 0;
  for (int i = 0; i < kAudioBlockCount; ++i) {
   auto& b = blocks[i];
   if (b.prepared && (b.header.dwFlags & WHDR_DONE)) { waveOutUnprepareHeader(output, &b.header, sizeof(WAVEHDR)); b.prepared = false; b.header = {}; b.bytes.clear(); }
   if (!b.prepared && freeIndex < 0) freeIndex = i;
   if (b.prepared) ++occupied;
  }
  if (eof) {
   if (!occupied) {
    ReleaseAudioBlocks(output, blocks); waveOutClose(output); output = nullptr;
    reader->Release(); reader = nullptr; eof = false;
    g_lastMusicFinished.store(generation, std::memory_order_release);
   } else WaitForSingleObject(g_lastMusicWake, 10);
   continue;
  }
  if (freeIndex < 0) { WaitForSingleObject(g_lastMusicWake, 10); continue; }
  DWORD stream = 0, flags = 0; LONGLONG timestamp = 0; IMFSample* sample = nullptr;
  const HRESULT hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &stream, &flags, &timestamp, &sample);
  bool ok = SUCCEEDED(hr);
  if (sample) {
   IMFMediaBuffer* buffer = nullptr; BYTE* bytes = nullptr; DWORD maximum = 0, length = 0;
   if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer)) && buffer && SUCCEEDED(buffer->Lock(&bytes, &maximum, &length))) {
    if (bytes && length && length <= kAudioSampleMax && length % format.nBlockAlign == 0) {
     auto& b = blocks[freeIndex];
     try { b.bytes.assign(bytes, bytes + length); } catch (...) { ok = false; }
     if (ok) {
     ScalePcm16(b.bytes, g_lastMusicVolume.load(std::memory_order_relaxed));
     b.header = {}; b.header.lpData = (LPSTR)b.bytes.data(); b.header.dwBufferLength = length;
     // Recheck bounded intent after a potentially blocking MF read.
     if (generation == g_lastMusicGeneration.load(std::memory_order_acquire) && g_lastMusicActive.load(std::memory_order_acquire)
      && g_lastMusicEnabled.load(std::memory_order_acquire) && !g_lastMusicStop.load(std::memory_order_acquire)) {
      if (waveOutPrepareHeader(output, &b.header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
       b.prepared = true; ok = waveOutWrite(output, &b.header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR;
       if (ok && !started) { started = true; g_lastMusicStarted.store(generation, std::memory_order_release); }
      } else ok = false;
     }
     }
    } else ok = false;
    buffer->Unlock();
   } else ok = false;
   if (buffer) buffer->Release(); sample->Release();
  }
  if (flags & MF_SOURCE_READERF_ENDOFSTREAM) eof = true;
  if (!sample && !(flags & MF_SOURCE_READERF_ENDOFSTREAM) && ok) WaitForSingleObject(g_lastMusicWake, 10);
  if (!ok) {
   ReleaseAudioBlocks(output, blocks); waveOutClose(output); output = nullptr; reader->Release(); reader = nullptr;
   if (generation == g_lastMusicGeneration.load(std::memory_order_acquire)) g_lastMusicState.store(-1, std::memory_order_release);
  }
 }
 if (output) { ReleaseAudioBlocks(output, blocks); waveOutClose(output); }
 if (reader) reader->Release(); if (SUCCEEDED(mf)) MFShutdown(); if (SUCCEEDED(com)) CoUninitialize();
 return 0;
}
static void LastMusicStopWorker() {
 g_lastMusicStop.store(true, std::memory_order_release); LastMusicWake();
 if (g_lastMusicWorker) { WaitForSingleObject(g_lastMusicWorker, INFINITE); CloseHandle(g_lastMusicWorker); g_lastMusicWorker = nullptr; }
 AcquireSRWLockExclusive(&g_lastMusicLock);
 if (g_lastMusicWake) { CloseHandle(g_lastMusicWake); g_lastMusicWake = nullptr; }
 ReleaseSRWLockExclusive(&g_lastMusicLock);
}
}

void SkeetoLastMusicConfigure(bool enabled, int volumePercent) {
 if (volumePercent < 0) volumePercent = 0; if (volumePercent > 100) volumePercent = 100;
 g_lastMusicVolume.store(volumePercent, std::memory_order_relaxed);
 if (g_lastMusicEnabled.exchange(enabled, std::memory_order_acq_rel) == enabled) return;
 SkeetoLastMusicSetActive(false); SkeetoLastMusicReload();
}
void SkeetoLastMusicSetActive(bool active) {
 active = active && g_lastMusicEnabled.load(std::memory_order_acquire);
 if (g_lastMusicActive.exchange(active, std::memory_order_acq_rel) == active
  && (!active || g_lastMusicFinished.load(std::memory_order_acquire) != g_lastMusicGeneration.load(std::memory_order_acquire))) return;
 g_lastMusicGeneration.fetch_add(1, std::memory_order_acq_rel); LastMusicWake();
}
void SkeetoLastMusicReload() {
 g_lastMusicReload.store(true, std::memory_order_release); g_lastMusicState.store(0, std::memory_order_release);
 g_lastMusicGeneration.fetch_add(1, std::memory_order_acq_rel);
 LastMusicWake();
}
void SkeetoLastMusicResourcePump(bool allowEngineAccess) {
 if (g_lastMusicReload.exchange(false, std::memory_order_acq_rel)) {
  DlcCacheExtractCancel(g_lastMusicExtract); g_lastMusicExtract = nullptr; g_lastMusicScanAttempted = false;
  AcquireSRWLockExclusive(&g_lastMusicLock); g_lastMusicPath[0] = 0; ReleaseSRWLockExclusive(&g_lastMusicLock);
 }
 if (!g_lastMusicEnabled.load(std::memory_order_acquire) || !allowEngineAccess) return;
 if (!g_lastMusicScanAttempted) { g_lastMusicScanAttempted = true; g_lastMusicExtract = DlcCacheExtractBegin(kLastMusicPath, L".mp3", 128u * 1024u * 1024u); }
 if (!g_lastMusicExtract) { if (!g_lastMusicState.load(std::memory_order_acquire)) g_lastMusicState.store(-1, std::memory_order_release); return; }
 wchar_t path[520]{};
 const int result = DlcCacheExtractPump(g_lastMusicExtract, 256u * 1024u, path, _countof(path));
 if (!result) return;
 DlcCacheExtractCancel(g_lastMusicExtract); g_lastMusicExtract = nullptr;
 AcquireSRWLockExclusive(&g_lastMusicLock); if (result > 0) wcscpy_s(g_lastMusicPath, path); ReleaseSRWLockExclusive(&g_lastMusicLock);
 g_lastMusicState.store(result > 0 ? 1 : -1, std::memory_order_release); LastMusicWake();
}
int SkeetoLastMusicState() { return g_lastMusicState.load(std::memory_order_acquire); }
unsigned SkeetoLastMusicStartedGeneration() {
 const unsigned started = g_lastMusicStarted.load(std::memory_order_acquire);
 return g_lastMusicEnabled.load(std::memory_order_acquire) && g_lastMusicActive.load(std::memory_order_acquire)
  && started == g_lastMusicGeneration.load(std::memory_order_acquire) ? started : 0;
}
void SkeetoLastMusicLifecyclePump() {
 if (!g_lastMusicEnabled.load(std::memory_order_acquire)) { if (g_lastMusicWorker) LastMusicStopWorker(); return; }
 if (g_lastMusicWorker) return;
 g_lastMusicStop.store(false, std::memory_order_release);
 AcquireSRWLockExclusive(&g_lastMusicLock); g_lastMusicWake = CreateEventW(nullptr, FALSE, FALSE, nullptr); ReleaseSRWLockExclusive(&g_lastMusicLock);
 if (!g_lastMusicWake) return;
 g_lastMusicWorker = CreateThread(nullptr, 0, LastMusicThread, nullptr, 0, nullptr);
 if (!g_lastMusicWorker) { LastMusicStopWorker(); g_lastMusicState.store(-1, std::memory_order_release); }
}
void SkeetoLastMusicShutdown() {
 g_lastMusicEnabled.store(false, std::memory_order_release); g_lastMusicActive.store(false, std::memory_order_release); LastMusicStopWorker();
 DlcCacheExtractAbandon(g_lastMusicExtract); g_lastMusicExtract = nullptr;
}
