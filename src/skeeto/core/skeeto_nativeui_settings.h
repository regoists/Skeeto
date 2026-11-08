#pragma once
#include "skeeto_nativeui_config.h"

// Menu input/settings IO can run on the worker; native controls are Paint-only.
// A config copy is small and only taken at the throttled apply boundary.
static SRWLOCK g_nativeUiConfigLock=SRWLOCK_INIT;
static sknativeui::Config g_nativeUiConfig[2]={sknativeui::Defaults(0),sknativeui::Defaults(1)};
static std::atomic_uint g_nativeUiRevision[2]{{1},{1}},g_nativeUiWanted{0};
static std::atomic_int g_nativeUiStatus[2]{}; // 0 off, 1 waiting, 2 active, 3 ABI unavailable, 4 fault/font failure.
static std::atomic_uint g_nativeUiBindings[2]{}; // Published control/capability mask for the user-facing status command.
static sknativeui::Config NativeUiConfig(int target) {
    AcquireSRWLockShared(&g_nativeUiConfigLock);
    const auto c=g_nativeUiConfig[target];
    ReleaseSRWLockShared(&g_nativeUiConfigLock); return c;
}
static void NativeUiStore(int target,const sknativeui::Config& c) {
    AcquireSRWLockExclusive(&g_nativeUiConfigLock);
    g_nativeUiConfig[target]=c;
    g_nativeUiWanted.store((g_nativeUiConfig[0].v[sknativeui::Enabled]?1u:0u)
        |(g_nativeUiConfig[1].v[sknativeui::Enabled]?2u:0u),std::memory_order_release);
    g_nativeUiRevision[target].fetch_add(1,std::memory_order_release);
    ReleaseSRWLockExclusive(&g_nativeUiConfigLock);
}
static void NativeUiSave(FILE* f) {
    sknativeui::Config copies[2];
    AcquireSRWLockShared(&g_nativeUiConfigLock);
    for(int t=0;t<2;++t)copies[t]=g_nativeUiConfig[t];
    ReleaseSRWLockShared(&g_nativeUiConfigLock);
    for(int t=0;t<2;++t)sknativeui::Save(f,copies[t],t);
}
static bool NativeUiLoadLine(const char* line) {
    for(int t=0;t<2;++t){auto c=NativeUiConfig(t);if(sknativeui::ReadLine(c,t,line)){NativeUiStore(t,c);return true;}}
    return false;
}
static void NativeUiLoadDefaults() {
    for(int t=0;t<2;++t)NativeUiStore(t,sknativeui::Defaults(t));
}
static void NativeUiPaintPump(int mode);
