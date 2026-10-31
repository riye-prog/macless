#include "core.hpp"
#include "storage.hpp"
#include "input.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <unordered_map>
#include <map>
#include <cstring>
#include <optional>

namespace toolscreen {
OverlayStyle defaultOverlayStyle(int type){
    OverlayStyle result{.94,.93,.89,.62,.63,.61,.055,.055,.055,1,1,0,1,1,type<=2?1.0:0.0,1,0,2,12,8,26,3,0,1,1,1,1};
    if(type==2){result[TextRed]=.90;result[TextGreen]=.92;result[TextBlue]=.95;result[MutedRed]=.61;result[MutedGreen]=.65;result[MutedBlue]=.70;result[BackgroundRed]=.075;result[BackgroundGreen]=.082;result[BackgroundBlue]=.095;result[FontFace]=0;result[TextOutline]=0;result[RowHeight]=30;result[Padding]=14;result[CornerRadius]=10;}
    if(type==3)result[ShowLocal]=0;
    return result;
}
Mirror mirrorForOutput(const Mirror& source,int route){auto result=source;if(route==1&&source.separateObs){result.values=source.obsValues;result.style=source.obsStyle;result.values[32]=source.values[32];result.values[14]=source.values[14];result.values[0]=source.values[0];result.values[33]=source.values[33];}return result;}
}

namespace {
using Clock = std::chrono::steady_clock;
struct Configuration {
    toolscreen::Settings settings{};
    std::vector<toolscreen::Mode> modes;
    std::vector<toolscreen::Mirror> mirrors;
    std::vector<toolscreen::ImageSource> images;
    std::vector<toolscreen::KeyRemap> remaps;
    std::array<std::vector<int>,17> bindings;
    std::map<std::string,std::string> options;
    int selectedMirror = 0;
    int nextSource = 100;
};
std::mutex stateMutex;
std::string configPath;
bool unsaved = false;
uint64_t editRevision = 0;
std::vector<Configuration> undoStack;
std::vector<Configuration> redoStack;
int previousEdit = -999;
int editTransactionDepth = 0;
bool editTransactionRecorded = false;
Clock::time_point previousEditTime{};
int heldModeKey = 0;
int modeBeforeHold = 0;
bool runtimeWorld=false;
double vanillaSensitivity=.5;
int displayWidth=0,displayHeight=0,textureLimit=16384;
std::optional<std::array<int,2>> oversizedResolution;
std::string modeFailure;
std::array<int,2> modeViewport(const Configuration& state,int mode,int width,int height,int limit){
    const auto& s=state.settings;double w=width,h=height;
    if(mode==1&&s[36]>.5){w=std::max(330,int(std::round(width/16.0))*2);h=std::max(2,int(std::round(height*.95/2))*2);}
    else if(mode==2&&s[37]>.5){w=std::max(2,int(std::round(width*.98/2))*2);h=std::max(1,int(std::round(height*.25)));}
    else if(mode>=1&&mode<=3){int offset=(mode-1)*2+1;w=s[offset]>0?s[offset]:width;h=s[offset+1]>0?s[offset+1]:height;}
    else if(mode==4){w=s[5];h=s[6];}
    else if(mode>=5&&mode<int(state.modes.size())){const auto& custom=state.modes[size_t(mode)];w=custom.width<=1?std::max(1.0,std::round(custom.width*width)):custom.width;h=custom.height<=1?std::max(1.0,std::round(custom.height*height)):custom.height;}
    double scale=std::min({1.0,std::min(limit,16384)/std::max(w,1.0),std::min(limit,16384)/std::max(h,1.0),std::sqrt(16777216.0/std::max(w*h,1.0))});
    return {std::max(1,int(w*scale)),std::max(1,int(h*scale))};
}
bool allowResolution(const Configuration& state,int mode){
    if(displayWidth<1||displayHeight<1||mode==0)return true;
    auto size=modeViewport(state,mode,displayWidth,displayHeight,textureLimit);
    if(size[0]<=displayWidth&&size[1]<=displayHeight){modeFailure.clear();return true;}
    if(oversizedResolution&&size!=*oversizedResolution){modeFailure="One oversized resolution is allowed per session";return false;}
    if(runtimeWorld&&!oversizedResolution)oversizedResolution=size;
    modeFailure.clear();return true;
}
void normalizeRemaps(std::vector<toolscreen::KeyRemap>& mappings){
    std::vector<toolscreen::KeyRemap> result;std::vector<int> used;
    for(const auto& mapping:mappings){if(mapping.sourceScan<1||mapping.targetScan<1||mapping.sourceScan==mapping.targetScan)continue;if(std::find(used.begin(),used.end(),mapping.sourceScan)!=used.end()||std::find(used.begin(),used.end(),mapping.targetScan)!=used.end())continue;used.push_back(mapping.sourceScan);used.push_back(mapping.targetScan);result.push_back(mapping);result.push_back({mapping.targetScan,mapping.targetKey,mapping.sourceScan,mapping.sourceKey});}
    mappings=std::move(result);
}

toolscreen::Mirror mirrorPreset(const std::string& name, int preset) {
    toolscreen::Mirror result{name, {0, .78, 0, .22, .09, .65, .04, .32, .14, 1, 0, .12, 0, 1, 65535, 0, .72, .79, .87, 1}};
    if (preset == 1) result.values = {1, 0, .02, .22, .08, .03, .03, .27, .10, 1, 0, .12, 0, 1, 65535, 1, .72, .79, .87, 1};
    if (preset == 2) result.values = {1, .74, .64, .26, .36, .76, .70, .22, .28, 1, 0, .12, 0, 1, 65535, 1, .72, .79, .87, 1};
    if (preset == 3) result.values = {1, 0, 0, .32, .40, .02, .25, .31, .42, 1, 0, .12, 0, 1, 65535, 1, .72, .79, .87, 1};
    if (preset == 4) result.values[0] = 1;
    if (preset == 1){auto& v=result.values;v[20]=1;v[21]=14;v[22]=38;v[23]=23;v[24]=7;v[26]=1;v[27]=v[28]=v[29]=221.0/255;v[30]=1;v[31]=8;v[11]=.01;}
    result.values[32]=1;
    if(preset>=1&&preset<=3){auto& v=result.values;v[33]=preset;v[31]=0;v[5]=.98;v[6]=preset==1?.12:preset==2?.46:.30;v[7]=preset==2?.31:preset==3?.17:.25;v[8]=preset==1?.07:preset==2?.34:.12;v[16]=.22;v[17]=.24;v[18]=.27;v[35]=1;v[36]=preset==2?1.0:.94;v[37]=1;v[15]=0;if(preset==2)v[38]=9;if(preset==3)v[34]=0;}
    result.values[23]=std::max(1.0,result.values[23]);result.values[24]=std::max(1.0,result.values[24]);result.style=toolscreen::defaultOverlayStyle(preset);result.obsStyle=result.style;result.obsValues=result.values;
    return result;
}

Configuration defaults() {
    Configuration result;
    auto& s = result.settings;
    std::array<double, 36> legacy{0, 384, 0, 0, 540, 384, 16384, 140, .047, .055, .078,
        1, 30, 1300, .04, .125, .32, .75, 1, 9, 0,
        .78, 0, .22, .09, .65, .04, .32, .14, 1, 0, .12, 0, 1, 1, 1};
    std::copy(legacy.begin(), legacy.end(), s.begin());
    s[36] = 1;
    s[37] = 1;
    s[38] = 1;
    s[39] = 1;
    s[40] = 1;
    s[41] = 'i';
    s[42] = 192;
    s[43] = 1;
    s[44] = 1; s[45] = 182.0/255; s[46] = 193.0/255;
    s[47] = 173.0/255; s[48] = 216.0/255; s[49] = 230.0/255;
    s[53] = s[54] = s[55] = 0;
    s[50] = s[51] = s[52] = 1;
    s[56] = 1;
    s[57] = 0;
    s[58] = 24;
    s[59] = 28;
    s[60] = s[61] = s[62] = 1;
    s[64] = 1073741891;
    s[65] = 1073741887;
    s[66] = 1073741888;
    s[67] = 1073741890;
    s[68] = 1073741886;
    s[114]=1;s[115]=1;s[116]=122.0/255;s[117]=21.0/255;s[118]=162.0/255;s[119]=1;
    s[123]=1;s[125]=.09;s[126]=.065;s[127]=.15;
    result.modes = {{"Fullscreen",0,0},{"Thin",384,0},{"Wide",0,540},{"EyeZoom",384,16384},{"Preemptive",384,16384}};
    s[128]=1;s[129]=0;s[130]=60;s[132]=1;s[133]=1;s[134]=1;s[135]=3;s[136]=.73;s[137]=.16;s[138]=.25;s[139]=18;s[140]=.94;s[141]=3;s[142]=1;s[143]=250;s[153]=1;s[145]=1;
    for(int i=0;i<5;++i)result.bindings[i]={toolscreen::scanFromCode(int(s[64+i]))};
    result.bindings[16]={12,224};
    result.options["ninja_url"]="http://127.0.0.1:52533";result.options["overlay_design"]="vanilla-3";result.options["eyezoom_grid"]="classic-9";result.options["approval_profile"]="v8-review-1";result.options["profiler_design"]="smooth-1";
    result.mirrors={mirrorPreset("E Counter",1),mirrorPreset("Profiler",2),mirrorPreset("Mapless",3)};
    return result;
}
Configuration config = defaults();
struct SourceDimensions{std::string path;int width;int height;};
std::unordered_map<int,SourceDimensions> sourceDimensions;

void recordEdit(int key) {
    ++editRevision;
    auto now = Clock::now();
    if ((editTransactionDepth>0&&!editTransactionRecorded)||(editTransactionDepth==0&&(key != previousEdit || now-previousEditTime > std::chrono::milliseconds(450)))) {
        undoStack.push_back(config);
        if (undoStack.size() > 64) undoStack.erase(undoStack.begin());
    }
    if(editTransactionDepth>0)editTransactionRecorded=true;
    redoStack.clear();
    previousEdit = key;
    previousEditTime = now;
    unsaved = true;
}

double sanitize(int index, double value) {
    if (index == 0) return std::round(std::clamp(value, 0.0, double(config.modes.size()-1)));
    if ((index >= 1 && index <= 6) || index == 12 || index == 13) return std::round(std::clamp(value, 0.0, 16384.0));
    if (index == 7) return std::clamp(value, 0.0, 600.0);
    if (index == 19) return std::round(std::clamp(value, 1.0, 9.0));
    if (index == 34) return std::clamp(value, .75, 2.0);
    if (index == 39 || index == 40) return std::clamp(value, .001, 20.0);
    if(index==145)return value>=.5?1:0;
    if (index == 41 || (index >= 64 && index < 80)) return std::round(std::clamp(value, 0.0, 2147483647.0));
    if (index == 42 || (index >= 80 && index < 96)) return std::round(std::clamp(value, 0.0, 4095.0));
    if (index == 58 || index == 59) return std::clamp(value, 8.0, 96.0);
    if (index == 57) return std::round(std::clamp(value, 0.0, 2.0));
    if(index==115)return std::clamp(value,0.0,12.0);
    if(index==120)return std::clamp(value,0.0,48.0);
    if(index==121)return std::round(std::clamp(value,0.0,2147483647.0));
    if(index==122||index==124)return std::round(std::clamp(value,0.0,2.0));
    if(index==114)return value>=.5?1:0;
    if(index==130)return std::round(std::clamp(value,15.0,120.0));
    if(index==132||index==133||index==135||index==153)return std::round(std::clamp(value,0.0,3.0));
    if(index==139)return std::clamp(value,10.0,48.0);
    if(index==141)return std::round(std::clamp(value,1.0,5.0));
    if(index==143)return std::round(std::clamp(value,100.0,2000.0));
    if(index==128||index==129||index==134||index==142)return value>=.5?1:0;
    if (index == 11 || index == 18 || index == 35 || index == 36 || index == 37 || index == 38 || index == 43 || (index >= 60 && index <= 63) || (index >= 96 && index <= 113)) return value >= .5 ? 1 : 0;
    return std::clamp(value, 0.0, 1.0);
}

double sanitizeMirror(int field, double value) {
    if (field == 0 || field == 10 || field == 12 || field == 13 || field == 20 || field == 26 || field == 30 || field == 37) return value >= .5 ? 1 : 0;
    if (field == 14) return std::round(std::clamp(value, 0.0, 65535.0));
    if (field == 15) return std::clamp(value, 0.0, 20.0);
    if(field==21||field==22)return std::round(std::clamp(value,-16384.0,16384.0));
    if(field==23||field==24)return std::round(std::clamp(value,1.0,16384.0));
    if(field==25)return std::round(std::clamp(value,0.0,4.0));
    if(field==31)return std::clamp(value,0.0,32.0);
    if(field==34)return 0;
    if(field==38)return std::round(std::clamp(value,0.0,32.0));
    if(field==39)return std::clamp(value,0.0,1.6);
    if(field==35)return std::clamp(value,.5,2.5);
    if(field==32||field==33)return std::round(std::clamp(value,0.0,3.0));
    return std::clamp(value, 0.0, 1.0);
}

toolscreen::Quad quad(double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh,
                     int flags, double threshold, double seconds, double opacity, double pixelWidth, double pixelHeight, double cloneWidth, int source = 0,
                     double red = 0, double green = 0, double blue = 0) {
    return {float(sx),float(std::clamp(1-sy-sh,0.0,1.0)),float(sw),float(sh),
        float(dx),float(1-dy-dh),float(dw),float(dh),float(red),float(green),float(blue),1,
        float(flags),float(threshold),float(seconds),float(opacity),float(pixelWidth),float(pixelHeight),float(cloneWidth),float(source)};
}

void appendSource(std::vector<toolscreen::Quad>& output, const toolscreen::SourceValues& v, int source, int mode,
                  int width, int height, int viewWidth, int viewHeight, double seconds) {
    if (v[0] < .5 || (int(v[14]) & (1 << mode)) == 0 || v[33]>.5) return;
    double sw = std::clamp(v[3], 1.0/viewWidth, 1.0);
    double sh = std::clamp(v[4], 1.0/viewHeight, 1.0);
    double sx = std::min(v[1], 1-sw);
    double sy = std::min(v[2], 1-sh);
    if(v[20]>.5){double cropWidth=std::clamp(v[23],1.0,double(viewWidth)),cropHeight=std::clamp(v[24],1.0,double(viewHeight));int anchor=int(v[25]);double x=(anchor==1||anchor==3?viewWidth-cropWidth:anchor==4?viewWidth/2-int(cropWidth)/2:0)+v[21];double y=(anchor==2||anchor==3?viewHeight-cropHeight:anchor==4?viewHeight/2-int(cropHeight)/2:0)+v[22];sx=std::clamp(x,0.0,viewWidth-cropWidth)/viewWidth;sy=std::clamp(y,0.0,viewHeight-cropHeight)/viewHeight;sw=cropWidth/viewWidth;sh=cropHeight/viewHeight;}
    double dw = std::max(v[7], .005);
    double dh = std::max(v[8], .005);
    if(v[31]>0){dw=std::min(1.0,sw*viewWidth*v[31]/width);dh=std::min(1.0,sh*viewHeight*v[31]/height);}
    if (v[15] > 0) {
        double bx = v[15]/width, by = v[15]/height;
        output.push_back(quad(0,0,1,1,v[5]-bx,v[6]-by,dw+2*bx,dh+2*by,8,0,seconds,v[19]*v[9],0,0,0,0,v[16],v[17],v[18]));
    }
    int flags = (v[10] > .5 && v[26]<.5 ? 1 : 0) | (v[12] > .5 ? 2 : 0) | (v[13] > .5 ? 64 : 0) | (source>0?16:512) | (v[26]>.5?4:0) | (v[30]>.5?256:0);
    output.push_back(quad(sx,sy,sw,sh,v[5],v[6],dw,dh,flags,v[11],seconds,v[9],dw*width,dh*height,0,source,v[26]>.5?v[27]:0,v[26]>.5?v[28]:0,v[26]>.5?v[29]:0));
}

double sanitizeOverlay(int field,double value){using namespace toolscreen;double maximum=field<=BackgroundBlue?1:field<=ShowFooter?1:field==Decimals?2:field==Padding?40:field==CornerRadius?30:field==RowHeight?64:field==FontFace?3:field==TextAlignment?2:field==ValueScale||field==TitleScale?3:1;double minimum=field==RowHeight?18:field==ValueScale||field==TitleScale?.5:0;value=std::clamp(value,minimum,maximum);if((field>=ShowTitle&&field<=ShowFooter)||field==FontFace||field==TextAlignment||field==Decimals||field==AutoWidth||field==TextOutline)value=std::round(value);return value;}

bool parse(const std::string& path, Configuration& result) {
    std::ifstream stream(toolscreen::filePath(path));
    std::string magic;
    int version = 0;
    if (!(stream >> magic >> version) || magic != "toolscreen" || version<1 || version>6) return false;
    result = defaults();result.options.erase("overlay_design");result.options.erase("eyezoom_grid");result.options.erase("approval_profile");result.options.erase("profiler_design");
    int count = version == 1 ? 36 : toolscreen::settingCount;
    if (version >= 2 && (!(stream >> count) || count != (version<4?128:toolscreen::settingCount))) return false;
    for (int i = 0; i < count; ++i) if (!(stream >> result.settings[i]) || !std::isfinite(result.settings[i])) return false;
    if (version == 1) {
        if (result.settings[6] == 8192) result.settings[6] = 16384;
        result.settings[7] = 140;
        result.settings[19] = std::clamp(result.settings[19]/2,1.0,48.0);
        result.settings[35] = 1;
        result.settings[36] = result.settings[1] == 384 && result.settings[2] == 0 ? 1 : 0;
        result.settings[37] = result.settings[3] == 0 && result.settings[4] == 540 ? 1 : 0;
        for (int i = 0; i < 14; ++i) result.mirrors[0].values[i] = sanitizeMirror(i,result.settings[20+i]);
        return true;
    }
    int modeCount = 0;
    if (!(stream >> modeCount) || modeCount < 5 || modeCount > 16) return false;
    result.modes.clear();
    for (int i = 0; i < modeCount; ++i) {
        toolscreen::Mode mode;
        if (!(stream >> std::quoted(mode.name) >> mode.width >> mode.height) || !std::isfinite(mode.width) || !std::isfinite(mode.height)) return false;
        mode.width = std::clamp(mode.width,0.0,16384.0);mode.height=std::clamp(mode.height,0.0,16384.0);
        result.modes.push_back(mode);
    }
    int mirrorCount = 0;
    if (!(stream >> mirrorCount) || mirrorCount < 1 || mirrorCount > 32) return false;
    result.mirrors.clear();
    for (int i = 0; i < mirrorCount; ++i) {
        toolscreen::Mirror mirror{};
        if (!(stream >> std::quoted(mirror.name))) return false;
        for (int field = 0; field < (version>=4?40:version>=3?32:20); ++field) {
            double value;
            if (!(stream >> value) || !std::isfinite(value)) return false;
            mirror.values[field] = sanitizeMirror(field,value);
        }
        result.mirrors.push_back(mirror);
        if(version<4){auto& v=result.mirrors.back().values;v[32]=3;if(mirror.name=="E Counter"||mirror.name=="Counter")v[33]=1;else if(mirror.name=="Debug pie"||mirror.name=="Pie")v[33]=2;else if(mirror.name=="Mapless"){v[33]=3;v[34]=1;}else if(mirror.name=="Timer")v[0]=0;}
    }
    int imageCount = 0;
    if (!(stream >> imageCount) || imageCount < 0 || imageCount > 32) return false;
    for (int i = 0; i < imageCount; ++i) {
        toolscreen::ImageSource image{};
        if (!(stream >> image.id >> std::quoted(image.name) >> std::quoted(image.path)) || image.id < 100) return false;
        for (int field = 0; field < (version>=4?40:version>=3?32:20); ++field) {
            double value;
            if (!(stream >> value) || !std::isfinite(value)) return false;
            image.values[field] = sanitizeMirror(field,value);
        }
        result.nextSource = std::max(result.nextSource,image.id+1);
        result.images.push_back(image);
        if(version<4)result.images.back().values[32]=3;
    }
    result.settings[0] = std::clamp(result.settings[0],0.0,double(result.modes.size()-1));
    int remapCount=0;
    if(stream>>remapCount){if(remapCount<0||remapCount>128)return false;for(int i=0;i<remapCount;++i){toolscreen::KeyRemap mapping;if(!(stream>>mapping.sourceScan>>mapping.sourceKey>>mapping.targetScan>>mapping.targetKey)||mapping.sourceScan<0||mapping.sourceScan>512||mapping.targetScan<0||mapping.targetScan>512)return false;result.remaps.push_back(mapping);}}
    else if(!stream.eof())return false;
    if(version>=4){int count=0;if(!(stream>>count)||count!=17)return false;for(auto& keys:result.bindings){int size=0;if(!(stream>>size)||size<0||size>8)return false;keys.clear();for(int i=0;i<size;++i){int key=0;if(!(stream>>key)||key<1||key>511)return false;keys.push_back(key);}}int options=0;if(!(stream>>options)||options<0||options>32)return false;for(int i=0;i<options;++i){std::string name,value;if(!(stream>>std::quoted(name)>>std::quoted(value))||name.size()>128||value.size()>4096)return false;result.options[name]=value;}}
    else for(int i=0;i<16;++i){auto keys=toolscreen::modifierScans(((int(result.settings[80+i])&1)?1:0)|((int(result.settings[80+i])&2)?64:0)|((int(result.settings[80+i])&4)?256:0)|((int(result.settings[80+i])&8)?1024:0));int key=toolscreen::scanFromCode(int(result.settings[64+i]));if(key>0)keys.push_back(key);result.bindings[i]=keys;}
    if(version<4){result.mirrors.erase(std::remove_if(result.mirrors.begin(),result.mirrors.end(),[](auto& mirror){return mirror.values[33]<1||mirror.values[33]>3;}),result.mirrors.end());const char* names[]={"E Counter","Debug pie","Mapless"};for(int type=1;type<=3;++type)if(std::none_of(result.mirrors.begin(),result.mirrors.end(),[type](auto& mirror){return int(mirror.values[33])==type;}))result.mirrors.push_back(mirrorPreset(names[type-1],type));}
    if(version<5)for(auto& mirror:result.mirrors){mirror.values[35]=1;mirror.values[36]=.99;mirror.values[37]=1;}
    for(auto& mirror:result.mirrors){mirror.style=toolscreen::defaultOverlayStyle(int(mirror.values[33]));if(int(mirror.values[33])==1){mirror.style[toolscreen::ShowTitle]=0;if(version<6){mirror.values[36]=0;mirror.values[15]=0;}}mirror.obsValues=mirror.values;mirror.obsStyle=mirror.style;}
    if(version>=6){int count=0;if(!(stream>>count)||count!=int(result.mirrors.size()))return false;for(auto& mirror:result.mirrors){int separate=0,initialized=0;if(!(stream>>separate>>initialized)||(separate!=0&&separate!=1)||(initialized!=0&&initialized!=1))return false;mirror.separateObs=separate;mirror.obsInitialized=initialized;for(int field=0;field<toolscreen::OverlayPropertyCount;++field){double value;if(!(stream>>value)||!std::isfinite(value))return false;mirror.style[field]=sanitizeOverlay(field,value);}for(int field=0;field<40;++field){double value;if(!(stream>>value)||!std::isfinite(value))return false;mirror.obsValues[field]=sanitizeMirror(field,value);}for(int field=0;field<toolscreen::OverlayPropertyCount;++field){double value;if(!(stream>>value)||!std::isfinite(value))return false;mirror.obsStyle[field]=sanitizeOverlay(field,value);}}}
    result.options.erase("syphon_name");
    return true;
}
}

namespace toolscreen {
Settings snapshot() {
    std::lock_guard lock(stateMutex);
    Settings result = config.settings;
    for (int i=0;i<14;++i) result[20+i]=config.mirrors[size_t(config.selectedMirror)].values[i];
    return result;
}
void runtimeContext(bool inWorld,double baseSensitivity){std::lock_guard lock(stateMutex);runtimeWorld=inWorld;if(std::isfinite(baseSensitivity))vanillaSensitivity=std::clamp(baseSensitivity,0.0,1.0);}
std::string resolutionError(){std::lock_guard lock(stateMutex);return modeFailure;}
bool set(int index,double value) {
    if (index<0 || index>=settingCount || !std::isfinite(value)) return false;
    std::lock_guard lock(stateMutex);
    if (index>=20 && index<=33) {
        value = sanitizeMirror(index-20,value);
        if (config.mirrors[size_t(config.selectedMirror)].values[index-20] != value) {
            recordEdit(index);config.mirrors[size_t(config.selectedMirror)].values[index-20]=value;
        }
    } else {
        value=sanitize(index,value);
        if(index==0&&!allowResolution(config,int(value)))return false;
        if((index>=1&&index<=6)||index==36||index==37){auto candidate=config;candidate.settings[index]=value;if(!allowResolution(candidate,int(candidate.settings[0])))return false;}
        if((index==39||index==40)&&config.settings[38]<.5){recordEdit(index);config.settings[38]=1;}
        if(index==39||index==40){int other=index==39?40:39;if(config.settings[other]!=value){recordEdit(index);config.settings[other]=value;}}
        if (config.settings[index]!=value) { recordEdit(index);config.settings[index]=value;if(index>=64&&index<96){int mode=(index-64)%16;auto keys=modifierScans(((int(config.settings[80+mode])&1)?1:0)|((int(config.settings[80+mode])&2)?64:0)|((int(config.settings[80+mode])&4)?256:0)|((int(config.settings[80+mode])&8)?1024:0));int key=scanFromCode(int(config.settings[64+mode]));if(key>0)keys.push_back(key);config.bindings[mode]=keys;}}
    }
    return true;
}
void reset() { releasePhysicalInput(false);std::lock_guard lock(stateMutex);recordEdit(-1);config=defaults();heldModeKey=0; }
bool load(const std::string& path) {
    std::lock_guard lock(stateMutex);configPath=path;
    if (!std::filesystem::exists(filePath(path))) return true;
    Configuration loaded;
    bool recovered=false;
    if (!parse(path,loaded)) { if (!parse(path+".bak",loaded)) return false; recovered=true; }
    bool upgraded=loaded.options["overlay_design"]!="vanilla-3",previousDesign=loaded.options["overlay_design"]=="vanilla-2";
    if(upgraded){
        for(auto& mirror:loaded.mirrors){
            int type=int(mirror.values[33]);if(type<1||type>3)continue;
            OverlayStyle previous{1,1,1,.67,.67,.67,.025,.025,.025,1,1,0,1,1,type==2?1.0:0.0,1,0,type==3?3.0:2.0,10,0,20,type>=2?3.0:0.0,0,1,1,1,1};
            if(type==1)previous[ShowTitle]=0;
            if(type==3){previous[ShowDetails]=0;previous[ShowLocal]=0;}
            auto next=defaultOverlayStyle(type);if(previousDesign){if(mirror.style[CornerRadius]==0)mirror.style[CornerRadius]=next[CornerRadius];continue;}
            for(int field=0;field<OverlayPropertyCount;++field)if(std::abs(mirror.style[field]-previous[field])<.0001)mirror.style[field]=next[field];
            double previousBackground=type==1?0:.66;if(std::abs(mirror.values[36]-previousBackground)<.0001)mirror.values[36]=.94;
            if(std::abs(mirror.values[5]-(type==2?.67:.73))<.0001)mirror.values[5]=.98;
            double previousY=type==1?.04:type==2?.63:.44;if(std::abs(mirror.values[6]-previousY)<.0001)mirror.values[6]=type==1?.12:type==2?.98:.30;
            if(type==2&&mirror.name=="Debug pie")mirror.name="Profiler";
        }
        loaded.options["overlay_design"]="vanilla-3";
    }
    bool gridUpgraded=loaded.options["eyezoom_grid"]!="classic-9";
    if(gridUpgraded){
        const double changedGrid[]={.34,.29,.21,.23,.28,.35};
        const double originalGrid[]={1,182.0/255,193.0/255,173.0/255,216.0/255,230.0/255};
        for(int group=0;group<2;++group){bool unchanged=true;for(int channel=0;channel<3;++channel)unchanged=unchanged&&std::abs(loaded.settings[44+group*3+channel]-changedGrid[group*3+channel])<.0001;if(unchanged)for(int channel=0;channel<3;++channel)loaded.settings[44+group*3+channel]=originalGrid[group*3+channel];}
        if(std::abs(loaded.settings[53]-.94)<.0001&&std::abs(loaded.settings[54]-.94)<.0001&&std::abs(loaded.settings[55]-.94)<.0001)loaded.settings[53]=loaded.settings[54]=loaded.settings[55]=0;
        if(std::abs(loaded.settings[56]-.92)<.0001)loaded.settings[56]=1;
        loaded.options["eyezoom_grid"]="classic-9";
    }
    loaded.settings[129]=0;
    for(auto& mirror:loaded.mirrors){if(!(int(mirror.values[32])&1))mirror.values[0]=0;mirror.values[32]=1;mirror.separateObs=false;}
    for(auto& source:loaded.images){if(!(int(source.values[32])&1))source.values[0]=0;source.values[32]=1;}
    bool approvalUpgraded=loaded.options["approval_profile"]!="v8-review-1";
    normalizeRemaps(loaded.remaps);
    if(approvalUpgraded){for(auto& mirror:loaded.mirrors){mirror.values[34]=0;if(int(mirror.values[33])==2&&std::abs(mirror.values[6]-.98)<.0001)mirror.values[6]=.46;mirror.style[Decimals]=std::min(2.0,mirror.style[Decimals]);mirror.obsStyle[Decimals]=std::min(2.0,mirror.obsStyle[Decimals]);}loaded.options["approval_profile"]="v8-review-1";}
    bool profilerUpgraded=loaded.options["profiler_design"]!="smooth-1";
    if(profilerUpgraded){
        OverlayStyle previous{.94,.93,.89,.62,.63,.61,.055,.055,.055,1,1,0,1,1,1,1,0,2,12,8,26,3,0,1,1,1,1};auto next=defaultOverlayStyle(2);
        for(auto& mirror:loaded.mirrors)if(int(mirror.values[33])==2){for(auto* style:{&mirror.style,&mirror.obsStyle})for(int field=0;field<OverlayPropertyCount;++field)if(std::abs((*style)[field]-previous[field])<.0001)(*style)[field]=next[field];if(std::abs(mirror.values[36]-.94)<.0001)mirror.values[36]=1.0;}
        loaded.options["profiler_design"]="smooth-1";
    }
    config=loaded;
    for(int i=0;i<settingCount;++i) config.settings[i]=sanitize(i,config.settings[i]);
    unsaved=recovered||upgraded||gridUpgraded||approvalUpgraded||profilerUpgraded;undoStack.clear();redoStack.clear();previousEdit=-999;
    return true;
}
bool save() {
    std::lock_guard lock(stateMutex);
    if(configPath.empty())return false;
    std::string temporary=configPath+".tmp";
    {
        std::ofstream stream(filePath(temporary),std::ios::trunc);
        stream<<"toolscreen 6\n"<<settingCount<<'\n'<<std::setprecision(17);
        for(int i=0;i<settingCount;++i)stream<<(i==0?(heldModeKey?modeBeforeHold:inputPersistentMode(int(config.settings[0]))):config.settings[i])<<'\n';
        stream<<config.modes.size()<<'\n';
        for(auto& mode:config.modes)stream<<std::quoted(mode.name)<<' '<<mode.width<<' '<<mode.height<<'\n';
        stream<<config.mirrors.size()<<'\n';
        for(auto& mirror:config.mirrors){stream<<std::quoted(mirror.name);for(double value:mirror.values)stream<<' '<<value;stream<<'\n';}
        stream<<config.images.size()<<'\n';
        for(auto& image:config.images){stream<<image.id<<' '<<std::quoted(image.name)<<' '<<std::quoted(image.path);for(double value:image.values)stream<<' '<<value;stream<<'\n';}
        stream<<config.remaps.size()<<'\n';for(auto& mapping:config.remaps)stream<<mapping.sourceScan<<' '<<mapping.sourceKey<<' '<<mapping.targetScan<<' '<<mapping.targetKey<<'\n';
        stream<<17<<'\n';for(auto& keys:config.bindings){stream<<keys.size();for(int key:keys)stream<<' '<<key;stream<<'\n';}stream<<config.options.size()<<'\n';for(auto& [name,value]:config.options)stream<<std::quoted(name)<<' '<<std::quoted(value)<<'\n';
        stream<<config.mirrors.size()<<'\n';for(auto& mirror:config.mirrors){stream<<mirror.separateObs<<' '<<mirror.obsInitialized;for(double value:mirror.style)stream<<' '<<value;for(double value:mirror.obsValues)stream<<' '<<value;for(double value:mirror.obsStyle)stream<<' '<<value;stream<<'\n';}
        stream.flush();if(!stream.good())return false;
    }
    {std::ifstream source(filePath(configPath));std::string signature;int version=0;if(source>>signature>>version&&signature=="toolscreen"&&version>=1&&version<6){std::error_code error;std::filesystem::copy_file(filePath(configPath),filePath(configPath+".v"+std::to_string(version)+".bak"),std::filesystem::copy_options::skip_existing,error);}}
    Configuration previous;
    if(parse(configPath,previous)){std::error_code error;std::filesystem::copy_file(filePath(configPath),filePath(configPath+".bak"),std::filesystem::copy_options::overwrite_existing,error);}
    bool result=replaceFile(temporary,configPath);
    if(result)unsaved=false;
    return result;
}
bool dirty(){std::lock_guard lock(stateMutex);return unsaved;}
void beginEdit(){std::lock_guard lock(stateMutex);if(editTransactionDepth++==0){editTransactionRecorded=false;previousEdit=-999;}}
void endEdit(){std::lock_guard lock(stateMutex);if(editTransactionDepth>0&&--editTransactionDepth==0){editTransactionRecorded=false;previousEdit=-999;}}
std::string configurationDirectory(){std::lock_guard lock(stateMutex);return configPath.empty()?"":pathText(filePath(configPath).parent_path());}
bool canSave(){std::lock_guard lock(stateMutex);return !configPath.empty();}
bool canUndo(){std::lock_guard lock(stateMutex);return !undoStack.empty();}
bool canRedo(){std::lock_guard lock(stateMutex);return !redoStack.empty();}
uint64_t revision(){std::lock_guard lock(stateMutex);return editRevision;}
bool undo(){std::lock_guard lock(stateMutex);if(undoStack.empty())return false;redoStack.push_back(config);config=undoStack.back();undoStack.pop_back();previousEdit=-999;unsaved=true;++editRevision;return true;}
bool redo(){std::lock_guard lock(stateMutex);if(redoStack.empty())return false;undoStack.push_back(config);config=redoStack.back();redoStack.pop_back();previousEdit=-999;unsaved=true;++editRevision;return true;}
std::vector<Mode> modes(){std::lock_guard lock(stateMutex);return config.modes;}
int addMode(const std::string& name){std::lock_guard lock(stateMutex);if(config.modes.size()>=16)return -1;recordEdit(-10);config.modes.push_back({name,.5,.75});return int(config.modes.size()-1);}
void editMode(int index,const std::string& name,double width,double height){std::lock_guard lock(stateMutex);if(index<5||index>=int(config.modes.size())||!std::isfinite(width)||!std::isfinite(height))return;recordEdit(-11);config.modes[size_t(index)]={name,std::clamp(width,0.0,16384.0),std::clamp(height,0.0,16384.0)};}
void removeMode(int index){std::lock_guard lock(stateMutex);if(index<5||index>=int(config.modes.size()))return;recordEdit(-12);int last=int(config.modes.size())-1;config.modes.erase(config.modes.begin()+index);for(int base:{64,80,96}){for(int i=index;i<last;++i)config.settings[base+i]=config.settings[base+i+1];config.settings[base+last]=0;}for(int i=index;i<last;++i)config.bindings[i]=config.bindings[i+1];config.bindings[last].clear();auto shift=[index](SourceValues& values){int mask=int(values[14]);values[14]=(mask&((1<<index)-1))|((mask>>(index+1))<<index);};for(auto& mirror:config.mirrors)shift(mirror.values);for(auto& image:config.images)shift(image.values);int active=int(config.settings[0]);config.settings[0]=active==index?0:active>index?active-1:active;}
std::vector<Mirror> mirrors(){std::lock_guard lock(stateMutex);return config.mirrors;}
int selectedMirror(){std::lock_guard lock(stateMutex);return config.selectedMirror;}
void selectMirror(int index){std::lock_guard lock(stateMutex);if(index>=0&&index<int(config.mirrors.size()))config.selectedMirror=index;}
int addMirror(const std::string& name,int preset){std::lock_guard lock(stateMutex);if(config.mirrors.size()>=32)return -1;recordEdit(-20);config.mirrors.push_back(mirrorPreset(name,preset));config.selectedMirror=int(config.mirrors.size()-1);return config.selectedMirror;}
void editMirror(int index,int field,double value){std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size())||field<0||field>=40||!std::isfinite(value))return;value=sanitizeMirror(field,value);if(config.mirrors[size_t(index)].values[field]!=value||((field==7||field==8)&&config.mirrors[size_t(index)].values[31]!=0)){recordEdit(1000+index*40+field);config.mirrors[size_t(index)].values[field]=value;if(field==7)config.mirrors[size_t(index)].style[AutoWidth]=0;if(field==8&&config.mirrors[size_t(index)].values[33]>=2)config.mirrors[size_t(index)].values[37]=0;if(field==7||field==8)config.mirrors[size_t(index)].values[31]=0;}}
void setMirrorObsOverride(int index,bool enabled){std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size()))return;auto& mirror=config.mirrors[size_t(index)];if(mirror.separateObs==enabled)return;recordEdit(-60);if(enabled&&!mirror.obsInitialized){mirror.obsValues=mirror.values;mirror.obsStyle=mirror.style;mirror.obsInitialized=true;}mirror.separateObs=enabled;}
void editMirrorOutput(int index,int route,int field,double value){if(route!=1){editMirror(index,field,value);return;}std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size())||field<0||field>=40||!std::isfinite(value))return;auto& mirror=config.mirrors[size_t(index)];value=sanitizeMirror(field,value);if(!mirror.separateObs){recordEdit(-61);mirror.separateObs=true;if(!mirror.obsInitialized){mirror.obsValues=mirror.values;mirror.obsStyle=mirror.style;mirror.obsInitialized=true;}}if(mirror.obsValues[field]!=value){recordEdit(10000+index*40+field);mirror.obsValues[field]=value;if(field==7)mirror.obsStyle[AutoWidth]=0;if(field==8&&mirror.values[33]>=2)mirror.obsValues[37]=0;}}
void editOverlayStyle(int index,int route,int field,double value){std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size())||field<0||field>=OverlayPropertyCount||!std::isfinite(value))return;auto& mirror=config.mirrors[size_t(index)];if(route==1&&!mirror.separateObs){recordEdit(-62);mirror.separateObs=true;if(!mirror.obsInitialized){mirror.obsValues=mirror.values;mirror.obsStyle=mirror.style;mirror.obsInitialized=true;}}auto& style=route==1?mirror.obsStyle:mirror.style;value=sanitizeOverlay(field,value);if(style[field]!=value){recordEdit(20000+route*3000+index*OverlayPropertyCount+field);style[field]=value;}}
void copyMirrorOutput(int index,int sourceRoute,int destinationRoute){std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size())||sourceRoute==destinationRoute)return;recordEdit(-63);auto& mirror=config.mirrors[size_t(index)];auto source=mirrorForOutput(mirror,sourceRoute);if(destinationRoute==1){mirror.obsValues=source.values;mirror.obsStyle=source.style;mirror.separateObs=true;mirror.obsInitialized=true;}else{int routes=int(mirror.values[32]);mirror.values=source.values;mirror.values[32]=routes;mirror.style=source.style;}}
void duplicateMirror(int index){std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size())||config.mirrors.size()>=32)return;recordEdit(-64);auto duplicate=config.mirrors[size_t(index)];duplicate.name+=" copy";config.mirrors.push_back(duplicate);config.selectedMirror=int(config.mirrors.size()-1);}
void renameMirror(int index,const std::string& name){std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size()))return;recordEdit(-21);config.mirrors[size_t(index)].name=name;}
void removeMirror(int index){std::lock_guard lock(stateMutex);if(index<0||index>=int(config.mirrors.size()))return;recordEdit(-22);config.mirrors.erase(config.mirrors.begin()+index);if(config.mirrors.empty()){auto mirror=mirrorPreset("E Counter",1);mirror.values[0]=0;config.mirrors.push_back(mirror);}config.selectedMirror=std::min(config.selectedMirror,int(config.mirrors.size()-1));}
std::vector<ImageSource> images(){std::lock_guard lock(stateMutex);auto result=config.images;for(auto& image:result){auto found=sourceDimensions.find(image.id);if(found!=sourceDimensions.end()&&found->second.path==image.path){image.width=found->second.width;image.height=found->second.height;}}return result;}
int addImage(const std::string& path){std::lock_guard lock(stateMutex);if(config.images.size()>=32||path.empty())return -1;recordEdit(-30);auto base=mirrorPreset("",4).values;base[1]=base[2]=0;base[3]=base[4]=1;base[5]=.65;base[6]=.1;base[7]=.3;base[8]=.3;int id=config.nextSource++;std::string name=path.starts_with("browser:")?"Browser overlay":path.starts_with("window:")?"Window overlay":pathText(filePath(path).stem());config.images.push_back({id,name,path,base});return id;}
int addBackground(const std::string& path){std::lock_guard lock(stateMutex);if(path.empty())return -1;auto previous=std::find_if(config.images.begin(),config.images.end(),[](auto& image){return image.id==int(config.settings[121]);});if(previous==config.images.end()&&config.images.size()>=32)return -1;recordEdit(-33);if(previous!=config.images.end()){if(previous->path!=path)previous->width=previous->height=0;previous->path=path;config.settings[124]=2;return previous->id;}auto values=mirrorPreset("",4).values;values[1]=values[2]=values[5]=values[6]=0;values[3]=values[4]=values[7]=values[8]=1;int id=config.nextSource++;config.images.push_back({id,"Canvas background",path,values});config.settings[121]=id;config.settings[124]=2;return id;}
void imageSize(int id,int width,int height){std::lock_guard lock(stateMutex);for(auto& image:config.images)if(image.id==id){image.width=width;image.height=height;sourceDimensions[id]={image.path,width,height};}}
void editImage(int id,int field,double value){std::lock_guard lock(stateMutex);if(field<0||field>=40||!std::isfinite(value))return;for(auto& image:config.images)if(image.id==id){value=sanitizeMirror(field,value);if(image.values[field]!=value||((field==7||field==8)&&image.values[31]!=0)){recordEdit(3000+id*40+field);image.values[field]=value;if(field==7||field==8)image.values[31]=0;}}}
void removeImage(int id){std::lock_guard lock(stateMutex);recordEdit(-31);config.images.erase(std::remove_if(config.images.begin(),config.images.end(),[id](auto& image){return image.id==id;}),config.images.end());if(int(config.settings[121])==id){config.settings[121]=0;config.settings[124]=0;}}
void renameImage(int id,const std::string& name){std::lock_guard lock(stateMutex);for(auto& image:config.images)if(image.id==id){recordEdit(-32);image.name=name;}}

int keyEvent(int keycode,int modifiers,int action,bool panelOpen){
    if(action==1&&(keycode==1073741889||(keycode=='i'&&(modifiers&192))))return 1;
    std::lock_guard lock(stateMutex);
    if(action==0&&keycode==heldModeKey){config.settings[0]=modeBeforeHold;heldModeKey=0;unsaved=true;return 2;}
    if(panelOpen)return 0;
    if(action!=1)return 0;
    int mask=((modifiers&3)?1:0)|((modifiers&192)?2:0)|((modifiers&768)?4:0)|((modifiers&3072)?8:0);
    for(int i=0;i<int(config.modes.size());++i){
        if(int(config.settings[64+i])!=keycode||int(config.settings[80+i])!=mask)continue;
        int current=int(config.settings[0]);int target=config.settings[96+i]>.5?i:i==current?0:i;if(!allowResolution(config,target))return 0;recordEdit(-40);
        if(config.settings[96+i]>.5){heldModeKey=keycode;modeBeforeHold=current;config.settings[0]=i;}
        else config.settings[0]=i==current?0:i;
        return 2;
    }
    return 0;
}
std::array<double,2> sensitivity(){
    std::lock_guard lock(stateMutex);const auto& s=config.settings;
    if(s[128]<.5||(int(s[0])==3&&s[145]>.5))return {1,1};
    double requested=s[38]>.5?s[39]:1;
    double base=std::pow(vanillaSensitivity*.6+.2,3);
    double value=std::clamp(base*requested,std::pow(.2,3),std::pow(.8,3))/base;
    return {value,value};
}
std::vector<KeyRemap> remaps(){std::lock_guard lock(stateMutex);return config.remaps;}
void bindRemap(KeyRemap mapping){
    if(mapping.sourceScan<1||mapping.sourceScan>511||mapping.targetScan<1||mapping.targetScan>511)return;
    std::lock_guard lock(stateMutex);recordEdit(-50);
    config.remaps.erase(std::remove_if(config.remaps.begin(),config.remaps.end(),[&](const auto& current){return current.sourceScan==mapping.sourceScan||current.targetScan==mapping.sourceScan||current.sourceScan==mapping.targetScan||current.targetScan==mapping.targetScan;}),config.remaps.end());
    if(mapping.sourceScan!=mapping.targetScan&&config.remaps.size()<=126){config.remaps.push_back(mapping);config.remaps.push_back({mapping.targetScan,mapping.targetKey,mapping.sourceScan,mapping.sourceKey});}
}
void clearRemap(int scan){std::lock_guard lock(stateMutex);recordEdit(-51);config.remaps.erase(std::remove_if(config.remaps.begin(),config.remaps.end(),[&](const auto& mapping){return mapping.sourceScan==scan||mapping.targetScan==scan;}),config.remaps.end());}
std::array<int,2> remapKey(int scan,int key){std::lock_guard lock(stateMutex);if(config.settings[112]>.5)for(auto& mapping:config.remaps)if(mapping.sourceScan==scan)return {mapping.targetScan,mapping.targetKey};return {scan,key};}
void releaseHeldInput(){releasePhysicalInput();std::lock_guard lock(stateMutex);if(heldModeKey){config.settings[0]=modeBeforeHold;heldModeKey=0;}}

std::array<int,2> viewport(int width,int height,int limit,bool suspended){
    if(width<1||height<1||limit<1)return {1,1};
    std::lock_guard lock(stateMutex);displayWidth=width;displayHeight=height;textureLimit=std::min(limit,16384);
    int mode=suspended?0:int(config.settings[0]);if(mode==0)return {width,height};
    if(!allowResolution(config,mode)&&oversizedResolution)return *oversizedResolution;
    return modeViewport(config,mode,width,height,textureLimit);
}

std::vector<Quad> plan(int width,int height,int viewWidth,int viewHeight,bool suspended,double seconds){
    return planForOutput(width,height,viewWidth,viewHeight,suspended,seconds,0);
}
std::vector<int> binding(int action){std::lock_guard lock(stateMutex);return action>=0&&action<17?config.bindings[action]:std::vector<int>{};}
void bindKeys(int action,const std::vector<int>& keys){if(action<0||action>=17||keys.size()>8||std::any_of(keys.begin(),keys.end(),[](int key){return key<1||key>511;}))return;std::lock_guard lock(stateMutex);recordEdit(-60);config.bindings[action]=keys;std::sort(config.bindings[action].begin(),config.bindings[action].end());config.bindings[action].erase(std::unique(config.bindings[action].begin(),config.bindings[action].end()),config.bindings[action].end());for(int i=0;i<17;++i)if(i!=action&&config.bindings[i]==config.bindings[action]&&!keys.empty())config.bindings[i].clear();}
std::string option(const std::string& name){std::lock_guard lock(stateMutex);auto found=config.options.find(name);return found==config.options.end()?"":found->second;}
void setOption(const std::string& name,const std::string& value){if(name.size()>128||value.size()>4096)return;std::lock_guard lock(stateMutex);if(config.options[name]!=value){recordEdit(-61);config.options[name]=value;}}
std::vector<Quad> planForOutput(int width,int height,int viewWidth,int viewHeight,bool suspended,double seconds,int route){
    if(width<=0||height<=0||viewWidth<=0||viewHeight<=0||!std::isfinite(seconds))return {};
    auto s=snapshot();int mode=int(s[0]);
    int offsetX=width/2-viewWidth/2,offsetY=height/2-viewHeight/2;
    double left=std::max(0,offsetX),top=std::max(0,offsetY),visibleWidth=std::min(width,offsetX+viewWidth)-left,visibleHeight=std::min(height,offsetY+viewHeight)-top;
    double sx=(left-offsetX)/viewWidth,sy=(top-offsetY)/viewHeight;
    double dx=left/width,dy=top/height;
    int routeMask=1<<std::clamp(route,0,1);
    int border=512|(!suspended&&mode!=0&&s[114]>.5&&(int(s[153])&routeMask)?128:0);
    std::vector<Quad> result{quad(sx,sy,visibleWidth/viewWidth,visibleHeight/viewHeight,dx,dy,visibleWidth/width,visibleHeight/height,border,s[115],seconds,1,visibleWidth,visibleHeight,0,0,s[116],s[117],s[118])};
    if(suspended)return result;
    auto imageList=images();
    if(mode!=0&&s[124]>0&&(int(s[133])&routeMask)){
        std::array<std::array<double,4>,4> regions{{{0,0,1,dy},{0,dy+visibleHeight/height,1,1-dy-visibleHeight/height},{0,dy,dx,visibleHeight/height},{dx+visibleWidth/width,dy,1-dx-visibleWidth/width,visibleHeight/height}}};
        auto background=std::find_if(imageList.begin(),imageList.end(),[&](auto& image){return image.id==int(s[121]);});
        for(auto rect:regions){
            if(rect[2]<=0||rect[3]<=0)continue;
            if(int(s[124])==1){auto layer=quad(0,0,1,1,rect[0],rect[1],rect[2],rect[3],32,0,seconds,1,0,0,0,0,s[8],s[9],s[10]);layer[0]=float(s[125]);layer[1]=float(s[126]);layer[2]=float(s[127]);result.push_back(layer);}
            else if(background!=imageList.end()&&background->values[0]>.5&&(int(background->values[32])&routeMask)&&(int(background->values[14])&(1<<mode))&&background->width>0&&background->height>0){
                double ux=0,uy=0,uw=1,uh=1;double imageAspect=double(background->width)/background->height,screenAspect=double(width)/height;
                if(int(s[122])==0){if(imageAspect>screenAspect){uw=screenAspect/imageAspect;ux=(1-uw)/2;}else{uh=imageAspect/screenAspect;uy=(1-uh)/2;}}
                if(int(s[122])==1){double fit=std::min(double(width)/background->width,double(height)/background->height);double iw=background->width*fit/width,ih=background->height*fit/height,ix=(1-iw)/2,iy=(1-ih)/2;double x=std::max(rect[0],ix),y=std::max(rect[1],iy),right=std::min(rect[0]+rect[2],ix+iw),bottom=std::min(rect[1]+rect[3],iy+ih);if(right<=x||bottom<=y)continue;result.push_back(quad((x-ix)/iw,(y-iy)/ih,(right-x)/iw,(bottom-y)/ih,x,y,right-x,bottom-y,16,0,seconds,s[123],0,0,0,background->id));}
                else result.push_back(quad(ux+rect[0]*uw,uy+rect[1]*uh,rect[2]*uw,rect[3]*uh,rect[0],rect[1],rect[2],rect[3],16,0,seconds,s[123],0,0,0,background->id));
            }
        }
    }
    if(mode==3&&s[11]>.5&&(int(s[132])&routeMask)){
        double cloneWidth=std::clamp(s[12],1.0,double(viewWidth)),cloneHeight=std::clamp(s[13],1.0,double(viewHeight));
        double outputX=std::round(s[14]*width),outputY=std::round(s[15]*height),outputWidth=std::max(std::round(s[16]*width),1.0),outputHeight=std::max(std::round(s[17]*height),1.0);
        if(s[35]>.5){double freeLeft=std::max(0,width/2-viewWidth/2);double margin=std::floor(freeLeft/10);outputX=margin;outputWidth=std::max(1.0,freeLeft-2*margin);outputY=std::floor(height/8.0);outputHeight=height-2*outputY;}
        outputWidth=std::min(outputWidth,double(width));outputHeight=std::min(outputHeight,double(height));outputX=std::clamp(outputX,0.0,width-outputWidth);outputY=std::clamp(outputY,0.0,height-outputHeight);
        if(outputWidth>20)result.push_back(quad(double(viewWidth/2-int(cloneWidth)/2)/viewWidth,double(viewHeight/2-int(cloneHeight)/2)/viewHeight,cloneWidth/viewWidth,cloneHeight/viewHeight,outputX/width,outputY/height,outputWidth/width,outputHeight/height,576,0,seconds,1,outputWidth,outputHeight,cloneWidth));
    }
    for(auto& mirror:mirrors())if(int(mirror.values[32])&routeMask)appendSource(result,mirror.values,0,mode,width,height,viewWidth,viewHeight,seconds);
    for(auto& image:imageList)if(image.id!=int(s[121])&&(int(image.values[32])&routeMask))appendSource(result,image.values,image.id,mode,width,height,viewWidth,viewHeight,seconds);
    return result;
}
}

extern "C" {
void ts_runtime_context(int inWorld,double baseSensitivity){toolscreen::runtimeContext(inWorld!=0,baseSensitivity);}
void ts_sensitivity(double* output){if(output){auto values=toolscreen::sensitivity();std::copy(values.begin(),values.end(),output);}}
int ts_plan_output(int width,int height,int viewWidth,int viewHeight,int suspended,double seconds,int route,float* output,int capacity){auto result=toolscreen::planForOutput(width,height,viewWidth,viewHeight,suspended!=0,seconds,route);int required=int(result.size())*20;if(!output||capacity<required)return -required;for(size_t i=0;i<result.size();++i)std::copy(result[i].begin(),result[i].end(),output+i*20);return required;}
int ts_mirror_name(int index,char* output,int capacity){auto mirrors=toolscreen::mirrors();if(index<0||index>=int(mirrors.size()))return 0;auto& name=mirrors[size_t(index)].name;if(!output||capacity<=int(name.size()))return -int(name.size()+1);std::memcpy(output,name.c_str(),name.size()+1);return int(name.size());}
int ts_set(int index,double value){return toolscreen::set(index,value);}
double ts_get(int index){return index>=0&&index<toolscreen::settingCount?toolscreen::snapshot()[index]:0;}
void ts_reset(){toolscreen::reset();}
int ts_load(const char* path){return path&&toolscreen::load(path);}
int ts_save(){return toolscreen::save();}
void ts_viewport(int width,int height,int limit,int suspended,int* output){if(!output)return;auto result=toolscreen::viewport(width,height,limit,suspended!=0);std::copy(result.begin(),result.end(),output);}
int ts_plan(int width,int height,int viewWidth,int viewHeight,int suspended,double seconds,float* output,int capacity){auto result=toolscreen::plan(width,height,viewWidth,viewHeight,suspended!=0,seconds);int required=int(result.size())*20;if(!output||capacity<required)return -required;for(size_t i=0;i<result.size();++i)std::copy(result[i].begin(),result[i].end(),output+i*20);return required;}
int ts_add_mirror(const char* name,int preset){return toolscreen::addMirror(name?name:"Mirror",preset);}
void ts_select_mirror(int index){toolscreen::selectMirror(index);}
void ts_edit_mirror(int index,int field,double value){toolscreen::editMirror(index,field,value);}
void ts_mirror_obs_override(int index,int enabled){toolscreen::setMirrorObsOverride(index,enabled!=0);}
void ts_edit_mirror_output(int index,int route,int field,double value){toolscreen::editMirrorOutput(index,route,field,value);}
void ts_edit_overlay_style(int index,int route,int field,double value){toolscreen::editOverlayStyle(index,route,field,value);}
void ts_copy_mirror_output(int index,int sourceRoute,int destinationRoute){toolscreen::copyMirrorOutput(index,sourceRoute,destinationRoute);}
int ts_mirror_output(int index,int route,double* output){auto mirrors=toolscreen::mirrors();if(index<0||index>=int(mirrors.size())||!output)return 0;auto mirror=toolscreen::mirrorForOutput(mirrors[size_t(index)],route);std::copy(mirror.values.begin(),mirror.values.end(),output);std::copy(mirror.style.begin(),mirror.style.end(),output+40);return 40+toolscreen::OverlayPropertyCount;}

int ts_add_mode(const char* name){return toolscreen::addMode(name?name:"Custom");}
int ts_key_event(int keycode,int modifiers,int action,int panelOpen){return toolscreen::keyEvent(keycode,modifiers,action,panelOpen!=0);}
int ts_add_image(const char* path){return path?toolscreen::addImage(path):-1;}
int ts_add_background(const char* path){return path?toolscreen::addBackground(path):-1;}
void ts_image_size(int id,int width,int height){toolscreen::imageSize(id,width,height);}
void ts_edit_image(int id,int field,double value){toolscreen::editImage(id,field,value);}
int ts_undo(){return toolscreen::undo();}
int ts_redo(){return toolscreen::redo();}
void ts_bind_remap(int sourceScan,int sourceKey,int targetScan,int targetKey){toolscreen::bindRemap({sourceScan,sourceKey,targetScan,targetKey});}
void ts_remap_key(int scan,int key,int* output){if(!output)return;auto value=toolscreen::remapKey(scan,key);output[0]=value[0];output[1]=value[1];}
}
