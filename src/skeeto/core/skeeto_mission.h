#pragma once
#include <cstring>
#include <cstddef>

// Small mission-only KeyValues reader: no engine calls, heap allocation or IO.
namespace skmission {
constexpr int kMaxMaps = 32;
struct Chapters {
	char maps[kMaxMaps][80]{}, title[260]{}, names[kMaxMaps][260]{};
	int count = 0;
	bool bad = false;
};
inline bool Equal(const char* a, const char* b) {
	while (*a && *b) { char x=*a++, y=*b++; if(x>='A'&&x<='Z')x+=32; if(y>='A'&&y<='Z')y+=32; if(x!=y)return false; }
	return *a==*b;
}
inline bool Stem(const char* path, char (&out)[80]) {
	const char* start=path;
	for(const char* p=path;*p;++p)if(*p=='/'||*p=='\\')start=p+1;
	size_t n=strlen(start);if(n>4&&Equal(start+n-4,".bsp"))n-=4;
	if(!n||n>=sizeof(out))return false;
	for(size_t i=0;i<n;++i){char c=start[i];out[i]=(c>='A'&&c<='Z')?c+32:c;}out[n]=0;return true;
}
inline int Index(const Chapters& chapters,const char* stem) {
	for(int i=0;i<chapters.count;++i)if(Equal(chapters.maps[i],stem))return i+1;return 0;
}
class Reader {
	const char* p; const char* end;
public:
	Reader(const char* text,size_t n):p(text),end(text+n){if(n>=3&&(unsigned char)p[0]==0xef&&(unsigned char)p[1]==0xbb&&(unsigned char)p[2]==0xbf)p+=3;}
	// 0=end, 1=word, 2=open, 3=close, 4=oversized word, -1=malformed.
	int Next(char (&out)[260]) {
		for(;;){while(p<end&&(unsigned char)*p<=32)++p;
			if(end-p>=2&&p[0]=='/'&&p[1]=='/'){while(p<end&&*p!='\n')++p;continue;}
			if(end-p>=2&&p[0]=='/'&&p[1]=='*'){p+=2;while(end-p>=2&&!(p[0]=='*'&&p[1]=='/'))++p;if(end-p<2)return -1;p+=2;continue;}break;}
		out[0]=0;if(p==end)return 0;if(*p=='{'){++p;return 2;}if(*p=='}'){++p;return 3;}
		const bool quoted=*p=='"';if(quoted)++p;size_t n=0;bool oversized=false;
		while(p<end){char c=*p;if(quoted&&c=='"'){++p;out[n]=0;return oversized?4:1;}
			if(!quoted&&((unsigned char)c<=32||c=='{'||c=='}')){out[n]=0;return oversized?4:(n?1:-1);}
			++p;if(quoted&&c=='\\'&&p<end&&(*p=='"'||*p=='\\'))c=*p++;
			if(c==0)return -1;if(n+1>=sizeof(out))oversized=true;else out[n++]=c;}
		out[n]=0;return quoted?-1:(oversized?4:(n?1:0));
	}
};
inline bool Complete(const Chapters& c) {
	if(c.bad||!c.count)return false;
	for(int i=0;i<c.count;++i){if(!c.maps[i][0])return false;for(int j=0;j<i;++j)if(Equal(c.maps[i],c.maps[j]))return false;}
	return true;
}
inline bool Parse(const char* text,size_t size,const char* mode,Chapters& out) {
	out={};if(!text||!mode||size>65536)return false;
	Reader reader(text,size);char stack[16][128]{};int depth=0;char key[260]{},value[260]{};
	Chapters exact{},coop{};bool exactPresent=false;char title[260]{};
	for(;;){int k=reader.Next(key);if(k==0)break;if(k==3){if(!depth)return false;--depth;continue;}if(k!=1)return false;
		int v=reader.Next(value);if(v==2){if(depth==16||strlen(key)>=128)return false;
			if(depth&&Equal(stack[depth-1],"modes")&&Equal(key,mode))exactPresent=true;
			strcpy(stack[depth++],key);continue;}if(v!=1&&v!=4)return false;
		if(depth==1&&Equal(stack[0],"mission")&&Equal(key,"DisplayTitle")){
			if(v==1)strcpy(title,value);continue;}
		const bool map=Equal(key,"Map");
		if(depth<3||!Equal(stack[depth-3],"modes")||(!map&&!Equal(key,"DisplayName")))continue;
		if(map&&v!=1)return false;
		Chapters* c=Equal(stack[depth-2],mode)?&exact:Equal(stack[depth-2],"coop")?&coop:nullptr;if(!c)continue;
		const char* number=stack[depth-1];int index=0;if(!*number)return false;
		for(const char* n=number;*n;++n){if(*n<'0'||*n>'9'||index>kMaxMaps)return false;index=index*10+(*n-'0');}
		if(index<1||index>kMaxMaps)return false;
		if(map){if(c->maps[index-1][0]||!Stem(value,c->maps[index-1]))c->bad=true;
			if(index>c->count)c->count=index;}
		else if(v==1)strcpy(c->names[index-1],value);
	}
	if(depth)return false;
	if(exactPresent){if(!Complete(exact))return false;out=exact;strcpy(out.title,title);return true;}
	// Only known realism inherits coop here. A named mutation can be versus
	// or survival; without a matching mission section its total stays unknown.
	if(!Equal(mode,"realism"))return false;
	if(!Complete(coop))return false;out=coop;strcpy(out.title,title);return true;
}
}
