#pragma once
#include "app.h"
namespace pulse {
constexpr size_t MAX_SLOT_BYTES=1+1+28+14+1024*2;
constexpr size_t MAX_STATE_BYTES=5+64*4+SAVED_CODE_COUNT*MAX_SLOT_BYTES;
inline void put(std::vector<uint8_t>& b,uint64_t v,int n){for(int i=0;i<n;i++)b.push_back(v>>(8*i));}
struct StateReader {
 const std::vector<uint8_t>& bytes;size_t off=0;bool ok=true;
 uint64_t take(int n){if(off+n>bytes.size()){ok=false;return 0;}uint64_t v=0;for(int i=0;i<n;i++)v|=uint64_t(bytes[off++])<<(8*i);return v;}
 bool done()const{return ok&&off==bytes.size();}
};
inline std::vector<uint8_t> packSlot(const Slot& slot){
 std::vector<uint8_t> b;put(b,slot.used,1);if(!slot.used)return b;
 put(b,slot.description.size(),1);for(char c:slot.description)b.push_back(c);
 const auto& c=slot.code;put(b,c.protocol,1);put(b,c.bits,1);put(b,c.khz,2);put(b,c.value,8);put(b,c.raw.size(),2);for(auto v:c.raw)put(b,v,2);
 return b;
}
inline bool readSlot(StateReader& r,Slot& slot){
 auto u=r.take(1);if(u>1)return false;slot.used=u;if(!u)return r.ok;
 auto len=r.take(1);if(!len||len>28)return false;for(size_t j=0;j<len;j++)slot.description+=char(r.take(1));
 auto& c=slot.code;c.protocol=r.take(1);c.bits=r.take(1);c.khz=r.take(2);c.value=r.take(8);auto nr=r.take(2);if(nr>1024||c.protocol>5)return false;
 for(size_t j=0;j<nr;j++)c.raw.push_back(r.take(2));
 Code checked;std::string error;return r.ok&&parse(serialize(c),checked,error);
}
inline std::vector<uint8_t> packFavorites(const State& s){
 std::vector<uint8_t> b;put(b,s.favorites.size(),1);for(auto id:s.favorites)put(b,id,4);return b;
}
inline bool readFavorites(StateReader& r,State& s){
 auto n=r.take(1);if(n>64)return false;for(size_t i=0;i<n;i++)s.favorites.push_back(r.take(4));return r.ok;
}
inline std::vector<uint8_t> pack(const State& s){
 std::vector<uint8_t> b={'P','S','T',2};auto favorites=packFavorites(s);b.insert(b.end(),favorites.begin(),favorites.end());
 for(const auto& slot:s.slots){auto record=packSlot(slot);b.insert(b.end(),record.begin(),record.end());}return b;
}
inline bool unpack(const std::vector<uint8_t>& b,State& result){
 if(b.size()<5||b.size()>MAX_STATE_BYTES||b[0]!='P'||b[1]!='S'||b[2]!='T'||(b[3]!=1&&b[3]!=2))return false;
 StateReader r{b,4};State s;if(!readFavorites(r,s))return false;
 // Version 1 stored eight slots; additional slots start empty on upgrade.
 int count=b[3]==1?8:SAVED_CODE_COUNT;
 for(int i=0;i<count;i++)if(!readSlot(r,s.slots[i]))return false;
 if(!r.done())return false;result=std::move(s);return true;
}
}
