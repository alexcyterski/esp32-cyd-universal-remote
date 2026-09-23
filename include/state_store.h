#pragma once
#include "storage.h"
namespace pulse {
// Each UI edit changes one record. Small atomic NVS writes leave room for
// replacement data even with 24 maximum-sized RAW codes in the 64 KB partition.
template<class Backend> class StateStore {
 Backend& prefs;bool records=false;
 static std::string key(int slot){return "slot"+std::to_string(slot);}
 bool write(const char* name,const std::vector<uint8_t>& b){return prefs.putBytes(name,b.data(),b.size())==b.size();}
 std::vector<uint8_t> read(const char* name,size_t limit){
  size_t n=prefs.getBytesLength(name);if(!n||n>limit)return {};
  std::vector<uint8_t> b(n);if(prefs.getBytes(name,b.data(),n)!=n)return {};return b;
 }
 bool migrate(const State& state){
  if(!write("favorites",packFavorites(state)))return false;
  for(int i=0;i<SAVED_CODE_COUNT;i++)if(!write(key(i).c_str(),packSlot(state.slots[i])))return false;
  // Publish only after every record exists. An interrupted migration reloads
  // the original blob and retries; deletion is safe only after this marker.
  if(prefs.putBool("records",true)!=1)return false;
  records=true;prefs.remove("state");return true;
 }
public:
 explicit StateStore(Backend& backend):prefs(backend){}
 bool load(State& result){
  records=prefs.getBool("records",false);
  if(!records){
   if(!prefs.getBytesLength("state")){result={};return true;}
   return unpack(read("state",MAX_STATE_BYTES),result);
  }
  State next;auto b=read("favorites",257);StateReader f{b};if(!readFavorites(f,next)||!f.done())return false;
  for(int i=0;i<SAVED_CODE_COUNT;i++){
   b=read(key(i).c_str(),MAX_SLOT_BYTES);StateReader r{b};if(!readSlot(r,next.slots[i])||!r.done())return false;
  }
  result=std::move(next);prefs.remove("state");return true;
 }
 bool save(const State& previous,const State& next){
  auto favorites=packFavorites(next);bool changedFavorites=favorites!=packFavorites(previous);
  int changed=-1;
  for(int i=0;i<SAVED_CODE_COUNT;i++)if(packSlot(previous.slots[i])!=packSlot(next.slots[i])){
   if(changedFavorites||changed>=0)return false;changed=i;
  }
  if(!changedFavorites&&changed<0)return true;
  if(!records&&!migrate(previous))return false;
  return changedFavorites?write("favorites",favorites):write(key(changed).c_str(),packSlot(next.slots[changed]));
 }
};
}
