#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>

namespace pulse {
enum Protocol : uint8_t { RAW=0, NEC=1, SAMSUNG=2, SONY=3, RC5=4, RC6=5 };
inline const char* protocolName(uint8_t p) {
  static const char* names[]={"RAW","NEC","SAMSUNG","SONY","RC5","RC6"};
  return p<6?names[p]:"INVALID";
}
struct Code { uint8_t protocol=NEC,bits=32; uint16_t khz=38; uint64_t value=0; std::vector<uint16_t> raw; };
struct ToggleState {
 bool rc5=false,rc6=false;
 uint64_t next(uint8_t protocol,uint64_t value){
  if(protocol==RC5){rc5=!rc5;return (value&~(1ULL<<11))|(uint64_t(rc5)<<11);}
  if(protocol==RC6){rc6=!rc6;return (value&~(1ULL<<16))|(uint64_t(rc6)<<16);}
  return value;
 }
};
inline bool number(const std::string& s,int base,uint64_t max,uint64_t& out) {
  if(s.empty())return false; out=0;
  size_t i=0;if(base==16 && s.size()>2 && s[0]=='0' && (s[1]=='x'||s[1]=='X'))i=2;
  if(i==s.size())return false;
  for(;i<s.size();++i){char c=s[i];int d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
    if(d<0||d>=base||uint64_t(d)>max||out>(max-d)/base)return false;out=out*base+d;}
  return true;
}
inline std::string serialize(const Code& c) {
  std::ostringstream o;o<<protocolName(c.protocol)<<":";
  if(c.protocol==RAW){o<<c.khz<<":";for(size_t i=0;i<c.raw.size();++i){if(i)o<<",";o<<c.raw[i];}}
  else{o<<int(c.bits)<<":"<<std::uppercase<<std::hex<<c.value;}
  return o.str();
}
inline bool parse(const std::string& input,Code& result,std::string& error) {
  std::string s;for(char c:input)if(c!=' '&&c!='\r'&&c!='\n'&&c!='\t')s+=c;
  auto a=s.find(':'),b=a==s.npos?s.npos:s.find(':',a+1);
  if(a==s.npos||b==s.npos){error="Use NEC:32:20DF10EF";return false;}
  std::string p=s.substr(0,a);std::transform(p.begin(),p.end(),p.begin(),[](unsigned char c){return char(std::toupper(c));});
  Code c;int id=-1;for(int i=0;i<6;i++)if(p==protocolName(i))id=i;
  if(id<0){error="Unknown protocol";return false;}c.protocol=id;
  uint64_t n=0;
  if(!number(s.substr(a+1,b-a-1),10,64,n)){error="Invalid bits / kHz";return false;}
  if(id==RAW){
    if(n<30||n>60){error="Carrier: 30-60 kHz";return false;}c.khz=n;c.bits=0;
    size_t start=b+1;uint64_t total=0;
    while(start<s.size()){
      size_t end=s.find(',',start);if(end==s.npos)end=s.size();uint64_t v;
      if(!number(s.substr(start,end-start),10,65535,v)||!v){error="Invalid raw duration";return false;}
      total+=v;if(total>1500000||c.raw.size()>=1024){error="Raw capture too long";return false;}
      c.raw.push_back(v);start=end+1;
    }
    if(c.raw.size()<3||s.back()==','){error="Need mark,space,mark";return false;}
  }else{
    bool ok=(id==NEC||id==SAMSUNG)?n==32:id==SONY?(n==12||n==15||n==20):id==RC5?n==12:n==20;
    if(!ok){error="Unsupported bit length";return false;}c.bits=n;
    if(!number(s.substr(b+1),16,(uint64_t(1)<<n)-1,c.value)){error="Hex value exceeds bits";return false;}
    c.khz=id==SONY?40:(id==RC5||id==RC6)?36:38;
  }
  result=std::move(c);error.clear();return true;
}
struct Entry { uint32_t id=0; std::string type,brand,model,action; Code code; };
#pragma pack(push,1)
struct Record { uint32_t id,type,brand,model,action; uint64_t value;uint32_t rawOffset;uint16_t rawCount,khz;uint8_t protocol,bits; };
#pragma pack(pop)
static_assert(sizeof(Record)==38,"catalog format");
class Catalog {
 const uint8_t* data=nullptr;uint32_t count_=0,pool=0;
public:
 void begin(const uint8_t* p){data=p; if(std::memcmp(p,"PUL1",4)==0){std::memcpy(&count_,p+4,4);std::memcpy(&pool,p+8,4);}}
 uint32_t size() const{return count_;}
 Record record(uint32_t i)const{Record r{};if(i<count_)std::memcpy(&r,data+12+i*sizeof(r),sizeof(r));return r;}
 const char* str(uint32_t off)const{return reinterpret_cast<const char*>(data+pool+off);}
 Entry get(uint32_t i)const{Record r=record(i);Entry e;e.id=r.id;e.type=str(r.type);e.brand=str(r.brand);e.model=str(r.model);e.action=str(r.action);
   e.code.protocol=r.protocol;e.code.bits=r.bits;e.code.value=r.value;e.code.khz=r.khz;
   e.code.raw.resize(r.rawCount);if(r.rawCount)std::memcpy(e.code.raw.data(),data+pool+r.rawOffset,r.rawCount*2);return e;}
 int find(uint32_t id)const{for(uint32_t i=0;i<count_;i++)if(record(i).id==id)return i;return -1;}
};
}
