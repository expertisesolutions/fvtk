#version 450
#extension GL_ARB_separate_shader_objects : enable

const int tex_max_size = 4096;

// input from vertex shader
layout(location = 0) in vec2 fragTexCoord;
//layout(location = 1) in flat uint zindex;
layout(location = 2) in flat uint InstanceID;
//layout(location = 2) in flat uint chain_array_size;
// from descriptors
layout(set = 0, binding = 0) uniform texture2D tex[tex_max_size];
layout(set = 1, binding = 0) uniform sampler samp;
// layout(std430, binding = 2) buffer chain_pixel_buffer
// {
//   uint chain_index[];
// };
// layout(std430, binding = 3) buffer zindex_chain_layout
// {
//   uint zindex_chain[]; // indexed by zindex
// };
struct image_info
{
  uint ii_zindex;
  uint ii_x, ii_y, ii_w, ii_h;
  uint alpha_compositing;
  uint found_alpha;
  uint padding;
  //uint cover_bitset[tex_max_size/32];
};
layout  (std430, set = 2, binding = 0) buffer image_infos
{
  image_info ii[];
};
layout  (std430, set = 2, binding = 1) buffer image_zindex
{
  uint zindex_image[];
};
struct fragment_data
{
  uint fg_zindex;
};

layout (std430, set = 2, binding = 2) buffer indirect_draw
{
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

// output
layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord;

void main() {
  //uint zindex = fg_zindex[fragment_data_length - InstanceID - 1];
  uint zindex = fg_zindex[InstanceID];
  vec4 color;

  color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
  // if (zindex == 0)
  //   color = vec4(0.25f, 0.0f, 0.0f, 0.5f);
  //   //color = texture(sampler2D(tex[zindex], samp), fragTexCoord);
  // else
    if (zindex == 1)
    //color = texture(sampler2D(tex[zindex], samp), fragTexCoord);
    color = vec4(0.0f, 0.25f, 0.0f, 0.5f);
  else if (zindex == 2)
    //color = texture(sampler2D(tex[zindex], samp), fragTexCoord);
    color = vec4(0.0f, 0.0f, 0.25f, 0.5f);

  //color = texture(sampler2D(tex[zindex], samp), fragTexCoord);

  if (ii[zindex].ii_zindex != zindex)
    color = vec4(0.0f, 0.0, 1.0f, 0.5f);
  // else if (zindex == 0)
  //   color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  // else
    // color = texture(sampler2D(tex[zindex], samp), fragTexCoord);
  // if (zindex == 1)
  //   //color = texture(sampler2D(tex[2], samp), fragTexCoord);
  //   color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  // else
  // if (ii[zindex].ii_zindex != zindex)
  //   color = vec4(0.25f, 0.0, 0.0f, 0.5f);
  // else
  //   color = texture(sampler2D(tex[zindex], samp), fragTexCoord);

  if (InstanceID != 0)
    color += vec4(0.0f, 0.5f, 0.0f, 0.5f);
  
  // if (color.a != 1.0f)
  //   atomicExchange(ii[zindex].found_alpha, 1);
  outColor = color;
}

