#pragma once
#include "skeeto_memread.h"
#include <psapi.h>
#include <cstdint>
#include <cstring>

// One native TextureManager bulk warm-up call, not a per-texture/render hook.
// The owner/loading wrapper decides whether to omit this optional pass. Native
// texture reads, allocation, handles and ordinary rendering remain untouched.
namespace sktexturewarm {
enum class Status { Off, Ready, Unsupported, Conflict, PatchFailed };
struct Controller {
    using Native = void(__thiscall*)(void*);
    uint8_t* image=nullptr;
    volatile LONG* slot=nullptr;
    LONG native=0,hook=0;
    bool installed=false;
    Status status=Status::Off;
    uint64_t skipped=0;
    unsigned failures=0;
    void Run(void* self,bool omit) {
        if(omit){++skipped;return;} // Only the admitted owner thread writes it.
        reinterpret_cast<Native>(uintptr_t(native))(self);
    }
    bool Patch(bool attach) {
        DWORD old=0,ignored=0;
        if(!VirtualProtect(const_cast<LONG*>(slot),4,PAGE_READWRITE,&old)){
            ++failures;status=Status::PatchFailed;return false;
        }
        const LONG before=InterlockedCompareExchange(slot,attach?hook:native,attach?native:hook);
        const bool wrote=before==(attach?native:hook);
        if(wrote)installed=attach;
        const bool protectedAgain=VirtualProtect(const_cast<LONG*>(slot),4,old,&ignored)!=FALSE;
        if(!wrote || !protectedAgain)++failures;
        status=!wrote?Status::Conflict:!protectedAgain?Status::PatchFailed:attach?Status::Ready:Status::Off;
        return wrote && protectedAgain;
    }
    bool Configure(bool want,void* replacement) {
        if(!want){
            if(!installed){status=Status::Off;return true;}
            if(GetModuleHandleA("materialsystem.dll")!=reinterpret_cast<HMODULE>(image)){
                installed=false;status=Status::Unsupported;return true; // No late module writes.
            }
            return Patch(false);
        }
        if(installed)return status==Status::Ready;
        auto* module=reinterpret_cast<uint8_t*>(GetModuleHandleA("materialsystem.dll"));
        MODULEINFO info{};uintptr_t object=0,table=0,target=0;
        const uint8_t expected[]{0x55,0x8b,0xec,0x83,0xec,8,0x53,0x57,0x6a,0,0x8b,0xd9};
        uint8_t actual[sizeof(expected)]{};
        // Retail x86 material SHA 09fd324c...fcf0ba. Match both the concrete
        // TextureManager instance/vtable and its void thiscall entry. Cold only.
        if(!module || !GetModuleInformation(GetCurrentProcess(),reinterpret_cast<HMODULE>(module),&info,sizeof(info))
            || info.SizeOfImage<0x11134c || !skmem::Copy(&object,module+0xf8588,4)
            || object!=uintptr_t(module+0x111348) || !skmem::Copy(&table,reinterpret_cast<void*>(object),4)
            || table!=uintptr_t(module+0xa129c) || !skmem::Copy(&target,reinterpret_cast<void*>(table+0x70),4)){
            ++failures;status=Status::Unsupported;return false;
        }
        if(target!=uintptr_t(module+0x489f0)) {++failures;status=Status::Conflict;return false;}
        if(!skmem::Copy(actual,reinterpret_cast<void*>(target),sizeof(actual)) || std::memcmp(actual,expected,sizeof(actual))){
            ++failures;status=Status::Unsupported;return false;
        }
        image=module;slot=reinterpret_cast<volatile LONG*>(table+0x70);
        native=LONG(target);hook=LONG(uintptr_t(replacement));return Patch(true);
    }
};
inline Controller g_controller;
}
