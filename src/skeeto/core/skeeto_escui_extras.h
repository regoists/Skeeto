#pragma once
// Included after the ESC text helpers. Paint owns these snapshots/actions;
// the preset worker receives fixed value copies, never Source pointers.
static int g_escUiExtraPanel=0,g_escUiPresetSlot=0,g_escUiRenameSlot=-1;
static MainUiTextRequest g_escUiRenameRequest=0;
static wchar_t g_escUiExtraNotice[192]{};
static DWORD g_escUiExtraNoticeUntil=0,g_escUiServerAt=0;
static wchar_t g_escUiServerText[5][192]{};
static char g_escUiServerAddress[96]{};
static unsigned g_escUiServerFaults=0;
static int g_escUiPresetSkipped=0;
static void EscUiExtraNotice(const wchar_t* text){wcsncpy_s(g_escUiExtraNotice,text,_TRUNCATE);g_escUiExtraNoticeUntil=GetTickCount()+6000u;}
static void EscUiExtrasClose(){
    g_escUiExtraPanel=0;g_escUiServerAt=0;g_escUiServerAddress[0]=0;
    MainUiServicesCancelText(g_escUiRenameRequest);g_escUiRenameRequest=0;g_escUiRenameSlot=-1;
}
static void EscUiExtrasPump(){
    if(skescpreset::busy){int result=skescpreset::result.exchange(0,std::memory_order_acq_rel);
        if(result){skescpreset::busy=false;
            if(result==1&&g_escUiPresetSkipped){wchar_t text[192]{};swprintf_s(text,UiText(UiId::EscPresetSkipped,L"预设已保存；%d 个滑块的变量不可读取，未收入预设"),g_escUiPresetSkipped);EscUiExtraNotice(text);}
            else EscUiExtraNotice(result==1?UiText(UiId::EscPresetSaved,L"ConVar 预设已保存"):UiText(UiId::EscPresetSaveError,L"预设保存失败，磁盘原件保留"));g_escUiPresetSkipped=0;}}
    if(g_escUiRenameSlot>=0){wchar_t text[64]{};
        auto result=MainUiServicesTakeText(g_escUiRenameRequest,text,64);
        if(result!=MainUiTextResult::None){g_escUiRenameRequest=0;
            int slot=g_escUiRenameSlot;g_escUiRenameSlot=-1;
            if(result==MainUiTextResult::Accepted){wcscpy_s(skescpreset::bank.slots[slot].name,text);
            if(!skescpreset::Save())EscUiExtraNotice(UiText(UiId::EscPresetBusy,L"预设忙碌或文件损坏，当前操作未保存"));}
        }
    }
}
static void EscUiServerSample(){
    DWORD now=GetTickCount();if(g_escUiServerAt&&(DWORD)(now-g_escUiServerAt)<1000u)return;g_escUiServerAt=now;
    MainUiServerRecord server{};
    if(!TryAcquireSRWLockShared(&g_mainUiServerLock))return;
    server=g_mainUiCurrentServer;ReleaseSRWLockShared(&g_mainUiServerLock);
    void* channel=EngNetChanFromClient();
    const char* address=channel?ListenChanAddr(channel):"";
    strncpy_s(g_escUiServerAddress,address&&address[0]?address:server.address,_TRUNCATE);
    bool same=!server.address[0]||!address||!address[0]||!_stricmp(server.address,address)
        ||(ListenAddrIsLoopback(server.address)&&ListenAddrIsLoopback(address));
    const wchar_t* missing=UiText(UiId::EscInfoMissing,L"未获取");
    wchar_t name[192]{},map[96]{},endpoint[96]{};
    if(same&&server.name[0])MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,server.name,-1,name,192);
    if(g_elimLastMap[0])MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,g_elimLastMap,-1,map,96);
    if(g_escUiServerAddress[0])MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,g_escUiServerAddress,-1,endpoint,96);
    swprintf_s(g_escUiServerText[0],UiText(UiId::EscInfoServer,L"服务器：%.150s"),name[0]?name:missing);
    swprintf_s(g_escUiServerText[1],UiText(UiId::EscInfoMap,L"地图：%s"),map[0]?map:missing);
    float ping=channel?NetGraphHudChanFloat(channel,10,0,0.f,10.f):-1.f;
    if(ping>=0)swprintf_s(g_escUiServerText[2],UiText(UiId::EscInfoPing,L"延迟：%.0f ms"),double(ping*1000.f));
    else swprintf_s(g_escUiServerText[2],UiText(UiId::EscInfoPingMissing,L"延迟：%s"),missing);
    using InfoFn=bool(__thiscall*)(void*,int,PlayerInfo*);auto info=(InfoFn)VGet(g_engine,8);
    int players=0;bool have=info&&IsExec((void*)info);
    // Validate the getter once, rather than calling the probed wrapper 64 times.
    if(have){__try{for(int i=1;i<=64;++i){PlayerInfo p{};if(info(g_engine,i,&p)&&p.userid>0&&p.name[0]){if(!PlayerInfoIsBot(p))++players;}}}
        __except(EXCEPTION_EXECUTE_HANDLER){have=false;++g_escUiServerFaults;}}
    int maximum=g_escUiServerMax.load(std::memory_order_relaxed);
    if(have&&maximum>0)swprintf_s(g_escUiServerText[3],UiText(UiId::EscInfoPlayersMax,L"人数：%d/%d"),players,maximum);
    else if(have)swprintf_s(g_escUiServerText[3],UiText(UiId::EscInfoPlayers,L"人数：%d"),players);
    else swprintf_s(g_escUiServerText[3],UiText(UiId::EscInfoPlayersMissing,L"人数：%s"),missing);
    swprintf_s(g_escUiServerText[4],UiText(UiId::EscInfoAddress,L"IP：%s"),endpoint[0]?endpoint:missing);
}
// Read-only components share Paint snapshots. Server reads are 1 Hz while
// visible; clocks use wall/round time, and names use the existing mission cache.
static void EscUiComponentText(const skesc::Item& b,wchar_t (&out)[192]){
    const char* parameter=b.parameter;
    if(b.action==skesc::kServerInfo){
        EscUiServerSample();const char* fields[]={"server","map","ping","players","address"};
        for(int i=0;i<5;++i)if(!strcmp(parameter,fields[i])){wcscpy_s(out,g_escUiServerText[i]);return;}
        swprintf_s(out,L"%.30s / %.30s / %.30s / %.30s / %.30s",g_escUiServerText[0],g_escUiServerText[1],g_escUiServerText[2],g_escUiServerText[3],g_escUiServerText[4]);return;
    }
    if(b.action==skesc::kClock){SYSTEMTIME t{};GetLocalTime(&t);
        if(!strcmp(parameter,"date"))swprintf_s(out,L"%04u-%02u-%02u",t.wYear,t.wMonth,t.wDay);
        else if(!strcmp(parameter,"full"))swprintf_s(out,L"%04u-%02u-%02u %02u:%02u:%02u",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
        else swprintf_s(out,L"%02u:%02u:%02u",t.wHour,t.wMinute,t.wSecond);return;
    }
    if(b.action==skesc::kRoundTimer){unsigned sec=RoundElapsedMs()/1000u;
        if(!strcmp(parameter,"seconds"))swprintf_s(out,UiText(UiId::EscTimerSeconds,L"本局 %u s"),sec);
        else swprintf_s(out,UiText(UiId::EscTimerValue,L"本局 %02u:%02u:%02u"),sec/3600u,sec/60u%60u,sec%60u);return;
    }
    char stem[80]{};MapHudStem(g_elimLastMap,stem,80);int camp=0,chapter=0;MapHudParse(stem,&camp,&chapter);
    wchar_t code[80]{};MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,stem,-1,code,80);
    const wchar_t* campaign=MapHudCampaignName(camp);if(!campaign)campaign=MapHudMissionName(stem,false);
    const wchar_t* level=MapHudMissionName(stem,true);if(!campaign)campaign=code;if(!level)level=code;
    if(!strcmp(parameter,"campaign"))wcsncpy_s(out,campaign,_TRUNCATE);
    else if(!strcmp(parameter,"chapter"))wcsncpy_s(out,level,_TRUNCATE);
    else if(!strcmp(parameter,"code"))wcscpy_s(out,code);
    else swprintf_s(out,L"%.55s / %.55s / %.65s",campaign,level,code);
}
static void EscUiCopyCommand(const wchar_t* command){
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,(wcslen(command)+1)*sizeof(wchar_t));bool ok=false;
    if(memory){void* target=GlobalLock(memory);if(target){memcpy(target,command,(wcslen(command)+1)*sizeof(wchar_t));GlobalUnlock(memory);
        if(OpenClipboard(g_gameHwnd)){ok=EmptyClipboard()&&SetClipboardData(CF_UNICODETEXT,memory)!=nullptr;CloseClipboard();}}
        if(!ok)GlobalFree(memory);}
    EscUiExtraNotice(ok?UiText(UiId::EscCommandCopied,L"命令已复制"):UiText(UiId::EscInfoCopyError,L"剪贴板暂不可用，命令未复制"));
}
static void EscUiCopyServer(){
    EscUiServerSample();
    if(!g_escUiServerAddress[0]){EscUiExtraNotice(UiText(UiId::EscInfoNoAddress,L"当前没有可复制的连接地址"));return;}
    wchar_t address[96]{},command[112]{};MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,g_escUiServerAddress,-1,address,96);
    swprintf_s(command,L"connect %s",address);EscUiCopyCommand(command);
}
static void EscUiCopyMap(){
    char stem[80]{};MapHudStem(g_elimLastMap,stem,80);
    if(!stem[0]){EscUiExtraNotice(UiText(UiId::EscMapMissing,L"当前没有可复制的建图代码"));return;}
    wchar_t code[80]{},command[96]{};MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,stem,-1,code,80);
    swprintf_s(command,L"map %s",code);EscUiCopyCommand(command);
}
static bool EscUiCapturePreset(){
    static skescpreset::Preset capture{};capture=skescpreset::Preset{};int skipped=0;
    wcscpy_s(capture.name,skescpreset::bank.slots[g_escUiPresetSlot].name);
    for(int i=0;i<g_escUiLayout.count;++i){const auto& item=g_escUiLayout.items[i];if(!skesc::IsSlider(item.action))continue;
        skmenu::CvarSpec spec{};if(!skmenu::CvarParse(item.parameter,&spec))continue;
        bool duplicate=false;for(int j=0;j<capture.count;++j)if(!_stricmp(capture.values[j].name,spec.name))duplicate=true;if(duplicate)continue;
        void* var=CvarFind(spec.name);float value=0;if(!var||!CvarReadPair(var,nullptr,&value)||!_finite(value)){++skipped;continue;}
        if(capture.count==skescpreset::Entries){EscUiExtraNotice(UiText(UiId::EscPresetLimit,L"预设最多保存 64 个不同变量，本次未保存"));return false;}
        auto& v=capture.values[capture.count++];strcpy_s(v.name,spec.name);v.value=value;v.pinned=item.action==skesc::kPinnedSlider?1:0;
    }
    if(!capture.count){EscUiExtraNotice(UiText(UiId::EscPresetEmptyCapture,L"当前布局没有可读取的 ConVar 滑块"));return false;}
    skescpreset::bank.slots[g_escUiPresetSlot]=capture;g_escUiPresetSkipped=skipped;return skescpreset::Save();
}
static void EscUiPresetAction(int action){
    if(action==2111){g_escUiExtraPanel=0;return;}
    if(action>=2100&&action<2108){g_escUiPresetSlot=action-2100;return;}
    if(skescpreset::busy||!skescpreset::readable){EscUiExtraNotice(UiText(UiId::EscPresetBusy,L"预设忙碌或文件损坏，当前操作未保存"));return;}
    auto& p=skescpreset::bank.slots[g_escUiPresetSlot];
    if(action==2109){EscUiCapturePreset();return;}
    if(action==2110){
        char caption[128]{};WideCharToMultiByte(CP_UTF8,0,UiText(UiId::EscPresetRename,L"命名 ConVar 预设"),-1,caption,128,nullptr,nullptr);
        auto request=MainUiServicesText(p.name,caption,63);
        if(request){g_escUiRenameRequest=request;g_escUiRenameSlot=g_escUiPresetSlot;}
        else EscUiExtraNotice(UiText(UiId::EscPresetBusy,L"预设忙碌或文件损坏，当前操作未保存"));return;
    }
    if(action==2108){
        if(!p.count){EscUiExtraNotice(UiText(UiId::EscPresetEmpty,L"该预设为空，请先保存当前滑块值"));return;}
        int attempted=0,failed=0;
        for(int i=0;i<p.count;++i){const auto& v=p.values[i];
            if(v.pinned){if(SktCvarMenuWrite(v.name,v.value))++attempted;else ++failed;}
            else if(CvarFind(v.name)){char command[128]{};snprintf(command,sizeof(command),"%s %.9g",v.name,double(v.value));EngClientCmdUnrestricted(command);++attempted;}
            else ++failed;
        }
        g_escUiTextDirty=true;wchar_t text[192]{};swprintf_s(text,UiText(UiId::EscPresetApplied,L"已请求应用 %d 项；%d 项未应用，最终值由游戏/服务器决定"),attempted,failed);EscUiExtraNotice(text);
    }
}
