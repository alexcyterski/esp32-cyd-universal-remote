#pragma once
#include <cstdint>
#include <string>
namespace pulse {
constexpr uint16_t BATTERY_EMPTY_MV=3000,BATTERY_FULL_MV=4200;
inline bool batteryValid(uint16_t mv){return mv>=2500&&mv<=4400;}
inline int batteryPercent(uint16_t mv){
 if(mv<=BATTERY_EMPTY_MV)return 0;if(mv>=BATTERY_FULL_MV)return 100;
 return (mv-BATTERY_EMPTY_MV)*100/(BATTERY_FULL_MV-BATTERY_EMPTY_MV);
}
inline std::string batteryText(uint16_t mv){
 if(!batteryValid(mv))return "--.-- V";
 unsigned hundredths=(mv+5)/10;
 return std::to_string(hundredths/100)+"."+(hundredths%100<10?"0":"")+std::to_string(hundredths%100)+" V";
}
}
