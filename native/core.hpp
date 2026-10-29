#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace toolscreen {
constexpr int settingCount = 256;
using Settings = std::array<double, settingCount>;
using Quad = std::array<float, 20>;
using SourceValues = std::array<double, 40>;
enum OverlayProperty { TextRed,TextGreen,TextBlue,MutedRed,MutedGreen,MutedBlue,BackgroundRed,BackgroundGreen,BackgroundBlue,ShowTitle,ShowDetails,ShowPath,ShowLocal,ShowGlobal,ShowBars,ShowIndices,ShowFooter,Decimals,Padding,CornerRadius,RowHeight,FontFace,TextAlignment,AutoWidth,TextOutline,ValueScale,TitleScale,OverlayPropertyCount };
using OverlayStyle=std::array<double,OverlayPropertyCount>;
OverlayStyle defaultOverlayStyle(int type=2);
struct Mirror {
    std::string name;
    SourceValues values;
    OverlayStyle style=defaultOverlayStyle();
    bool separateObs=false;
    bool obsInitialized=false;
    SourceValues obsValues{};
    OverlayStyle obsStyle=defaultOverlayStyle();
};
struct Mode {
    std::string name;
    double width;
    double height;
};
struct ImageSource {
    int id;
    std::string name;
    std::string path;
    SourceValues values;
    int width=0;
    int height=0;
};
struct KeyRemap{int sourceScan,sourceKey,targetScan,targetKey;};
Settings snapshot();
void runtimeContext(bool inWorld,double baseSensitivity);
std::string resolutionError();
bool set(int index, double value);
void reset();
bool load(const std::string& path);
bool save();
bool dirty();
bool canSave();
std::string configurationDirectory();
void beginEdit();
void endEdit();
bool canUndo();
bool canRedo();
uint64_t revision();
bool undo();
bool redo();
std::vector<Mode> modes();
int addMode(const std::string& name);
void editMode(int index, const std::string& name, double width, double height);
void removeMode(int index);
std::vector<Mirror> mirrors();
int selectedMirror();
void selectMirror(int index);
int addMirror(const std::string& name, int preset);
void editMirror(int index, int field, double value);
void renameMirror(int index, const std::string& name);
Mirror mirrorForOutput(const Mirror& mirror,int route);
void setMirrorObsOverride(int index,bool enabled);
void editMirrorOutput(int index,int route,int field,double value);
void editOverlayStyle(int index,int route,int field,double value);
void copyMirrorOutput(int index,int sourceRoute,int destinationRoute);
void duplicateMirror(int index);
void removeMirror(int index);
std::vector<ImageSource> images();
int addImage(const std::string& path);
int addBackground(const std::string& path);
void imageSize(int id,int width,int height);
void editImage(int id, int field, double value);
void removeImage(int id);
void renameImage(int id,const std::string& name);
int keyEvent(int keycode, int modifiers, int action, bool panelOpen);
std::array<double, 2> sensitivity();
std::vector<KeyRemap> remaps();
void bindRemap(KeyRemap mapping);
void clearRemap(int scan);
std::array<int,2> remapKey(int scan,int key);
void releaseHeldInput();
std::array<int, 2> viewport(int width, int height, int limit, bool suspended);
std::vector<Quad> plan(int width, int height, int viewWidth, int viewHeight, bool suspended, double seconds);
std::vector<Quad> planForOutput(int width,int height,int viewWidth,int viewHeight,bool suspended,double seconds,int route);
std::vector<int> binding(int action);
void bindKeys(int action,const std::vector<int>& keys);
std::string option(const std::string& name);
void setOption(const std::string& name,const std::string& value);
}

extern "C" {
int ts_set(int index, double value);
void ts_runtime_context(int inWorld,double baseSensitivity);
double ts_get(int index);
void ts_reset();
int ts_load(const char* path);
int ts_save();
void ts_viewport(int width, int height, int limit, int suspended, int* output);
int ts_plan(int width, int height, int viewWidth, int viewHeight, int suspended, double seconds, float* output, int capacity);
int ts_add_mirror(const char* name, int preset);
void ts_select_mirror(int index);
void ts_edit_mirror(int index, int field, double value);
int ts_add_mode(const char* name);
int ts_key_event(int keycode, int modifiers, int action, int panelOpen);
int ts_add_image(const char* path);
void ts_edit_image(int id,int field,double value);
int ts_undo();
int ts_redo();
void ts_bind_remap(int sourceScan,int sourceKey,int targetScan,int targetKey);
void ts_remap_key(int scan,int key,int* output);
}
