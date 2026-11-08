#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <cwchar>
#include <initializer_list>
#include "skeeto_bootstrap_abi.h"
#include "skeeto_bootstrap_pe.h"
constexpr unsigned SKT_PATH_CAP = 1024;
static inline bool SktJoinPath(const wchar_t* root, const wchar_t* leaf, wchar_t (&out)[SKT_PATH_CAP]) {
    size_t a = wcslen(root), b = wcslen(leaf);
    if (!a || a + b + 2 > SKT_PATH_CAP) return false;
    memcpy(out, root, a * sizeof(wchar_t)); out[a] = L'\\';
    memcpy(out + a + 1, leaf, (b + 1) * sizeof(wchar_t)); return true;
}
static inline bool SktStripParent(wchar_t* path, const wchar_t* expected) {
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash || slash == path || (expected && _wcsicmp(slash + 1, expected))) return false;
    *slash = 0; return true;
}
static inline bool SktModuleRoot(HMODULE module, bool proxy, wchar_t (&root)[SKT_PATH_CAP]) {
    DWORD n = GetModuleFileNameW(module, root, SKT_PATH_CAP);
    if (!n || n >= SKT_PATH_CAP) { root[0] = 0; return false; }
    if (!SktStripParent(root, nullptr)) return false;
    if (proxy) return SktStripParent(root, L"bin");
    // bin\neko\plugins\<dll> is three directories below the game root.
    const wchar_t* leaf = wcsrchr(root, L'\\');
    if (leaf && !_wcsicmp(leaf + 1, L"plugins")) {
        wchar_t candidate[SKT_PATH_CAP]{};
        wcscpy_s(candidate, root);
        if (SktStripParent(candidate, L"plugins") && SktStripParent(candidate, L"neko") && SktStripParent(candidate, L"bin"))
            wcscpy_s(root, candidate);
        // A game root itself named "plugins" is still a valid root install.
    }
    return true;
}
static inline bool SktHasProbeFlag() {
    const wchar_t* command = GetCommandLineW();
    const wchar_t* flag = L"-skeeto_boot_probe";
    const size_t length = wcslen(flag);
    for (const wchar_t* p = command; (p = wcsstr(p, flag)) != nullptr; ++p) {
        const bool before = p == command || p[-1] == L' ' || p[-1] == L'\t' || p[-1] == L'"';
        const wchar_t after = p[length];
        if (before && (!after || after == L' ' || after == L'\t' || after == L'"')) return true;
    }
    return false;
}
// Only opted-in startup diagnostics. No hook, per-frame work or default log IO.
static inline void SktProbe(const wchar_t* root, const char* entry, const char* event, DWORD error = 0) {
    if (!SktHasProbeFlag()) return;
    wchar_t path[SKT_PATH_CAP]{}; if (!SktJoinPath(root, L"skeeto_bootstrap_probe.log", path)) return;
    HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    char line[200]{};
    int n = _snprintf_s(line, sizeof(line), _TRUNCATE, "pid=%lu tick=%llu entry=%s event=%s error=%lu\r\n",
                        GetCurrentProcessId(), GetTickCount64(), entry, event, error);
    if (n > 0) { DWORD written = 0; WriteFile(file, line, static_cast<DWORD>(n), &written, nullptr); }
    CloseHandle(file);
}
static inline const SktBootstrapIdentity* SktIdentity(HMODULE module) {
    if (!module) return nullptr;
    auto query = reinterpret_cast<SktBootstrapQueryFn>(GetProcAddress(module, "SkeetoBootstrapQuery"));
    if (!query) return nullptr;
    const auto id = query();
    return id && id->size == sizeof(*id) && id->magic == SKT_BOOT_MAGIC && id->abi == SKT_BOOT_ABI &&
           id->processId == GetCurrentProcessId() && id->module == module ? id : nullptr;
}
static inline HMODULE SktLoadedNativeNecola() {
    // The real current Necola core exports this decorated marker; Skeeto's
    // compatible alias exports an undecorated marker and our identity instead.
    for (const wchar_t* name : {L"necola.dll", L"necola_orig.dll"}) {
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(0, name, &module)) continue;
        if (module && !GetProcAddress(module, "SkeetoBootstrapQuery") &&
            !GetProcAddress(module, "necola_export") && GetProcAddress(module, "?necola_export@@YAXXZ")) return module;
        FreeLibrary(module); // Release only our unsuccessful lookup reference.
    }
    return nullptr;
}
static inline HMODULE SktFindNativeNecola(const wchar_t* root, bool allowLoad) {
    if (HMODULE loaded = SktLoadedNativeNecola()) return loaded;
    // A saved opt-out skips disk inspection/loading, but keeps launcher-loaded
    // native Necola visible to the existing hook coexistence code.
    if (!allowLoad) return nullptr;
    for (const wchar_t* name : {L"necola.dll", L"necola_orig.dll"}) {
        wchar_t path[SKT_PATH_CAP]{};
        if (!SktJoinPath(root, name, path)) continue;
        SktPeFile image;
        if (!image.nativeNecola(path)) continue;
        // A launcher may have loaded native code while the file was inspected.
        if (HMODULE loaded = SktLoadedNativeNecola()) return loaded;
        HMODULE module = LoadLibraryW(path); // Startup worker only, never DllMain.
        if (module && !GetProcAddress(module, "SkeetoBootstrapQuery") &&
            !GetProcAddress(module, "necola_export") && GetProcAddress(module, "?necola_export@@YAXXZ")) return module;
        // Never try another native DLL after executing one whose startup may run.
        return nullptr;
    }
    return nullptr;
}
static inline void SktLoadCore(const wchar_t* root, const char* entry) {
    for (const wchar_t* name : {L"skeeto.dll", L"necola.dll"}) {
        HMODULE module = nullptr;
        if (!GetModuleHandleExW(0, name, &module)) continue;
        const auto id = SktIdentity(module);
        if (!id) { FreeLibrary(module); continue; }
        LONG state = InterlockedCompareExchange(&id->state, 0, 0);
        if (state == SKT_BOOT_LOADING || state == SKT_BOOT_WORKER || state == SKT_BOOT_READY || state == SKT_BOOT_HIDDEN) {
            // Retain the existing owner without loading a second complete core.
            SktProbe(root, entry, "reuse-core-owner"); return;
        }
        FreeLibrary(module);
    }
    wchar_t path[SKT_PATH_CAP]{};
    if (!SktJoinPath(root, L"skeeto.dll", path)) { SktProbe(root, entry, "bad-core-path"); return; }
    HMODULE core = LoadLibraryW(path);
    if (!core) SktProbe(root, entry, "core-load-failed", GetLastError());
    else SktProbe(root, entry, SktIdentity(core) ? "core-loaded" : "core-identity-missing");
    // Keep a process-lifetime reference: the core owns workers and hooks.
}
