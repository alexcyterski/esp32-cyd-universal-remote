#pragma once
#include "app.h"
namespace pulse {
inline void put(std::vector<uint8_t>& b,uint64_t v,int n){for(int i=0;i<n;i++)b.push_back(v>>(8*i));}
inline std::vector<uint8_t> pack(const State& s){
 std::vector<uint8_t> b={'P','S','T',1};put(b,s.favorites.size(),1);for(auto id:s.favorites)put(b,id,4);
 for(const auto& slot:s.slots){put(b,slot.used,1);if(!slot.used)continue;
  put(b,slot.description.size(),1);for(char c:slot.description)b.push_back(c);
  const auto& c=slot.code;put(b,c.protocol,1);put(b,c.bits,1);put(b,c.khz,2);put(b,c.value,8);put(b,c.raw.size(),2);for(auto v:c.raw)put(b,v,2);
 }return b;
}
inline bool unpack(const std::vector<uint8_t>& b,State& result){
 if(b.size()<5||b[0]!='P'||b[1]!='S'||b[2]!='T'||b[3]!=1)return false;
 size_t off=4;bool ok=true;
 auto take=[&](int n)->uint64_t{if(off+n>b.size()){ok=false;return 0;}uint64_t v=0;for(int i=0;i<n;i++)v|=uint64_t(b[off++])<<(8*i);return v;};
 State s;auto nf=take(1);if(nf>64)return false;for(size_t i=0;i<nf;i++)s.favorites.push_back(take(4));
 for(auto& slot:s.slots){auto u=take(1);if(u>1)return false;slot.used=u;if(!u)continue;
  auto len=take(1);if(!len||len>28)return false;for(size_t j=0;j<len;j++)slot.description+=char(take(1));
  auto& c=slot.code;c.protocol=take(1);c.bits=take(1);c.khz=take(2);c.value=take(8);auto nr=take(2);if(nr>1024||c.protocol>5)return false;
  for(size_t j=0;j<nr;j++)c.raw.push_back(take(2));
  Code checked;std::string error;if(!parse(serialize(c),checked,error))return false;
 }
 if(!ok||off!=b.size())return false;result=std::move(s);return true;
}
}
