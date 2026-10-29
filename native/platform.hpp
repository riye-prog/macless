#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
struct ImGuiContext;
namespace toolscreen {
void initializePlatform(void* environment);
struct WindowSource{uint32_t id;std::string application;std::string title;};
void openImagePicker(bool background=false);
const char* clipboardRead(ImGuiContext* context);
void clipboardWrite(ImGuiContext* context,const char* text);
bool captureAccess();
void requestCaptureAccess();
void refreshWindows();
std::vector<WindowSource> windowSources();
std::string captureStatus();
void startSource(int id,const std::string& path);
void stopSource(int id);
std::array<int,3> sourceInfo(int id);
int copySource(int id,uint8_t* destination,int capacity,int previousSequence);
std::string sourceError(int id);
}
