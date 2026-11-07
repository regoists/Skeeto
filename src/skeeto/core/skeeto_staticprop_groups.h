// Retail x86 completion-only repair for static props registered before mesh
// metadata was ready. One bounded pass on the loading/game thread; no mesh
// load, allocation, hook, retained map pointer or worker. Missing
// material metadata uses the same native preparation as VVD-ready notification.
// That native material lookup may allocate/read; full-load costs include it.
#pragma once
#include "skeeto_memread.h"
namespace skload {
struct StaticPropGroups {
    using SetGroup=void(__thiscall*)(void*,unsigned short,int);
    using Initialize=void(__thiscall*)(void*,void*,unsigned short);
    uint8_t* engine=nullptr;
    Initialize initialize=nullptr;
    SetGroup setter=nullptr;
    unsigned failures=0;
    bool Bind(uint8_t* e,size_t engineBytes,uint8_t* c,size_t clientBytes) {
        if(!e || engineBytes<0x6e0c18 || !c || clientBytes<0xbd3b4)return false;
        uint8_t factory[]{0xb8,0,0,0,0,0xc3};const uintptr_t manager=uintptr_t(e+0x4333b8);memcpy(factory+1,&manager,4);
        const uint8_t getter[]{0x55,0x8b,0xec,0x8b,0x45,8,0x85,0xc0,0x74,0x12,0xf6,0x80,0x14,1,0,0,2};
        const uint8_t method[]{0x55,0x8b,0xec,0x66,0x8b,0x45,8,0xba,0xff,0xff,0,0,0x66,0x3b,0xc2,0x74,0x1f,0x0f,0xb7,0xc0,0x6b,0xc0,0x4c,3,0x41,0x24,0x8b,0x4d,0x0c,0xc1,0xe1,0x0c,0x66,0x33,0x48,0x16,0xba,0,0x30,0,0,0x66,0x23,0xca,0x66,0x31,0x48,0x16,0x5d,0xc2,8,0};
        uint8_t prepare[]{0x55,0x8b,0xec,0x81,0xec,0,2,0,0,0x8b,0x0d,0,0,0,0,0x8b,1,0x8b,0x55,0x0c,0x8b,0x40,0x28,0x53,0x56,0x57,0x52,0xff,0xd0,0x8b,0x75,8};
        const uintptr_t cacheGlobal=uintptr_t(e+0x6e0c14);memcpy(prepare+11,&cacheGlobal,4);
        uint8_t actual[sizeof(method)]{};
        if(!skmem::Copy(actual,e+0x1f4200,sizeof(factory)) || memcmp(actual,factory,sizeof(factory))
            || !skmem::Copy(actual,e+0x1aff30,sizeof(getter)) || memcmp(actual,getter,sizeof(getter))
            || !skmem::Copy(actual,e+0xe9430,sizeof(prepare)) || memcmp(actual,prepare,sizeof(prepare))
            || !skmem::Copy(actual,c+0xbd380,sizeof(method)) || memcmp(actual,method,sizeof(method)))return false;
        engine=e;initialize=reinterpret_cast<Initialize>(e+0xe9430);setter=reinterpret_cast<SetGroup>(c+0xbd380);return true;
    }
    bool Repair() {
        // Native manager owns this map's array. Its leaf handles are 16-bit;
        // no traversal is admitted beyond that native representable bound.
        struct {uintptr_t array;int capacity,grow,count;} props{};
        if(!skmem::Copy(&props,engine+0x4333b8+0x20,sizeof(props)) || props.count<0 || props.count>65535) {++failures;return false;}
        if(!props.count)return true;
        uintptr_t leaf=0,table=0,target=0,nodes=0;
        if(!skmem::Copy(&leaf,engine+0x4ee790,4) || !leaf
            || !skmem::Copy(&table,reinterpret_cast<void*>(leaf),4)
            || !skmem::Copy(&target,reinterpret_cast<void*>(table+3*4),4) || target!=uintptr_t(setter)
            || !skmem::Copy(&nodes,reinterpret_cast<void*>(leaf+0x24),4)) {++failures;return false;}
        for(int i=0;i<props.count;++i) {
            uint8_t prop[0x50]{};
            if(!skmem::Copy(prop,reinterpret_cast<void*>(props.array+unsigned(i)*0xd8),sizeof(prop))) {++failures;return false;}
            uintptr_t model=0;uint16_t handle=0;memcpy(&model,prop+0x28,4);memcpy(&handle,prop+0x4c,2);
            if(!model || handle==0xffff)continue;
            struct {int type;unsigned flags;} metadata{};
            if(!skmem::Copy(&metadata,reinterpret_cast<void*>(model+0x110),sizeof(metadata))) {++failures;return false;}
            if(metadata.type!=3)continue;
            // Mesh async omits the VVD-ready notification that normally
            // populates model flags. Its native material-only preparation
            // enumerates studio-header materials; it does not request HWD.
            if(!metadata.flags) {
                uint16_t mdl=0;
                if(!skmem::Copy(&mdl,reinterpret_cast<void*>(model+0x134),2)) {++failures;return false;}
                initialize(engine+0x428300,reinterpret_cast<void*>(model),mdl);
                if(!skmem::Copy(&metadata.flags,reinterpret_cast<void*>(model+0x114),4)) {++failures;return false;}
            }
            if(!(metadata.flags&2u))continue;
            uint16_t flags=0;
            if(!skmem::Copy(&flags,reinterpret_cast<void*>(nodes+unsigned(handle)*0x4c+0x16),2)) {++failures;return false;}
            // Preserve a nonzero native, potentially skin/body-specific result.
            // Native cached flags use masks 0x2=translucent, 0x8=two passes.
            if(flags&0x3000u)continue;
            setter(reinterpret_cast<void*>(leaf),handle,(metadata.flags&8u)?2:1);
        }
        return true;
    }
} inline g_staticPropGroups;
} // namespace skload
