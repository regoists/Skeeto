#pragma once

// Native VGUI appearance only. No hooks, message interception or replacement UI.
// All object access happens on EnginePaint. Disabled: one atomic mask check.
// Enabled: <= 32 cached controls checked at 4 Hz; subtree discovery <=384 panels,
// depth<=10 at most once/2s per target. Fonts: at most two owned HFonts total.
namespace sknativeui_runtime {
using namespace sknativeui;
enum Role { Root, History, Entry, Prompt, ConsolePage, ChatLine };
struct Panel {
    unsigned int id=0; void* object=nullptr; void* vtable=nullptr; uint32_t identity=0;
    Role role=Root; uint32_t bg=0,fg=0,font=0,lastBg=0,lastFg=0,lastFont=0;
    Rect bounds{},lastBounds{}; bool geometry=false;
    int paintType=0; bool border=false,lastBorder=false;
    uint32_t extra[6]{},lastExtra[6]{}; bool special=false;
    float alpha=255;bool alphaOwned=false;
};
struct Cvar { const char* name; char before[64]{},last[64]{}; void* object=nullptr; };
struct Target {
    Panel panels[32]{}; int count=0; HMODULE module=nullptr;
    void* richFont=nullptr; void* entryFont=nullptr; void* setString=nullptr; void* frameScheme=nullptr;void* setAlpha=nullptr;
    bool resolved=false; DWORD searchAt=0; unsigned revision=0;
    unsigned long font=0; int fontPx=0,fontFace=-1,fontFlags=-1,fontWeight=0;
    bool wasEnabled=false,fontReady=false,abiUnsupported=false,cvarUnsupported=false; int sw=0,sh=0;
    DWORD mapEpoch=0;
};
static Target g_targets[2];
static Cvar g_chatCvars[]={{"hud_saytext_time"},{"cl_color_survivor"},{"cl_color_zombie"}};
static DWORD g_tickAt=0;
static const char* kFaces[]={"Microsoft YaHei","SimHei","Consolas","Tahoma"};
static constexpr int kFrameFields[]={0x196,0x19a,0x186,0x18a,0x18e,0x192};

// Verified on retail x86 gameui.dll/client.dll, 2026-10-24. ABI evidence and
// hashes: docs/troubleshooting/20261024-原生聊天框与控制台样式.md.
// Panel GetBg/Fg: slots60/61 return Color through a hidden 4-byte result buffer.
static bool ColorAbi(void* object,Role role) {
    const unsigned char bg[]={0x55,0x8b,0xec,0x8b,0x89,0x90,0,0,0,0x8b,0x45,8,0x89,8,0x5d,0xc2,4,0};
    const unsigned char fg[]={0x55,0x8b,0xec,0x8b,0x89,0x8c,0,0,0,0x8b,0x45,8,0x89,8,0x5d,0xc2,4,0};
    const unsigned char border[]={0x55,0x8b,0xec,0x80,0x7d,8,0,0x74,9,0x66,0x83,0x49,0x60,4,0x5d,0xc2,4,0};
    const unsigned char type[]={0x55,0x8b,0xec,0x8b,0x45,8,0x83,0xf8,2,0x7e,0x0f,0xb8,2,0,0,0,0x89,0x81,0x20,1,0,0,0x5d,0xc2};
    unsigned char a[sizeof(bg)]{},b[sizeof(fg)]{};
    unsigned char c[sizeof(border)]{},d[sizeof(type)]{};
    auto foreground=(uint8_t*)VGet(object,61);
    if(role==Prompt){
        // Label::GetFgColor forwards the same hidden result to Panel's getter.
        // The prompt is a Label, unlike the history/input controls.
        const unsigned char head[]={0x55,0x8b,0xec,0x56,0x8b,0x75,8,0x56,0xe8};
        const unsigned char tail[]={0x8b,0xc6,0x5e,0x5d,0xc2,4,0};unsigned char code[20]{};
        if(!skmem::Copy(code,foreground,sizeof(code)) || memcmp(code,head,sizeof(head))
            || memcmp(code+13,tail,sizeof(tail)))return false;
        int displacement=0;memcpy(&displacement,code+9,4);foreground+=13+displacement;
    }
    return skmem::Copy(a,VGet(object,60),sizeof(a)) && skmem::Copy(b,foreground,sizeof(b))
        && !memcmp(a,bg,sizeof(a)) && !memcmp(b,fg,sizeof(b))
        && skmem::Copy(c,VGet(object,70),sizeof(c)) && !memcmp(c,border,sizeof(c))
        && skmem::Copy(d,VGet(object,74),sizeof(d)) && !memcmp(d,type,sizeof(d))
        && IsExec(VGet(object,4)) && IsExec(VGet(object,58)) && IsExec(VGet(object,59));
}
static void* Object(unsigned int id,int target) {
    using Fn=void*(__thiscall*)(void*,unsigned int,const char*);
    auto fn=(Fn)VGet(g_ipanel,55);
    return fn(g_ipanel,id,target?"GameUI":"ClientDLL");
}
static uint32_t GetColor(void* p,bool foreground) {
    using Fn=void(__thiscall*)(void*,uint32_t*);
    uint32_t value=0; ((Fn)VGet(p,foreground?61:60))(p,&value); return value;
}
static void SetColor(void* p,bool foreground,uint32_t color) {
    using Fn=void(__thiscall*)(void*,uint32_t);
    ((Fn)VGet(p,foreground?59:58))(p,color);
    // Retail Panel::SetBg/FgColor only stores the value. Explicitly invalidate
    // paint so a quiet native panel updates without requiring mouse movement.
    using Repaint=void(__thiscall*)(void*);((Repaint)VGet(p,4))(p);
}
static bool Alive(const Panel& p,int target) {
    return HudVguiPanelIdentity(p.id)==p.identity && Object(p.id,target)==p.object
        && *(void**)p.object==p.vtable;
}
static void Resolve(Target& s,int target) {
    if(s.resolved)return;
    s.resolved=true;
    const char* mod=target?"gameui.dll":"client.dll";
    s.module=GetModuleHandleA(mod);
    if(!s.module){s.resolved=false;return;}
    // RichText::SetFont is nonvirtual; TextEntry's derived vtable extent differs.
    // Match the actual store and InvalidateLayout call, never guess a derived slot.
    s.richFont=(void*)FindPat(mod,"55 8B EC 8B 45 08 56 8B F1 8B 16 6A 00 89 86 C0 01 00 00 8B 82 08 01 00 00");
    s.entryFont=(void*)FindPat(mod,"55 8B EC 8B 45 08 56 8B F1 8B 16 6A 00 89 86 CC 01 00 00 8B 82 08 01 00 00");
    if(!target){
        s.setString=(void*)FindPat(mod,"55 8B EC 83 EC 38 A1 ?? ?? ?? ?? 33 C5 89 45 FC 56 8B F1 8B 06 8B 50 08 57 8B 7D 08 68 00 00 B0 00");
        // Panel::SetAlpha stores its int argument as float at +118. Native
        // PaintTraverse multiplies this into the whole subtree's alpha.
        s.setAlpha=(void*)FindPat(mod,"55 8B EC 0F 57 C0 F3 0F 2A 45 08 F3 0F 11 81 18 01 00 00 5D C2 04 00");
    }
    else s.frameScheme=(void*)FindPat(mod,"55 8B EC 83 EC 08 53 56 57 8B 7D 08 57 8B F1 E8 ?? ?? ?? ?? 8B 06 8B 90 5C 01 00 00 57 68 ?? ?? ?? ?? 8D 4D 08 51");
}
static void Add(Target& s,int target,unsigned int id,Role role) {
    for(int i=0;i<s.count;++i)if(s.panels[i].id==id)return;
    if(s.count==32)return;
    Panel p{};p.id=id;p.role=role;p.identity=HudVguiPanelIdentity(id);p.object=Object(id,target);
    if(!p.object || !p.identity || !ColorAbi(p.object,role)) {s.abiUnsupported=true;return;}
    p.vtable=*(void**)p.object; p.bg=p.lastBg=GetColor(p.object,false);p.fg=p.lastFg=GetColor(p.object,true);
    uint16_t flags=0;skmem::Copy(&flags,(uint8_t*)p.object+0x60,2);p.border=p.lastBorder=(flags&4)!=0;
    skmem::Copy(&p.paintType,(uint8_t*)p.object+0x120,4);
    HudVguiGetPos(id,&p.bounds.x,&p.bounds.y);HudVguiGetSize(id,&p.bounds.w,&p.bounds.h);p.lastBounds=p.bounds;
    const int off=(role==History || role==ChatLine)?0x1c0:role==Entry?0x1cc:0;
    if(off)skmem::Copy(&p.font,(uint8_t*)p.object+off,4);
    p.lastFont=p.font;
    if(target && role==ConsolePage){
        const unsigned char stores[]={0x89,0x86,0x98,1,0,0};unsigned char code[6]{};
        // CConsolePanel::ApplySchemeSettings +0x2a stores normal output at +198.
        auto scheme=(uint8_t*)VGet(p.object,88);
        p.special=skmem::Copy(code,scheme+0x2a,6)&&!memcmp(code,stores,6);
        const unsigned char debugStore[]={0x89,0x86,0x9c,1,0,0};
        p.special=p.special && skmem::Copy(code,scheme+0x47,6)&&!memcmp(code,debugStore,6);
        if(p.special){skmem::Copy(p.extra,(uint8_t*)p.object+0x198,8);memcpy(p.lastExtra,p.extra,8);}
    }
    if(target && role==Root){
        // CGameConsoleDialog derives Frame. Verify the six scheme-owned fields
        // against the same retail ApplyScheme implementation before using them.
        // The exact Frame fields are only used for a class-identified native dialog.
        const char* cls=HudVguiClass(id);
        p.special=!strcmp(cls,"CGameConsoleDialog") && s.frameScheme && VGet(p.object,88)==s.frameScheme;
        if(p.special)for(int i=0;i<6;++i){skmem::Copy(&p.extra[i],(uint8_t*)p.object+kFrameFields[i],4);p.lastExtra[i]=p.extra[i];}
    }
    if(!target && role==Root){
        if(s.setAlpha)skmem::Copy(&p.alpha,(uint8_t*)p.object+0x118,4);
        // CHudChat::GetTextColorForClient (primary slot31) reads +220 for
        // ordinary messages; Panel::FgColor is not the chat message color.
        // Its Panel base is +38, so that owned color is Panel+1e8.
        const unsigned char load[]={0x8b,0x8e,0x20,2,0,0};unsigned char code[6]{};
        auto fn=(uint8_t*)VGet((uint8_t*)p.object-0x38,31);
        p.special=skmem::Copy(code,fn+0x8e,6) && !memcmp(code,load,6);
        if(p.special){skmem::Copy(p.extra,(uint8_t*)p.object+0x1e8,4);p.lastExtra[0]=p.extra[0];}
        else s.abiUnsupported=true;
    }
    s.panels[s.count++]=p;
}
static bool RootMatch(unsigned int id,int target) {
    const char* name=HudVguiName(id);const char* cls=HudVguiClass(id);
    return target?(!strcmp(cls,"CGameConsoleDialog"))
        :(!strcmp(name,"HudChat") || !strcmp(cls,"CHudChat"));
}
static void Discover(Target& s,int target,unsigned int id,int depth,int& budget,bool inside) {
    if(!id || depth>10 || --budget<0 || s.count==32)return;
    if(!inside && RootMatch(id,target)){Add(s,target,id,Root);inside=true;}
    else if(inside){
        const char* name=HudVguiName(id);const char* cls=HudVguiClass(id);
        if(!strcmp(name,"ConsoleHistory") || !strcmp(name,"HudChatHistory") || !strcmp(name,"ChatHistory"))Add(s,target,id,History);
        else if(strstr(cls,"HudChatLine") || strstr(name,"ChatLine"))Add(s,target,id,ChatLine);
        else if(!strcmp(name,"ConsoleEntry") || !strcmp(name,"ChatInput"))Add(s,target,id,Entry);
        else if(!strcmp(name,"ChatInputPrompt"))Add(s,target,id,Prompt);
        else if(target && !strcmp(cls,"CConsolePanel"))Add(s,target,id,ConsolePage);
    }
    const int count=HudVguiChildCount(id);
    for(int i=0;i<count && budget>0;++i)Discover(s,target,HudVguiChild(id,i),depth+1,budget,inside);
}
static void DiscoverChat(Target& s) {
    // Use the existing HUD-element lookup instead of walking unrelated viewport
    // controls until the search budget runs out. CHudChat has two vtables: the
    // HUD base at +0 and the Panel base at +38 (retail RTTI + constructor).
    using Find=void*(__cdecl*)();auto find=(Find)LoadChatFinder();
    if(!find)return;
    void* chat=find();void* print=nullptr;
    if(!chat)return;
    if(!LoadChatObject(chat,&print)){s.abiUnsupported=true;return;}
    auto panel=(uint8_t*)chat+0x38;
    const unsigned char getter[]={0x8b,0x41,0x48,0xc3};unsigned char code[4]{};
    if(!skmem::Copy(code,VGet(panel,0),4) || memcmp(code,getter,4)){s.abiUnsupported=true;return;}
    using GetPanel=unsigned(__thiscall*)(void*);const unsigned id=((GetPanel)VGet(panel,0))(panel);
    if(!id || Object(id,0)!=panel){s.abiUnsupported=true;return;}
    int budget=384;Discover(s,0,id,0,budget,false);
}
static void DiscoverConsole(Target& s) {
    // The native console is a Surface popup outside the GameUI homepage tree.
    // Use the same retail popup API as MainUiScanPopups, with a fixed ceiling.
    using Count=int(__thiscall*)(void*);using Get=unsigned(__thiscall*)(void*,int);
    auto count=(Count)VGet(g_surf,76);auto get=(Get)VGet(g_surf,77);
    if(!IsExec((void*)count) || !IsExec((void*)get)){g_nativeUiStatus[1].store(3);return;}
    const int n=count(g_surf);if(n<0 || n>4096){g_nativeUiStatus[1].store(3);return;}
    int budget=384;
    for(int i=n-1,seen=0;i>=0 && seen<128 && budget>0;--i,++seen){
        const unsigned id=get(g_surf,i);
        if(RootMatch(id,1)){Discover(s,1,id,0,budget,false);break;}
    }
}
static bool Font(Target& s,const Config& c,int sh,bool& changed) {
    changed=false;
    if(!c.v[FontOverride])return true;
    int px=c.v[AutoScale]?(c.v[FontSize]*sh+540)/1080:c.v[FontSize]; if(px<10)px=10;if(px>96)px=96;
    const int flags=kFontFlagAntiAlias | (c.v[Shadow]?kFontFlagDropShadow:0) | (c.v[Outline]?kFontFlagOutline:0);
    const int weight=c.v[Bold]?700:400;
    if(s.font && px==s.fontPx && flags==s.fontFlags && weight==s.fontWeight && c.v[FontFace]==s.fontFace)return s.fontReady;
    if(!g_surf || g_sktUiFontsHold)return false;
    using Create=unsigned long(__thiscall*)(void*);
    using Glyph=bool(__thiscall*)(void*,unsigned long,const char*,int,int,int,int,int,int,int);
    auto create=(Create)VGet(g_surf,kVmtSurfCreateFont);auto glyph=(Glyph)VGet(g_surf,kVmtSurfSetFontGlyphSet);
    if(!IsExec((void*)create) || !IsExec((void*)glyph))return false;
    if(!s.font)s.font=create(g_surf);
    s.fontPx=px;s.fontFlags=flags;s.fontWeight=weight;s.fontFace=c.v[FontFace];
    s.fontReady=s.font && glyph(g_surf,s.font,kFaces[c.v[FontFace]],px,weight,0,0,flags,0,0);
    changed=s.fontReady;
    return s.fontReady;
}
static void ApplyFont(Target& s,Panel& p,uint32_t want,bool force=false) {
    const bool history=p.role==History || p.role==ChatLine;
    void* fn=history?s.richFont:p.role==Entry?s.entryFont:nullptr;
    if(!fn || !want)return;
    const int off=history?0x1c0:0x1cc;uint32_t cur=0;
    if(!skmem::Copy(&cur,(uint8_t*)p.object+off,4) || (!force && cur==want))return;
    using Fn=void(__thiscall*)(void*,uint32_t);((Fn)fn)(p.object,want);p.lastFont=want;
}
static void Apply(Target& s,Panel& p,const Config& c,int target,int sw,int sh,bool fontChanged) {
    const bool entry=p.role==Entry;
    const bool chatRoot=!target && p.role==Root;
    int chatMode=0;
    // Native StartMessageMode/StopMessageMode own HUD+1f8 = Panel+1c0.
    if(chatRoot && !skmem::Copy(&chatMode,(uint8_t*)p.object+0x1c0,4))return;
    const bool backdrop=p.role==Root;
    // Leave native chat history/line fading alone. A fixed background on those
    // controls fights the game's fade state every 250ms and causes flashing.
    const bool ownsBackground=target || chatRoot || entry;
    const uint32_t bg=chatRoot && !chatMode?p.bg
        :Pack(c.v[entry?InputBackground:Background],entry?c.v[InputBgAlpha]:backdrop?c.v[BgAlpha]:0);
    const uint32_t fg=Pack(c.v[entry?Input:Text],100);
    if(ownsBackground){if(GetColor(p.object,false)!=bg)SetColor(p.object,false,bg);p.lastBg=bg;}
    // Theme borders can paint an opaque backing independently of BgColor.
    // Retail Panel slots70/74 are border enable and plain background type.
    using BorderFn=void(__thiscall*)(void*,bool);using TypeFn=void(__thiscall*)(void*,int);
    uint16_t flags=0;int type=0;skmem::Copy(&flags,(uint8_t*)p.object+0x60,2);skmem::Copy(&type,(uint8_t*)p.object+0x120,4);
    const bool border=c.v[Border]!=0;
    if(ownsBackground){
        if(((flags&4)!=0)!=border)((BorderFn)VGet(p.object,70))(p.object,border);
        if(type!=0)((TypeFn)VGet(p.object,74))(p.object,0);
        p.lastBorder=border;
    }
    if(chatRoot && s.setAlpha){
        float alpha=0;if(!skmem::Copy(&alpha,(uint8_t*)p.object+0x118,4))return;
        if(c.v[SayTime]==0 && !chatMode){
            if(alpha!=0){using Fn=void(__thiscall*)(void*,int);((Fn)s.setAlpha)(p.object,0);}
            p.alphaOwned=true;
        }else if(p.alphaOwned){
            if(alpha==0)skmem::Write((uint8_t*)p.object+0x118,&p.alpha,4);
            p.alphaOwned=false;
        }
    }
    if(p.role==History || p.role==ChatLine || entry || p.role==Prompt){if(GetColor(p.object,true)!=fg)SetColor(p.object,true,fg);p.lastFg=fg;}
    if(p.role==History || p.role==ChatLine || entry)ApplyFont(s,p,c.v[FontOverride]?(s.fontReady?s.font:0):p.font,
        c.v[FontOverride] && fontChanged);
    if(chatRoot){
        if(c.v[Geometry]){
            const Rect r=Bounds(c,sw,sh);int x=0,y=0,w=0,h=0;
            HudVguiGetPos(p.id,&x,&y);HudVguiGetSize(p.id,&w,&h);
            if(w!=r.w || h!=r.h)HudVguiSetSize(p.id,r.w,r.h);
            if(x!=r.x || y!=r.y)HudVguiSetPos(p.id,r.x,r.y);
            p.lastBounds=r;p.geometry=true;
        }else if(p.geometry){HudVguiSetSize(p.id,p.bounds.w,p.bounds.h);HudVguiSetPos(p.id,p.bounds.x,p.bounds.y);p.geometry=false;}
    }
    if(p.special){
        const int n=!target?1:p.role==ConsolePage?2:6;
        for(int i=0;i<n;++i){
            uint32_t want=!target?Pack(c.v[Text],100):p.role==ConsolePage?Pack(c.v[i?Developer:Text],100)
                :Pack(c.v[i<2?Background:i<4?TitleBackground:TitleText],i<4?c.v[BgAlpha]:100);
            const int off=!target?0x1e8:p.role==ConsolePage?0x198+i*4:kFrameFields[i];uint32_t cur=0;
            if(skmem::Copy(&cur,(uint8_t*)p.object+off,4) && cur!=want)skmem::Write((uint8_t*)p.object+off,&want,4);
            p.lastExtra[i]=want;
        }
    }
}
static void Restore(Target& s,Panel& p,int target) {
    if(!Alive(p,target))return;
    if(p.alphaOwned){float alpha=0;
        if(skmem::Copy(&alpha,(uint8_t*)p.object+0x118,4) && alpha==0)skmem::Write((uint8_t*)p.object+0x118,&p.alpha,4);
        p.alphaOwned=false;
    }
    // Match Apply's ownership: native chat history/line/prompt fading and
    // theme properties were never overridden, so disabling must not reset them.
    if(target || p.role==Root || p.role==Entry){
        if(GetColor(p.object,false)==p.lastBg)SetColor(p.object,false,p.bg);
        uint16_t flags=0;int type=0;skmem::Copy(&flags,(uint8_t*)p.object+0x60,2);skmem::Copy(&type,(uint8_t*)p.object+0x120,4);
        using BorderFn=void(__thiscall*)(void*,bool);using TypeFn=void(__thiscall*)(void*,int);
        if(((flags&4)!=0)==p.lastBorder)((BorderFn)VGet(p.object,70))(p.object,p.border);
        if(type==0)((TypeFn)VGet(p.object,74))(p.object,p.paintType);
    }
    if(GetColor(p.object,true)==p.lastFg && p.lastFg!=p.fg)SetColor(p.object,true,p.fg);
    if(p.role==Entry || p.role==History || p.role==ChatLine){uint32_t cur=0;const int off=p.role==Entry?0x1cc:0x1c0;
        if(skmem::Copy(&cur,(uint8_t*)p.object+off,4) && cur==p.lastFont)ApplyFont(s,p,p.font);}
    if(p.geometry){int x=0,y=0,w=0,h=0;HudVguiGetPos(p.id,&x,&y);HudVguiGetSize(p.id,&w,&h);
        if(x==p.lastBounds.x && y==p.lastBounds.y && w==p.lastBounds.w && h==p.lastBounds.h){HudVguiSetSize(p.id,p.bounds.w,p.bounds.h);HudVguiSetPos(p.id,p.bounds.x,p.bounds.y);}}
    if(p.special)for(int i=0,n=!target?1:p.role==ConsolePage?2:6;i<n;++i){
        const int off=!target?0x1e8:p.role==ConsolePage?0x198+i*4:kFrameFields[i];uint32_t cur=0;
        if(skmem::Copy(&cur,(uint8_t*)p.object+off,4) && cur==p.lastExtra[i])skmem::Write((uint8_t*)p.object+off,&p.extra[i],4);
    }
}
static bool CvarString(Target& s,void* var,const char* value) {
    // Retail ConVar::InternalSetValue(const char*) is primary slot12. It runs
    // synchronously on Paint, avoiding queued commands racing restored flags.
    void* parent=CvarParent(var);
    if(!parent || !s.setString || VGet(parent,12)!=s.setString)return false;
    using Fn=void(__thiscall*)(void*,const char*);((Fn)s.setString)(parent,value);
    char actual[64]{};const char* text=*(const char**)((uint8_t*)parent+0x24);
    return skmem::CopyCStr(actual,sizeof(actual),text) && !strcmp(actual,value);
}
static void ChatCvars(Target& s,const Config& c,bool enabled,bool changed) {
    if(!changed)return;
    s.cvarUnsupported=false;
    for(int i=0;i<3;++i){
        auto& snap=g_chatCvars[i];void* var=CvarFind(snap.name);void* parent=CvarParent(var);
        if(!parent){if(enabled)s.cvarUnsupported=true;continue;}
        char cur[64]{};const char* str=*(const char**)((uint8_t*)parent+0x24);
        if(!skmem::CopyCStr(cur,sizeof(cur),str)){if(enabled)s.cvarUnsupported=true;continue;}
        if(enabled){
            if(snap.object!=parent){snap.object=parent;strcpy_s(snap.before,cur);}
            char want[64]{};
            if(i==0)snprintf(want,sizeof(want),"%d",c.v[SayTime]);
            else {int rgb=c.v[i==1?Survivor:Infected];snprintf(want,sizeof(want),"%d %d %d",rgb>>16,(rgb>>8)&255,rgb&255);}
            if(strcmp(cur,want) && !CvarString(s,var,want)){s.cvarUnsupported=true;continue;}
            strcpy_s(snap.last,want);
        }else if(snap.object==parent){
            if(!strcmp(cur,snap.last))CvarString(s,var,snap.before);
            snap.object=nullptr;snap.before[0]=snap.last[0]=0;
        }
    }
}
static void TickTarget(int target,unsigned wanted,unsigned revision,int sw,int sh,DWORD now) {
    Target& s=g_targets[target];const Config c=NativeUiConfig(target);
    const bool enabled=(wanted&(1u<<target))!=0;
    const DWORD epoch=g_levelInitResetRequestedAt.load(std::memory_order_relaxed);
    const bool changed=revision!=s.revision || sw!=s.sw || sh!=s.sh || enabled!=s.wasEnabled || (!target && epoch!=s.mapEpoch);
    if(!enabled && !s.wasEnabled)return;
    const HMODULE module=GetModuleHandleA(target?"gameui.dll":"client.dll");
    if(s.module && s.module!=module){
        s.count=0;s.resolved=false;s.abiUnsupported=s.cvarUnsupported=false;s.richFont=s.entryFont=s.setString=s.frameScheme=s.setAlpha=nullptr;s.searchAt=0;
        g_nativeUiBindings[target].store(0);g_nativeUiStatus[target].store(enabled?1:0);
        if(!target)for(auto& cv:g_chatCvars){cv.object=nullptr;cv.before[0]=cv.last[0]=0;}
    }
    if(!target && epoch!=s.mapEpoch){s.mapEpoch=epoch;s.searchAt=0;}
    Resolve(s,target);
    if(!module){if(!enabled){s.wasEnabled=false;g_nativeUiStatus[target].store(0);}return;}
    if(!enabled){
        for(int i=0;i<s.count;++i)Restore(s,s.panels[i],target);
        s.count=0;s.searchAt=0;g_nativeUiStatus[target].store(0);g_nativeUiBindings[target].store(0);
    }else {
        g_nativeUiStatus[target].store(1);
        for(int i=0;i<s.count;){if(Alive(s.panels[i],target)){++i;continue;}s.panels[i]=s.panels[--s.count];s.searchAt=0;}
        if(!s.searchAt || (LONG)(now-s.searchAt)>=0){
            s.abiUnsupported=false;
            if(target)DiscoverConsole(s);
            else DiscoverChat(s);
            s.searchAt=now+2000;
        }
        unsigned bindings=0;
        for(int i=0;i<s.count;++i){const auto& p=s.panels[i];bindings|=1u<<p.role;
            if(p.special)bindings|=!target?128u:p.role==Root?256u:p.role==ConsolePage?128u:0u;}
        g_nativeUiBindings[target].store(bindings);
        if(s.count){
            g_nativeUiStatus[target].store((bindings&7u)==7u?2:1);
            if((c.v[FontOverride] && (!s.richFont || !s.entryFont))
                || (!target && c.v[SayTime]==0 && !s.setAlpha)
                || (target && (bindings&7u)==7u && (bindings&(128u|256u))!=(128u|256u)))g_nativeUiStatus[target].store(3);
            bool fontChanged=false;
            if(!Font(s,c,sh,fontChanged))g_nativeUiStatus[target].store(4);
            for(int i=0;i<s.count;++i)Apply(s,s.panels[i],c,target,sw,sh,fontChanged);
        }
    }
    if(!target)ChatCvars(s,c,enabled,changed);
    if(enabled && (s.abiUnsupported || s.cvarUnsupported))g_nativeUiStatus[target].store(3);
    s.wasEnabled=enabled;s.revision=revision;s.sw=sw;s.sh=sh;
}
}
static void NativeUiPaintPump(int mode) {
    using namespace sknativeui_runtime;
    unsigned wanted=g_nativeUiWanted.load(std::memory_order_acquire);
    if(!wanted && !g_targets[0].wasEnabled && !g_targets[1].wasEnabled)return;
    if(!(mode&kPaintUiPanels) || !g_run.load(std::memory_order_relaxed))return;
    const DWORD now=GetTickCount();if(g_tickAt && (LONG)(now-g_tickAt)<0)return;g_tickAt=now+250;
    if(!SkeetoFeaturesOn())wanted=0;
    if(!HudVguiReady() || !g_surf)return;
    if(!IsExec(VGet(g_ipanel,55)))return;
    int sw=0,sh=0;if(!SurfGetScreenSize(&sw,&sh) || sw<=0 || sh<=0)return;
    for(int target=0;target<2;++target){
        __try {TickTarget(target,wanted,g_nativeUiRevision[target].load(std::memory_order_acquire),sw,sh,now);}
        __except(skmem::FaultFilter(GetExceptionCode(),GetExceptionInformation())){
            skmem::NoteFault((uintptr_t)_ReturnAddress());g_nativeUiStatus[target].store(4);
            // Map/UI rebuild can invalidate VPANELs. Drop snapshots, never write
            // through them from the worker or after an owning DLL unload.
            g_targets[target].count=0;g_targets[target].searchAt=now+2000;
        }
    }
}
