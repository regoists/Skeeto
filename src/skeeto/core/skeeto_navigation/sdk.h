#pragma once
// Data layouts used by the authorized L4N_Minimap 0.1.19 algorithms.
// This is a Skeeto-owned Win32/MSVC adapter; it does not load the L4N platform.
#include <Windows.h>
#include <cstddef>
#include <cmath>
#include <cstring>
#include "../skeeto_memread.h"

inline bool Same(const char* a,const char* b){return a&&b&&std::strcmp(a,b)==0;}
inline void WideCopy(WCHAR* out,unsigned cap,const WCHAR* s){if(!out||!cap)return;unsigned n=0;while(s&&s[n]&&n+1<cap){out[n]=s[n];++n;}out[n]=0;}
inline void CopyText(char* out,unsigned cap,const char* s){if(!out||!cap)return;unsigned n=0;while(s&&s[n]&&n+1<cap){out[n]=s[n];++n;}out[n]=0;}
inline int Limit(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
inline float Limit(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
inline bool Finite(float v){return std::isfinite(v)!=0;}
// Guarded reads, not page queries: cost does not depend on the surrounding heap region.
__declspec(noinline) inline bool Readable(const void* p,unsigned bytes){return skmem::SpanProbeAt(p,bytes,(uintptr_t)_ReturnAddress());}
__declspec(noinline) inline bool Executable(const void* p){return skmem::ExecutableAt(p,(uintptr_t)_ReturnAddress());}
template<class F>inline F VCall(void* p,int i){return reinterpret_cast<F>((*reinterpret_cast<void***>(p))[i]);}
inline bool MethodOK(void* p,int i){return Readable(p,4)&&Readable(*(void***)p,(i+1)*4)&&Executable((*(void***)p)[i]);}
inline void* GetInterface(const wchar_t* module,const char* name){auto h=GetModuleHandleW(module);using Factory=void*(__cdecl*)(const char*,int*);auto f=h?(Factory)GetProcAddress(h,"CreateInterface"):nullptr;return f?f(name,nullptr):nullptr;}
struct Vec{float x=0,y=0,z=0;Vec operator+(Vec b)const{return{x+b.x,y+b.y,z+b.z};}Vec operator-(Vec b)const{return{x-b.x,y-b.y,z-b.z};}Vec operator*(float a)const{return{x*a,y*a,z*a};}float Dot(Vec b)const{return x*b.x+y*b.y+z*b.z;}};
struct Matrix{float m[4][4];};
struct RecvTable;
struct RecvProp{const char* name;int type,flags,stringSize;bool inside;const void* extra;RecvProp* array;void* arrayLength;void* proxy;void* tableProxy;RecvTable* table;int offset,stride,count;const char* parentName;};
struct RecvTable{RecvProp* props;int count;void* decoder;const char* name;bool initialized,inList;};
struct ClientClass{void* create;void* createEvent;const char* name;RecvTable* table;ClientClass* next;int id;};
struct alignas(16) Vec4{float x,y,z,w;};
struct alignas(16) Ray{Vec4 start,delta,offset,extents;const void* worldAxisTransform;bool isRay,isSwept;};
struct Trace{Vec start,end,normal;float planeDistance;unsigned char planeType,signBits,pad[2];float fraction;int contents;unsigned short flags;bool allSolid,startSolid;float leftSolid;const char* surfaceName;short surfaceProps;unsigned short surfaceFlags;int hitgroup;short physicsBone;unsigned short padding;void* entity;int hitbox;};
class TraceFilter{public:void* skip=nullptr;virtual bool ShouldHitEntity(void* p,int){return p!=skip;}virtual int GetTraceType()const{return 0;}};
static_assert(sizeof(void*)==4&&sizeof(RecvProp)==60&&sizeof(Trace)==84&&sizeof(Ray)==80,"L4D2 x86 ABI required");
