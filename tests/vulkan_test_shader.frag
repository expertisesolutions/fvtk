#version 450
#extension GL_ARB_separate_shader_objects : enable

// layout(constant_id = 0) const float outColorR = 0.0f;
// layout(constant_id = 1) const float outColorG = 0.0f;
// layout(constant_id = 2) const float outColorB = 0.0f;
// layout(constant_id = 3) const float outColorA = 1.0f;

layout(push_constant) uniform PushConstsL {
  vec3 inPosition1;
  float color_channel1;
  vec3 inPosition2;
  float color_channel2;
  vec3 inPosition3;
  float color_channel3;
  float color_channel4;
} PushConsts;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(PushConsts.color_channel1
    , PushConsts.color_channel2
    , PushConsts.color_channel3
    , PushConsts.color_channel4);
}
