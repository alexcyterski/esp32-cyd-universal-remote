#pragma once
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <initializer_list>
namespace pulse {
// A release belongs only to the screen on which its press began.
struct TouchGesture {
 bool down=false;int pressed=-1,x=0,y=0;uint32_t started=0,revision=0;
 void begin(int button,int px,int py,uint32_t now,uint32_t view,bool consume){
  down=true;pressed=consume?-1:button;x=px;y=py;started=now;revision=view;
 }
 void move(int px,int py){if(std::abs(px-x)>18||std::abs(py-y)>18)pressed=-1;}
 int release(uint32_t now,uint32_t view,bool pendingView){
  int result=down&&!pendingView&&revision==view&&uint32_t(now-started)>=35?pressed:-1;
  down=false;pressed=-1;return result;
 }
 bool held(uint32_t now)const{return uint32_t(now-started)>=650;}
};
struct Calibration {
 float ax=0,bx=0,cx=0,ay=0,by=0,cy=0;
 bool valid()const{
  if(!std::isfinite(ax)||!std::isfinite(bx)||!std::isfinite(cx)||
     !std::isfinite(ay)||!std::isfinite(by)||!std::isfinite(cy))return false;
  // Bound mappings of the complete 12-bit input range before lround().
  for(int x:{0,4095})for(int y:{0,4095}){
   if(std::fabs(ax*x+bx*y+cx)>10000||std::fabs(ay*x+by*y+cy)>10000)return false;
  }
  return std::fabs(ax*by-bx*ay)>0.000001f;
 }
};
}
