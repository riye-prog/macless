#include "features.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <regex>

namespace {
using Json=nlohmann::json;
const Json emptyObject=Json::object(),emptyArray=Json::array();
const Json& object(const Json& source,const char* key){auto item=source.find(key);return item!=source.end()&&item->is_object()?*item:emptyObject;}
const Json& array(const Json& source,const char* key){auto item=source.find(key);return item!=source.end()&&item->is_array()?*item:emptyArray;}
double number(const Json& source,const char* key,double fallback=0){auto item=source.find(key);if(item==source.end()||!item->is_number())return fallback;double value=item->get<double>();return std::isfinite(value)?value:fallback;}
std::string text(const Json& source,const char* key){auto item=source.find(key);return item!=source.end()&&item->is_string()?item->get<std::string>():"";}
bool boolean(const Json& source,const char* key){auto item=source.find(key);return item!=source.end()&&((item->is_boolean()&&item->get<bool>())||(item->is_number()&&number(source,key)!=0));}
std::string plainText(std::string value){value=std::regex_replace(value,std::regex("<[^>]*>"),"");for(auto pair:{std::pair{"&lt;","<"},std::pair{"&gt;",">"},std::pair{"&amp;","&"}}){size_t at=0;while((at=value.find(pair.first,at))!=std::string::npos){value.replace(at,std::char_traits<char>::length(pair.first),pair.second);at+=std::char_traits<char>::length(pair.second);}}return value;}
}

namespace toolscreen {
void ninjaUpdate(const std::string& json,const std::string& error){
    NinjaState result;if(!error.empty()){result.status=error.substr(0,512);setNinja(result);return;}if(json.size()>262144){result.status="Ninjabrain response exceeded the size limit";setNinja(result);return;}
    try{
        auto root=Json::parse(json);if(!root.is_object())throw std::invalid_argument("object");auto& stronghold=object(root,"stronghold");if(!stronghold.contains("resultType")||!stronghold["resultType"].is_string()||!stronghold.contains("predictions")||!stronghold["predictions"].is_array()){result.status="Invalid Ninjabrain stronghold response";setNinja(result);return;}
        result.connected=true;result.status="Connected";result.version=text(object(root,"version"),"version");result.resultType=text(stronghold,"resultType");auto& player=object(stronghold,"playerPosition");result.playerValid=player.contains("xInOverworld")&&player.contains("zInOverworld");result.playerX=number(player,"xInOverworld");result.playerZ=number(player,"zInOverworld");result.playerAngle=number(player,"horizontalAngle");
        for(auto& value:array(stronghold,"predictions")){if(result.predictions.size()>=5)break;if(!value.is_object()||!value.contains("chunkX")||!value["chunkX"].is_number()||!value.contains("chunkZ")||!value["chunkZ"].is_number()||std::abs(number(value,"chunkX"))>1875000||std::abs(number(value,"chunkZ"))>1875000)continue;Prediction prediction;prediction.x=number(value,"chunkX")*16+4;prediction.z=number(value,"chunkZ")*16+4;prediction.certainty=std::clamp(number(value,"certainty"),0.0,1.0);prediction.distance=number(value,"overworldDistance");result.predictions.push_back(prediction);}
        for(auto& value:array(stronghold,"eyeThrows")){if(result.eyes.size()>=8)break;result.eyes.push_back({number(value,"xInOverworld"),number(value,"zInOverworld"),number(value,"angle"),number(value,"error"),number(value,"correction"),text(value,"type")});}
        auto& boat=object(root,"boat");result.boatState=text(boat,"boatState");result.hasBoatAngle=boat.contains("boatAngle");result.boatAngle=number(boat,"boatAngle");auto& blind=object(root,"blind");result.blind=boolean(blind,"isBlindModeEnabled");auto& blindResult=object(blind,"blindResult");result.blindEvaluation=text(blindResult,"evaluation");result.blindChance=std::clamp(number(blindResult,"highrollProbability"),0.0,1.0);result.blindThreshold=number(blindResult,"highrollThreshold",400);result.blindDistance=number(blindResult,"improveDistance");result.blindDirection=number(blindResult,"improveDirection");auto& divine=object(root,"divine");result.divine=boolean(divine,"isDivineModeEnabled");auto& divineResult=object(divine,"divineResult");result.divineSafe=text(divineResult,"formattedSafeCoords");result.divineHighroll=text(divineResult,"formattedHighrollCoords");
        for(auto& value:array(object(root,"messages"),"informationMessages")){if(result.messages.size()>=8)break;result.messages.emplace_back(text(value,"severity"),plainText(text(value,"message")));}setNinja(result);
    }catch(const std::exception&){result={};result.status="Invalid Ninjabrain API response";setNinja(result);}
}
}
extern "C" {
void ts_ninja_update(const char* json,const char* error){toolscreen::ninjaUpdate(json?json:"",error?error:"");}
}
