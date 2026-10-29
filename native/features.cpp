#include "features.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <mutex>

namespace {
std::mutex featureMutex;
toolscreen::DebugState currentDebug;
toolscreen::NinjaState currentNinja;
}

namespace toolscreen {
uint32_t profileColor(double value){return std::isfinite(value)?uint32_t(int64_t(std::clamp(value,-2147483648.0,4294967295.0)))&0xffffff:0xffffff;}
DebugState debugState(){std::lock_guard lock(featureMutex);return currentDebug;}
void clearDebug(){std::lock_guard lock(featureMutex);currentDebug={};}
void updateCounter(const std::string& value){std::lock_guard lock(featureMutex);currentDebug.counter=value.substr(0,128);}
void updatePie(const std::string& path,double global,const std::vector<ProfileRow>& rows){std::lock_guard lock(featureMutex);currentDebug.path=path.substr(0,1024);currentDebug.global=std::isfinite(global)?std::clamp(global,0.0,100.0):0;currentDebug.rows.clear();for(auto row:rows){if(currentDebug.rows.size()>=32)break;if(!std::isfinite(row.percentage)||!std::isfinite(row.global))continue;row.name=row.name.substr(0,128);row.percentage=std::clamp(row.percentage,0.0,100.0);row.global=std::clamp(row.global,0.0,100.0);currentDebug.rows.push_back(row);}}
bool maplessRow(const DebugState& state,ProfileRow& row,bool){for(const auto& item:state.rows){std::string name=item.name;std::transform(name.begin(),name.end(),name.begin(),[](unsigned char value){return char(std::tolower(value));});if(name=="blockentities"||name=="block_entities"){row=item;return true;}}std::string path=state.path;std::transform(path.begin(),path.end(),path.begin(),[](unsigned char value){return char(std::tolower(value));});if(path.ends_with("blockentities")||path.ends_with("block_entities")){std::string name=state.path.substr(state.path.find_last_of('/')+1);uint32_t hash=0;for(unsigned char character:name)hash=hash*31+character;row={name,100,state.global,(hash&0xaaaaaa)+0x444444};return true;}return false;}
bool maplessRow(ProfileRow& row,bool follow){return maplessRow(debugState(),row,follow);}

NinjaState ninjaState(){std::lock_guard lock(featureMutex);return currentNinja;}
void setNinja(const NinjaState& value){std::lock_guard lock(featureMutex);currentNinja=value;}
}

extern "C" {
void ts_debug_counter(const char* value){toolscreen::updateCounter(value?value:"");}
void ts_debug_clear(){toolscreen::clearDebug();}
void ts_debug_pie(const char* path,const char* names,const double* values,int count){if(!values||count<0||count>32)return;std::vector<toolscreen::ProfileRow> rows;std::string labels=names?names:"";size_t start=0;for(int i=0;i<count;++i){size_t end=labels.find('\n',start);rows.push_back({labels.substr(start,end==std::string::npos?end:end-start),values[1+i*3],values[2+i*3],toolscreen::profileColor(values[3+i*3])});start=end==std::string::npos?labels.size():end+1;}toolscreen::updatePie(path?path:"",values[0],rows);}
int ts_feature_probe(double* output,int capacity){if(!output||capacity<10)return -10;auto debug=toolscreen::debugState();auto ninja=toolscreen::ninjaState();toolscreen::ProfileRow row;bool mapless=toolscreen::maplessRow(row);double values[]={debug.counter.empty()?0.0:1.0,double(debug.rows.size()),mapless?1.0:0.0,mapless?row.global:0.0,double(ninja.predictions.size()),ninja.connected?1.0:0.0,ninja.predictions.empty()?0:ninja.predictions[0].x,ninja.predictions.empty()?0:ninja.predictions[0].z,ninja.predictions.empty()?0:ninja.predictions[0].certainty,ninja.predictions.empty()?0:ninja.predictions[0].correction};std::copy(values,values+10,output);return 10;}
}
