#version 450
#extension GL_ARB_separate_shader_objects : enable

const int tex_max_size = 4096;
const int screen_width = 2000;
const int screen_height = 1500;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 2) out flat uint InstanceID;

vec4 background_position[6]
=
  {
   {-1.0f, -1.0f, 0.0f, 1.0f}
   ,  {1.0f, -1.0f, 0.0f, 1.0f}
   ,  {1.0f,  1.0f, 0.0f, 1.0f}
   ,  {1.0f,  1.0f, 0.0f, 1.0f}
   , {-1.0f,  1.0f, 0.0f, 1.0f}
   , {-1.0f, -1.0f, 0.0f, 1.0f}
  };
vec2 background_texcoord[6]
=
  {
      {0.0f,  0.0f}
   ,  {1.0f,  0.0f}
   ,  {1.0f,  1.0f}
   ,  {1.0f,  1.0f}
   ,  {0.0f,  1.0f}
   ,  {0.0f,  0.0f}
  };

void main() {
  gl_Position = background_position[gl_VertexIndex];
  fragTexCoord = background_texcoord[gl_VertexIndex];
  InstanceID = gl_InstanceIndex;
}
