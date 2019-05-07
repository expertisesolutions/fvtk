#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(constant_id = 0) const float outColorR = 0.0f;
layout(constant_id = 1) const float outColorG = 0.0f;
layout(constant_id = 2) const float outColorB = 0.0f;
layout(constant_id = 3) const float outColorA = 1.0f;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(outColorR, outColorG, outColorB, outColorA);
}
