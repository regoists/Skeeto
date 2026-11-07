#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "skeeto_cache_offload.h"
#include "skeeto_cache_protocol.h"
#include "skeeto_memread.h"
#include <bcrypt.h>
#include <shlobj.h>
// Shell's RPC headers define the legacy IDL keyword as a macro.
#ifdef small
#undef small
#endif
#include <intrin.h>
#include <atomic>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace skeeto_cache {
using namespace skeeto_cache_ipc;
namespace {
constexpr unsigned kRecordCount = 65536;
constexpr unsigned kCopyBudget = 512 * 1024;
constexpr unsigned kCopyChunk = 64 * 1024;
constexpr unsigned kCopyJobs = 16;
constexpr uint64_t kPendingBudget = 8ull * 1024 * 1024;
constexpr unsigned kScanBudget = 192;
constexpr unsigned kEligibilityBudget = 8;
constexpr DWORD kIdleMs = 10000;
constexpr DWORD kInitialIdleMs = 3000;
constexpr DWORD kInitialWindowMs = 30000;
constexpr DWORD kMaintenanceIntervalMs = 50;
enum Kind { Mesh, Animation, Audio, Unmanaged = -1 };
struct CopyJob;
struct Record {
    void* section;
    unsigned handle, client, nativeSize, bytes, payloadOffset;
    void* payload;
    void* owner; // Audio keeps its small native descriptor resident.
    DWORD touched;
    bool unsafe;
    HANDLE mapping;
    unsigned slot;
    Kind kind;
    CopyJob* pending;
};
struct CopyJob {
    unsigned handle, copied, ticket;
    Record* record;
    HANDLE mapping;
    void* view;
    unsigned slot;
    DWORD touched, started;
    bool offered;
};
struct Cell { void* data; unsigned size, client; void* section; unsigned handle; };
static_assert(sizeof(Cell) == 20, "Retail x86 cache record header");
std::atomic<Status> g_status{Status::Off};
std::atomic_bool g_reclaim{false};
std::atomic_bool g_prepared{false};
std::atomic_bool g_timingEnabled{false};
HANDLE g_controlMapping{}, g_wake{}, g_host{};
HANDLE g_hostImage{}; // Keep the verified image immutable through process lifetime.
std::atomic<HANDLE> g_job{nullptr};
Control* g_control{};
BYTE* g_dc{};
BYTE* g_engine{};
void* g_root{};
void* g_sections[3]{};
void* g_fs{};
void* g_mdl{};
using AsyncReadFn = int (__thiscall*)(void*, const void*, int, void*);
using FsFreeFn = void (__thiscall*)(void*, void*);
AsyncReadFn g_asyncRead{};
FsFreeFn g_fsFree{};
// Win32 allocations are indexed by their 64-KiB reservation base. Small
// buffers share pages (4/8/16/32 KiB); an empty page releases its entire VA.
// Metadata is allocated only when the startup switch is enabled.
struct BufferPage { unsigned unit, used, bytes, next, previous; };
BufferPage* g_bufferPages{};
unsigned g_bufferHeads[4]{};
SRWLOCK g_bufferLock = SRWLOCK_INIT;
uint64_t g_bufferVa{}, g_moveReleasedVa{};
unsigned g_bufferFailures{};
volatile LONG g_meshReaders{}; // Readers >= 0; -1 means a commit owns the gate.
__declspec(thread) unsigned g_meshFrameDepth{};
__declspec(thread) unsigned g_meshReadDepth{};
__declspec(thread) unsigned g_meshInternalDepth{};
__declspec(thread) unsigned g_meshWriteDepth{};
std::atomic_bool g_meshFinishWanted{false};
Record* g_records{};
CopyJob g_copies[kCopyJobs]{};
uint64_t g_pendingBytes{};
unsigned g_pendingItems{},g_copyRotation{};
bool g_playing{};
DWORD g_enteredAt{};
bool g_installed{}, g_audioVerified{}, g_reclaiming{}, g_independentAnimations{};
DWORD g_launchAt{}, g_bindAt{}, g_statsAt{}, g_maintenanceAt{};
DWORD g_retryAt{};
unsigned g_scan{}, g_slotScan{}, g_ticket{}, g_restores{}, g_failures{}, g_reclaimCursor{};
unsigned g_poolScan{}, g_poolCapacity{}, g_poolRotation{}, g_inventoryCycleMs{};
DWORD g_inventoryAt{};
bool g_inventoryReady{};
struct Inventory {
    unsigned lockedBytes[3]{}, lockedItems[3]{};
    uint64_t unsafeBytes[3]{}, slabBytes[3]{}, idleBytes[3]{};
};
Inventory g_inventory{}, g_inventoryPending{};
uint64_t g_coldBytes[3]{}, g_reclaimed{};
uint64_t g_movedBytes{},g_peakColdBytes{};
uint64_t g_audioMovedBytes{},g_audioRestoredBytes{},g_audioPeakBytes{};
unsigned g_audioMetadataChecks{},g_audioColdMetadataChecks{};
unsigned g_moves{};
LONGLONG g_clockFrequency{},g_maintenanceTicks{},g_maintenancePeakTicks{},g_restorePeakTicks{};
unsigned g_maintenanceSamples{};
unsigned g_coldItems{};
unsigned g_meshPinReleases{},g_meshDeferredCommits{};
LONGLONG g_meshCommitPeakTicks{};
unsigned g_unsafeItems{};
unsigned g_trackedItems[3]{},g_unsafePerKind[3]{};
Snapshot g_snapshot{};
SRWLOCK g_snapshotLock = SRWLOCK_INIT;
struct CostScope {
    bool maintenance, enabled;
    LARGE_INTEGER started{};
    CostScope(bool pump):maintenance(pump),enabled(g_clockFrequency>0 && g_timingEnabled.load(std::memory_order_relaxed)) {
        if(enabled)QueryPerformanceCounter(&started);
    }
    ~CostScope(){
        if(!enabled)return;
        LARGE_INTEGER ended{};QueryPerformanceCounter(&ended);
        LONGLONG ticks=ended.QuadPart-started.QuadPart;
        if(maintenance){g_maintenanceTicks+=ticks;++g_maintenanceSamples;g_maintenancePeakTicks=std::max(g_maintenancePeakTicks,ticks);}
        else g_restorePeakTicks=std::max(g_restorePeakTicks,ticks);
    }
};
// One budget covers both existing jobs and new candidates. The deadline is
// checked between bounded operations; Win32 allocation/free cannot be preempted.
struct PumpBudget {
    unsigned bytes{kCopyBudget};
    LONGLONG deadline{};
    PumpBudget() {
        if(g_clockFrequency>0){
            LARGE_INTEGER now{};QueryPerformanceCounter(&now);
            deadline=now.QuadPart+std::max<LONGLONG>(1,g_clockFrequency/2000); // 0.5 ms
        }
    }
    bool Available() const {
        if(!deadline)return true;
        LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        return now.QuadPart<deadline;
    }
};
static DWORD IdleMs(DWORD now) {
    return g_playing && now-g_enteredAt<kInitialWindowMs?kInitialIdleMs:kIdleMs;
}

// Private CDataManagerBase lookup and the global pool mutex were verified in
// retail datacache.dll (2.2.4.3). Every engine access below runs on its caller's
// existing engine thread, under this same native mutex. The worker never uses it.
using VoidFn = void (__thiscall*)(void*);
using GetFn = void* (__thiscall*)(void*, unsigned, bool);
using LockFn = void* (__thiscall*)(void*, unsigned);
using AddFn = bool (__thiscall*)(void*, unsigned, const void*, unsigned, unsigned, unsigned*);
using RemoveFn = int (__thiscall*)(void*, unsigned, const void**, unsigned*, bool);
using GetCellFn = Cell* (__thiscall*)(void*, unsigned);
using UpdateFn = void (__thiscall*)(void*, unsigned, unsigned);
using UnlockFn = int (__thiscall*)(void*, unsigned);
using LockCountFn = int (__thiscall*)(void*, unsigned);
using FlushFn = unsigned (__thiscall*)(void*, bool, bool);
using FindSectionFn = void* (__thiscall*)(void*, const char*);
using RemoveSectionFn = void (__thiscall*)(void*, const char*, bool);
struct Notification { int type; const char* name; unsigned client; const void* data; unsigned bytes; };
using NotifyFn = bool (__thiscall*)(void*, const Notification*);
static NotifyFn g_notify{};
static void** g_notifySlot{};
static RemoveSectionFn g_removeSection{};
struct Entry { unsigned slot, rva; void* hook; };
static Entry g_entries[9]{};
static Entry g_mdlEntries[7]{};
constexpr unsigned kExpectedCount = 30;
static BYTE g_expected[kExpectedCount][16]{};
static unsigned g_relocMask[kExpectedCount]{}, g_preferredBase{};
constexpr unsigned kExpectedRvas[kExpectedCount]{0x1ee0,0x2030,0x20f0,0x2270,0x32a0,0x3190,0x4370,0x4480,
    0x23f0,0x11c0,0x1200,0x17430,0x6560,0xb530,0x33e0,0x3460,0xed40,0x5b60,0x6200,0x23e0,
    0x21a0,0x2300,0xb750,0xf430,0x102f0,0xf620,0xfdb0,0xcad0,0xcb20,0xd8c0};

template<class T> T Dc(unsigned rva) { return reinterpret_cast<T>(g_dc + rva); }
static void Enter(void* section) { Dc<VoidFn>(0x11c0)(section); }
static void Leave(void* section) { Dc<VoidFn>(0x1200)(section); }
static volatile LONG* PoolMutex() { return *reinterpret_cast<volatile LONG**>(static_cast<BYTE*>(g_root)+0x8c); }
// Same accounting as CDataCacheSection::LockMutex/UnlockMutex. Used only after
// owning the mutex, or after destroying a section whose saved mutex survives.
static void EnterOwned(volatile LONG* mutex) { ++*reinterpret_cast<int*>(g_dc+0x3d800); ++mutex[1]; }
static void LeaveOwned(volatile LONG* mutex) {
    --*reinterpret_cast<int*>(g_dc+0x3d800);
    if(!(--mutex[1]))InterlockedExchange(mutex,0);
}
static Cell* CellFor(unsigned handle) { return Dc<GetCellFn>(0x17430)(static_cast<BYTE*>(g_root) + 8, handle); }
static Kind SectionKind(void* section) {
    for (int i = 0; i < 3; ++i) if (section && section == g_sections[i]) return static_cast<Kind>(i);
    return Unmanaged;
}
static Record* Known(void* section, unsigned handle) {
    if (!handle || !g_records) return nullptr;
    Record& r = g_records[(handle - 1u) & 0xffffu];
    return r.handle == handle && r.section == section ? &r : nullptr;
}
// Small fixed-size engine objects only (descriptors, section headers). Never VirtualQuery:
// the pool's data lives in large contiguous regions where one query costs 200-600us.
// Payload ranges are not pre-checked; CopyStep reads them through skmem::Copy.
static bool Readable(const void* p, size_t bytes) {
    return skmem::SpanProbe(p, bytes);
}
static void RetireSlot(unsigned slot) {
    if (g_control && slot < kSlots) {
        InterlockedExchange(&g_control->slots[slot].state, Retire);
        SetEvent(g_wake);
    }
}
static void DropCold(Record& r) {
    if (!r.mapping) return;
    g_coldBytes[r.kind] -= r.bytes;
    --g_coldItems;
    RetireSlot(r.slot);
    CloseHandle(r.mapping);
    r.mapping = nullptr;
}
static void CancelCopy(CopyJob& copy,bool unlock);
static void ClearRecord(Record& r) {
    if(r.pending)CancelCopy(*r.pending,false);
    DropCold(r);
    if(r.unsafe){--g_unsafeItems;--g_unsafePerKind[r.kind];}
    if(r.handle)--g_trackedItems[r.kind];
    r = {};
}
static void CancelCopy(CopyJob& copy,bool unlock) {
    if (!copy.record) return;
    RetireSlot(copy.slot); // Also release a parent's not-yet-offered reservation.
    if (copy.view) UnmapViewOfFile(copy.view);
    if (copy.mapping) CloseHandle(copy.mapping);
    Record* r = copy.record;
    unsigned handle = copy.handle;
    g_pendingBytes-=r->bytes;--g_pendingItems;
    if(r->pending==&copy)r->pending=nullptr;
    copy = {};
    if (unlock && r->handle == handle) {
        Cell* cell = CellFor(handle);
        if (cell && cell->section == r->section) Dc<UnlockFn>(0x1f60)(r->section, handle);
    }
}
static void CancelCopies(void* section=nullptr) {
    for(CopyJob& copy:g_copies)
        if(copy.record && (!section || copy.record->section==section))CancelCopy(copy,true);
}

static unsigned BufferClass(unsigned unit) {
    return unit==4096?0:unit==8192?1:unit==16384?2:3;
}
static void UnlinkBufferPage(unsigned index) {
    BufferPage& page=g_bufferPages[index];
    if(page.previous)g_bufferPages[page.previous].next=page.next;
    else g_bufferHeads[BufferClass(page.unit)]=page.next;
    if(page.next)g_bufferPages[page.next].previous=page.previous;
    page.next=page.previous=0;
}
static void LinkBufferPage(unsigned index) {
    BufferPage& page=g_bufferPages[index];
    unsigned& head=g_bufferHeads[BufferClass(page.unit)];
    page.previous=0;page.next=head;
    if(head)g_bufferPages[head].previous=index;
    head=index;
}
static void* AllocateBuffer(unsigned bytes) {
    if(!bytes || bytes>kMaxAssetBytes || !g_bufferPages)return nullptr;
    unsigned unit=4096;
    while(unit<bytes && unit<32768)unit*=2;
    const bool small=bytes<=32768;
    AcquireSRWLockExclusive(&g_bufferLock);
    unsigned index=small?g_bufferHeads[BufferClass(unit)]:0;
    if(!index){
        unsigned reserved=small?65536u:(bytes+65535u)&~65535u;
        void* base=VirtualAlloc(nullptr,reserved,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        if(!base){++g_bufferFailures;ReleaseSRWLockExclusive(&g_bufferLock);return nullptr;}
        index=static_cast<unsigned>(reinterpret_cast<uintptr_t>(base)>>16);
        BufferPage& page=g_bufferPages[index];
        page.unit=small?unit:reserved;page.bytes=reserved;page.used=0;
        g_bufferVa+=reserved;
        if(small)LinkBufferPage(index);
    }
    BufferPage& page=g_bufferPages[index];
    unsigned slot=0;
    if(small){
        // At most 16 bits, independent of asset or native-pool size.
        while(page.used&(1u<<slot))++slot;
        page.used|=1u<<slot;
        if(page.used==((1u<<(65536u/unit))-1u))UnlinkBufferPage(index);
    }else page.used=1;
    void* result=reinterpret_cast<void*>((index<<16)+slot*unit);
    ReleaseSRWLockExclusive(&g_bufferLock);
    return result;
}
static bool FreeBuffer(void* data,unsigned* released=nullptr) {
    if(released)*released=0;
    if(!data || !g_bufferPages)return false;
    unsigned address=static_cast<unsigned>(reinterpret_cast<uintptr_t>(data));
    unsigned index=address>>16,offset=address&65535u;
    AcquireSRWLockExclusive(&g_bufferLock);
    BufferPage& page=g_bufferPages[index];
    if(!page.bytes || offset%page.unit || offset/page.unit>=65536u/std::min(page.unit,65536u)
        || !(page.used&(1u<<(offset/page.unit)))){
        ReleaseSRWLockExclusive(&g_bufferLock);return false;
    }
    const bool small=page.unit<=32768;
    unsigned mask=1u<<(offset/page.unit);
    unsigned full=small?((1u<<(65536u/page.unit))-1u):1u;
    if(page.used==mask){
        unsigned bytes=page.bytes;
        if(VirtualFree(reinterpret_cast<void*>(index<<16),0,MEM_RELEASE)){
            if(small && page.used!=full)UnlinkBufferPage(index);
            page={};g_bufferVa-=bytes;
            if(released)*released=bytes;
        }else ++g_bufferFailures; // Never pass a foreign allocation to native free.
    }else{
        bool wasFull=page.used==full;
        page.used&=~mask;
        if(wasFull)LinkBufferPage(index);
    }
    ReleaseSRWLockExclusive(&g_bufferLock);
    return true;
}
static void __fastcall HookFsFree(void* p,void*,void* data) {
    if(!FreeBuffer(data))g_fsFree(p,data);
}
static int __fastcall HookAsyncRead(void* p,void*,const void* requests,int count,void* control) {
    // Retail 5b60 builds a 48-byte request on its stack. Limit allocation
    // substitution to this animation read call; other filesystem users retain
    // their existing destination, flags, allocator and async behavior.
    if(_ReturnAddress()!=g_dc+0x5bd0 || p!=g_fs || count!=1 || !requests || !control)
        return g_asyncRead(p,requests,count,control);
    auto request=static_cast<const unsigned*>(requests);
    auto owner=static_cast<const unsigned*>(control);
    if(owner[0] || owner[2]!=4 || request[1] || !request[3] || request[3]>kMaxAssetBytes
        || (request[7]&~5u))return g_asyncRead(p,requests,count,control);
    void* buffer=AllocateBuffer(request[3]);
    if(!buffer)return g_asyncRead(p,requests,count,control);
    unsigned copy[12];memcpy(copy,requests,sizeof(copy));
    copy[1]=static_cast<unsigned>(reinterpret_cast<uintptr_t>(buffer));
    copy[7]&=~1u; // Provided destination: same flags as retail's old slab path.
    int result=g_asyncRead(p,copy,count,control);
    if(result<0 && !owner[0])FreeBuffer(buffer);
    return result;
}
static void* Allocate(const Record& r) {
    if (r.kind != Mesh) {
        return AllocateBuffer(r.bytes);
    }
    // CMDLCache::AllocData's 32-byte alignment/header, without its fatal Error
    // branch. Its matching native free routines continue to own the allocation.
    auto pp = *reinterpret_cast<void***>(g_dc + 0x2a138);
    void* allocator = *pp;
    using Fn = void* (__thiscall*)(void*, size_t);
    void* raw = reinterpret_cast<Fn>((*static_cast<void***>(allocator))[1])(allocator, r.bytes + 35u);
    if (!raw) return nullptr;
    BYTE* aligned = reinterpret_cast<BYTE*>((reinterpret_cast<uintptr_t>(raw) + 35u) & ~uintptr_t(31));
    *reinterpret_cast<void**>(aligned - 4) = raw;
    return aligned;
}
static unsigned FreePayload(Kind kind, void* data) {
    if (!data) return 0;
    unsigned released{};
    if(kind!=Mesh && FreeBuffer(data,&released))return released;
    if (kind == Mesh || kind == Animation) {
        using Fn = void (__stdcall*)(int, void*);
        Dc<Fn>(0x6560)(kind == Mesh ? 6 : 4, data);
    } else {
        using Fn = void (__thiscall*)(void*, void*);
        reinterpret_cast<Fn>((*static_cast<void***>(g_fs))[84])(g_fs, data);
    }
    return 0;
}
static void ResizeResident(Record& r,Cell* cell,unsigned bytes) {
    const LONG delta=static_cast<LONG>(bytes)-static_cast<LONG>(cell->size);
    const bool unlocked=Dc<LockCountFn>(0x23e0)(r.section,r.handle)==0;
    Dc<UpdateFn>(0x23f0)(r.section,r.handle,bytes);
    // Retail UpdateSize expects a caller lock: it adjusts locked byte totals
    // even when its own temporary pool pin was the only lock. Restore precedes
    // the caller's Get/Lock, so undo that locked-only delta for an unlocked item.
    // Do not Lock/Unlock around this: Unlock may evict the just-restored naked
    // Get result before its caller can consume it. Global native mutex is held.
    if(unlocked && delta){
        *reinterpret_cast<LONG*>(static_cast<BYTE*>(r.section)+0x94)-=delta;
        InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(static_cast<BYTE*>(g_root)+0x64),-delta);
    }
}
static bool Restore(Record& r) {
    if (!r.mapping) return true;
    CostScope cost(false); // Restore-only work, on the original cache caller thread.
    Cell* cell = CellFor(r.handle);
    if (!cell || cell->section != r.section || cell->client != r.client
        || (r.kind == Audio ? (cell->data != r.owner || cell->size!=0x5cu
            || !Readable(r.owner,0x5c) || static_cast<void**>(r.owner)[2]!=nullptr
            || static_cast<void**>(r.owner)[3]!=nullptr) : cell->data != nullptr)) {
        ClearRecord(r); return false;
    }
    const void* view = MapViewOfFile(r.mapping, FILE_MAP_READ, 0, 0, r.bytes);
    if (!view) { ++g_failures; return false; }
    void* buffer = Allocate(r);
    if (!buffer) { UnmapViewOfFile(view); ++g_failures; return false; }
    memcpy(buffer, view, r.bytes);
    UnmapViewOfFile(view);
    if (r.kind == Audio) {
        // +8 is the consumed subrange; +0xC owns the allocation. Rebuild both
        // pointers, keeping the checked offset instead of the old address.
        static_cast<void**>(r.owner)[2] = static_cast<BYTE*>(buffer)+r.payloadOffset;
        static_cast<void**>(r.owner)[3] = buffer;
    }
    else cell->data = buffer;
    r.payload = buffer;
    ResizeResident(r,cell,r.nativeSize);
    DropCold(r);
    r.touched = GetTickCount();
    ++g_restores;
    if(r.kind==Audio)g_audioRestoredBytes+=r.bytes;
    return true;
}
static Record* Track(void* section, unsigned handle) {
    Kind kind = SectionKind(section);
    if (kind == Unmanaged || !handle) return nullptr;
    Cell* cell = CellFor(handle);
    if (!cell || cell->section != section) return nullptr;
    Record& r = g_records[(handle - 1u) & 0xffffu];
    if (r.handle == handle && r.section == section) return &r;
    ClearRecord(r);
    if ((kind == Mesh && (cell->client & 0xffffu) != 6)
        || (kind == Animation && (cell->client & 0xffffu) != 4)) return nullptr;
    r.section = section; r.handle = handle; r.kind = kind; r.client = cell->client;
    ++g_trackedItems[kind];
    r.nativeSize = cell->size; r.touched = GetTickCount();
    if(g_status.load(std::memory_order_acquire)==Status::Starting){
        r.unsafe=true;++g_unsafeItems;++g_unsafePerKind[kind];
    }
    return &r;
}
static void Observe(Record* r, bool protectedAccess) {
    if (!r) return;
    r->touched = GetTickCount();
    // A naked pointer with neither a normal lock nor a real frame lock cannot
    // be retired safely. Keep that entire native-handle generation resident.
    if (!protectedAccess && !r->unsafe) {r->unsafe=true;++g_unsafeItems;++g_unsafePerKind[r->kind];}
}
static BYTE* ModelInfo(unsigned model) {
    if(!g_mdl || (model&0xffffu)>=*reinterpret_cast<unsigned*>(static_cast<BYTE*>(g_mdl)+0x30))return nullptr;
    auto slots=*reinterpret_cast<BYTE**>(static_cast<BYTE*>(g_mdl)+0x2c);
    return slots?*reinterpret_cast<BYTE**>(slots+(model&0xffffu)*16+12):nullptr;
}
static bool MeshOwnedPin(const Record& r,const Cell* cell) {
    BYTE* info=ModelInfo(r.client>>16);
    return info && *reinterpret_cast<unsigned*>(info+0x54)==r.handle
        && *reinterpret_cast<void**>(info+0x70)==cell->data && cell->data;
}
static void EnterMeshRead() {
    if(g_meshReadDepth++ || g_meshWriteDepth)return;
    for(;;){
        LONG readers=InterlockedCompareExchange(&g_meshReaders,0,0);
        if(readers>=0 && InterlockedCompareExchange(&g_meshReaders,readers+1,readers)==readers)return;
        SwitchToThread(); // Only native reader callers; maintenance never waits.
    }
}
static void LeaveMeshRead() {
    if(g_meshReadDepth && !--g_meshReadDepth && !g_meshWriteDepth)InterlockedDecrement(&g_meshReaders);
}
static bool TryMeshWrite() {
    if(g_meshReadDepth || g_meshWriteDepth || InterlockedCompareExchange(&g_meshReaders,-1,0)!=0)return false;
    ++g_meshWriteDepth;return true;
}
static void LeaveMeshWrite() {
    if(g_meshWriteDepth && !--g_meshWriteDepth){
        // A native eviction callback may reenter an accessor/frame scope on
        // this writer thread. Preserve any surviving scope as one reader when
        // publishing the completed commit, rather than self-deadlocking.
        InterlockedExchange(&g_meshReaders,g_meshReadDepth?1:0);
    }
}
static void ObserveVertex(unsigned model,void* value) {
    void* section=g_sections[Mesh];
    if(!value || !section)return;
    Enter(section);
    BYTE* info=ModelInfo(model);
    if(info){
        unsigned handle=*reinterpret_cast<unsigned*>(info+0x54);
        Cell* cell=CellFor(handle);
        if(cell && cell->section==section && cell->data==value)
            Observe(Track(section,handle),g_meshFrameDepth!=0 || g_meshInternalDepth!=0);
    }
    Leave(section);
}
static void* __fastcall HookVertex(void* p,void*,unsigned model) {
    EnterMeshRead();
    using Fn=void* (__thiscall*)(void*,unsigned);
    void* value=Dc<Fn>(0xb750)(p,model);
    if(p==g_mdl)ObserveVertex(model,value);
    LeaveMeshRead();return value;
}
static void* __fastcall HookVertexClient(void* p,void*,void* header) {
    EnterMeshRead();
    using Fn=void* (__thiscall*)(void*,void*);
    void* value=Dc<Fn>(0xf430)(p,header);
    if(p==static_cast<BYTE*>(g_mdl)+8 && header)
        ObserveVertex(*reinterpret_cast<unsigned short*>(static_cast<BYTE*>(header)+0x158),value);
    LeaveMeshRead();return value;
}
static void InternalModelRead(void* p,unsigned model,unsigned rva) {
    // Both internal load/touch methods consume VVD without returning a byte
    // pointer. 102f0 also stores its own native pin in modelInfo+70.
    EnterMeshRead();++g_meshInternalDepth;
    using Fn=void (__thiscall*)(void*,unsigned);
    Dc<Fn>(rva)(p,model);
    --g_meshInternalDepth;LeaveMeshRead();
}
static void __fastcall HookMarkLoaded(void* p,void*,unsigned model){InternalModelRead(p,model,0x102f0);}
static void __fastcall HookTouchModel(void* p,void*,unsigned model){InternalModelRead(p,model,0xf620);}
static void __fastcall HookFlushModel(void* p,void*,unsigned model,int flags) {
    EnterMeshRead();
    using Fn=void (__thiscall*)(void*,unsigned,int);
    Dc<Fn>(0xfdb0)(p,model,flags);
    LeaveMeshRead();
}
static void __fastcall HookFlushModels(void* p,void*,int flags) {
    EnterMeshRead();
    using Fn=void (__thiscall*)(void*,int);
    Dc<Fn>(0xcad0)(p,flags);
    LeaveMeshRead();
}
static void __fastcall HookForceUnlock(void* p,void*) {
    EnterMeshRead();Dc<VoidFn>(0xcb20)(p);LeaveMeshRead();
}
static int __fastcall HookReleaseModel(void* p,void*,unsigned model) {
    EnterMeshRead();
    using Fn=int (__thiscall*)(void*,unsigned);
    int result=Dc<Fn>(0xd8c0)(p,model);
    LeaveMeshRead();return result;
}
static void FinishMeshAtFrameEnd();
static int __fastcall HookBeginFrame(void* p,void*) {
    if(p==g_sections[Mesh]){EnterMeshRead();++g_meshFrameDepth;}
    using Fn=int (__thiscall*)(void*);
    return Dc<Fn>(0x21a0)(p);
}
static int __fastcall HookEndFrame(void* p,void*) {
    using Fn=int (__thiscall*)(void*);
    int result=Dc<Fn>(0x2300)(p);
    if(p==g_sections[Mesh] && g_meshFrameDepth){
        --g_meshFrameDepth;LeaveMeshRead();
        if(!g_meshReadDepth && g_meshFinishWanted.load(std::memory_order_relaxed))FinishMeshAtFrameEnd();
    }
    return result;
}
static void* Access(void* section, unsigned handle, int method, bool frame,void* caller=nullptr) {
    if (SectionKind(section) == Unmanaged) {
        if (method == 10 || method == 16) return Dc<LockFn>(method == 10 ? 0x1ee0 : 0x2270)(section, handle);
        return Dc<GetFn>(method == 12 ? 0x2030 : 0x20f0)(section, handle, frame);
    }
    Enter(section);
    Record* r = Track(section, handle);
    // These verified calls use only the resident descriptor's existence,
    // readiness and async priority. They never read/return its audio buffer.
    // Do not restore bytes or reset idle age merely for a metadata query.
    const bool audioMetadata=r && r->kind==Audio && g_audioVerified && g_engine
        && (caller==g_engine+0x33624 || caller==g_engine+0x33689 || caller==g_engine+0x336c3);
    bool coldMetadata=false;
    if(audioMetadata && r->mapping){
        Cell* cell=CellFor(handle);
        coldMetadata=cell && cell->section==section && cell->client==r->client
            && cell->size==0x5cu && cell->data==r->owner && Readable(r->owner,0x5c)
            && static_cast<void**>(r->owner)[2]==nullptr && static_cast<void**>(r->owner)[3]==nullptr;
    }
    bool restored = !r || coldMetadata || Restore(*r);
    if(!restored && r && r->handle!=handle){r=nullptr;restored=true;}
    void* value{};
    if (restored) {
        if (method == 10 || method == 16) value = Dc<LockFn>(method == 10 ? 0x1ee0 : 0x2270)(section, handle);
        else value = Dc<GetFn>(method == 12 ? 0x2030 : 0x20f0)(section, handle, frame);
    } else if(r) {
        // A failed restore becomes an ordinary native miss. Retire the old
        // handle, allowing the engine's existing load path to read the asset.
        if(!Dc<RemoveFn>(0x32a0)(section,handle,nullptr,nullptr,true)) ClearRecord(*r);
        r=nullptr;
    }
    if(value && audioMetadata){
        ++g_audioMetadataChecks;
        if(coldMetadata)++g_audioColdMetadataChecks;
    }else if(value){
        // Metadata never exposes payload; an already unsafe generation stays
        // resident regardless of later locks. Only safe payload readers need
        // a lock-count query to classify possible pointer escape.
        bool protectedAccess=false;
        if(r && !r->unsafe){
            int locks=Dc<LockCountFn>(0x23e0)(section,handle);
            const bool ownPin=r->pending && r->pending->handle==handle;
            protectedAccess=locks>(ownPin?1:0);
        }
        Observe(r,protectedAccess);
    }
    Leave(section);
    return value;
}
static void* __fastcall HookLock(void* p, void*, unsigned h) { return Access(p,h,10,false); }
static void* __fastcall HookFrameLock(void* p, void*, unsigned h) { return Access(p,h,16,true); }
static void* __fastcall HookGet(void* p, void*, unsigned h, bool f) { return Access(p,h,12,f,_ReturnAddress()); }
static void* __fastcall HookNoTouch(void* p, void*, unsigned h, bool f) { return Access(p,h,13,f,_ReturnAddress()); }
static bool __fastcall HookAdd(void* p, void*, unsigned id, const void* data, unsigned bytes, unsigned flags, unsigned* handle) {
    if (SectionKind(p) == Unmanaged) return Dc<AddFn>(0x3190)(p,id,data,bytes,flags,handle);
    Enter(p);
    unsigned localHandle{};
    unsigned* output=handle?handle:&localHandle;
    bool added = Dc<AddFn>(0x3190)(p,id,data,bytes,flags,output);
    // AddEx permits a null output. Still capture the newly created generation:
    // relying on its first Get misses preloaded assets that then stay idle.
    if (added) Track(p, *output);
    Leave(p); return added;
}
static int __fastcall HookRemove(void* p, void*, unsigned h, const void** data, unsigned* bytes, bool notify) {
    if (SectionKind(p) == Unmanaged) return Dc<RemoveFn>(0x32a0)(p,h,data,bytes,notify);
    Enter(p);
    Record* r = Known(p,h);
    if(r && r->pending)CancelCopy(*r->pending,true);
    int result = Dc<RemoveFn>(0x32a0)(p,h,data,bytes,notify);
    if (!result && r) ClearRecord(*r);
    Leave(p); return result;
}
static void SweepRemoved(unsigned count,PumpBudget& budget) {
    for (unsigned i = 0; i < count; ++i) {
        if(!(i&15u) && !budget.Available())break;
        Record& r = g_records[g_scan++ & 0xffffu];
        if (!r.handle) continue;
        Cell* cell = CellFor(r.handle);
        if (!cell || cell->section != r.section || cell->client != r.client) {
            ClearRecord(r);
        }
    }
}
static unsigned __fastcall HookFlush(void* p, void*, bool unlocked, bool notify) {
    if (SectionKind(p) == Unmanaged) return Dc<FlushFn>(0x4370)(p,unlocked,notify);
    Enter(p);
    CancelCopies(p);
    unsigned result = Dc<FlushFn>(0x4370)(p,unlocked,notify);
    // Cleanup of retired mappings is budgeted in Paint, rather than scanning
    // 65K records inside an engine flush/eviction callback.
    Leave(p); return result;
}
static bool __fastcall HookNotify(void* p, void*, const Notification* n) {
    // These two MDL notification branches only free the byte allocation.
    // A cold item has no resident allocation; the native cache still performs
    // its normal handle, frame-link and counter removal after this callback.
    if (n && n->name && (!strcmp(n->name,"ModelMesh") || !strcmp(n->name,"AnimBlock"))
        && n->type >= 1 && n->type <= 3 && !n->data
        && ((n->client & 0xffffu) == 4 || (n->client & 0xffffu) == 6)) return true;
    return g_notify(p,n);
}
static void __fastcall HookRemoveSection(void* p, void*, const char* name, bool flush) {
    void* section = Dc<FindSectionFn>(0x3460)(p,name);
    Kind kind = SectionKind(section);
    if (kind != Unmanaged) {
        auto mutex=PoolMutex();
        Enter(section);
        CancelCopies(section);
        // This is the section's explicit destruction edge, not a per-frame scan.
        for (unsigned i = 0; i < kRecordCount; ++i) if (g_records[i].section == section) ClearRecord(g_records[i]);
        g_sections[kind] = nullptr;
        g_poolCapacity=0;g_inventoryReady=false;
        g_removeSection(p,name,flush);
        LeaveOwned(mutex); // section has been destroyed; never dereference it.
        return;
    }
    g_removeSection(p,name,flush);
}

static bool Replace(void** slot, void* before, void* after) {
    DWORD old{};
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old)) return false;
    bool ok = InterlockedCompareExchangePointer(slot, after, before) == before;
    DWORD ignored{}; VirtualProtect(slot, sizeof(void*), old, &ignored);
    return ok;
}
static bool SetIndependentAnimations(bool enabled) {
    // Retail ec30 initializes EAX=0, then conditionally allocates a 32-KiB
    // animation slab block at ed48. Skip that allocation so 5b60 supplies a
    // null destination / ALLOCNOFREE, exactly like its native large-block path.
    // Animation AsyncRead destinations are substituted by HookAsyncRead.
    // Both completion and failure release through the matching filesystem
    // FreeOptimalReadBuffer hook; unrelated buffers retain native ownership.
    // One atomic opcode change (JG -> JMP), rather than a live 5-byte detour.
    BYTE* site=g_dc+0xed46;
    DWORD old{};
    if(!VirtualProtect(site,1,PAGE_EXECUTE_READWRITE,&old))return false;
    char before=enabled?0x7f:static_cast<char>(0xeb);
    char after=enabled?static_cast<char>(0xeb):0x7f;
    bool ok=_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(site),after,before)==before;
    if(ok)FlushInstructionCache(GetCurrentProcess(),site,1);
    DWORD ignored{};VirtualProtect(site,1,old,&ignored);
    return ok;
}
static bool BindSections() {
    const char* names[3]{"ModelMesh", "AnimBlock", "WaveData"};
    void* candidate[3]{};
    for (int i = 0; i < 3; ++i) {
        if (i == Audio && !g_audioVerified) continue;
        candidate[i] = Dc<FindSectionFn>(0x3460)(g_root,names[i]);
        if (!candidate[i]) continue;
        BYTE* p = static_cast<BYTE*>(candidate[i]);
        if (!Readable(p,0xdc) || (static_cast<BYTE*>(*reinterpret_cast<void**>(p)) != g_dc+0x2a424
            && static_cast<BYTE*>(*reinterpret_cast<void**>(p)) != g_dc+0x2a68c)) return false;
        if (strcmp(reinterpret_cast<char*>(p+0xc0),names[i]) != 0
            || *reinterpret_cast<void**>(p+8) != static_cast<BYTE*>(g_root)+8
            || *reinterpret_cast<void**>(p+0xd8) != *reinterpret_cast<void**>(static_cast<BYTE*>(g_root)+0x8c)) return false;
        if(i==Audio) {
            auto engine=reinterpret_cast<BYTE*>(GetModuleHandleW(L"engine.dll"));
            void* client=*reinterpret_cast<void**>(p+0xb4);
            if(!engine || !Readable(client,sizeof(void*)) || *static_cast<void**>(client)!=engine+0x354d44
                || (*static_cast<void***>(client))[0]!=engine+0x33b40)candidate[i]=nullptr;
        }
    }
    // Keep restore routing for already-owned cold handles. A failed rebind must
    // never turn a managed cold section into an unmanaged native null payload.
    if(g_installed)for(int i=0;i<3;++i)if(g_sections[i] && candidate[i]!=g_sections[i]) {
        g_status.store(Status::Conflict);return false;
    }
    for (int i = 0; i < 3; ++i) {
        if(g_sections[i]!=candidate[i]){g_poolCapacity=0;g_inventoryReady=false;}
        g_sections[i] = candidate[i];
    }
    return g_sections[Mesh] && g_sections[Animation];
}
static bool Install() {
    g_dc = reinterpret_cast<BYTE*>(GetModuleHandleW(L"datacache.dll"));
    if (!g_dc) return false;
    g_engine=reinterpret_cast<BYTE*>(GetModuleHandleW(L"engine.dll"));
    using Factory = void* (__cdecl*)(const char*, int*);
    auto factory = reinterpret_cast<Factory>(GetProcAddress(reinterpret_cast<HMODULE>(g_dc),"CreateInterface"));
    if (!factory) return false;
    g_root = factory("VDataCache003",nullptr);
    if (!Readable(g_root,0x98) || *static_cast<void**>(g_root) != g_dc+0x2a5ac) return false;
    for (unsigned i = 0; i < kExpectedCount; ++i) {
        BYTE expected[16];memcpy(expected,g_expected[i],sizeof(expected));
        DWORD delta=static_cast<DWORD>(reinterpret_cast<uintptr_t>(g_dc))-g_preferredBase;
        for(unsigned off=0;off<13;++off)if(g_relocMask[i]&(1u<<off)) *reinterpret_cast<DWORD*>(expected+off)+=delta;
        if(memcmp(g_dc+kExpectedRvas[i],expected,sizeof(expected))) {g_status.store(Status::Conflict);return false;}
    }
    BYTE* fsModule = reinterpret_cast<BYTE*>(GetModuleHandleW(L"filesystem_stdio.dll"));
    g_fs = *reinterpret_cast<void**>(g_dc+0x403dc);
    if (!fsModule || !Readable(g_fs,sizeof(void*))
        || *static_cast<void**>(g_fs)!=fsModule+0x47e44
        || (*static_cast<void***>(g_fs))[39] != fsModule+0x11e40
        || (*static_cast<void***>(g_fs))[83] != fsModule+0x164f0
        || (*static_cast<void***>(g_fs))[84] != fsModule+0x159e0) return false;
    if (!BindSections()) return false;
    g_entries[0]={10,0x1ee0,reinterpret_cast<void*>(&HookLock)};
    g_entries[1]={12,0x2030,reinterpret_cast<void*>(&HookGet)};
    g_entries[2]={13,0x20f0,reinterpret_cast<void*>(&HookNoTouch)};
    g_entries[3]={16,0x2270,reinterpret_cast<void*>(&HookFrameLock)};
    g_entries[4]={8,0x32a0,reinterpret_cast<void*>(&HookRemove)};
    g_entries[5]={29,0x3190,reinterpret_cast<void*>(&HookAdd)};
    g_entries[6]={23,0x4370,reinterpret_cast<void*>(&HookFlush)};
    g_entries[7]={17,0x2300,reinterpret_cast<void*>(&HookEndFrame)};
    g_entries[8]={14,0x21a0,reinterpret_cast<void*>(&HookBeginFrame)};
    g_mdlEntries[0]={17,0xb750,reinterpret_cast<void*>(&HookVertex)};
    g_mdlEntries[1]={18,0xf620,reinterpret_cast<void*>(&HookTouchModel)};
    g_mdlEntries[2]={36,0x102f0,reinterpret_cast<void*>(&HookMarkLoaded)};
    g_mdlEntries[3]={22,0xfdb0,reinterpret_cast<void*>(&HookFlushModel)};
    g_mdlEntries[4]={23,0xcad0,reinterpret_cast<void*>(&HookFlushModels)};
    g_mdlEntries[5]={34,0xcb20,reinterpret_cast<void*>(&HookForceUnlock)};
    g_mdlEntries[6]={8,0xd8c0,reinterpret_cast<void*>(&HookReleaseModel)};
    void** tables[2]{reinterpret_cast<void**>(g_dc+0x2a424),reinterpret_cast<void**>(g_dc+0x2a68c)};
    for (auto table : tables) for (const Entry& entry : g_entries)
        if (table[entry.slot] != g_dc+entry.rva) { g_status.store(Status::Conflict); return false; }
    void* client = *reinterpret_cast<void**>(static_cast<BYTE*>(g_sections[Animation])+0xb4);
    if (!Readable(client,sizeof(void*)) || (*static_cast<void***>(client))[0] != g_dc+0xb530) return false;
    if(*reinterpret_cast<void**>(static_cast<BYTE*>(g_sections[Mesh])+0xb4)!=client)return false;
    g_mdl=static_cast<BYTE*>(client)-12;
    if(!Readable(g_mdl,0x38) || *static_cast<void**>(g_mdl)!=g_dc+0x2ad14
        || *reinterpret_cast<void**>(static_cast<BYTE*>(g_mdl)+8)!=g_dc+0x2acf4
        || (*reinterpret_cast<void***>(static_cast<BYTE*>(g_mdl)+8))[6]!=g_dc+0xf430)return false;
    for(unsigned i=0;i<7;++i)if((*static_cast<void***>(g_mdl))[g_mdlEntries[i].slot]!=g_dc+g_mdlEntries[i].rva){g_status.store(Status::Conflict);return false;}
    g_asyncRead=reinterpret_cast<AsyncReadFn>((*static_cast<void***>(g_fs))[39]);
    g_fsFree=reinterpret_cast<FsFreeFn>((*static_cast<void***>(g_fs))[84]);
    g_notifySlot = *static_cast<void***>(client);
    g_notify = reinterpret_cast<NotifyFn>(*g_notifySlot);
    g_removeSection = Dc<RemoveSectionFn>(0x33e0);
    // Cold native handles can outlive the startup worker. Keep hook code valid
    // through native engine teardown; disabling this feature requires restart.
    HMODULE pinned{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&StartWorker),&pinned))return false;
    if (!Replace(g_notifySlot,reinterpret_cast<void*>(g_notify),reinterpret_cast<void*>(&HookNotify))) return false;
    bool ok = Replace(reinterpret_cast<void**>(g_dc+0x2a5ac)+10,reinterpret_cast<void*>(g_removeSection),reinterpret_cast<void*>(&HookRemoveSection));
    if (ok) for (auto table : tables) for (const Entry& entry : g_entries) {
        if (!Replace(table+entry.slot,g_dc+entry.rva,entry.hook)) ok=false;
    }
    void** vertexClientSlot=reinterpret_cast<void**>(g_dc+0x2ad0c);
    void** mdlTable=*static_cast<void***>(g_mdl);
    void** fsTable=*static_cast<void***>(g_fs);
    if(ok)for(unsigned i=0;i<7;++i){const Entry& entry=g_mdlEntries[i];if(!Replace(mdlTable+entry.slot,g_dc+entry.rva,entry.hook))ok=false;}
    if(ok)ok=Replace(vertexClientSlot,g_dc+0xf430,reinterpret_cast<void*>(&HookVertexClient));
    if(ok)ok=Replace(fsTable+84,reinterpret_cast<void*>(g_fsFree),reinterpret_cast<void*>(&HookFsFree));
    if(ok){ok=SetIndependentAnimations(true);g_independentAnimations=ok;}
    // Install allocation last. No failure-prone installation step follows the
    // first possibility of a foreign allocation reaching an async reader.
    if(ok)ok=Replace(fsTable+39,reinterpret_cast<void*>(g_asyncRead),reinterpret_cast<void*>(&HookAsyncRead));
    if (!ok) {
        if(g_independentAnimations){SetIndependentAnimations(false);g_independentAnimations=false;}
        for(unsigned i=0;i<7;++i){const Entry& entry=g_mdlEntries[i];Replace(mdlTable+entry.slot,entry.hook,g_dc+entry.rva);}
        Replace(vertexClientSlot,reinterpret_cast<void*>(&HookVertexClient),g_dc+0xf430);
        Replace(fsTable+84,reinterpret_cast<void*>(&HookFsFree),reinterpret_cast<void*>(g_fsFree));
        for (auto table : tables) for (const Entry& entry : g_entries) Replace(table+entry.slot,entry.hook,g_dc+entry.rva);
        Replace(g_notifySlot,reinterpret_cast<void*>(&HookNotify),reinterpret_cast<void*>(g_notify));
        Replace(reinterpret_cast<void**>(g_dc+0x2a5ac)+10,reinterpret_cast<void*>(&HookRemoveSection),reinterpret_cast<void*>(g_removeSection));
        g_status.store(Status::Conflict); return false;
    }
    g_installed=true; g_status.store(Status::Ready); return true;
}

static bool Eligible(Record& r, DWORD now, unsigned expectedLocks) {
    if (!r.handle || r.mapping || r.pending || r.unsafe || !r.section || r.section != g_sections[r.kind]
        || now-r.touched < IdleMs(now)) return false;
    Cell* cell = CellFor(r.handle);
    if (!cell || cell->section != r.section || cell->client != r.client)return false;
    const bool ownedPin=r.kind==Mesh && MeshOwnedPin(r,cell);
    if(Dc<LockCountFn>(0x23e0)(r.section,r.handle)!=static_cast<int>(expectedLocks+(ownedPin?1:0)))return false;
    r.nativeSize=cell->size;
    r.owner=nullptr;
    if(ownedPin)r.owner=ModelInfo(r.client>>16);
    r.payloadOffset=0;
    if (r.kind == Audio) {
        if (!Readable(cell->data,0x5c)) return false;
        auto object=static_cast<unsigned*>(cell->data);
        // 33110 saves the read prefix in +0x18 and normalizes request length
        // +0x1C to +0. Allocation length was prefix + requested usable bytes.
        // 33f40 can read BEFORE +8, so copy the prefix as well as the subrange.
        if ((object[0x16]&3u)!=1 || object[0x10] || !object[0] || !object[1]
            || object[0]>kMaxAssetBytes || !object[3] || object[2]<object[3]
            || cell->size != object[0]+0x5cu) return false;
        unsigned offset=object[2]-object[3];
        if(offset!=object[6] || object[7]!=object[0] || offset>kMaxAssetBytes-object[0]
            || object[1]>object[0])return false;
        r.payloadOffset=offset;
        r.owner=object; r.payload=reinterpret_cast<void*>(object[3]); r.bytes=object[0]+offset;
    } else {
        r.payload=cell->data; r.bytes=cell->size;
        if (r.kind==Mesh) {
            unsigned head[16];
            if (!skmem::Copy(head, r.payload, sizeof(head)) || head[0]!=0x56534449u || head[1]!=4 || head[12]!=0) return false;
        }
    }
    // Pre-install slab allocations may still exist. New small animation blocks
    // use the verified native independent-buffer path and can be freed.
    if(r.kind==Animation){
        uintptr_t begin=*reinterpret_cast<uintptr_t*>(g_dc+0x3dde0);
        uintptr_t end=*reinterpret_cast<uintptr_t*>(g_dc+0x3dde4);
        uintptr_t data=reinterpret_cast<uintptr_t>(r.payload);
        if(begin && data>=begin && data<end)return false;
    }
    // Tiny sounds otherwise cost a 64-KiB VA reservation per mapping during
    // restore. This floor is per asset, not a section-wide resident threshold.
    if (!r.bytes || r.bytes>kMaxAssetBytes || (r.kind==Audio && r.bytes<4096)) return false;
    return r.payload!=nullptr;
}
static bool HostAlive() {
    return g_host && WaitForSingleObject(g_host,0)==WAIT_TIMEOUT && g_control
        && InterlockedCompareExchange(&g_control->hostReady,0,0);
}
static bool BeginCopy(Record& r, DWORD now) {
    if (!HostAlive() || r.pending || g_pendingItems==kCopyJobs
        || g_coldBytes[0]+g_coldBytes[1]+g_coldBytes[2]+g_pendingBytes+r.bytes>kMaxStoredBytes
        || (g_pendingItems && g_pendingBytes+r.bytes>kPendingBudget)) return false;
    CopyJob* copy=nullptr;
    for(CopyJob& candidate:g_copies)if(!candidate.record){copy=&candidate;break;}
    if(!copy)return false;
    unsigned slot=kSlots;
    for (unsigned i=0;i<64;++i) {
        unsigned candidate=g_slotScan++%kSlots;
        if (InterlockedCompareExchange(&g_control->slots[candidate].state,Reserved,Empty)==Empty) {slot=candidate;break;}
    }
    if (slot==kSlots) return false;
    g_control->slots[slot].remoteHandle=0;
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,r.bytes,nullptr);
    if (!mapping) {InterlockedExchange(&g_control->slots[slot].state,Empty);++g_failures;g_retryAt=now+5000u;return false;}
    void* view=MapViewOfFile(mapping,FILE_MAP_WRITE,0,0,r.bytes);
    if (!view) {CloseHandle(mapping);InterlockedExchange(&g_control->slots[slot].state,Empty);++g_failures;g_retryAt=now+5000u;return false;}
    if (Dc<LockFn>(0x1ee0)(r.section,r.handle)==nullptr) {
        UnmapViewOfFile(view);CloseHandle(mapping);InterlockedExchange(&g_control->slots[slot].state,Empty);return false;
    }
    *copy={r.handle,0,++g_ticket,&r,mapping,view,slot,r.touched,now,false};
    r.pending=copy;g_pendingBytes+=r.bytes;++g_pendingItems;
    return true;
}
static void CopyStep(CopyJob& copy,DWORD now,PumpBudget* budget,bool meshWriteOwned=false) {
    Record& r=*copy.record;
    Cell* cell=CellFor(copy.handle);
    if (!HostAlive() || r.handle!=copy.handle || r.pending!=&copy || !cell || cell->section!=r.section || cell->client!=r.client
        || cell->size!=r.nativeSize || (r.kind==Audio?cell->data!=r.owner:cell->data!=r.payload)
        || r.touched!=copy.touched || r.unsafe || now-copy.started>5000u) {CancelCopy(copy,true);return;}
    if(r.kind==Audio){
        auto object=static_cast<unsigned*>(r.owner);
        if(object[0]!=r.bytes-r.payloadOffset || object[3]!=reinterpret_cast<uintptr_t>(r.payload)
            || object[2]!=object[3]+r.payloadOffset || (object[0x16]&3u)!=1 || object[0x10]
            || object[6]!=r.payloadOffset || object[7]!=object[0]
            || !object[1] || object[1]>object[0]){CancelCopy(copy,true);return;}
    }
    while (copy.copied<r.bytes) {
        if(!budget || !budget->bytes || !budget->Available())return;
        unsigned bytes=std::min(kCopyChunk,std::min(budget->bytes,r.bytes-copy.copied));
        // The payload is read here for the first time, so this is where unreadable memory is caught.
        if(!skmem::Copy(static_cast<BYTE*>(copy.view)+copy.copied,static_cast<BYTE*>(r.payload)+copy.copied,bytes)){
            ++g_failures;g_retryAt=now+5000u;CancelCopy(copy,true);return;
        }
        copy.copied+=bytes;budget->bytes-=bytes;
    }
    if(budget && !budget->Available())return;
    Slot& slot=g_control->slots[copy.slot];
    if (!copy.offered) {
        UnmapViewOfFile(copy.view);copy.view=nullptr;
        HANDLE remote{};
        if (!DuplicateHandle(GetCurrentProcess(),copy.mapping,g_host,&remote,FILE_MAP_READ,FALSE,0)) {++g_failures;g_retryAt=now+5000u;CancelCopy(copy,true);return;}
        slot.remoteHandle=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(remote));
        slot.bytes=r.bytes;slot.ticket=copy.ticket;
        InterlockedExchange(&slot.state,Offer);copy.offered=true;SetEvent(g_wake);return;
    }
    LONG state=InterlockedCompareExchange(&slot.state,0,0);
    if (state==Failed) {++g_failures;g_retryAt=now+5000u;CancelCopy(copy,true);return;}
    if (state!=Ready) return;
    struct MeshWrite {
        bool acquired{},allowed{true};
        MeshWrite(bool mesh,bool already){
            if(mesh && !already){
                allowed=TryMeshWrite();
                acquired=allowed;
            }
        }
        ~MeshWrite(){if(acquired)LeaveMeshWrite();}
    } write(r.kind==Mesh,meshWriteOwned);
    if(!write.allowed){g_meshFinishWanted.store(true,std::memory_order_relaxed);return;}
    const bool ownedPin=r.kind==Mesh && r.owner;
    if(Dc<LockCountFn>(0x23e0)(r.section,r.handle)!=static_cast<int>(1+(ownedPin?1:0))
        || (ownedPin && (ModelInfo(r.client>>16)!=r.owner || !MeshOwnedPin(r,cell)))){
        CancelCopy(copy,true);return;
    }
    if(ownedPin){
        // Only CMDLCache's own cached VVD pin is released. The copy pin remains
        // until commit completes, so Unlock cannot evict the live byte buffer.
        // Both public VVD accessors and complete per-thread frame scopes are
        // excluded by the reader gate; a bare accessor has marked this handle
        // unsafe before its pointer could leave the accessor.
        *reinterpret_cast<void**>(static_cast<BYTE*>(r.owner)+0x70)=nullptr;
        Dc<UnlockFn>(0x1f60)(r.section,r.handle);
        ++g_meshPinReleases;
    }
    // Commit only after the host has mapped the complete immutable snapshot.
    // Parent retains its mapping handle, so host loss cannot destroy cold bytes.
    // Nonzero native bookkeeping keeps retail Unlock's item counters balanced;
    // its zero-byte branch otherwise omits the last-unlock item decrement.
    unsigned residentSize=r.kind==Audio ? 0x5cu : sizeof(Cell);
    ResizeResident(r,cell,residentSize);
    if (r.kind==Audio) {
        static_cast<void**>(r.owner)[2]=nullptr;
        static_cast<void**>(r.owner)[3]=nullptr;
    }
    else cell->data=nullptr;
    g_moveReleasedVa+=FreePayload(r.kind,r.payload);r.payload=nullptr;
    r.mapping=copy.mapping;r.slot=copy.slot;
    g_coldBytes[r.kind]+=r.bytes;++g_coldItems;
    g_movedBytes+=r.bytes;++g_moves;
    g_peakColdBytes=std::max(g_peakColdBytes,g_coldBytes[0]+g_coldBytes[1]+g_coldBytes[2]);
    if(r.kind==Audio){g_audioMovedBytes+=r.bytes;g_audioPeakBytes=std::max(g_audioPeakBytes,g_coldBytes[Audio]);}
    unsigned handle=copy.handle;
    g_pendingBytes-=r.bytes;--g_pendingItems;r.pending=nullptr;copy={};
    Dc<UnlockFn>(0x1f60)(r.section,handle);
}

static void FinishMeshAtFrameEnd() {
    // Paint may run inside an MDL frame. Finish only its already-copied,
    // host-acknowledged item when the outer frame releases its reader gate.
    // No scan, copy, allocation, worker or wait is added to this native edge.
    if(!g_meshFinishWanted.load(std::memory_order_relaxed)
        || !TryMeshWrite())return;
    auto mutex=PoolMutex();
    LONG tid=static_cast<LONG>(GetCurrentThreadId());
    LONG owner=InterlockedCompareExchange(mutex,tid,0);
    if(owner && owner!=tid){LeaveMeshWrite();return;}
    EnterOwned(mutex);
    g_meshFinishWanted.store(false,std::memory_order_relaxed);
    for(CopyJob& copy:g_copies){
        if(!copy.record || copy.record->kind!=Mesh || !copy.offered
            || copy.copied!=copy.record->bytes
            || InterlockedCompareExchange(&g_control->slots[copy.slot].state,0,0)!=Ready)continue;
        LARGE_INTEGER begin{},end{};
        const bool timing=g_clockFrequency>0 && g_timingEnabled.load(std::memory_order_relaxed);
        if(timing)QueryPerformanceCounter(&begin);
        unsigned before=g_moves;
        CopyStep(copy,GetTickCount(),nullptr,true);
        if(before!=g_moves)++g_meshDeferredCommits;
        if(timing){QueryPerformanceCounter(&end);g_meshCommitPeakTicks=std::max(g_meshCommitPeakTicks,end.QuadPart-begin.QuadPart);}
        break; // Keep this native frame boundary to at most one commit.
    }
    for(const CopyJob& copy:g_copies)
        if(copy.record && copy.record->kind==Mesh && copy.offered
            && InterlockedCompareExchange(&g_control->slots[copy.slot].state,0,0)==Ready)
            g_meshFinishWanted.store(true,std::memory_order_relaxed);
    LeaveOwned(mutex);
    LeaveMeshWrite();
}

static void AdvanceCopies(DWORD now,PumpBudget& budget) {
    const unsigned start=g_copyRotation++%kCopyJobs;
    for(unsigned i=0;i<kCopyJobs && budget.Available();++i){
        CopyJob& copy=g_copies[(start+i)%kCopyJobs];
        if(copy.record)CopyStep(copy,now,&budget);
    }
}
static void InspectPool(DWORD now, bool candidates,PumpBudget& budget) {
    // The back-off is one-shot: clear it once due, otherwise after ~24.9 days of uptime the signed
    // difference turns negative again and silently blocks every candidate.
    if(g_retryAt && (LONG)(now-g_retryAt)>=0)g_retryAt=0;
    BYTE* manager=static_cast<BYTE*>(g_root)+8;
    unsigned capacity=*reinterpret_cast<unsigned*>(manager+0x3c);
    auto slots=*reinterpret_cast<BYTE**>(manager+0xc);
    if(!slots || !capacity || capacity>kRecordCount){g_inventoryReady=false;return;}
    if(capacity!=g_poolCapacity){
        g_poolCapacity=capacity;g_poolScan=0;g_poolRotation=0;g_inventoryAt=now;
        g_inventoryPending={};g_inventoryReady=false;
    }
    unsigned probes=0;
    // The pool's generation and Lookup validate live slots under its mutex.
    // Walking the actual capacity also avoids searching 65K mostly empty
    // shadow records, and gives a bounded, rolling lock-count sample.
    for(unsigned n=0;n<std::min(kScanBudget,capacity);++n){
        if(!(n&15u) && !budget.Available())break;
        unsigned index=(g_poolScan++ + g_poolRotation)%capacity;
        unsigned generation=*reinterpret_cast<unsigned short*>(slots+index*12+2);
        unsigned handle=(generation<<16)|(index+1u);
        Cell* cell=CellFor(handle);
        Record& old=g_records[index];
        if(old.handle && (!cell || old.handle!=handle || old.section!=cell->section || old.client!=cell->client)){
            ClearRecord(old);
        }
        if(cell){
            Kind kind=SectionKind(cell->section);
            if(kind!=Unmanaged){
                Record* r=Known(cell->section,handle);
                if(!r){
                    // A pre-install item may already have escaped pointers.
                    // Inventory must not turn "unobserved" into "safe".
                    r=Track(cell->section,handle);Observe(r,false);
                }
                int locks=Dc<LockCountFn>(0x23e0)(cell->section,handle);
                if(r && r->pending && r->pending->handle==handle)--locks;
                if(locks>0){g_inventoryPending.lockedBytes[kind]+=cell->size;++g_inventoryPending.lockedItems[kind];}
                if(r && !r->mapping){
                    if(r->unsafe)g_inventoryPending.unsafeBytes[kind]+=cell->size;
                    else if(now-r->touched<IdleMs(now))g_inventoryPending.idleBytes[kind]+=cell->size;
                    if(kind==Animation){
                        uintptr_t data=reinterpret_cast<uintptr_t>(cell->data);
                        uintptr_t begin=*reinterpret_cast<uintptr_t*>(g_dc+0x3dde0);
                        uintptr_t end=*reinterpret_cast<uintptr_t*>(g_dc+0x3dde4);
                        if(begin && data>=begin && data<end)g_inventoryPending.slabBytes[kind]+=cell->size;
                    }
                    bool ownedPin=kind==Mesh && locks==1 && MeshOwnedPin(*r,cell);
                    if(candidates && g_pendingItems<kCopyJobs && !r->pending && !r->unsafe && (!locks || ownedPin) && now-r->touched>=IdleMs(now)
                        && probes<kEligibilityBudget && !g_retryAt){
                        ++probes;
                        if(budget.bytes && budget.Available() && Eligible(*r,now,0) && budget.Available()
                            && BeginCopy(*r,now) && budget.Available())CopyStep(*r->pending,now,&budget);
                    }
                }
            }
        }
        if(g_poolScan==capacity){
            g_inventory=g_inventoryPending;g_inventoryPending={};
            g_inventoryCycleMs=now-g_inventoryAt;g_inventoryAt=now;g_inventoryReady=true;g_poolScan=0;
            // Rotate the next pass so eight repeatedly unsupported entries
            // cannot exhaust every probe budget ahead of the same candidates.
            g_poolRotation=(g_poolRotation+kEligibilityBudget)%capacity;
            break;
        }
    }
}

static void Publish() {
    Snapshot s{};s.status=g_status.load();
    s.hostPid=g_control?g_control->hostPid:0;s.coldItems=g_coldItems;s.restores=g_restores;
    s.failures=g_failures;s.unsafeItems=g_unsafeItems;s.meshBytes=g_coldBytes[Mesh];s.animationBytes=g_coldBytes[Animation];s.audioBytes=g_coldBytes[Audio];
    s.hostBytes=HostAlive()?static_cast<uint64_t>(InterlockedCompareExchange64(&g_control->mappedBytes,0,0)):0;
    s.reclaimedBytes=g_reclaimed;s.pendingBytes=static_cast<unsigned>(g_pendingBytes);s.pendingItems=g_pendingItems;
    s.idleMs=IdleMs(GetTickCount());
    s.reclaiming=g_reclaiming;
    s.movedBytes=g_movedBytes;s.peakColdBytes=g_peakColdBytes;s.moves=g_moves;
    s.audioVerified=g_audioVerified;
    s.audioMovedBytes=g_audioMovedBytes;s.audioRestoredBytes=g_audioRestoredBytes;s.audioPeakBytes=g_audioPeakBytes;
    s.audioMetadataChecks=g_audioMetadataChecks;s.audioColdMetadataChecks=g_audioColdMetadataChecks;
    s.inventoryReady=g_inventoryReady;s.inventoryCycleMs=g_inventoryCycleMs;
    s.independentAnimations=g_independentAnimations;
    static uint64_t lastBufferVa{};
    static unsigned lastBufferFailures{};
    if(TryAcquireSRWLockShared(&g_bufferLock)){
        lastBufferVa=g_bufferVa;lastBufferFailures=g_bufferFailures;
        ReleaseSRWLockShared(&g_bufferLock);
    }
    s.bufferVaBytes=lastBufferVa;s.bufferFailures=lastBufferFailures;
    s.moveReleasedVaBytes=g_moveReleasedVa;
    s.meshPinReleases=g_meshPinReleases;s.meshDeferredCommits=g_meshDeferredCommits;
    s.meshCommitPeakMs=g_clockFrequency?1000.0*g_meshCommitPeakTicks/g_clockFrequency:0.0;
    for(int i=0;i<3;++i){
        s.trackedItems[i]=g_trackedItems[i];s.unsafePerKind[i]=g_unsafePerKind[i];
        if(!g_sections[i])continue;
        s.boundMask|=1u<<i;
        s.residentBytes[i]=*reinterpret_cast<unsigned*>(static_cast<BYTE*>(g_sections[i])+0x8c);
        s.rawLockedBytes[i]=*reinterpret_cast<unsigned*>(static_cast<BYTE*>(g_sections[i])+0x94);
        s.rawLockedItems[i]=*reinterpret_cast<unsigned*>(static_cast<BYTE*>(g_sections[i])+0x98);
        s.lockedBytes[i]=g_inventory.lockedBytes[i];s.lockedItems[i]=g_inventory.lockedItems[i];
        s.unsafeBytes[i]=g_inventory.unsafeBytes[i];s.slabBytes[i]=g_inventory.slabBytes[i];s.idleBytes[i]=g_inventory.idleBytes[i];
    }
    s.timing=g_timingEnabled.load(std::memory_order_relaxed) && g_clockFrequency>0;
    if(s.timing){
        const double unit=1000.0/g_clockFrequency;
        s.maintenanceAvgMs=g_maintenanceSamples?unit*g_maintenanceTicks/g_maintenanceSamples:0.0;
        s.maintenancePeakMs=unit*g_maintenancePeakTicks;
        s.restorePeakMs=unit*g_restorePeakTicks;
    }
    g_maintenanceTicks=0;g_maintenancePeakTicks=0;g_maintenanceSamples=0;
    // Bounded metadata sample, not a scan of process VA or all cache records.
    if (TryAcquireSRWLockExclusive(&g_snapshotLock)) {g_snapshot=s;ReleaseSRWLockExclusive(&g_snapshotLock);}
}

static bool HostImageDigest(const BYTE* image,DWORD bytes,wchar_t (&text)[65]) {
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};BYTE digest[32]{};
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0
        && BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0
        && BCryptHashData(hash,const_cast<BYTE*>(image),bytes,0)>=0
        && BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    if(ok){
        const wchar_t hex[]=L"0123456789abcdef";
        for(unsigned i=0;i<32;++i){text[i*2]=hex[digest[i]>>4];text[i*2+1]=hex[digest[i]&15u];}
        text[64]=0;
    }
    return ok;
}
static HANDLE OpenHostImage(const wchar_t* path,const BYTE* image,DWORD bytes) {
    // No write/delete sharing: retain this handle through CreateProcess and
    // thereafter so a validated cache file cannot be replaced before launch.
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if(file==INVALID_HANDLE_VALUE)return nullptr;
    LARGE_INTEGER size{};bool ok=GetFileSizeEx(file,&size) && size.QuadPart==bytes;
    BYTE buffer[4096];
    for(DWORD at=0;ok && at<bytes;){
        DWORD chunk=std::min<DWORD>(sizeof(buffer),bytes-at),read{};
        ok=ReadFile(file,buffer,chunk,&read,nullptr) && read==chunk && !memcmp(buffer,image+at,chunk);
        at+=chunk;
    }
    if(!ok){CloseHandle(file);return nullptr;}
    return file;
}
static bool PrepareEmbeddedHost(wchar_t* path,size_t count) {
    // Startup worker only. A disabled module never queries resources, creates
    // files or starts a host. Source/IPC behavior is independent of this step.
    HMODULE self{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&PrepareEmbeddedHost),&self))return false;
    HRSRC resource=FindResourceW(self,L"SKEETO_CACHE_HOST",MAKEINTRESOURCEW(10));
    DWORD bytes=resource?SizeofResource(self,resource):0;
    const BYTE* image=resource?static_cast<const BYTE*>(LockResource(LoadResource(self,resource))):nullptr;
    if(!image || bytes<sizeof(IMAGE_DOS_HEADER) || bytes>4u*1024u*1024u
        || image[0]!='M' || image[1]!='Z')return false;
    wchar_t digest[65];if(!HostImageDigest(image,bytes,digest))return false;
    wchar_t local[MAX_PATH],directory[MAX_PATH];
    if(FAILED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,nullptr,SHGFP_TYPE_CURRENT,local)))return false;
    if(_snwprintf_s(directory,_countof(directory),_TRUNCATE,L"%s\\Skeeto",local)<0
        || (!CreateDirectoryW(directory,nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS))return false;
    if(_snwprintf_s(directory,_countof(directory),_TRUNCATE,L"%s\\Skeeto\\CacheHost",local)<0
        || (!CreateDirectoryW(directory,nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS))return false;
    if(_snwprintf_s(path,count,_TRUNCATE,L"%s\\skeeto_cache_host-%s.exe",directory,digest)<0)return false;
    if(HANDLE existing=OpenHostImage(path,image,bytes)){g_hostImage=existing;return true;}

    // Write a private complete file first, then publish atomically. Different
    // host contents have different names, so an old running host is untouched.
    wchar_t temporary[MAX_PATH];HANDLE file=INVALID_HANDLE_VALUE;
    for(unsigned attempt=0;attempt<8;++attempt){
        if(_snwprintf_s(temporary,_countof(temporary),_TRUNCATE,L"%s\\host-%lu-%lu-%u.tmp",
            directory,GetCurrentProcessId(),GetCurrentThreadId(),attempt)<0)return false;
        file=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE)break;
        if(GetLastError()!=ERROR_FILE_EXISTS && GetLastError()!=ERROR_ALREADY_EXISTS)return false;
    }
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written{};bool complete=WriteFile(file,image,bytes,&written,nullptr) && written==bytes;
    CloseHandle(file);
    if(complete)MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    DeleteFileW(temporary); // Only our CREATE_NEW file; never sweep user files.
    // Another game instance can win publication. Always verify the final file,
    // including that race or a blocked replacement; never launch stale bytes.
    g_hostImage=OpenHostImage(path,image,bytes);
    return g_hostImage!=nullptr;
}

static bool HashFile(const wchar_t* path,const char* expected,BYTE* expectedCode=nullptr) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE hash{};BYTE result[32]{};BYTE buffer[65536];DWORD count{};
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0
        && BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    while(ok) {
        if(!ReadFile(file,buffer,sizeof(buffer),&count,nullptr)){ok=false;break;}
        if(!count)break;
        ok=BCryptHashData(hash,buffer,count,0)>=0;
    }
    if(hash) ok=BCryptFinishHash(hash,result,sizeof(result),0)>=0 && ok;
    if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);
    if(ok) for(unsigned i=0;i<32;++i){char pair[3]{expected[i*2],expected[i*2+1],0};if(result[i]!=strtoul(pair,nullptr,16)){ok=false;break;}}
    if(ok && expectedCode) {
        SetFilePointer(file,0,nullptr,FILE_BEGIN);
        DWORD size=GetFileSize(file,nullptr);
        BYTE* image=static_cast<BYTE*>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        if(!image || !ReadFile(file,image,size,&count,nullptr) || count!=size)ok=false;
        if(ok) {
            auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);
            auto nt=reinterpret_cast<IMAGE_NT_HEADERS32*>(image+dos->e_lfanew);
            auto sections=IMAGE_FIRST_SECTION(nt);
            g_preferredBase=nt->OptionalHeader.ImageBase;
            auto raw=[&](DWORD rva)->BYTE*{for(unsigned n=0;n<nt->FileHeader.NumberOfSections;++n)if(rva>=sections[n].VirtualAddress && rva-sections[n].VirtualAddress<sections[n].SizeOfRawData)return image+sections[n].PointerToRawData+rva-sections[n].VirtualAddress;return nullptr;};
            for(unsigned i=0;i<kExpectedCount;++i){BYTE* from=raw(kExpectedRvas[i]);if(!from){ok=false;break;}memcpy(expectedCode+i*16,from,16);}
            DWORD relocRva=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
            DWORD relocSize=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            for(DWORD off=0;ok && off<relocSize;){
                auto block=reinterpret_cast<IMAGE_BASE_RELOCATION*>(raw(relocRva+off));
                if(!block || block->SizeOfBlock<8 || block->SizeOfBlock>relocSize-off){ok=false;break;}
                WORD* entries=reinterpret_cast<WORD*>(block+1);
                for(DWORD n=0;n<(block->SizeOfBlock-8)/2;++n)if((entries[n]>>12)==IMAGE_REL_BASED_HIGHLOW){
                    DWORD rva=block->VirtualAddress+(entries[n]&0xfffu);
                    for(unsigned i=0;i<kExpectedCount;++i)if(rva>=kExpectedRvas[i] && rva+4<=kExpectedRvas[i]+16)
                        g_relocMask[i]|=1u<<(rva-kExpectedRvas[i]);
                }
                off+=block->SizeOfBlock;
            }
        }
        if(image)VirtualFree(image,0,MEM_RELEASE);
    }
    CloseHandle(file);return ok;
}
}

void StartWorker(bool enabled,const wchar_t* gameRoot) {
    if(!enabled)return;
    g_status.store(Status::Starting);g_launchAt=GetTickCount();
    LARGE_INTEGER frequency{};
    if(QueryPerformanceFrequency(&frequency))g_clockFrequency=frequency.QuadPart;
    auto fail=[](Status status){
        // Startup has not published g_prepared or installed any engine hooks.
        // Release partial Win32 state before exposing a terminal status.
        StopWorker();
        if(g_host){CloseHandle(g_host);g_host=nullptr;}
        if(g_control){UnmapViewOfFile(g_control);g_control=nullptr;}
        if(g_controlMapping){CloseHandle(g_controlMapping);g_controlMapping=nullptr;}
        if(g_wake){CloseHandle(g_wake);g_wake=nullptr;}
        if(g_hostImage){CloseHandle(g_hostImage);g_hostImage=nullptr;}
        if(g_records){VirtualFree(g_records,0,MEM_RELEASE);g_records=nullptr;}
        if(g_bufferPages){VirtualFree(g_bufferPages,0,MEM_RELEASE);g_bufferPages=nullptr;}
        g_status.store(status,std::memory_order_release);
    };
    wchar_t path[MAX_PATH];
    swprintf_s(path,L"%s\\bin\\datacache.dll",gameRoot);
    if(!HashFile(path,"520aa669f89d3177c50098542a6df063cc168c4125cafc654f6b78dbcf25a4b9",&g_expected[0][0])){g_status.store(Status::Unsupported);return;}
    swprintf_s(path,L"%s\\bin\\filesystem_stdio.dll",gameRoot);
    if(!HashFile(path,"d0296dda2b65d86cfd7b63b970b2c60ddb40b459b022afc2a70364e3234afdc9")){g_status.store(Status::Unsupported);return;}
    swprintf_s(path,L"%s\\bin\\engine.dll",gameRoot);
    g_audioVerified=HashFile(path,"8a6d468059394bf4cfce32f11dcba12ef45de88da1373084d5a2916d0bf7ba86");
    if(!PrepareEmbeddedHost(path,_countof(path))){fail(Status::HostFailed);return;}
    g_records=static_cast<Record*>(VirtualAlloc(nullptr,sizeof(Record)*kRecordCount,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    g_bufferPages=static_cast<BufferPage*>(VirtualAlloc(nullptr,sizeof(BufferPage)*65536u,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE};
    g_controlMapping=CreateFileMappingW(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE,0,sizeof(Control),nullptr);
    g_wake=CreateEventW(&sa,FALSE,FALSE,nullptr);
    g_control=static_cast<Control*>(MapViewOfFile(g_controlMapping,FILE_MAP_WRITE,0,0,sizeof(Control)));
    g_job=CreateJobObjectW(nullptr,nullptr);
    HANDLE parent{};
    DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),GetCurrentProcess(),&parent,
        SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,TRUE,0);
    if(!g_records || !g_bufferPages || !g_control || !g_wake || !g_job || !parent){if(parent)CloseHandle(parent);fail(Status::Failed);return;}
    g_control->magic=kMagic;g_control->version=kVersion;g_control->parentPid=GetCurrentProcessId();g_control->structureBytes=sizeof(Control);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if(!SetInformationJobObject(g_job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){CloseHandle(parent);fail(Status::HostFailed);return;}
    SIZE_T attrBytes{};InitializeProcThreadAttributeList(nullptr,1,0,&attrBytes);
    auto attrs=static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(),0,attrBytes));
    HANDLE inherited[3]{g_controlMapping,g_wake,parent};
    bool attrInitialized=attrs && InitializeProcThreadAttributeList(attrs,1,0,&attrBytes);
    bool attrOk=attrInitialized
        && UpdateProcThreadAttribute(attrs,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr);
    STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.lpAttributeList=attrs;
    PROCESS_INFORMATION process{};
    wchar_t command[MAX_PATH+128];swprintf_s(command,L"\"%s\" %x %x %x",path,
        static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_controlMapping)),static_cast<unsigned>(reinterpret_cast<uintptr_t>(g_wake)),static_cast<unsigned>(reinterpret_cast<uintptr_t>(parent)));
    bool created=attrOk && CreateProcessW(path,command,nullptr,nullptr,TRUE,
        CREATE_NO_WINDOW|CREATE_SUSPENDED|EXTENDED_STARTUPINFO_PRESENT|BELOW_NORMAL_PRIORITY_CLASS,nullptr,nullptr,&startup.StartupInfo,&process);
    if(attrInitialized)DeleteProcThreadAttributeList(attrs);if(attrs)HeapFree(GetProcessHeap(),0,attrs);CloseHandle(parent);
    if(!created){fail(Status::HostFailed);return;}
    if(!AssignProcessToJobObject(g_job,process.hProcess)){
        TerminateProcess(process.hProcess,5);CloseHandle(process.hThread);CloseHandle(process.hProcess);fail(Status::HostFailed);return;
    }
    g_host=process.hProcess;ResumeThread(process.hThread);CloseHandle(process.hThread);
    HANDLE wait=g_host;
    for(unsigned i=0;i<150 && !InterlockedCompareExchange(&g_control->hostReady,0,0);++i)
        if(WaitForSingleObject(wait,20)!=WAIT_TIMEOUT)break;
    if(!HostAlive())fail(Status::HostFailed);
    else g_prepared.store(true,std::memory_order_release);
}

void Pump(bool playing,bool loading) {
    Status status=g_status.load(std::memory_order_acquire);
    if(status==Status::Off)return;
    DWORD now=GetTickCount();
    const bool active=playing && !loading;
    if(active && !g_playing)g_enteredAt=now;
    g_playing=active;
    if(status==Status::Starting && !g_installed){
        if(!g_prepared.load(std::memory_order_acquire))return;
        if(!HostAlive()){if(now-g_launchAt>5000u)g_status.store(Status::HostFailed);return;}
        if(now-g_bindAt<250u)return;g_bindAt=now;
        if(!Install() && now-g_launchAt>60000u && g_status.load()==Status::Starting)g_status.store(Status::Failed);
        return;
    }
    if(!g_installed){
        if(g_job.load(std::memory_order_relaxed))StopWorker();
        if(now-g_statsAt>1000u){g_statsAt=now;Publish();}return;
    }
    // Try-lock the native recursive mutex, then let the original method account
    // for recursion and the native "in transaction" counter. Never wait in Paint.
    auto mutex=PoolMutex();
    LONG tid=static_cast<LONG>(GetCurrentThreadId());
    LONG owner=InterlockedCompareExchange(mutex,tid,0);
    if(owner && owner!=tid)return;
    EnterOwned(mutex);
    // Queue state is shared with native callers and frame-end commits: read it
    // only under the pool mutex. Preserve full Paint rate during map warmup,
    // in-flight work and explicit reclamation; stable inventory needs only 20 Hz.
    const bool fullRate=g_pendingItems || g_reclaiming || g_reclaim.load(std::memory_order_relaxed)
        || (active && now-g_enteredAt<kInitialWindowMs);
    if(!fullRate && now-g_maintenanceAt<kMaintenanceIntervalMs){LeaveOwned(mutex);return;}
    g_maintenanceAt=now;
    CostScope maintenanceCost(true); // Times work passes only, when HUD timing is on.
    if(!HostAlive())g_status.store(Status::HostFailed);
    PumpBudget budget;
    if(now-g_bindAt>1000u){
        g_bindAt=now;
        if(!BindSections() && (g_sections[Mesh] || g_sections[Animation]))g_status.store(Status::Conflict);
    }
    SweepRemoved(kScanBudget,budget);
    if(g_reclaim.exchange(false) && !g_reclaiming){CancelCopies();g_reclaiming=true;g_reclaimCursor=0;}
    if(g_reclaiming){
        for(unsigned n=0;n<kScanBudget && g_reclaimCursor<kRecordCount;++n){
            if(!(n&7u) && !budget.Available())break;
            Record& r=g_records[g_reclaimCursor++];
            if(!r.mapping)continue;
            Cell* cell=CellFor(r.handle);
            if(!cell || cell->section!=r.section){ClearRecord(r);continue;}
            if(Dc<LockCountFn>(0x23e0)(r.section,r.handle))continue;
            uint64_t bytes=r.bytes;
            // Cold audio retains its descriptor, so its normal destructor must
            // run. MDL cold entries have no byte allocation to free.
            int result=Dc<RemoveFn>(0x32a0)(r.section,r.handle,nullptr,nullptr,true);
            if(!result){ClearRecord(r);g_reclaimed+=bytes;}
        }
        if(g_reclaimCursor==kRecordCount)g_reclaiming=false;
    }else if(g_pendingItems){
        if(!active || g_status.load()!=Status::Ready)CancelCopies();
        else AdvanceCopies(now,budget);
    }
    InspectPool(now,active && !g_reclaiming && g_status.load()==Status::Ready && HostAlive(),budget);
    if(now-g_statsAt>1000u){g_statsAt=now;Publish();}
    LeaveOwned(mutex);
}
Snapshot ReadSnapshot() {
    Snapshot out{};
    AcquireSRWLockShared(&g_snapshotLock);out=g_snapshot;ReleaseSRWLockShared(&g_snapshotLock);
    out.status=g_status.load(std::memory_order_acquire);return out;
}
void RequestReclaim(){g_reclaim.store(true,std::memory_order_release);}
void SetTimingEnabled(bool enabled){g_timingEnabled.store(enabled,std::memory_order_relaxed);}
void StopWorker(){
    if(g_control){InterlockedExchange(&g_control->stopping,1);if(g_wake)SetEvent(g_wake);}
    // Handles and cold records are process-lifetime resources. Closing the job
    // at process exit kills the child; cold mapping handles remain valid until
    // Windows tears down the game, including its native cache destructors.
    if(HANDLE job=g_job.exchange(nullptr,std::memory_order_acq_rel))CloseHandle(job);
}
}
