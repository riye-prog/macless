#include "overlays.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
struct TextRun {
    ImFont* font;
    float size,x,y,wrap;
    ImU32 color;
    std::string text;
    bool shadow;
};
struct Rectangle {float x,y,width,height;ImU32 color;float radius=0;};
struct Slice {double value;ImU32 color;};
struct Pie {float x=0,y=0,radius=0,depth=0;std::vector<Slice> slices;};
struct OverlayLayout {
    float width=0,height=0,fit=1;
    std::vector<TextRun> text;
    std::vector<Rectangle> rectangles;
    Pie pie;
};
struct Counter {
    std::string rendered,total;
    double fraction=0;
    bool valid=false;
};
ImU32 color(float red,float green,float blue,float alpha){return IM_COL32(int(std::clamp(red,0.0f,1.0f)*255),int(std::clamp(green,0.0f,1.0f)*255),int(std::clamp(blue,0.0f,1.0f)*255),int(std::clamp(alpha,0.0f,1.0f)*255));}
ImU32 styleColor(const toolscreen::OverlayStyle& style,int field,float alpha){return color(float(style[field]),float(style[field+1]),float(style[field+2]),alpha);}
ImU32 profileColor(uint32_t value,float alpha){return IM_COL32((value>>16)&255,(value>>8)&255,value&255,int(std::clamp(alpha,0.0f,1.0f)*255));}
ImU32 shade(ImU32 value,float brightness,float opacity=1){auto c=ImGui::ColorConvertU32ToFloat4(value);return color(c.x*brightness,c.y*brightness,c.z*brightness,c.w*opacity);}
float measure(ImFont* font,float size,const std::string& text){return font->CalcTextSizeA(size,FLT_MAX,0,text.c_str()).x;}
ImFont* face(int choice){auto& fonts=ImGui::GetIO().Fonts->Fonts;return fonts[choice==3&&fonts.Size>6?6:choice==1&&fonts.Size>4?4:choice==2&&fonts.Size>1?1:0];}
ImFont* valueFace(int choice){auto& fonts=ImGui::GetIO().Fonts->Fonts;return fonts[choice==3&&fonts.Size>7?7:choice==1&&fonts.Size>5?5:3];}
std::string percent(double value,int decimals){char text[64];std::snprintf(text,sizeof(text),"%.*f%%",decimals,value);return text;}
std::string sectionName(const std::string& raw){
    if(raw=="blockEntities"||raw=="block_entities"||raw=="extractVisibleBlockEntitiesForRendering")return "Block entities";
    if(raw=="gameRenderer")return "Renderer";
    if(raw=="scheduledPacketProcessing")return "Packets";
    if(raw=="unspecified")return "Other";
    if(raw=="updateDisplay")return "Display";
    if(raw=="fpsPie")return "Profiler";
    std::string result;
    for(size_t i=0;i<raw.size();++i){unsigned char c=raw[i];if(c=='_'){result+=' ';continue;}if(i&&std::isupper(c)&&std::islower(static_cast<unsigned char>(raw[i-1])))result+=' ';result+=char(c);}
    if(!result.empty())result[0]=char(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}
bool blockSection(const std::string& raw){return raw=="blockEntities"||raw=="block_entities"||raw=="extractVisibleBlockEntitiesForRendering";}
Counter parseCounter(const std::string& text){
    Counter result;auto slash=text.find('/');if(slash==std::string::npos)return result;
    size_t left=slash,right=slash+1;while(left>0&&std::isdigit(static_cast<unsigned char>(text[left-1])))--left;while(right<text.size()&&std::isdigit(static_cast<unsigned char>(text[right])))++right;
    if(left==slash||right==slash+1)return result;
    result.rendered=text.substr(left,slash-left);result.total=text.substr(slash+1,right-slash-1);
    double rendered=std::strtod(result.rendered.c_str(),nullptr),total=std::strtod(result.total.c_str(),nullptr);
    result.fraction=total>0?std::clamp(rendered/total,0.0,1.0):0;result.valid=true;return result;
}
OverlayLayout layout(const toolscreen::Mirror& mirror,ImVec2 size,const toolscreen::DebugState& debug){
    using namespace toolscreen;
    OverlayLayout out;const auto& v=mirror.values;const auto& s=mirror.style;
    int type=int(v[33]),decimals=int(s[Decimals]),fontChoice=int(s[FontFace]);bool pixel=fontChoice==3;
    float unit=std::max(.001f,size.y/900*float(v[35]));
    auto snap=[&](float value){return pixel?std::max(1.0f,std::round(value)):value;};
    float textSize=snap((type==2?15:14)*unit),smallSize=snap((type==2?12:11)*unit),titleSize=snap((type==2?16:14)*unit*float(s[TitleScale]));
    float numberSize=snap((type==1?34:36)*unit*float(s[ValueScale]));
    ImFont* font=face(fontChoice),*numbers=face(pixel?3:1),*strong=valueFace(fontChoice);
    float alpha=float(v[9]),padding=float(s[Padding])*unit,safety=3*unit,gap=6*unit;
    auto ink=styleColor(s,TextRed,alpha),muted=styleColor(s,MutedRed,alpha);
    Counter counter=parseCounter(debug.counter);ProfileRow mapless;
    if(type==3&&!maplessRow(debug,mapless,v[34]>.5))return {};
    float numericWidth=std::max(measure(numbers,textSize,percent(100,decimals)),measure(font,smallSize,"Parent"))+12*unit;
    float nameWidth=140*unit;
    for(const auto& row:debug.rows){numericWidth=std::max(numericWidth,measure(numbers,textSize,percent(row.global,decimals))+12*unit);nameWidth=std::max(nameWidth,measure(font,textSize,sectionName(row.name)));}
    int columns=int(s[ShowLocal]>.5)+int(s[ShowGlobal]>.5);float chartScale=v[39]>0?float(v[39]):1;
    float required=0;
    if(type==1){required=counter.valid?measure(strong,numberSize,counter.rendered)+measure(strong,numberSize*.7f,counter.total)+38*unit:measure(strong,numberSize,debug.counter);required=std::max(required,188*unit);}
    else if(type==3){required=std::max(194*unit,measure(strong,numberSize,percent(s[ShowGlobal]>.5?mapless.global:mapless.percentage,decimals)));}
    else{required=std::clamp(nameWidth,140*unit,240*unit)+columns*numericWidth+40*unit+(s[ShowBars]>.5?176*unit*chartScale:0);}
    if(s[ShowTitle]>.5)required=std::max(required,measure(font,titleSize,mirror.name));
    float safe=std::min(12*size.y/900,std::min(size.x,size.y)*.03f),maximumWidth=std::max(8.0f,size.x-2*safe),maximumHeight=std::max(8.0f,size.y-2*safe);
    float wanted=s[AutoWidth]>.5?required+2*(padding+safety):float(v[7])*size.x;
    out.width=std::clamp(wanted,8.0f,maximumWidth);padding=std::min(padding,out.width*.16f);
    float left=padding+safety,right=out.width-padding-safety,content=std::max(1.0f,right-left),y=padding+safety;
    auto rect=[&](float x,float top,float width,float height,ImU32 fill,float radius=0){if(width>0&&height>0)out.rectangles.push_back({x,top,width,height,fill,radius});};
    auto add=[&](ImFont* f,float height,const std::string& text,ImU32 tint,float x,float top,float wrap=0){if(text.empty())return 0.0f;float used=f->CalcTextSizeA(height,FLT_MAX,wrap,text.c_str()).y;out.text.push_back({f,height,x,top,wrap,tint,text,s[TextOutline]>.5});return std::max(height,used);};
    auto line=[&](ImFont* f,float height,const std::string& text,ImU32 tint,int alignment=0){float width=measure(f,height,text);float x=left+std::max(0.0f,content-width)*(alignment==1?.5f:alignment==2?1:0);float used=add(f,height,text,tint,x,y,content);if(used>0)y+=used+gap;};
    auto meter=[&](double fraction,ImU32 tint){float h=5*unit;rect(left,y,content,h,color(0,0,0,.6f*alpha));float fill=std::max(0.0f,content-2*unit)*float(std::clamp(fraction,0.0,1.0));rect(left+unit,y+unit,fill,3*unit,shade(tint,.65f));rect(left+unit,y+unit,fill,unit,tint);for(int i=1;i<10;++i)rect(left+content*i*.1f,y,unit,h,color(0,0,0,.35f*alpha));y+=h+gap;};
    if(s[ShowTitle]>.5){
        float h=add(type==2&&fontChoice==0?face(2):font,titleSize,mirror.name,ink,left,y,content);y+=h+10*unit;
        rect(left,y,content,unit,color(1,1,1,(type==2?.065f:.12f)*alpha));if(type!=2)rect(left,y+unit,content,unit,color(0,0,0,.3f*alpha));y+=11*unit;
    }
    if(type==1){
        if(counter.valid){
            bool rendered=s[ShowLocal]>.5,total=s[ShowGlobal]>.5;if(!rendered&&!total)rendered=true;
            float mainSize=numberSize,totalSize=rendered?numberSize*.7f:numberSize,slashSize=numberSize*.6f;
            float renderedWidth=rendered?measure(strong,mainSize,counter.rendered):0,totalWidth=total?measure(strong,totalSize,counter.total):0;
            float separator=rendered&&total?measure(font,slashSize,"/")+18*unit:0;
            float renderedArea=rendered?std::max(renderedWidth,s[ShowDetails]>.5?measure(font,smallSize,"Rendered"):0.0f):0,totalArea=total?std::max(totalWidth,s[ShowDetails]>.5?measure(font,smallSize,"Total"):0.0f):0;
            float groupWidth=renderedArea+separator+totalArea,fit=std::min(1.0f,content/std::max(1.0f,groupWidth));mainSize*=fit;totalSize*=fit;slashSize*=fit;renderedWidth*=fit;totalWidth*=fit;renderedArea*=fit;totalArea*=fit;separator*=fit;groupWidth*=fit;
            float x=left+std::max(0.0f,content-groupWidth)*(s[TextAlignment]==1?.5f:s[TextAlignment]==2?1:0);
            if(rendered)add(strong,mainSize,counter.rendered,ink,x,y);
            float totalX=x+renderedArea+separator;
            if(rendered&&total)add(font,slashSize,"/",muted,x+renderedArea+9*unit*fit,y+mainSize-slashSize);
            if(total)add(strong,totalSize,counter.total,rendered?muted:ink,totalX,y+mainSize-totalSize);
            y+=mainSize+4*unit;
            if(s[ShowDetails]>.5){float labelHeight=0;if(rendered)labelHeight=add(font,smallSize,"Rendered",muted,x,y,std::max(1.0f,renderedArea+separator));if(total)labelHeight=std::max(labelHeight,add(font,smallSize,"Total",muted,totalX,y,std::max(1.0f,totalArea)));y+=labelHeight+gap;}
            if(s[ShowBars]>.5){y+=5*unit;meter(counter.fraction,shade(ink,.74f));}
        }else line(strong,numberSize*std::min(1.0f,content/std::max(1.0f,measure(strong,numberSize,debug.counter))),debug.counter,ink,int(s[TextAlignment]));
    }else if(type==3){
        if(s[ShowGlobal]>.5||s[ShowLocal]>.5){
            std::string value=percent(s[ShowGlobal]>.5?mapless.global:mapless.percentage,decimals);value.pop_back();
            float size=numberSize,percentSize=numberSize*.52f,percentWidth=measure(font,percentSize,"%"),width=measure(strong,size,value)+percentWidth+5*unit;
            float fit=std::min(1.0f,content/std::max(1.0f,width));size*=fit;percentSize*=fit;width*=fit;
            float x=left+std::max(0.0f,content-width)*(s[TextAlignment]==1?.5f:s[TextAlignment]==2?1:0);
            add(strong,size,value,ink,x,y);add(font,percentSize,"%",muted,x+measure(strong,size,value)+5*unit*fit,y+size-percentSize);y+=size+gap;
        }
        if(s[ShowDetails]>.5)line(font,smallSize,"Block entities",muted,int(s[TextAlignment]));
        if(s[ShowGlobal]>.5&&s[ShowLocal]>.5){y+=3*unit;line(numbers,smallSize,"Parent  "+percent(mapless.percentage,decimals),muted,int(s[TextAlignment]));}
        if(s[ShowBars]>.5){y+=3*unit;meter(mapless.global*.01,profileColor(mapless.color,alpha));}
    }else if(type==2){
        if(s[ShowPath]>.5)line(font,smallSize,debug.path,muted);
        float chartWidth=s[ShowBars]>.5&&content>320*unit?std::min(164*unit*chartScale,content*.40f):0;
        float legendLeft=left+(chartWidth>0?chartWidth+14*unit:0),legendWidth=right-legendLeft;
        bool stacked=legendWidth-columns*numericWidth<112*unit;
        float nameRight=stacked?right:right-columns*numericWidth-6*unit,bodyStart=y;size_t legendTextStart=out.text.size(),legendRectangleStart=out.rectangles.size();
        if(!stacked&&columns&&s[ShowDetails]>.5){
            float x=nameRight+6*unit;
            if(s[ShowLocal]>.5){add(font,smallSize,"Parent",muted,x+numericWidth-measure(font,smallSize,"Parent"),y);x+=numericWidth;}
            if(s[ShowGlobal]>.5)add(font,smallSize,"Frame",muted,x+numericWidth-measure(font,smallSize,"Frame"),y);
            y+=smallSize+12*unit;
        }
        size_t count=v[38]>0?std::min(debug.rows.size(),size_t(std::clamp(int(v[38]),1,32))):debug.rows.size();
        for(size_t i=0;i<count;++i){
            const auto& row=debug.rows[i];float rowY=y,indexWidth=s[ShowIndices]>.5?39*unit:12*unit,nameLeft=legendLeft+indexWidth,nameRoom=std::max(1.0f,nameRight-nameLeft);
            float nameHeight=font->CalcTextSizeA(textSize,FLT_MAX,nameRoom,sectionName(row.name).c_str()).y;
            float height=std::max(float(s[RowHeight])*unit,nameHeight+12*unit);if(stacked&&columns)height+=smallSize+6*unit;
            auto tint=profileColor(row.color,alpha);
            if(blockSection(row.name))rect(legendLeft-7*unit,rowY-5*unit,legendWidth+7*unit,height-2*unit,shade(tint,1,.06f),5*unit);
            if(i+1<count)rect(legendLeft,rowY+height-7*unit,legendWidth,unit,color(1,1,1,.035f*alpha));
            if(s[ShowIndices]>.5&&row.name!="unspecified"){
                float w=21*unit,h=20*unit;rect(legendLeft,rowY-2*unit,w,h,color(1,1,1,.045f*alpha),4*unit);
                auto label=std::to_string(i+1);add(numbers,smallSize,label,muted,legendLeft+(w-measure(numbers,smallSize,label))*.5f,rowY-2*unit+(h-smallSize)*.5f);
            }
            rect(legendLeft+(s[ShowIndices]>.5?28:1)*unit,rowY+2*unit,3*unit,12*unit,tint,1.5f*unit);
            add(font,textSize,sectionName(row.name),ink,nameLeft,rowY,nameRoom);
            if(stacked){
                std::string values;if(s[ShowLocal]>.5)values="Parent "+percent(row.percentage,decimals);if(s[ShowGlobal]>.5)values+=(values.empty()?"":"   ")+std::string("Frame ")+percent(row.global,decimals);
                float valueHeight=add(numbers,smallSize,values,muted,nameLeft,rowY+nameHeight+4*unit,nameRoom);height=std::max(height,nameHeight+valueHeight+12*unit);
            }else{
                float x=nameRight+6*unit;
                if(s[ShowLocal]>.5){auto value=percent(row.percentage,decimals);add(numbers,textSize,value,muted,x+numericWidth-measure(numbers,textSize,value),rowY);x+=numericWidth;}
                if(s[ShowGlobal]>.5){auto value=percent(row.global,decimals);add(numbers,textSize,value,ink,x+numericWidth-measure(numbers,textSize,value),rowY);}
            }
            y=rowY+height;
        }
        if(count==0)line(font,textSize,"No sections",muted);
        if(count<debug.rows.size()){y+=4*unit;line(font,smallSize,"+"+std::to_string(debug.rows.size()-count)+" sections",muted);}
        if(chartWidth>0){
            float radius=chartWidth*.47f,depth=8*unit,chartHeight=radius*1.28f+depth;
            float captionHeight=s[ShowDetails]>.5?38*unit:0,legendShift=std::max(0.0f,(chartHeight-(y-bodyStart))*.5f);
            if(legendShift>0){for(size_t i=legendTextStart;i<out.text.size();++i)out.text[i].y+=legendShift;for(size_t i=legendRectangleStart;i<out.rectangles.size();++i)out.rectangles[i].y+=legendShift;}
            y=std::max(y,bodyStart+chartHeight+captionHeight+8*unit);
            out.pie={left+chartWidth*.5f,bodyStart+std::max(0.0f,(y-bodyStart-chartHeight-captionHeight)*.5f)+radius*.64f,radius,depth,{}};
            for(const auto& row:debug.rows)out.pie.slices.push_back({std::max(0.0,row.percentage),profileColor(row.color,alpha)});
            if(s[ShowDetails]>.5){float top=out.pie.y+radius*.64f+depth+9*unit;auto value=percent(debug.global,decimals);float valueSize=std::min(textSize,chartWidth/std::max(1.0f,measure(numbers,1,value)));add(numbers,valueSize,value,ink,left+(chartWidth-measure(numbers,valueSize,value))*.5f,top);auto caption=std::string("of frame");add(font,smallSize,caption,muted,left+(chartWidth-measure(font,smallSize,caption))*.5f,top+valueSize+4*unit);}
        }
        if(s[ShowFooter]>.5){y+=6*unit;line(font,smallSize,"0  Back",muted);}
    }
    float naturalHeight=std::max(8.0f,y-gap+padding+safety),contentRight=out.width;
    for(const auto& run:out.text){auto extent=run.font->CalcTextSizeA(run.size,FLT_MAX,run.wrap,run.text.c_str());contentRight=std::max(contentRight,run.x+extent.x+padding+safety);naturalHeight=std::max(naturalHeight,run.y+extent.y+padding+safety);}
    out.height=std::clamp(v[37]>.5?naturalHeight:float(v[8])*size.y,8.0f,maximumHeight);
    out.fit=std::min({1.0f,out.height/naturalHeight,out.width/contentRight});
    return out;
}
void triangle(ImDrawList* draw,ImVec2 a,ImVec2 b,ImVec2 c,ImU32 ca,ImU32 cb,ImU32 cc){
    auto uv=ImGui::GetFontTexUvWhitePixel();draw->PrimReserve(3,3);auto base=draw->_VtxCurrentIdx;
    draw->PrimWriteIdx(ImDrawIdx(base));draw->PrimWriteIdx(ImDrawIdx(base+1));draw->PrimWriteIdx(ImDrawIdx(base+2));
    draw->PrimWriteVtx(a,uv,ca);draw->PrimWriteVtx(b,uv,cb);draw->PrimWriteVtx(c,uv,cc);
}
void drawPie(ImDrawList* draw,const Pie& pie,ImVec2 origin,float fit){
    if(pie.radius<=0)return;
    double sum=0;int visibleSlices=0;for(const auto& slice:pie.slices){sum+=slice.value;if(slice.value>0)++visibleSlices;}if(sum<=0)return;
    float radius=pie.radius*fit,depth=pie.depth*fit;ImVec2 center(origin.x+pie.x*fit,origin.y+pie.y*fit);
    auto point=[&](float angle,float elevation,float inset=0){return ImVec2(center.x+std::cos(angle)*(radius-inset),center.y+std::sin(angle)*(radius-inset)*.64f+elevation);};
    auto flags=draw->Flags;draw->Flags&=~ImDrawListFlags_AntiAliasedFill;
    float opacity=0;for(const auto& slice:pie.slices)opacity=std::max(opacity,float((slice.color>>24)&255)/255);
    for(int layer=3;layer>0;--layer){std::vector<ImVec2> shadow;for(int i=0;i<96;++i){float angle=i*6.2831853072f/96;shadow.emplace_back(center.x+std::cos(angle)*(radius+layer*fit),center.y+depth+3*fit+std::sin(angle)*(radius*.64f+layer*fit));}draw->AddConvexPolyFilled(shadow.data(),int(shadow.size()),color(0,0,0,.10f*opacity));}
    float angle=-1.5707963268f;
    for(const auto& slice:pie.slices){
        if(slice.value<=0)continue;
        float span=float(slice.value/sum)*6.2831853072f;int count=std::max(1,int(std::ceil(span*24)));
        for(int i=0;i<count;++i){
            float a=angle+span*i/count,b=angle+span*(i+1)/count;
            if(std::sin((a+b)*.5f)>0){
                auto p0=point(a,0),p1=point(b,0),p2=point(b,depth),p3=point(a,depth);
                float lightA=.40f+.16f*std::sin(a)+.06f*std::cos(a+1),lightB=.40f+.16f*std::sin(b)+.06f*std::cos(b+1);
                auto topA=shade(slice.color,lightA),topB=shade(slice.color,lightB),bottomA=shade(slice.color,lightA*.64f),bottomB=shade(slice.color,lightB*.64f);
                triangle(draw,p0,p1,p2,topA,topB,bottomB);triangle(draw,p0,p2,p3,topA,bottomB,bottomA);
                draw->AddLine(p3,p2,shade(slice.color,.28f),std::max(.5f,fit));
            }
        }
        angle+=span;
    }
    angle=-1.5707963268f;
    for(const auto& slice:pie.slices){
        if(slice.value<=0)continue;
        float span=float(slice.value/sum)*6.2831853072f,gap=visibleSlices>1?std::min(.009f,span*.08f):0,bevel=std::min(3*fit,radius*.06f);
        int count=std::max(2,int(std::ceil(span*24)));
        for(int i=0;i<count;++i){
            float a=angle+gap+(span-2*gap)*i/count,b=angle+gap+(span-2*gap)*(i+1)/count;
            auto outerA=point(a,0),outerB=point(b,0),innerA=point(a,-fit,bevel),innerB=point(b,-fit,bevel);
            float lightA=1.02f+.14f*std::cos(a+2.3f),lightB=1.02f+.14f*std::cos(b+2.3f);
            auto edgeA=shade(slice.color,lightA),edgeB=shade(slice.color,lightB),surfaceA=shade(slice.color,.94f),surfaceB=shade(slice.color,.94f);
            triangle(draw,center,innerA,innerB,shade(slice.color,1.08f),surfaceA,surfaceB);
            triangle(draw,innerA,outerA,outerB,surfaceA,edgeA,edgeB);triangle(draw,innerA,outerB,innerB,surfaceA,edgeB,surfaceB);
            draw->AddLine(outerA,outerB,shade(slice.color,lightA*.85f),std::max(.5f,fit*.6f));
        }
        if(visibleSlices>1&&span>.025f){auto a=point(angle+gap,-fit,bevel),b=point(angle+span-gap,-fit,bevel);draw->AddLine(center,a,shade(slice.color,.74f),std::max(.5f,fit*.5f));draw->AddLine(center,b,shade(slice.color,.62f),std::max(.5f,fit*.5f));}
        angle+=span;
    }
    draw->Flags=flags;
}
void drawPanel(ImDrawList* draw,ImVec2 point,ImVec2 end,const toolscreen::Mirror& mirror,float unit){
    using namespace toolscreen;const auto& v=mirror.values;const auto& s=mirror.style;float alpha=float(v[9]*v[36]);
    if(alpha>0)draw->AddRectFilled(point,end,styleColor(s,BackgroundRed,alpha),float(s[CornerRadius])*unit);

}
}

namespace toolscreen {
ImVec2 runtimeMirrorExtent(const Mirror& mirror,ImVec2 size,const DebugState& debug){if(mirror.values[33]<1)return ImVec2(float(mirror.values[7])*size.x,float(mirror.values[8])*size.y);auto computed=layout(mirror,size,debug);return ImVec2(computed.width,computed.height);}
ImVec2 runtimeMirrorPosition(const SourceValues& values,ImVec2 size,ImVec2 extent){float safe=std::min(12*size.y/900,std::min(size.x,size.y)*.03f);return ImVec2(std::clamp(float(values[5])*size.x,safe,std::max(safe,size.x-safe-extent.x)),std::clamp(float(values[6])*size.y,safe,std::max(safe,size.y-safe-extent.y)));}
void drawRuntimeOverlays(ImDrawList* draw,ImVec2 origin,ImVec2 size,int route,ImFont*,ImFont*,ImFont*){
    auto settings=snapshot();if(settings[128]<.5||size.x<8||size.y<8)return;
    int mask=1<<std::clamp(route,0,1),mode=int(settings[0]);auto debug=debugState();
    draw->PushClipRect(origin,ImVec2(origin.x+size.x,origin.y+size.y),true);
    for(const auto& source:mirrors()){
        auto mirror=mirrorForOutput(source,0);const auto& v=mirror.values;const auto& s=mirror.style;int type=int(v[33]);
        if(v[0]<.5||!(int(v[14])&(1<<mode))||!(int(v[32])&mask)||type<1||type>3||v[9]<=0)continue;
        if((type==1&&debug.counter.empty())||(type>=2&&debug.path.empty()))continue;
        auto computed=layout(mirror,size,debug);if(computed.width<8||computed.height<8)continue;
        auto offset=runtimeMirrorPosition(v,size,ImVec2(computed.width,computed.height));
        ImVec2 point(std::floor(origin.x+offset.x),std::floor(origin.y+offset.y)),end(point.x+computed.width,point.y+computed.height);
        float unit=size.y/900*float(v[35]);drawPanel(draw,point,end,mirror,unit);
        for(const auto& box:computed.rectangles){ImVec2 p(std::round(point.x+box.x*computed.fit),std::round(point.y+box.y*computed.fit));draw->AddRectFilled(p,ImVec2(std::round(p.x+box.width*computed.fit),std::round(p.y+box.height*computed.fit)),box.color,box.radius*computed.fit);}
        drawPie(draw,computed.pie,point,computed.fit);
        for(const auto& run:computed.text){
            ImVec2 p(std::round(point.x+run.x*computed.fit),std::round(point.y+run.y*computed.fit));float height=run.size*computed.fit,wrap=run.wrap*computed.fit;
            if(run.shadow){float shadow=(type==2?std::max(.5f,unit*.5f):std::max(1.0f,std::floor(unit)))*computed.fit;draw->AddText(run.font,height,ImVec2(p.x+(type==2?0:shadow),p.y+shadow),color(0,0,0,(type==2?.22f:.85f)*float(v[9])),run.text.c_str(),nullptr,wrap);}
            draw->AddText(run.font,height,p,run.color,run.text.c_str(),nullptr,wrap);
        }
        if(v[15]>0)draw->AddRect(point,end,color(float(v[16]),float(v[17]),float(v[18]),float(v[19]*v[9])),float(s[CornerRadius])*unit,0,std::max(.5f,float(v[15])*unit));
    }
    draw->PopClipRect();
}
}
