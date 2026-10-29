#pragma once
#include "imgui.h"
#include <algorithm>
#include <cstdint>

namespace toolscreen::ui {
inline ImVec4 rgba(uint32_t rgb,float alpha=1){return ImVec4(float((rgb>>16)&255)/255,float((rgb>>8)&255)/255,float(rgb&255)/255,alpha);}
inline void applyTheme(ImGuiStyle& style){
    style.WindowPadding=ImVec2(16,14);
    style.FramePadding=ImVec2(10,7);
    style.ItemSpacing=ImVec2(10,10);
    style.ItemInnerSpacing=ImVec2(6,5);
    style.CellPadding=ImVec2(8,6);
    style.WindowRounding=12;
    style.ChildRounding=8;
    style.FrameRounding=6;
    style.PopupRounding=10;
    style.GrabRounding=5;
    style.ScrollbarRounding=5;
    style.WindowBorderSize=1;
    style.ChildBorderSize=0;
    style.FrameBorderSize=0;
    style.ScrollbarSize=7;
    style.ButtonTextAlign=ImVec2(.5f,.5f);
    style.CurveTessellationTol=.2f;
    style.CircleTessellationMaxError=.1f;
    style.AntiAliasedLines=true;
    style.AntiAliasedFill=true;
    style.Colors[ImGuiCol_Text]=rgba(0xf1f2f4);
    style.Colors[ImGuiCol_TextDisabled]=rgba(0xa6abb3);
    style.Colors[ImGuiCol_WindowBg]=rgba(0x18191b);
    style.Colors[ImGuiCol_ChildBg]=rgba(0,0);
    style.Colors[ImGuiCol_PopupBg]=rgba(0x222428);
    style.Colors[ImGuiCol_Border]=rgba(0x3b3e44,.82f);
    style.Colors[ImGuiCol_BorderShadow]=rgba(0,0);
    style.Colors[ImGuiCol_FrameBg]=rgba(0x282b30);
    style.Colors[ImGuiCol_FrameBgHovered]=rgba(0x34383f);
    style.Colors[ImGuiCol_FrameBgActive]=rgba(0x3e444d);
    style.Colors[ImGuiCol_Button]=rgba(0x282b30);
    style.Colors[ImGuiCol_ButtonHovered]=rgba(0x373b43);
    style.Colors[ImGuiCol_ButtonActive]=rgba(0x454b55);
    style.Colors[ImGuiCol_Header]=rgba(0x282b30,0);
    style.Colors[ImGuiCol_HeaderHovered]=rgba(0x373b43,.65f);
    style.Colors[ImGuiCol_HeaderActive]=rgba(0x454b55,.8f);
    style.Colors[ImGuiCol_CheckMark]=rgba(0xe1e5eb);
    style.Colors[ImGuiCol_SliderGrab]=rgba(0xcbd0d9);
    style.Colors[ImGuiCol_SliderGrabActive]=rgba(0xf1f2f4);
    style.Colors[ImGuiCol_Separator]=rgba(0x3b3e44,.7f);
    style.Colors[ImGuiCol_SeparatorHovered]=rgba(0x858b94);
    style.Colors[ImGuiCol_SeparatorActive]=rgba(0xcbd0d9);
    style.Colors[ImGuiCol_ResizeGrip]=rgba(0xcbd0d9,.04f);
    style.Colors[ImGuiCol_ResizeGripHovered]=rgba(0xcbd0d9,.35f);
    style.Colors[ImGuiCol_ResizeGripActive]=rgba(0xe1e5eb,.7f);
    style.Colors[ImGuiCol_ScrollbarBg]=rgba(0,0);
    style.Colors[ImGuiCol_ScrollbarGrab]=rgba(0x5e646e,.8f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]=rgba(0xa6abb3);
    style.Colors[ImGuiCol_ScrollbarGrabActive]=rgba(0xe1e5eb);
    style.Colors[ImGuiCol_TextSelectedBg]=rgba(0x606873,.6f);
    style.Colors[ImGuiCol_NavCursor]=rgba(0xdce1e8);
    style.Colors[ImGuiCol_TableHeaderBg]=rgba(0,0);
    style.Colors[ImGuiCol_TableRowBgAlt]=rgba(0xcbd0d9,.025f);
    style.TabRounding=5;
}
inline void drawShadow(ImDrawList* draw,ImVec2 point,ImVec2 size,float scale){
    for(int i=12;i>0;--i){float spread=i*2*scale,weight=1-float(i)/13;draw->AddRectFilled(ImVec2(point.x-spread,point.y-spread+6*scale),ImVec2(point.x+size.x+spread,point.y+size.y+spread+6*scale),IM_COL32(0,0,0,int(18*weight*weight)),12*scale+spread);}
}
}
