#pragma once
#include <windows.h>
#include <stdint.h>

// Experimental and unreliable: not yet thoroughly tested in game.
namespace skeeto_cache {
enum class Status { Off, Starting, Ready, Unsupported, Conflict, HostFailed, Failed };
struct Snapshot {
    Status status;
    unsigned hostPid, coldItems, restores, failures, unsafeItems;
    uint64_t meshBytes, animationBytes, audioBytes, hostBytes, reclaimedBytes;
    unsigned pendingBytes, pendingItems, idleMs;
    bool reclaiming;
    uint64_t movedBytes, peakColdBytes;
    unsigned moves, residentBytes[3], lockedBytes[3];
    unsigned boundMask, trackedItems[3], unsafePerKind[3], lockedItems[3];
    // Bounded native-pool walk. Each item is sampled under the pool mutex;
    // a completed cycle is not a simultaneous snapshot of all frame locks.
    bool inventoryReady, independentAnimations;
    unsigned inventoryCycleMs, rawLockedBytes[3], rawLockedItems[3];
    uint64_t unsafeBytes[3], slabBytes[3], idleBytes[3];
    bool audioVerified;
    uint64_t audioMovedBytes, audioRestoredBytes, audioPeakBytes;
    unsigned audioMetadataChecks, audioColdMetadataChecks;
    uint64_t bufferVaBytes, moveReleasedVaBytes;
    unsigned bufferFailures, meshPinReleases, meshDeferredCommits;
    double meshCommitPeakMs;
    bool timing;
    double maintenanceAvgMs, maintenancePeakMs, restorePeakMs;
};
// Called once from Skeeto's existing startup worker; only Win32/file operations.
void StartWorker(bool enabled, const wchar_t* gameRoot);
// Engine discovery, bounded copy/commit and requested reclamation: Paint thread.
void Pump(bool playing, bool loading);
Snapshot ReadSnapshot();
void RequestReclaim();
// Timing is optional HUD diagnostics, not part of the cold-cache lifecycle.
void SetTimingEnabled(bool enabled);
// Process exit: stop host only. Do not call Source or flush engine caches here.
void StopWorker();
}
