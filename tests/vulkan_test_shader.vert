#version 450

layout(push_constant) uniform PushConstsL {
  vec3 inPosition1;
  float color_channel1;
  vec3 inPosition2;
  float color_channel2;
  vec3 inPosition3;
  float color_channel3;
  float color_channel4;
} PushConsts;

vec3 inPosition[] =
 {
   PushConsts.inPosition1
 , PushConsts.inPosition2
 , PushConsts.inPosition3
 };

out gl_PerVertex
{
	vec4 gl_Position;
};


void main() {
    gl_Position = vec4(inPosition[gl_VertexIndex], 1.0);
}
