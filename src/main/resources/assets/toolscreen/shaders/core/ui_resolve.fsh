#version 330
#extension GL_ARB_separate_shader_objects : require

uniform sampler2D SceneSampler;
uniform sampler2D UiSampler;
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 sceneCoord;
layout(location = 0) out vec4 fragColor;

vec3 decodeColor(vec3 value) {
    return mix(value / 12.92, pow((value + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), value));
}

vec3 encodeColor(vec3 value) {
    value = max(value, vec3(0));
    return mix(value * 12.92, 1.055 * pow(value, vec3(1.0 / 2.4)) - 0.055, step(vec3(0.0031308), value));
}

void main() {
    vec4 scene = texture(SceneSampler, sceneCoord);
    vec4 ui = texture(UiSampler, texCoord);
    fragColor = vec4(encodeColor(ui.rgb + decodeColor(scene.rgb) * (1.0 - ui.a)), ui.a + scene.a * (1.0 - ui.a));
}
