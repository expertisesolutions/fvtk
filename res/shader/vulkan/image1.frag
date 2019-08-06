#version 450
#extension GL_ARB_separate_shader_objects : enable

const int tex_max_size = 4096;

// input from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in flat uint zindex;
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
};
layout  (std430, binding = 2, set = 0) buffer image_infos
{
  image_info ii[];
};

layout  (std430, binding = 2, set = 1) buffer image_infos1
{
  vec4 somethingelse;
};

// output
layout(location = 0) out vec4 outColor;

layout(origin_upper_left) in vec4 gl_FragCoord;

void main() {
  // if (zindex != pixels[uint(gl_FragCoord.x) + uint(gl_FragCoord.y)*1280])
  // {
  //   pixels[uint(gl_FragCoord.x) + uint(gl_FragCoord.y)*1280] = zindex;
  //   zindex_array[zindex] = 1;
  // }
  vec4 color;
  // if (zindex == 1)
  //   color = texture(sampler2D(tex[zindex-1], samp), fragTexCoord);
  // else
  color = texture(sampler2D(tex[zindex], samp), fragTexCoord);
  // if (color.a != 1.0f)
  //   ii[zindex].found_alpha = 1;
  outColor = color;
}

