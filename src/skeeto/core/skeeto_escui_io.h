#pragma once
#include <Windows.h>
#include <ShlObj.h>
#include <cwchar>
#include "skeeto_escui_layout.h"

// Existing worker only. Bounded file buffer; missing settings create nothing.
namespace skescio {
inline char buffer[skesc::kFileBytes]{};
inline int Load(const wchar_t* path,skesc::Layout& out){
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND||GetLastError()==ERROR_PATH_NOT_FOUND?0:-1;
    LARGE_INTEGER size{};DWORD read=0;
    bool ok=GetFileSizeEx(file,&size)&&size.QuadPart>0&&size.QuadPart<skesc::kFileBytes
        &&ReadFile(file,buffer,(DWORD)size.QuadPart,&read,nullptr)&&read==size.QuadPart;
    CloseHandle(file);if(!ok)return -1;return skesc::Parse(buffer,read,out)?1:-1;
}
inline bool Save(const wchar_t* path,const skesc::Layout& l){
    size_t bytes=skesc::Format(l,buffer,sizeof(buffer));if(!bytes)return false;
    wchar_t parent[1024]{},temporary[1024]{};size_t n=std::wcslen(path);if(n+5>_countof(temporary))return false;
    std::memcpy(parent,path,(n+1)*sizeof(wchar_t));wchar_t* slash=std::wcsrchr(parent,L'\\');if(!slash)return false;*slash=0;
    int directory=SHCreateDirectoryExW(nullptr,parent,nullptr);
    if(directory!=ERROR_SUCCESS&&directory!=ERROR_ALREADY_EXISTS&&directory!=ERROR_FILE_EXISTS)return false;
    std::memcpy(temporary,path,n*sizeof(wchar_t));std::memcpy(temporary+n,L".tmp",5*sizeof(wchar_t));
    HANDLE file=CreateFileW(temporary,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;bool ok=WriteFile(file,buffer,(DWORD)bytes,&written,nullptr)&&written==bytes&&FlushFileBuffers(file);
    if(!CloseHandle(file))ok=false;
    if(ok)ok=MoveFileExW(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
    if(!ok)DeleteFileW(temporary);return ok;
}
}
