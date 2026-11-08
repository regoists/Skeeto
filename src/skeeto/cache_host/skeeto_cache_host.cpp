#define WIN32_LEAN_AND_MEAN
#include "skeeto_cache_protocol.h"
#include <stdlib.h>

using namespace skeeto_cache_ipc;
struct Asset { HANDLE mapping; const void* view; uint32_t bytes; };
static Asset g_assets[kSlots]{};

static void Drop(Control* control, unsigned index) {
    Asset& asset = g_assets[index];
    if (asset.view) {
        UnmapViewOfFile(asset.view);
        InterlockedExchangeAdd64(&control->mappedBytes, -static_cast<LONG64>(asset.bytes));
        InterlockedDecrement(&control->assets);
    }
    if (asset.mapping) CloseHandle(asset.mapping);
    asset = {};
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 4) return 2;
    HANDLE mapping = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(_wcstoui64(argv[1], nullptr, 16)));
    HANDLE wake = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(_wcstoui64(argv[2], nullptr, 16)));
    HANDLE parent = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(_wcstoui64(argv[3], nullptr, 16)));
    Control* control = static_cast<Control*>(MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(Control)));
    if (!control) return 3;
    if (control->magic != kMagic || control->version != kVersion || control->structureBytes != sizeof(Control)
        || GetProcessId(parent) != control->parentPid) {
        UnmapViewOfFile(control); return 4;
    }
    control->hostPid = GetCurrentProcessId();
    InterlockedExchange(&control->hostReady, 1);
    HANDLE waits[2]{parent, wake};
    while (!InterlockedCompareExchange(&control->stopping, 0, 0)) {
        // Every Offer/Retire and shutdown signals the auto-reset event. A
        // signal during this scan remains pending for the next wait, including
        // cancellation while MapViewOfFile is running. Idle needs no polling.
        DWORD wait = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
        if (wait != WAIT_OBJECT_0 + 1 || InterlockedCompareExchange(&control->stopping, 0, 0)) break;
        // Host-only scan. No engine calls and no work in the game process.
        for (unsigned i = 0; i < kSlots; ++i) {
            Slot& slot = control->slots[i];
            LONG state = InterlockedCompareExchange(&slot.state, 0, 0);
            if (state == Retire) {
                // Retire may arrive before the host consumes Offer.
                if (!g_assets[i].mapping && slot.remoteHandle)
                    CloseHandle(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(slot.remoteHandle)));
                Drop(control, i);
                slot.remoteHandle = 0;
                InterlockedExchange(&slot.state, Empty);
                continue;
            }
            if (state != Offer || InterlockedCompareExchange(&slot.state, Working, Offer) != Offer) continue;
            Asset& asset = g_assets[i];
            asset.mapping = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(slot.remoteHandle));
            asset.bytes = slot.bytes;
            if (asset.bytes && asset.bytes <= kMaxAssetBytes
                && static_cast<uint64_t>(InterlockedCompareExchange64(&control->mappedBytes, 0, 0))
                   + asset.bytes <= kMaxStoredBytes) {
                asset.view = MapViewOfFile(asset.mapping, FILE_MAP_READ, 0, 0, asset.bytes);
            }
            if (asset.view) {
                InterlockedExchangeAdd64(&control->mappedBytes, asset.bytes);
                InterlockedIncrement(&control->assets);
                // Parent can cancel while MapViewOfFile is in progress.
                InterlockedCompareExchange(&slot.state, Ready, Working);
            } else {
                InterlockedIncrement(&control->failures);
                InterlockedCompareExchange(&slot.state, Failed, Working);
            }
        }
    }
    InterlockedExchange(&control->hostReady, 0);
    for (unsigned i = 0; i < kSlots; ++i) Drop(control, i);
    UnmapViewOfFile(control);
    CloseHandle(parent); CloseHandle(wake); CloseHandle(mapping);
    return 0;
}
