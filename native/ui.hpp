#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace toolscreen {
std::vector<uint8_t> uiFrame(int width, int height, float delta, const std::string& backend, const std::string& error);
std::vector<uint8_t> fontAtlas();
uint64_t fontRevision();
std::vector<uint8_t> overlayFrame(int width,int height,int viewWidth,int viewHeight,int route=0);
void uiDisplayScale(float value);
void uiMouse(float x, float y);
void uiButton(int button, bool down);
void uiScroll(float x, float y);
void uiKey(int keycode, int modifiers, bool down);
void uiText(uint32_t codepoint);
void uiFocus(bool focused);
bool uiCloseRequested();
bool uiBindingActive();
bool uiWantsText();
bool uiEscapeActive();
bool uiPhysicalKey(int scan,int keycode,int modifiers,bool down);
void uiAssetStatus(int id,int width,int height,const std::string& error,int canvasWidth,int canvasHeight);
}

extern "C" {
int ts_ui_frame(int width, int height, float delta, uint8_t* output, int capacity);
void ts_ui_mouse(float x, float y);
void ts_ui_button(int button, int down);
int ts_ui_rect(int control, float* output);
}
