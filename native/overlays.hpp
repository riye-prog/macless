#pragma once
#include "imgui.h"
#include "core.hpp"
#include "features.hpp"

namespace toolscreen {
ImVec2 runtimeMirrorExtent(const Mirror& mirror,ImVec2 size,const DebugState& debug);
ImVec2 runtimeMirrorPosition(const SourceValues& values,ImVec2 size,ImVec2 extent);
void drawRuntimeOverlays(ImDrawList* draw,ImVec2 origin,ImVec2 size,int route,ImFont* font,ImFont* numbers,ImFont* values);
}
