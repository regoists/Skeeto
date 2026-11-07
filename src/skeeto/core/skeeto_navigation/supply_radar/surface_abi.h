#pragma once
#include "radar_abi.h"
// Installed L4D2 VGUI_Surface031 differs from Source SDK 2013's six-argument
// DrawSetTextureRGBA. Slot 28 is RVA 0x9A60, ending C2 10 00 at RVA 0x9A86:
// exactly four stack arguments. The host upload (SkeetoNavUploadRgba) relies on
// this four-argument contract; the six-argument SDK call would corrupt the stack.
inline bool TextureUploadABI(void* surface){
    auto base=(unsigned char*)GetModuleHandleW(L"vguimatsurface.dll");
    if(!MethodOK(surface,28)||(*(void***)surface)[28]!=base+0x9a60)return false;
    const unsigned char expected[]={0x55,0x8b,0xec,0xe8,0x58,0xd6,0,0,0x8b,0x4d,0x14,0x8b,0x10,0x8b,0x52,0x40,0x6a,0,0x6a,0,0x51,0x8b,0x4d,0x10,0x51,0x8b,0x4d,0x0c,0x51,0x8b,0x4d,8,0x51,0x8b,0xc8,0xff,0xd2,0x5d,0xc2,0x10,0};
    if(!Readable(base+0x9a60,sizeof expected))return false;
    for(unsigned i=0;i<sizeof expected;++i)if(base[0x9a60+i]!=expected[i])return false;
    return true;
}
// Slot 33 forwards one ID to the texture dictionary's destroy method (slot 1).
// Recreate on size changes, avoiding stale procedural texture allocations.
inline bool TextureReleaseABI(void* surface){
    auto base=(unsigned char*)GetModuleHandleW(L"vguimatsurface.dll");
    const unsigned char bytes[]={0x55,0x8b,0xec,0xe8,0x28,0xec,0,0,0x8b,0x4d,0x08,0x8b,0x10,0x8b,0x52,0x04,0x51,0x8b,0xc8,0xff,0xd2,0x32,0xc0,0x5d,0xc2,0x04,0};
    if(!base||!MethodOK(surface,33)||(*(void***)surface)[33]!=base+0x8490||!Readable(base+0x8490,sizeof bytes))return false;
    for(unsigned i=0;i<sizeof bytes;++i)if(base[0x8490+i]!=bytes[i])return false;
    return true;
}
// Retail Surface031 GetFontTall is slot 66, not the SDK 2013 slot number.
// RVA 0x8670 forwards one HFont to FontManager and ends in RET 4. Its height
// becomes zero after the engine clears the font's glyph mapping during a video
// mode change; the plugin can then rebuild only its own font handles.
inline bool FontMetricsABI(void* surface){
    auto base=(unsigned char*)GetModuleHandleW(L"vguimatsurface.dll");
    if(!base||!MethodOK(surface,66)||(*(void***)surface)[66]!=base+0x8670)return false;
    const unsigned char bytes[]={0x55,0x8b,0xec,0x8b,0x45,0x08,0x50,0xe8,0xa4,0xd8,0x00,0x00,0x8b,0xc8,0xe8,0xed,0xdb,0x00,0x00,0x5d,0xc2,0x04,0x00};
    if(!Readable(base+0x8670,sizeof bytes))return false;
    for(unsigned i=0;i<sizeof bytes;++i)if(base[0x8670+i]!=bytes[i])return false;
    return true;
}
// Retail Surface031 slot 72 forwards (HFont, wchar_t*, width&, height&)
// to FontManager::GetTextSize at RVA 0x16590 and ends in RET 16.
// Use the selected font's measured advances, not an estimated digit width.
inline bool TextSizeABI(void* surface){
    auto base=(unsigned char*)GetModuleHandleW(L"vguimatsurface.dll");
    const unsigned char bytes[]={0x55,0x8b,0xec,0x8b,0x45,0x14,0x8b,0x4d,0x10,0x8b,0x55,0x0c,0x50,0x8b,0x45,0x08,0x51,0x52,0x50,0xe8,0x98,0xd7,0,0,0x8b,0xc8,0xe8,0x01,0xde,0,0,0x5d,0xc2,0x10,0};
    if(!base||!MethodOK(surface,72)||(*(void***)surface)[72]!=base+0x8770||!Readable(base+0x8770,sizeof bytes))return false;
    for(unsigned i=0;i<sizeof bytes;++i)if(base[0x8770+i]!=bytes[i])return false;
    return true;
}
inline bool EntityScanABI(void* entities){
    // VClientEntityList003 slot 6, verified in the installed client.dll:
    // mov eax,[ecx+24h]; ret. No stack arguments.
    auto base=(unsigned char*)GetModuleHandleW(L"client.dll");
    return MethodOK(entities,6)&&(*(void***)entities)[6]==base+0xb8db0&&Readable(base+0xb8db0,4)&&*(unsigned*)(base+0xb8db0)==0xc324418b;
}
inline bool EntityHandleABI(void* entities){
    auto base=(unsigned char*)GetModuleHandleW(L"client.dll");
    // VClientEntityList003 resolves a full handle through slot 2 (including
    // its serial), then obtains IClientEntity. Never mask a stale parent handle
    // into an entity index and accidentally follow a replacement object.
    const unsigned char bytes[]={0x55,0x8b,0xec,0x8b,0x55,0x08,0x51,0x8b,0xc4,0x89,0x10,0x8b,0x01,0x8b,0x50,0x08,0xff,0xd2,0x85,0xc0,0x74,0x0d,0x8b,0x10,0x8b,0xc8,0x8b,0x42,0x18,0xff,0xd0,0x5d,0xc2,0x04,0x00};
    if(!MethodOK(entities,4)||(*(void***)entities)[4]!=base+0xb8e90||!Readable(base+0xb8e90,sizeof bytes))return false;
    for(unsigned i=0;i<sizeof bytes;++i)if(base[0xb8e90+i]!=bytes[i])return false;
    const unsigned char serialCheck[]={0xc1,0xea,0x0c,0x39,0x50,0x04,0x75,0x06,0x8b,0x00,0x5d,0xc2,0x04,0x00};
    if(!MethodOK(entities,2)||(*(void***)entities)[2]!=base+0xbb4c0||!Readable(base+0xbb4de,sizeof serialCheck))return false;
    for(unsigned i=0;i<sizeof serialCheck;++i)if(base[0xbb4de+i]!=serialCheck[i])return false;
    return true;
}
