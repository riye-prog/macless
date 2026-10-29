#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace toolscreen {
uint32_t profileColor(double value);
struct ProfileRow{std::string name;double percentage=0;double global=0;uint32_t color=0xffffff;};
struct DebugState{std::string counter;std::string path;double global=0;std::vector<ProfileRow> rows;};
struct Prediction{double x=0,z=0,certainty=0,distance=0,angle=0,correction=0;};
struct EyeThrow{double x=0,z=0,angle=0,error=0,correction=0;std::string type;};
struct NinjaState{
    bool connected=false;
    std::string status="Waiting for Ninjabrain Bot",version,resultType,boatState="NONE",blindEvaluation,divineSafe,divineHighroll;
    double boatAngle=0,blindChance=0,blindThreshold=400,blindDistance=0,blindDirection=0,playerX=0,playerZ=0,playerAngle=0;
    bool hasBoatAngle=false,blind=false,divine=false,playerValid=false;
    std::vector<Prediction> predictions;
    std::vector<EyeThrow> eyes;
    std::vector<std::pair<std::string,std::string>> messages;
};
DebugState debugState();
void clearDebug();
void updateCounter(const std::string& value);
void updatePie(const std::string& path,double global,const std::vector<ProfileRow>& rows);
bool maplessRow(const DebugState& state,ProfileRow& row,bool follow=true);
bool maplessRow(ProfileRow& row,bool follow=true);
NinjaState ninjaState();
void setNinja(const NinjaState& value);
void ninjaUpdate(const std::string& json,const std::string& error);
}
