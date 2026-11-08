"""Exercise the retail x86 static-prop classification completion pass."""
import test_fast_load as native
native.FIXTURE = r'''
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "skeeto_fast_load.h"
static void require(bool ok,const char* why) {if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static unsigned preparations=0;static void* expectedLoader=nullptr;
static bool integration=false,drained=false;
static void __fastcall PrepareFixture(void* loader,void*,void* model,unsigned short handle) { if(integration)require(drained,"native pending jobs drain before material initialization"); require(loader==expectedLoader,"native material initialization preserves loader this pointer");require(handle==17,"native material initialization receives MDL handle");++preparations;*reinterpret_cast<unsigned*>(static_cast<uint8_t*>(model)+0x114)=0x2006; }
static int leased[3]{};static uint16_t* completionGroup=nullptr;
static unsigned SwitchSlot(const char* name) {if(!std::strcmp(name,"mod_load_mesh_async"))return 0;if(!std::strcmp(name,"mod_load_vcollide_async"))return 1;return 2;}
static bool ReadSwitch(const char* name,int* n,float* f) {unsigned i=SwitchSlot(name);*n=leased[i];*f=float(*n);return true;}
static bool WriteSwitch(const char* name,int n,float) {if(!n)require(drained && (*completionGroup&0x3000)==0x1000,"repair completes before restoring native async numbers");leased[SwitchSlot(name)]=n;return true;}
static void __fastcall DrainFixture(void*,void*) {require(leased[0]==1,"drain runs with mesh lease intact");drained=true;}
int main() {
 auto* engine=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x700000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
 auto* client=static_cast<uint8_t*>(VirtualAlloc(nullptr,0xc0000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
 require(engine&&client,"allocate fixture images");
 uint8_t factory[]{0xb8,0,0,0,0,0xc3};uintptr_t manager=uintptr_t(engine+0x4333b8);memcpy(factory+1,&manager,4);memcpy(engine+0x1f4200,factory,sizeof(factory));
 const uint8_t getter[]{0x55,0x8b,0xec,0x8b,0x45,8,0x85,0xc0,0x74,0x12,0xf6,0x80,0x14,1,0,0,2};memcpy(engine+0x1aff30,getter,sizeof(getter));
 const uint8_t setter[]{0x55,0x8b,0xec,0x66,0x8b,0x45,8,0xba,0xff,0xff,0,0,0x66,0x3b,0xc2,0x74,0x1f,0x0f,0xb7,0xc0,0x6b,0xc0,0x4c,3,0x41,0x24,0x8b,0x4d,0x0c,0xc1,0xe1,0x0c,0x66,0x33,0x48,0x16,0xba,0,0x30,0,0,0x66,0x23,0xca,0x66,0x31,0x48,0x16,0x5d,0xc2,8,0};
 memcpy(client+0xbd380,setter,sizeof(setter));
 uint8_t prepare[]{0x55,0x8b,0xec,0x81,0xec,0,2,0,0,0x8b,0x0d,0,0,0,0,0x8b,1,0x8b,0x55,0x0c,0x8b,0x40,0x28,0x53,0x56,0x57,0x52,0xff,0xd0,0x8b,0x75,8};uintptr_t cacheGlobal=uintptr_t(engine+0x6e0c14);memcpy(prepare+11,&cacheGlobal,4);memcpy(engine+0xe9430,prepare,sizeof(prepare));
 uintptr_t table[4]{};table[3]=uintptr_t(client+0xbd380);alignas(4)uint8_t leaf[0x30]{},nodes[5*0x4c]{},props[5*0xd8]{},models[5*0x138]{};
 *reinterpret_cast<uintptr_t*>(leaf)=uintptr_t(table);*reinterpret_cast<uintptr_t*>(leaf+0x24)=uintptr_t(nodes);
 *reinterpret_cast<uintptr_t*>(engine+0x4ee790)=uintptr_t(leaf);*reinterpret_cast<uintptr_t*>(manager+0x20)=uintptr_t(props);*reinterpret_cast<int*>(manager+0x2c)=5;
 const unsigned flags[5]{0,0x200e,0x2004,0x2006,0x2006};
 for(unsigned i=0;i<5;++i){auto* m=models+i*0x138;*reinterpret_cast<int*>(m+0x110)=3;*reinterpret_cast<unsigned*>(m+0x114)=flags[i];*reinterpret_cast<uint16_t*>(m+0x134)=17;*reinterpret_cast<uintptr_t*>(props+i*0xd8+0x28)=uintptr_t(m);*reinterpret_cast<uint16_t*>(props+i*0xd8+0x4c)=uint16_t(i);*reinterpret_cast<uint16_t*>(nodes+i*0x4c+0x16)=0x8a5;}
 *reinterpret_cast<uint16_t*>(nodes+3*0x4c+0x16)|=0x2000;
 *reinterpret_cast<uint16_t*>(props+4*0xd8+0x4c)=0xffff;
 skload::StaticPropGroups repair;
 require(repair.Bind(engine,0x700000,client,0xc0000),"admit exact retail factory, cached-category getter and native setter");
 expectedLoader=engine+0x428300;
 repair.initialize=reinterpret_cast<skload::StaticPropGroups::Initialize>(&PrepareFixture);
 require(repair.Repair(),"repair at completion");
 require(preparations==1,"only missing material metadata is initialized, without a hardware getter");
 require(*reinterpret_cast<uint16_t*>(nodes+0x16)==0x18a5,"translucent mesh gets native group 1 and retains other bits");
 require(*reinterpret_cast<uint16_t*>(nodes+0x4c+0x16)==0x28a5,"two-pass mesh gets native group 2");
 require(*reinterpret_cast<uint16_t*>(nodes+2*0x4c+0x16)==0x8a5,"opaque mesh remains unchanged");
 require(*reinterpret_cast<uint16_t*>(nodes+3*0x4c+0x16)==0x28a5,"nonzero native skin/body category remains authoritative");
 require(*reinterpret_cast<uint16_t*>(nodes+4*0x4c+0x16)==0x8a5,"invalid render handle is skipped");
 require(repair.Repair() && preparations==1,"duplicate explicit completion does not repeat material preparation");
 *reinterpret_cast<int*>(manager+0x2c)=65536;require(!repair.Repair(),"native 16-bit renderable limit bounds work");
 *reinterpret_cast<int*>(manager+0x2c)=0;require(repair.Repair(),"empty map has no traversal");
 *reinterpret_cast<int*>(manager+0x2c)=5;table[3]++;require(!repair.Repair(),"foreign setter is not called");table[3]--;
 require(!repair.Bind(engine,0x1000,client,0xc0000),"reject truncated image");
 client[0xbd380]^=1;require(!repair.Bind(engine,0x700000,client,0xc0000),"reject changed retail ABI");
 client[0xbd380]^=1; // Restore the deliberately changed ABI for integration.
 skload::g_staticPropGroups=repair;
 *reinterpret_cast<unsigned*>(models+0x114)=0;completionGroup=reinterpret_cast<uint16_t*>(nodes+0x16);*completionGroup&=~0x3000u;
 auto& preload=skload::g_nativePreload;preload.object=engine;preload.read=ReadSwitch;preload.write=WriteSwitch;preload.finish=reinterpret_cast<decltype(preload.finish)>(&DrainFixture);
 integration=true;skload::g_nativeAsyncWant.store(7);preload.Begin();require(preload.active && leased[0]==1 && leased[1]==1 && leased[2]==1,"three native leases stay enabled");
 require(preload.Complete() && !preload.active && !leased[0] && !leased[1] && !leased[2] && preparations==2,"production completion drains, prepares, classifies and restores");
 require(preload.Complete() && preparations==2,"disabled completion has no extra work");
 VirtualFree(client,0,MEM_RELEASE);VirtualFree(engine,0,MEM_RELEASE);
 std::puts("PASS completion-only static props: native setter ABI/categories/bounded/idempotent/foreign slot");
}
'''
if __name__ == '__main__':native.main()
