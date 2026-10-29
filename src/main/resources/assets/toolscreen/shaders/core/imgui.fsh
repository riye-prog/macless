#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D FontSampler;
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 vertexColor;
layout(location = 0) out vec4 fragColor;

vec3 decodeColor(vec3 value) {
    return mix(value / 12.92, pow((value + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), value));
}

void main() {
    vec4 sampled = texture(FontSampler, texCoord);
    fragColor = vec4(decodeColor(vertexColor.rgb) * decodeColor(sampled.rgb), vertexColor.a * sampled.a);
}
