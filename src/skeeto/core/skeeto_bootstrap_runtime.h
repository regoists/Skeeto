#pragma once
#include "skeeto_bootstrap_abi.h"
// Per-module storage; the named mutex is an existence gate, never a lock to wait
// on. A successful owner retains the handle for the lifetime of the process.
static SktBootstrapIdentity sktBootstrapIdentity{sizeof(SktBootstrapIdentity), SKT_BOOT_MAGIC, SKT_BOOT_ABI, 0, nullptr, SKT_BOOT_DUPLICATE};
static HANDLE sktBootstrapGate = nullptr;
static inline bool SktBootstrapClaim(HMODULE module) {
    sktBootstrapIdentity.module = module;
    sktBootstrapIdentity.processId = GetCurrentProcessId();
    wchar_t name[64] = L"Local\\SkeetoRuntime.v1.";
    unsigned at = 0; while (name[at]) ++at;
    DWORD pid = sktBootstrapIdentity.processId;
    for (int shift = 28; shift >= 0; shift -= 4) name[at++] = L"0123456789abcdef"[(pid >> shift) & 15];
    name[at] = 0;
    SetLastError(ERROR_SUCCESS);
    HANDLE gate = CreateMutexW(nullptr, FALSE, name);
    DWORD error = GetLastError();
    if (!gate || error == ERROR_ALREADY_EXISTS) {
        if (gate) CloseHandle(gate);
        sktBootstrapIdentity.state = gate ? SKT_BOOT_DUPLICATE : SKT_BOOT_FAILED;
        return false;
    }
    sktBootstrapGate = gate;
    InterlockedExchange(&sktBootstrapIdentity.state, SKT_BOOT_LOADING);
    return true;
}
static inline void SktBootstrapMark(LONG state) { InterlockedExchange(&sktBootstrapIdentity.state, state); }
static inline bool SktBootstrapOwnsRuntime() { return sktBootstrapGate != nullptr; }
static inline bool SktBootstrapPinOwner() {
    HMODULE pinned = nullptr;
    return GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                             reinterpret_cast<LPCWSTR>(sktBootstrapIdentity.module), &pinned) != FALSE;
}
static inline void SktBootstrapAbandon() {
    SktBootstrapMark(SKT_BOOT_FAILED);
    if (sktBootstrapGate) { CloseHandle(sktBootstrapGate); sktBootstrapGate = nullptr; }
}
