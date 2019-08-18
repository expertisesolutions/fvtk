#version 450
#extension GL_ARB_separate_shader_objects : enable

const int tex_max_size = 4096;
const int screen_width = 1280;
const int screen_height = 1000;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat4 transformMatrix;
layout(location = 6) in uint zindex;
//layout(location = 7) in uint chain_array_size;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out flat uint out_zindex;
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

struct image_info
{
  uint ii_zindex;
  uint ii_x, ii_y, ii_w, ii_h;
  uint alpha_compositing;
  uint found_alpha;
  uint padding;
  uint cover_bitset[tex_max_size/32];
};
layout  (std430, set = 2, binding = 0) buffer image_infos
{
  image_info ii[];
};
layout  (std430, set = 2, binding = 1) buffer image_zindex
{
  uint zindex_image[];
};
layout (std430, set = 2, binding = 2) buffer indirect_draw
{
  //indirect_draw_struct indirect_draw_array[];
  uint vertex_count;
  uint instance_count;
  uint first_vertex;
  uint first_instance;

  uint buffer_included [tex_max_size];
};

void main() {
  gl_Position = background_position[gl_VertexIndex] * transformMatrix;
  // if (transformMatrix[0].r == 0.0)
  //   gl_Position = vec4 (positions[gl_VertexIndex], 0.0, 1.0);
  // else
  //   gl_Position = vec4 (positions2[gl_VertexIndex], 0.0, 1.0);
  // gl_Position = inPosition;
  fragTexCoord = inTexCoord;
  out_zindex = zindex;
  InstanceID = gl_InstanceIndex;
  //fragTexCoord = background_texcoord[gl_VertexIndex];
}
