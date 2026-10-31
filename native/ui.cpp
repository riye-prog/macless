#include "ui.hpp"
#include "core.hpp"
#include "platform.hpp"
#include "input.hpp"
#include "features.hpp"
#include "overlays.hpp"
#include "fonts.hpp"
#include "ui_theme.hpp"
#include "ui_layout.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/freetype/imgui_freetype.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <set>
#include <cctype>

namespace {
std::mutex uiMutex;
ImGuiContext* context=nullptr;
ImGuiContext* overlayContext=nullptr;
ImFont* bodyFont=nullptr;
ImFont* headingFont=nullptr;
ImFont* gridFont=nullptr;
ImFont* dataFont=nullptr;
ImFont* brandFont=nullptr;
ImFont* captionFont=nullptr;
float fontRasterScale=0;
uint64_t fontGeneration=0;
ImGuiStyle baseStyle;
enum class LayoutTarget{Game,Projector,Mirror,Source,Appearance};
int selectedPage=0;
int settingsMode=1;
LayoutTarget layoutTarget=LayoutTarget::Game;
bool settingsRequested=false;
bool saveFailed=false;
uint64_t lastEditRevision=0;
double nextAutosave=0;
double statusStarted=0;
std::string previousStatus;
ImVec2 lastDisplay{};
float lastWindowScale=1;
bool pendingWindowMove=false;
bool pendingWindowResize=false;
ImVec2 pendingWindowSize{};
ImVec2 pendingWindowPosition{};
toolscreen::ui::WindowLayout windowLayout;
constexpr int previewRoute=0;
bool commandPaletteRequested=false;
int selectedImage=-1;
int bindingMode=-1;
int bindingRemapScan=-1;
int bindingRemapKey=0;
std::set<int> bindingCollected;
std::set<int> bindingPressed;
bool closeRequested=false;
bool pixelUnits=true;
bool canvasEditable=true;
bool canvasNavigationFocus=false;
bool selectNextImage=false;
int previousSourceCount=-1;
std::string status;
std::unordered_map<int,ImVec4> controlBounds;
struct AssetStatus{int width=0,height=0;std::string error;};
std::unordered_map<int,AssetStatus> assetStatuses;
int displayWidth=1920;
int displayHeight=1080;
float scale=1;
float displayScale=1;
ImVec4 accent(.86f,.89f,.92f,1);
ImVec4 copper(.91f,.702f,.545f,1);
ImVec4 muted=toolscreen::ui::rgba(0xa6abb3);
ImVec4 bright=toolscreen::ui::rgba(0xf1f2f4);
struct Bounds{float x,y,w,h;};
struct DragState{bool active=false;int resizeHandle=0;int lockedAxis=0;double storedHeight=-1,storedWidth=-1;bool committed=false;int kind=0;int index=0;ImVec2 mouse{};std::array<double,4> values{};}drag;

void finishDrag(){if(drag.committed)toolscreen::endEdit();drag.active=false;drag.committed=false;}

ImU32 color(ImVec4 value){return ImGui::ColorConvertFloat4ToU32(value);}
ImVec2 endOf(const Bounds& value){return ImVec2(value.x+value.w,value.y+value.h);}
bool contains(const Bounds& value,ImVec2 point){return point.x>=value.x&&point.x<=value.x+value.w&&point.y>=value.y&&point.y<=value.y+value.h;}
void nextColumn(){ImGui::TableNextColumn();ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset=0;}
void rememberBounds(int id){auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();controlBounds[id]=ImVec4(a.x,a.y,b.x,b.y);}

void rebuildFonts(float rasterScale){
    auto& io=ImGui::GetIO();io.Fonts->Clear();
    ImFontConfig fontConfig;fontConfig.OversampleH=1;fontConfig.OversampleV=1;fontConfig.RasterizerDensity=2;io.Fonts->TexGlyphPadding=3;fontConfig.FontBuilderFlags=ImGuiFreeTypeBuilderFlags_LightHinting;
    static const ImWchar glyphs[]={0x20,0xFF,0x2000,0x206F,0x2190,0x21FF,0};
    fontConfig.FontDataOwnedByAtlas=false;fontConfig.FontNo=0;
    bodyFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::interRegular),sizeof(toolscreen::assets::interRegular),16,&fontConfig,glyphs);
    headingFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::interSemiBold),sizeof(toolscreen::assets::interSemiBold),20,&fontConfig,glyphs);
    brandFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::interSemiBold),sizeof(toolscreen::assets::interSemiBold),21,&fontConfig,glyphs);
    gridFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::interSemiBold),sizeof(toolscreen::assets::interSemiBold),48,&fontConfig,glyphs);
    fontConfig.FontDataOwnedByAtlas=true;
    const char* mono="/System/Library/Fonts/SFNSMono.ttf";fontConfig.RasterizerDensity=2;static const ImWchar numberGlyphs[]={0x20,0x7e,0};if(std::filesystem::exists(mono)){dataFont=io.Fonts->AddFontFromFileTTF(mono,16,&fontConfig,glyphs);io.Fonts->AddFontFromFileTTF(mono,48,&fontConfig,numberGlyphs);}else{fontConfig.FontDataOwnedByAtlas=false;dataFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::jetBrainsMono),sizeof(toolscreen::assets::jetBrainsMono),16,&fontConfig,glyphs);io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::jetBrainsMono),sizeof(toolscreen::assets::jetBrainsMono),48,&fontConfig,numberGlyphs);}
    fontConfig.FontDataOwnedByAtlas=false;fontConfig.FontBuilderFlags=ImGuiFreeTypeBuilderFlags_Monochrome|ImGuiFreeTypeBuilderFlags_MonoHinting;fontConfig.PixelSnapH=true;
    io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::monocraft),sizeof(toolscreen::assets::monocraft),16,&fontConfig,glyphs);
    io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::monocraft),sizeof(toolscreen::assets::monocraft),48,&fontConfig,numberGlyphs);
    fontConfig.FontDataOwnedByAtlas=false;fontConfig.FontBuilderFlags=ImGuiFreeTypeBuilderFlags_LightHinting;fontConfig.PixelSnapH=true;fontConfig.RasterizerDensity=1;fontConfig.RasterizerMultiply=1.06f;
    auto pixels=[rasterScale](float size){return std::max(8.0f,std::round(size*rasterScale));};
    bodyFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::interRegular),sizeof(toolscreen::assets::interRegular),pixels(16),&fontConfig,glyphs);
    headingFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::interSemiBold),sizeof(toolscreen::assets::interSemiBold),pixels(20),&fontConfig,glyphs);
    brandFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::interSemiBold),sizeof(toolscreen::assets::interSemiBold),pixels(21),&fontConfig,glyphs);
    dataFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::jetBrainsMono),sizeof(toolscreen::assets::jetBrainsMono),pixels(16),&fontConfig,glyphs);
    captionFont=io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(toolscreen::assets::jetBrainsMono),sizeof(toolscreen::assets::jetBrainsMono),pixels(13),&fontConfig,glyphs);
    io.FontDefault=bodyFont;io.Fonts->FontBuilderIO=ImGuiFreeType::GetBuilderForFreeType();io.Fonts->Build();io.Fonts->SetTexID((ImTextureID)1);
    fontRasterScale=rasterScale;++fontGeneration;
}

void ensureContext(){
    if(context){ImGui::SetCurrentContext(context);return;}
    IMGUI_CHECKVERSION();context=ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    io.BackendPlatformName="toolscreen-jni";io.BackendRendererName="toolscreen-renderpearl";
    auto& platform=ImGui::GetPlatformIO();platform.Platform_GetClipboardTextFn=toolscreen::clipboardRead;platform.Platform_SetClipboardTextFn=toolscreen::clipboardWrite;
    rebuildFonts(1);
    ImGuiStyle& style=ImGui::GetStyle();toolscreen::ui::applyTheme(style);
    baseStyle=style;
}

float windowInset(){return std::min(12*displayScale,std::min(float(displayWidth),float(displayHeight))*.04f);}
ImVec2 fitWindowPosition(ImVec2 position,ImVec2 size){float inset=windowInset();return ImVec2(std::clamp(position.x,inset,std::max(inset,displayWidth-inset-size.x)),std::clamp(position.y,inset,std::max(inset,displayHeight-inset-size.y)));}

bool beginPanel(const char* id,ImVec2 size,bool automatic=false){
    ImGui::PushStyleColor(ImGuiCol_ChildBg,toolscreen::ui::rgba(0x202124));ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(18*scale,18*scale));ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,9*scale);
    auto flags=ImGuiChildFlags_AlwaysUseWindowPadding|(automatic?ImGuiChildFlags_AutoResizeY:ImGuiChildFlags_None);
    ImGui::BeginChild(id,size,flags,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(2*scale,2*scale));
    bool visible=ImGui::BeginChild("panel-content",ImVec2(0,0),flags,ImGuiWindowFlags_NoBackground|(automatic?ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse:ImGuiWindowFlags_None));ImGui::PushTextWrapPos(0);return visible;
}
void endPanel(){ImGui::PopTextWrapPos();ImGui::Dummy(ImVec2(0,2*scale));ImGui::EndChild();ImGui::PopStyleVar();ImGui::EndChild();ImGui::PopStyleVar(2);ImGui::PopStyleColor();}

bool beginPopup(const char* id,ImGuiWindowFlags flags=0,float minimumWidth=0,float maximumWidth=FLT_MAX){float inset=windowInset(),availableWidth=std::max(1.0f,displayWidth-2*inset);ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(minimumWidth,availableWidth),0),ImVec2(std::min(maximumWidth,availableWidth),std::max(1.0f,std::string(id)=="Find a control"?displayHeight*.62f:displayHeight-2*inset)));bool visible=ImGui::BeginPopup(id,flags);if(visible)ImGui::PushTextWrapPos(0);return visible;}
void endPopup(){ImGui::PopTextWrapPos();ImGui::EndPopup();}

void windowDragZone(const char* id,float x,float width,ImVec2 position,ImVec2 size){
    if(width<=0)return;ImGui::SetCursorPos(ImVec2(x,8*scale));ImGui::InvisibleButton(id,ImVec2(width,34*scale));
    if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(0)){auto delta=ImGui::GetIO().MouseDelta;pendingWindowPosition=fitWindowPosition(ImVec2(position.x+delta.x,position.y+delta.y),size);pendingWindowMove=true;}
}

void heading(const char* text,const char* detail=nullptr){
    ImGui::PushFont(headingFont);ImGui::TextUnformatted(text);ImGui::PopFont();
    if(detail&&ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s",detail);
}
void section(const char* text){ImGui::Spacing();ImGui::TextColored(muted,"%s",text);}
void tooltip(const char* text){if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s",text);}

void centeredLabel(ImDrawList* draw,const char* text,ImVec2 minimum,ImVec2 maximum){
    auto font=ImGui::GetFont();float fontSize=ImGui::GetFontSize(),factor=fontSize/font->FontSize,pen=0;ImVec2 inkMin(FLT_MAX,FLT_MAX),inkMax(-FLT_MAX,-FLT_MAX);const char* end=ImGui::FindRenderedTextEnd(text);
    for(const char* cursor=text;cursor<end;){unsigned int character;int length=ImTextCharFromUtf8(&character,cursor,end);if(length<=0)break;cursor+=length;const ImFontGlyph* glyph=font->FindGlyph((ImWchar)character);if(!glyph)continue;if(glyph->Visible){inkMin.x=std::min(inkMin.x,pen+glyph->X0*factor);inkMin.y=std::min(inkMin.y,glyph->Y0*factor);inkMax.x=std::max(inkMax.x,pen+glyph->X1*factor);inkMax.y=std::max(inkMax.y,glyph->Y1*factor);}pen+=glyph->AdvanceX*factor;}
    if(inkMin.x==FLT_MAX)return;ImVec2 origin(std::floor((minimum.x+maximum.x-inkMin.x-inkMax.x)*.5f),std::floor((minimum.y+maximum.y-inkMin.y-inkMax.y)*.5f));draw->PushClipRect(minimum,maximum,true);draw->AddText(font,fontSize,origin,ImGui::GetColorU32(ImGuiCol_Text),text,end);draw->PopClipRect();
}

std::string fittedLabel(const char* text,float width){
    std::string label(text,ImGui::FindRenderedTextEnd(text));
    if(ImGui::CalcTextSize(label.c_str()).x<=width)return label;
    const char* ellipsis="…";float endWidth=ImGui::CalcTextSize(ellipsis).x;
    while(!label.empty()&&ImGui::CalcTextSize(label.c_str()).x+endWidth>width){size_t last=label.size()-1;while(last>0&&(static_cast<unsigned char>(label[last])&0xc0)==0x80)--last;label.resize(last);}
    return label+ellipsis;
}

bool button(const char* label,ImVec2 size=ImVec2(0,0)){
    if(ImGui::GetCurrentWindow()->SkipItems)return false;auto padding=ImGui::GetStyle().FramePadding;auto textSize=ImGui::CalcTextSize(label,nullptr,true);size=ImGui::CalcItemSize(size,textSize.x+padding.x*2,textSize.y+padding.y*2);size.y=std::max(size.y,textSize.y+padding.y*2);ImGui::PushID(label);bool pressed=ImGui::Button("##centered",size);ImGui::PopID();auto minimum=ImGui::GetItemRectMin(),maximum=ImGui::GetItemRectMax();minimum.x+=std::min(padding.x,size.x*.15f);maximum.x-=std::min(padding.x,size.x*.15f);auto visible=fittedLabel(label,std::max(0.0f,maximum.x-minimum.x));centeredLabel(ImGui::GetWindowDrawList(),visible.c_str(),minimum,maximum);return pressed;
}

bool smallButton(const char* label){auto padding=ImGui::GetStyle().FramePadding;ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(padding.x,0));bool pressed=button(label);ImGui::PopStyleVar();return pressed;}

bool pill(const char* text,bool active,float width=0,bool prominent=false){
    bool light=active&&prominent;
    ImGui::PushStyleColor(ImGuiCol_Button,active?toolscreen::ui::rgba(light?0xd5d9e0:0x3c424c):toolscreen::ui::rgba(0x282b30));
    ImGui::PushStyleColor(ImGuiCol_Text,light?toolscreen::ui::rgba(0x18191b):active?bright:muted);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,toolscreen::ui::rgba(light?0xf1f2f4:0x484f5a));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,toolscreen::ui::rgba(light?0xbcc4cf:0x555d69));
    bool pressed=button(text,ImVec2(width,32*scale));ImGui::PopStyleColor(4);return pressed;
}

bool disclosure(const char* text,ImGuiTreeNodeFlags flags=0){
    ImGui::PushStyleColor(ImGuiCol_Text,muted);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(8*scale,7*scale));
    bool open=ImGui::CollapsingHeader(text,flags);
    ImGui::PopStyleVar();ImGui::PopStyleColor();
    const char* names[]={"Position and size","Content","Typography","Colors and border","Modes","Manage","Key remapping","Grid options","Source crop","Appearance","Custom modes","Window size","Diagnostics","Measurements"};
    for(int i=0;i<14;++i)if(std::strcmp(text,names[i])==0)rememberBounds(9100+i);
    return open;
}

void icon(ImDrawList* draw,ImVec2 p,int kind,ImU32 tint,float size=18){
    float t=1.6f*scale;float k=size/18;
    if(kind==0){for(int i=0;i<4;++i){float x=p.x+(i&1?size:0),y=p.y+(i&2?size:0);float sx=i&1?-1:1,sy=i&2?-1:1;draw->AddLine(ImVec2(x,y),ImVec2(x+sx*6*k,y),tint,t);draw->AddLine(ImVec2(x,y),ImVec2(x,y+sy*6*k),tint,t);}}
    if(kind==1){draw->AddRect(p,ImVec2(p.x+12*k,p.y+12*k),tint,2,0,t);draw->AddRect(ImVec2(p.x+6*k,p.y+6*k),ImVec2(p.x+18*k,p.y+18*k),tint,2,0,t);}
    if(kind==2){draw->AddRect(p,ImVec2(p.x+size,p.y+size),tint,3,0,t);draw->AddTriangle(ImVec2(p.x+3*k,p.y+14*k),ImVec2(p.x+9*k,p.y+6*k),ImVec2(p.x+15*k,p.y+14*k),tint,t);}
    if(kind==3){draw->AddRect(p,ImVec2(p.x+size,p.y+14*k),tint,3,0,t);for(int y=0;y<2;++y)for(int x=0;x<4;++x)draw->AddCircleFilled(ImVec2(p.x+(3+x*4)*k,p.y+(4+y*5)*k),k,tint);draw->AddLine(ImVec2(p.x+5*k,p.y+18*k),ImVec2(p.x+13*k,p.y+18*k),tint,t);}
    if(kind==4){for(int y=0;y<3;++y){float yy=p.y+(2+y*7)*k;draw->AddLine(ImVec2(p.x,yy),ImVec2(p.x+size,yy),tint,t);draw->AddCircleFilled(ImVec2(p.x+(y==1?12:6)*k,yy),3*k,tint);}}
    if(kind==5){draw->AddCircle(ImVec2(p.x+size/2,p.y+size/2),size/2,tint,24,t);draw->AddLine(ImVec2(p.x+9*k,p.y+8*k),ImVec2(p.x+9*k,p.y+13*k),tint,t);draw->AddCircleFilled(ImVec2(p.x+9*k,p.y+5*k),k,tint);}
    if(kind==6){draw->AddCircle(ImVec2(p.x+9*k,p.y+9*k),7*k,tint,24,t);draw->AddCircleFilled(ImVec2(p.x+9*k,p.y+9*k),2.5f*k,tint);draw->AddLine(ImVec2(p.x+9*k,p.y),ImVec2(p.x+9*k,p.y+3*k),tint,t);draw->AddLine(ImVec2(p.x+9*k,p.y+15*k),ImVec2(p.x+9*k,p.y+18*k),tint,t);}
    if(kind==7){draw->AddRect(p,ImVec2(p.x+18*k,p.y+13*k),tint,2,0,t);draw->AddLine(ImVec2(p.x+9*k,p.y+13*k),ImVec2(p.x+9*k,p.y+17*k),tint,t);draw->AddLine(ImVec2(p.x+5*k,p.y+18*k),ImVec2(p.x+13*k,p.y+18*k),tint,t);draw->AddTriangleFilled(ImVec2(p.x+7*k,p.y+3*k),ImVec2(p.x+7*k,p.y+10*k),ImVec2(p.x+12*k,p.y+6.5f*k),tint);}
}

bool switchButton(const char* id,bool& value){
    ImGui::PushID(id);
    auto position=ImGui::GetCursorScreenPos();
    bool pressed=ImGui::InvisibleButton("switch",ImVec2(34*scale,30*scale),ImGuiButtonFlags_EnableNav);
    if(pressed)value=!value;
    auto storage=ImGui::GetStateStorage();auto itemId=ImGui::GetItemID();
    float progress=storage->GetFloat(itemId,value?1.0f:0.0f);
    progress+=(float(value)-progress)*std::clamp(ImGui::GetIO().DeltaTime*22,0.0f,1.0f);storage->SetFloat(itemId,progress);
    auto draw=ImGui::GetWindowDrawList();float top=position.y+6*scale;
    ImVec4 track(.22f+progress*.41f,.235f+progress*.42f,.255f+progress*.43f,1);
    draw->AddRectFilled(ImVec2(position.x,top),ImVec2(position.x+34*scale,top+18*scale),ImGui::GetColorU32(track),9*scale);
    draw->AddCircleFilled(ImVec2(position.x+(9+16*progress)*scale,top+9*scale),6*scale,ImGui::GetColorU32(value?ImVec4(.12f,.14f,.16f,1):bright));
    if(ImGui::IsItemFocused())draw->AddRect(ImVec2(position.x-3*scale,top-3*scale),ImVec2(position.x+37*scale,top+21*scale),color(accent),11*scale,0,1.5f*scale);
    ImGui::PopID();return pressed;
}

void fittedText(const char* text,float width,const ImVec4& tint){
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+std::max(1.0f,width));ImGui::TextColored(tint,"%s",text);ImGui::PopTextWrapPos();
}

bool numericInput(const char* id,double& value,double minimum,double maximum,const char* format){
    static std::unordered_map<ImGuiID,double> drafts;ImGuiID field=ImGui::GetID(id);auto& draft=drafts[field];
    if(ImGui::GetActiveID()!=field)draft=value;
    ImGui::InputDouble(id,&draft,0,0,format,ImGuiInputTextFlags_AutoSelectAll);
    if(ImGui::IsItemDeactivatedAfterEdit()){if(std::isfinite(draft)){value=std::clamp(draft,minimum,maximum);return true;}}
    return false;
}

void numericCaption(const char* format){
    const char* conversion=std::strchr(format,'%');if(conversion&&conversion>format){std::string label(format,conversion);while(!label.empty()&&std::isspace(static_cast<unsigned char>(label.back())))label.pop_back();if(!label.empty())ImGui::TextColored(muted,"%s",label.c_str());}
}

bool numberField(const char* id,float* value,float,float minimum,float maximum,const char* format,int=0){
    float width=ImGui::CalcItemWidth();numericCaption(format);ImGui::SetNextItemWidth(width);double entered=*value;if(numericInput(id,entered,minimum,maximum,"%.6g")){*value=float(entered);return true;}return false;
}

bool rangeField(const char* id,float* value,float minimum,float maximum,const char* format="%.3f",int=0){
    float width=ImGui::CalcItemWidth();numericCaption(format);ImGui::SetNextItemWidth(width);double entered=*value;if(numericInput(id,entered,minimum,maximum,"%.6g")){*value=float(entered);return true;}return false;
}

bool integerField(const char* id,int* value,int minimum,int maximum,const char* ="%d",int=0){
    double entered=*value;if(numericInput(id,entered,minimum,maximum,"%.0f")){*value=int(std::round(entered));return true;}return false;
}

bool switchRow(const char* text,bool& value){
    ImGui::PushID(text);float left=ImGui::GetCursorPosX(),width=ImGui::GetContentRegionAvail().x;ImGui::AlignTextToFramePadding();fittedText(text,width-44*scale,bright);ImGui::SameLine(left+width-34*scale);bool changed=switchButton("value",value);ImGui::PopID();return changed;
}

void settingToggle(const char* text,int key){
    bool value=toolscreen::snapshot()[key]>.5;ImGui::PushID(key);if(switchRow(text,value))toolscreen::set(key,value?1:0);rememberBounds(key);ImGui::PopID();
}

void integer(const char* text,int key,int minimum=0,int maximum=16384,int automatic=-1){
    double value=toolscreen::snapshot()[key];float left=ImGui::GetCursorPosX(),width=ImGui::GetContentRegionAvail().x,field=std::min(132*scale,width*.55f);ImGui::PushID(key);
    ImGui::AlignTextToFramePadding();fittedText(text,width-field-12*scale,muted);ImGui::SameLine(left+width-field);ImGui::SetNextItemWidth(field);
    if(numericInput("##value",value,minimum,maximum,"%.0f")){if(automatic>=0)toolscreen::set(automatic,0);if(!toolscreen::set(key,std::round(value)))status=toolscreen::resolutionError();}rememberBounds(key);tooltip(automatic>=0?"Type a size. Enter applies it and disables automatic sizing. 0 uses the window size.":"Type a value and press Enter.");ImGui::PopID();
}

void fraction(const char* text,int key,float minimum=0,float maximum=1){
    float value=float(toolscreen::snapshot()[key]),left=ImGui::GetCursorPosX(),width=ImGui::GetContentRegionAvail().x,field=std::min(118*scale,width*.52f);ImGui::PushID(key);
    ImGui::AlignTextToFramePadding();fittedText(text,width-field-10*scale,muted);ImGui::SameLine(left+width-field);ImGui::SetNextItemWidth(field);
    if(numberField("##value",&value,.005f,minimum,maximum,"%.3f",ImGuiSliderFlags_AlwaysClamp|ImGuiSliderFlags_NoRoundToFormat))toolscreen::set(key,value);ImGui::PopID();
}

bool iconButton(const char* label,int kind,bool enabled=true){
    ImGui::PushID(label);ImGui::BeginDisabled(!enabled);auto position=ImGui::GetCursorScreenPos();
    bool pressed=ImGui::InvisibleButton("action",ImVec2(30*scale,30*scale),ImGuiButtonFlags_EnableNav);auto draw=ImGui::GetWindowDrawList();
    bool hover=ImGui::IsItemHovered();if(hover||ImGui::IsItemActive()||ImGui::IsItemFocused())draw->AddRectFilled(position,ImVec2(position.x+30*scale,position.y+30*scale),ImGui::GetColorU32(ImGui::IsItemActive()?ImVec4(.27f,.29f,.32f,1):ImVec4(.19f,.21f,.24f,1)),5*scale);
    ImU32 tint=ImGui::GetColorU32(enabled?bright:ImVec4(.35f,.37f,.4f,1));ImVec2 center(position.x+15*scale,position.y+15*scale);float stroke=1.5f*scale;
    if(kind==0||kind==1){float direction=kind==0?1.0f:-1.0f;auto p=[&](float x,float y){return ImVec2(center.x+x*scale*direction,center.y+y*scale);};draw->PathLineTo(p(6,6));draw->PathLineTo(p(6,3));draw->PathBezierCubicCurveTo(p(6,-1),p(3,-4),p(-1,-4));draw->PathLineTo(p(-7,-4));draw->PathStroke(tint,0,stroke);draw->AddLine(p(-7,-4),p(-2,-9),tint,stroke);draw->AddLine(p(-7,-4),p(-2,1),tint,stroke);}
    else if(kind==2){for(int i=0;i<32;++i){float angle=float(i)*.19634954f,radius=(i%4==0||i%4==3)?8.5f:6.8f;draw->PathLineTo(ImVec2(center.x+std::cos(angle)*radius*scale,center.y+std::sin(angle)*radius*scale));}draw->PathStroke(tint,ImDrawFlags_Closed,stroke);draw->AddCircle(center,3*scale,tint,24,stroke);}
    else if(kind==3){draw->AddLine(ImVec2(center.x-4*scale,center.y-4*scale),ImVec2(center.x+4*scale,center.y+4*scale),tint,stroke);draw->AddLine(ImVec2(center.x+4*scale,center.y-4*scale),ImVec2(center.x-4*scale,center.y+4*scale),tint,stroke);}
    else if(kind==7||kind==8){draw->AddRect(ImVec2(center.x-6*scale,center.y-1*scale),ImVec2(center.x+6*scale,center.y+8*scale),tint,2*scale,0,stroke);draw->PathArcTo(ImVec2(center.x+(kind==8?2:0)*scale,center.y-2*scale),4*scale,3.141593f,6.283186f,24);draw->PathStroke(tint,0,stroke);}
    else if(kind==4){draw->AddLine(ImVec2(center.x-6*scale,center.y),ImVec2(center.x+6*scale,center.y),tint,stroke);draw->AddLine(ImVec2(center.x,center.y-6*scale),ImVec2(center.x,center.y+6*scale),tint,stroke);}
    else if(kind==5){for(int i=-1;i<=1;i++)draw->AddCircleFilled(ImVec2(center.x+i*5*scale,center.y),1.4f*scale,tint);}
    else icon(draw,ImVec2(position.x+7*scale,position.y+7*scale),kind==6?4:1,tint,16*scale);
    if(ImGui::IsItemFocused())draw->AddRect(position,ImVec2(position.x+30*scale,position.y+30*scale),color(accent),5*scale,0,scale);
    ImGui::EndDisabled();if(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled|ImGuiHoveredFlags_DelayShort))ImGui::SetTooltip("%s",label);ImGui::PopID();return pressed;
}

bool selectable(const char* text,bool selected=false,ImGuiSelectableFlags flags=0,ImVec2 requested=ImVec2(0,0)){
    float maximum=std::max(1.0f,displayWidth-2*windowInset()-2*ImGui::GetStyle().WindowPadding.x),width=std::min(requested.x>0?requested.x:320*scale,maximum);auto font=ImGui::GetFont();float fontSize=ImGui::GetFontSize(),wrap=std::max(1.0f,width-24*scale),textHeight=font->CalcTextSizeA(fontSize,FLT_MAX,wrap,text).y;float height=std::max(requested.y,textHeight+16*scale);
    ImGui::PushID(text);bool pressed=ImGui::Selectable("##item",selected,flags,ImVec2(width,height));ImGui::PopID();auto point=ImGui::GetItemRectMin();ImGui::GetWindowDrawList()->AddText(font,fontSize,ImVec2(std::round(point.x+12*scale),std::round(point.y+(height-textHeight)*.5f)),ImGui::GetColorU32(ImGuiCol_Text),text,nullptr,wrap);return pressed;
}

bool pageTab(const char* label,int page,float width){
    ImGui::PushID(label);auto position=ImGui::GetCursorScreenPos();bool active=selectedPage==page;
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.16f,.18f,.20f,1));ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(.21f,.23f,.25f,1));ImGui::PushStyleColor(ImGuiCol_Text,active?bright:muted);
    bool pressed=button(label,ImVec2(width,34*scale));if(active)ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(position.x+12*scale,position.y+33*scale),ImVec2(position.x+width-12*scale,position.y+35*scale),color(accent),scale);
    ImGui::PopStyleColor(4);ImGui::PopID();if(pressed){selectedPage=page;finishDrag();}return pressed;
}

void sensitivityControl(const char* text,int key){
    static std::unordered_map<int,double> exact;auto state=toolscreen::snapshot();double value=state[key],minimum=.001,maximum=20;int enabledKey=38;bool enabled=state[enabledKey]>.5;
    float left=ImGui::GetCursorPosX(),width=ImGui::GetContentRegionAvail().x,field=std::min(112*scale,width*.40f);ImGui::PushID(key);
    bool active=state[128]>.5&&enabled&&(int(state[0])!=3||state[145]<.5);
    ImGui::AlignTextToFramePadding();fittedText(text,width-field-54*scale,active?bright:muted);ImGui::SameLine(left+width-field-44*scale);ImGui::SetNextItemWidth(field);
    if(numericInput("##value",value,minimum,maximum,"%.8g"))toolscreen::set(key,value);rememberBounds(key);
    if(ImGui::IsItemClicked(ImGuiMouseButton_Right)){exact[key]=value;ImGui::OpenPopup("Adjust sensitivity");}tooltip("Type a value and press Enter. Right-click for an exact value.");
    ImGui::SameLine(left+width-34*scale);if(switchButton("enabled",enabled))toolscreen::set(enabledKey,enabled?1:0);rememberBounds(enabledKey);
    if(beginPopup("Adjust sensitivity")){ImGui::TextUnformatted("Normal");ImGui::SetNextItemWidth(220*scale);if(ImGui::IsWindowAppearing())ImGui::SetKeyboardFocusHere();if(ImGui::InputDouble("##exact",&exact[key],0,0,"%.8f",ImGuiInputTextFlags_AutoSelectAll))toolscreen::set(key,exact[key]);if(button("Done"))ImGui::CloseCurrentPopup();endPopup();}ImGui::PopID();
}

void drawGrid(ImDrawList* draw,const Bounds& box,const toolscreen::Quad& layer,const toolscreen::Settings& state){
    if(state[18]<.5||layer[18]<=0)return;
    float ratio=box.w/std::max(layer[16],1.0f);
    float cell=box.w/layer[18];int half=std::min(int(state[19]),int(layer[18]/2));
    float center=box.x+box.w/2,cy=box.y+box.h/2;
    float height=float(state[59])*ratio;
    draw->PushClipRect(ImVec2(box.x,box.y),endOf(box),true);
    ImVec4 a{float(state[44]),float(state[45]),float(state[46]),float(state[56])};
    ImVec4 b{float(state[47]),float(state[48]),float(state[49]),float(state[56])};
    ImU32 text=color(ImVec4(float(state[53]),float(state[54]),float(state[55]),1));
    float uniform=std::min(float(state[58])*ratio,cell/.99f);
    for(int i=-half;i<half;++i){
        int number=i<0?-i:i+1;char label[8];std::snprintf(label,sizeof(label),"%d",number);
        float fontSize=uniform;
        if(int(state[57])==0)fontSize=std::min(42*ratio,cell/.99f);
        if(int(state[57])==1)fontSize=std::min(42*ratio,cell/(number>=10?.99f:.52f));
        if(int(state[57])==2)fontSize=float(state[58])*ratio;
        fontSize=std::min(fontSize,std::max(height-3*ratio,4.0f));
        float x=center+i*cell;draw->AddRectFilled(ImVec2(x,cy-height/2),ImVec2(x+cell,cy+height/2),color((i+half)%2?a:b));
        ImVec2 size=gridFont->CalcTextSizeA(fontSize,1000,0,label);
        draw->AddText(gridFont,fontSize,ImVec2(x+(cell-size.x)/2,cy-size.y/2),text,label);
    }
    if(state[43]>.5)draw->AddLine(ImVec2(center,box.y),ImVec2(center,box.y+box.h),color(ImVec4(float(state[50]),float(state[51]),float(state[52]),1)),std::max(1.0f,ratio));
    draw->PopClipRect();
}

toolscreen::Quad eyeLayer(const std::vector<toolscreen::Quad>& plan){for(auto& layer:plan)if(layer[18]>0)return layer;return {};}

int mirrorOutputRoute(int){return 0;}
void editMirrorValue(int index,int field,double value){toolscreen::editMirrorOutput(index,field==0||field==14||field==32||field==33?0:mirrorOutputRoute(index),field,value);}

void rectangleFields(const char* label,const toolscreen::SourceValues& values,int first,int kind,int id,int sourceWidth,int sourceHeight){
    if(label[0]!='#')ImGui::TextDisabled("%s",label);
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(8*scale,6*scale));
    if(ImGui::BeginTable(label,2,ImGuiTableFlags_SizingStretchSame)){
        const char* names[]={"X","Y","Width","Height"};
        for(int i=0;i<4;++i){
            nextColumn();ImGui::PushID(first+i+id*100);ImGui::TextDisabled("%s",names[i]);ImGui::SetNextItemWidth(-1);
            float extent=float(i%2==0?sourceWidth:sourceHeight);float factor=pixelUnits?extent:1;
            float value=float(values[size_t(first+i)])*factor;
            double entered=value;if(numericInput("##value",entered,0,factor,pixelUnits?"%.0f":"%.3f")){value=float(entered);
                if(kind==0)editMirrorValue(id,first+i,value/factor);
                else if(kind==1)toolscreen::editImage(id,first+i,value/factor);
                else toolscreen::set(14+i,value/factor);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

struct CanvasObject{Bounds bounds;int kind;int index;};

bool objectSelected(const CanvasObject& object){
    return (object.kind==0&&layoutTarget==LayoutTarget::Projector)||(object.kind==1&&layoutTarget==LayoutTarget::Mirror&&object.index==toolscreen::selectedMirror())||(object.kind==2&&layoutTarget==LayoutTarget::Source&&object.index==selectedImage);
}

void selectObject(const CanvasObject& object){
    if(object.kind==0)layoutTarget=LayoutTarget::Projector;
    else if(object.kind==1){layoutTarget=LayoutTarget::Mirror;toolscreen::selectMirror(object.index);}
    else if(object.kind==2){layoutTarget=LayoutTarget::Source;selectedImage=object.index;}
}

void editObjectRectangle(int kind,int index,const std::array<double,4>& values){
    for(int i=0;i<4;++i){if(kind==0)toolscreen::set(14+i,values[i]);else if(kind==1)editMirrorValue(index,5+i,values[i]);else if(kind==2)toolscreen::editImage(index,5+i,values[i]);}
}

int canvasResizeHandle(const Bounds& bounds,ImVec2 mouse){
    float threshold=7*scale;bool insideX=mouse.x>=bounds.x-threshold&&mouse.x<=bounds.x+bounds.w+threshold,insideY=mouse.y>=bounds.y-threshold&&mouse.y<=bounds.y+bounds.h+threshold;
    if(!insideX||!insideY)return 0;
    int horizontal=std::abs(mouse.x-bounds.x)<=threshold?1:std::abs(mouse.x-bounds.x-bounds.w)<=threshold?2:0;
    int vertical=std::abs(mouse.y-bounds.y)<=threshold?4:std::abs(mouse.y-bounds.y-bounds.h)<=threshold?8:0;
    return horizontal|vertical;
}

ImGuiMouseCursor canvasCursor(int handle){
    if(handle==5||handle==10)return ImGuiMouseCursor_ResizeNWSE;
    if(handle==9||handle==6)return ImGuiMouseCursor_ResizeNESW;
    if(handle&3)return ImGuiMouseCursor_ResizeEW;
    if(handle&12)return ImGuiMouseCursor_ResizeNS;
    return ImGuiMouseCursor_ResizeAll;
}

void snapCanvasObject(std::array<double,4>& values,const std::vector<CanvasObject>& objects,const Bounds& canvas,int resizeHandle,double& guideX,double& guideY){
    auto snap=[&](bool horizontal){
        int first=horizontal?0:1,extent=horizontal?2:3;double threshold=6*scale/(horizontal?canvas.w:canvas.h),best=threshold,delta=0,target=-1;
        std::vector<double> targets{0,.5,1};
        for(const auto& object:objects){if(object.kind==drag.kind&&object.index==drag.index)continue;double start=horizontal?(object.bounds.x-canvas.x)/canvas.w:(object.bounds.y-canvas.y)/canvas.h,length=horizontal?object.bounds.w/canvas.w:object.bounds.h/canvas.h;targets.push_back(start);targets.push_back(start+length*.5);targets.push_back(start+length);}
        std::vector<double> edges;
        if(resizeHandle){int leading=horizontal?1:4,trailing=horizontal?2:8;if(resizeHandle&leading)edges.push_back(values[first]);if(resizeHandle&trailing)edges.push_back(values[first]+values[extent]);}
        else edges={values[first],values[first]+values[extent]*.5,values[first]+values[extent]};
        for(double candidate:targets)for(double edge:edges){double distance=std::abs(candidate-edge);if(candidate>=0&&candidate<=1&&distance<best){best=distance;delta=candidate-edge;target=candidate;}}
        if(target<0)return;
        if(resizeHandle){int leading=horizontal?1:4;if(resizeHandle&leading){values[first]+=delta;values[extent]-=delta;}else values[extent]+=delta;}
        else values[first]+=delta;
        if(horizontal)guideX=target;else guideY=target;
    };
    if(resizeHandle||drag.lockedAxis!=2)snap(true);if(resizeHandle||drag.lockedAxis!=1)snap(false);
}

void compositionCanvas(float availableHeight){
    auto state=toolscreen::snapshot();auto view=toolscreen::viewport(displayWidth,displayHeight,16384,false);auto layers=toolscreen::planForOutput(displayWidth,displayHeight,view[0],view[1],false,0,previewRoute);
    auto position=ImGui::GetCursorScreenPos();float width=ImGui::GetContentRegionAvail().x;Bounds frame{position.x,position.y,width,std::max(64*scale,availableHeight)};auto draw=ImGui::GetWindowDrawList();
    float fit=std::min((frame.w-24*scale)/displayWidth,(frame.h-24*scale)/displayHeight);Bounds canvas{frame.x+(frame.w-displayWidth*fit)/2,frame.y+(frame.h-displayHeight*fit)/2,displayWidth*fit,displayHeight*fit};std::vector<CanvasObject> objects;
    for(auto& layer:layers)if(layer[18]>0&&int(state[0])==3){Bounds box{canvas.x+layer[4]*canvas.w,canvas.y+(1-layer[5]-layer[7])*canvas.h,layer[6]*canvas.w,layer[7]*canvas.h};objects.push_back({box,0,0});}
    for(auto& source:toolscreen::images()){auto& v=source.values;if(source.id!=int(state[121])&&v[0]>.5&&(int(v[14])&(1<<int(state[0])))&&(int(v[32])&(1<<previewRoute)))objects.push_back({{canvas.x+float(v[5])*canvas.w,canvas.y+float(v[6])*canvas.h,float(v[7])*canvas.w,float(v[8])*canvas.h},2,source.id});}
    auto mirrors=toolscreen::mirrors();auto debug=toolscreen::debugState();
    for(size_t i=0;i<mirrors.size();++i){auto routed=toolscreen::mirrorForOutput(mirrors[i],previewRoute);auto& v=routed.values;if(v[0]<.5||!(int(v[14])&(1<<int(state[0])))||!(int(v[32])&(1<<previewRoute)))continue;bool available=true;if(int(v[33])==1)available=!debug.counter.empty();if(int(v[33])==2)available=!debug.path.empty();if(int(v[33])==3){toolscreen::ProfileRow row;available=toolscreen::maplessRow(debug,row,v[34]>.5);}if(!available)continue;auto extent=toolscreen::runtimeMirrorExtent(routed,ImVec2(canvas.w,canvas.h),debug);double w=extent.x/canvas.w,h=extent.y/canvas.h;if(v[31]>0){w=std::min(1.0,(v[20]>.5?std::clamp(v[23],1.0,double(view[0])):v[3]*view[0])*v[31]/displayWidth);h=std::min(1.0,(v[20]>.5?std::clamp(v[24],1.0,double(view[1])):v[4]*view[1])*v[31]/displayHeight);}auto position=toolscreen::runtimeMirrorPosition(v,ImVec2(canvas.w,canvas.h),extent);objects.push_back({{canvas.x+position.x,canvas.y+position.y,float(w)*canvas.w,float(h)*canvas.h},1,int(i)});}
    ImGui::InvisibleButton("composition",ImVec2(frame.w,frame.h),ImGuiButtonFlags_MouseButtonLeft);rememberBounds(400);
    canvasNavigationFocus=ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);bool hovered=ImGui::IsItemHovered();auto& io=ImGui::GetIO();auto mouse=io.MousePos;const CanvasObject* hit=nullptr;int handle=0;
    if(hovered&&canvasEditable){
        for(auto it=objects.rbegin();it!=objects.rend();++it)if(objectSelected(*it)&&(handle=canvasResizeHandle(it->bounds,mouse))!=0){hit=&*it;break;}
        if(!hit)for(auto it=objects.rbegin();it!=objects.rend();++it)if(contains(it->bounds,mouse)){hit=&*it;break;}
        if(hit)ImGui::SetMouseCursor(canvasCursor(handle));
        if(ImGui::IsMouseClicked(0)){
            finishDrag();
            if(hit){
                selectObject(*hit);drag.active=true;drag.resizeHandle=handle;drag.lockedAxis=0;drag.storedHeight=-1;drag.storedWidth=-1;drag.kind=hit->kind;drag.index=hit->index;drag.mouse=mouse;
                drag.values={(hit->bounds.x-canvas.x)/canvas.w,(hit->bounds.y-canvas.y)/canvas.h,hit->bounds.w/canvas.w,hit->bounds.h/canvas.h};
                if(hit->kind==1){auto routed=toolscreen::mirrorForOutput(mirrors[size_t(hit->index)],previewRoute);if(!(handle&12)&&routed.values[37]>.5)drag.storedHeight=routed.values[8];if(!(handle&3)&&routed.style[toolscreen::AutoWidth]>.5)drag.storedWidth=routed.values[7];}
            }else if(contains(canvas,mouse))layoutTarget=LayoutTarget::Game;
        }
    }
    controlBounds[401]=ImVec4(hovered?1:0,canvasEditable?1:0,hit?hit->kind:-1,int(layoutTarget));
    double guideX=-1,guideY=-1;std::array<double,4> activeValues=drag.values;
    if(drag.active){
        if(drag.committed||ImGui::IsMouseDragging(0,3*scale)){
            if(!drag.committed){toolscreen::beginEdit();drag.committed=true;if(drag.kind==0){toolscreen::set(35,0);editObjectRectangle(0,0,drag.values);}}
            double dx=(mouse.x-drag.mouse.x)/canvas.w,dy=(mouse.y-drag.mouse.y)/canvas.h;auto values=drag.values;
            if(drag.resizeHandle){
                double minimumWidth=std::min(.25,8.0*scale/canvas.w),minimumHeight=std::min(.25,8.0*scale/canvas.h),right=std::clamp(values[0]+values[2],minimumWidth,1.0),bottom=std::clamp(values[1]+values[3],minimumHeight,1.0),left=std::clamp(values[0],0.0,right-minimumWidth),top=std::clamp(values[1],0.0,bottom-minimumHeight);
                if(drag.resizeHandle&1)left=std::clamp(left+dx,0.0,right-minimumWidth);
                if(drag.resizeHandle&2)right=std::clamp(right+dx,left+minimumWidth,1.0);
                if(drag.resizeHandle&4)top=std::clamp(top+dy,0.0,bottom-minimumHeight);
                if(drag.resizeHandle&8)bottom=std::clamp(bottom+dy,top+minimumHeight,1.0);
                if(io.KeyShift&&drag.kind==2&&(drag.resizeHandle&3)&&(drag.resizeHandle&12)){
                    double aspect=drag.values[2]/std::max(drag.values[3],.00001),width=right-left,height=bottom-top;
                    if(std::abs(dx)/drag.values[2]>std::abs(dy)/drag.values[3])height=width/aspect;else width=height*aspect;
                    if(drag.resizeHandle&1)left=right-width;else right=left+width;
                    if(drag.resizeHandle&4)top=bottom-height;else bottom=top+height;
                }
                values={std::max(0.0,left),std::max(0.0,top),std::min(1.0,right)-std::max(0.0,left),std::min(1.0,bottom)-std::max(0.0,top)};
            }else{
                if(io.KeyShift){if(!drag.lockedAxis)drag.lockedAxis=std::abs(dx)>std::abs(dy)?1:2;if(drag.lockedAxis==1)dy=0;else dx=0;}else drag.lockedAxis=0;
                values[0]=std::clamp(values[0]+dx,0.0,std::max(0.0,1-values[2]));values[1]=std::clamp(values[1]+dy,0.0,std::max(0.0,1-values[3]));
            }
            if(!io.KeyAlt&&!(io.KeyShift&&drag.resizeHandle&&drag.kind==2))snapCanvasObject(values,objects,canvas,drag.resizeHandle,guideX,guideY);
            values[0]=std::clamp(values[0],0.0,1.0);values[1]=std::clamp(values[1],0.0,1.0);values[2]=std::clamp(values[2],.001,std::max(.001,1-values[0]));values[3]=std::clamp(values[3],.001,std::max(.001,1-values[1]));
            activeValues=values;if(drag.storedWidth>=0)values[2]=drag.storedWidth;if(drag.storedHeight>=0)values[3]=drag.storedHeight;editObjectRectangle(drag.kind,drag.index,values);ImGui::SetMouseCursor(canvasCursor(drag.resizeHandle));
        }
        if(!ImGui::IsMouseDown(0))finishDrag();
    }
    draw->AddRectFilled(position,endOf(frame),IM_COL32(14,15,17,255),8*scale);
    draw->PushClipRect(ImVec2(canvas.x,canvas.y),endOf(canvas),true);draw->AddImage((ImTextureID)(previewRoute==0?3:4),ImVec2(canvas.x,canvas.y),endOf(canvas),ImVec2(0,0),ImVec2(1,1));
    auto currentState=toolscreen::snapshot();if(int(currentState[0])==3){auto currentLayers=toolscreen::planForOutput(displayWidth,displayHeight,view[0],view[1],false,0,previewRoute);for(const auto& layer:currentLayers)if(layer[18]>0){Bounds box{canvas.x+layer[4]*canvas.w,canvas.y+(1-layer[5]-layer[7])*canvas.h,layer[6]*canvas.w,layer[7]*canvas.h};drawGrid(draw,box,layer,currentState);}}
    toolscreen::drawRuntimeOverlays(draw,ImVec2(canvas.x,canvas.y),ImVec2(canvas.w,canvas.h),previewRoute,bodyFont,dataFont,gridFont);draw->PopClipRect();draw->AddRect(ImVec2(canvas.x,canvas.y),endOf(canvas),IM_COL32(48,51,56,255));
    draw->PushClipRect(ImVec2(frame.x,frame.y),endOf(frame),true);
    if(guideX>=0){float x=std::floor(canvas.x+float(guideX)*canvas.w)+.5f;draw->AddLine(ImVec2(x,canvas.y),ImVec2(x,canvas.y+canvas.h),color(copper),scale);}
    if(guideY>=0){float y=std::floor(canvas.y+float(guideY)*canvas.h)+.5f;draw->AddLine(ImVec2(canvas.x,y),ImVec2(canvas.x+canvas.w,y),color(copper),scale);}
    for(auto& object:objects){
        if(drag.active&&drag.committed&&object.kind==drag.kind&&object.index==drag.index)object.bounds={canvas.x+float(activeValues[0])*canvas.w,canvas.y+float(activeValues[1])*canvas.h,float(activeValues[2])*canvas.w,float(activeValues[3])*canvas.h};
        bool selected=objectSelected(object);if(!canvasEditable||(!selected&&hit!=&object))continue;auto corner=endOf(object.bounds);
        draw->AddRect(ImVec2(object.bounds.x,object.bounds.y),corner,selected?color(accent):IM_COL32(166,173,183,180),0,0,scale);
        if(selected)for(int y=0;y<3;++y)for(int x=0;x<3;++x){if(x==1&&y==1)continue;ImVec2 center(object.bounds.x+object.bounds.w*x*.5f,object.bounds.y+object.bounds.h*y*.5f);float half=3*scale;draw->AddRectFilled(ImVec2(center.x-half,center.y-half),ImVec2(center.x+half,center.y+half),IM_COL32(24,25,27,255),scale);draw->AddRect(ImVec2(center.x-half,center.y-half),ImVec2(center.x+half,center.y+half),color(accent),scale,0,scale);}
    }
    if(drag.active&&drag.committed){
        char text[96];std::snprintf(text,sizeof(text),drag.resizeHandle?"W %.0f   H %.0f":"X %.0f   Y %.0f",activeValues[drag.resizeHandle?2:0]*displayWidth,activeValues[drag.resizeHandle?3:1]*displayHeight);
        auto extent=captionFont->CalcTextSizeA(captionFont->FontSize,1000,0,text);ImVec2 point(std::clamp(mouse.x+14*scale,frame.x+4*scale,frame.x+frame.w-extent.x-16*scale),std::clamp(mouse.y+18*scale,frame.y+4*scale,frame.y+frame.h-extent.y-12*scale));
        draw->AddRectFilled(point,ImVec2(point.x+extent.x+12*scale,point.y+extent.y+8*scale),IM_COL32(37,40,45,255),4*scale);draw->AddText(captionFont,captionFont->FontSize,ImVec2(point.x+6*scale,point.y+4*scale),color(bright),text);
    }
    draw->PopClipRect();
}

void chooseMode(){
    auto modeList=toolscreen::modes();int active=int(toolscreen::snapshot()[0]);float available=ImGui::GetContentRegionAvail().x;int columns=available>530*scale?5:3;float width=(available-(columns-1)*8*scale)/columns;
    const char* shortNames[]={"Fullscreen","Thin","Wide","EyeZoom","Preemptive"};
    float start=ImGui::GetCursorPosX();size_t count=std::min(size_t(5),modeList.size());for(size_t i=0;i<count;++i){if(i%columns)ImGui::SameLine(0,8*scale);else{int items=std::min(columns,int(count-i));ImGui::SetCursorPosX(start+(available-items*width-(items-1)*8*scale)*.5f);}ImGui::PushID(int(i));const char* label=width<100*scale?(i==0?"Full":i==4?"Preempt":shortNames[i]):modeList[i].name.c_str();if(pill(label,active==int(i),width,true)){if(!toolscreen::set(0,int(i)))status=toolscreen::resolutionError();layoutTarget=i==3?LayoutTarget::Projector:LayoutTarget::Game;}rememberBounds(100+int(i));ImGui::PopID();}
    if(modeList.size()>5){ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##custom-modes",active>=5?modeList[size_t(active)].name.c_str():"Custom modes")){for(size_t i=5;i<modeList.size();++i)if(selectable(modeList[i].name.c_str(),active==int(i)))toolscreen::set(0,int(i));ImGui::EndCombo();}}
}

void gridInspector(){
    auto s=toolscreen::snapshot();settingToggle("Center line",43);
    integer("Labels per side",19,1,9);
    int mode=int(s[57]);const char* items[]={"Fit labels","Per-cell size","Manual size"};ImGui::TextDisabled("Label size");ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##label-sizing",&mode,items,3))toolscreen::set(57,mode);
    if(mode==2)integer("Text size",58,8,96);
    integer("Band height",59,8,96);
    for(int group=0;group<4;++group){int first=44+group*3;float c[]={float(s[first]),float(s[first+1]),float(s[first+2])};const char* names[]={"Grid pink","Grid blue","Center line","Text"};if(ImGui::ColorEdit3(names[group],c,ImGuiColorEditFlags_NoInputs))for(int i=0;i<3;++i)toolscreen::set(first+i,c[i]);}
}

void modeInspector(int requested=-1){
    auto state=toolscreen::snapshot();auto modes=toolscreen::modes();int mode=requested<0?int(state[0]):std::clamp(requested,0,int(modes.size())-1);heading(requested<0?"Game":modes[size_t(mode)].name.c_str());section("Resolution");
    if(mode==0)ImGui::Text("%d × %d",displayWidth,displayHeight);
    else if(mode==1||mode==2){settingToggle("Automatic",mode==1?36:37);integer("Width",mode==1?1:3,0,16384,mode==1?36:37);integer("Height",mode==1?2:4,0,16384,mode==1?36:37);}
    else if(mode==3||mode==4){integer("Width",5,1);integer("Height",6,1);}
    else{char name[128];std::snprintf(name,sizeof(name),"%s",modes[size_t(mode)].name.c_str());double width=modes[size_t(mode)].width,height=modes[size_t(mode)].height;ImGui::SetNextItemWidth(-1);bool changed=ImGui::InputText("##name",name,sizeof(name));ImGui::TextColored(muted,"Width");ImGui::SetNextItemWidth(-1);changed=numericInput("##width",width,0,16384,"%.3f")||changed;rememberBounds(8050);ImGui::TextColored(muted,"Height");ImGui::SetNextItemWidth(-1);changed=numericInput("##height",height,0,16384,"%.3f")||changed;rememberBounds(8051);if(changed)toolscreen::editMode(mode,name,width,height);}
    ImGui::Spacing();if(button("Canvas style",ImVec2(-1,30*scale))){selectedPage=0;layoutTarget=LayoutTarget::Appearance;}
    if(requested<0&&disclosure("Custom modes")){if(button("Add mode",ImVec2(-1,30*scale))){int added=toolscreen::addMode("Custom mode");if(added>=0)toolscreen::set(0,added);}if(mode>=5&&button("Remove mode",ImVec2(-1,30*scale)))toolscreen::removeMode(mode);}
}

void projectorInspector(){
    auto state=toolscreen::snapshot();heading("Eye projector");settingToggle("Enabled",11);settingToggle("Auto placement",35);if(int(state[0])!=3)ImGui::TextDisabled("Shown in EyeZoom");
    if(state[35]<.5){toolscreen::SourceValues values{};for(int i=0;i<4;++i)values[i]=state[14+i];section("Position");rectangleFields("##projector-position",values,0,2,0,displayWidth,displayHeight);}
    settingToggle("Numbered grid",18);if(disclosure("Grid options"))gridInspector();if(disclosure("Source crop")){integer("Width",12,1);integer("Height",13,1);}
}

void sourceEffects(const toolscreen::SourceValues& values,int kind,int id){
    auto edit=[kind,id](int field,double value){if(kind==0)editMirrorValue(id,field,value);else toolscreen::editImage(id,field,value);};
    float opacity=float(values[9]);ImGui::TextColored(muted,"Opacity");ImGui::SetNextItemWidth(-1);if(numberField("##opacity",&opacity,.01f,0,1,"%.2f",ImGuiSliderFlags_AlwaysClamp))edit(9,opacity);
    if(disclosure("Appearance")){if(values[33]>=1){float textScale=float(values[35]>0?values[35]:1);ImGui::TextColored(muted,"Text scale");ImGui::SetNextItemWidth(-1);if(numberField("##text-scale",&textScale,.01f,.5f,2.5f,"%.2f ×",ImGuiSliderFlags_AlwaysClamp))edit(35,textScale);if(values[33]>=2){float background=float(values[36]);ImGui::TextColored(muted,"Background");ImGui::SetNextItemWidth(-1);if(numberField("##background-opacity",&background,.01f,0,1,"%.2f",ImGuiSliderFlags_AlwaysClamp))edit(36,background);}}if(values[33]<.5){bool nearest=values[13]>.5;if(switchRow("Sharp scaling",nearest))edit(13,nearest?1:0);bool key=values[10]>.5;if(switchRow("Remove black",key)){edit(10,key?1:0);if(key)edit(26,0);}bool isolate=values[26]>.5;if(switchRow("Color key",isolate)){edit(26,isolate?1:0);if(isolate)edit(10,0);}if(isolate){float target[]={float(values[27]),float(values[28]),float(values[29])};if(ImGui::ColorEdit3("Color",target,ImGuiColorEditFlags_NoInputs))for(int i=0;i<3;++i)edit(27+i,target[i]);bool white=values[30]>.5;if(switchRow("White foreground",white))edit(30,white?1:0);}if(key||isolate){float tolerance=float(values[11]);ImGui::SetNextItemWidth(-1);if(numberField("##tolerance",&tolerance,.001f,0,1,"Tolerance %.3f",ImGuiSliderFlags_AlwaysClamp))edit(11,tolerance);}bool rainbow=values[12]>.5;if(switchRow("Rainbow",rainbow))edit(12,rainbow?1:0);}float thickness=float(values[15]);ImGui::SetNextItemWidth(-1);if(numberField("##border",&thickness,.1f,0,20,"Border %.1f px",ImGuiSliderFlags_AlwaysClamp))edit(15,thickness);if(thickness>0){float tint[]={float(values[16]),float(values[17]),float(values[18]),float(values[19])};if(ImGui::ColorEdit4("Border color",tint,ImGuiColorEditFlags_NoInputs))for(int i=0;i<4;++i)edit(16+i,tint[i]);}}
    if(disclosure("Modes")){int mask=int(values[14]);auto modes=toolscreen::modes();for(size_t i=0;i<modes.size();++i){bool enabled=(mask&(1<<i))!=0;if(switchRow(modes[i].name.c_str(),enabled)){if(enabled)mask|=1<<i;else mask&=~(1<<i);edit(14,mask);}}}
}

void overlayAppearance(int index,const toolscreen::Mirror& mirror,int route){
    using namespace toolscreen;auto& style=mirror.style;int type=int(mirror.values[33]);auto set=[&](int field,double value){editOverlayStyle(index,route,field,value);};
    auto toggle=[&](const char* label,int field){bool value=style[field]>.5;if(switchRow(label,value))set(field,value?1:0);};
    auto slider=[&](const char* label,int field,float minimum,float maximum,const char* format){float value=float(style[field]);ImGui::TextColored(muted,"%s",label);ImGui::SetNextItemWidth(-1);ImGui::PushID(field);if(numberField("##style",&value,.05f,minimum,maximum,format,ImGuiSliderFlags_AlwaysClamp))set(field,value);ImGui::PopID();};
    section("Style");float half=(ImGui::GetContentRegionAvail().x-6*scale)*.5f;
    for(int preset=0;preset<2;++preset){if(preset)ImGui::SameLine(0,6*scale);if(button(type==2?(preset?"Pixel":"Smooth"):(preset?"Clean":"Vanilla"),ImVec2(half,30*scale))){auto next=defaultOverlayStyle(type);if(type==2){if(preset)next[FontFace]=3;}else if(preset){next[FontFace]=0;next[CornerRadius]=6;next[TextOutline]=0;}beginEdit();for(int field=0;field<OverlayPropertyCount;++field)set(field,next[field]);editMirrorValue(index,36,type==2?1.0:preset?.9:.94);editMirrorValue(index,15,0);endEdit();}}
    int font=int(style[FontFace]);const char* fonts[]={"Interface","Monospace","Semibold","Pixel"};ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##font",&font,fonts,4))set(FontFace,font);
    float textScale=float(mirror.values[35]);ImGui::TextColored(muted,"Text size");ImGui::SetNextItemWidth(-1);if(numberField("##text-scale",&textScale,.01f,.5f,2.5f,"%.2f ×",ImGuiSliderFlags_AlwaysClamp))editMirrorValue(index,35,textScale);
    float opacity=float(mirror.values[36]);ImGui::TextColored(muted,"Background");ImGui::SetNextItemWidth(-1);if(rangeField("##panel-opacity",&opacity,0,1,"%.2f"))editMirrorValue(index,36,opacity);
    if(type==2){toggle("Pie chart",ShowBars);if(style[ShowBars]>.5){float chartScale=mirror.values[39]>0?float(mirror.values[39]):1;ImGui::TextColored(muted,"Chart size");ImGui::SetNextItemWidth(-1);if(rangeField("##chart-size",&chartScale,.65f,1.6f,"%.2f ×"))editMirrorValue(index,39,chartScale);}int rows=int(mirror.values[38]);ImGui::TextColored(muted,"Visible rows");ImGui::SetNextItemWidth(-1);if(integerField("##visible-rows",&rows,0,32,rows==0?"All":"%d"))editMirrorValue(index,38,rows);}
    if(disclosure("Content")){toggle("Title",ShowTitle);if(type==1){toggle("Labels",ShowDetails);toggle("Rendered",ShowLocal);toggle("Total",ShowGlobal);toggle("Meter",ShowBars);}else if(type==2){toggle("Path",ShowPath);toggle("Column labels",ShowDetails);toggle("Parent values",ShowLocal);toggle("Frame values",ShowGlobal);toggle("Row numbers",ShowIndices);toggle("Back hint",ShowFooter);}else if(type==3){toggle("Section label",ShowDetails);toggle("Parent value",ShowLocal);toggle("Frame value",ShowGlobal);toggle("Meter",ShowBars);}if(type>=2){int value=int(style[Decimals]);ImGui::TextColored(muted,"Decimal places");ImGui::SetNextItemWidth(-1);if(integerField("##decimals",&value,0,2))set(Decimals,value);}}
    if(disclosure("Typography")){if(type!=2){int alignment=int(style[TextAlignment]);const char* labels[]={"Left","Center","Right"};ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##alignment",&alignment,labels,3))set(TextAlignment,alignment);slider("Value size",ValueScale,.5f,3,"%.2f ×");}slider("Title size",TitleScale,.5f,3,"%.2f ×");if(type==2)slider("Row spacing",RowHeight,18,64,"%.0f px");toggle("Text shadow",TextOutline);}
    if(disclosure("Colors and border")){for(int group=0;group<3;++group){int first=group*3;float color[]={float(style[first]),float(style[first+1]),float(style[first+2])};const char* labels[]={"Text","Secondary","Background"};if(ImGui::ColorEdit3(labels[group],color,ImGuiColorEditFlags_NoInputs))for(int i=0;i<3;++i)set(first+i,color[i]);}slider("Padding",Padding,0,40,"%.0f px");slider("Corners",CornerRadius,0,30,"%.0f px");float border=float(mirror.values[15]);ImGui::SetNextItemWidth(-1);if(numberField("##panel-border",&border,.1f,0,20,"Border %.1f px",ImGuiSliderFlags_AlwaysClamp))editMirrorValue(index,15,border);float color[]={float(mirror.values[16]),float(mirror.values[17]),float(mirror.values[18]),float(mirror.values[19])};if(ImGui::ColorEdit4("Border",color,ImGuiColorEditFlags_NoInputs))for(int i=0;i<4;++i)editMirrorValue(index,16+i,color[i]);}
}

void mirrorInspector(){
    auto list=toolscreen::mirrors();int selected=toolscreen::selectedMirror();if(selected<0||selected>=int(list.size())){layoutTarget=LayoutTarget::Game;return;}auto item=toolscreen::mirrorForOutput(list[size_t(selected)],previewRoute);int route=mirrorOutputRoute(selected);heading("Overlay");char name[128];std::snprintf(name,sizeof(name),"%s",item.name.c_str());ImGui::SetNextItemWidth(-1);if(ImGui::InputText("##mirror-name",name,sizeof(name)))toolscreen::renameMirror(selected,name);rememberBounds(21);bool enabled=item.values[0]>.5;if(switchRow("Enabled",enabled))editMirrorValue(selected,0,enabled?1:0);rememberBounds(20);

    auto debug=toolscreen::debugState();int type=int(item.values[33]);if(type==1&&debug.counter.empty())ImGui::TextDisabled("F3 counter unavailable");if(type==2&&debug.path.empty())ImGui::TextWrapped("Open the profiler to show this layer");if(type==3)ImGui::TextWrapped("Uses the selected profiler section");
    overlayAppearance(selected,item,route);
    auto positionValues=item.values;bool autoWidth=item.style[toolscreen::AutoWidth]>.5,autoHeight=item.values[37]>.5;if(disclosure("Position and size")){if(switchRow("Auto width",autoWidth))toolscreen::editOverlayStyle(selected,route,toolscreen::AutoWidth,autoWidth?1:0);if(switchRow("Auto height",autoHeight))editMirrorValue(selected,37,autoHeight?1:0);auto extent=toolscreen::runtimeMirrorExtent(item,ImVec2(float(displayWidth),float(displayHeight)),debug);auto position=toolscreen::runtimeMirrorPosition(item.values,ImVec2(float(displayWidth),float(displayHeight)),extent);positionValues[5]=position.x/displayWidth;positionValues[6]=position.y/displayHeight;if(autoWidth)positionValues[7]=extent.x/displayWidth;if(autoHeight)positionValues[8]=extent.y/displayHeight;rectangleFields("##mirror-position",positionValues,5,0,selected,displayWidth,displayHeight);}
    float opacity=float(item.values[9]);ImGui::TextColored(muted,"Opacity");ImGui::SetNextItemWidth(-1);if(numberField("##opacity",&opacity,.01f,0,1,"%.2f",ImGuiSliderFlags_AlwaysClamp))editMirrorValue(selected,9,opacity);
    if(disclosure("Modes")){auto modes=toolscreen::modes();int mask=int(item.values[14]);for(size_t i=0;i<modes.size();++i){bool active=(mask&(1<<i))!=0;if(switchRow(modes[i].name.c_str(),active))toolscreen::editMirror(selected,14,active?mask|(1<<i):mask&~(1<<i));}}
    if(disclosure("Manage")){if(button("Duplicate",ImVec2(-1,30*scale)))toolscreen::duplicateMirror(selected);if(button("Remove",ImVec2(-1,30*scale))){toolscreen::removeMirror(selected);layoutTarget=LayoutTarget::Game;}}
}

void layerMenu(){
    if(button("Layers",ImVec2(82*scale,30*scale)))ImGui::OpenPopup("layers");
    if(beginPopup("layers")){ImGui::SetNextItemWidth(280*scale);if(selectable("Game",layoutTarget==LayoutTarget::Game,0,ImVec2(260*scale,28*scale)))layoutTarget=LayoutTarget::Game;if(selectable("Eye projector",layoutTarget==LayoutTarget::Projector,0,ImVec2(260*scale,28*scale)))layoutTarget=LayoutTarget::Projector;
        auto mirrors=toolscreen::mirrors();for(size_t i=0;i<mirrors.size();++i){ImGui::PushID(int(i));if(selectable(mirrors[i].name.c_str(),layoutTarget==LayoutTarget::Mirror&&toolscreen::selectedMirror()==int(i),0,ImVec2(260*scale,28*scale))){toolscreen::selectMirror(int(i));layoutTarget=LayoutTarget::Mirror;}ImGui::PopID();}for(auto& source:toolscreen::images())if(source.id!=int(toolscreen::snapshot()[121])){ImGui::PushID(source.id);if(selectable(source.name.c_str(),layoutTarget==LayoutTarget::Source&&selectedImage==source.id,0,ImVec2(260*scale,28*scale))){selectedImage=source.id;layoutTarget=LayoutTarget::Source;}ImGui::PopID();}endPopup();}
}

void imageInspector(){
    auto sources=toolscreen::images();auto found=std::find_if(sources.begin(),sources.end(),[](auto& source){return source.id==selectedImage;});if(found==sources.end()){layoutTarget=LayoutTarget::Game;return;}auto& source=*found;heading(source.path.starts_with("browser:")?"Browser":source.path.starts_with("window:")?"Window":"Image");char name[160];std::snprintf(name,sizeof(name),"%s",source.name.c_str());ImGui::SetNextItemWidth(-1);if(ImGui::InputText("##source-name",name,sizeof(name)))toolscreen::renameImage(source.id,name);tooltip(source.path.c_str());auto foundStatus=assetStatuses.find(source.id);if(foundStatus!=assetStatuses.end()&&!foundStatus->second.error.empty())ImGui::TextWrapped("%s",foundStatus->second.error.c_str());bool enabled=source.values[0]>.5;if(switchRow("Enabled",enabled))toolscreen::editImage(source.id,0,enabled?1:0);section("Position");rectangleFields("##source-position",source.values,5,1,source.id,displayWidth,displayHeight);if(disclosure("Crop"))rectangleFields("##source-crop",source.values,1,1,source.id,std::max(source.width,1),std::max(source.height,1));sourceEffects(source.values,1,source.id);if(disclosure("Manage")){if(button("Duplicate",ImVec2(-1,30*scale))){toolscreen::beginEdit();int added=toolscreen::addImage(source.path);for(int i=0;i<40;++i)toolscreen::editImage(added,i,source.values[i]);toolscreen::renameImage(added,source.name+" copy");selectedImage=added;toolscreen::endEdit();}if(button("Remove",ImVec2(-1,30*scale))){toolscreen::removeImage(source.id);layoutTarget=LayoutTarget::Game;}}
}

std::string keyName(int code,int mods){
    std::string name;if(mods&2)name+="Ctrl + ";if(mods&1)name+="Shift + ";if(mods&4)name+="Alt + ";if(mods&8)name+="Cmd + ";
    if(code>=1073741882&&code<=1073741893)name+="F"+std::to_string(code-1073741881);
    else if(code==32)name+="Space";
    else if(code>31&&code<127)name+=char(code>='a'&&code<='z'?code-32:code);
    else if(code==9)name+="Tab";
    else if(code==13)name+="Enter";
    else if(code==8)name+="Backspace";
    else if(code==1073742049)name+="Left Shift";
    else if(code==1073742048)name+="Left Ctrl";
    else if(code==1073742050)name+="Left Alt";
    else if(code==0)name+="Unbound";else name+="Key "+std::to_string(code);
    return name;
}

void keyboardRemaps(){
    settingToggle("Enable remapping",112);auto mappings=toolscreen::remaps();
    for(auto& mapping:mappings){ImGui::PushID(mapping.sourceScan);float left=ImGui::GetCursorPosX(),width=ImGui::GetContentRegionAvail().x;ImGui::AlignTextToFramePadding();std::string label=keyName(mapping.sourceKey,0)+" → "+keyName(mapping.targetKey,0);fittedText(label.c_str(),width-38*scale,bright);ImGui::SameLine(left+width-30*scale);if(iconButton("Remove remap",3))toolscreen::clearRemap(mapping.sourceScan);ImGui::PopID();}
    if(bindingRemapScan==-1){if(button("Add remap")){bindingRemapScan=-2;bindingMode=-1;}}
    else{ImGui::TextWrapped("%s",bindingRemapScan==-2?"Press the original key":"Press the replacement key");if(button("Cancel"))bindingRemapScan=-1;}
}

void sensitivityPanel(){
    heading("Mouse");sensitivityControl("Normal",39);settingToggle("Boat Eye godsens",145);tooltip("Use godsens for vanilla camera turns, preserve the normal turn rate, and slow EyeZoom with whole mouse counts. Cinematic smoothing is bypassed while enabled.");ImGui::TextDisabled("Ninjabrain sensitivity: 0.02291165");
}

void bindingTable(){
    auto state=toolscreen::snapshot();auto modes=toolscreen::modes();heading("Shortcuts");ImGui::Spacing();ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(6*scale,5*scale));
    if(!ImGui::BeginTable("bindings",3,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoPadOuterX)){ImGui::PopStyleVar();return;}
    ImGui::TableSetupColumn("Mode",ImGuiTableColumnFlags_WidthStretch,1);
    ImGui::TableSetupColumn("Shortcut",ImGuiTableColumnFlags_WidthStretch,1.7f);
    ImGui::TableSetupColumn("Hold",ImGuiTableColumnFlags_WidthFixed,44*scale);
    ImGui::TableNextRow(0,32*scale);for(const char* label:{"Mode","Shortcut","Hold"}){nextColumn();ImGui::TextColored(muted,"%s",label);}rememberBounds(710);
    for(size_t row=0;row<=modes.size();++row){
        int action=row==modes.size()?16:int(row);ImGui::PushID(action);ImGui::TableNextRow(0,42*scale);
        nextColumn();ImGui::AlignTextToFramePadding();fittedText(action==16?"Menu":modes[row].name.c_str(),ImGui::GetContentRegionAvail().x,bright);
        nextColumn();std::string label;auto keys=toolscreen::binding(action);
        std::stable_sort(keys.begin(),keys.end(),[](int a,int b){return (a>=224&&a<=231)>(b>=224&&b<=231);});
        for(int scan:keys){if(!label.empty())label+=" + ";label+=toolscreen::scanName(scan);}
        if(label.empty())label="Set binding";
        bool listening=bindingMode==action;std::string fullLabel=label;if(listening)label="Press keys";
        float width=ImGui::GetContentRegionAvail().x-(listening?34*scale:0);
        ImGui::PushStyleColor(ImGuiCol_Button,listening?toolscreen::ui::rgba(0x45413d):toolscreen::ui::rgba(0x282b30));
        if(button(label.c_str(),ImVec2(width,30*scale))){
            if(listening){bindingMode=-1;bindingCollected.clear();bindingPressed.clear();}
            else{bindingMode=action;bindingCollected.clear();bindingPressed.clear();bindingRemapScan=-1;}
        }
        ImGui::PopStyleColor();rememberBounds(200+action);
        if(ImGui::IsItemClicked(1))toolscreen::bindKeys(action,{});
        if(listening){auto point=ImGui::GetItemRectMin();ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(point.x+10*scale,point.y+15*scale),2.5f*scale,color(copper));tooltip("Press and release a key or chord.");ImGui::SameLine(0,4*scale);if(iconButton("Cancel binding",3)){bindingMode=-1;bindingCollected.clear();bindingPressed.clear();}}
        else tooltip(fullLabel=="Set binding"?"Click to bind. Right-click to clear.":fullLabel.c_str());
        nextColumn();if(action<16){float offset=std::max(0.0f,(ImGui::GetContentRegionAvail().x-34*scale)*.5f);ImGui::SetCursorPosX(ImGui::GetCursorPosX()+offset);bool hold=state[96+action]>.5;if(switchButton("hold",hold))toolscreen::set(96+action,hold?1:0);}
        ImGui::PopID();
    }
    ImGui::EndTable();ImGui::PopStyleVar();
}

void bindings(){
    bool wide=ImGui::GetContentRegionAvail().x>660*scale;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(10*scale,wide?0:8*scale));
    if(ImGui::BeginTable("controls",wide?2:1,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoPadOuterX)){
        if(wide){ImGui::TableSetupColumn("Mouse",ImGuiTableColumnFlags_WidthStretch,1);ImGui::TableSetupColumn("Keyboard",ImGuiTableColumnFlags_WidthStretch,1.5f);}nextColumn();
        beginPanel("mouse-settings",ImVec2(0,0),!wide);sensitivityPanel();ImGui::Separator();settingToggle("Gameplay controls",128);tooltip("Shortcuts are inactive in chat and menus.");if(disclosure("Key remapping"))keyboardRemaps();endPanel();nextColumn();
        beginPanel("keyboard-settings",ImVec2(0,0),!wide);bindingTable();endPanel();ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

void appearance(){
    auto state=toolscreen::snapshot();heading("Canvas style");int type=int(state[124]);const char* types[]={"Solid","Gradient","Image"};ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##background-type",&type,types,3))toolscreen::set(124,type);float first[]={float(state[8]),float(state[9]),float(state[10])};if(ImGui::ColorEdit3("Background",first,ImGuiColorEditFlags_NoInputs))for(int i=0;i<3;++i)toolscreen::set(8+i,first[i]);if(type==1){float second[]={float(state[125]),float(state[126]),float(state[127])};if(ImGui::ColorEdit3("Gradient end",second,ImGuiColorEditFlags_NoInputs))for(int i=0;i<3;++i)toolscreen::set(125+i,second[i]);}if(type==2){if(button("Choose image",ImVec2(-1,30*scale)))toolscreen::openImagePicker(true);int fit=int(state[122]);const char* modes[]={"Fill","Fit","Stretch"};ImGui::SetNextItemWidth(-1);if(ImGui::Combo("##fit",&fit,modes,3))toolscreen::set(122,fit);fraction("Opacity",123);}ImGui::Spacing();settingToggle("Viewport border",114);if(state[114]>.5){float thickness=float(state[115]);ImGui::SetNextItemWidth(-1);if(numberField("##border-width",&thickness,.1f,0,12,"%.1f px",ImGuiSliderFlags_AlwaysClamp))toolscreen::set(115,thickness);float tint[]={float(state[116]),float(state[117]),float(state[118])};if(ImGui::ColorEdit3("Border color",tint,ImGuiColorEditFlags_NoInputs))for(int i=0;i<3;++i)toolscreen::set(116+i,tint[i]);}
}

void diagnostics(const std::string& backend,const std::string& error){
    ImGui::Text("Renderer: %s",backend.c_str());ImGui::Text("Minecraft: %s",toolscreen::option("minecraft_version").c_str());if(!error.empty())ImGui::TextWrapped("%s",error.c_str());
}

void connectionBadge(const char* label,bool healthy){
    auto point=ImGui::GetCursorScreenPos();auto tint=healthy?accent:muted;ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(point.x+3*scale,point.y+9*scale),3*scale,color(tint));ImGui::Dummy(ImVec2(10*scale,18*scale));ImGui::SameLine();ImGui::TextColored(tint,"%s",label);
}

void ninjabrainSettings(){
    auto data=toolscreen::ninjaState();bool wide=ImGui::GetContentRegionAvail().x>650*scale;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(12*scale,wide?0:8*scale));
    if(ImGui::BeginTable("ninja-settings",wide?2:1,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoPadOuterX)){
        if(wide){ImGui::TableSetupColumn("Connection",ImGuiTableColumnFlags_WidthStretch,.85f);ImGui::TableSetupColumn("Predictions",ImGuiTableColumnFlags_WidthStretch,1.35f);}nextColumn();beginPanel("ninja-connection",ImVec2(0,0),!wide);heading("Connection");settingToggle("Enabled",134);connectionBadge(data.connected?"Connected":"Offline",data.connected);tooltip(data.status.c_str());char url[1024];std::snprintf(url,sizeof(url),"%s",toolscreen::option("ninja_url").c_str());ImGui::TextColored(muted,"API address");ImGui::SetNextItemWidth(-1);if(ImGui::InputText("##ninja-url",url,sizeof(url)))toolscreen::setOption("ninja_url",url);if(!data.version.empty())ImGui::TextDisabled("Version %s",data.version.c_str());
        endPanel();nextColumn();beginPanel("ninja-predictions",ImVec2(0,0),!wide);heading("Predictions");if(!data.connected||data.predictions.empty())ImGui::TextWrapped("No predictions");for(size_t i=0;i<data.predictions.size();++i){auto& prediction=data.predictions[i];ImGui::PushID(int(i));ImGui::PushFont(headingFont);ImGui::TextWrapped("%.0f, %.0f",prediction.x,prediction.z);ImGui::PopFont();ImGui::TextColored(muted,"%.1f%%",prediction.certainty*100);ImGui::SameLine();if(smallButton("Copy")){char coordinates[96];std::snprintf(coordinates,sizeof(coordinates),"%.0f %.0f",prediction.x,prediction.z);ImGui::SetClipboardText(coordinates);status="Copied";}ImGui::TextDisabled("%.0f blocks",prediction.distance);ImGui::Spacing();ImGui::PopID();}if(disclosure("Measurements")){if(data.eyes.empty())ImGui::TextDisabled("No measurements");for(auto& eye:data.eyes){ImGui::Text("Yaw %.8f°",eye.angle);ImGui::TextDisabled("Correction %+.6f",eye.correction);ImGui::TextDisabled("Error %.6f",eye.error);}if(!data.boatState.empty())ImGui::TextWrapped("Boat: %s",data.boatState.c_str());}if(data.blind){section("Blind travel");ImGui::TextWrapped("%.1f%% within %.0f blocks",data.blindChance*100,data.blindThreshold);}if(data.divine){section("Divine travel");ImGui::TextWrapped("%s",data.divineSafe.c_str());}for(auto& message:data.messages)ImGui::TextWrapped("%s",message.second.c_str());endPanel();ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

bool modeRow(const char* label,bool selected){
    auto point=ImGui::GetCursorScreenPos();float width=ImGui::GetContentRegionAvail().x,wrap=std::max(1.0f,width-24*scale);auto font=ImGui::GetFont();float fontSize=ImGui::GetFontSize();auto text=font->CalcTextSizeA(fontSize,FLT_MAX,wrap,label);float height=std::max(38*scale,text.y+20*scale);
    ImGui::PushID(label);bool pressed=ImGui::InvisibleButton("mode-row",ImVec2(width,height),ImGuiButtonFlags_EnableNav);
    auto draw=ImGui::GetWindowDrawList();if(selected||ImGui::IsItemHovered()||ImGui::IsItemFocused())draw->AddRectFilled(point,ImVec2(point.x+width,point.y+height),selected?IM_COL32(48,51,56,255):IM_COL32(40,42,46,255),6*scale);
    draw->AddText(font,fontSize,ImVec2(std::round(point.x+12*scale),std::round(point.y+(height-text.y)*.5f)),color(selected?bright:muted),label,nullptr,wrap);
    if(ImGui::IsItemFocused())draw->AddRect(point,ImVec2(point.x+width,point.y+height),color(accent),6*scale);
    ImGui::PopID();return pressed;
}

void interfaceScale(){
    double value=toolscreen::snapshot()[34];ImGui::TextColored(muted,"Interface scale");ImGui::SetNextItemWidth(-1);
    if(numericInput("##interface-scale",value,.75,2,"%.3f"))toolscreen::set(34,value);rememberBounds(34);tooltip("Type a value from 0.75 to 2. Press Enter to apply.");
}

void modesSettings(){
    auto modes=toolscreen::modes();settingsMode=std::clamp(settingsMode,0,int(modes.size())-1);auto state=toolscreen::snapshot();bool wide=ImGui::GetContentRegionAvail().x>600*scale;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(12*scale,wide?0:8*scale));
    if(ImGui::BeginTable("mode-settings",wide?2:1,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoPadOuterX)){
        if(wide){ImGui::TableSetupColumn("Modes",ImGuiTableColumnFlags_WidthFixed,210*scale);ImGui::TableSetupColumn("Settings",ImGuiTableColumnFlags_WidthStretch);}nextColumn();beginPanel("mode-list",ImVec2(0,0),!wide);heading("Resizing modes");ImGui::Spacing();
        bool grid=!wide;bool table=grid&&ImGui::BeginTable("mode-grid",ImGui::GetContentRegionAvail().x>=360*scale?3:2,ImGuiTableFlags_SizingStretchSame);for(size_t i=0;i<modes.size();++i){if(table)nextColumn();ImGui::PushID(int(i));if(modeRow(modes[i].name.c_str(),settingsMode==int(i)))settingsMode=int(i);rememberBounds(800+int(i));ImGui::PopID();}if(table)ImGui::EndTable();
        ImGui::Spacing();if(button("Add mode",ImVec2(-1,30*scale))){int added=toolscreen::addMode("Custom mode");if(added>=0)settingsMode=added;}endPanel();nextColumn();beginPanel("mode-details",ImVec2(0,0),!wide);modeInspector(settingsMode);ImGui::Spacing();
        bool active=int(state[0])==settingsMode;ImGui::BeginDisabled(active);if(button(active?"Active":"Activate",ImVec2(-1,34*scale))&&!toolscreen::set(0,settingsMode))status=toolscreen::resolutionError();ImGui::EndDisabled();
        if(settingsMode==3||settingsMode==4)ImGui::TextWrapped("EyeZoom and Preemptive share their source resolution.");
        if(settingsMode>=5&&button("Remove mode",ImVec2(-1,30*scale))){toolscreen::removeMode(settingsMode);settingsMode=std::max(0,settingsMode-1);}endPanel();ImGui::EndTable();
    }
    ImGui::PopStyleVar();
}

void selectOverlay(int type){auto list=toolscreen::mirrors();for(size_t i=0;i<list.size();++i)if(int(list[i].values[33])==type){toolscreen::selectMirror(int(i));layoutTarget=LayoutTarget::Mirror;return;}const char* names[]={"E Counter","Profiler","Mapless"};toolscreen::addMirror(names[type-1],type);layoutTarget=LayoutTarget::Mirror;}

void addLayerMenu(){
    bool browser=false,window=false;if(iconButton("Add layer",4))ImGui::OpenPopup("add-layer");rememberBounds(9200);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(18*scale,16*scale));ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(0,5*scale));
    if(beginPopup("add-layer",0,256*scale,320*scale)){auto item=[&](const char* label,int id){bool selected=selectable(label,false,0,ImVec2(ImGui::GetContentRegionAvail().x,40*scale));rememberBounds(id);return selected;};const char* names[]={"E-counter","Profiler","Mapless"};for(int i=0;i<3;++i)if(item(names[i],9201+i)){toolscreen::addMirror(names[i],i+1);layoutTarget=LayoutTarget::Mirror;}ImGui::Spacing();ImGui::Separator();ImGui::Spacing();if(item("Image",9204)){selectNextImage=true;toolscreen::openImagePicker();}browser=item("Browser",9205);if(item("Window",9206)){toolscreen::refreshWindows();window=true;}endPopup();}
    ImGui::PopStyleVar(2);
    if(browser)ImGui::OpenPopup("browser-source");if(window)ImGui::OpenPopup("window-source");
    if(beginPopup("browser-source")){static char url[2048]="https://";ImGui::TextUnformatted("Browser URL");ImGui::SetNextItemWidth(std::min(360*scale,float(displayWidth)-80));ImGui::InputText("##url",url,sizeof(url));
#if !defined(__APPLE__) || defined(MACLESS_PORTABLE_DESKTOP)
        if(disclosure("Browser executable")){char executable[2048];std::snprintf(executable,sizeof(executable),"%s",toolscreen::option("browser_executable").c_str());ImGui::SetNextItemWidth(-1);if(ImGui::InputTextWithHint("##browser-executable","Auto-detect Chrome, Edge or Chromium",executable,sizeof(executable)))toolscreen::setOption("browser_executable",executable);}
#endif
        if(button("Add browser")){selectedImage=toolscreen::addImage(std::string("browser:")+url);layoutTarget=LayoutTarget::Source;ImGui::CloseCurrentPopup();}endPopup();}
    if(beginPopup("window-source")){if(!toolscreen::captureAccess()){ImGui::TextUnformatted("Screen recording access required");if(button("Allow capture"))toolscreen::requestCaptureAccess();}if(button("Refresh"))toolscreen::refreshWindows();auto message=toolscreen::captureStatus();if(!message.empty())ImGui::TextWrapped("%s",message.c_str());for(auto& source:toolscreen::windowSources()){std::string label=source.application+" / "+source.title;if(selectable(label.c_str())){selectedImage=toolscreen::addImage("window:"+std::to_string(source.id)+"|"+source.application+"|"+source.title);toolscreen::renameImage(selectedImage,source.title);layoutTarget=LayoutTarget::Source;ImGui::CloseCurrentPopup();}}endPopup();}
}

void nudgeSelectedObject(){
    if(selectedPage!=0||!canvasEditable||!canvasNavigationFocus||ImGui::GetIO().WantTextInput||ImGui::IsAnyItemActive()||ImGui::GetCurrentContext()->OpenPopupStack.Size)return;double dx=0,dy=0,step=ImGui::GetIO().KeyShift?10:1;if(ImGui::IsKeyPressed(ImGuiKey_LeftArrow))dx=-step/displayWidth;if(ImGui::IsKeyPressed(ImGuiKey_RightArrow))dx=step/displayWidth;if(ImGui::IsKeyPressed(ImGuiKey_UpArrow))dy=-step/displayHeight;if(ImGui::IsKeyPressed(ImGuiKey_DownArrow))dy=step/displayHeight;if(dx==0&&dy==0)return;
    toolscreen::beginEdit();auto state=toolscreen::snapshot();if(layoutTarget==LayoutTarget::Projector){if(state[35]>.5){auto view=toolscreen::viewport(displayWidth,displayHeight,16384,false);auto eye=eyeLayer(toolscreen::planForOutput(displayWidth,displayHeight,view[0],view[1],false,0,previewRoute));if(eye[18]>0){state[14]=eye[4];state[15]=1-eye[5]-eye[7];state[16]=eye[6];state[17]=eye[7];for(int i=0;i<4;++i)toolscreen::set(14+i,state[14+i]);}}toolscreen::set(35,0);toolscreen::set(14,state[14]+dx);toolscreen::set(15,state[15]+dy);}else if(layoutTarget==LayoutTarget::Mirror){auto mirrors=toolscreen::mirrors();int index=toolscreen::selectedMirror();if(index>=0&&index<int(mirrors.size())){editMirrorValue(index,5,toolscreen::mirrorForOutput(mirrors[size_t(index)],previewRoute).values[5]+dx);editMirrorValue(index,6,toolscreen::mirrorForOutput(mirrors[size_t(index)],previewRoute).values[6]+dy);}}else if(layoutTarget==LayoutTarget::Source){for(auto& source:toolscreen::images())if(source.id==selectedImage){toolscreen::editImage(source.id,5,source.values[5]+dx);toolscreen::editImage(source.id,6,source.values[6]+dy);}}
    toolscreen::endEdit();
}

void studio(){
    auto sources=toolscreen::images();if(previousSourceCount>=0&&int(sources.size())>previousSourceCount&&selectNextImage){selectedImage=sources.back().id;layoutTarget=LayoutTarget::Source;selectNextImage=false;}previousSourceCount=int(sources.size());
    auto available=ImGui::GetContentRegionAvail();float inspectorWidth=std::clamp(available.x*.31f,224*scale,280*scale);bool wide=available.x>=inspectorWidth+384*scale;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,ImVec2(12*scale,0));
    if(ImGui::BeginTable("layout",wide?2:1,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoPadOuterX)){
        if(wide){ImGui::TableSetupColumn("Canvas",ImGuiTableColumnFlags_WidthStretch,1);ImGui::TableSetupColumn("Inspector",ImGuiTableColumnFlags_WidthFixed,inspectorWidth);}nextColumn();
        float previewHeight=wide?0:std::min(available.y,std::clamp(available.y*.6f,280*scale,400*scale));beginPanel("canvas",ImVec2(0,previewHeight));chooseMode();
        bool wrapToolbar=ImGui::GetContentRegionAvail().x<394*scale;float toolbarHeight=wrapToolbar?62*scale+ImGui::GetStyle().ItemSpacing.y:32*scale;compositionCanvas(std::max(64*scale,ImGui::GetContentRegionAvail().y-toolbarHeight-ImGui::GetStyle().ItemSpacing.y-6*scale));
        for(int type:{3,2,1}){if(type!=3)ImGui::SameLine(0,4*scale);auto list=toolscreen::mirrors();int selected=toolscreen::selectedMirror();bool active=layoutTarget==LayoutTarget::Mirror&&selected>=0&&selected<int(list.size())&&int(list[size_t(selected)].values[33])==type;if(pill(type==3?"Mapless":type==2?"Profiler":"E",active,(type==1?30:76)*scale))selectOverlay(type);}if(!wrapToolbar)ImGui::SameLine(0,10*scale);layerMenu();ImGui::SameLine(0,2*scale);addLayerMenu();ImGui::SameLine(0,2*scale);if(iconButton("Canvas style",6))layoutTarget=LayoutTarget::Appearance;ImGui::SameLine(0,2*scale);if(iconButton(canvasEditable?"Lock layout":"Unlock layout",canvasEditable?8:7))canvasEditable=!canvasEditable;rememberBounds(402);endPanel();nextColumn();
        if(!wide)ImGui::Spacing();
        beginPanel("inspector",ImVec2(0,0),!wide);
        if(layoutTarget==LayoutTarget::Game)modeInspector();else if(layoutTarget==LayoutTarget::Projector)projectorInspector();else if(layoutTarget==LayoutTarget::Mirror)mirrorInspector();else if(layoutTarget==LayoutTarget::Source)imageInspector();else appearance();endPanel();ImGui::EndTable();
    }
    ImGui::PopStyleVar();nudgeSelectedObject();
}

bool flushSettings(){
    if(!toolscreen::canSave()||!toolscreen::dirty())return true;bool saved=toolscreen::save();saveFailed=!saved;if(!saved)status="Could not save settings";else if(status=="Could not save settings")status.clear();return saved;
}

void autosave(){
    double now=ImGui::GetTime();uint64_t current=toolscreen::revision();if(current!=lastEditRevision){lastEditRevision=current;nextAutosave=now+.45;}if(toolscreen::dirty()&&toolscreen::canSave()&&now>=nextAutosave&&!ImGui::IsAnyItemActive()&&!drag.active&&bindingMode<0&&bindingRemapScan==-1){flushSettings();nextAutosave=now+2;}
}

void windowSizeFields(){
    if(!disclosure("Window size"))return;auto window=ImGui::FindWindowByName("Macless");if(!window)return;
    float inset=windowInset();double width=window->Size.x,height=window->Size.y;double maximumWidth=displayWidth-2*inset,maximumHeight=displayHeight-2*inset;
    ImGui::TextColored(muted,"Width");ImGui::SetNextItemWidth(-1);if(numericInput("##window-width",width,std::min(720.0*scale,maximumWidth),maximumWidth,"%.0f")){pendingWindowSize=ImVec2(float(width),window->Size.y);pendingWindowResize=true;}rememberBounds(9001);
    ImGui::TextColored(muted,"Height");ImGui::SetNextItemWidth(-1);if(numericInput("##window-height",height,std::min(420.0*scale,maximumHeight),maximumHeight,"%.0f")){pendingWindowSize=ImVec2(window->Size.x,float(height));pendingWindowResize=true;}rememberBounds(9002);
}

void settingsPopup(const std::string& backend,const std::string& error){
    if(settingsRequested){ImGui::OpenPopup("settings");settingsRequested=false;}ImGui::SetNextWindowPos(ImVec2(displayWidth*.5f,displayHeight*.5f),ImGuiCond_Appearing,ImVec2(.5f,.5f));if(beginPopup("settings",0,280*scale,340*scale)){heading("Settings");if(displayWidth<600*scale){ImGui::BeginDisabled(!toolscreen::canUndo());if(button("Undo"))toolscreen::undo();ImGui::EndDisabled();ImGui::SameLine();ImGui::BeginDisabled(!toolscreen::canRedo());if(button("Redo"))toolscreen::redo();ImGui::EndDisabled();}interfaceScale();windowSizeFields();if(button("Center window",ImVec2(-1,30*scale))){auto window=ImGui::FindWindowByName("Macless");if(window){pendingWindowPosition=ImVec2((displayWidth-window->Size.x)*.5f,(displayHeight-window->Size.y)*.5f);pendingWindowMove=true;}ImGui::CloseCurrentPopup();}if(disclosure("Diagnostics"))diagnostics(backend,error);if(saveFailed){ImGui::TextWrapped("Could not save settings");if(button("Retry save"))flushSettings();}ImGui::Spacing();if(button("Reset settings",ImVec2(-1,30*scale)))ImGui::OpenPopup("reset");if(beginPopup("reset")){ImGui::TextUnformatted("Reset all settings?");if(button("Reset")){toolscreen::reset();layoutTarget=LayoutTarget::Game;ImGui::ClosePopupToLevel(0,true);}ImGui::SameLine();if(button("Cancel"))ImGui::CloseCurrentPopup();endPopup();}endPopup();}
}

void commandPalette(){
    static char query[128]="";if(commandPaletteRequested){query[0]=0;ImGui::OpenPopup("Find a control");commandPaletteRequested=false;}ImGui::SetNextWindowSize(ImVec2(std::min(480*scale,float(displayWidth)-32),0),ImGuiCond_Always);ImGui::SetNextWindowPos(ImVec2(displayWidth*.5f,displayHeight*.5f),ImGuiCond_Appearing,ImVec2(.5f,.5f));
    if(beginPopup("Find a control")){if(ImGui::IsWindowAppearing())ImGui::SetKeyboardFocusHere();ImGui::SetNextItemWidth(-1);bool enter=ImGui::InputTextWithHint("##find","Find a control",query,sizeof(query),ImGuiInputTextFlags_EnterReturnsTrue);rememberBounds(300);std::string needle=query;std::transform(needle.begin(),needle.end(),needle.begin(),[](unsigned char c){return char(std::tolower(c));});const char* names[]={"Layout","Controls and sensitivity","Modes","Ninjabrain","Mapless overlay","Profiler overlay","Image and browser layers","Canvas style","Fullscreen","Thin","Wide","EyeZoom","Preemptive","Undo","Redo","Settings"};int first=-1;bool openSettings=false;
        for(int i=0;i<16;++i){std::string candidate=names[i];std::transform(candidate.begin(),candidate.end(),candidate.begin(),[](unsigned char c){return char(std::tolower(c));});if(!needle.empty()&&candidate.find(needle)==std::string::npos)continue;if(first<0)first=i;if(selectable(names[i],false,0,ImVec2(0,28*scale))||(enter&&i==first)){if(i<4)selectedPage=i;else if(i==4||i==5){selectedPage=0;selectOverlay(i==4?3:2);}else if(i==6){selectedPage=0;auto sources=toolscreen::images();if(!sources.empty()){selectedImage=sources.front().id;layoutTarget=LayoutTarget::Source;}}else if(i==7){selectedPage=0;layoutTarget=LayoutTarget::Appearance;}else if(i<13){toolscreen::set(0,i-8);selectedPage=0;layoutTarget=i==11?LayoutTarget::Projector:LayoutTarget::Game;}else if(i==13)toolscreen::undo();else if(i==14)toolscreen::redo();else openSettings=true;ImGui::CloseCurrentPopup();}}if(first<0)ImGui::TextDisabled("No matches");endPopup();if(openSettings)settingsRequested=true;}
}

struct Vertex{float x,y,z,u,v;uint32_t color;};
struct Command{uint32_t first,count;float x,y,width,height;uint32_t texture,reserved;};
static_assert(sizeof(Vertex)==24);static_assert(sizeof(Command)==32);

std::vector<uint8_t> serialize(){
    auto data=ImGui::GetDrawData();std::vector<Vertex> vertices;std::vector<Command> commands;vertices.reserve(size_t(data->TotalIdxCount));
    for(int listIndex=0;listIndex<data->CmdListsCount;++listIndex){const auto list=data->CmdLists[listIndex];for(auto& draw:list->CmdBuffer){if(draw.UserCallback)continue;uint32_t first=uint32_t(vertices.size());for(unsigned i=0;i<draw.ElemCount;++i){const auto& v=list->VtxBuffer[int(draw.VtxOffset+list->IdxBuffer[int(draw.IdxOffset+i)])];vertices.push_back({v.pos.x,v.pos.y,0,v.uv.x,v.uv.y,v.col});}commands.push_back({first,draw.ElemCount,draw.ClipRect.x,draw.ClipRect.y,draw.ClipRect.z-draw.ClipRect.x,draw.ClipRect.w-draw.ClipRect.y,uint32_t(draw.GetTexID()),0});}}
    std::vector<uint8_t> output(8+vertices.size()*sizeof(Vertex)+commands.size()*sizeof(Command));uint32_t header[]={uint32_t(vertices.size()),uint32_t(commands.size())};std::memcpy(output.data(),header,8);if(!vertices.empty())std::memcpy(output.data()+8,vertices.data(),vertices.size()*sizeof(Vertex));if(!commands.empty())std::memcpy(output.data()+8+vertices.size()*sizeof(Vertex),commands.data(),commands.size()*sizeof(Command));return output;
}
}

namespace toolscreen {
std::vector<uint8_t> uiFrame(int width,int height,float delta,const std::string& backend,const std::string& error){
    std::lock_guard lock(uiMutex);ensureContext();displayWidth=width;displayHeight=height;auto state=snapshot();scale=float(state[34])*displayScale;if(std::abs(fontRasterScale-scale)>.001f)rebuildFonts(scale);auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(float(std::max(1,width)),float(std::max(1,height)));io.DeltaTime=std::clamp(delta,.001f,.2f);io.FontGlobalScale=1;ImGui::GetStyle()=baseStyle;ImGui::GetStyle().ScaleAllSizes(scale);ImGui::NewFrame();controlBounds.clear();if(drag.active&&selectedPage!=0)finishDrag();
    float inset=windowInset();auto maximum=ImVec2(std::max(1.0f,float(width)-2*inset),std::max(1.0f,float(height)-2*inset));auto minimum=ImVec2(std::min(720*scale,maximum.x),std::min(420*scale,maximum.y));ImGui::SetNextWindowSizeConstraints(minimum,maximum);ImGui::SetNextWindowSize(ImVec2(std::min(900*scale,maximum.x),std::min(500*scale,maximum.y)),ImGuiCond_FirstUseEver);ImGui::SetNextWindowPos(ImVec2(width*.5f,height*.5f),ImGuiCond_FirstUseEver,ImVec2(.5f,.5f));
    if(windowLayout.read(configurationDirectory())){auto placement=windowLayout.restore();ImVec2 size(std::clamp(placement.width*scale,minimum.x,maximum.x),std::clamp(placement.height*scale,minimum.y,maximum.y));ImGui::SetNextWindowSize(size,ImGuiCond_Always);ImGui::SetNextWindowPos(fitWindowPosition(ImVec2(width*placement.centerX-size.x*.5f,height*placement.centerY-size.y*.5f),size),ImGuiCond_Always);}
    if(pendingWindowResize){ImGui::SetNextWindowSize(pendingWindowSize,ImGuiCond_Always);auto current=ImGui::FindWindowByName("Macless");if(current)ImGui::SetNextWindowPos(fitWindowPosition(current->Pos,pendingWindowSize),ImGuiCond_Always);pendingWindowResize=false;}
    if(pendingWindowMove){ImGui::SetNextWindowPos(pendingWindowPosition,ImGuiCond_Always);pendingWindowMove=false;}auto previous=ImGui::FindWindowByName("Macless");if(previous&&lastDisplay.x>0&&(lastDisplay.x!=width||lastDisplay.y!=height||std::abs(lastWindowScale-scale)>.001f)){float factor=scale/std::max(lastWindowScale,.1f);ImVec2 size(std::clamp(previous->Size.x*factor,minimum.x,maximum.x),std::clamp(previous->Size.y*factor,minimum.y,maximum.y));ImVec2 center(previous->Pos.x+previous->Size.x*.5f,previous->Pos.y+previous->Size.y*.5f);ImGui::SetNextWindowSize(size,ImGuiCond_Always);ImGui::SetNextWindowPos(fitWindowPosition(ImVec2(center.x-size.x*.5f,center.y-size.y*.5f),size),ImGuiCond_Always);}lastDisplay=ImVec2(float(width),float(height));lastWindowScale=scale;
    ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0,0),io.DisplaySize,IM_COL32(0,0,0,232));ImGui::Begin("Macless",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoScrollbar);auto size=ImGui::GetWindowSize(),position=ImGui::GetWindowPos();controlBounds[501]=ImVec4(position.x,position.y,position.x+size.x,position.y+size.y);auto draw=ImGui::GetWindowDrawList();toolscreen::ui::drawShadow(ImGui::GetBackgroundDrawList(),position,size,scale);draw->AddLine(ImVec2(position.x+14*scale,position.y+51*scale),ImVec2(position.x+size.x-14*scale,position.y+51*scale),IM_COL32(52,55,60,200));
    bool compactHeader=size.x<600*scale,showBrand=size.x>850*scale;float actionWidth=(compactHeader?64:132)*scale,tabWidth=std::min(476*scale,size.x-actionWidth-32*scale),tabLeft=showBrand?std::max(150*scale,(size.x-tabWidth)*.5f):16*scale;
    if(showBrand){windowDragZone("move-window",16*scale,118*scale,position,size);draw->AddText(brandFont,brandFont->FontSize,ImVec2(position.x+16*scale,position.y+14*scale),color(bright),"Macless");windowDragZone("move-left",134*scale,std::max(0.0f,tabLeft-142*scale),position,size);}
    windowDragZone("move-right",tabLeft+tabWidth+8*scale,std::max(0.0f,size.x-actionWidth-20*scale-tabLeft-tabWidth-8*scale),position,size);
    ImGui::SetCursorPos(ImVec2(tabLeft,8*scale));pageTab("Layout",0,tabWidth/4);rememberBounds(-1);ImGui::SameLine(0,0);pageTab(compactHeader?"Input":"Controls",1,tabWidth/4);rememberBounds(-2);ImGui::SameLine(0,0);pageTab("Modes",2,tabWidth/4);rememberBounds(-3);ImGui::SameLine(0,0);pageTab(compactHeader?"Ninja":"Ninjabrain",3,tabWidth/4);rememberBounds(-4);
    ImGui::SetCursorPos(ImVec2(size.x-actionWidth-12*scale,10*scale));if(!compactHeader){if(iconButton("Undo / Ctrl+Z",0,canUndo()))undo();ImGui::SameLine(0,4*scale);if(iconButton("Redo / Ctrl+Shift+Z",1,canRedo()))redo();ImGui::SameLine(0,4*scale);}if(iconButton("Settings",2))settingsRequested=true;rememberBounds(500);if(saveFailed)draw->AddCircleFilled(ImVec2(ImGui::GetItemRectMax().x-3*scale,ImGui::GetItemRectMin().y+3*scale),3*scale,color(copper));ImGui::SameLine(0,4*scale);if(iconButton("Close / Esc",3)){if(flushSettings())closeRequested=true;}
    ImGui::SetCursorPos(ImVec2(20*scale,72*scale));ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(2*scale,4*scale));ImGui::BeginChild("content",ImVec2(size.x-40*scale,std::max(1.0f,size.y-96*scale)),ImGuiChildFlags_AlwaysUseWindowPadding);if(selectedPage==0)studio();else if(selectedPage==1)bindings();else if(selectedPage==2)modesSettings();else ninjabrainSettings();ImGui::EndChild();ImGui::PopStyleVar();settingsPopup(backend,error);commandPalette();autosave();
    if(status!=previousStatus){previousStatus=status;statusStarted=ImGui::GetTime();}if(!status.empty()&&(saveFailed||ImGui::GetTime()-statusStarted<2.5)){float wrap=std::max(1.0f,size.x-64*scale);auto extent=ImGui::CalcTextSize(status.c_str(),nullptr,false,wrap);ImVec2 point(position.x+16*scale,position.y+size.y-extent.y-24*scale);draw->AddRectFilled(point,ImVec2(point.x+extent.x+24*scale,point.y+extent.y+16*scale),IM_COL32(43,46,51,245),5*scale);draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),ImVec2(std::round(point.x+12*scale),std::round(point.y+8*scale)),color(saveFailed?copper:bright),status.c_str(),nullptr,wrap);}windowLayout.observe(position,size,io.DisplaySize,scale,ImGui::GetTime(),ImGui::IsMouseDown(0)||pendingWindowMove);ImGui::End();ImGui::Render();return serialize();
}

std::vector<uint8_t> overlayFrame(int width,int height,int viewWidth,int viewHeight,int route){
    std::lock_guard lock(uiMutex);ensureContext();if(!overlayContext){overlayContext=ImGui::CreateContext(ImGui::GetIO().Fonts);ImGui::GetIO().IniFilename=nullptr;ImGui::GetIO().LogFilename=nullptr;}ImGui::SetCurrentContext(overlayContext);auto state=snapshot();auto& io=ImGui::GetIO();io.DisplaySize=ImVec2(float(width),float(height));io.DeltaTime=1.0f/60;io.FontGlobalScale=1;
    ImGui::NewFrame();if(int(state[0])==3&&state[11]>.5){auto layers=planForOutput(width,height,viewWidth,viewHeight,false,0,route);auto layer=eyeLayer(layers);if(layer[18]>0){Bounds box{layer[4]*width,(1-layer[5]-layer[7])*height,layer[6]*width,layer[7]*height};drawGrid(ImGui::GetBackgroundDrawList(),box,layer,state);}}drawRuntimeOverlays(ImGui::GetBackgroundDrawList(),ImVec2(0,0),ImVec2(float(width),float(height)),route,bodyFont,dataFont,gridFont);
    ImGui::Render();auto result=serialize();ImGui::SetCurrentContext(context);return result;
}
uint64_t fontRevision(){std::lock_guard lock(uiMutex);ensureContext();return fontGeneration;}
std::vector<uint8_t> fontAtlas(){std::lock_guard lock(uiMutex);ensureContext();unsigned char* pixels;int width,height;ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);std::vector<uint8_t> result(size_t(width)*height*4+8);std::memcpy(result.data(),&width,4);std::memcpy(result.data()+4,&height,4);std::memcpy(result.data()+8,pixels,size_t(width)*height*4);return result;}
void uiDisplayScale(float value){std::lock_guard lock(uiMutex);displayScale=std::clamp(value,.5f,4.0f);}
void uiMouse(float x,float y){std::lock_guard lock(uiMutex);ensureContext();ImGui::GetIO().AddMousePosEvent(x,y);}
void uiButton(int button,bool down){std::lock_guard lock(uiMutex);ensureContext();if(button>=0&&button<5)ImGui::GetIO().AddMouseButtonEvent(button,down);}
void uiScroll(float x,float y){std::lock_guard lock(uiMutex);ensureContext();ImGui::GetIO().AddMouseWheelEvent(x,y);}
void uiKey(int keycode,int modifiers,bool down){
    std::lock_guard lock(uiMutex);ensureContext();auto& io=ImGui::GetIO();
    if(bindingMode>=0||bindingRemapScan!=-1)return;
    if(down&&drag.active&&keycode==27){bool restore=drag.committed;finishDrag();if(restore)undo();return;}
    if(down&&drag.active&&(modifiers&3264))finishDrag();
    io.AddKeyEvent(ImGuiMod_Shift,(modifiers&3)!=0);io.AddKeyEvent(ImGuiMod_Ctrl,(modifiers&192)!=0);io.AddKeyEvent(ImGuiMod_Alt,(modifiers&768)!=0);io.AddKeyEvent(ImGuiMod_Super,(modifiers&3072)!=0);
    if(down&&(modifiers&3072||modifiers&192)&&!io.WantTextInput){if(keycode=='k')commandPaletteRequested=true;if(keycode=='z'){if(modifiers&3)redo();else undo();}if(keycode=='y')redo();if(keycode=='s')save();if(keycode>='1'&&keycode<='4')selectedPage=keycode-'1';if(keycode=='5'){selectedPage=0;layoutTarget=LayoutTarget::Appearance;}if(keycode=='6')settingsRequested=true;if(keycode=='7')selectedPage=3;}
    ImGuiKey key=ImGuiKey_None;if(keycode>='a'&&keycode<='z')key=ImGuiKey(ImGuiKey_A+keycode-'a');else if(keycode>='0'&&keycode<='9')key=ImGuiKey(ImGuiKey_0+keycode-'0');
    else switch(keycode){case 8:key=ImGuiKey_Backspace;break;case 9:key=ImGuiKey_Tab;break;case 13:key=ImGuiKey_Enter;break;case 27:key=ImGuiKey_Escape;break;case 32:key=ImGuiKey_Space;break;case 127:key=ImGuiKey_Delete;break;case 1073741897:key=ImGuiKey_Insert;break;case 1073741898:key=ImGuiKey_Home;break;case 1073741899:key=ImGuiKey_PageUp;break;case 1073741901:key=ImGuiKey_End;break;case 1073741902:key=ImGuiKey_PageDown;break;case 1073741903:key=ImGuiKey_RightArrow;break;case 1073741904:key=ImGuiKey_LeftArrow;break;case 1073741905:key=ImGuiKey_DownArrow;break;case 1073741906:key=ImGuiKey_UpArrow;break;default:break;}
    if(key!=ImGuiKey_None)io.AddKeyEvent(key,down);
}
void uiText(uint32_t codepoint){std::lock_guard lock(uiMutex);ensureContext();ImGui::GetIO().AddInputCharacter(codepoint);}
void uiFocus(bool focused){std::lock_guard lock(uiMutex);ensureContext();ImGui::GetIO().AddFocusEvent(focused);closeRequested=false;if(!focused){flushSettings();if(ImGui::GetCurrentContext()->OpenPopupStack.Size)ImGui::ClosePopupToLevel(0,false);ImGui::ClearActiveID();ImGui::GetIO().WantTextInput=false;ImGui::GetIO().ClearInputKeys();ImGui::GetIO().ClearInputMouse();finishDrag();bindingMode=-1;bindingRemapScan=-1;bindingCollected.clear();bindingPressed.clear();}}
bool uiCloseRequested(){std::lock_guard lock(uiMutex);return closeRequested;}
bool uiEscapeActive(){std::lock_guard lock(uiMutex);ensureContext();return drag.active||bindingMode>=0||bindingRemapScan!=-1||ImGui::GetCurrentContext()->OpenPopupStack.Size>0||ImGui::GetIO().WantTextInput;}
bool uiWantsText(){std::lock_guard lock(uiMutex);ensureContext();return ImGui::GetIO().WantTextInput;}
bool uiBindingActive(){std::lock_guard lock(uiMutex);return bindingMode>=0||bindingRemapScan!=-1;}
bool uiPhysicalKey(int scan,int keycode,int modifiers,bool down){std::lock_guard lock(uiMutex);ensureContext();if(scan<=0||scan>511)return false;if(bindingMode>=0){for(int key=224;key<=231;++key)bindingPressed.erase(key);for(int key:modifierScans(modifiers)){bindingPressed.insert(key);if(bindingCollected.size()<8)bindingCollected.insert(key);}if(down){bindingPressed.insert(scan);if(bindingCollected.size()<8)bindingCollected.insert(scan);}else bindingPressed.erase(scan);if(!down&&bindingPressed.empty()&&!bindingCollected.empty()){bindKeys(bindingMode,std::vector<int>(bindingCollected.begin(),bindingCollected.end()));bindingMode=-1;bindingCollected.clear();}return true;}if(bindingRemapScan!=-1){if(down){if(bindingRemapScan==-2){bindingRemapScan=scan;bindingRemapKey=keycode;}else{bindRemap({bindingRemapScan,bindingRemapKey,scan,keycode});bindingRemapScan=-1;}}return true;}return false;}
void uiAssetStatus(int id,int width,int height,const std::string& error,int canvasWidth,int canvasHeight){std::lock_guard lock(uiMutex);bool first=!assetStatuses.contains(id)||assetStatuses[id].width<=0;assetStatuses[id]={width,height,error};if(width>0&&height>0)imageSize(id,width,height);if(first&&width>0&&height>0){auto sources=images();for(auto& image:sources)if(image.id==id&&std::abs(image.values[7]-.3)<.0001&&std::abs(image.values[8]-.3)<.0001){double ratio=double(height)/width*std::max(canvasWidth,1)/std::max(canvasHeight,1);editImage(id,8,std::clamp(image.values[7]*ratio,.01,1.0));}}}
}

extern "C" {
int ts_ui_physical_key(int scan,int keycode,int modifiers,int down){return toolscreen::uiPhysicalKey(scan,keycode,modifiers,down!=0);}
int ts_overlay_frame(int width,int height,int viewWidth,int viewHeight,int route,uint8_t* output,int capacity){auto data=toolscreen::overlayFrame(width,height,viewWidth,viewHeight,route);if(!output||capacity<int(data.size()))return -int(data.size());std::memcpy(output,data.data(),data.size());return int(data.size());}
int ts_overlay_extent(int index,int route,int width,int height,float* output){std::lock_guard lock(uiMutex);ensureContext();auto mirrors=toolscreen::mirrors();if(index<0||index>=int(mirrors.size())||!output)return 0;auto mirror=toolscreen::mirrorForOutput(mirrors[size_t(index)],route);auto extent=toolscreen::runtimeMirrorExtent(mirror,ImVec2(float(width),float(height)),toolscreen::debugState());auto point=toolscreen::runtimeMirrorPosition(mirror.values,ImVec2(float(width),float(height)),extent);output[0]=point.x;output[1]=point.y;output[2]=extent.x;output[3]=extent.y;return 4;}

void ts_ui_text(unsigned int character){toolscreen::uiText(character);}
void ts_ui_focus(int focused){toolscreen::uiFocus(focused!=0);}
int ts_ui_escape_active(){return toolscreen::uiEscapeActive();}
int ts_ui_wants_text(){std::lock_guard lock(uiMutex);ensureContext();return ImGui::GetIO().WantTextInput?1:0;}
int ts_ui_frame(int width,int height,float delta,uint8_t* output,int capacity){auto data=toolscreen::uiFrame(width,height,delta,"Test renderer","");if(!output||capacity<int(data.size()))return -int(data.size());std::memcpy(output,data.data(),data.size());return int(data.size());}
void ts_ui_display_scale(float value){toolscreen::uiDisplayScale(value);}
void ts_ui_mouse(float x,float y){toolscreen::uiMouse(x,y);}
void ts_ui_scroll(float x,float y){toolscreen::uiScroll(x,y);}
void ts_ui_button(int button,int down){toolscreen::uiButton(button,down!=0);}
void ts_ui_key(int keycode,int modifiers,int down){toolscreen::uiKey(keycode,modifiers,down!=0);}
int ts_ui_probe(int control,float* output){std::lock_guard lock(uiMutex);ensureContext();auto found=controlBounds.find(control);for(int i=0;i<4;++i)output[i]=found==controlBounds.end()?0:(&found->second.x)[i];auto& io=ImGui::GetIO();output[4]=io.MousePos.x;output[5]=io.MousePos.y;output[6]=io.MouseDown[0];output[7]=io.MouseDown[1];output[8]=io.AppFocusLost;output[9]=ImGui::GetCurrentContext()->OpenPopupStack.Size;output[10]=selectedPage;return 11;}
int ts_ui_rect(int control,float* output){std::lock_guard lock(uiMutex);auto found=controlBounds.find(control);if(found==controlBounds.end()||!output)return 0;auto& rectangle=found->second;output[0]=rectangle.x;output[1]=rectangle.y;output[2]=rectangle.z;output[3]=rectangle.w;return 1;}
}
