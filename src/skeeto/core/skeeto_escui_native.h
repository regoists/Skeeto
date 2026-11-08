#pragma once
// Native ESC homepage stays logically visible and retains its entire child tree.
// Only its class's PaintTraverse/IsWithinTraverse are replaced. No reparenting,
// visibility interception, or manual ownership of the game's flyout lifecycle.
using EscUiPaintFn=void(__thiscall*)(void*,bool,bool);
using EscUiHitFn=unsigned int(__thiscall*)(void*,int,int,bool);
using EscUiPanelFn=unsigned int(__thiscall*)(void*);
using EscUiFlagFn=bool(__thiscall*)(void*,unsigned int);
using EscUiKeyboardFn=void(__thiscall*)(void*,unsigned int,bool);
using EscUiActiveWindowFn=int(__thiscall*)(void*);
static EscUiPaintFn g_escUiOrigHomePaint=nullptr;
static EscUiHitFn g_escUiOrigHomeHit=nullptr;
static EscUiPanelFn g_escUiNativePanel=nullptr;
static EscUiFlagFn g_escUiKeyboardEnabled=nullptr,g_escUiIsPopup=nullptr;
static EscUiFlagFn g_escUiNativeVisible=nullptr,g_escUiMouseEnabled=nullptr;
static EscUiKeyboardFn g_escUiKeyboardSet=nullptr;
static EscUiActiveWindowFn g_escUiActiveWindow=nullptr;
static uintptr_t g_escUiActiveFlyout=0;
static void** g_escUiHomeTable=nullptr;
static HMODULE g_escUiNativeModule=nullptr;
static void* g_escUiBasePanel=nullptr;
static void* g_escUiFlyoutObject=nullptr;
static unsigned int g_escUiMaskedPanel=0;
static uint32_t g_escUiMaskedIdentity=0;
static bool g_escUiSavedKeyboard=false;
static std::atomic_bool g_escUiMaskArmed{false};
static std::atomic_uint g_escUiHomePaintSuppressed{0},g_escUiNativeFaults{0};
static void EscUiNativeKeyboard(unsigned int panel,bool enabled){
    // GUI thread, one read and a write only on a real input-mode change.
    if(g_escUiKeyboardEnabled(g_ipanel,panel)!=enabled)g_escUiKeyboardSet(g_ipanel,panel,enabled);
}
static void* EscUiNativeFlyoutObject(){
    if(!g_escUiFlyoutObject)return nullptr;
    void* active=nullptr;
    if(!skmem::Copy(&active,(void*)g_escUiActiveFlyout,sizeof(active))){g_escUiNativeFaults.fetch_add(1);return nullptr;}
    // The game alone clears/replaces this pointer before destroying a flyout.
    // Do not dereference our saved object after native ownership has ended.
    return active==g_escUiFlyoutObject?active:nullptr;
}
static void __fastcall Hooked_EscUiHomePaint(void* self,void*,bool force,bool allow){
    if(!g_escUiMaskArmed.load(std::memory_order_acquire)){g_escUiOrigHomePaint(self,force,allow);return;}
    const unsigned int panel=g_escUiNativePanel(self);
    if(!g_escUiNativeVisible(g_ipanel,panel))return;
    if(panel!=g_escUiMaskedPanel){
        g_escUiMaskedPanel=panel;g_escUiMaskedIdentity=HudVguiPanelIdentity(panel);
        g_escUiSavedKeyboard=g_escUiKeyboardEnabled(g_ipanel,panel);
    }
    // First opening can create the homepage inside original EngineVGui::Paint.
    // Suppress that very first traversal and discover it on the next Paint.
    if(!g_escUiHome)g_escUiSearchAt=0;
    void* vote=EscUiNativeFlyoutObject();EscUiNativeKeyboard(panel,vote!=nullptr);
    g_escUiHomePaintSuppressed.fetch_add(1,std::memory_order_relaxed);
    if(vote){
        unsigned int child=g_escUiNativePanel(vote);
        // Popups receive their own Surface traversal. Ordinary flyouts need
        // this child traversal because the homepage traversal was suppressed.
        if(!g_escUiIsPopup(g_ipanel,child)){
            auto paint=(EscUiPaintFn)VGet(vote,3);paint(vote,force,allow);
        }
    }
}
static unsigned int __fastcall Hooked_EscUiHomeHit(void* self,void*,int x,int y,bool popups){
    if(!g_escUiMaskArmed.load(std::memory_order_acquire))return g_escUiOrigHomeHit(self,x,y,popups);
    unsigned int panel=g_escUiNativePanel(self);
    if(!g_escUiNativeVisible(g_ipanel,panel)||!g_escUiMouseEnabled(g_ipanel,panel))return 0;
    void* vote=EscUiNativeFlyoutObject();
    if(!vote)return 0; // Never hit invisible stock homepage buttons.
    auto hit=(EscUiHitFn)VGet(vote,5);return hit(vote,x,y,popups);
}
static void EscUiNativeFlyoutReturn(){
    // The native menu owns CloseMenu, caller pointers, focus and destruction.
    g_escUiFlyoutObject=nullptr;
}
static void EscUiNativeRelease(){
    g_escUiMaskArmed.store(false,std::memory_order_release);EscUiNativeFlyoutReturn();
    if(g_escUiHomeTable&&GetModuleHandleA("gameui.dll")==g_escUiNativeModule){
        if(g_escUiHomeTable[3]==(void*)&Hooked_EscUiHomePaint
            &&!VtProtectWrite(&g_escUiHomeTable[3],(void*)g_escUiOrigHomePaint,nullptr))g_escUiNativeFaults.fetch_add(1);
        if(g_escUiHomeTable[5]==(void*)&Hooked_EscUiHomeHit
            &&!VtProtectWrite(&g_escUiHomeTable[5],(void*)g_escUiOrigHomeHit,nullptr))g_escUiNativeFaults.fetch_add(1);
        if(g_escUiMaskedPanel&&HudVguiPanelIdentity(g_escUiMaskedPanel)==g_escUiMaskedIdentity)
            EscUiNativeKeyboard(g_escUiMaskedPanel,g_escUiSavedKeyboard);
    }
    g_escUiHomeTable=nullptr;g_escUiNativeModule=nullptr;g_escUiBasePanel=nullptr;
    g_escUiMaskedPanel=0;g_escUiMaskedIdentity=0;
}
static bool EscUiNativeAcquire(){
    HMODULE mod=GetModuleHandleA("gameui.dll");if(!mod)return false;
    uintptr_t command=FindPat("gameui.dll","55 8B EC 81 EC C0 00 00 00 A1 ? ? ? ? 33 C5 89 45 FC 53 56 8B 75 08 57 8B F9");
    if(!g_escUiActiveWindow){
        g_escUiActiveWindow=(EscUiActiveWindowFn)FindPat("gameui.dll","53 56 8B D9 57 BF 07 00 00 00 8D B3 78 02 00 00 8B 46 FC 83 EE 04 4F 85 C0 74 20 8D 8C 83 A8 01 00 00 E8 ? ? ? ? 85 C0 74 10 8B 10 8B C8 8B 82 88 00 00 00 FF D0 84 C0 75 0A 85 FF 7F D1 5F 5E 33 C0 5B C3 8B 84 BB 5C 02 00 00 5F 5E 5B C3");
        uintptr_t open=FindPat("gameui.dll","55 8B EC 83 EC 0C 56 8B F1 8B 0D ? ? ? ? 3B CE 75 10 6A 00 8B CE E8 ? ? ? ? 5E");
        if(open&&!skmem::Copy(&g_escUiActiveFlyout,(void*)(open+11),sizeof(g_escUiActiveFlyout)))return false;
    }
    if(!command||!g_escUiActiveWindow||!g_escUiActiveFlyout)return false;
    // Cold class-vtable discovery, before any homepage instance is required.
    // Retail constructor assigns this table; slot96 is its unique OnCommand.
    void** table=nullptr;int hits=0;
    __try{
        auto dos=(PIMAGE_DOS_HEADER)mod;auto nt=(PIMAGE_NT_HEADERS)((uint8_t*)mod+dos->e_lfanew);
        auto sections=IMAGE_FIRST_SECTION(nt);
        for(WORD i=0;i<nt->FileHeader.NumberOfSections;++i){
            auto& sec=sections[i];if(!(sec.Characteristics&IMAGE_SCN_MEM_READ)||(sec.Characteristics&IMAGE_SCN_MEM_EXECUTE))continue;
            uintptr_t start=(uintptr_t)mod+sec.VirtualAddress,end=start+sec.Misc.VirtualSize;
            for(uintptr_t at=start+96*sizeof(void*);at+sizeof(void*)<=end;at+=sizeof(void*)){
                if(*(uintptr_t*)at!=command)continue;
                void** candidate=(void**)at-96;
                if(!IsExec(candidate[0])||!IsExec(candidate[3])||!IsExec(candidate[5])||IsExec(candidate[-1]))continue;
                table=candidate;++hits;
            }
        }
        if(hits!=1)return false;
        // Do not capture another plugin's wrapper of an earlier hook as orig.
        if((g_escUiOrigHomePaint&&table[3]!=(void*)g_escUiOrigHomePaint)
            ||(g_escUiOrigHomeHit&&table[5]!=(void*)g_escUiOrigHomeHit))return false;
        g_escUiOrigHomePaint=(EscUiPaintFn)table[3];g_escUiOrigHomeHit=(EscUiHitFn)table[5];
        g_escUiNativePanel=(EscUiPanelFn)table[0];
        g_escUiKeyboardEnabled=(EscUiFlagFn)HudVguiMethod(33);
        g_escUiNativeVisible=(EscUiFlagFn)HudVguiMethod(15);g_escUiMouseEnabled=(EscUiFlagFn)HudVguiMethod(34);
        g_escUiIsPopup=(EscUiFlagFn)HudVguiMethod(24);
        g_escUiKeyboardSet=(EscUiKeyboardFn)HudVguiMethod(31);
        if(!g_escUiKeyboardEnabled||!g_escUiKeyboardSet||!g_escUiIsPopup||!g_escUiNativeVisible||!g_escUiMouseEnabled)return false;
        if(!VtProtectWrite(&table[3],(void*)&Hooked_EscUiHomePaint,nullptr))return false;
        if(!VtProtectWrite(&table[5],(void*)&Hooked_EscUiHomeHit,nullptr)){
            VtProtectWrite(&table[3],(void*)g_escUiOrigHomePaint,nullptr);return false;
        }
        g_escUiHomeTable=table;g_escUiNativeModule=mod;g_escUiOwned=true;
        g_escUiNativeFaults.store(0);g_escUiMaskArmed.store(true,std::memory_order_release);
    }__except(EXCEPTION_EXECUTE_HANDLER){g_escUiNativeFaults.fetch_add(1);return false;}
    return true;
}
static bool EscUiNativeBaseReady(){
    if(g_escUiBasePanel)return true;
    using GetFn=void*(__thiscall*)(void*,unsigned int,const char*);
    auto get=(GetFn)VGet(g_ipanel,55);if(!get||!IsExec((void*)get))return false;
    __try{
        void* base=get(g_ipanel,g_escUiParent,"GameUI");void** slot=nullptr;uintptr_t paint=0;
        if(!base||!BackgroundLocateGameUiPaint(g_escUiNativeModule,&slot,&paint))return false;
        void* actual=VGet(base,127);
        if(actual!=(void*)paint&&actual!=(void*)&Hooked_BackgroundGameUiPaint)return false;
        g_escUiBasePanel=base;
    }__except(EXCEPTION_EXECUTE_HANDLER){g_escUiNativeFaults.fetch_add(1);return false;}
    return true;
}
static bool EscUiNativeFlyoutTake(const char* expected){
    void* flyout=nullptr;
    if(!skmem::Copy(&flyout,(void*)g_escUiActiveFlyout,sizeof(flyout))||!flyout)return false;
    unsigned int p=g_escUiNativePanel(flyout);
    if(HudVguiParent(p)!=g_escUiHome||strncmp(HudVguiName(p),expected,strlen(expected)))return false;
    g_escUiFlyoutObject=flyout;return true;
}
static bool EscUiNativeHomeReady(){
    if(!g_escUiOwned||!EscUiHomeMatches())return false;
    if(EscUiNativeFlyoutObject()){g_escUiGate="native-flyout";return false;}
    EscUiNativeFlyoutReturn();
    int active=0;
    __try{active=g_escUiActiveWindow(g_escUiBasePanel);}
    __except(EXCEPTION_EXECUTE_HANDLER){g_escUiFault=true;g_escUiGate="native-window-fault";return false;}
    if(active&&active!=0x0e){g_escUiGate="native-window";return false;}
    if(BackgroundConsoleOpen()){g_escUiGate="console";return false;}
    return true;
}
