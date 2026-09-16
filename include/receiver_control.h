#pragma once
namespace pulse {
// IRremoteESP8266's ESP32 timer must exist before disableIRIn() is called.
template<class Receiver> class ReceiverControl {
 Receiver& receiver_;bool running_=false;
public:
 explicit ReceiverControl(Receiver& receiver):receiver_(receiver){}
 void set(bool on){
  if(on==running_)return;
  if(on)receiver_.enableIRIn();else receiver_.disableIRIn();
  running_=on;
 }
};
}
