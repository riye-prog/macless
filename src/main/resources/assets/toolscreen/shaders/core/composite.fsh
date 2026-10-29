#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform Layer {
    vec4 Source;
    vec4 Destination;
    vec4 KeyColor;
    vec4 Effects;
    vec4 Dimensions;
};
uniform sampler2D InSampler;
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 localCoord;
layout(location = 0) out vec4 fragColor;

void main(){
    int flags=int(Effects.x);
    if((flags&8)!=0){fragColor=vec4(KeyColor.rgb,Effects.w);return;}
    if((flags&32)!=0){float y=Destination.y+localCoord.y*Destination.w;fragColor=vec4(mix(Source.rgb,KeyColor.rgb,y),1);return;}
    vec2 uv=vec2(texCoord.x,(flags&16)!=0?1.0-texCoord.y:texCoord.y);
    vec4 color=texture(InSampler,uv);
    if((flags&512)!=0)color.a=1.0;
    if((flags&128)!=0){float edge=1e10;if(Source.x<0.000001)edge=min(edge,localCoord.x*Dimensions.x);if(Source.x+Source.z>0.999999)edge=min(edge,(1.0-localCoord.x)*Dimensions.x);if(Source.y<0.000001)edge=min(edge,localCoord.y*Dimensions.y);if(Source.y+Source.w>0.999999)edge=min(edge,(1.0-localCoord.y)*Dimensions.y);if(edge<Effects.y){fragColor=vec4(KeyColor.rgb,1);return;}}
    float foreground=smoothstep(Effects.y,Effects.y+0.03,distance(color.rgb,KeyColor.rgb));
    if((flags&4)!=0){foreground=1.0-smoothstep(Effects.y,Effects.y+0.005,distance(color.rgb,KeyColor.rgb));color.a*=foreground;if((flags&256)!=0)color.rgb=vec3(1);}
    else if((flags&1)!=0)color.a*=foreground;
    if((flags&2)!=0&&foreground>0.5)color.rgb=0.55+0.45*cos(6.283185*(Effects.z*0.18+localCoord.x+vec3(0,0.3333,0.6667)));
    color.a*=Effects.w;
    fragColor=color;
}
