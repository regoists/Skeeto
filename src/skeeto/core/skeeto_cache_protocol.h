#pragma once
#include <windows.h>
#include <stdint.h>

// Anonymous mappings and an explicit inherited-handle list: no public pipe,
// command listener, game-memory access, or third-party executable.
namespace skeeto_cache_ipc {
constexpr uint32_t kMagic = 0x43434b53; // SKCC
constexpr uint32_t kVersion = 2;
constexpr uint32_t kSlots = 8192;
constexpr uint32_t kMaxAssetBytes = 32u * 1024u * 1024u;
constexpr uint64_t kMaxStoredBytes = 1024ull * 1024ull * 1024ull;
enum State : LONG { Empty, Offer, Working, Ready, Retire, Failed, Reserved };
struct Slot {
    volatile LONG state;
    uint32_t remoteHandle;
    uint32_t bytes;
    uint32_t ticket;
};
struct Control {
    uint32_t magic, version, parentPid, structureBytes;
    volatile LONG hostReady;
    volatile LONG stopping;
    uint32_t hostPid, reserved;
    alignas(8) volatile LONG64 mappedBytes;
    volatile LONG assets, failures;
    Slot slots[kSlots];
};
static_assert(sizeof(Slot) == 16, "IPC layout");
static_assert(sizeof(void*) == 4, "Build both components with MSVC x86");
}
