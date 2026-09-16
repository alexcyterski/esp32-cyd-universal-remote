#include "storage.h"
#include "power_guard.h"
#include "input_guard.h"
#include "power_mode.h"
#include "receiver_control.h"
#include <limits>
#include <fstream>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <filesystem>
using namespace pulse;
std::string xml(std::string s){std::string o;for(char c:s)o+=c=='&'?"&amp;":c=='<'?"&lt;":c=='>'?"&gt;":std::string(1,c);return o;}
struct Svg:Canvas {
 std::ofstream f;Svg(const std::string& path):f(path){f<<"<svg xmlns='http://www.w3.org/2000/svg' width='240' height='320' viewBox='0 0 240 320'>";}
 ~Svg(){f<<"</svg>";}
 void color(uint32_t c){f<<"#"<<std::hex<<std::setw(6)<<std::setfill('0')<<c<<std::dec;}
 void rect(int x,int y,int w,int h,uint32_t c){f<<"<rect x='"<<x<<"' y='"<<y<<"' width='"<<w<<"' height='"<<h<<"' fill='";color(c);f<<"'/>";}
 void text(int x,int y,const std::string& s,int size,uint32_t c,uint32_t bg=BG){renderText(*this,x,y,s,size,c,bg);}
};
void press(App& a,Act action,int param=0,bool held=false){a.build();for(auto b:a.view.buttons)if(b.action==action&&b.param==param){a.click(b,held);a.build();return;}assert(false&&"button absent");}
void snapshot(App& a,const std::string& name){a.build();for(auto b:a.view.buttons){assert(b.x>=0&&b.y>=0&&b.x+b.w<=240&&b.y+b.h<=320);}Svg c("preview/"+name+".svg");draw(c,a.view);}
int main(){
 struct FakeReceiver {bool running=false;int starts=0,stops=0;
  void enableIRIn(){assert(!running);running=true;++starts;}
  void disableIRIn(){assert(running);running=false;++stops;}
 } fake;
 ReceiverControl<FakeReceiver> reception(fake);
 reception.set(false);assert(fake.stops==0);reception.set(true);reception.set(true);assert(fake.starts==1);
 reception.set(false);reception.set(false);assert(fake.stops==1);reception.set(true);assert(fake.starts==2);
 assert(!idleDue(59999,0,false,false)&&idleDue(60000,0,false,false));
 assert(!idleDue(19999,0,true,false)&&idleDue(20000,0,true,false));
 assert(!idleDue(90000,0,true,true)); // no sleep while learning/touching
 assert(!idleDue(25000,10000,true,false)); // interaction resets the deadline
 assert(idleDue(10000,UINT32_MAX-10000,true,false)); // clock wrap
 assert(activeBrightness(true)==128&&activeBrightness(false)==255);
 WakeRelease wake;assert(!wake.block(false,0));wake.arm();assert(wake.block(true,1));
 assert(wake.block(true,10000));assert(wake.block(false,10001));assert(wake.block(false,10050));
 assert(wake.block(true,10051));assert(wake.block(false,10052));assert(!wake.block(false,10102));
 wake.arm();assert(wake.block(false,UINT32_MAX-20));assert(!wake.block(false,40));
 TouchGesture touch;
 touch.begin(4,20,30,100,1,false);assert(touch.release(150,1,false)==4);
 touch.begin(4,20,30,100,1,false);assert(touch.release(150,2,false)==-1); // screen changed
 touch.begin(4,20,30,100,1,false);assert(touch.release(150,1,true)==-1); // capture pending redraw
 touch.begin(4,20,30,100,1,true);assert(touch.release(150,1,false)==-1); // wake touch
 touch.begin(4,20,30,100,1,false);touch.move(60,30);touch.move(20,30);assert(touch.release(150,1,false)==-1);
 touch.begin(4,20,30,100,1,false);assert(touch.release(120,1,false)==-1); // bounce
 touch.begin(4,20,30,UINT32_MAX-20,1,false);assert(touch.release(40,1,false)==4); // clock wrap
 touch.begin(4,20,30,100,1,false);assert(touch.held(750)&&touch.release(750,1,false)==4);
 Calibration good{0.065f,0,-10,0,0.085f,-15};assert(good.valid());assert(!Calibration{}.valid());
 for(auto member:{&Calibration::ax,&Calibration::bx,&Calibration::cx,&Calibration::ay,&Calibration::by,&Calibration::cy}){
  auto broken=good;broken.*member=std::numeric_limits<float>::quiet_NaN();assert(!broken.valid());
  broken=good;broken.*member=std::numeric_limits<float>::infinity();assert(!broken.valid());
 }
 auto broken=good;broken.cx=1e30f;assert(!broken.valid());
 ToggleState toggles;assert(toggles.next(RC5,0)==0x800);assert(toggles.next(NEC,0x1234)==0x1234);
 assert(toggles.next(RC6,0)==0x10000);assert(toggles.next(RC5,0)==0);assert(toggles.next(RC6,0)==0);
 BatteryGuard guard;assert(!guard.allowed());guard.update(3450);assert(guard.allowed());guard.update(3399);assert(!guard.allowed());guard.update(3500);assert(!guard.allowed());guard.update(3550);assert(guard.allowed());guard.update(0);assert(!guard.allowed());guard.update(4500);assert(!guard.allowed());guard.update(4000);assert(guard.allowed());
 std::ifstream f("assets/catalog.bin",std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});assert(bytes.size()>12);
 App a;a.catalog.begin(bytes.data());assert(a.catalog.size()>0);uint32_t pool;memcpy(&pool,bytes.data()+8,4);assert(pool==12+38*a.catalog.size());std::set<uint32_t> ids;
 for(uint32_t i=0;i<a.catalog.size();i++){auto r=a.catalog.record(i);assert(ids.insert(r.id).second);for(auto off:{r.type,r.brand,r.model,r.action}){assert(pool+off<bytes.size());assert(memchr(bytes.data()+pool+off,0,bytes.size()-pool-off));}assert(pool+r.rawOffset+2*r.rawCount<=bytes.size());auto e=a.catalog.get(i);Code c;std::string err;assert(parse(serialize(e.code),c,err));assert(serialize(c)==serialize(e.code));}
 Code c;std::string err;
 assert(parse("NEC:32:20DF10EF",c,err)&&c.value==0x20DF10EF);
 for(auto bad:{"NEC:32:100000000","NEC:12:FF","RC5:12:1000","RAW:29:1,2,3","RAW:38:0,2,3","RAW:38:1,2,3,","RAW:38:65536,1,2","RAW:38:1,-2,3","BAD:32:AA","NEC:32:"})assert(!parse(bad,c,err));
 uint64_t v;assert(!number("9",10,4,v));
 std::filesystem::create_directories("preview");
 int sent=0,saves=0;a.transmit=[&](const Code&){sent++;return true;};a.persist=[&](const State&){saves++;return true;};
 a.init();press(a,TAB,1);snapshot(a,"02-all");press(a,OPEN,0);snapshot(a,"03-manufacturers");press(a,OPEN,0);snapshot(a,"04-actions");press(a,OPEN,0);snapshot(a,"05-detail");press(a,FAVORITE);assert(a.state.favorites.size()==1);press(a,TAB,0);press(a,OPEN,0);assert(sent==1);press(a,OPEN,0,true);assert(a.screen==DETAIL&&sent==1);
 // Paging reuses the tree list, and returning to another level rebuilds it.
 press(a,TAB,1);std::string firstType=a.view.buttons[3].title;
 press(a,NEXT);assert(a.page==1);press(a,PREV);assert(a.page==0&&a.view.buttons[3].title==firstType);
 press(a,OPEN,0);assert(a.type==firstType);press(a,OPEN,0);press(a,BACK);assert(a.screen==BRANDS);press(a,BACK);assert(a.screen==TYPES&&a.view.buttons[3].title==firstType);
 // Three real catalog actions for a representative Favorites screen.
 a.state.favorites.clear();for(auto action:{"Power","Vol up","Mute"})for(uint32_t i=0;i<a.catalog.size();i++){auto e=a.catalog.get(i);if(e.type=="TVs"&&e.brand=="Samsung"&&e.action==action){a.state.favorites.push_back(e.id);break;}}
 if(a.state.favorites.empty())for(int i=0;i<3;i++)a.state.favorites.push_back(a.catalog.get(i).id);
 press(a,TAB,0);snapshot(a,"01-favorites");press(a,TAB,2);snapshot(a,"06-listen-emit");
 assert(!a.powerSaving);press(a,POWER);assert(a.screen==POWER_VIEW);
 int powerSaves=0;a.persistPower=[&](bool){++powerSaves;return true;};
 press(a,SET_POWER,1);assert(a.powerSaving&&powerSaves==1);press(a,SET_POWER,1);assert(powerSaves==1);
 snapshot(a,"10-power");a.persistPower=[](bool){return false;};press(a,SET_POWER,0);
 assert(a.powerSaving&&a.status.find("failed")!=a.status.npos);
 a.persistPower=[](bool){return true;};press(a,SET_POWER,0);assert(!a.powerSaving);press(a,BACK);assert(a.screen==LAB);
 assert(parse("NEC:32:20DF10EF",c,err));a.learned(c,"Decoded remote action");snapshot(a,"07-captured");press(a,SAVE);snapshot(a,"09-keyboard");
 press(a,KEY,'T');press(a,KEYPAGE);press(a,KEY,'V');press(a,DONE);assert(a.state.slots[0].used&&a.state.slots[0].description=="TV");snapshot(a,"08-saved");press(a,OPEN,0);assert(sent==2);press(a,OPEN,0,true);assert(a.screen==DELETEVIEW);press(a,RENAME);press(a,ERASE);press(a,KEY,'A');press(a,DONE);assert(a.state.slots[0].description=="TA");
 State original=a.state;for(int i=0;i<8;i++)original.slots[i]={true,"SAVED "+std::to_string(i),c};auto packed=pack(original);State loaded;assert(unpack(packed,loaded));assert(pack(loaded)==packed);for(size_t n=0;n<packed.size();n++){auto truncated=packed;truncated.resize(n);assert(!unpack(truncated,loaded));}
 a.state=original;a.learned(c,"Test");press(a,SAVE);assert(a.screen==CAPTURE&&a.status.find("8/8")!=a.status.npos);
 a.persist=[](const State&){return false;};press(a,TAB,0);a.selected=a.catalog.get(0);a.screen=DETAIL;auto before=pack(a.state);press(a,FAVORITE);assert(pack(a.state)==before&&a.status.find("failed")!=a.status.npos);
 Code raw;raw.protocol=RAW;raw.khz=38;raw.raw.assign(1024,500);for(auto& s:original.slots)s.code=raw;assert(unpack(pack(original),loaded));assert(pack(original).size()<20000);
 std::cout<<"PASS: "<<a.catalog.size()<<" catalog records; parser, navigation, tap/hold, favorites, eight-slot limit, rename, persistence failures, truncation, maximum raw storage.\n";
}
