#include "skeeto_plugins.h"
#include "skeeto_memread.h"
#include "skeeto_runtime.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace skplugins {
namespace {
struct Record {
    Plugin view;
    HMODULE module=nullptr;
    SktPluginInfo api{};
    SktHostApi host{};
    char directory[1024]{},dataDirectory[2048]{};
};
// Zero-initialized storage; no large constructors or heap work on the off path.
Record g_records[kMaxPlugins];
Page g_page;
SktMenuItem g_buildItems[kMaxItems];
std::atomic_uint g_count{0};
std::atomic_uint g_scanError{0};
std::atomic_bool g_started{false},g_loading{false},g_limited{false},g_work{false};
unsigned g_nextInit=0;
Counters g_stats;
enum class Request { None,Page,Action };
Request g_request=Request::None;
unsigned g_requestedPlugin=0;
uint32_t g_requestedPage=0,g_requestedItem=0;
double g_requestedValue=0;

template<size_t N> bool Text(wchar_t (&out)[N],const char* text,bool required=false) {
    out[0]=0;
    if(!text)return !required;
    char bytes[N*3+1]{};
    if(!skmem::CopyCStr(bytes,sizeof(bytes),text))return false;
    const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes,-1,out,(int)N);
    return n>0 && (!required || out[0]);
}
const char* Language(const char* zh,const char* en,bool english) {
    return english?(en?en:zh):(zh?zh:en);
}
bool Identifier(char (&out)[48],const char* text) {
    // CopyCStr reserves its last byte; allow the SDK's full 47-character ID.
    char id[49]{};
    if(!text || !skmem::CopyCStr(id,sizeof(id),text) || !id[0])return false;
    for(const unsigned char* p=(const unsigned char*)id;*p;++p)
        if(!((*p>='a'&&*p<='z')||(*p>='A'&&*p<='Z')||(*p>='0'&&*p<='9')||*p=='.'||*p=='_'||*p=='-'))return false;
    // Win32 normalizes a trailing dot, which would alias another plugin's data.
    if(id[strlen(id)-1]=='.')return false;
    strcpy_s(out,id);return true;
}
bool Metadata(Record& r,const wchar_t* root,const wchar_t* directory) {
    auto& a=r.api;auto& v=r.view;
    if(a.size<SKT_PLUGIN_INFO_V1_SIZE || a.size>sizeof(SktPluginInfo) || !Identifier(v.id,a.id)
        || !Text(v.nameZh,Language(a.name_zh,a.name_en,false),true)
        || !Text(v.nameEn,Language(a.name_zh,a.name_en,true),true)
        || !Text(v.author,a.author) || !Text(v.version,a.version))return false;
    if((a.on_host_ready && !skmem::Executable((void*)a.on_host_ready))
        || (a.build_page && !skmem::Executable((void*)a.build_page))
        || (a.on_action && !skmem::Executable((void*)a.on_action)))return false;
    if(!WideCharToMultiByte(CP_UTF8,0,directory,-1,r.directory,(int)sizeof(r.directory),nullptr,nullptr))return false;
    char rootUtf8[1024]{};
    if(!WideCharToMultiByte(CP_UTF8,0,root,-1,rootUtf8,sizeof(rootUtf8),nullptr,nullptr))return false;
    if(snprintf(r.dataDirectory,sizeof(r.dataDirectory),"%s\\left4dead2\\skeeto\\plugins\\%s",rootUtf8,v.id)<0)return false;
    r.host={sizeof(SktHostApi),SKT_PLUGIN_API_VERSION,r.directory,r.dataDirectory};
    v.hasMenu=a.build_page!=nullptr;return true;
}
void Fail(Record& r,State state,uint32_t code) {
    r.view.state=state;r.view.error=code;
    g_page.valid=false;
}
bool CopyItem(Item& out,const SktMenuItem& in,bool english) {
    if(in.size!=sizeof(SktMenuItem) || !in.id || in.type>SKT_MENU_SUBMENU
        || (in.flags&~SKT_MENU_DISABLED))return false;
    out={};out.id=in.id;out.type=in.type;out.flags=in.flags;out.targetPage=in.page_id;
    if(!Text(out.label,Language(in.label_zh,in.label_en,english),true)
        || !Text(out.text,Language(in.text_zh,in.text_en,english)))return false;
    out.value=in.value;out.min=in.min_value;out.max=in.max_value;out.step=in.step;
    if(in.type==SKT_MENU_TOGGLE)return in.value==0 || in.value==1;
    if(in.type==SKT_MENU_NUMBER)return std::isfinite(in.value)&&std::isfinite(in.min_value)
        &&std::isfinite(in.max_value)&&std::isfinite(in.step)&&in.min_value<=in.max_value
        &&in.value>=in.min_value&&in.value<=in.max_value&&in.step>0;
    if(in.type==SKT_MENU_CHOICE){
        if(!Text(out.choices,Language(in.choices_zh,in.choices_en,english),true))return false;
        unsigned count=1;bool empty=true;
        for(const wchar_t* p=out.choices;*p;++p){
            if(*p==L'|'){if(empty)return false;++count;empty=true;}else empty=false;
        }
        if(empty || count>32 || !std::isfinite(in.value) || in.value<0 || in.value>=count || floor(in.value)!=in.value)return false;
        out.choice_count=count;
    }
    return true;
}
void Build(Record& r,unsigned plugin,uint32_t page,bool english) {
    g_page.valid=false;
    memset(g_buildItems,0,sizeof(g_buildItems));
    SktMenuPage native{sizeof(SktMenuPage),page,nullptr,nullptr,g_buildItems,kMaxItems,0};
    ++g_stats.builds;
    const int32_t result=r.api.build_page(r.api.context,page,&native);
    if(result){Fail(r,State::PageError,(uint32_t)result);return;}
    if(native.size!=sizeof(native) || native.page_id!=page || native.items!=g_buildItems
        || native.capacity!=kMaxItems || native.count>kMaxItems
        || !Text(g_page.title,Language(native.title_zh,native.title_en,english),true)){
        Fail(r,State::PageError,ERROR_INVALID_DATA);return;
    }
    for(unsigned i=0;i<native.count;++i){
        if(!CopyItem(g_page.items[i],native.items[i],english)){Fail(r,State::PageError,ERROR_INVALID_DATA);return;}
        for(unsigned j=0;j<i;++j)if(g_page.items[j].id==g_page.items[i].id){Fail(r,State::PageError,ERROR_DUP_NAME);return;}
        const unsigned type=g_page.items[i].type;
        if(type!=SKT_MENU_INFO && type!=SKT_MENU_SUBMENU && !r.api.on_action){Fail(r,State::PageError,ERROR_PROC_NOT_FOUND);return;}
    }
    g_page.plugin=plugin;g_page.page_id=page;g_page.count=native.count;g_page.valid=true;
}
}

void Start(const wchar_t* root,bool enabled) {
    if(!enabled)return;
    if(!g_run.load(std::memory_order_relaxed))return;
    g_started.store(true,std::memory_order_relaxed);g_loading.store(true,std::memory_order_release);
    wchar_t directory[1024]{},pattern[1050]{};
    if(swprintf_s(directory,L"%s\\bin\\skeeto\\plugins",root)<0
        || swprintf_s(pattern,L"%s\\*.dll",directory)<0){g_scanError.store(ERROR_FILENAME_EXCED_RANGE);g_loading.store(false,std::memory_order_release);return;}
    WIN32_FIND_DATAW data{};HANDLE search=FindFirstFileW(pattern,&data);
    if(search==INVALID_HANDLE_VALUE){
        const DWORD error=GetLastError();
        if(error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND)g_scanError.store(error,std::memory_order_relaxed);
        g_loading.store(false,std::memory_order_release);return;
    }
    wchar_t files[kMaxPlugins][MAX_PATH]{};unsigned count=0;
    do {
        if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)continue;
        if(count==kMaxPlugins){g_limited.store(true,std::memory_order_relaxed);break;}
        wcscpy_s(files[count++],data.cFileName);
    }while(g_run.load(std::memory_order_relaxed) && FindNextFileW(search,&data));
    const DWORD findError=GetLastError();
    if(g_run.load(std::memory_order_relaxed) && !g_limited.load(std::memory_order_relaxed) && findError!=ERROR_NO_MORE_FILES)g_scanError.store(findError,std::memory_order_relaxed);
    FindClose(search);
    // <=32 names, no runtime discovery or mount-dependent ordering.
    for(unsigned i=1;i<count;++i)for(unsigned j=i;j && _wcsicmp(files[j-1],files[j])>0;--j){
        wchar_t swap[MAX_PATH];wcscpy_s(swap,files[j-1]);wcscpy_s(files[j-1],files[j]);wcscpy_s(files[j],swap);
    }
    unsigned processed=0;
    for(unsigned i=0;i<count;++i){
        if(!g_run.load(std::memory_order_relaxed))break;
        auto& r=g_records[i];wcscpy_s(r.view.file,files[i]);wchar_t path[1300]{};
        processed=i+1;
        if(swprintf_s(path,L"%s\\%s",directory,files[i])<0){r.view.error=ERROR_FILENAME_EXCED_RANGE;continue;}
        ++g_stats.loads;
        r.module=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!r.module){r.view.error=GetLastError();continue;}
        r.view.state=State::Loaded;
        if(!g_run.load(std::memory_order_relaxed))break;
        const auto query=(SktPluginQuery)GetProcAddress(r.module,"SkeetoPlugin_Query");
        if(!query){r.view.state=State::EntryMissing;r.view.error=ERROR_PROC_NOT_FOUND;continue;}
        r.api.size=sizeof(r.api);
        const int32_t result=query(SKT_PLUGIN_API_VERSION,&r.api);
        if(result || (r.api.api_version>>16)!=(SKT_PLUGIN_API_VERSION>>16)
            || (r.api.api_version&0xffffu)>(SKT_PLUGIN_API_VERSION&0xffffu)){
            r.view.state=State::ApiMismatch;r.view.error=result?(uint32_t)result:ERROR_REVISION_MISMATCH;continue;
        }
        if(!Metadata(r,root,directory)){r.view.state=State::InvalidDescriptor;r.view.error=ERROR_INVALID_DATA;continue;}
        bool duplicate=false;for(unsigned j=0;j<i;++j)if(g_records[j].view.id[0] && !_stricmp(g_records[j].view.id,r.view.id)){duplicate=true;break;}
        if(duplicate){r.view.state=State::DuplicateId;r.view.error=ERROR_DUP_NAME;continue;}
        r.view.state=State::Loaded;
    }
    // No FreeLibrary: even a rejected DLL may have started its own code in DllMain.
    g_count.store(processed,std::memory_order_release);
    g_loading.store(false,std::memory_order_release);
    if(processed && g_run.load(std::memory_order_relaxed))g_work.store(true,std::memory_order_release);
}
bool Started(){return g_started.load(std::memory_order_relaxed);}
bool Loading(){return g_loading.load(std::memory_order_acquire);}
bool Limited(){return g_limited.load(std::memory_order_relaxed);}
uint32_t ScanError(){return g_scanError.load(std::memory_order_relaxed);}
unsigned Count(){return g_count.load(std::memory_order_acquire);}
const Plugin* At(unsigned index){return index<Count()?&g_records[index].view:nullptr;}
bool NeedsPaint(){return g_work.load(std::memory_order_acquire);}
bool Pending(){return g_request!=Request::None;}
const Page& CurrentPage(){return g_page;}
Counters Stats(){return Count()?g_stats:Counters{};}

bool RequestPage(unsigned plugin,uint32_t page) {
    if(Pending() || plugin>=Count())return false;
    const auto& r=g_records[plugin];
    if(r.view.state!=State::Active || !r.api.build_page)return false;
    g_requestedPlugin=plugin;g_requestedPage=page;g_request=Request::Page;g_page.valid=false;
    g_work.store(true,std::memory_order_release);return true;
}
bool RequestAction(uint32_t item,double value) {
    if(Pending() || !g_page.valid || !std::isfinite(value))return false;
    const auto& r=g_records[g_page.plugin];if(r.view.state!=State::Active || !r.api.on_action)return false;
    for(unsigned i=0;i<g_page.count;++i){const auto& row=g_page.items[i];if(row.id!=item)continue;
        if(row.flags&SKT_MENU_DISABLED)return false;
        if(row.type==SKT_MENU_INFO || row.type==SKT_MENU_SUBMENU)return false;
        if(row.type==SKT_MENU_TOGGLE && value!=0 && value!=1)return false;
        if(row.type==SKT_MENU_CHOICE && (value<0 || value>=row.choice_count || floor(value)!=value))return false;
        if(row.type==SKT_MENU_NUMBER && (value<row.min || value>row.max))return false;
        g_requestedPlugin=g_page.plugin;g_requestedPage=g_page.page_id;g_requestedItem=item;g_requestedValue=value;
        g_request=Request::Action;g_work.store(true,std::memory_order_release);return true;
    }
    return false;
}
void PaintPump(bool english) {
    if(!NeedsPaint() || !g_run.load(std::memory_order_relaxed))return;
    const unsigned count=Count();
    // No foreign calls while loader metadata is still being published.
    if(Loading())return;
    while(g_nextInit<count){
        auto& r=g_records[g_nextInit++];if(r.view.state!=State::Loaded)continue;
        ++g_stats.inits;const int32_t result=r.api.on_host_ready?r.api.on_host_ready(r.api.context,&r.host):0;
        if(result){r.view.state=State::InitError;r.view.error=(uint32_t)result;}else r.view.state=State::Active;
        if(g_nextInit==count && !Pending())g_work.store(false,std::memory_order_release);
        return;
    }
    const Request request=g_request;g_request=Request::None;
    if(request!=Request::None){
        auto& r=g_records[g_requestedPlugin];
        if(request==Request::Action){
            ++g_stats.actions;const int32_t result=r.api.on_action(r.api.context,g_requestedPage,g_requestedItem,g_requestedValue);
            if(result){Fail(r,State::ActionError,(uint32_t)result);g_work.store(false,std::memory_order_release);return;}
        }
        Build(r,g_requestedPlugin,g_requestedPage,english);
    }
    g_work.store(false,std::memory_order_release);
}
}
