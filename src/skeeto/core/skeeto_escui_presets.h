#pragma once
#include <cmath>
// Fixed Windows/x86 document, independent of the ESC layout. Worker-only disk
// access. Save requests copy one bank; no game pointers cross the task slot.
namespace skescpreset {
constexpr int Slots=8,Entries=64;
struct Entry {char name[64]{};float value=0;int pinned=0;};
struct Preset {wchar_t name[64]{};int count=0;Entry values[Entries]{};};
struct Bank {unsigned magic=0x31564353,version=1,bytes=sizeof(Preset)*Slots;Preset slots[Slots]{};};
inline Bank bank{},job{};
inline wchar_t path[SKT_PATH_CAP]{};
inline std::atomic_int pending{0},result{0};
inline bool busy=false,readable=true;
inline bool Valid(const Bank& b){
    if(b.magic!=0x31564353||b.version!=1||b.bytes!=sizeof(Preset)*Slots)return false;
    for(const auto& p:b.slots){
        if(!wmemchr(p.name,0,64)||p.count<0||p.count>Entries)return false;
        for(int i=0;i<p.count;++i){const auto& v=p.values[i];
            if(!memchr(v.name,0,64)||!v.name[0]||!std::isfinite(v.value)||(v.pinned!=0&&v.pinned!=1))return false;
            for(const char* c=v.name;*c;++c)if(!std::isalnum((unsigned char)*c)&&*c!='_')return false;
            for(int j=0;j<i;++j)if(!_stricmp(p.values[j].name,v.name))return false;
        }
    }return true;
}
inline void Startup(const wchar_t* directory){
    if(!SktJoinPath(directory,L"cvar_presets.bin",path)){readable=false;return;}
    HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE){readable=GetLastError()==ERROR_FILE_NOT_FOUND||GetLastError()==ERROR_PATH_NOT_FOUND;return;}
    LARGE_INTEGER size{};DWORD n=0;
    readable=GetFileSizeEx(f,&size)&&size.QuadPart==sizeof(Bank)&&ReadFile(f,&job,sizeof(job),&n,nullptr)&&n==sizeof(job)&&Valid(job);
    CloseHandle(f);if(readable)bank=job;
}
inline bool Save(){
    if(busy||!readable)return false;
    job=bank;busy=true;pending.store(1,std::memory_order_release);return true;
}
inline void Worker(){
    if(!pending.exchange(0,std::memory_order_acquire))return;
    wchar_t directory[SKT_PATH_CAP]{},temporary[SKT_PATH_CAP+4]{};wcscpy_s(directory,path);
    wchar_t* slash=wcsrchr(directory,L'\\');if(!slash){result.store(-1,std::memory_order_release);return;}*slash=0;
    int made=SHCreateDirectoryExW(nullptr,directory,nullptr);bool ok=made==ERROR_SUCCESS||made==ERROR_ALREADY_EXISTS||made==ERROR_FILE_EXISTS;
    swprintf_s(temporary,L"%s.tmp",path);
    HANDLE f=ok?CreateFileW(temporary,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr):INVALID_HANDLE_VALUE;
    DWORD n=0;ok=f!=INVALID_HANDLE_VALUE&&WriteFile(f,&job,sizeof(job),&n,nullptr)&&n==sizeof(job)&&FlushFileBuffers(f);
    if(f!=INVALID_HANDLE_VALUE&&!CloseHandle(f))ok=false;
    if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temporary);result.store(ok?1:-1,std::memory_order_release);
}
}
