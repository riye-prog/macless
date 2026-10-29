#pragma once
#include "imgui.h"
#include "storage.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>

namespace toolscreen::ui {
struct WindowPlacement {
    float centerX=.5f,centerY=.5f,width=900,height=500;
};
class WindowLayout {
    std::string path;
    WindowPlacement placement;
    bool initialized=false,restorePending=false,dirty=false;
    double saveAt=0;
public:
    bool read(const std::string& directory){
        std::string requested=directory.empty()?"":toolscreen::pathText(toolscreen::filePath(directory)/"ui-state.conf");
        if(initialized&&requested==path)return restorePending;
        initialized=true;path=requested;restorePending=false;dirty=false;
        if(path.empty())return false;
        std::ifstream input(toolscreen::filePath(path));std::string signature;int version=0;WindowPlacement loaded;
        if(input>>signature>>version>>loaded.centerX>>loaded.centerY>>loaded.width>>loaded.height&&signature=="toolscreen-ui"&&(version==1||version==2)&&std::isfinite(loaded.centerX)&&std::isfinite(loaded.centerY)&&std::isfinite(loaded.width)&&std::isfinite(loaded.height)&&loaded.centerX>=0&&loaded.centerX<=1&&loaded.centerY>=0&&loaded.centerY<=1&&loaded.width>=320&&loaded.width<=2400&&loaded.height>=240&&loaded.height<=1800){if(version==1){loaded.width=std::min(loaded.width,900.0f);loaded.height=std::min(loaded.height,500.0f);}placement=loaded;restorePending=true;}
        return restorePending;
    }
    WindowPlacement restore(){restorePending=false;return placement;}
    void observe(ImVec2 position,ImVec2 size,ImVec2 display,float scale,double now,bool interacting){
        if(path.empty()||display.x<=0||display.y<=0||scale<=0)return;
        WindowPlacement current{(position.x+size.x*.5f)/display.x,(position.y+size.y*.5f)/display.y,size.x/scale,size.y/scale};
        if(std::abs(current.centerX-placement.centerX)>.0001f||std::abs(current.centerY-placement.centerY)>.0001f||std::abs(current.width-placement.width)>.1f||std::abs(current.height-placement.height)>.1f){placement=current;dirty=true;saveAt=now+.4;}
        if(!dirty||interacting||now<saveAt)return;
        std::error_code error;if(!std::filesystem::exists(toolscreen::filePath(path).parent_path(),error))return;
        std::string temporary=path+".tmp";
        {std::ofstream output(toolscreen::filePath(temporary),std::ios::trunc);output<<"toolscreen-ui 2\n"<<std::setprecision(9)<<placement.centerX<<' '<<placement.centerY<<' '<<placement.width<<' '<<placement.height<<'\n';output.flush();if(!output.good())return;}
        if(toolscreen::replaceFile(temporary,path))dirty=false;
    }
};
}
