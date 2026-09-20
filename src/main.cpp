#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "board_config.h"
#include "storage.h"
#include "power_guard.h"
#include "input_guard.h"
#include "power_mode.h"
#include "receiver_control.h"
#include <cmath>

extern const uint8_t catalog_start[] asm("_binary_assets_catalog_bin_start");
TFT_eSPI tft;
SPIClass touchSPI(VSPI);
// Poll while awake; GPIO36 is reserved for RTC wake, avoiding its digital ISR.
XPT2046_Touchscreen touch(board::TOUCH_CS);
IRsend emitter(board::IR_TX);
IRrecv receiver(board::IR_RX,1024,50,true);
pulse::ReceiverControl<IRrecv> reception(receiver);
Preferences prefs;
pulse::App app;
pulse::BatteryGuard batteryGuard;
uint16_t sampleBattery(){
 uint32_t samples[9];for(auto& value:samples)value=analogReadMilliVolts(board::BATTERY_ADC);
 std::sort(samples,samples+9);return lround(samples[4]*board::BATTERY_SCALE);
}
uint32_t listenStart=0,lastInteraction=0,flashUntil=0;
bool screenOff=false;
pulse::WakeRelease wakeRelease;
constexpr int BACKLIGHT_CHANNEL=7;
void brightness(uint8_t level){
 static int previous=-1;if(previous!=level){ledcWrite(BACKLIGHT_CHANNEL,level);previous=level;}
}
void wakeScreen(){screenOff=false;brightness(pulse::activeBrightness(app.powerSaving));lastInteraction=millis();}
pulse::ToggleState toggles;
pulse::Calibration cal;
struct Display: pulse::Canvas {
 void rect(int x,int y,int w,int h,uint32_t c) override {tft.fillRect(x,y,w,h,tft.color565(c>>16,c>>8,c));}
 void text(int x,int y,const std::string& s,int size,uint32_t c,uint32_t bg=pulse::BG) override {
  pulse::renderText(*this,x,y,s,size,c,bg);
 }
} display;

TS_Point sample(int x,int y){
 display.rect(0,0,240,320,pulse::BG);display.text(45,5,"Touch calibration",1,pulse::TEXT);
 auto accent=tft.color565((pulse::ACCENT>>16)&255,(pulse::ACCENT>>8)&255,pulse::ACCENT&255);
 tft.drawCircle(x,y,10,accent);tft.drawFastHLine(x-15,y,30,accent);tft.drawFastVLine(x,y-15,30,accent);
 while(touch.touched())delay(10);
 for(;;){
  while(!touch.touched())delay(10);
  long sx=0,sy=0;int n=0;
  for(int i=0;i<12;i++){if(touch.touched()){auto p=touch.getPoint();if(p.z>100){sx+=p.x;sy+=p.y;n++;}}delay(8);}
  if(n>=6){while(touch.touched())delay(10);return TS_Point(sx/n,sy/n,1);}
 }
}
void calibrate(){
 wakeScreen();
 for(;;){
  auto a=sample(24,45),b=sample(216,160),c=sample(36,285);
  float d=float(a.x)*(b.y-c.y)+float(b.x)*(c.y-a.y)+float(c.x)*(a.y-b.y);
  if(fabs(d)<1000)continue;
  auto solve=[&](float u,float v,float w,float& aa,float& bb,float& cc){
   aa=(u*(b.y-c.y)+v*(c.y-a.y)+w*(a.y-b.y))/d;
   bb=(u*(c.x-b.x)+v*(a.x-c.x)+w*(b.x-a.x))/d;
   cc=(u*(float(b.x)*c.y-float(c.x)*b.y)+v*(float(c.x)*a.y-float(a.x)*c.y)+w*(float(a.x)*b.y-float(b.x)*a.y))/d;
  };
  solve(24,216,36,cal.ax,cal.bx,cal.cx);solve(45,160,285,cal.ay,cal.by,cal.cy);
  if(!cal.valid())continue;
  auto check=sample(200,280);
  if(fabs(cal.ax*check.x+cal.bx*check.y+cal.cx-200)<20&&fabs(cal.ay*check.x+cal.by*check.y+cal.cy-280)<20){prefs.putBytes("cal",&cal,sizeof(cal));lastInteraction=millis();return;}
 }
}
bool emit(const pulse::Code& c){
 if(board::FULL_BUILD){batteryGuard.update(sampleBattery());if(!batteryGuard.allowed()){
  app.transmitError=batteryGuard.valid()?"Battery low: recharge":"Battery reading invalid";digitalWrite(board::IR_TX,LOW);return false;
 }}
 pulse::Code checked;std::string error;if(!pulse::parse(pulse::serialize(c),checked,error))return false;
 reception.set(false);const uint64_t txValue=toggles.next(c.protocol,c.value);
 switch(c.protocol){
  case pulse::NEC:emitter.sendNEC(c.value,c.bits);break;
  case pulse::SAMSUNG:emitter.sendSAMSUNG(c.value,c.bits);break;
  case pulse::SONY:emitter.sendSony(c.value,c.bits,2);break;
  case pulse::RC5:emitter.sendRC5(txValue,c.bits);break;
  case pulse::RC6:emitter.sendRC6(txValue,c.bits);break;
  case pulse::RAW:emitter.sendRaw(c.raw.data(),c.raw.size(),c.khz);break;
  default:return false;
 }
 digitalWrite(board::IR_TX,LOW);if(board::FULL_BUILD)batteryGuard.update(sampleBattery());flashUntil=millis()+400;Serial.println(pulse::serialize(c).c_str());return true;
}
void pollReceiver(){
 if(!app.listening)return;
 if(millis()-listenStart>board::LEARN_TIMEOUT_MS){app.fail("Timed out. Try another press.");return;}
 decode_results r;
 if(!receiver.decode(&r))return;
 if(r.overflow){receiver.resume();app.fail("Signal too long (1024 timings).");return;}
 if(r.repeat){receiver.resume();return;}
 pulse::Code c;c.value=r.value;c.bits=r.bits;
 bool decoded=true;
 switch(r.decode_type){
  case NEC:c.protocol=pulse::NEC;break;
  case SAMSUNG:c.protocol=pulse::SAMSUNG;break;
  case SONY:c.protocol=pulse::SONY;c.khz=40;break;
  case RC5:c.protocol=pulse::RC5;c.khz=36;break;
  case RC6:c.protocol=pulse::RC6;c.khz=36;break;
  default:decoded=false;
 }
 pulse::Code checked;std::string error;
 if(decoded&&!pulse::parse(pulse::serialize(c),checked,error))decoded=false;
 if(!decoded){
  c={};c.protocol=pulse::RAW;c.khz=board::RAW_CARRIER_KHZ;
  if(r.rawlen<4||r.rawlen-1>board::MAX_RAW){receiver.resume();app.fail("Signal exceeds capture limit.");return;}
  for(uint16_t i=1;i<r.rawlen;i++){
   uint32_t us=uint32_t(r.rawbuf[i])*kRawTick;
   if(!us||us>65535){receiver.resume();app.fail("Signal gap exceeds raw limit.");return;}
   c.raw.push_back(us);
  }
  if(!pulse::parse(pulse::serialize(c),checked,error)){receiver.resume();app.fail(error);return;}
 }
 receiver.resume();Serial.println(pulse::serialize(c).c_str());lastInteraction=millis();app.learned(c,decoded?"Decoded remote action":"RAW: carrier assumed 38 kHz");
}
void setup(){
 pinMode(board::IR_TX,OUTPUT);digitalWrite(board::IR_TX,LOW);
 Serial.begin(115200);pinMode(0,INPUT_PULLUP);pinMode(board::BACKLIGHT,OUTPUT);digitalWrite(board::BACKLIGHT,LOW);
 ledcSetup(BACKLIGHT_CHANNEL,5000,8);ledcAttachPin(board::BACKLIGHT,BACKLIGHT_CHANNEL);
 tft.init();tft.setRotation(board::DISPLAY_ROTATION);
 analogReadResolution(12);analogSetPinAttenuation(board::BATTERY_ADC,ADC_11db);
 touchSPI.begin(board::TOUCH_CLK,board::TOUCH_MISO,board::TOUCH_MOSI,board::TOUCH_CS);touch.begin(touchSPI);touch.setRotation(0);
 pinMode(board::TOUCH_IRQ,INPUT); // Board/controller provides the idle-high IRQ.
 bool storageReady=prefs.begin("pulse",false);
 app.powerSaving=storageReady?prefs.getBool("eco",false):false;
 app.persistPower=[storageReady](bool value){return storageReady&&prefs.putBool("eco",value)==1;};
 brightness(pulse::activeBrightness(app.powerSaving));
 bool hasCal=prefs.getBytesLength("cal")==sizeof(cal)&&prefs.getBytes("cal",&cal,sizeof(cal))==sizeof(cal);
 if(!hasCal||!cal.valid()||digitalRead(0)==LOW)calibrate();
 app.catalog.begin(catalog_start);emitter.begin();
 size_t n=prefs.getBytesLength("state");
 if(n&&n<20000){std::vector<uint8_t> b(n);prefs.getBytes("state",b.data(),n);if(!pulse::unpack(b,app.state))app.status="Saved data invalid; not loaded.";}
 app.persist=[storageReady](const pulse::State& s){if(!storageReady)return false;auto b=pulse::pack(s);return prefs.putBytes("state",b.data(),b.size())==b.size();};
 app.transmit=emit;
 app.receiver=[](bool on){if(on)listenStart=millis();reception.set(on);};
 app.init();lastInteraction=millis();
 Serial.println(board::FULL_BUILD?"PULSE FULL: nominal 33% carrier; battery TX guard":"PULSE SIMPLE: nominal 50% carrier; one low-current LED");
 Serial.println("USB commands: BAT, CAL, TXTEST, or CODE NEC:32:20DF10EF");
}
void sleepDevice(){
 // All calls occur on the UI task, after synchronous sends and NVS writes finish.
 // Leave the XPT2046 in its PD=00 state with PENIRQ armed (library's last read).
 if(touch.touched()||digitalRead(board::TOUCH_IRQ)==LOW||Serial.available()){lastInteraction=millis();return;}
 const gpio_num_t irq=static_cast<gpio_num_t>(board::TOUCH_IRQ);
 esp_err_t err=esp_sleep_enable_ext0_wakeup(irq,0);
 if(err!=ESP_OK){app.fail("Touch wake unavailable");lastInteraction=millis();return;}
 reception.set(false);digitalWrite(board::IR_TX,LOW);brightness(0);
 tft.writecommand(ILI9341_DISPOFF);tft.writecommand(ILI9341_SLPIN);delay(120);
 Serial.println("Power saving: asleep; tap screen to wake");Serial.flush();
 // A touch/USB byte during panel shutdown cancels entry into CPU sleep.
 if(digitalRead(board::TOUCH_IRQ)==HIGH&&!Serial.available())err=esp_light_sleep_start();
 esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
 rtc_gpio_deinit(irq);pinMode(board::TOUCH_IRQ,INPUT);
 digitalWrite(board::IR_TX,LOW);
 tft.writecommand(ILI9341_SLPOUT);delay(120);tft.writecommand(ILI9341_DISPON);
 flashUntil=0;screenOff=false;lastInteraction=millis();wakeRelease.arm();app.dirty=true;
 // Redraw before re-enabling the backlight in loop().
 if(err!=ESP_OK&&err!=ESP_ERR_SLEEP_REJECT)app.fail("Sleep failed; staying awake");
 Serial.println("Awake");
}
void loop(){
 pollReceiver();
 static std::string serialLine;static bool serialOverflow=false;
 // Keep touch and receive polling responsive under a continuous USB stream.
 for(int budget=256;budget>0&&Serial.available();--budget){
  wakeScreen();
  char ch=Serial.read();if(ch=='\n'){
   if(!serialOverflow&&serialLine=="BAT"){auto mv=sampleBattery();batteryGuard.update(mv);Serial.printf("BAT node=%u mV, scale=%.4f, profile=%s, guard=%s\n",mv,board::BATTERY_SCALE,board::FULL_BUILD?"FULL":"SIMPLE",board::FULL_BUILD?(batteryGuard.allowed()?"ready":"blocked"):"disabled");}
   if(!serialOverflow&&serialLine=="CAL"){app.fail("Calibration updated");calibrate();lastInteraction=millis();}
   if(!serialOverflow&&serialLine=="TXTEST"){
    pulse::Code test;test.protocol=pulse::RAW;test.khz=38;test.raw={50000,50000,50000,50000,50000,50000,50000,50000,50000,50000,50000};
    Serial.println(emit(test)?"TXTEST: six 50 ms carrier bursts sent":"TXTEST: blocked");
   }
   if(!serialOverflow&&serialLine.rfind("CODE ",0)==0){pulse::Code c;std::string err;if(pulse::parse(serialLine.substr(5),c,err))app.learned(c,"Imported over USB");else app.fail(err);}
   if(serialOverflow)app.fail("USB input too long");
   serialLine.clear();serialOverflow=false;
  }else if(ch!='\r'){if(serialLine.size()<6210)serialLine+=ch;else serialOverflow=true;}
 }
 static pulse::TouchGesture gesture;static uint32_t viewRevision=0;
 bool touching=touch.touched();
 if(screenOff&&touching){wakeScreen();wakeRelease.arm();}
 bool blocked=wakeRelease.block(touching||digitalRead(board::TOUCH_IRQ)==LOW,millis());
 if(blocked){gesture={};lastInteraction=millis();}
 else if(touching){auto p=touch.getPoint();int x=lround(cal.ax*p.x+cal.bx*p.y+cal.cx),y=lround(cal.ay*p.x+cal.by*p.y+cal.cy);
  if(!gesture.down){gesture.begin(pulse::hit(app.view,x,y),x,y,millis(),viewRevision,app.dirty);}
  else gesture.move(x,y);
  lastInteraction=millis();
 }else if(gesture.down){
  const uint32_t now=millis();int pressed=gesture.release(now,viewRevision,app.dirty);
  if(pressed>=0&&pressed<int(app.view.buttons.size())){auto b=app.view.buttons[pressed];app.click(b,gesture.held(now));if(b.action==pulse::CODE_TEXT)Serial.println(pulse::serialize(app.selected.code).c_str());}
 }
 if(app.dirty){++viewRevision;app.build();tft.startWrite();pulse::draw(display,app.view,int32_t(flashUntil-millis())>0);tft.endWrite();app.dirty=false;}
 if(flashUntil&&int32_t(millis()-flashUntil)>=0){flashUntil=0;display.rect(0,0,240,3,pulse::PANEL);}
 bool busy=app.listening||touching||gesture.down||blocked||Serial.available();
 if(!screenOff&&pulse::idleDue(millis(),lastInteraction,app.powerSaving,busy)){
  // Expire any abandoned partial USB command at the idle boundary.
  serialLine.clear();serialOverflow=false;
  if(app.powerSaving){gesture={};sleepDevice();return;}
  screenOff=true;
 }
 brightness(screenOff?0:pulse::activeBrightness(app.powerSaving));
 delay(5);
}
