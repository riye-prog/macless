#include "input.hpp"
#include "core.hpp"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <set>

namespace {
std::mutex inputMutex;
std::set<int> pressed;
std::set<int> consumed;
std::vector<int> heldChord;
std::atomic<int> restoreMode{-1};
}

namespace toolscreen {
int scanFromCode(int code){
    if(code>='a'&&code<='z')return code-'a'+4;
    if(code>='1'&&code<='9')return code-'1'+30;
    if(code=='0')return 39;
    if(code&1073741824)return code&~1073741824;
    switch(code){case 13:return 40;case 27:return 41;case 8:return 42;case 9:return 43;case 32:return 44;case '-':return 45;case '=':return 46;case '[':return 47;case ']':return 48;case '\\':return 49;case ';':return 51;case '\'':return 52;case '`':return 53;case ',':return 54;case '.':return 55;case '/':return 56;case 127:return 76;default:return 0;}
}
std::string scanName(int scan){
#if defined(_WIN32)
    if(scan==227)return "Left Win";if(scan==231)return "Right Win";
#elif defined(__linux__)
    if(scan==227)return "Left Super";if(scan==231)return "Right Super";
#endif
    if(scan>=4&&scan<=29)return std::string(1,char('A'+scan-4));
    if(scan>=30&&scan<=38)return std::string(1,char('1'+scan-30));
    if(scan==39)return "0";
    if(scan>=58&&scan<=69)return "F"+std::to_string(scan-57);
    if(scan>=104&&scan<=115)return "F"+std::to_string(scan-91);
    const std::pair<int,const char*> names[]={{40,"Enter"},{41,"Escape"},{42,"Backspace"},{43,"Tab"},{44,"Space"},{45,"-"},{46,"="},{47,"["},{48,"]"},{49,"\\"},{50,"ISO #"},{51,";"},{52,"'"},{53,"`"},{54,","},{55,"."},{56,"/"},{57,"Caps Lock"},{70,"Print Screen"},{71,"Scroll Lock"},{72,"Pause"},{73,"Insert"},{74,"Home"},{75,"Page Up"},{76,"Delete"},{77,"End"},{78,"Page Down"},{79,"Right"},{80,"Left"},{81,"Down"},{82,"Up"},{83,"Num Lock"},{84,"Keypad /"},{85,"Keypad *"},{86,"Keypad -"},{87,"Keypad +"},{88,"Keypad Enter"},{98,"Keypad 0"},{99,"Keypad ."},{100,"ISO \\"},{101,"Application"},{102,"Power"},{103,"Keypad ="},{117,"Help"},{118,"Menu"},{127,"Mute"},{128,"Volume Up"},{129,"Volume Down"},{224,"Left Ctrl"},{225,"Left Shift"},{226,"Left Alt"},{227,"Left Cmd"},{228,"Right Ctrl"},{229,"Right Shift"},{230,"Right Alt"},{231,"Right Cmd"}};
    if(scan>=89&&scan<=97)return "Keypad "+std::to_string(scan-88);
    for(auto& name:names)if(name.first==scan)return name.second;
    return "Key "+std::to_string(scan);
}
std::vector<int> modifierScans(int modifiers){std::vector<int> result;const int masks[]={64,1,256,1024,128,2,512,2048};for(int i=0;i<8;++i)if(modifiers&masks[i])result.push_back(224+i);return result;}
int inputPersistentMode(int current){int mode=restoreMode.load();return mode>=0?mode:current;}
void releasePhysicalInput(bool restore){std::lock_guard lock(inputMutex);int mode=restoreMode.exchange(-1);if(restore&&mode>=0)set(0,mode);pressed.clear();consumed.clear();heldChord.clear();}
int inputEvent(int scan,int modifiers,int action,bool gameplay,bool editorAllowed){
    if(scan<=0||scan>511)return 0;
    auto state=snapshot();std::vector<std::vector<int>> binds;for(int i=0;i<17;++i)binds.push_back(binding(i));
    std::lock_guard lock(inputMutex);
    for(int key=224;key<=231;++key)pressed.erase(key);
    for(int key:modifierScans(modifiers))pressed.insert(key);
    if(action==0)pressed.erase(scan);else pressed.insert(scan);
    bool wasConsumed=action==0&&consumed.erase(scan)>0;int result=wasConsumed?3:0;
    if(restoreMode.load()>=0&&!std::all_of(heldChord.begin(),heldChord.end(),[](int key){return pressed.contains(key);})){set(0,restoreMode.exchange(-1));heldChord.clear();result=wasConsumed?2:4;}
    if(action==2)return consumed.contains(scan)?3:result;
    if(action!=1)return result;
    auto matches=[&](const std::vector<int>& keys){if(keys.empty()||std::find(keys.begin(),keys.end(),scan)==keys.end())return false;for(int key:pressed)if(((key>=224&&key<=231)||key==60)&&std::find(keys.begin(),keys.end(),key)==keys.end())return false;return std::all_of(keys.begin(),keys.end(),[](int key){return pressed.contains(key);});};
    bool f8Assigned=false;for(int i=0;i<16;++i)if(binds[i]==std::vector<int>{65})f8Assigned=true;
    if(editorAllowed&&(matches(binds[16])||(scan==65&&!f8Assigned&&modifierScans(modifiers).empty()&&!pressed.contains(60)))){consumed.insert(scan);return 1;}
    if(!gameplay||state[128]<.5){pressed.erase(scan);return result;}
    int selected=-1;for(int i=0;i<int(modes().size());++i)if(matches(binds[i])&&(selected<0||binds[i].size()>binds[selected].size()))selected=i;
    if(selected<0)return result;
    int current=int(state[0]);if(state[96+selected]>.5){if(restoreMode.load()<0)restoreMode=current;heldChord=binds[selected];set(0,selected);}else{restoreMode=-1;heldChord.clear();set(0,current==selected?0:selected);}consumed.insert(scan);return 2;
}
}

extern "C" {
int ts_input_event(int scan,int modifiers,int action,int gameplay,int editorAllowed){return toolscreen::inputEvent(scan,modifiers,action,gameplay!=0,editorAllowed!=0);}
void ts_bind_keys(int action,const int* keys,int count){if(count<0||count>8||(!keys&&count))return;if(count==0)toolscreen::bindKeys(action,{});else toolscreen::bindKeys(action,std::vector<int>(keys,keys+count));}
}
