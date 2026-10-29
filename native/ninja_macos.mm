#import <Foundation/Foundation.h>
#include "features.hpp"
#include <algorithm>
#include <cmath>
namespace {
NSDictionary* object(id value){return [value isKindOfClass:[NSDictionary class]]?value:@{};}
NSArray* array(id value){return [value isKindOfClass:[NSArray class]]?value:@[];}
double number(NSDictionary* value,NSString* key,double fallback=0){id item=value[key];double result=[item isKindOfClass:[NSNumber class]]?[item doubleValue]:fallback;return std::isfinite(result)?result:fallback;}
std::string text(id value){return [value isKindOfClass:[NSString class]]?std::string([(NSString*)value UTF8String]):"";}
bool boolean(NSDictionary* value,NSString* key){id item=value[key];return [item isKindOfClass:[NSNumber class]]&&[item boolValue];}
std::string plainText(id value){if(![value isKindOfClass:[NSString class]])return "";NSString* source=value;NSRegularExpression* pattern=[NSRegularExpression regularExpressionWithPattern:@"<[^>]*>" options:0 error:nil];NSString* result=[pattern stringByReplacingMatchesInString:source options:0 range:NSMakeRange(0,source.length) withTemplate:@""];result=[result stringByReplacingOccurrencesOfString:@"&lt;" withString:@"<"];result=[result stringByReplacingOccurrencesOfString:@"&gt;" withString:@">"];result=[result stringByReplacingOccurrencesOfString:@"&amp;" withString:@"&"];return text(result);}
}
namespace toolscreen {
void ninjaUpdate(const std::string& json,const std::string& error){
    NinjaState result;if(!error.empty()){result.status=error.substr(0,512);setNinja(result);return;}
    @autoreleasepool{
        if(json.size()>262144){result.status="Ninjabrain response exceeded the size limit";setNinja(result);return;}
        NSData* data=[NSData dataWithBytes:json.data() length:json.size()];NSError* failure=nil;id decoded=[NSJSONSerialization JSONObjectWithData:data options:0 error:&failure];if(failure||![decoded isKindOfClass:[NSDictionary class]]){result.status="Invalid Ninjabrain API response";setNinja(result);return;}NSDictionary* root=decoded;NSDictionary* stronghold=object(root[@"stronghold"]);if(![stronghold[@"resultType"] isKindOfClass:[NSString class]]||![stronghold[@"predictions"] isKindOfClass:[NSArray class]]){result.status="Invalid Ninjabrain stronghold response";setNinja(result);return;}result.connected=true;result.status="Connected";result.version=text(object(root[@"version"])[@"version"]);result.resultType=text(stronghold[@"resultType"]);
        NSDictionary* player=object(stronghold[@"playerPosition"]);result.playerValid=player[@"xInOverworld"]&&player[@"zInOverworld"];result.playerX=number(player,@"xInOverworld");result.playerZ=number(player,@"zInOverworld");result.playerAngle=number(player,@"horizontalAngle");
        for(id item:array(stronghold[@"predictions"])){if(result.predictions.size()>=5)break;NSDictionary* value=object(item);if(![value[@"chunkX"] isKindOfClass:[NSNumber class]]||![value[@"chunkZ"] isKindOfClass:[NSNumber class]]||!std::isfinite([value[@"chunkX"] doubleValue])||!std::isfinite([value[@"chunkZ"] doubleValue])||std::abs([value[@"chunkX"] doubleValue])>1875000||std::abs([value[@"chunkZ"] doubleValue])>1875000)continue;Prediction prediction;prediction.x=number(value,@"chunkX")*16+4;prediction.z=number(value,@"chunkZ")*16+4;prediction.certainty=std::clamp(number(value,@"certainty"),0.0,1.0);prediction.distance=number(value,@"overworldDistance");result.predictions.push_back(prediction);}
        for(id item:array(stronghold[@"eyeThrows"])){if(result.eyes.size()>=8)break;NSDictionary* value=object(item);result.eyes.push_back({number(value,@"xInOverworld"),number(value,@"zInOverworld"),number(value,@"angle"),number(value,@"error"),number(value,@"correction"),text(value[@"type"])});}
        NSDictionary* boat=object(root[@"boat"]);result.boatState=text(boat[@"boatState"]);result.hasBoatAngle=boat[@"boatAngle"]!=nil;result.boatAngle=number(boat,@"boatAngle");NSDictionary* blind=object(root[@"blind"]);result.blind=boolean(blind,@"isBlindModeEnabled");NSDictionary* blindResult=object(blind[@"blindResult"]);result.blindEvaluation=text(blindResult[@"evaluation"]);result.blindChance=std::clamp(number(blindResult,@"highrollProbability"),0.0,1.0);result.blindThreshold=number(blindResult,@"highrollThreshold",400);result.blindDistance=number(blindResult,@"improveDistance");result.blindDirection=number(blindResult,@"improveDirection");NSDictionary* divine=object(root[@"divine"]);result.divine=boolean(divine,@"isDivineModeEnabled");NSDictionary* divineResult=object(divine[@"divineResult"]);result.divineSafe=text(divineResult[@"formattedSafeCoords"]);result.divineHighroll=text(divineResult[@"formattedHighrollCoords"]);
        for(id item:array(object(root[@"messages"])[@"informationMessages"])){if(result.messages.size()>=8)break;NSDictionary* value=object(item);result.messages.emplace_back(text(value[@"severity"]),plainText(value[@"message"]));}setNinja(result);
    }
}
}
extern "C" {
void ts_ninja_update(const char* json,const char* error){toolscreen::ninjaUpdate(json?json:"",error?error:"");}
}
