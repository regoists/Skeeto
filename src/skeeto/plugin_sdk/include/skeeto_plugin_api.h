#ifndef SKEETO_PLUGIN_API_H
#define SKEETO_PLUGIN_API_H
#include <stdint.h>

#if defined(_MSC_VER)
#define SKT_CALL __cdecl
#else
#define SKT_CALL __attribute__((cdecl))
#endif
#define SKT_PLUGIN_API_VERSION UINT32_C(0x00010000)
#define SKT_PLUGIN_INFO_V1_SIZE UINT32_C(44)
#define SKT_MENU_DISABLED UINT32_C(1)
#define SKT_MENU_MAX_ITEMS UINT32_C(64)

enum SktMenuType {
    SKT_MENU_INFO=0, SKT_MENU_ACTION=1, SKT_MENU_TOGGLE=2,
    SKT_MENU_CHOICE=3, SKT_MENU_NUMBER=4, SKT_MENU_SUBMENU=5
};

#pragma pack(push,8)
typedef struct SktHostApi {
    uint32_t size, api_version;
    const char* plugin_directory_utf8;
    const char* data_directory_utf8;
} SktHostApi;

typedef struct SktMenuItem {
    uint32_t size, id, type, flags;
    const char* label_zh;
    const char* label_en;
    const char* text_zh;
    const char* text_en;
    double value, min_value, max_value, step;
    const char* choices_zh;
    const char* choices_en;
    uint32_t page_id;
} SktMenuItem;

typedef struct SktMenuPage {
    uint32_t size, page_id;
    const char* title_zh;
    const char* title_en;
    SktMenuItem* items;
    uint32_t capacity, count;
} SktMenuPage;

typedef int32_t (SKT_CALL *SktOnHostReady)(void* context,const SktHostApi* host);
typedef int32_t (SKT_CALL *SktBuildPage)(void* context,uint32_t page_id,SktMenuPage* page);
typedef int32_t (SKT_CALL *SktOnAction)(void* context,uint32_t page_id,uint32_t item_id,double value);
typedef struct SktPluginInfo {
    uint32_t size, api_version;
    const char* id;
    const char* name_zh;
    const char* name_en;
    const char* author;
    const char* version;
    void* context;
    SktOnHostReady on_host_ready;
    SktBuildPage build_page;
    SktOnAction on_action;
} SktPluginInfo;
typedef int32_t (SKT_CALL *SktPluginQuery)(uint32_t host_api_version,SktPluginInfo* out_info);
#pragma pack(pop)

/* Export exactly "SkeetoPlugin_Query" (no x86 name decoration).
 * Query returns descriptors on the loader worker. One size-bounded local
 * configuration read is allowed; no scanning, networking, waits or Source/VGUI.
 * Other callbacks run on the game/Paint thread and must return promptly.
 * UTF-8 strings are copied by the host immediately after each callback.
 * No exceptions, STL or cross-CRT allocations cross this ABI.
 * API major must match; a plugin's required minor must not exceed the host's.
 * Preserve the API 1.0 descriptor prefix. SktMenuItem ARRAY STRIDE is frozen
 * at 80 bytes for API 1.x: do not append fields to that array element type.
 */
#endif
