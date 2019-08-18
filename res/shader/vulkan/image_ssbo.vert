#version 450
#extension GL_ARB_separate_shader_objects : enable

const int tex_max_size = 4096;
const int screen_width = 1280;
const int screen_height = 1000;

//layout(location = 0) in vec4 inPosition;
//layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat4 transformMatrix;
//layout(location = 6) in uint zindex;
//layout(location = 7) in uint chain_array_size;

layout(location = 0) out vec2 fragTexCoord;
//layout(location = 1) out flat uint out_zindex;
layout(location = 2) out flat uint InstanceID;

// vec4 background_position[6]
// =
//   {
//    {-1.0f, -1.0f, 0.0f, 1.0f}
//    ,  {1.0f, -1.0f, 0.0f, 1.0f}
//    ,  {1.0f,  1.0f, 0.0f, 1.0f}
//    ,  {1.0f,  1.0f, 0.0f, 1.0f}
//    , {-1.0f,  1.0f, 0.0f, 1.0f}
//    , {-1.0f, -1.0f, 0.0f, 1.0f}
//   };
vec2 texcoord[6]
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
  //uint cover_bitset[tex_max_size/32];
};
layout  (std430, set = 2, binding = 0) readonly buffer image_infos
{
  image_info ii[];
};
layout  (std430, set = 2, binding = 1) readonly buffer image_zindex
{
  uint zindex_image[];
};
layout (std430, set = 2, binding = 2) readonly buffer indirect_draw
{
  //indirect_draw_struct indirect_draw_array[];
  uint vertex_count;
  uint instance_count;
  uint first_vertex;
  uint first_instance;

  uint image_length;
  uint fragment_data_length;
  uint buffers_to_draw [tex_max_size];
  //fragment_data fragment_datas[tex_max_size];
  uint fg_zindex[tex_max_size];
};

void main()
{
  uint zindex = fg_zindex[gl_InstanceIndex];
  vec4 positions[6] =
    vec4[6](
       vec4 ((ii[zindex].ii_x / (screen_width-1))*2-1.0f
             , (ii[zindex].ii_y/(screen_height-1))*2-1.0f, 0.0f, 1.0f)
     , vec4 (((ii[zindex].ii_x + ii[zindex].ii_w-1)/(screen_width-1))*2-1.0f
             , (ii[zindex].ii_y/(screen_height-1))*2-1.0f, 0.0f, 1.0f)
     , vec4 (((ii[zindex].ii_x + ii[zindex].ii_w-1)/(screen_width-1))*2-1.0f
             , ((ii[zindex].ii_y + ii[zindex].ii_h-1)/(screen_height-1))*2-1.0f, 0.0f, 1.0f)
     , vec4 (((ii[zindex].ii_x + ii[zindex].ii_w-1)/(screen_width-1))*2-1.0f
             , ((ii[zindex].ii_y + ii[zindex].ii_h-1)/(screen_height-1))*2-1.0f, 0.0f, 1.0f)
     , vec4 (((ii[zindex].ii_x)/(screen_width-1))*2-1.0f
             , ((ii[zindex].ii_y + ii[zindex].ii_h-1)/(screen_height-1))*2-1.0f, 0.0f, 1.0f)
     , vec4 ((ii[zindex].ii_x/(screen_width-1))*2-1.0f
             , (ii[zindex].ii_y/(screen_height-1))*2-1.0f, 0.0f, 1.0f)
    );
  
  gl_Position = positions[gl_VertexIndex] /** transformMatrix*/ /**screenMatrix*/;
  fragTexCoord = texcoord[gl_VertexIndex];
  InstanceID = gl_InstanceIndex;
}
