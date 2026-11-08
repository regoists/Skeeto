#include "skeeto_plugin_api.h"

namespace {
int enabled=0,choice=1;
double amount=0.25;
unsigned actions=0;
int32_t SKT_CALL Ready(void*,const SktHostApi* host) {
    return host && host->size>=sizeof(SktHostApi) && (host->api_version>>16)==1?0:1;
}
void Item(SktMenuItem& row,uint32_t id,uint32_t type,const char* zh,const char* en) {
    row={};row.size=sizeof(row);row.id=id;row.type=type;row.label_zh=zh;row.label_en=en;
}
int32_t SKT_CALL Build(void*,uint32_t page,SktMenuPage* menu) {
    if(menu->capacity<6 || page>1)return 1;
    menu->title_zh=page?"示例子菜单":"HelloMenu：SDK 示例";
    menu->title_en=page?"Example submenu":"HelloMenu: SDK example";
    menu->count=page?2:6;
    if(page){
        Item(menu->items[0],101,SKT_MENU_INFO,"说明","About");
        menu->items[0].text_zh="此示例不修改游戏功能";menu->items[0].text_en="This example does not modify gameplay";
        Item(menu->items[1],102,SKT_MENU_ACTION,"恢复示例默认值","Reset example values");return 0;
    }
    Item(menu->items[0],1,SKT_MENU_INFO,"状态","Status");
    menu->items[0].text_zh=actions?"已处理菜单操作":"等待菜单操作";
    menu->items[0].text_en=actions?"Menu action received":"Waiting for an action";
    Item(menu->items[1],2,SKT_MENU_TOGGLE,"示例开关","Example toggle");menu->items[1].value=enabled;
    Item(menu->items[2],3,SKT_MENU_NUMBER,"示例数值","Example value");
    menu->items[2].value=amount;menu->items[2].min_value=0;menu->items[2].max_value=1;menu->items[2].step=0.01;
    Item(menu->items[3],4,SKT_MENU_CHOICE,"示例档位","Example choice");
    menu->items[3].value=choice;menu->items[3].choices_zh="安静|标准|增强";menu->items[3].choices_en="Quiet|Normal|Enhanced";
    Item(menu->items[4],5,SKT_MENU_SUBMENU,"子菜单","Submenu");menu->items[4].page_id=1;
    Item(menu->items[5],6,SKT_MENU_ACTION,"恢复示例默认值","Reset example values");return 0;
}
int32_t SKT_CALL Action(void*,uint32_t,uint32_t item,double value) {
    if(item==2)enabled=value!=0;
    else if(item==3)amount=value;
    else if(item==4)choice=(int)value;
    else if(item==6 || item==102){enabled=0;choice=1;amount=0.25;}
    else return 1;
    ++actions;return 0;
}
}
extern "C" __declspec(dllexport) int32_t SKT_CALL SkeetoPlugin_Query(uint32_t host,SktPluginInfo* out) {
    if(!out || out->size<sizeof(*out) || (host>>16)!=1)return 1;
    out->size=sizeof(*out);out->api_version=SKT_PLUGIN_API_VERSION;
    out->id="skeeto.hello_menu";out->name_zh="HelloMenu 示例";out->name_en="HelloMenu example";
    out->author="Skeeto SDK";out->version="1.0.0";out->context=nullptr;
    out->on_host_ready=Ready;out->build_page=Build;out->on_action=Action;return 0;
}
