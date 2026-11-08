// Explicit main-menu clicks only. Retail call sites supply ABI and ownership.
#pragma once
// Retail gameui 6A440E8A / 263000: MainMenu::OnThink sets IconAddons
// visible iff BasePanel+0x2D0 is false (call site RVA 67DE9). BasePanel's
// BasePanel queues update_addon_paths/mission_reload before setting that byte.
// Read the same native flag; hiding MainMenu must not freeze this indicator.
// Never infer completion from a timer, the VPK directory, or Steam downloads.
static int MainUiAddonsRead(){
 if(!g_optMainMenuUi||!g_mainUiDraw||!g_mainUiOwned||g_mainUiEditing||g_mainUiConnected||g_mainUiInGame||g_mainUiLoading){MainUiAddonsStop();return -1;}
 const DWORD now=GetTickCount();if(g_mainUiAddonsReadAt&&(LONG)(now-g_mainUiAddonsReadAt)<0)return g_mainUiAddonsState;
 g_mainUiAddonsReadAt=now+100u;g_mainUiAddonsState=-1;
 g_mainUiAddonsState=NativeAddonsLoadState();
 return g_mainUiAddonsState;
}

struct MainUiLobbyNativeApi {
 void*(__cdecl* base)()=nullptr;
 void*(__cdecl* parse)(const char*,const char*,const char**)=nullptr;
 void*(__thiscall* open)(void*,const int*,void*,bool,void*)=nullptr;
 void(__thiscall* close)(void*,int)=nullptr;
 void(__thiscall* destroy)(void*)=nullptr;
};
static bool MainUiLobbyNativeBind(MainUiLobbyNativeApi* out){
 static HMODULE checked=nullptr;static MainUiLobbyNativeApi api{};
 HMODULE module=GetModuleHandleA("gameui.dll");if(!module)return false;
 if(module!=checked){checked=module;api=MainUiLobbyNativeApi{};
  uintptr_t parse=FindPat("gameui.dll","6A 00 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 8E FC 02 00 00 83 C4 0C 68 ? ? ? ? 68 ? ? ? ? 8B F8");
  uintptr_t window=FindPat("gameui.dll","57 6A 01 8D 4D B8 6A 00 51 C7 45 B8 0B 00 00 00 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 85 FF 0F 84 ? ? ? ? 8B CF E8 ? ? ? ?");

  auto target=[&](uintptr_t p)->uintptr_t{if(!p||!IsExec((void*)p)||*(unsigned char*)p!=0xE8)return 0;int relative=0;memcpy(&relative,(void*)(p+1),4);uintptr_t fn=p+5+relative;return BackgroundPtrInModule((void*)fn,module)&&IsExec((void*)fn)?fn:0;};
  uintptr_t make=parse?target(parse+12):0,base=window?target(window+16):0,open=window?target(window+23):0,free=window?target(window+38):0;
  uintptr_t close=window&&BackgroundPtrInModule((void*)(window-14),module)&&*(unsigned char*)(window-14)==0x6A&&*(unsigned char*)(window-13)==0&&target(window-12)==base?target(window-5):0;
  if(make&&base&&open&&free&&close){api.close=(decltype(api.close))close;api.parse=(decltype(api.parse))make;api.base=(decltype(api.base))base;api.open=(decltype(api.open))open;api.destroy=(decltype(api.destroy))free;}
 }
 if(!api.base)return false;if(out)*out=api;return true;
}
static bool MainUiOpenLobbyPreset(const char* parameter){
 // No command execution while importing/editing; this is reached by a click edge.
 if(!g_mainUiInputReady.load(std::memory_order_relaxed)||g_mainUiUnderlay||g_mainUiEditing||g_mainUiConnected||g_mainUiInGame||g_mainUiLoading||!MainUiHomeMatches())return false;
 char settings[768]{};if(!MainUiLobbySettingsText(parameter,settings,sizeof(settings))){g_mainUiNotice=2;return false;}
 MainUiLobbyNativeApi api{};if(!MainUiLobbyNativeBind(&api)){g_mainUiNotice=1;return false;}
 // FromString allocates via GameUI's own KeyValues allocator; OpenWindow copies
 // synchronously through GameSettings::SetDataSettings. The caller then deletes.
 __try{
  void* manager=api.base();if(!manager||!IfaceAlive(manager)){g_mainUiNotice=1;return false;}
  void* values=api.parse("Settings",settings,nullptr);if(!values){g_mainUiNotice=1;return false;}
  MainUiRestore();g_mainUiNative=false;g_mainUiFlyout=-1;
  // Match retail CreateGame: CloseAllWindows(0) before GameSettings.
  api.close(manager,0);
  const int window=11;void* panel=api.open(manager,&window,nullptr,true,values);api.destroy(values);
  if(!panel){g_mainUiNotice=1;return false;}
  snprintf(g_mainUiLastCommand,sizeof(g_mainUiLastCommand),"CreateLobby preset: %.60s",parameter);g_mainUiPopupAt=0;g_mainUiInputSeed=true;g_mainUiNotice=0;return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){g_mainUiFault=true;g_mainUiNotice=1;MainUiGate("lobby-preset-fault");MainUiRestore();return false;}
}
