#pragma once
#include "core.h"
#include "ui.h"
#include <array>
#include <functional>
#include <set>
namespace pulse {
struct Slot {bool used=false;std::string description;Code code;};
constexpr int SAVED_CODE_COUNT=24;
struct State {std::vector<uint32_t> favorites;std::array<Slot,SAVED_CODE_COUNT> slots;};
enum Act {TAB=1,BACK,OPEN,SEND,FAVORITE,PREV,NEXT,LISTEN,ENTER,SAVED,SAVE,DELETE_SLOT,CONFIRM_DELETE,KEY,KEYPAGE,ERASE,DONE,CANCEL,CODE_TEXT,RENAME,POWER,SET_POWER};
enum Screen {FAVS,TYPES,BRANDS,ACTIONS,DETAIL,LAB,SLOTS,CAPTURE,EDITOR,CODEVIEW,DELETEVIEW,POWER_VIEW};
class App {
public:
 Catalog catalog;State state;View view;Screen screen=FAVS,returnScreen=LAB;bool dirty=true,listening=false;int page=0,tab=0;
 std::string type,brand,status,transmitError;Entry selected;int selectedSlot=-1;
 std::function<bool(const State&)> persist=[](const State&){return true;};
 std::function<bool(const Code&)> transmit=[](const Code&){return true;};
 std::function<void(bool)> receiver=[](bool){};
 bool powerSaving=false;
 uint16_t batteryMv=0;
 std::function<bool(bool)> persistPower=[](bool){return true;};
private:
 std::vector<std::string> names;std::vector<uint32_t> indices;
 bool listReady=false;
 bool editingName=false;std::string edit;int keyPage=0,codePage=0;
 void button(int x,int y,int w,int h,const std::string& name,Act act,int param=0,const std::string& context="",bool active=false){view.buttons.push_back({x,y,w,h,act,param,name,context,active});}
 void nav(){button(0,272,78,48,"Favorites",TAB,0,"",tab==0);button(80,272,78,48,"All",TAB,1,"",tab==1);button(160,272,80,48,"Listen",TAB,2,"/ Emit",tab==2);}
 void listButton(int row,const std::string& name,int param,const std::string& context=""){button(8,58+row*55,224,50,name,OPEN,param,context);}
 void pages(size_t n){if(n<=3)return;button(8,224,60,44,"<",PREV);button(172,224,60,44,">",NEXT);view.lines={};view.subtitle += "  "+std::to_string(page+1)+"/"+std::to_string((n+2)/3);}
 void stop(){if(listening){listening=false;receiver(false);}}
 bool commit(const State& next){if(!persist(next)){status="Save failed; try again";return false;}state=next;return true;}
 void go(Screen s){stop();screen=s;page=0;status.clear();listReady=false;dirty=true;}
 void openCode(){edit.clear();editingName=false;returnScreen=LAB;go(EDITOR);}
 void saveName(){
   if(selectedSlot<0){for(int i=0;i<SAVED_CODE_COUNT;i++)if(!state.slots[i].used){selectedSlot=i;break;}}
   if(selectedSlot<0){status=std::to_string(SAVED_CODE_COUNT)+"/"+std::to_string(SAVED_CODE_COUNT)+" saved. Delete one first.";dirty=true;return;}
   edit=state.slots[selectedSlot].used?state.slots[selectedSlot].description:"";editingName=true;keyPage=0;returnScreen=screen;go(EDITOR);
 }
 size_t listSize() const {return screen==TYPES||screen==BRANDS?names.size():screen==ACTIONS?indices.size():screen==FAVS?state.favorites.size():screen==SLOTS?indices.size():0;}
public:
 void init(){build();}
 void fail(const std::string& s){stop();status=s;dirty=true;}
 void learned(const Code& c,const std::string& info){stop();selected={};selected.action="Captured code";selected.model=info;selected.code=c;selectedSlot=-1;go(CAPTURE);status="Captured. Test before saving.";}
 void build(){
  view={};view.tab=tab;view.status=status;view.batteryMv=batteryMv;
  if(screen==EDITOR){
    view.title=editingName?"Name this code":"Enter IR code";view.subtitle=editingName?"One description, 28 characters":"NEC:32:20DF10EF";view.keyboard=true;
    view.lines={edit.size()>33?".."+edit.substr(edit.size()-31):edit};
    std::string keys=keyPage?"UVWXYZ0123456789:,- ":"ABCDEFGHIJKLMNOPQRST";
    for(int i=0;i<20;i++)button(5+(i%5)*47,80+(i/5)*40,43,38,keys[i]==' '?"_":std::string(1,keys[i]),KEY,keys[i]);
    button(5,240,95,38,keyPage?"ABC":"U-Z / 123",KEYPAGE);button(105,240,60,38,"Erase",ERASE);button(170,240,65,38,"Done",DONE,0,"",true);
    button(5,280,230,40,"Cancel",CANCEL);view.status="";if(!status.empty())view.subtitle=status;return;
  }
  nav();
  if(screen==FAVS){
    page=std::min(page,int(state.favorites.empty()?0:(state.favorites.size()-1)/3));
    view.title="Favorites";view.subtitle="Tap to send. Hold for details.";
    if(state.favorites.empty())view.lines={"Your go-to actions live here.","Find a code in All, then", "choose Add favorite."};
    for(int j=0;j<3&&page*3+j<int(state.favorites.size());j++){
      int i=catalog.find(state.favorites[page*3+j]);if(i<0){listButton(j,"Unavailable code",page*3+j,"Hold to remove");continue;}
      Entry e=catalog.get(i);listButton(j,e.action,page*3+j,e.brand+" / "+e.type);
    }pages(state.favorites.size());
  }else if(screen==TYPES||screen==BRANDS||screen==ACTIONS){
    view.title=screen==TYPES?"All codes":screen==BRANDS?type:brand;
    view.subtitle=screen==TYPES?std::to_string(catalog.size())+" catalog actions":screen==BRANDS?"Choose manufacturer":type+" / choose action";
    if(!listReady){names.clear();indices.clear();std::set<std::string> distinct;
    for(uint32_t i=0;i<catalog.size();i++){
      auto r=catalog.record(i);if(screen==TYPES)distinct.insert(catalog.str(r.type));
      else if(type==catalog.str(r.type)){if(screen==BRANDS)distinct.insert(catalog.str(r.brand));else if(brand==catalog.str(r.brand))indices.push_back(i);}
    }
    names.assign(distinct.begin(),distinct.end());listReady=true;}
    size_t n=screen==ACTIONS?indices.size():names.size();
    for(int j=0;j<3&&page*3+j<int(n);j++){
      int k=page*3+j;if(screen==ACTIONS){auto e=catalog.get(indices[k]);listButton(j,e.action,k,e.model);}else listButton(j,names[k],k);
    }
    pages(n);if(screen!=TYPES)button(190,4,44,44,"<",BACK);
  }else if(screen==DETAIL||screen==CAPTURE){
    view.title=fit(selected.action,17);view.subtitle=selected.brand.empty()?selected.model:selected.brand+" / "+selected.type;
    view.lines={fit(selected.model,35),std::string(protocolName(selected.code.protocol))+" / "+std::to_string(selected.code.protocol==RAW?selected.code.raw.size():selected.code.bits)+(selected.code.protocol==RAW?" timings":" bits"),fit(serialize(selected.code),35)};
    button(8,120,224,50,"Emit",SEND,0,"",true);
    if(screen==DETAIL)button(8,176,224,44,isFavorite(selected.id)?"Remove favorite":"Add favorite",FAVORITE);
    else button(8,176,224,44,"Save code",SAVE);
    button(8,225,108,43,"View code",CODE_TEXT);button(124,225,108,43,"Back",BACK);view.status="";
    if(!status.empty())view.subtitle=status;
  }else if(screen==LAB){
    view.title="Listen / Emit";view.subtitle=listening?"Point original remote here...":"Learn a key, or enter a code.";
    button(190,4,44,44,"Eco",POWER,0,"",powerSaving);
    button(8, sixty(),224,54,listening?"Cancel listening":"Listen",LISTEN,0,listening?"Waiting up to 12 seconds":"Capture one remote action",listening);
    button(8,122,224,54,"Enter & emit",ENTER,0,"Protocol, bit count, hex value");
    int used=0;for(auto& s:state.slots)used+=s.used;
    button(8,184,224,54,"Saved codes",SAVED,0,std::to_string(used)+" / "+std::to_string(SAVED_CODE_COUNT)+" slots used");
  }else if(screen==POWER_VIEW){
    view.title="Power";view.subtitle="Tap a mode to apply and save.";
    button(8,62,224,54,"Default",SET_POWER,0,"100% / screen off in 60s",!powerSaving);
    button(8,124,224,54,"Power saving",SET_POWER,1,"50% / device sleeps in 20s",powerSaving);
    button(8,194,224,48,"Back",BACK);
  }else if(screen==SLOTS){
    view.title="Saved codes";view.subtitle="Tap to send. Hold to manage.";indices.clear();
    for(int i=0;i<SAVED_CODE_COUNT;i++)if(state.slots[i].used)indices.push_back(i);
    page=std::min(page,int(indices.empty()?0:(indices.size()-1)/3));
    for(int j=0;j<3&&page*3+j<int(indices.size());j++){int slot=indices[page*3+j];listButton(j,state.slots[slot].description,slot,protocolName(state.slots[slot].code.protocol));}
    if(indices.empty())view.lines={"Nothing saved yet.","Listen or enter a code first."};pages(indices.size());button(190,4,44,44,"<",BACK);
  }else if(screen==DELETEVIEW){
    view.title="Manage code";view.subtitle=state.slots[selectedSlot].description;
    button(8,62,224,48,"Rename",RENAME);button(8,120,224,48,"Delete this code",CONFIRM_DELETE);button(8,178,224,48,"Keep / Back",BACK);
  }else if(screen==CODEVIEW){
    view.title="Code details";view.subtitle="Same text printed over USB.";std::string s=serialize(selected.code);
    for(int j=0;j<8;j++){size_t off=(codePage*8+j)*35;if(off<s.size())view.lines.push_back(s.substr(off,35));}
    button(8,218, sixty(),46,"<",PREV);button(78,218,84,46,"Back",BACK);button(172,218,60,46,">",NEXT);view.status="";
  }
 }
 static int sixty(){return 60;}
 bool isFavorite(uint32_t id)const{return std::find(state.favorites.begin(),state.favorites.end(),id)!=state.favorites.end();}
 void send(const Code& code){stop();transmitError.clear();bool ok=transmit(code);status=ok?"Sent":transmitError.empty()?"Could not send":transmitError;dirty=true;}
 void click(const Button& b,bool held=false){
   dirty=true;status.clear();
   if(b.action==TAB){tab=b.param;go(tab==0?FAVS:tab==1?TYPES:LAB);return;}
   switch(b.action){
   case OPEN:
    if(screen==TYPES){type=names[b.param];go(BRANDS);}
    else if(screen==BRANDS){brand=names[b.param];go(ACTIONS);}
    else if(screen==ACTIONS){selected=catalog.get(indices[b.param]);selectedSlot=-1;go(DETAIL);}
    else if(screen==FAVS){int i=catalog.find(state.favorites[b.param]);if(i>=0){selected=catalog.get(i);selectedSlot=-1;if(held)go(DETAIL);else send(selected.code);}else if(held){State n=state;n.favorites.erase(n.favorites.begin()+b.param);commit(n);}}
    else if(screen==SLOTS){selectedSlot=b.param;selected={};selected.action=state.slots[b.param].description;selected.code=state.slots[b.param].code;if(held)go(DELETEVIEW);else send(selected.code);}
    break;
   case SEND:send(selected.code);break;
   case FAVORITE:{State n=state;auto it=std::find(n.favorites.begin(),n.favorites.end(),selected.id);if(it!=n.favorites.end())n.favorites.erase(it);else if(n.favorites.size()<64)n.favorites.push_back(selected.id);else{status="64 favorites maximum";break;}if(commit(n))status="Favorites updated";break;}
   case PREV:if(screen==CODEVIEW){if(codePage>0)--codePage;}else if(page>0)--page;break;
   case NEXT:if(screen==CODEVIEW){if((codePage+1)*280<int(serialize(selected.code).size()))++codePage;}else if((page+1)*3<int(listSize()))++page;break;
   case LISTEN:if(listening)stop();else{listening=true;receiver(true);}break;
   case POWER:go(POWER_VIEW);break;
   case SET_POWER:{bool next=b.param!=0;if(next==powerSaving)break;
    if(persistPower(next)){powerSaving=next;status="Power mode saved";}else status="Save failed; try again";break;}
   case ENTER:stop();selectedSlot=-1;openCode();break;
   case SAVED:go(SLOTS);break;
   case SAVE:saveName();break;
   case RENAME:saveName();break;
   case CONFIRM_DELETE:{State n=state;n.slots[selectedSlot]={};if(commit(n)){selectedSlot=-1;go(SLOTS);}break;}
   case CODE_TEXT:returnScreen=screen;codePage=0;go(CODEVIEW);break;
   case KEY:if(edit.size()<(editingName?28:6200))edit+=char(b.param);break;
   case KEYPAGE:keyPage=!keyPage;break;
   case ERASE:if(!edit.empty())edit.pop_back();break;
   case CANCEL:go(returnScreen);break;
   case DONE:
    if(editingName){if(edit.find_first_not_of(' ')==edit.npos){status="Enter a description";break;}State n=state;n.slots[selectedSlot]={true,edit,selected.code};if(commit(n))go(SLOTS);}
    else{Code code;std::string err;if(!parse(edit,code,err)){status=err;break;}selected={};selected.action="Entered code";selected.model="Ready to test";selected.code=code;go(CAPTURE);}
    break;
   case BACK:
    if(screen==BRANDS)go(TYPES);else if(screen==ACTIONS)go(BRANDS);else if(screen==DETAIL)go(tab==0?FAVS:ACTIONS);else if(screen==DELETEVIEW)go(SLOTS);else if(screen==CODEVIEW)go(returnScreen);else go(LAB);break;
   default:break;
   }
 }
};
}
