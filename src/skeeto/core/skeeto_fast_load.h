// Loading experiments: background sleep, pure VPK path reuse and directory candidates.
// The verified retail caller is main-thread-only. Configure/Observe/GetStats run
// on Paint; native loading-finished clears work even without Paint. Command
// callbacks apply only on the verified Paint thread, otherwise publish requests.
// No new worker, file cache or allocation in this controller.
#pragma once
#include "skeeto_memread.h"
#include <psapi.h>
#include <intrin.h>
#include "skeeto_staticprop_groups.h"
#include "skeeto_vpk_experiments.h"
#include "skeeto_texture_warmup.h"

namespace skload {

enum class Status { Off, Ready, Unsupported, Conflict, PatchFailed };
// Local server level/option edges, on the established game/Paint thread only.
// The retail transition scheduler reads this numeric ConVar once. No resource
// loading, global timescale, per-frame native polling or remote server writes.
struct LocalTransitionWait {
    bool (*read)(const char*,int*,float*) = nullptr;
    bool (*write)(const char*,int,float) = nullptr;
    void* image = nullptr;
    bool owned = false;
    int savedInt = 0;
    float savedFloat = 0;
    Status status = Status::Off;
    unsigned writes = 0, restores = 0, conflicts = 0, failures = 0;
    static constexpr const char* name = "changelevel_pause_interval";
    bool Begin(void* module) {
        if(owned)return true;
        if(!module || !read || !write || !read(name,&savedInt,&savedFloat)) {
            ++failures;status=Status::Unsupported;return false;
        }
        status=Status::Ready;
        if(savedFloat<=0.f)return true; // Do not override an already disabled wait.
        image=module;
        if(!write(name,0,0.f)) {++failures;status=Status::PatchFailed;return false;}
        owned=true;++writes;return true;
    }
    bool End(void* module) {
        if(!owned)return true;
        if(module!=image) {owned=false;++failures;status=Status::Unsupported;return true;}
        int n=0;float f=0;
        if(!read(name,&n,&f)) {++failures;status=Status::PatchFailed;return false;}
        if(n!=0 || f!=0.f) {owned=false;++conflicts;status=Status::Conflict;return true;}
        if(!write(name,savedInt,savedFloat)) {++failures;status=Status::PatchFailed;return false;}
        owned=false;++restores;status=Status::Off;return true;
    }
} inline g_localTransitionWait;
// Cold game/Paint option edges only. Retail client CanHideLoadingPlaque reads
// only m_isTeamFrozen. Keep native resource completion and server state intact;
// replace its short JZ with same-size JMP to the existing true return.
// No per-frame work, saved resources, worker or call interception.
struct EarlyGameView {
    uint8_t* image = nullptr;
    bool owned = false;
    Status status = Status::Off;
    unsigned writes = 0, restores = 0, failures = 0, conflicts = 0;
    bool Swap(bool enable) {
        uint8_t expected[]{0x8b,0x0d,0,0,0,0,0x85,0xc9,0x74,0x0c,0xe8,0xd1,0x8d,0x1b,0,0x84,0xc0,0x74,3,0x32,0xc0,0xc3,0xb0,1,0xc3};
        const uintptr_t resource=uintptr_t(image+0x72e700);
        memcpy(expected+2,&resource,4);if(!enable)expected[17]=0xeb;
        uint8_t actual[sizeof(expected)]{};
        if(!skmem::Copy(actual,image+0xa93d0,sizeof(actual))) { ++failures;status=Status::PatchFailed;return false; }
        if(memcmp(actual,expected,sizeof(actual))) { owned=false;++conflicts;status=Status::Conflict;return !enable; }
        auto* site=image+0xa93e1;DWORD old=0,ignored=0;
        if(!VirtualProtect(site,1,PAGE_EXECUTE_READWRITE,&old)) { ++failures;status=Status::PatchFailed;return false; }
        const uint8_t before=uint8_t(_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(site),
            enable?char(0xeb):char(0x74),enable?char(0x74):char(0xeb)));
        const bool wrote=before==(enable?0x74:0xeb);
        const bool protectedAgain=VirtualProtect(site,1,old,&ignored)!=FALSE;
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,1)!=FALSE;
        if(wrote) { owned=enable;if(enable)++writes;else ++restores; }
        else {owned=false;++conflicts;}
        if(!protectedAgain || !flushed)++failures;
        status=!wrote?Status::Conflict:(!protectedAgain||!flushed)?Status::PatchFailed:enable?Status::Ready:Status::Off;
        return wrote && protectedAgain && flushed;
    }
    bool Begin(uint8_t* module,size_t bytes,void* baseClient) {
        if(owned)return true;
        // x86 client SHA c0a8e1e8...eeb4af1f; reject other ABI, changed
        // VClient016 slot 66 or a getter that no longer reads the frozen flag.
        IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS32 nt{};
        uintptr_t table=0,predicate=0;uint8_t getter[7]{};
        const uint8_t nativeGetter[]{0x8a,0x81,0xf1,0x10,0,0,0xc3};
        if(!module || bytes<0x72e704 || !skmem::Copy(&dos,module,sizeof(dos))
            || dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || size_t(dos.e_lfanew)>bytes-sizeof(nt)
            || !skmem::Copy(&nt,module+dos.e_lfanew,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE
            || nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC
            || !baseClient || !skmem::Copy(&table,baseClient,4) || !table
            || !skmem::Copy(&predicate,reinterpret_cast<void*>(table+66*4),4) || predicate!=uintptr_t(module+0xa93d0)
            || !skmem::Copy(getter,module+0x2621b0,sizeof(getter)) || memcmp(getter,nativeGetter,sizeof(getter))) {
            ++failures;status=Status::Unsupported;return false;
        }
        image=module;return Swap(true);
    }
    bool End(uint8_t* currentModule) {
        if(!owned)return true;
        if(currentModule!=image) { owned=false;++failures;status=Status::Unsupported;return true; }
        return Swap(false);
    }
    // Paint only, and only while a one-shot map apply is still waiting.
    // Begin already admitted this image, the rules global and the frozen getter.
    // True means the loading plaque hid early and the team has not started.
    bool HoldsFrozenPreview() const {
        if(!owned || !image) return false;
        void* rules=nullptr;
        if(!skmem::Copy(&rules,image+0x72e700,sizeof(rules)) || !rules) return false;
        uint8_t frozen=0;
        if(!skmem::Copy(&frozen,static_cast<uint8_t*>(rules)+0x10f1,1)) return false;
        return frozen!=0;
    }
} inline g_earlyGameView;
// Cold game-thread edges only. The native ADDSS remains one instruction;
// atomically select its existing read-only 0.0/3.0 constant with ONE byte.
// No server state, readiness test, resource job or shared constant is changed.
struct LocalReadyDelay {
    uint8_t* image = nullptr;
    std::atomic_bool owned{false}; // Foreign shutdown only reads ownership and queues a close.
    Status status = Status::Off;
    unsigned begins = 0, restores = 0, failures = 0, conflicts = 0;
    DWORD beginThread = 0, endThread = 0;
    bool Swap(bool enable) {
        uint8_t expected[]{0xf3,0x0f,0x58,0x05,0,0,0,0}, actual[8]{};
        const uintptr_t constant = uintptr_t(image + (enable ? 0x5d4920 : 0x5d491c));
        memcpy(expected+4,&constant,4);
        auto* site=image+0x28d69f;
        if(!skmem::Copy(actual,site,8)) { ++failures;status=Status::PatchFailed;return false; }
        if(memcmp(actual,expected,8)) { ++conflicts;owned=false;status=Status::Conflict;return !enable; }
        DWORD old=0,ignored=0;
        if(!VirtualProtect(site+4,1,PAGE_EXECUTE_READWRITE,&old)) { ++failures;status=Status::PatchFailed;return false; }
        const char before=_InterlockedCompareExchange8(reinterpret_cast<volatile char*>(site+4),
            enable?0x1c:0x20,enable?0x20:0x1c);
        const bool wrote=before==(enable?0x20:0x1c);
        const bool protectedAgain=VirtualProtect(site+4,1,old,&ignored)!=FALSE;
        const bool flushed=FlushInstructionCache(GetCurrentProcess(),site,8)!=FALSE;
        if(wrote) {
            owned=enable;
            if(enable){++begins;beginThread=GetCurrentThreadId();}
            else {++restores;endThread=GetCurrentThreadId();}
        } else {++conflicts;owned=false;}
        status=!wrote?Status::Conflict:(!protectedAgain||!flushed)?Status::PatchFailed:enable?Status::Ready:Status::Off;
        if(!protectedAgain||!flushed)++failures;
        return wrote && protectedAgain && flushed;
    }
    bool Begin(uint8_t* module,size_t bytes) {
        if(owned)return true;
        // Retail x86 server SHA a93ec916...2e1ea12. Admit the native wait
        // context and relocated operand, not an address from another build.
        IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS32 nt{};
        const uint8_t head[]{0x55,0x8b,0xec,0x83,0xec,0x2c,0x53,0x8b,0xd9,0x80,0x7b,0x28,0};
        const uint8_t store[]{0xf3,0x0f,0x11,0x43,0x2c};
        uint8_t actual[sizeof(head)]{};float constants[2]{};
        if(!module || bytes<0x5d4924 || !skmem::Copy(&dos,module,sizeof(dos))
            || dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || size_t(dos.e_lfanew)>bytes-sizeof(nt)
            || !skmem::Copy(&nt,module+dos.e_lfanew,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE
            || nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC
            || !skmem::Copy(actual,module+0x28d500,sizeof(head)) || memcmp(actual,head,sizeof(head))
            || !skmem::Copy(actual,module+0x28d6a7,sizeof(store)) || memcmp(actual,store,sizeof(store))
            || !skmem::Copy(constants,module+0x5d491c,sizeof(constants)) || constants[0]!=0.f || constants[1]!=3.f
            || (uintptr_t(module+0x5d491c)&~uintptr_t(255))!=(uintptr_t(module+0x5d4920)&~uintptr_t(255))) {
            ++failures;status=Status::Unsupported;return false;
        }
        image=module;return Swap(true);
    }
    bool End(uint8_t* currentModule) {
        if(!owned)return true;
        if(currentModule!=image) { owned=false;++failures;status=Status::Unsupported;return true; }
        return Swap(false);
    }
} inline g_localReadyDelay;
inline std::atomic<unsigned> g_localReadyForeignThread{0};
inline std::atomic_bool g_localReadyEndPending{false};
// Called on Paint: integer composition only; no native calls or allocation.
inline unsigned ConfiguredMode(bool master,bool paths,bool vpk,bool async,bool background,bool localReady,bool earlyView,bool measure,unsigned loadingExperiments=0,unsigned vpkExperiments=0) {
    if(!master)return 0;
    const unsigned experiments=(paths?8u:0u)|(vpk?16u:0u)|(async?32u:0u)|(background?4u:0u)|(localReady?64u:0u)|(earlyView?128u:0u)|((loadingExperiments&7u)<<8)|((vpkExperiments&3u)<<11)|((loadingExperiments&8u)?0x2000u:0u);
    return (experiments?(experiments|1u):0u)|(measure?2u:0u);
}
struct Stats {
    uint64_t calls = 0, drawn = 0, drawTicks = 0;
    uint64_t pathCalls = 0, pathCandidates = 0, pathHits = 0, pathBypasses = 0, pathTicks = 0;
    uint64_t pathNativeCalls = 0, pathSamples = 0;
    uint64_t readCalls = 0, readBytes = 0, readSmall = 0, readFailures = 0, readSamples = 0, readTicks = 0;
    uint64_t indexQueries = 0, indexRejected = 0, indexBuilds = 0, indexNodes = 0, indexTicks = 0, indexBypasses = 0;
    uint64_t indexVerified = 0, indexMismatches = 0;
    uint64_t candidateLists = 0, loopRejected = 0, loopSkipped = 0;
    uint64_t asyncBegins = 0, asyncWrites = 0, asyncRestores = 0, asyncFailures = 0, asyncConflicts = 0;
    uint64_t asyncFinishTicks = 0;
    unsigned waitEdges = 0, finishedCalls = 0;
    unsigned reentries = 0;
    DWORD firstAt = 0, progressToPaintMs = 0;
    bool completed = false;
};
using DrawFn = void(__cdecl*)();
inline std::atomic<unsigned> g_mode{0}; // 1: experiment; 2: timing; 4: load-only sleep; 8: path reuse; 16: directory candidates; 32: native async; 64: local ready countdown; 128: early game view; 256/512/1024: loading-only data/touch/preload experiments
inline unsigned g_requestedMode = ~0u; // One configuration attempt per explicit mode edge.
inline DrawFn g_original = nullptr;
inline uint8_t* g_site = nullptr;
inline uint8_t g_saved[5]{}, g_patch[5]{};
inline bool g_installed = false, g_checked = false, g_inside = false;
inline LARGE_INTEGER g_frequency{};
inline Status g_status = Status::Off;
inline Stats g_stats;
inline std::atomic<DWORD> g_ownerThread{0};
inline bool EndLocalReadyDelay() {
    if(!g_localReadyDelay.owned)return true;
    if(GetCurrentThreadId()!=g_ownerThread.load(std::memory_order_relaxed)) {
        g_localReadyForeignThread.fetch_add(1,std::memory_order_relaxed);
        g_localReadyEndPending.store(true,std::memory_order_release);return false;
    }
    g_localReadyEndPending.store(false,std::memory_order_relaxed);
    return g_localReadyDelay.End(reinterpret_cast<uint8_t*>(GetModuleHandleA("server.dll")));
}
inline std::atomic<bool> g_loading{false};
inline void (*g_loadingEdge)(bool) = nullptr;
inline void __fastcall TextureWarmup(void* self,void*) {
    const bool omit=(g_mode.load(std::memory_order_relaxed)&0x2000u)
        && g_loading.load(std::memory_order_relaxed)
        && GetCurrentThreadId()==g_ownerThread.load(std::memory_order_relaxed);
    sktexturewarm::g_controller.Run(self,omit);
}
// Paint publishes the selected subset; Begin/Complete run on the loading thread.
// Bits: 1 mesh, 2 collision, 4 animation. Unselected switches are not read or written.
inline std::atomic<unsigned> g_nativeAsyncWant{0};
// Game/loading thread only. Up to three native switches and one completion-only,
// bounded static-prop classification pass. Repair follows the mesh switch only.
// No worker, resource cache or mesh IO.
struct NativePreload {
    static constexpr unsigned kCount = 3;
    void* object = nullptr;
    bool (*read)(const char*,int*,float*) = nullptr;
    bool (*write)(const char*,int,float) = nullptr;
    void (__thiscall* finish)(void*) = nullptr;
    const char* names[kCount]{"mod_load_mesh_async","mod_load_vcollide_async","mod_load_anims_async"};
    int savedInts[kCount]{};
    float savedFloats[kCount]{};
    bool owned[kCount]{}, active = false;
    void Begin() {
        if (active) return;
        const unsigned mask = g_nativeAsyncWant.load(std::memory_order_relaxed) & ((1u << kCount) - 1u);
        if (!mask) return;
        if (!object) { ++g_stats.asyncFailures;return; }
        for(unsigned i=0;i<kCount;++i) if((mask&(1u<<i))
            && !read(names[i],&savedInts[i],&savedFloats[i])) { ++g_stats.asyncFailures;return; }
        unsigned pending=0;
        for(unsigned i=0;i<kCount;++i) if((mask&(1u<<i)) && !savedInts[i]) ++pending;
        if(!pending)return;
        active=true;++g_stats.asyncBegins;
        for(unsigned i=0;i<kCount;++i) if((mask&(1u<<i)) && !savedInts[i]) {
            owned[i]=true;++g_stats.asyncWrites;
            if(!write(names[i],1,1.f))++g_stats.asyncFailures;
        }
    }
    bool Complete() {
        if(!active)return true;
        LARGE_INTEGER a{},b{};QueryPerformanceCounter(&a);
        // Drain while async mode still describes the submitted jobs. This is
        // inside loading, before native LoadingFinished exposes normal play.
        // A failed restoration leaves async enabled and can admit new jobs.
        // Every later explicit completion drains again before restoring.
        finish(object);
        if(owned[0] && !g_staticPropGroups.Repair())++g_stats.asyncFailures;
        for(unsigned i=0;i<kCount;++i)if(owned[i]) {
            int n=0;float f=0;
            if(!read(names[i],&n,&f)) { ++g_stats.asyncFailures;continue; }
            if(n!=1 || f!=1.f) { ++g_stats.asyncConflicts;owned[i]=false;continue; }
            if(!write(names[i],savedInts[i],savedFloats[i])) { ++g_stats.asyncFailures;continue; }
            ++g_stats.asyncRestores;owned[i]=false;
        }
        active=false;
        for(unsigned i=0;i<kCount;++i) if(owned[i]) active=true;
        QueryPerformanceCounter(&b);g_stats.asyncFinishTicks+=uint64_t(b.QuadPart-a.QuadPart);
        return !active;
    }
} inline g_nativePreload;
// Owner loading/game thread only, three bounded numeric leases at load edges.
// No worker, polling, resource retention or callbacks. Foreign writes win.
struct LoadingExperiments {
    bool (*read)(const char*,int*,float*) = nullptr;
    bool (*write)(const char*,int,float) = nullptr;
    const char* names[3]{"mod_forcedata","mod_forcetouchdata","cl_forcepreload"};
    int savedInts[3]{};float savedFloats[3]{};
    bool owned[3]{},active=false;
    unsigned writes=0,restores=0,conflicts=0,failures=0;
    void Begin(unsigned mask) {
        if(active)return;
        for(unsigned i=0;i<3;++i)if(mask&(1u<<i)) {
            if(!read(names[i],&savedInts[i],&savedFloats[i])) {++failures;continue;}
            if(savedInts[i]==0 && savedFloats[i]==0.f)continue;
            owned[i]=active=true;++writes;
            if(!write(names[i],0,0.f))++failures;
        }
    }
    bool Complete() {
        if(!active)return true;
        for(unsigned i=0;i<3;++i)if(owned[i]) {
            int n=0;float f=0;
            if(!read(names[i],&n,&f)) {++failures;continue;}
            if(n!=0 || f!=0.f) {owned[i]=false;++conflicts;continue;}
            if(!write(names[i],savedInts[i],savedFloats[i])) {++failures;continue;}
            owned[i]=false;++restores;
        }
        active=owned[0]||owned[1]||owned[2];return !active;
    }
} inline g_loadingExperiments;
inline uint8_t* g_waitSite = nullptr;
inline bool g_waitSuppressed = false, g_waitChecked = false;
inline Status g_waitStatus = Status::Off;
using SplitFn = void(__cdecl*)(const char*, char*, char*, char*);
inline SplitFn g_splitOriginal = nullptr;
inline uint8_t* g_pathSite = nullptr;
inline LONG g_pathSaved = 0, g_pathPatch = 0;
inline bool g_pathInstalled = false, g_pathChecked = false;
inline bool g_pathUsesCandidate = false;
inline void* g_pathRejectSite = nullptr;
inline void* g_loopParentReturn = nullptr;
inline void* g_loopInlineReturn = nullptr;
inline Status g_pathStatus = Status::Off;
using FinishedFn = void(__thiscall*)(void*);
inline FinishedFn g_finishedOriginal = nullptr;
inline volatile LONG* g_finishedSlot = nullptr;
inline bool g_finishedInstalled = false, g_finishedChecked = false;
inline Status g_finishedStatus = Status::Off;
inline std::atomic<DWORD> g_finishedThread{0};
using ReadFn = BOOL(WINAPI*)(HANDLE, void*, DWORD, DWORD*, OVERLAPPED*);
inline ReadFn g_readOriginal = nullptr;
inline volatile LONG* g_readSlot = nullptr;
inline void* g_vpkReadReturn = nullptr;
inline bool g_readInstalled = false, g_readChecked = false;
inline Status g_readStatus = Status::Off;
using AddStoreFn = void(__thiscall*)(void*,const char*,int);
using RemoveStoreFn = void(__thiscall*)(void*,const char*);
inline AddStoreFn g_addStore = nullptr, g_moveStore = nullptr;
inline RemoveStoreFn g_removeStore = nullptr;
struct MutationSlot { volatile LONG* slot = nullptr; LONG native = 0, hook = 0; bool installed = false; };
inline MutationSlot g_mutationSlots[6];
inline bool g_candidateChecked = false;
inline Status g_candidateStatus = Status::Off;
using FindEntryFn = void*(__thiscall*)(void*,const char*,const char*,const char*,void*,void*);
inline FindEntryFn g_candidateFind = nullptr;
inline bool g_candidateMismatch = false;
// One main-thread entry, 1040 bytes plus lengths; no resource/lookup results.
// Original input bytes are the key, so case, slashes and '..' remain native.
inline struct PathCache {
    char key[260]{}, dir[260]{}, file[260]{}, ext[260]{};
    size_t keyLen = 0, dirLen = 0, fileLen = 0, extLen = 0;
    bool valid = false;
    uint32_t directoryHash = 0;
} g_pathCache;
// Owner-only input identity for the optional exact-file miss cache. It changes
// on actual path changes, not per store, and does not retain resource nodes.
inline unsigned g_pathRequest=0;

// Fixed 512-slot map, at most 256 indexed stores; 32 bytes of directory Bloom
// bits/store. No file data, lookup winners, native nodes or handles are retained.
// Owner-only, once/store/load: <=4096 directory nodes/store, <=65536/load.
constexpr unsigned kIndexNodeBudget = 65536;
struct CandidateEntry { void* store = nullptr; unsigned epoch = 0; bool ready = false; uint32_t bits[8]{}; };
inline CandidateEntry g_candidates[512];
inline std::atomic<unsigned> g_candidateEpoch{1}, g_candidateMutating{0}, g_candidateMutations{0};
inline std::atomic<DWORD> g_candidateMutationThread{0};
inline unsigned g_indexSeenEpoch = 0, g_indexCount = 0, g_indexBudget = 0;
// One ordered directory list, <=256 packs. next[i] is the first possible pack
// at/after i, or count. Hash collisions retain native lookups. No file winners.
inline struct DirectoryList {
    void* fs = nullptr;
    void** stores = nullptr;
    unsigned count = 0, epoch = 0;
    uint32_t hash = 0;
    bool valid = false;
    uint16_t next[257]{};
} g_directoryList;
inline uint32_t CandidateHash(const char* dir, const char* ext) {
    uint32_t h = 2166136261u;
    for (; *ext; ++ext) h = (h ^ uint8_t(*ext)) * 16777619u;
    h = (h ^ 0xffu) * 16777619u;
    for (; *dir; ++dir) h = (h ^ uint8_t(*dir)) * 16777619u;
    return h;
}
inline void ResetCandidates() {
    g_candidateEpoch.fetch_add(1,std::memory_order_acq_rel);
    g_indexSeenEpoch = 0; g_indexCount = 0; g_indexBudget = 0;
    g_directoryList.valid = false;
    skvpkexp::g_controller.Reset();
}
inline void CandidateMutationBegin() {
    g_candidateMutating.fetch_add(1,std::memory_order_acq_rel);
    g_candidateEpoch.fetch_add(1,std::memory_order_acq_rel);
    g_candidateMutations.fetch_add(1,std::memory_order_relaxed);
    g_candidateMutationThread.store(GetCurrentThreadId(),std::memory_order_relaxed);
}
inline void CandidateMutationEnd() {
    g_candidateEpoch.fetch_add(1,std::memory_order_acq_rel);
    g_candidateMutating.fetch_sub(1,std::memory_order_release);
}
inline void __fastcall AddStore(void* self,void*,const char* path,int where) {
    CandidateMutationBegin();g_addStore(self,path,where);CandidateMutationEnd();
}
inline void __fastcall RemoveStore(void* self,void*,const char* path) {
    CandidateMutationBegin();g_removeStore(self,path);CandidateMutationEnd();
}
inline void __fastcall MoveStore(void* self,void*,const char* path,int where) {
    CandidateMutationBegin();g_moveStore(self,path,where);CandidateMutationEnd();
}
inline bool PatchMutation(MutationSlot& s,bool attach) {
    if (s.installed == attach) return true;
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(const_cast<LONG*>(s.slot),4,PAGE_READWRITE,&old)) { g_candidateStatus = Status::PatchFailed;return false; }
    const LONG before = InterlockedCompareExchange(s.slot,attach ? s.hook : s.native,attach ? s.native : s.hook);
    const bool wrote = before == (attach ? s.native : s.hook);
    if (wrote) s.installed = attach;
    const bool protectedAgain = VirtualProtect(const_cast<LONG*>(s.slot),4,old,&ignored) != FALSE;
    if (!wrote) g_candidateStatus = Status::Conflict;
    else if (!protectedAgain) g_candidateStatus = Status::PatchFailed;
    return wrote && protectedAgain;
}
inline bool DetachCandidates() {
    bool off = true;
    for (auto& s : g_mutationSlots) if (s.installed && !PatchMutation(s,false)) off = false;
    if (off) g_candidateStatus = Status::Off;
    return off;
}
inline void SetCandidateBits(CandidateEntry& e, uint32_t h) {
    const unsigned a = h & 255u, b = ((h >> 16) ^ (h * 0x85ebca6bu)) & 255u;
    e.bits[a >> 5] |= 1u << (a & 31); e.bits[b >> 5] |= 1u << (b & 31);
}
inline bool BuildCandidate(CandidateEntry& e) {
    uintptr_t roots[15]{};
    if (!skmem::Copy(roots,static_cast<uint8_t*>(e.store) + 0x22c,sizeof(roots))) return false;
    struct Extension { uintptr_t next; const char* name; uintptr_t dirs[43]; };
    struct Directory { uintptr_t next; const char* name; };
    unsigned extensions = 0, nodes = 0;
    char ext[260], dir[260];
    for (uintptr_t root : roots) {
        for (uintptr_t p = root; p;) {
            Extension node{};
            if (++extensions > 128 || !skmem::Copy(&node,reinterpret_cast<void*>(p),sizeof(node))
                || !skmem::CopyCStr(ext,sizeof(ext),node.name)) return false;
            for (uintptr_t head : node.dirs) {
                for (uintptr_t d = head; d;) {
                    Directory item{};
                    if (++nodes > 4096 || g_indexBudget >= kIndexNodeBudget) return false;
                    ++g_indexBudget; ++g_stats.indexNodes;
                    if (!skmem::Copy(&item,reinterpret_cast<void*>(d),sizeof(item))
                        || !skmem::CopyCStr(dir,sizeof(dir),item.name)) return false;
                    SetCandidateBits(e,CandidateHash(dir,ext));d = item.next;
                }
            }
            p = node.next;
        }
    }
    return true;
}
inline bool MayContain(void* store, uint32_t hash) {
    ++g_stats.indexQueries;
    const unsigned epoch = g_candidateEpoch.load(std::memory_order_acquire);
    if (g_candidateMutating.load(std::memory_order_acquire)) return true;
    if (epoch != g_indexSeenEpoch) { g_indexSeenEpoch = epoch; g_indexCount = 0; }
    unsigned at = (((uint32_t(uintptr_t(store)) >> 4) * 2654435761u) >> 23);
    CandidateEntry* entry = nullptr;
    for (unsigned i = 0; i < 512; ++i, at = (at + 1) & 511) {
        auto& e = g_candidates[at];
        if (e.epoch == epoch && e.store == store) { entry = &e; break; }
        if (e.epoch == epoch) continue;
        if (g_indexCount == 256) { ++g_stats.indexBypasses; return true; }
        ++g_indexCount;e = {};e.store = store;e.epoch = epoch;
        LARGE_INTEGER a{}, b{};QueryPerformanceCounter(&a);
        e.ready = BuildCandidate(e);
        QueryPerformanceCounter(&b);g_stats.indexTicks += uint64_t(b.QuadPart - a.QuadPart);++g_stats.indexBuilds;
        if (!e.ready) ++g_stats.indexBypasses;
        entry = &e;break;
    }
    if (!entry || !entry->ready || epoch != g_candidateEpoch.load(std::memory_order_acquire)
        || g_candidateMutating.load(std::memory_order_acquire)) return true;
    const unsigned a = hash & 255u, b = ((hash >> 16) ^ (hash * 0x85ebca6bu)) & 255u;
    const bool maybe = (entry->bits[a >> 5] & (1u << (a & 31))) && (entry->bits[b >> 5] & (1u << (b & 31)));
    if (!maybe) ++g_stats.indexRejected;
    return maybe;
}

__declspec(noinline) inline void BuildDirectoryList(void* fs,void** stores,unsigned count,unsigned epoch,
                                                  uint32_t hash,unsigned at,void* store) {
    auto& list = g_directoryList;
    list.valid = false;list.fs = fs;list.stores = stores;list.count = count;list.epoch = epoch;list.hash = hash;
    // A native Add may replace the borrowed array. Read it once at use, never
    // retain it for dereferencing. Keep this 1KiB frame off the hot cache hit.
    void* snapshot[256];
    if (!skmem::Copy(snapshot,stores,count * sizeof(void*)) || snapshot[at] != store
        || epoch != g_candidateEpoch.load(std::memory_order_acquire)
        || g_candidateMutating.load(std::memory_order_acquire)) { ++g_stats.indexBypasses;return; }
    list.next[count] = uint16_t(count);
    for (unsigned i = count;i--;) list.next[i] = MayContain(snapshot[i],hash) ? uint16_t(i) : list.next[i + 1];
    ++g_stats.candidateLists;
    if (epoch == g_candidateEpoch.load(std::memory_order_acquire) && !g_candidateMutating.load(std::memory_order_acquire)) list.valid = true;
}
inline unsigned DirectoryNextCandidate(void* fs,unsigned at,void* store,uint32_t hash,
                              const char* dir,const char* file,const char* ext) {
    // fs and at come from an admitted, currently executing native loop frame.
    // No module/interface queries or node probes in the cached-list path.
    const unsigned epoch = g_candidateEpoch.load(std::memory_order_acquire);
    if (g_candidateMutating.load(std::memory_order_acquire)) return at;
    struct NativeArray { void** stores; unsigned capacity,grow,count; } native{};
    if (!skmem::Copy(&native,static_cast<uint8_t*>(fs) + 0x1cc,sizeof(native))) { ++g_stats.indexBypasses;return at; }
    const unsigned count = native.count;
    if (count > 256 || at >= count) { ++g_stats.indexBypasses;return at; }
    auto** stores = native.stores;
    auto& list = g_directoryList;
    if (list.fs != fs || list.stores != stores || list.count != count || list.epoch != epoch || list.hash != hash)
        BuildDirectoryList(fs,stores,count,epoch,hash,at,store);
    // A failed build is tried only once/key/epoch. In-flight mutations are
    // reconsidered through their next epoch, never a per-query exception loop.
    if (!list.valid || epoch != g_candidateEpoch.load(std::memory_order_acquire)
        || g_candidateMutating.load(std::memory_order_acquire)) return at;
    const unsigned next = list.next[at];
    if (next == at) return at;
    const uint64_t before = g_stats.loopRejected + g_stats.loopSkipped;
    const uint64_t after = before + next - at;
    // Check one actual excluded pack whenever this run crosses a 4096 boundary.
    if ((g_mode.load(std::memory_order_relaxed) & 2u) && (before >> 12) != (after >> 12)) {
        const unsigned sample = at + unsigned(4096 - (before & 4095u)) - 1;
        void* sampleStore = nullptr;
        if (!skmem::Copy(&sampleStore,stores + sample,sizeof(sampleStore))
            || epoch != g_candidateEpoch.load(std::memory_order_acquire)
            || g_candidateMutating.load(std::memory_order_acquire)) { ++g_stats.indexBypasses;return at; }
        ++g_stats.indexVerified;
        if (g_candidateFind(sampleStore,dir,file,ext,nullptr,nullptr)) {
            ++g_stats.indexMismatches;g_candidateMismatch = true;return at;
        }
    }
    ++g_stats.loopRejected;g_stats.loopSkipped += next - at - 1;
    return next;
}

// Optional experiments retain the native loop and original order. They also
// operate when a native list is above the stable directory filter's 256 limit.
inline unsigned NextCandidate(void* fs,unsigned at,void* store,uint32_t hash,
                              const char* dir,const char* file,const char* ext) {
    auto& extra=skvpkexp::g_controller;
    if(extra.enabled)extra.ObserveBsp(dir,file,ext);
    const unsigned next=DirectoryNextCandidate(fs,at,store,hash,dir,file,ext);
    if(!extra.enabled||extra.inside)return next;
    const unsigned epoch=g_candidateEpoch.load(std::memory_order_acquire);
    struct NativeArray {void** stores;unsigned capacity,grow,count;} list{};
    if(g_candidateMutating.load(std::memory_order_acquire)||!skmem::Copy(&list,(uint8_t*)fs+0x1cc,sizeof(list)))return at;
    if(next>=list.count)return next;
    void* candidate=store;
    if(next!=at&&!skmem::Copy(&candidate,list.stores+next,sizeof(candidate)))return at;
    extra.find=g_candidateFind;
    const unsigned result=extra.Next(list.stores,list.count,next,candidate,epoch,dir,file,ext,g_pathRequest);
    return epoch==g_candidateEpoch.load(std::memory_order_acquire)&&!g_candidateMutating.load(std::memory_order_acquire)?result:at;
}

// Diagnostic-only VPK payload reads, on the owner thread and within a load.
// All other calls are native passthrough. Two QPC reads per 64 measured reads;
// exact call/byte/size counters, sampled latency is NOT a total or physical IO.
inline BOOL ReadAt(void* caller, HANDLE handle, void* buffer, DWORD size, DWORD* done, OVERLAPPED* overlapped) {
    if (!(g_mode.load(std::memory_order_relaxed) & 2u) || !g_loading.load(std::memory_order_relaxed)
        || caller != g_vpkReadReturn || GetCurrentThreadId() != g_ownerThread.load(std::memory_order_relaxed))
        return g_readOriginal(handle,buffer,size,done,overlapped);
    ++g_stats.readCalls;
    if (size <= 4096) ++g_stats.readSmall;
    const bool sample = (g_stats.readCalls & 63u) == 1;
    LARGE_INTEGER a{}, b{};
    if (sample) QueryPerformanceCounter(&a);
    const BOOL ok = g_readOriginal(handle,buffer,size,done,overlapped);
    if (sample) {
        const DWORD error = GetLastError();
        QueryPerformanceCounter(&b);
        g_stats.readTicks += uint64_t(b.QuadPart - a.QuadPart); ++g_stats.readSamples;
        SetLastError(error);
    }
    // Verified caller always supplies a synchronous byte-count pointer.
    if (ok) g_stats.readBytes += *done; else ++g_stats.readFailures;
    return ok;
}
inline BOOL WINAPI ReadFileHook(HANDLE h, void* b, DWORD n, DWORD* done, OVERLAPPED* ov) {
    return ReadAt(_ReturnAddress(),h,b,n,done,ov);
}
inline bool PatchRead(volatile LONG* slot, ReadFn original, bool attach) {
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(const_cast<LONG*>(slot),4,PAGE_READWRITE,&old)) { g_readStatus = Status::PatchFailed; return false; }
    const LONG native = LONG(uintptr_t(original)), hook = LONG(uintptr_t(&ReadFileHook));
    if (attach) { g_readOriginal = original; g_readSlot = slot; }
    const LONG before = InterlockedCompareExchange(slot,attach ? hook : native,attach ? native : hook);
    const bool wrote = before == (attach ? native : hook);
    if (wrote) g_readInstalled = attach;
    const bool protectedAgain = VirtualProtect(const_cast<LONG*>(slot),4,old,&ignored) != FALSE;
    g_readStatus = !wrote ? Status::Conflict : !protectedAgain ? Status::PatchFailed : attach ? Status::Ready : Status::Off;
    return wrote && protectedAgain;
}
inline bool DetachRead() { return !g_readInstalled || PatchRead(g_readSlot,g_readOriginal,false); }

inline bool SuppressWait(bool want) {
    if (want == g_waitSuppressed) return true;
    if (!g_waitSite) return false;
    uint8_t current = 0;
    if (!skmem::Copy(&current, g_waitSite, 1) || current != (g_waitSuppressed ? 0xeb : 0x7e)) {
        g_waitStatus = Status::Conflict;
        return false;
    }
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(g_waitSite, 1, PAGE_EXECUTE_READWRITE, &old)) {
        g_waitStatus = Status::PatchFailed; return false;
    }
    const uint8_t opcode = want ? 0xeb : 0x7e;
    const bool wrote = skmem::Write(g_waitSite, &opcode, 1);
    const bool protectedAgain = VirtualProtect(g_waitSite, 1, old, &ignored) != FALSE;
    FlushInstructionCache(GetCurrentProcess(), g_waitSite, 1);
    if (wrote) { g_waitSuppressed = want; if (GetCurrentThreadId() == g_ownerThread) ++g_stats.waitEdges; }
    g_waitStatus = wrote && protectedAgain ? Status::Ready : Status::PatchFailed;
    return wrote && protectedAgain;
}
inline void SetLoading(bool loading) {
    if (loading != g_loading.load(std::memory_order_relaxed)) {
        if (((g_mode.load(std::memory_order_relaxed)&(32u|0x700u|0x1800u)) || g_nativePreload.active || g_loadingExperiments.active) && g_loadingEdge) g_loadingEdge(loading);
        g_pathCache.valid = false;
        if (g_mode.load(std::memory_order_relaxed) & 16u) ResetCandidates();
    }
    g_loading.store(loading, std::memory_order_relaxed);
    const bool want = loading && (g_mode.load(std::memory_order_relaxed) & 4u);
    // Fail once until an explicit mode change; no per-frame protection retries.
    if (g_waitStatus == Status::Conflict || g_waitStatus == Status::PatchFailed) return;
    if (!want && !g_waitSuppressed) return;
    SuppressWait(want);
}

inline void __cdecl SplitPath(const char* path, char* dir, char* file, char* ext) {
    // The call site can run on filesystem workers. Only the known Paint thread
    // touches the fixed cache/counters; all others execute the native function.
    if (GetCurrentThreadId() != g_ownerThread.load(std::memory_order_relaxed) || !g_loading.load(std::memory_order_relaxed)) {
        g_splitOriginal(path, dir, file, ext); return;
    }
    const unsigned mode = g_mode.load(std::memory_order_relaxed);
    if (!(mode & 26u)) { g_splitOriginal(path, dir, file, ext); return; }
    ++g_stats.pathCalls;
    // A retail load repeats the same key millions of times. Path reuse or
    // explicit diagnostics tracks its key; VPK-only hashes native outputs.
    const bool reuse = (mode & 8u) != 0;
    const bool trackKey = reuse || (mode & (2u|0x1000u));
    const bool same = trackKey && g_pathCache.valid
        && !std::strncmp(path, g_pathCache.key, g_pathCache.keyLen + 1);
    if (same) ++g_stats.pathCandidates;
    if (same && reuse) {
        ++g_stats.pathHits;
        std::memcpy(dir, g_pathCache.dir, g_pathCache.dirLen + 1);
        std::memcpy(file, g_pathCache.file, g_pathCache.fileLen + 1);
        std::memcpy(ext, g_pathCache.ext, g_pathCache.extLen + 1);
        return;
    }
    const size_t n = trackKey ? (same ? g_pathCache.keyLen : strnlen_s(path, sizeof(g_pathCache.key))) : 0;
    if (trackKey && n == sizeof(g_pathCache.key)) {
        ++g_stats.pathBypasses;
        g_pathCache.valid = false;
        g_splitOriginal(path, dir, file, ext); return;
    }
    ++g_stats.pathNativeCalls;
    // Measured retail load exceeds ten million calls. Time just one in 4096
    // native splits; counters remain exact, sampled duration is not a total.
    const bool timing = (mode & 2u) && (g_stats.pathNativeCalls & 4095u) == 1;
    LARGE_INTEGER a{}, b{};
    if (timing) QueryPerformanceCounter(&a);
    g_splitOriginal(path, dir, file, ext);
    if(!same && (mode&0x1000u))++g_pathRequest;
    if (timing) { QueryPerformanceCounter(&b); g_stats.pathTicks += uint64_t(b.QuadPart - a.QuadPart); ++g_stats.pathSamples; }
    if (trackKey && !same) { std::memcpy(g_pathCache.key, path, n + 1); g_pathCache.keyLen = n; }
    if (mode & 8u) {
        g_pathCache.dirLen = strnlen_s(dir, 260);
        g_pathCache.fileLen = strnlen_s(file, 260);
        g_pathCache.extLen = strnlen_s(ext, 260);
        if (g_pathCache.dirLen == 260 || g_pathCache.fileLen == 260 || g_pathCache.extLen == 260) {
            ++g_stats.pathBypasses; g_pathCache.valid = false; return;
        }
        std::memcpy(g_pathCache.dir, dir, g_pathCache.dirLen + 1);
        std::memcpy(g_pathCache.file, file, g_pathCache.fileLen + 1);
        std::memcpy(g_pathCache.ext, ext, g_pathCache.extLen + 1);
        if (mode & 16u) g_pathCache.directoryHash = CandidateHash(g_pathCache.dir,g_pathCache.ext);
    } else if(mode & 16u) g_pathCache.directoryHash=CandidateHash(dir,ext);
    g_pathCache.valid = true;
}

// 0: continue natively; -1: parent-stack cursor already advanced;
// positive: inline loop's EBX should become returnValue-1 before its native INC.
inline int __cdecl CandidateSplit(const char* path, char* dir, char* file, char* ext, void* store,
                                 uintptr_t* frame = nullptr,unsigned inlineIndex = 0) {
    const unsigned mode=g_mode.load(std::memory_order_relaxed);
    if (GetCurrentThreadId() != g_ownerThread.load(std::memory_order_relaxed)
        || !g_loading.load(std::memory_order_relaxed) || !(mode & 16u)
        || g_candidateStatus != Status::Ready || g_candidateMismatch) {
        SplitPath(path,dir,file,ext);return 0;
    }
    if (!frame || (reinterpret_cast<void*>(frame[1]) != g_loopParentReturn && reinterpret_cast<void*>(frame[1]) != g_loopInlineReturn)) {
        SplitPath(path,dir,file,ext);return 0;
    }
    const bool inlineLoop = reinterpret_cast<void*>(frame[1]) == g_loopInlineReturn;
    auto* parent = reinterpret_cast<uintptr_t*>(frame[0]);
    void* fs = reinterpret_cast<void*>(inlineLoop ? frame[-0x314 / 4] : parent[-1]);
    const unsigned at = inlineLoop ? inlineIndex : unsigned(parent[2]);
    const bool same = (mode & 8u) && g_pathCache.valid && !std::strncmp(path,g_pathCache.key,g_pathCache.keyLen + 1);
    if (same) { ++g_stats.pathCalls;++g_stats.pathCandidates;++g_stats.pathHits; }
    else SplitPath(path,dir,file,ext);
    if (g_pathCache.valid) {
        const unsigned next = NextCandidate(fs,at,store,g_pathCache.directoryHash,
            same ? g_pathCache.dir : dir,same ? g_pathCache.file : file,same ? g_pathCache.ext : ext);
        if (next > at && !g_candidateMismatch) {
            if (inlineLoop) return int(next);
            parent[2] = next - 1;return -1;
        }
    }
    if (same) {
        std::memcpy(dir,g_pathCache.dir,g_pathCache.dirLen + 1);
        std::memcpy(file,g_pathCache.file,g_pathCache.fileLen + 1);
        std::memcpy(ext,g_pathCache.ext,g_pathCache.extLen + 1);
    }
    return 0;
}
// Existing retail CALL passes four cdecl args, EDI=store, ESI=output handle.
// A proven directory miss goes to the ORIGINAL miss epilogue (including cookie
// validation). Skip the caller's add esp,16 ourselves. Only the verified inline
// loop's EBX cursor advances; its outer method still restores its caller's EBX.
// The other registers and original native 9-DWORD miss contract are retained.
__declspec(naked) inline void CandidateThunk() {
    __asm {
        push ebx
        push ebp
        push edi
        push dword ptr [esp + 28]
        push dword ptr [esp + 28]
        push dword ptr [esp + 28]
        push dword ptr [esp + 28]
        call CandidateSplit
        add esp,28
        test eax,eax
        jz nativeContinue
        js nativeReject
        dec eax
        mov ebx,eax
    nativeReject:
        mov eax,g_pathRejectSite
        mov dword ptr [esp],eax
        ret 16
    nativeContinue:
        ret
    }
}

inline bool AttachPath(uint8_t* site, SplitFn original, bool candidate = false) {
    if (uintptr_t(site + 1) % alignof(LONG)) return false;
    uint8_t current[5];
    if (!skmem::Copy(current, site, 5) || current[0] != 0xe8) { g_pathStatus = Status::Conflict; return false; }
    std::memcpy(&g_pathSaved, current + 1, 4);
    if (g_pathSaved != LONG(uintptr_t(original) - uintptr_t(site + 5))) { g_pathStatus = Status::Conflict; return false; }
    const uintptr_t target = candidate ? uintptr_t(&CandidateThunk) : uintptr_t(&SplitPath);
    g_pathPatch = LONG(target - uintptr_t(site + 5));
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(site + 1, 4, PAGE_EXECUTE_READWRITE, &old)) { g_pathStatus = Status::PatchFailed; return false; }
    g_splitOriginal = original; g_pathSite = site;
    // Keep the CALL opcode. Its aligned rel32 is exchanged atomically while
    // workers may run; native target/body and argument ABI are unchanged.
    const LONG previous = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(site + 1), g_pathPatch, g_pathSaved);
    g_pathInstalled = previous == g_pathSaved;
    if (g_pathInstalled) g_pathUsesCandidate = candidate;
    const bool protectedAgain = VirtualProtect(site + 1, 4, old, &ignored) != FALSE;
    FlushInstructionCache(GetCurrentProcess(), site, 5);
    g_pathStatus = !g_pathInstalled ? Status::Conflict : protectedAgain ? Status::Ready : Status::PatchFailed;
    return g_pathInstalled && protectedAgain;
}
inline bool DetachPath() {
    if (!g_pathInstalled) return true;
    uint8_t opcode = 0;
    if (!skmem::Copy(&opcode, g_pathSite, 1) || opcode != 0xe8) { g_pathStatus = Status::Conflict; return false; }
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(g_pathSite + 1, 4, PAGE_EXECUTE_READWRITE, &old)) { g_pathStatus = Status::PatchFailed; return false; }
    const LONG previous = InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(g_pathSite + 1), g_pathSaved, g_pathPatch);
    if (previous == g_pathPatch) g_pathInstalled = false;
    const bool protectedAgain = VirtualProtect(g_pathSite + 1, 4, old, &ignored) != FALSE;
    FlushInstructionCache(GetCurrentProcess(), g_pathSite, 5);
    g_pathStatus = g_pathInstalled ? Status::Conflict : protectedAgain ? Status::Off : Status::PatchFailed;
    return !g_pathInstalled && protectedAgain;
}
// Retail VEngineVGui001 slot 22 is void(this), not the SDK's parameter list.
// UI loading-finished runs on the game thread even when minimized Paint stops.
// L4N 2.55 writes this slot at startup. l4n_loading_boost only gates the async
// ConVar writes inside that hook, so 0 still leaves left4neko.dll on the slot.
inline bool IsL4nModule(uintptr_t fn) {
    HMODULE module = nullptr;
    if (!fn || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(fn), &module) || !module)
        return false;
    char path[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(module, path, MAX_PATH);
    if (!n || n >= MAX_PATH) return false;
    const char* base = path;
    for (const char* p = path; *p; ++p)
        if (*p == '\\' || *p == '/') base = p + 1;
    return _stricmp(base, "left4neko.dll") == 0;
}
inline bool Detach();
inline void Configure(unsigned mode,void* engineVgui=nullptr);
inline void __fastcall LoadingFinished(void* self, void*) {
    const bool completing=g_nativePreload.active || g_loadingExperiments.active || g_localReadyDelay.owned;
    const bool asyncDone=g_nativePreload.Complete();
    const bool experimentsDone=g_loadingExperiments.Complete();
    if (!asyncDone || !experimentsDone) g_status=Status::PatchFailed;
    g_finishedOriginal(self);
    // With early view, visual completion can precede the server's readiness
    // decision. Keep the local countdown operand until LevelShutdown or an
    // option edge; otherwise the combined options would silently lose 3s.
    if(!g_earlyGameView.owned && !EndLocalReadyDelay())g_status=Status::PatchFailed;
    ++g_stats.finishedCalls;
    g_finishedThread.store(GetCurrentThreadId(), std::memory_order_relaxed);
    SetLoading(false);
    // A previous mode/disable edge may have failed only because the numeric
    // lease could not be restored then. Finish that request after recovery;
    // no Paint retry loop and no extra flag to strand a deferred close.
    if(!g_nativePreload.active && !g_loadingExperiments.active && !g_localReadyDelay.owned
        && ((completing && g_status==Status::PatchFailed)
            || (g_requestedMode!=g_mode.load(std::memory_order_relaxed) && (completing || g_status==Status::PatchFailed)))) {
        const unsigned requested=g_requestedMode;
        if(Detach() && requested) { g_requestedMode=~0u;Configure(requested,self); }
    }
}
inline bool AttachFinished(volatile LONG* slot, FinishedFn original) {
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(const_cast<LONG*>(slot),4,PAGE_READWRITE,&old)) { g_finishedStatus = Status::PatchFailed; return false; }
    g_finishedOriginal = original; g_finishedSlot = slot;
    const LONG before = InterlockedCompareExchange(slot,LONG(uintptr_t(&LoadingFinished)),LONG(uintptr_t(original)));
    g_finishedInstalled = before == LONG(uintptr_t(original));
    const bool protectedAgain = VirtualProtect(const_cast<LONG*>(slot),4,old,&ignored) != FALSE;
    g_finishedStatus = !g_finishedInstalled ? Status::Conflict : protectedAgain ? Status::Ready : Status::PatchFailed;
    return g_finishedInstalled && protectedAgain;
}
inline bool DetachFinished() {
    if (!g_finishedInstalled) return true;
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(const_cast<LONG*>(g_finishedSlot),4,PAGE_READWRITE,&old)) { g_finishedStatus = Status::PatchFailed; return false; }
    const LONG before = InterlockedCompareExchange(g_finishedSlot,LONG(uintptr_t(g_finishedOriginal)),LONG(uintptr_t(&LoadingFinished)));
    if (before == LONG(uintptr_t(&LoadingFinished))) g_finishedInstalled = false;
    const bool protectedAgain = VirtualProtect(const_cast<LONG*>(g_finishedSlot),4,old,&ignored) != FALSE;
    g_finishedStatus = g_finishedInstalled ? Status::Conflict : protectedAgain ? Status::Off : Status::PatchFailed;
    return !g_finishedInstalled && protectedAgain;
}
inline bool ConfigureFinished(void* object) {
    if (g_finishedInstalled) return g_finishedStatus == Status::Ready;
    if (g_finishedStatus == Status::Conflict || g_finishedStatus == Status::PatchFailed) return false;
    if (!g_finishedChecked) {
        g_finishedChecked = true;
        uintptr_t table = 0, original = 0;
        if (!object || !skmem::Copy(&table,object,4) || !table
            || !skmem::Copy(&original,reinterpret_cast<void*>(table + 22 * 4),4)) { g_finishedStatus = Status::Unsupported; return false; }
        const uint8_t prefix[]{0x53,0x56,0x8b,0xf1,0x8b,0x0d};
        const uint8_t suffix[]{0x33,0xdb,0x3b,0xcb,0x74,0x61};
        uint8_t body[16]{};
        const bool native = skmem::Copy(body,reinterpret_cast<void*>(original),sizeof(body))
            && !std::memcmp(body,prefix,sizeof(prefix))
            && !std::memcmp(body + 10,suffix,sizeof(suffix));
        if (!native && !IsL4nModule(original)) { g_finishedStatus = Status::Unsupported; return false; }
        g_finishedSlot = reinterpret_cast<volatile LONG*>(table + 22 * 4);
        g_finishedOriginal = reinterpret_cast<FinishedFn>(original);
    }
    return g_finishedSlot && AttachFinished(g_finishedSlot,g_finishedOriginal);
}

inline void ClearStats() {
    sktexturewarm::g_controller.skipped=0;
    skvpkexp::g_controller.stats={};
    g_loadingExperiments.writes=g_loadingExperiments.restores=g_loadingExperiments.conflicts=g_loadingExperiments.failures=0;
    g_stats = {};
    g_localReadyDelay.begins=g_localReadyDelay.restores=g_localReadyDelay.failures=g_localReadyDelay.conflicts=0;
    g_localReadyDelay.beginThread=g_localReadyDelay.endThread=0;
    g_earlyGameView.writes=g_earlyGameView.restores=g_earlyGameView.failures=g_earlyGameView.conflicts=0;
    g_localReadyForeignThread.store(0,std::memory_order_relaxed);
    g_pathCache.valid = false;
}

// One shared-memory clock read and fixed counter work per accepted engine
// progress update. QPC is diagnostic-only, two reads per native redraw.
inline void UpdateAt(DWORD now) {
    const unsigned mode = g_mode.load(std::memory_order_relaxed);
    if (!mode) { g_original(); return; }
    if (g_inside) { ++g_stats.reentries; return; }
    if (g_stats.completed) ClearStats();
    SetLoading(true);
    if (!g_stats.calls) g_stats.firstAt = now;
    ++g_stats.calls;
    ++g_stats.drawn;
    LARGE_INTEGER a{}, b{};
    if (mode & 2u) QueryPerformanceCounter(&a);
    g_inside = true;
    g_original();
    g_inside = false;
    if (mode & 2u) {
        QueryPerformanceCounter(&b);
        g_stats.drawTicks += uint64_t(b.QuadPart - a.QuadPart);
    }
}
inline void __cdecl ScreenUpdate() { UpdateAt(GetTickCount()); }

// Only writes one complete CALL instruction; cold, game-thread configuration.
// Never restores bytes owned by another plugin. Failed writes remain visible.
inline bool Attach(uint8_t* site, DrawFn original) {
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &old)) {
        g_status = Status::PatchFailed;
        return false;
    }
    std::memcpy(g_saved, site, 5);
    g_patch[0] = 0xe8;
    const int32_t rel = int32_t(uintptr_t(&ScreenUpdate) - uintptr_t(site + 5));
    std::memcpy(g_patch + 1, &rel, 4);
    g_original = original;
    g_site = site;
    const bool wrote = skmem::Write(site, g_patch, 5);
    const bool protectedAgain = VirtualProtect(site, 5, old, &ignored) != FALSE;
    FlushInstructionCache(GetCurrentProcess(), site, 5);
    g_installed = wrote;
    g_status = wrote && protectedAgain ? Status::Ready : Status::PatchFailed;
    return wrote && protectedAgain;
}
inline bool Detach() {
    const bool asyncDone=g_nativePreload.Complete();
    const bool experimentsDone=g_loadingExperiments.Complete();
    if (!asyncDone || !experimentsDone) { g_status=Status::PatchFailed;return false; }
    if (!EndLocalReadyDelay()) { g_status=Status::PatchFailed;return false; }
    if (g_earlyGameView.owned && !g_earlyGameView.End(reinterpret_cast<uint8_t*>(GetModuleHandleA("client.dll")))) {
        g_status=Status::PatchFailed;return false;
    }
    g_mode.store(0, std::memory_order_relaxed);
    g_loading.store(false, std::memory_order_relaxed);
    if (GetCurrentThreadId() == g_ownerThread) g_pathCache.valid = false;
    if (g_waitSuppressed) SuppressWait(false); // One explicit cleanup attempt, not a Paint retry loop.
    const bool pathsOff = DetachPath();
    const bool finishedOff = DetachFinished();
    const bool readsOff = DetachRead();
    const bool candidatesOff = DetachCandidates();
    const bool texturesOff = sktexturewarm::g_controller.Configure(false,nullptr);
    if (!g_installed) return pathsOff && finishedOff && readsOff && candidatesOff && texturesOff && !g_waitSuppressed;
    uint8_t actual[5];
    if (!skmem::Copy(actual, g_site, 5) || std::memcmp(actual, g_patch, 5)) {
        g_status = Status::Conflict;
        return false;
    }
    DWORD old = 0, ignored = 0;
    if (!VirtualProtect(g_site, 5, PAGE_EXECUTE_READWRITE, &old)) {
        g_status = Status::PatchFailed;
        return false;
    }
    const bool wrote = skmem::Write(g_site, g_saved, 5);
    const bool protectedAgain = VirtualProtect(g_site, 5, old, &ignored) != FALSE;
    FlushInstructionCache(GetCurrentProcess(), g_site, 5);
    if (wrote) g_installed = false;
    g_status = wrote && protectedAgain ? Status::Off : Status::PatchFailed;
    return wrote && protectedAgain && pathsOff && finishedOff && readsOff && candidatesOff && texturesOff && !g_waitSuppressed;
}

// This signature is the sole redraw call inside retail CEngineVGui's progress
// updater, after server loading-progress transmission and both UI updates.
// x86 engine SHA256 8a6d4680...0bf7ba86, RVA 0x21e50e, cdecl void(void).
// Find exactly one site and verify its callee ABI/body before writing. A changed
// game or pre-existing plugin patch produces Unsupported, with no fallback.
inline uint8_t* FindSite(uint8_t* code, size_t bytes) {
    const uint8_t prefix[]{0x0a,0xc3,0x74,0x05,0xe8};
    const uint8_t suffix[]{0x8b,0x45,0x08,0x89,0x87,0xac,0,0,0,0xff,0x15};
    constexpr size_t span = sizeof(prefix) + 4 + sizeof(suffix);
    uint8_t* found = nullptr;
    for (size_t i = 0; i + span <= bytes; ++i) {
        if (code[i] != prefix[0] || std::memcmp(code + i, prefix, sizeof(prefix))
            || std::memcmp(code + i + sizeof(prefix) + 4, suffix, sizeof(suffix))) continue;
        if (found) return nullptr;
        found = code + i + 4;
    }
    return found;
}
inline uint8_t* FindWaitSite(uint8_t* code, size_t bytes) {
    const uint8_t a[]{0x8b,0x40,0x30,0x85,0xc0,0x7e,0x1d,0x8b,0x0d};
    const uint8_t b[]{0x83,0xb9,0x1c,1,0,0,0,0x75,0x0e,0x8b,0x0d};
    const uint8_t c[]{0x8b,0x11,0x50,0x8b,0x42,0x7c,0xff,0xd0};
    constexpr size_t span = sizeof(a) + 4 + sizeof(b) + 4 + sizeof(c);
    uint8_t* found = nullptr;
    for (size_t i = 0; i + span <= bytes; ++i) {
        if (code[i] != a[0] || std::memcmp(code + i,a,sizeof(a))
            || std::memcmp(code + i + sizeof(a) + 4,b,sizeof(b))
            || std::memcmp(code + i + sizeof(a) + 4 + sizeof(b) + 4,c,sizeof(c))) continue;
        if (found) return nullptr;
        found = code + i + 5;
    }
    return found;
}
inline uint8_t* FindPathSite(uint8_t* code, size_t bytes) {
    const uint8_t a[]{0x8d,0x8d,0xf0,0xfc,0xff,0xff,0x51,0x50,0xe8};
    const uint8_t b[]{0x83,0xc4,0x10,0x8d,0x56,0x20,0x52,0x6a,0};
    constexpr size_t span = sizeof(a) + 4 + sizeof(b);
    uint8_t* found = nullptr;
    for (size_t i = 0; i + span <= bytes; ++i) {
        if (code[i] != a[0] || std::memcmp(code + i,a,sizeof(a))
            || std::memcmp(code + i + sizeof(a) + 4,b,sizeof(b))) continue;
        if (found) return nullptr;
        found = code + i + 8;
    }
    return found;
}
// Cold-only PE section lookup, one call per target on an explicit mode edge.
inline bool ModuleCode(const char* name, uint8_t*& image, size_t& start, size_t& size) {
    HMODULE module = GetModuleHandleA(name);
    MODULEINFO info{};
    if (!module || !GetModuleInformation(GetCurrentProcess(),module,&info,sizeof(info))) return false;
    image = static_cast<uint8_t*>(info.lpBaseOfDll);
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS32 nt{};
    if (!skmem::Copy(&dos,image,sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE
        || dos.e_lfanew < 0 || size_t(dos.e_lfanew) + sizeof(nt) > info.SizeOfImage
        || !skmem::Copy(&nt,image + dos.e_lfanew,sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE
        || nt.FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return false;
    start = nt.OptionalHeader.BaseOfCode; size = nt.OptionalHeader.SizeOfCode;
    return start < info.SizeOfImage && size <= info.SizeOfImage - start;
}
inline uint8_t* FindReadSite(uint8_t* code, size_t bytes) {
    const uint8_t a[]{0x8b,0x55,0x10,0x6a,0,0x8d,0x4d,8,0x51,0x8b,0x4d,0x0c,0x52,0x8b,0x50,4,0x51,0x52,0xff,0x15};
    const uint8_t b[]{0x8b,0x4d,8,0x01,0x4e,0x0c,0x8b,0x45,0xfc,0x8b,0x55,0xf8};
    constexpr size_t span = sizeof(a) + 4 + sizeof(b);
    uint8_t* found = nullptr;
    for (size_t i = 0; i + span <= bytes; ++i) {
        if (code[i] != a[0] || std::memcmp(code + i,a,sizeof(a))
            || std::memcmp(code + i + sizeof(a) + 4,b,sizeof(b))) continue;
        if (found) return nullptr;
        found = code + i + sizeof(a) - 2;
    }
    return found;
}
inline void ConfigureRead(bool want) {
    if (!want) { DetachRead(); return; }
    if (g_readInstalled || g_readStatus == Status::Conflict || g_readStatus == Status::PatchFailed) return;
    if (!g_readChecked) {
        g_readChecked = true;
        uint8_t* image = nullptr; size_t start = 0, size = 0;
        if (!ModuleCode("filesystem_stdio.dll",image,start,size)) { g_readStatus = Status::Unsupported; return; }
        uint8_t* site = FindReadSite(image + start,size);
        uintptr_t slot = 0, original = 0;
        MODULEINFO info{};
        if (!site || !skmem::Copy(&slot,site + 2,4) || slot % 4
            || !GetModuleInformation(GetCurrentProcess(),reinterpret_cast<HMODULE>(image),&info,sizeof(info))
            || slot < uintptr_t(image) || slot + 4 > uintptr_t(image) + info.SizeOfImage
            || !skmem::Copy(&original,reinterpret_cast<void*>(slot),4)) { g_readStatus = Status::Unsupported; return; }
        // Shared import: do not replace another plugin's wrapper.
        if (original != uintptr_t(GetProcAddress(GetModuleHandleA("kernel32.dll"),"ReadFile"))) { g_readStatus = Status::Conflict; return; }
        g_readSlot = reinterpret_cast<volatile LONG*>(slot);
        g_readOriginal = reinterpret_cast<ReadFn>(original);
        g_vpkReadReturn = site + 6;
    }
    if (g_readSlot) PatchRead(g_readSlot,g_readOriginal,true);
}
// The fixed RVAs below are admitted only with the retail caller's frame,
// original miss epilogue, mutation prologues and BOTH native vtable triples.
// x86 filesystem SHA256 d0296dda...34afdc9. No hashes or polling in hot paths.
inline bool ConfigureCandidates(bool want) {
    if (!want) return DetachCandidates();
    if (g_candidateStatus == Status::Conflict || g_candidateStatus == Status::PatchFailed) return false;
    if (!g_candidateChecked) {
        g_candidateChecked = true;
        uint8_t* image = nullptr;size_t start = 0,size = 0;MODULEINFO info{};
        if (!ModuleCode("filesystem_stdio.dll",image,start,size)
            || !GetModuleInformation(GetCurrentProcess(),reinterpret_cast<HMODULE>(image),&info,sizeof(info))
            || info.SizeOfImage < 0x48000 || g_pathSite != image + 0x2daf3) { g_candidateStatus = Status::Unsupported;return false; }
        const uint8_t frame[]{0x55,0x8b,0xec,0x81,0xec,0x10,3,0,0};
        const uint8_t registers[]{0x8b,0x45,0x0c,0x56,0x8b,0x75,8,0x57,0x8b,0xf9};
        const uint8_t reject[]{0x8b,0x4d,0xfc,0x5f,0xc7,6,0xff,0xff,0xff,0xff,0xc7,0x46,0x18,0,0,0,0,0x8b,0xc6,0x33,0xcd,0x5e,0xe8};
        const uint8_t tail[]{0x8b,0xe5,0x5d,0xc2,8,0};
        const uint8_t addHead[]{0x55,0x8b,0xec,0x81,0xec,0x0c,1,0,0};
        const uint8_t removeHead[]{0x55,0x8b,0xec,0x81,0xec,8,1,0,0};
        const uint8_t branch[]{0x85,0xc0,0x74,0x6e};
        const uint8_t findHead[]{0x55,0x8b,0xec,0x53,0x8b,0x5d,0x14,0x56,0x57,0x8b,0xf9};
        const uint8_t parentHead[]{0x55,0x8b,0xec,0x83,0xec,0x28,0x83,0xb9,0xd8,1,0,0,0,0x53,0x8b,0x5d,8,0x89,0x4d,0xfc};
        const uint8_t parentTail[]{0x83,0x3b,0xff,0x75,0x12,0x8b,0x45,8,0x8b,0x55,0xfc,0x40,0x89,0x45,8,0x3b,0x82,0xd8,1,0,0,0x7c,0xc1};
        const uint8_t inlineHead[]{0x8b,0x8e,0xcc,1,0,0,0x8b,0x0c,0x99,0x57,0x8d,0x85,0xac,0xfd,0xff,0xff,0x50};
        const uint8_t inlineTail[]{0x83,0xbd,0xac,0xfd,0xff,0xff,0xff,0x75,0x2e,0x43,0x3b,0x9e,0xd8,1,0,0,0x7c,0xd8};
        if (std::memcmp(image + 0x2dac0,frame,sizeof(frame)) || std::memcmp(image + 0x2dad3,registers,sizeof(registers))
            || std::memcmp(image + 0x2db23,branch,sizeof(branch)) || std::memcmp(image + 0x2db95,reject,sizeof(reject))
            || std::memcmp(image + 0x2dbb0,tail,sizeof(tail)) || std::memcmp(image + 0xa110,addHead,sizeof(addHead))
            || std::memcmp(image + 0x68b0,removeHead,sizeof(removeHead)) || std::memcmp(image + 0xa2f0,addHead,sizeof(addHead))
            || std::memcmp(image + 0x2d9e0,findHead,sizeof(findHead))
            || std::memcmp(image + 0x4a70,parentHead,sizeof(parentHead)) || std::memcmp(image + 0x4ac8,parentTail,sizeof(parentTail))
            || std::memcmp(image + 0xb0a0,inlineHead,sizeof(inlineHead)) || std::memcmp(image + 0xb0b6,inlineTail,sizeof(inlineTail))) {
            g_candidateStatus = Status::Unsupported;return false;
        }
        const size_t loopCalls[]{0x4ab8,0xb0b1};
        for (const size_t call : loopCalls) {
            int32_t rel = 0;std::memcpy(&rel,image + call + 1,4);
            if (image[call] != 0xe8 || image + call + 5 + rel != image + 0x2dac0) { g_candidateStatus = Status::Conflict;return false; }
        }
        const uintptr_t natives[]{uintptr_t(image + 0xa110),uintptr_t(image + 0x68b0),uintptr_t(image + 0xa2f0)};
        const uintptr_t hooks[]{uintptr_t(&AddStore),uintptr_t(&RemoveStore),uintptr_t(&MoveStore)};
        const size_t tables[]{0x47818,0x47ff0};
        for (unsigned t = 0;t < 2;++t) for (unsigned n = 0;n < 3;++n) {
            auto* slot = reinterpret_cast<volatile LONG*>(image + tables[t] + n * 4);
            if (*slot != LONG(natives[n])) { g_candidateStatus = Status::Conflict;return false; }
            g_mutationSlots[t * 3 + n] = {slot,LONG(natives[n]),LONG(hooks[n]),false};
        }
        g_addStore = reinterpret_cast<AddStoreFn>(natives[0]);g_removeStore = reinterpret_cast<RemoveStoreFn>(natives[1]);
        g_moveStore = reinterpret_cast<AddStoreFn>(natives[2]);g_pathRejectSite = image + 0x2db95;
        g_candidateFind = reinterpret_cast<FindEntryFn>(image + 0x2d9e0);
        g_loopParentReturn = image + 0x4abd;g_loopInlineReturn = image + 0xb0b6;
    }
    if (!g_pathRejectSite) return false;
    for (auto& s : g_mutationSlots) if (!PatchMutation(s,true)) {
        const auto failure = g_candidateStatus;DetachCandidates();g_candidateStatus = failure;return false;
    }
    g_candidateStatus = Status::Ready;return true;
}
inline void ConfigureExperiments(unsigned mode) {
    sktexturewarm::g_controller.Configure((mode&0x2000u)!=0,reinterpret_cast<void*>(&TextureWarmup));
    if (!g_ownerThread) g_ownerThread = GetCurrentThreadId();
    ConfigureRead((mode & 2u) != 0);
    if ((mode & 4u) && !g_waitChecked) {
        g_waitChecked = true;
        uint8_t* image = nullptr; size_t start = 0, size = 0;
        if (ModuleCode("engine.dll",image,start,size)) g_waitSite = FindWaitSite(image + start,size);
        g_waitStatus = g_waitSite ? Status::Ready : Status::Unsupported;
    }
    const bool wantPath = (mode & 26u) != 0;
    if (!wantPath || !(mode & 16u)) ConfigureCandidates(false);
    if (!wantPath) { DetachPath();return; }
    if (g_pathInstalled && g_pathUsesCandidate != ((mode & 16u) != 0)) DetachPath();
    if (g_pathStatus == Status::Conflict || g_pathStatus == Status::PatchFailed) { ConfigureCandidates(false);return; }
    if (!g_pathChecked) {
        g_pathChecked = true;
        uint8_t* image = nullptr; size_t start = 0, size = 0;
        if (!ModuleCode("filesystem_stdio.dll",image,start,size)) { g_pathStatus = Status::Unsupported; return; }
        uint8_t* site = FindPathSite(image + start,size);
        if (!site) { g_pathStatus = Status::Unsupported; return; }
        int32_t rel = 0; std::memcpy(&rel,site + 1,4);
        const uintptr_t target = uintptr_t(site + 5) + rel;
        const uint8_t nativePrefix[]{0x55,0x8b,0xec,0x56,0x8b,0x75,0x0c,0x57,0x8b,0x7d,0x08,0x68,0x04,1,0,0,0x56,0x57,0xe8};
        uint8_t body[sizeof(nativePrefix)]{};
        if (target < uintptr_t(image + start) || target + sizeof(body) > uintptr_t(image + start + size)
            || !skmem::Copy(body,reinterpret_cast<void*>(target),sizeof(body)) || std::memcmp(body,nativePrefix,sizeof(body))) {
            g_pathStatus = Status::Unsupported; return;
        }
        g_pathSite = site; g_splitOriginal = reinterpret_cast<SplitFn>(target);
    }
    const bool candidate = (mode & 16u) ? ConfigureCandidates(true) : (ConfigureCandidates(false),false);
    if (g_pathInstalled && g_pathUsesCandidate != candidate && !DetachPath()) { ConfigureCandidates(false);return; }
    if (g_pathSite && !g_pathInstalled && !AttachPath(g_pathSite,g_splitOriginal,candidate)) {
        const auto failure = g_pathStatus;DetachPath();ConfigureCandidates(false);g_pathStatus = failure;
    }
}
inline bool Resolve() {
    HMODULE engine = GetModuleHandleA("engine.dll");
    MODULEINFO info{};
    if (!engine || !GetModuleInformation(GetCurrentProcess(), engine, &info, sizeof(info))) return false;
    auto* image = static_cast<uint8_t*>(info.lpBaseOfDll);
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS32 nt{};
    if (!skmem::Copy(&dos, image, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE
        || dos.e_lfanew < 0 || size_t(dos.e_lfanew) + sizeof(nt) > info.SizeOfImage
        || !skmem::Copy(&nt, image + dos.e_lfanew, sizeof(nt))
        || nt.Signature != IMAGE_NT_SIGNATURE || nt.FileHeader.Machine != IMAGE_FILE_MACHINE_I386
        || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return false;
    const size_t start = nt.OptionalHeader.BaseOfCode, size = nt.OptionalHeader.SizeOfCode;
    if (start >= info.SizeOfImage || size > info.SizeOfImage - start) return false;
    uint8_t* site = FindSite(image + start, size);
    if (!site) return false;
    int32_t rel = 0;
    std::memcpy(&rel, site + 1, 4);
    const uintptr_t target = uintptr_t(site + 5) + rel;
    if (target < uintptr_t(image + start) || target + 26 > uintptr_t(image + start + size)) return false;
    uint8_t body[26]{};
    if (!skmem::Copy(body, reinterpret_cast<void*>(target), sizeof(body))) return false;
    const uint8_t head[]{0x8b,0x0d}, middle[]{0xd9,0x05};
    const uint8_t tail[]{0x8b,0x01,0x8b,0x90,0x90,0,0,0,0x51,0xd9,0x1c,0x24,0xff,0xd2};
    if (std::memcmp(body, head, sizeof(head)) || std::memcmp(body + 6, middle, sizeof(middle))
        || std::memcmp(body + 12, tail, sizeof(tail))) return false;
    return Attach(site, reinterpret_cast<DrawFn>(target));
}

// Configure is normally a cached mode comparison. Signature scanning and page
// protection changes occur only on an explicit option edge, never per frame.
inline void Configure(unsigned mode, void* engineVgui) {
    if (mode == g_requestedMode) return;
    g_requestedMode = mode;
    if (!mode) { Detach(); return; }
    const bool completing=g_nativePreload.active || g_loadingExperiments.active || g_localReadyDelay.owned;
    const bool asyncDone=g_nativePreload.Complete();
    const bool experimentsDone=g_loadingExperiments.Complete();
    if (!asyncDone || !experimentsDone) { g_status=Status::PatchFailed;return; }
    // An unrelated option edge must not cancel this level's selected countdown
    // lease. Level shutdown/native completion still own its lifetime boundary.
    if (!(mode&64u) && !EndLocalReadyDelay()) { g_status=Status::PatchFailed;return; }
    if(completing && g_status==Status::PatchFailed && !Detach())return;
    if (!g_ownerThread) g_ownerThread = GetCurrentThreadId();
    if (!ConfigureFinished(engineVgui)) { g_status = g_finishedStatus; return; }
    if (g_status == Status::Conflict || g_status == Status::PatchFailed) return;
    if (!g_installed) {
        if (g_checked) {
            if (!g_site) return;
            uint8_t current[5];
            if (!skmem::Copy(current, g_site, 5) || std::memcmp(current, g_saved, 5)) {
                g_status = Status::Conflict;
                return;
            }
            if (!Attach(g_site, g_original)) return;
        } else {
            g_checked = true;
            if (!Resolve()) { if (g_status != Status::PatchFailed) g_status = Status::Unsupported; return; }
        }
    }
    if (mode & 50u) QueryPerformanceFrequency(&g_frequency);
    SetLoading(false);
    ConfigureExperiments(mode);
    ClearStats();
    if(mode&128u) {
        if(!g_earlyGameView.owned) {
            HMODULE module=GetModuleHandleA("client.dll");MODULEINFO info{};
            using Factory=void*(__cdecl*)(const char*,int*);
            const auto factory=module?reinterpret_cast<Factory>(GetProcAddress(module,"CreateInterface")):nullptr;
            // Resolve on this explicit edge, including native-finish recovery;
            // never cache an interface across unload or poll it from Paint.
            if(factory && GetModuleInformation(GetCurrentProcess(),module,&info,sizeof(info)))
                g_earlyGameView.Begin(static_cast<uint8_t*>(info.lpBaseOfDll),info.SizeOfImage,factory("VClient016",nullptr));
            else {++g_earlyGameView.failures;g_earlyGameView.status=Status::Unsupported;}
        }
    } else if(g_earlyGameView.owned && !g_earlyGameView.End(reinterpret_cast<uint8_t*>(GetModuleHandleA("client.dll")))) {
        g_status=Status::PatchFailed;return; // Keep the old mode so native finish can retry this pending edge.
    }
    ResetCandidates();
    g_mode.store(mode, std::memory_order_relaxed);
}
inline void Observe(bool loading, bool ingame, DWORD now) {
    SetLoading(loading);
    if (!g_inside && g_stats.calls && !g_stats.completed && !loading && ingame) {
        g_stats.progressToPaintMs = now - g_stats.firstAt;
        g_stats.completed = true;
    }
}
inline double DrawMilliseconds() {
    return g_frequency.QuadPart ? 1000.0 * double(g_stats.drawTicks) / double(g_frequency.QuadPart) : 0.0;
}
inline double PathMilliseconds() {
    return g_frequency.QuadPart ? 1000.0 * double(g_stats.pathTicks) / double(g_frequency.QuadPart) : 0.0;
}
inline double ReadMilliseconds() {
    return g_frequency.QuadPart ? 1000.0 * double(g_stats.readTicks) / double(g_frequency.QuadPart) : 0.0;
}

} // namespace skload
