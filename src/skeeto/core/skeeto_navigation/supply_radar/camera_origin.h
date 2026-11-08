#pragma once
#include "radar_abi.h"
#include "radar_math.h"
namespace camera_origin {
struct Matrix {float m[4][4];};
static void* configuredEngine;static void* configuredGetter;static void* configuredViewGetter;static void** renderViewSlot;
// Retail VEngineClient013 slot 38 is WorldToViewMatrix(). The local engine
// stub loads the render-view interface and tail-calls its slot 12, returning
// the matrix reference in EAX without stack arguments. Validate the relocated
// pointer as well as all instruction bytes; do not assume SDK slot numbers.
inline bool Configure(void* engine){
    configuredEngine=configuredGetter=configuredViewGetter=nullptr;renderViewSlot=nullptr;
    auto base=(unsigned char*)GetModuleHandleW(L"engine.dll");
    const unsigned char code[]={0x8b,0x0d,0,0,0,0,0x8b,0x01,0x8b,0x50,0x30,0xff,0xe2};
    if(!radar_abi::EngineBuild(base)||!MethodOK(engine,38)||(*(void***)engine)[38]!=base+0x5d630||!Readable(base+0x5d630,sizeof code)||!Readable(base+0x427bc0,4))return false;
    for(unsigned i=0;i<sizeof code;++i)if((i<2||i>5)&&base[0x5d630+i]!=code[i])return false;
    if(*(unsigned*)(base+0x5d632)!=unsigned(base+0x427bc0))return false;
    const unsigned char getter[]={0x83,0xb9,0xe8,0,0,0,0x01,0x7e,0x1a,0x8b,0x81,0xe8,0,0,0,0x8b,0x89,0xdc,0,0,0,0x69,0xc0,0x94,0x01,0,0,0x8d,0x84,0x08,0x3c,0xff,0xff,0xff,0xc3,0x8d,0x41,0x1c,0xc3};
    auto view=*(void**)(base+0x427bc0);
    if(!MethodOK(view,12)||(*(void***)view)[12]!=base+0xc3130||!radar_abi::EqualBytes(base+0xc3130,getter,sizeof getter))return false;
    configuredEngine=engine;configuredGetter=base+0x5d630;configuredViewGetter=base+0xc3130;renderViewSlot=(void**)(base+0x427bc0);return true;
}
inline bool Position(const Matrix& view,Vec& position){
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)if(!Finite(view.m[r][c]))return false;
    // Reject projection, uninitialized, and corrupt matrices. Camera view
    // matrices are rigid transforms: orthonormal rows, affine final row.
    for(unsigned r=0;r<3;++r){float length=0;for(unsigned c=0;c<3;++c)length+=view.m[r][c]*view.m[r][c];if(radar_nav::Abs(length-1.f)>.01f)return false;
        for(unsigned s=r+1;s<3;++s){float dot=0;for(unsigned c=0;c<3;++c)dot+=view.m[r][c]*view.m[s][c];if(radar_nav::Abs(dot)>.01f)return false;}}
    if(radar_nav::Abs(view.m[3][0])>.0001f||radar_nav::Abs(view.m[3][1])>.0001f||radar_nav::Abs(view.m[3][2])>.0001f||radar_nav::Abs(view.m[3][3]-1.f)>.0001f)return false;
    // For view = [R | t], camera world position is -transpose(R)*t.
    float result[3]{};
    for(unsigned c=0;c<3;++c)result[c]=-(view.m[0][c]*view.m[0][3]+view.m[1][c]*view.m[1][3]+view.m[2][c]*view.m[2][3]);
    Vec camera{result[0],result[1],result[2]};if(!radar_nav::Position(camera))return false;position=camera;return true;
}
inline bool ReadView(void* engine,Matrix& copy){
    // Configure verified both fixed executable methods. Keep each live object,
    // current vtable and exact method identity checked here, without querying
    // the same immutable code page's executable protection every frame.
    if(!engine||engine!=configuredEngine||!renderViewSlot||!Readable(renderViewSlot,4))return false;
    void* renderView=*renderViewSlot;
    if(!Readable(renderView,4))return false;auto renderMethods=*(void***)renderView;
    if(!Readable(renderMethods,13*4)||renderMethods[12]!=configuredViewGetter)return false;
    if(!Readable(engine,4))return false;auto engineMethods=*(void***)engine;
    if(!Readable(engineMethods,39*4)||engineMethods[38]!=configuredGetter)return false;
    auto view=VCall<const Matrix*(__thiscall*)(void*)>(engine,38)(engine);
    if(!Readable(view,sizeof(Matrix)))return false;copy=*view;return true;
}
inline bool Read(void* engine,Vec& position){
    Matrix copy{};return ReadView(engine,copy)&&Position(copy,position);
}
}
