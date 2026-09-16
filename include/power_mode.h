#pragma once
#include <cstdint>
namespace pulse {
constexpr uint32_t idleTimeout(bool saver){return saver?20000:60000;}
constexpr uint8_t activeBrightness(bool saver){return saver?128:255;}
inline bool idleDue(uint32_t now,uint32_t last,bool saver,bool busy){
 return !busy&&uint32_t(now-last)>=idleTimeout(saver);
}
// Consume the entire wake gesture, including a held finger and release bounce.
class WakeRelease {
 bool blocked_=false,releasing_=false;uint32_t releasedAt_=0;
public:
 void arm(){blocked_=true;releasing_=false;}
 bool block(bool contact,uint32_t now){
  if(!blocked_)return false;
  if(contact)releasing_=false;
  else if(!releasing_){releasing_=true;releasedAt_=now;}
  else if(uint32_t(now-releasedAt_)>=50)blocked_=false;
  return blocked_;
 }
};
}
