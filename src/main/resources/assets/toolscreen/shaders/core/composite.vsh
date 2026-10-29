#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform Layer {
    vec4 Source;
    vec4 Destination;
    vec4 KeyColor;
    vec4 Effects;
    vec4 Dimensions;
};

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec2 localCoord;

void main() {
    vec2 corners[6] = vec2[6](vec2(0,0), vec2(1,0), vec2(0,1), vec2(0,1), vec2(1,0), vec2(1,1));
    vec2 uv = corners[gl_VertexIndex];
    gl_Position = vec4((Destination.xy + uv * Destination.zw) * 2.0 - 1.0, 0.0, 1.0);
    texCoord = Source.xy + uv * Source.zw;
    localCoord = uv;
}
