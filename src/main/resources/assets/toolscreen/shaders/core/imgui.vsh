#version 330
#extension GL_ARB_separate_shader_objects : require

layout(location = 0) in vec3 Position;
layout(location = 1) in vec2 UV0;
layout(location = 2) in vec4 Color;

layout(std140) uniform Screen {
    vec4 DisplaySize;
    vec4 Origin;
};

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec4 vertexColor;

void main() {
    gl_Position = vec4((Position.x - Origin.x) / DisplaySize.x * 2.0 - 1.0, 1.0 - (Position.y - Origin.y) / DisplaySize.y * 2.0, 0.0, 1.0);
    texCoord = vec2(UV0.x,DisplaySize.z>0.5?1.0-UV0.y:UV0.y);
    vertexColor = Color;
}
