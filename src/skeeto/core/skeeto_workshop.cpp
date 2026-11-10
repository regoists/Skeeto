#include "skeeto_ui_text.h"
#include "skeeto_workshop.h"
#include "skeeto_runtime.h"
#include "skeeto_version.generated.h"

#include <Windows.h>
#include <shellapi.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <bcrypt.h>

extern bool g_optWorkshopUpdate;
extern char g_workshopId[24];

const char kSkeetoWorkshopIdDefault[] = "3787239937";
// The maintenance engine reads this from the installed core. It replaces a
// game-root version file; the workshop package still carries VERSION.txt.
static const char kSkeetoInstalledSemver[] = "SkeetoInstalledSemver=" SKEETO_VERSION;

// The updater wire protocol is deliberately independent of install_layout.
// DLL owns only discovery, the fixed envelope, user consent and invocation.
// The downloaded engine owns validation, READY and all installation mutations.
static bool SkeetoUtf8ToWide(const char* u8, wchar_t* out, int cap) {
	if (!u8 || !out || cap < 1) return false;
	const int n = MultiByteToWideChar(CP_UTF8, 0, u8, -1, out, cap);
	return n > 0;
}

// SkeetoGameRoot uses GetModuleFileNameA: its directories are ACP, whereas
// package document leaves are UTF-8. Convert each component before joining.
static bool SkeetoRootPathW(const char* rootA, const char* leafUtf8, wchar_t* out, int cap) {
	wchar_t root[MAX_PATH * 2]{}, leaf[MAX_PATH]{};
	if (!MultiByteToWideChar(CP_ACP, 0, rootA, -1, root, _countof(root)) ||
		!SkeetoUtf8ToWide(leafUtf8, leaf, _countof(leaf))) return false;
	const size_t needed = wcslen(root) + wcslen(leaf) + 2;
	if (needed > static_cast<size_t>(cap)) return false;
	swprintf_s(out, cap, L"%s\\%s", root, leaf);
	return true;
}

static const char kSkeetoWsChangelogPendingFile[] = "skeeto_ws_show_changelog.txt";
static const char kSkeetoWsChangelogShownFile[] = "skeeto_ws_changelog_shown.txt";
// Staging-only Apply failure receipt (under skeeto_update/). Not a game-root file.
static const char kSkeetoWsApplyFailedFile[] = "skeeto_update\\apply_failed.txt";
static const char kSkeetoWsApplyResultFile[] = "skeeto_update\\apply_result.txt";
static bool SkeetoReadSemverFile(const char* path, char* out, size_t n);
static bool SkeetoReadSmallTextFile(const char* path, char* out, size_t cap) {
	if (!path || !out || cap < 2) return false;
	out[0] = 0;
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	size_t n = fread(out, 1, cap - 1, f);
	fclose(f);
	out[n] = 0;
	while (n && (out[n - 1] == '\n' || out[n - 1] == '\r' || out[n - 1] == ' '))
		out[--n] = 0;
	return n > 0;
}

static void SkeetoWorkshopWriteVersionMarker(const char* leaf, const char* ver) {
	if (!leaf || !leaf[0] || !ver || !ver[0]) return;
	char root[MAX_PATH]{};
	SkeetoGameRoot(root, sizeof(root));
	if (!root[0]) return;
	char path[MAX_PATH]{};
	snprintf(path, sizeof(path), "%s\\%s", root, leaf);
	FILE* f = fopen(path, "w");
	if (!f) {
		Log("workshop: failed to write marker %s err=%lu", leaf, GetLastError());
		return;
	}
	fprintf(f, "semver=%s\n", ver);
	fclose(f);
}

static bool SkeetoWorkshopVersionMarkerMatches(const char* leaf, const char* ver) {
	if (!leaf || !leaf[0] || !ver || !ver[0]) return false;
	char root[MAX_PATH]{}, path[MAX_PATH]{}, line[64]{};
	SkeetoGameRoot(root, sizeof(root));
	if (!root[0]) return false;
	snprintf(path, sizeof(path), "%s\\%s", root, leaf);
	FILE* f = fopen(path, "r");
	if (!f) return false;
	const bool readOk = fgets(line, sizeof(line), f) != nullptr;
	fclose(f);
	if (!readOk) return false;
	char* p = line;
	while (*p == ' ' || *p == '\t') ++p;
	if (!_strnicmp(p, "semver=", 7)) p += 7;
	char* end = p + strlen(p);
	while (end > p && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t'))
		*--end = 0;
	return _stricmp(p, ver) == 0;
}

static void SkeetoWorkshopMarkChangelogPending(const char* ver) {
	if (!g_optWorkshopUpdate || !ver || !ver[0]) return;
	SkeetoWorkshopWriteVersionMarker(kSkeetoWsChangelogPendingFile, ver);
	Log("workshop: marked changelog pending (%s)", ver);
}

void SkeetoWorkshopTryOpenChangelog() {
	if (!g_optWorkshopUpdate) return;
	char root[MAX_PATH]{};
	SkeetoGameRoot(root, sizeof(root));
	if (!root[0]) return;
	char path[MAX_PATH]{};
	snprintf(path, sizeof(path), "%s\\%s", root, kSkeetoWsChangelogPendingFile);
	if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return;
	char ver[32]{};
	FILE* f = fopen(path, "r");
	if (f) {
		char line[64]{};
		if (fgets(line, sizeof(line), f)) {
			char* p = line;
			while (*p == ' ' || *p == '\t') ++p;
			if (!_strnicmp(p, "semver=", 7)) p += 7;
			size_t i = 0;
			while (p[i] && p[i] != '\r' && p[i] != '\n' && i + 1 < sizeof(ver)) {
				ver[i] = p[i];
				++i;
			}
			ver[i] = 0;
		}
		fclose(f);
	}
	if (ver[0] && SkeetoWorkshopVersionMarkerMatches(kSkeetoWsChangelogShownFile, ver)) {
		DeleteFileA(path);
		return;
	}
	Log("workshop: opening changelog pending=%s running=%s", ver[0] ? ver : "?", SKEETO_VERSION);
	char readyPath[MAX_PATH]{}, stagedVersion[MAX_PATH]{}, stagedVer[32]{};
	snprintf(readyPath, sizeof(readyPath), "%s\\skeeto_update\\READY", root);
	snprintf(stagedVersion, sizeof(stagedVersion), "%s\\skeeto_update\\VERSION.txt", root);
	// Prepared updates publish no root files in-process. Show the verified stage
	// log until the exit transaction has installed the new root document.
	const bool staged = GetFileAttributesA(readyPath) != INVALID_FILE_ATTRIBUTES &&
		SkeetoReadSemverFile(stagedVersion, stagedVer, sizeof(stagedVer)) && ver[0] && !_stricmp(ver, stagedVer);
	if (!SkeetoOpenRootDoc(staged ? u8"skeeto_update\\Skeeto_更新日志.txt" : u8"Skeeto_更新日志.txt", nullptr)) {
		Log("workshop: changelog open failed — keeping %s", kSkeetoWsChangelogPendingFile);
		return;
	}
	if (ver[0])
		SkeetoWorkshopWriteVersionMarker(kSkeetoWsChangelogShownFile, ver);
	DeleteFileA(path);
	Log("workshop: changelog opened, cleared pending marker");
}

bool SkeetoOpenRootDoc(const char* utf8Leaf, const wchar_t* missingTip) {
	if (!utf8Leaf || !utf8Leaf[0]) return false;
	char root[MAX_PATH]{};
	SkeetoGameRoot(root, sizeof(root));
	if (!root[0]) {
		if (missingTip) MessageBoxW(nullptr, missingTip, L"Skeeto", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
		return false;
	}
	char pathUtf8[MAX_PATH * 2]{};
	snprintf(pathUtf8, sizeof(pathUtf8), "%s\\%s", root, utf8Leaf);
	wchar_t pathW[MAX_PATH * 2]{};
	wchar_t rootW[MAX_PATH]{};
	if (!SkeetoRootPathW(root, utf8Leaf, pathW, _countof(pathW)) ||
		GetFileAttributesW(pathW) == INVALID_FILE_ATTRIBUTES) {
		Log("open doc missing: %s", pathUtf8);
		if (!missingTip) return false;
		wchar_t body[640]{};
		if (missingTip[0])
			wcsncpy(body, missingTip, 319);
		else
			wcsncpy(body, UiText(UiId::Text_d3369181, L"找不到文件。"), 319);
		body[319] = 0;
		wchar_t tail[360]{};
		swprintf_s(tail, L"\n\n%s", pathW[0] ? pathW : L"?");
		wcsncat(body, tail, 319);
		MessageBoxW(nullptr, body, L"Skeeto", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
		return false;
	}
	MultiByteToWideChar(CP_ACP, 0, root, -1, rootW, _countof(rootW));
	HWND owner = FindWindowW(L"Valve001", nullptr);
	if (!owner) owner = GetForegroundWindow();
	Log("open doc: %s owner=%p", pathUtf8, owner);
	const HINSTANCE r = ShellExecuteW(owner, L"open", pathW, nullptr,
		rootW[0] ? rootW : nullptr, SW_SHOW);
	const INT_PTR code = (INT_PTR)r;
	Log("ShellExecuteW open doc code=%Id", code);
	if (code > 32) return true;
	wchar_t npath[MAX_PATH * 2]{};
	swprintf_s(npath, L"notepad.exe \"%s\"", pathW);
	STARTUPINFOW siN{};
	siN.cb = sizeof(siN);
	PROCESS_INFORMATION piN{};
	if (CreateProcessW(nullptr, npath, nullptr, nullptr, FALSE, 0, nullptr,
			rootW[0] ? rootW : nullptr, &siN, &piN)) {
		CloseHandle(piN.hThread);
		CloseHandle(piN.hProcess);
		Log("open doc via notepad ok");
		return true;
	}
	wchar_t cmdLine[1024]{};
	swprintf_s(cmdLine, L"cmd.exe /c start \"\" \"%s\"", pathW);
	STARTUPINFOW si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	if (CreateProcessW(nullptr, cmdLine, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
			rootW[0] ? rootW : nullptr, &si, &pi)) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		Log("open doc via cmd start ok");
		return true;
	}
	Log("open doc failed err=%lu", GetLastError());
	if (missingTip) MessageBoxW(nullptr, UiText(UiId::Text_1590173d, L"无法用系统默认程序打开该文件。"), L"Skeeto",
		MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
	return false;
}

static bool SkeetoParseSemver(const char* s, int* a, int* b, int* c) {
	if (!s || !a || !b || !c) return false;
	while (*s == ' ' || *s == '\t') ++s;
	int parts[3]{};
	for (int i = 0; i < 3; ++i) {
		if (!isdigit((unsigned char)*s)) return false;
		unsigned long v = 0;
		do {
			v = v * 10u + (unsigned long)(*s - '0');
			if (v > 2147483647ul) return false;
			++s;
		} while (isdigit((unsigned char)*s));
		parts[i] = (int)v;
		if (i < 2) {
			if (*s != '.') return false;
			++s;
		}
	}
	while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') ++s;
	if (*s) return false;
	*a = parts[0]; *b = parts[1]; *c = parts[2];
	return true;
}

static int SkeetoCmpSemver(const char* left, const char* right) {
	int la = 0, lb = 0, lc = 0, ra = 0, rb = 0, rc = 0;
	if (!SkeetoParseSemver(left, &la, &lb, &lc) || !SkeetoParseSemver(right, &ra, &rb, &rc))
		return 0;
	if (la != ra) return (la < ra) ? -1 : 1;
	if (lb != rb) return (lb < rb) ? -1 : 1;
	if (lc != rc) return (lc < rc) ? -1 : 1;
	return 0;
}

static bool SkeetoReadSemverFile(const char* path, char* out, size_t n) {
	if (!path || !out || n < 8) return false;
	out[0] = 0;
	FILE* f = fopen(path, "r");
	if (!f) return false;
	char line[256]{};
	for (int lines = 0; lines < 32 && fgets(line, sizeof(line), f); ++lines) {
		char* p = line;
		while (*p == ' ' || *p == '\t') ++p;
		if (!_strnicmp(p, "semver=", 7)) {
			p += 7;
			while (*p == ' ' || *p == '\t') ++p;
			size_t i = 0;
			while (p[i] && p[i] != '\r' && p[i] != '\n' && p[i] != ' ' && i + 1 < n) {
				out[i] = p[i];
				++i;
			}
			out[i] = 0;
			fclose(f);
			int a = 0, b = 0, c = 0;
			if (!SkeetoParseSemver(out, &a, &b, &c)) {
				out[0] = 0;
				return false;
			}
			return true;
		}
		int a = 0, b = 0, c = 0;
		const char* q = !_strnicmp(p, "Skeeto ", 7) ? p + 7 : p;
		if (!out[0] && SkeetoParseSemver(q, &a, &b, &c)) {
			snprintf(out, n, "%d.%d.%d", a, b, c);
			// Legacy display heading is only a fallback. The machine-readable
			// semver field below it is authoritative for update discovery.
		}
	}
	fclose(f);
	return out[0] != 0;
}

static bool SkeetoRunHidden(const char* app, char* cmdLineMutable) {
	STARTUPINFOA si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi{};
	// CREATE_NEW_CONSOLE + SW_HIDE: give console apps a real (hidden) console.
	// CREATE_NO_WINDOW asks WOW64 for a special hidden conhost and can return
	// 0xc0000142 on some Win10 LTSC hosts before user code. DETACHED_PROCESS
	// avoids that path but powershell/tar exit 0 immediately without running.
	if (!CreateProcessA(app, cmdLineMutable, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE | BELOW_NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi))
		return false;
	DWORD code = 1;
	if (WaitForSingleObject(pi.hProcess, 120000) == WAIT_OBJECT_0) GetExitCodeProcess(pi.hProcess, &code);
	else {
		TerminateProcess(pi.hProcess, ERROR_TIMEOUT);
		WaitForSingleObject(pi.hProcess, 5000);
		Log("workshop: extraction helper timed out; stopped before retry");
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return code == 0;
}

static bool SkeetoTarExtract(const char* zipPath, const char* destDir, const char* filesJoined) {
	char tarExe[MAX_PATH]{};
	CreateDirectoryA(destDir, nullptr);
	char cmd[1400]{};
	if (SearchPathA(nullptr, "tar.exe", nullptr, MAX_PATH, tarExe, nullptr)) {
		if (filesJoined && filesJoined[0]) snprintf(cmd, sizeof(cmd), "\"%s\" -xf \"%s\" -C \"%s\" %s", tarExe, zipPath, destDir, filesJoined);
		else snprintf(cmd, sizeof(cmd), "\"%s\" -xf \"%s\" -C \"%s\"", tarExe, zipPath, destDir);
		if (SkeetoRunHidden(tarExe, cmd)) return true;
	}
	// tar is absent on some installations. Use the same system PowerShell
	// already required by the engine; no extra EXE/script or platform dependency.
	char system[MAX_PATH]{}, powershell[MAX_PATH]{};
	if (!GetSystemDirectoryA(system, MAX_PATH)) return false;
	snprintf(powershell, sizeof(powershell), "%s\\WindowsPowerShell\\v1.0\\powershell.exe", system);
	char escapedZip[MAX_PATH * 2]{}, escapedDest[MAX_PATH * 2]{};
	auto quote = [](const char* input, char* output, size_t capacity) {
		size_t at = 0;
		for (; *input; ++input) {
			if (at + (*input == '\'' ? 2 : 1) >= capacity) return false;
			output[at++] = *input; if (*input == '\'') output[at++] = '\'';
		}
		output[at] = 0; return true;
	};
	if (!quote(zipPath, escapedZip, sizeof(escapedZip)) || !quote(destDir, escapedDest, sizeof(escapedDest))) return false;
	char fallback[MAX_PATH * 12]{};
	snprintf(fallback, sizeof(fallback), "\"%s\" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"$ErrorActionPreference='Stop'; Add-Type -AssemblyName System.IO.Compression.FileSystem; $r=[IO.Path]::GetFullPath('%s').TrimEnd([char]92)+[char]92; $z=[IO.Compression.ZipFile]::OpenRead('%s'); try {foreach($e in $z.Entries){%s $p=[IO.Path]::GetFullPath($r+$e.FullName); if(-not $p.StartsWith($r,[StringComparison]::OrdinalIgnoreCase)){throw 'Archive path escapes stage'}; if($e.Name){[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($p))|Out-Null; [IO.Compression.ZipFileExtensions]::ExtractToFile($e,$p,$true)}}} finally {$z.Dispose()}\"",
		powershell, escapedDest, escapedZip,
		filesJoined ? "if($e.FullName -cnotin @('SKEETO_WORKSHOP.txt','VERSION.txt')){continue};" : "");
	return SkeetoRunHidden(powershell, fallback);
}

static bool SkeetoIsWorkshopPackageDir(const char* dir) {
	char marker[MAX_PATH]{};
	snprintf(marker, sizeof(marker), "%s\\SKEETO_WORKSHOP.txt", dir);
	FILE* f = fopen(marker, "r");
	if (!f) return false;
	char buf[256]{};
	size_t n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = 0;
	return strstr(buf, "SkeetoWorkshopPackage=1") != nullptr;
}

// These are the protocol-1 envelope, not the current package layout. Keep the
// endpoint and verbs stable when core names, manifests or directories change.
static constexpr unsigned kSkeetoUpdateProtocol = 1;
static bool g_wsManualRequired = false;
static HANDLE g_wsApplyProcess = nullptr; // One worker-owned handle; released on helper exit/process exit.
static std::atomic_bool g_wsUpdateEnabled{false};
static bool SkeetoReadUpdateField(const char* dir, const char* key, char* out, size_t cap) {
    char path[MAX_PATH * 2]{}; snprintf(path, sizeof(path), "%s\\SKEETO_WORKSHOP.txt", dir);
    FILE* f = fopen(path, "r"); if (!f) return false;
    char line[256]{}; bool found = false;
    for (int i = 0; i < 32 && fgets(line, sizeof(line), f); ++i) {
        const size_t keySize = strlen(key);
        if (strncmp(line, key, keySize) || line[keySize] != '=') continue;
        char* value = line + keySize + 1;
        const size_t n = strcspn(value, "\r\n");
        if (n && n < cap) { memcpy(out, value, n); out[n] = 0; found = true; }
        break;
    }
    fclose(f); return found;
}

// Worker/one accepted package only: <=1 MiB, 8 KiB buffers; no game API and no
// cached handles. This checks completeness, not publisher authentication.
static bool SkeetoUpdateEngineHash(const wchar_t* path, const char* expected) {
    if (strlen(expected) != 64) return false;
    for (int i = 0; i < 64; ++i) if (!isxdigit(static_cast<unsigned char>(expected[i]))) return false;
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{}; BCRYPT_ALG_HANDLE alg{}; BCRYPT_HASH_HANDLE hash{};
    BYTE digest[32]{}, buffer[8192]{}; DWORD count = 0, total = 0;
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart <= 1024 * 1024
        && BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0
        && BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) >= 0;
    while (ok && total < static_cast<DWORD>(size.QuadPart)) {
        if (!ReadFile(file, buffer, sizeof(buffer), &count, nullptr) || !count) { ok = false; break; }
        total += count;
        ok = BCryptHashData(hash, buffer, count, 0) >= 0;
    }
    if (ok) ok = total == static_cast<DWORD>(size.QuadPart) && BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(file);
    char actual[65]{};
    if (ok) for (int i = 0; i < 32; ++i) sprintf_s(actual + i * 2, 3, "%02x", digest[i]);
    return ok && !_stricmp(actual, expected);
}

static void SkeetoRequireManualUpdateOnce() {
    g_wsManualRequired = true;
    static bool notified = false;
    if (notified) return;
    notified = true;
    MessageBoxW(nullptr, UiText(UiId::BootstrapUnsupportedLayout,
        L"该更新需要人工处理，或更新协议不受当前客户端支持。\n当前安装未改变。请保留设置和自定义内容，查看暂存包说明，必要时运行维护入口处理。"),
        L"Skeeto Update", MB_OK | MB_ICONINFORMATION);
}

// Result: 0=ready/success, 1=incomplete/retry, 2=manual migration. Only this
// endpoint/command contract is frozen; the engine's manifest/layout is opaque.
static DWORD SkeetoInvokeUpdateEngine(const char* payload, const char* gameRoot, bool apply, HANDLE* ownedProcess = nullptr) {
    char protocol[16]{}, expected[65]{};
    if (!kSkeetoInstalledSemver[0]) return 1;
    if (!SkeetoIsWorkshopPackageDir(payload)) return 1;
    if (!SkeetoReadUpdateField(payload, "update_protocol", protocol, sizeof(protocol)) || strcmp(protocol, "1")) return 2;
    wchar_t script[MAX_PATH * 2]{}, root[MAX_PATH * 2]{}, source[MAX_PATH * 2]{};
    if (!SkeetoRootPathW(payload, "Skeeto\\Maintain.ps1", script, _countof(script))
        || !MultiByteToWideChar(CP_ACP, 0, gameRoot, -1, root, _countof(root))
        || !MultiByteToWideChar(CP_ACP, 0, payload, -1, source, _countof(source))
        || !SkeetoReadUpdateField(payload, "engine_sha256", expected, sizeof(expected))
        || !SkeetoUpdateEngineHash(script, expected)) return 1;
    wchar_t command[MAX_PATH * 12]{}, system[MAX_PATH]{}, executable[MAX_PATH * 2]{};
    if (!GetSystemDirectoryW(system, _countof(system))) return 1;
    swprintf_s(executable, L"%s\\WindowsPowerShell\\v1.0\\powershell.exe", system);
    FILETIME created{}, exited{}, kernel{}, user{};
    ULARGE_INTEGER start{};
    wchar_t eventName[100]{};
    HANDLE acknowledged = nullptr;
    if (apply) {
        if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return 1;
        start.LowPart = created.dwLowDateTime; start.HighPart = created.dwHighDateTime;
        swprintf_s(eventName, L"Local\\SkeetoUpdateStarted.%lu.%llu", GetCurrentProcessId(), GetTickCount64());
        acknowledged = CreateEventW(nullptr, TRUE, FALSE, eventName);
        if (!acknowledged) return 1;
    }
    swprintf_s(command, L"\"%s\" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%s\" -Mode %s -GameRoot \"%s\" -PayloadRoot \"%s\" -UpdateProtocol %u -NonInteractive -WaitPid %lu -WaitStartTime %llu %s%s%s",
        executable, script, apply ? L"Apply" : L"Prepare", root, source, kSkeetoUpdateProtocol,
        apply ? static_cast<unsigned long>(GetCurrentProcessId()) : 0ul, start.QuadPart,
        apply ? L"-StartedEvent \"" : L"", eventName, apply ? L"\"" : L"");
    STARTUPINFOW startup{}; startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    // Same 32-bit Windows PowerShell 5.1, uninherited handles, StartedEvent and
    // helper-only timeout. CREATE_NEW_CONSOLE + SW_HIDE / -WindowStyle Hidden
    // allocates a normal hidden console. CREATE_NO_WINDOW can 0xc0000142 in
    // WOW64 conhost init on some LTSC hosts; DETACHED_PROCESS exits 0 without
    // running Maintain (no READY / no StartedEvent ACK). Offline A/B confirmed.
    if (!CreateProcessW(executable, command, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE | BELOW_NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startup, &process)) {
        if (acknowledged) CloseHandle(acknowledged);
        return 1;
    }
    DWORD code = 0;
    if (!apply) {
        const DWORD waited = WaitForSingleObject(process.hProcess, 120000);
        if (waited == WAIT_OBJECT_0) { if (!GetExitCodeProcess(process.hProcess, &code)) code = 1; }
        else {
            // Stop our timed-out preparation helper only, never the game.
            TerminateProcess(process.hProcess, ERROR_TIMEOUT);
            WaitForSingleObject(process.hProcess, 5000);
            code = 1;
            Log("workshop: update preparation timed out");
        }
    } else {
        HANDLE waitFor[] = {acknowledged, process.hProcess};
        DWORD ack = WaitForMultipleObjects(2, waitFor, FALSE, 15000);
        if (ack != WAIT_OBJECT_0) {
            DWORD helperExit = STILL_ACTIVE;
            GetExitCodeProcess(process.hProcess, &helperExit);
            // AV/slow PowerShell can take longer than 15s to reach StartedEvent.
            // If the helper is still alive, wait one more stretch before killing it.
            if (helperExit == STILL_ACTIVE && ack == WAIT_TIMEOUT)
                ack = WaitForMultipleObjects(2, waitFor, FALSE, 45000);
            if (ack != WAIT_OBJECT_0) {
                GetExitCodeProcess(process.hProcess, &helperExit);
                if (helperExit == STILL_ACTIVE) {
                    TerminateProcess(process.hProcess, ERROR_TIMEOUT);
                    WaitForSingleObject(process.hProcess, 5000);
                    GetExitCodeProcess(process.hProcess, &helperExit);
                }
                code = 1;
                Log("workshop: exit updater did not acknowledge startup exit=0x%08lX; retry later", helperExit);
            } else if (ownedProcess) {
                *ownedProcess = process.hProcess;
                process.hProcess = nullptr;
            }
        } else if (ownedProcess) {
            *ownedProcess = process.hProcess;
            process.hProcess = nullptr;
        }
    }
    if (acknowledged) CloseHandle(acknowledged);
    CloseHandle(process.hThread); if (process.hProcess) CloseHandle(process.hProcess);
    return code;
}

static bool SkeetoWorkshopFileLooksZip(const char* zipPath) {
	FILE* zf = fopen(zipPath, "rb");
	if (!zf) return false;
	unsigned char mag[2]{};
	const size_t nr = fread(mag, 1, 2, zf);
	fclose(zf);
	return nr >= 2 && mag[0] == 'P' && mag[1] == 'K';
}

// Used only for the two exact updater-owned scratch/stage directories.
// Recursive cleanup prevents files from an older package participating in a
// later validation. It is never called on the game root.
static bool SkeetoClearDirFiles(const char* dir) {
	if (!dir || !dir[0]) return false;
	const DWORD ownAttributes = GetFileAttributesA(dir);
	if (ownAttributes != INVALID_FILE_ATTRIBUTES && (ownAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
	char delPat[MAX_PATH]{};
	snprintf(delPat, sizeof(delPat), "%s\\*.*", dir);
	WIN32_FIND_DATAA dd{};
	CreateDirectoryA(dir, nullptr);
	HANDLE hd = FindFirstFileA(delPat, &dd);
	if (hd == INVALID_HANDLE_VALUE) return true;
	bool ok = true;
	do {
		if (!strcmp(dd.cFileName, ".") || !strcmp(dd.cFileName, "..")) continue;
		if (dd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) { ok = false; continue; }
		char fp[MAX_PATH]{};
		snprintf(fp, sizeof(fp), "%s\\%s", dir, dd.cFileName);
		if (dd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!SkeetoClearDirFiles(fp) || !RemoveDirectoryA(fp)) ok = false;
		} else if (!DeleteFileA(fp)) {
			ok = false;
		}
	} while (FindNextFileA(hd, &dd));
	FindClose(hd);
	return ok;
}

static bool SkeetoProbeWorkshopZip(const char* zipPath, const char* stageTmp, char* outVer, size_t outVerN) {
	if (!zipPath || !stageTmp || !outVer || outVerN < 8) return false;
	outVer[0] = 0;
	if (!SkeetoWorkshopFileLooksZip(zipPath)) return false;
	if (!SkeetoClearDirFiles(stageTmp)) return false;
	if (!SkeetoTarExtract(zipPath, stageTmp, "SKEETO_WORKSHOP.txt VERSION.txt"))
		return false;
	if (!SkeetoIsWorkshopPackageDir(stageTmp))
		return false;
	char verPath[MAX_PATH]{};
	snprintf(verPath, sizeof(verPath), "%s\\VERSION.txt", stageTmp);
	return SkeetoReadSemverFile(verPath, outVer, outVerN);
}

// Same Local\SkeetoMaintenance.v1.<SHA256(UTF-16LE upper root)> name as Maintain.ps1.
// Probe without keeping ownership so a waiting Apply helper remains the sole writer.
static bool SkeetoMaintenanceBusy(const char* gameRoot) {
	if (!gameRoot || !gameRoot[0]) return false;
	wchar_t rootW[MAX_PATH * 2]{};
	if (!MultiByteToWideChar(CP_ACP, 0, gameRoot, -1, rootW, _countof(rootW))) return false;
	CharUpperBuffW(rootW, static_cast<DWORD>(wcslen(rootW)));
	BCRYPT_ALG_HANDLE alg = nullptr;
	BCRYPT_HASH_HANDLE hash = nullptr;
	BYTE digest[32]{};
	bool ok = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0
		&& BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) >= 0
		&& BCryptHashData(hash, reinterpret_cast<PUCHAR>(rootW),
			static_cast<ULONG>(wcslen(rootW) * sizeof(wchar_t)), 0) >= 0
		&& BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
	if (hash) BCryptDestroyHash(hash);
	if (alg) BCryptCloseAlgorithmProvider(alg, 0);
	if (!ok) return false;
	wchar_t name[128]{};
	wcscpy_s(name, L"Local\\SkeetoMaintenance.v1.");
	wchar_t* hex = name + wcslen(name);
	for (int i = 0; i < 32; ++i) swprintf_s(hex + i * 2, 3, L"%02X", digest[i]);
	HANDLE mutex = CreateMutexW(nullptr, FALSE, name);
	if (!mutex) return false;
	const DWORD waited = WaitForSingleObject(mutex, 0);
	if (waited == WAIT_OBJECT_0 || waited == WAIT_ABANDONED) {
		ReleaseMutex(mutex);
		CloseHandle(mutex);
		return false;
	}
	CloseHandle(mutex);
	return true;
}

// One detached new-package engine waits for this exact game process instance.
// The DLL never copies a local engine first or calls an older installed engine.
static bool SkeetoArmExitUpdater(const char* gameRoot) {
    if (g_wsApplyProcess) return true;
    char stage[MAX_PATH]{}; snprintf(stage, sizeof(stage), "%s\\skeeto_update", gameRoot);
    return SkeetoInvokeUpdateEngine(stage, gameRoot, true, &g_wsApplyProcess) == 0;
}

static bool SkeetoPrepareStagedUpdate(const char* gameRoot, bool quiet) {
    char stage[MAX_PATH]{}, ready[MAX_PATH]{}, versionFile[MAX_PATH]{}, version[32]{}, active[MAX_PATH]{};
    snprintf(stage, sizeof(stage), "%s\\skeeto_update", gameRoot);
    snprintf(ready, sizeof(ready), "%s\\READY", stage);
    snprintf(active, sizeof(active), "%s\\skeeto_migrate_backup\\active.json", gameRoot);
    const bool recoveryPending = GetFileAttributesA(active) != INVALID_FILE_ATTRIBUTES;
    if (GetFileAttributesA(ready) == INVALID_FILE_ATTRIBUTES && !recoveryPending) return false;
    // An Apply helper already owns the stage (possibly from a prior process).
    // Probe before any READY deletion or Prepare so a waiting writer cannot lose its receipt.
    if (g_wsApplyProcess) return true;
    if (SkeetoMaintenanceBusy(gameRoot)) {
        Log("workshop: maintenance helper already active; leave pending stage untouched");
        return true;
    }
    snprintf(versionFile, sizeof(versionFile), "%s\\VERSION.txt", stage);
    if (!SkeetoReadSemverFile(versionFile, version, sizeof(version))) return false;
    // READY denotes unapplied work even if recovery restored the same semver
    // and cleared active.json. The downloaded engine revalidates the receipt.
    if (!recoveryPending && SkeetoCmpSemver(SKEETO_VERSION, version) > 0) { DeleteFileA(ready); return false; }
    const DWORD code = SkeetoInvokeUpdateEngine(stage, gameRoot, false);
    if (code == 2) { SkeetoRequireManualUpdateOnce(); return false; }
    if (code != 0 || GetFileAttributesA(ready) == INVALID_FILE_ATTRIBUTES) return false;
    if (!quiet) Log("workshop: new-package engine prepared update %s", version);
    return SkeetoArmExitUpdater(gameRoot);
}

static void SkeetoConsiderWorkshopCandidate(const char* zipPath, const char* fileName,
	const char* stageTmp, char* bestZip, size_t bestZipN, char* bestVer, size_t bestVerN,
	char* bestName, size_t bestNameN) {
	char ver[32]{};
	if (!SkeetoProbeWorkshopZip(zipPath, stageTmp, ver, sizeof(ver)))
		return;
	if (!bestVer[0] || SkeetoCmpSemver(bestVer, ver) < 0) {
		strncpy(bestVer, ver, bestVerN - 1);
		bestVer[bestVerN - 1] = 0;
		strncpy(bestZip, zipPath, bestZipN - 1);
		bestZip[bestZipN - 1] = 0;
		strncpy(bestName, fileName ? fileName : "", bestNameN - 1);
		bestName[bestNameN - 1] = 0;
	}
}

static bool SkeetoVerifyWorkshopInstallable(const char* zipPath, const char* stageTmp,
    const char* gameRoot, char* outVer, size_t outVerN) {
    outVer[0] = 0;
    if (!SkeetoClearDirFiles(stageTmp) || !SkeetoTarExtract(zipPath, stageTmp, nullptr)) return false;
    const DWORD code = SkeetoInvokeUpdateEngine(stageTmp, gameRoot, false);
    if (code == 2) { SkeetoRequireManualUpdateOnce(); return false; }
    if (code != 0) return false;
    char version[MAX_PATH]{}; snprintf(version, sizeof(version), "%s\\VERSION.txt", stageTmp);
    return SkeetoReadSemverFile(version, outVer, outVerN);
}

struct SkeetoWsZipWatch {
	DWORD lastSize = 0;
	DWORD lastWriteLow = 0;
	DWORD lastWriteHigh = 0;
	DWORD retryValidationAt = 0;
};

// File size stability is not a completion signal. Probe immediately; the full
// ZIP extraction + marker/version/core-DLL validation is the only readiness gate.
static bool SkeetoWorkshopZipReady(const char* zipPath, SkeetoWsZipWatch* watch) {
	if (!zipPath || !watch) return false;
	WIN32_FILE_ATTRIBUTE_DATA fad{};
	if (!GetFileAttributesExA(zipPath, GetFileExInfoStandard, &fad))
		return false;
	const DWORD sz = fad.nFileSizeLow;
	const DWORD writeLow = fad.ftLastWriteTime.dwLowDateTime;
	const DWORD writeHigh = fad.ftLastWriteTime.dwHighDateTime;
	// Archive extraction and the envelope decide readiness. A future smaller
	// core/package must not be blocked by today's arbitrary compressed size.
	if (sz < 4u)
		return false;
	if (sz != watch->lastSize || writeLow != watch->lastWriteLow || writeHigh != watch->lastWriteHigh) {
		watch->lastSize = sz;
		watch->lastWriteLow = writeLow;
		watch->lastWriteHigh = writeHigh;
		watch->retryValidationAt = 0;
		Log("workshop: candidate changed %s (%lu bytes); validating archive", zipPath, (unsigned long)sz);
	}
	return SkeetoWorkshopFileLooksZip(zipPath);
}

enum SkeetoWsAttempt {
	kWsRetry = 0,
	kWsDone = 1,
	kWsDisabled = 2,
	kWsDeclined = 3,
	kWsManual = 5,
	kWsWatching = 4, // installed version is current; wait for VPK metadata to change
};

static bool g_wsSessionSettled = false;

static bool SkeetoWorkshopNotifyRetryableError(const wchar_t* body, const char* logText) {
	static bool notified = false;
	Log("workshop: retryable error: %s", logText ? logText : "unknown");
	// A broken/half-written Steam file can be observed repeatedly. Keep retrying,
	// but never trap the user in a MessageBox every two seconds.
	if (!notified) {
		notified = true;
		return MessageBoxW(nullptr, body, L"Skeeto Update",
			MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST) != 0;
	}
	return false;
}

// One session-scoped explain when Apply failed and READY is still pending.
// Requires apply_failed.txt — quiet deferrals exit 0 without that receipt.
// Never attach to the success Arm notify thread.
static void SkeetoWorkshopNotifyApplyFailedOnce(const char* root) {
	if (!root || !root[0] || !g_optWorkshopUpdate) return;
	char readyPath[MAX_PATH]{}, failPath[MAX_PATH]{}, receipt[480]{};
	snprintf(readyPath, sizeof(readyPath), "%s\\skeeto_update\\READY", root);
	snprintf(failPath, sizeof(failPath), "%s\\%s", root, kSkeetoWsApplyFailedFile);
	if (GetFileAttributesA(readyPath) == INVALID_FILE_ATTRIBUTES) return;
	if (!SkeetoReadSmallTextFile(failPath, receipt, sizeof(receipt))) return;
	const char* detail = receipt;
	if (const char* bar = strchr(receipt, '|')) detail = bar + 1;
	while (*detail == ' ') ++detail;
	wchar_t reason[400]{};
	if (!MultiByteToWideChar(CP_UTF8, 0, detail, -1, reason, _countof(reason))
		&& !MultiByteToWideChar(CP_ACP, 0, detail, -1, reason, _countof(reason))) {
		wcsncpy_s(reason, L"(see skeeto_update\\apply_failed.txt)", _TRUNCATE);
	}
	if (wcslen(reason) > 280) reason[280] = 0;
	wchar_t body[900]{};
	swprintf_s(body,
		UiText(UiId::BootstrapApplyFailed,
			L"退出后自动安装未完成。\n原因：%s\n"
			L"暂存已保留，完全退出后会再试；不会强杀游戏。\n"
			L"也可关闭游戏后运行包内「Setup_安装 维护 卸载.bat」，或按工坊/桌面包说明手工覆盖。"),
		reason);
	char logText[520]{};
	snprintf(logText, sizeof(logText), "apply failed; READY retained; %s", receipt);
	const bool shown = SkeetoWorkshopNotifyRetryableError(body, logText);
	// Another error may already have used the session's one dialog. Keep an
	// unreported failure for the next session rather than silently consuming it.
	if (shown) DeleteFileA(failPath);
}

static DWORD WINAPI SkeetoWorkshopNotifyThread(LPVOID) {
	MessageBoxW(nullptr,
	UiText(UiId::BootstrapUpdateReady, L"更新已校验并暂存。\n请完全退出游戏，退出后将自动完成安装；失败会保留暂存并在下次退出后重试。"),
	L"Skeeto Update", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
	return 0;
}

static DWORD WINAPI SkeetoMigrationNoticeThread(LPVOID) {
	MessageBoxW(nullptr, UiText(UiId::BootstrapMigrationNotice,
		L"Skeeto 尚未完成 Steam 入口迁移。旧入口暂时仍可使用。\n请退出游戏，将完整包解压到游戏根目录。\n迁移后从 Steam 启动；旧 ini 中的 cmdline 请自行转到 Steam 启动选项。\n设置与自定义资源会保留。"),
		L"Skeeto", MB_OK | MB_ICONINFORMATION);
	return 0;
}

void SkeetoBootstrapMigrationNoticeOnce() {
	static bool done = false;
	if (done || !g_optWorkshopUpdate) return;
	done = true;
	char root[MAX_PATH]{}; SkeetoGameRoot(root, sizeof(root));
	if (!root[0]) return;
	char proxy[MAX_PATH]{}; snprintf(proxy, sizeof(proxy), "%s\\bin\\wininet.dll", root);
	// Extracted Steam users need no migration popup. Maintenance verifies the
	// packaged manifest for ownership; legacy core-only users are warned once.
	if (GetFileAttributesA(proxy) != INVALID_FILE_ATTRIBUTES) return;
	HANDLE thread = CreateThread(nullptr, 0, SkeetoMigrationNoticeThread, nullptr, 0, nullptr);
	if (thread) CloseHandle(thread);
}

static SkeetoWsAttempt SkeetoWorkshopUpdateAttempt(SkeetoWsZipWatch* zipWatch) {
	g_wsManualRequired = false; // Each attempt evaluates current files afresh.
	if (!g_wsUpdateEnabled.load(std::memory_order_relaxed))
		return kWsDisabled;
	if (!g_workshopId[0]) {
		strncpy(g_workshopId, kSkeetoWorkshopIdDefault, sizeof(g_workshopId) - 1);
		g_workshopId[sizeof(g_workshopId) - 1] = 0;
	}

	char root[MAX_PATH]{};
	SkeetoGameRoot(root, sizeof(root));
	if (!root[0]) return kWsRetry;
	char activeTransaction[MAX_PATH]{};
	snprintf(activeTransaction, sizeof(activeTransaction), "%s\\skeeto_migrate_backup\\active.json", root);
	bool recoveryPending = GetFileAttributesA(activeTransaction) != INVALID_FILE_ATTRIBUTES;

	if (SkeetoPrepareStagedUpdate(root, true)) {
		char stagedVerPath[MAX_PATH]{}, stagedVer[32]{};
		snprintf(stagedVerPath, sizeof(stagedVerPath), "%s\\skeeto_update\\VERSION.txt", root);
		if (SkeetoReadSemverFile(stagedVerPath, stagedVer, sizeof(stagedVer))
			&& !SkeetoWorkshopVersionMarkerMatches(kSkeetoWsChangelogShownFile, stagedVer)) {
			SkeetoWorkshopMarkChangelogPending(stagedVer);
			SkeetoWorkshopTryOpenChangelog();
		}
		// Pending READY path: no informational MessageBox (removed, not a
		// semver/manifestHash gate). First-consent path still notifies once after Arm.
		// Only when staging carries apply_failed.txt do we explain once.
		SkeetoWorkshopNotifyApplyFailedOnce(root);
		Log("workshop: pending update; exit updater armed or maintenance already active");
		return kWsDone;
	}
	char existingReady[MAX_PATH]{};
	snprintf(existingReady, sizeof(existingReady), "%s\\skeeto_update\\READY", root);
	const bool hadPendingStage = GetFileAttributesA(existingReady) != INVALID_FILE_ATTRIBUTES;
	// Apply/Prepare may hold the maintenance mutex. Never clear/re-extract the
	// pending stage from a new game process while that writer is in flight.
	if ((hadPendingStage || recoveryPending) && (g_wsApplyProcess || SkeetoMaintenanceBusy(root))) {
		Log("workshop: pending stage owned by active helper; skip re-extract");
		return kWsDone;
	}
	recoveryPending = recoveryPending || hadPendingStage;
	char pendingVersionPath[MAX_PATH]{}, pendingVersion[32]{};
	snprintf(pendingVersionPath, sizeof(pendingVersionPath), "%s\\skeeto_update\\VERSION.txt", root);
	if (hadPendingStage) SkeetoReadSemverFile(pendingVersionPath, pendingVersion, sizeof(pendingVersion));
	// Read only the candidate envelope before consent. Full extraction and
	// preparation happen once, in the final stage, after the user accepts.
	g_wsManualRequired = false;

	char workshopDir[MAX_PATH]{};
	char updateDir[MAX_PATH]{};
	char stageTmp[MAX_PATH]{};
	snprintf(workshopDir, sizeof(workshopDir), "%s\\left4dead2\\addons\\workshop", root);
	snprintf(updateDir, sizeof(updateDir), "%s\\skeeto_update", root);
	snprintf(stageTmp, sizeof(stageTmp), "%s\\skeeto_update\\_scan", root);
	CreateDirectoryA(updateDir, nullptr);

	char fileName[64]{};
	char zipPath[MAX_PATH]{};
	snprintf(fileName, sizeof(fileName), "%s.vpk", g_workshopId);
	snprintf(zipPath, sizeof(zipPath), "%s\\%s", workshopDir, fileName);
	if (GetFileAttributesA(zipPath) == INVALID_FILE_ATTRIBUTES) {
		if (zipWatch) {
			zipWatch->lastSize = 0;
			zipWatch->lastWriteLow = 0;
			zipWatch->lastWriteHigh = 0;
			zipWatch->retryValidationAt = 0;
		}
		Log("workshop: %s not found yet (subscribe https://steamcommunity.com/sharedfiles/filedetails/?id=%s)",
			fileName, g_workshopId);
		return kWsRetry;
	}
	if (!zipWatch || !SkeetoWorkshopZipReady(zipPath, zipWatch))
		return kWsRetry;
	const DWORD validateNow = GetTickCount();
	if (zipWatch->retryValidationAt && (LONG)(validateNow - zipWatch->retryValidationAt) < 0)
		return kWsRetry;

	const DWORD stableSz = zipWatch->lastSize;
	char bestZip[MAX_PATH]{};
	char bestVer[32]{};
	char bestName[MAX_PATH]{};
	static char s_probeZip[MAX_PATH]{};
	static DWORD s_probeZipSize = 0;
	static DWORD s_probeWriteLow = 0;
	static DWORD s_probeWriteHigh = 0;
	static char s_probeVer[32]{};
	if (!strcmp(zipPath, s_probeZip) && stableSz == s_probeZipSize
		&& zipWatch->lastWriteLow == s_probeWriteLow && zipWatch->lastWriteHigh == s_probeWriteHigh
		&& s_probeVer[0]) {
		strncpy(bestZip, zipPath, sizeof(bestZip) - 1);
		bestZip[sizeof(bestZip) - 1] = 0;
		strncpy(bestVer, s_probeVer, sizeof(bestVer) - 1);
		bestVer[sizeof(bestVer) - 1] = 0;
	} else {
		SkeetoConsiderWorkshopCandidate(zipPath, fileName, stageTmp,
			bestZip, sizeof(bestZip), bestVer, sizeof(bestVer), bestName, sizeof(bestName));
		if (bestZip[0] && bestVer[0]) {
			strncpy(s_probeZip, zipPath, sizeof(s_probeZip) - 1);
			s_probeZip[sizeof(s_probeZip) - 1] = 0;
			s_probeZipSize = stableSz;
			s_probeWriteLow = zipWatch->lastWriteLow;
			s_probeWriteHigh = zipWatch->lastWriteHigh;
			strncpy(s_probeVer, bestVer, sizeof(s_probeVer) - 1);
			s_probeVer[sizeof(s_probeVer) - 1] = 0;
		}
	}
	if (!bestZip[0] || !bestVer[0]) {
		Log("workshop: %s not a valid Skeeto package yet", fileName);
		// Retry unchanged incomplete packages at most once a minute. Size/write
		// changes reset this deadline immediately when Steam resumes downloading.
		zipWatch->retryValidationAt = GetTickCount() + 60000u;
		return kWsRetry;
	}


	const int compared = SkeetoCmpSemver(SKEETO_VERSION, bestVer);
	if (compared > 0 || (compared == 0 && !recoveryPending)) {
		static char s_loggedCurrent[32]{};
		if (_stricmp(s_loggedCurrent, bestVer)) {
			strncpy(s_loggedCurrent, bestVer, sizeof(s_loggedCurrent) - 1);
			s_loggedCurrent[sizeof(s_loggedCurrent) - 1] = 0;
			Log("workshop: installed %s >= current workshop %s; keep watching for Steam download",
				SKEETO_VERSION, bestVer);
		}
		return kWsWatching;
	}

	static bool s_userDeclined = false;
	if (s_userDeclined)
		return kWsDone;

	static char s_acceptedZip[MAX_PATH]{};
	static DWORD s_acceptedSize = 0, s_acceptedWriteLow = 0, s_acceptedWriteHigh = 0;
	const bool alreadyAccepted = (hadPendingStage && pendingVersion[0] && !_stricmp(bestVer, pendingVersion)) || (!strcmp(bestZip, s_acceptedZip)
		&& stableSz == s_acceptedSize && zipWatch->lastWriteLow == s_acceptedWriteLow
		&& zipWatch->lastWriteHigh == s_acceptedWriteHigh);
	if (!alreadyAccepted) {
		Log("workshop: update available installed=%s workshop=%s zip=%s", SKEETO_VERSION, bestVer, bestZip);
		wchar_t body[512]{};
		swprintf_s(body, UiText(UiId::BootstrapConfirmUpdate, L"你已订阅 Skeeto 工坊更新（%hs -> %hs），是否安装新版本？\n\n"
			L"完整校验与安装由新包自带的更新器负责。\n需要手动迁移时会明确提示，当前安装保持不变。"), SKEETO_VERSION, bestVer);
		const int ans = MessageBoxW(nullptr, body, L"Skeeto Update",
			MB_YESNO | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
		if (ans != IDYES) {
			Log("workshop: user declined update");
			s_userDeclined = true;
			return kWsDeclined;
		}
		strncpy(s_acceptedZip, bestZip, sizeof(s_acceptedZip) - 1);
		s_acceptedZip[sizeof(s_acceptedZip) - 1] = 0;
		s_acceptedSize = stableSz;
		s_acceptedWriteLow = zipWatch->lastWriteLow;
		s_acceptedWriteHigh = zipWatch->lastWriteHigh;
	}

	const DWORD preparationStarted = GetTickCount();
	char stage[MAX_PATH]{};
	snprintf(stage, sizeof(stage), "%s\\skeeto_update", root);
	char stagedVer[32]{};
	if (!SkeetoVerifyWorkshopInstallable(bestZip, stage, root, stagedVer, sizeof(stagedVer))
		|| _stricmp(stagedVer, bestVer) != 0
		|| SkeetoCmpSemver(SKEETO_VERSION, stagedVer) > 0
		|| (SkeetoCmpSemver(SKEETO_VERSION, stagedVer) == 0 && !recoveryPending)) {
		if (g_wsManualRequired) return kWsManual;
		SkeetoWorkshopNotifyRetryableError(
			UiText(UiId::Text_624c3e99, L"工坊包尚未完整下载或校验未通过，Skeeto 会继续重试。\nThe Workshop package is incomplete or invalid; Skeeto will retry."),
			"final stage validation failed");
		return kWsRetry;
	}
	// READY is issued only by the downloaded engine after its full validation.
	// Give the accepted update visible feedback as soon as it is prepared.
	// The completion notice still waits for the exit helper's acknowledgement.
	SkeetoWorkshopMarkChangelogPending(bestVer);
	SkeetoWorkshopTryOpenChangelog();

	// The new engine just prepared this same stage above. Do not repeat its
	// full payload/ownership validation; Apply revalidates after game exit.
	if (!SkeetoArmExitUpdater(root)) {
		SkeetoWorkshopNotifyRetryableError(
			UiText(UiId::Text_2da3309e, L"更新暂存不完整：启动前更新脚本或核心文件不可用。\n"
			L"Update staging is incomplete; Skeeto will retry."),
			"prepare staged update failed");
		return kWsRetry;
	}
	Log("workshop: update prepared and helper armed elapsed_ms=%lu", GetTickCount() - preparationStarted);
	HANDLE notifyTh = CreateThread(nullptr, 0, SkeetoWorkshopNotifyThread, nullptr, 0, nullptr);
	if (notifyTh) CloseHandle(notifyTh);
	return kWsDone;
}

static void SkeetoWorkshopUpdateWorkerPump() {
	if (g_wsSessionSettled || !g_run.load(std::memory_order_relaxed) || !g_wsUpdateEnabled.load(std::memory_order_relaxed))
		return;
	const DWORD now = GetTickCount();
	static DWORD s_nextAt = 0;
	static DWORD s_retryAt = 0;
	static DWORD s_readyRetryAt = 0;
	static SkeetoWsZipWatch s_zipWatch{};
	static bool s_observed = false;
	static bool s_hadZip = false;
	static bool s_changePending = false;
	static DWORD s_seenSize = 0, s_seenWriteLow = 0, s_seenWriteHigh = 0;
	static SkeetoWsAttempt s_lastResult = kWsRetry;
	if (s_nextAt && now - s_nextAt < 1000u)
		return;
	s_nextAt = now;
	// Acknowledged helper owns the stage while waiting. Observe its lifetime,
	// without rehashing or launching another helper. A dead helper is retryable.
	if (g_wsApplyProcess) {
		if (WaitForSingleObject(g_wsApplyProcess, 0) == WAIT_TIMEOUT) return;
		DWORD exitCode = 1;
		GetExitCodeProcess(g_wsApplyProcess, &exitCode);
		CloseHandle(g_wsApplyProcess); g_wsApplyProcess = nullptr;
		s_retryAt = 0; s_readyRetryAt = 0;
		char gameRoot[MAX_PATH]{}, readyPath[MAX_PATH]{}, resultPath[MAX_PATH]{}, outcome[480]{};
		SkeetoGameRoot(gameRoot, sizeof(gameRoot));
		if (gameRoot[0]) {
			snprintf(readyPath, sizeof(readyPath), "%s\\skeeto_update\\READY", gameRoot);
			snprintf(resultPath, sizeof(resultPath), "%s\\%s", gameRoot, kSkeetoWsApplyResultFile);
			const bool haveReady = GetFileAttributesA(readyPath) != INVALID_FILE_ATTRIBUTES;
			const bool haveOutcome = SkeetoReadSmallTextFile(resultPath, outcome, sizeof(outcome));
			Log("workshop: exit updater ended code=%lu ready=%d outcome=%s",
				exitCode, haveReady ? 1 : 0, haveOutcome ? outcome : "-");
			// exit 0 + READY retained = defer/no-op, never BootstrapApplyFailed.
			// Hard failures leave apply_failed.txt; only that receipt pops UI.
			if (exitCode != 0 && exitCode != 2 && haveReady)
				SkeetoWorkshopNotifyApplyFailedOnce(gameRoot);
		} else {
			Log("workshop: exit updater ended code=%lu; recheck pending update", exitCode);
		}
	}

	// Observe once per second, but keep the common
	// path one metadata read. Full archive/stage/doc validation runs only on first
	// observation, a real VPK change, a pending READY, or a slow retry after an
	// actual transient failure. An unchanged current package does no extraction.
	if (!g_workshopId[0]) {
		strncpy(g_workshopId, kSkeetoWorkshopIdDefault, sizeof(g_workshopId) - 1);
		g_workshopId[sizeof(g_workshopId) - 1] = 0;
	}
	char root[MAX_PATH]{}, zipPath[MAX_PATH]{}, readyPath[MAX_PATH]{};
	SkeetoGameRoot(root, sizeof(root));
	if (root[0]) {
		snprintf(zipPath, sizeof(zipPath), "%s\\left4dead2\\addons\\workshop\\%s.vpk", root, g_workshopId);
		snprintf(readyPath, sizeof(readyPath), "%s\\skeeto_update\\READY", root);
	}
	WIN32_FILE_ATTRIBUTE_DATA fad{};
	const bool haveZip = zipPath[0] && GetFileAttributesExA(zipPath, GetFileExInfoStandard, &fad);
	const DWORD size = haveZip ? fad.nFileSizeLow : 0;
	const DWORD writeLow = haveZip ? fad.ftLastWriteTime.dwLowDateTime : 0;
	const DWORD writeHigh = haveZip ? fad.ftLastWriteTime.dwHighDateTime : 0;
	const bool changed = !s_observed || haveZip != s_hadZip || size != s_seenSize
		|| writeLow != s_seenWriteLow || writeHigh != s_seenWriteHigh;
    char activePath[MAX_PATH]{};
    if (root[0]) snprintf(activePath, sizeof(activePath), "%s\\skeeto_migrate_backup\\active.json", root);
    const bool pendingReady = (readyPath[0] && GetFileAttributesA(readyPath) != INVALID_FILE_ATTRIBUTES)
        || (activePath[0] && GetFileAttributesA(activePath) != INVALID_FILE_ATTRIBUTES);
	const bool pendingReadyDue = pendingReady && (!s_readyRetryAt || (LONG)(now - s_readyRetryAt) >= 0);
	const bool retryDue = (s_lastResult == kWsRetry || s_lastResult == kWsManual) && (!s_retryAt || (LONG)(now - s_retryAt) >= 0);
	const bool firstObservation = !s_observed;
	s_observed = true;
	s_hadZip = haveZip;
	s_seenSize = size;
	s_seenWriteLow = writeLow;
	s_seenWriteHigh = writeHigh;
	if (changed && !firstObservation && !pendingReadyDue) {
		// A changing Steam file is definitely not ready. Wait for one unchanged
		// metadata observation, then use full archive validation as the real gate.
		s_changePending = true;
		return;
	}
	const bool changeDue = s_changePending && !changed;
	if (!firstObservation && !pendingReadyDue && !retryDue && !changeDue)
		return;

	const SkeetoWsAttempt r = SkeetoWorkshopUpdateAttempt(&s_zipWatch);
	// Preparation/acknowledgement may wait longer than the retry interval.
	// Start the cooldown after that work finishes, not before it began.
	const DWORD finishedAt = GetTickCount();
	s_changePending = false;
	s_lastResult = r;
	s_retryAt = (r == kWsRetry) ? finishedAt + 30000u : (r == kWsManual ? finishedAt + 600000u : 0);
    const bool readyStillPresent = (readyPath[0] && GetFileAttributesA(readyPath) != INVALID_FILE_ATTRIBUTES)
        || (activePath[0] && GetFileAttributesA(activePath) != INVALID_FILE_ATTRIBUTES);
	s_readyRetryAt = (readyStillPresent && r != kWsDone) ? finishedAt + (r == kWsManual ? 600000u : 30000u) : 0;
	if (r == kWsDisabled || r == kWsDeclined)
		g_wsSessionSettled = true;
}

// Archive extraction, PowerShell waits and user consent must not occupy the
// shared host worker (UI jobs/persistence). At most one optional low-priority
// update thread; no Source API, Paint/Event work or per-map resources.
static std::atomic_bool g_wsUpdateStop{false};
static HANDLE g_wsUpdateThread = nullptr; // Host worker owns this handle.
static DWORD WINAPI SkeetoWorkshopUpdateThread(LPVOID) {
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
	while (g_run.load(std::memory_order_relaxed) && !g_wsUpdateStop.load(std::memory_order_relaxed)) {
		if (g_wsUpdateEnabled.load(std::memory_order_relaxed)) SkeetoWorkshopUpdateWorkerPump();
		Sleep(1000);
	}
	if (g_wsApplyProcess) { CloseHandle(g_wsApplyProcess); g_wsApplyProcess = nullptr; }
	SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
	return 0;
}
void SkeetoWorkshopUpdatePump() {
	g_wsUpdateEnabled.store(g_optWorkshopUpdate, std::memory_order_relaxed);
	if (g_wsUpdateThread || !g_optWorkshopUpdate || g_wsUpdateStop.load(std::memory_order_relaxed)) return;
	static DWORD retryAt = 0;
	const DWORD now = GetTickCount();
	if (retryAt && (LONG)(now - retryAt) < 0) return;
	g_wsUpdateThread = CreateThread(nullptr, 0, SkeetoWorkshopUpdateThread, nullptr, 0, nullptr);
	if (!g_wsUpdateThread) { retryAt = now + 30000u; Log("workshop: could not start update worker error=%lu", GetLastError()); }
}
void SkeetoWorkshopUpdateShutdown() {
	g_wsUpdateStop.store(true, std::memory_order_relaxed);
	if (g_wsUpdateThread) { CloseHandle(g_wsUpdateThread); g_wsUpdateThread = nullptr; }
}

