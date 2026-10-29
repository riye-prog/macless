#version 330
#extension GL_ARB_separate_shader_objects : require

layout(std140) uniform Region {
    vec4 Destination;
};
layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec2 sceneCoord;

void main() {
    vec2 corners[6] = vec2[6](vec2(0,0), vec2(1,0), vec2(0,1), vec2(0,1), vec2(1,0), vec2(1,1));
    vec2 uv = corners[gl_VertexIndex];
    sceneCoord = Destination.xy + uv * Destination.zw;
    gl_Position = vec4(sceneCoord * 2.0 - 1.0, 0.0, 1.0);
    texCoord = uv;
}
