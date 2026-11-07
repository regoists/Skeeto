#include "skeeto_bootstrap_common.h"
struct SktProxyExport { const char* name; WORD ordinal; };
#include "wininet_exports.inc"
extern "C" {
    FARPROC SktProxyTargets[SKT_PROXY_EXPORT_COUNT]{};
    // Static PE ownership marker: maintenance can identify our proxy without
    // loading it, installing a manifest, or confusing it with another platform.
    __declspec(dllexport) unsigned __cdecl SkeetoWininetProxyIdentity() { return SKT_BOOT_MAGIC; }
}
static HMODULE sktProxySelf = nullptr;
static INIT_ONCE sktProxyOnce = INIT_ONCE_STATIC_INIT;
static DWORD sktProxyBindError = ERROR_SUCCESS;
static unsigned sktProxyMissingExports = 0;
static BOOL CALLBACK SktBindWininet(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t directory[SKT_PATH_CAP]{}, path[SKT_PATH_CAP]{};
    UINT size = GetSystemDirectoryW(directory, SKT_PATH_CAP);
    if (!size || size >= SKT_PATH_CAP || !SktJoinPath(directory, L"wininet.dll", path)) {
        sktProxyBindError = ERROR_BAD_PATHNAME; return TRUE;
    }
    HMODULE system = LoadLibraryW(path);
    if (!system || system == sktProxySelf) {
        sktProxyBindError = system ? ERROR_INVALID_DLL : GetLastError(); return TRUE;
    }
    for (unsigned i = 0; i < SKT_PROXY_EXPORT_COUNT; ++i) {
        const auto& item = sktProxyExports[i];
        FARPROC target = GetProcAddress(system, item.name ? item.name : MAKEINTRESOURCEA(item.ordinal));
        // Older Windows may omit newer APIs. Their absence does not prevent
        // the core or available APIs from loading; the thunk fails only if used.
        if (!target) ++sktProxyMissingExports;
        InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&SktProxyTargets[i]), reinterpret_cast<PVOID>(target));
    }
    return TRUE;
}
extern "C" void __cdecl SktProxyResolve(unsigned index) {
    DWORD lastError = GetLastError();
    InitOnceExecuteOnce(&sktProxyOnce, SktBindWininet, nullptr, nullptr);
    if (index >= SKT_PROXY_EXPORT_COUNT || !SktProxyTargets[index]) {
        // No null jump or invented generic return value for unknown signatures.
        ULONG_PTR error = sktProxyBindError ? sktProxyBindError : ERROR_PROC_NOT_FOUND;
        RaiseException(0xE0534B54, EXCEPTION_NONCONTINUABLE, 1, &error);
        TerminateProcess(GetCurrentProcess(), ERROR_PROC_NOT_FOUND);
    }
    SetLastError(lastError);
}
#if !defined(SKEETO_BOOTSTRAP_TEST_NO_WORKER)
static DWORD WINAPI SktProxyStart(void*) {
    HMODULE pinned = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                            reinterpret_cast<LPCWSTR>(sktProxySelf), &pinned)) return 0;
    InitOnceExecuteOnce(&sktProxyOnce, SktBindWininet, nullptr, nullptr);
    wchar_t root[SKT_PATH_CAP]{};
    if (!SktModuleRoot(sktProxySelf, true, root)) return 0;
    SktProbe(root, "wininet", "worker-start", sktProxyBindError);
    if (sktProxyBindError) { SktProbe(root, "wininet", "system-bind-incomplete", sktProxyBindError); return 0; }
    if (sktProxyMissingExports) SktProbe(root, "wininet", "optional-exports-missing", sktProxyMissingExports);
    SktLoadCore(root, "wininet");
    return 0;
}
#endif
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        sktProxySelf = module;
        // /MT: retain CRT thread notifications; do not disable them.
#if !defined(SKEETO_BOOTSTRAP_TEST_NO_WORKER)
        HANDLE thread = CreateThread(nullptr, 0, SktProxyStart, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
#endif
    }
    return TRUE;
}
