#pragma once
#include <cstdint>
namespace pulse {
// Pre-send voltage guard, not a substitute for a protected battery pack.
class BatteryGuard {
 bool valid_=false,low_=false;uint16_t mv_=0;
public:
 void update(uint16_t mv){mv_=mv;valid_=mv>=2500&&mv<=4400;if(!valid_)return;
  if(mv<3400)low_=true;else if(mv>=3550)low_=false;
 }
 bool allowed()const{return valid_&&!low_;}
 bool valid()const{return valid_;}
 uint16_t millivolts()const{return mv_;}
};
}
