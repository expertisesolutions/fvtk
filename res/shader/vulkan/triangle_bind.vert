#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

// out gl_PerVertex
// {
// 	vec4 gl_Position;
// };

layout (location = 0) out vec4 inFragColor;


void main() {
  gl_Position = vec4(inPosition, 0.0, 1.0);
  inFragColor = inColor;
}
