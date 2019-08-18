#version 450
#extension GL_ARB_separate_shader_objects : enable

const int tex_max_size = 4096;
const int screen_width = 1280;
const int screen_height = 1000;

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
// layout (std430, set = 3, binding = 1) buffer indirect_draw_shared_state
// {
  
// };

// output
layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord;

void main()
{
  if (gl_SampleMaskIn[0] == 0)
    return;

  uint error = 0;
  for (uint i = image_length; i != 0; --i)
  {
    uint j = i-1;
    uint tries = 0;
    if (atomicCompSwap(buffers_to_draw[j], 0, 0) == 0)
    {
      uint unorm_x = uint(floor(gl_FragCoord.x));
      uint unorm_y = uint(floor(gl_FragCoord.y));
      if (unorm_x >= ii[j].ii_x && unorm_x < ii[j].ii_x + ii[j].ii_w
          && unorm_y >= ii[j].ii_y && unorm_y < ii[j].ii_y + ii[j].ii_h)
      {
        // found correct image
        if (atomicExchange(buffers_to_draw[j], 1) == 0)
        {
          atomicAdd (instance_count, 1);

          uint old_value = 0xFFFFFFFF;
          uint value = j;
          uint cur_index = 0;
          while (true)
          {
            uint length = atomicAdd(fragment_data_length, 0);
            while (cur_index != length)
            {
              old_value = atomicMin (fg_zindex[cur_index], value);
              ++cur_index;
              if (value < old_value)
                value = old_value;
            }

            // theoretically after sequence
            uint old_index = 0xFFFFFFFF;
            if ((old_index = atomicCompSwap (fg_zindex[cur_index], 0xFFFFFFFF, value)) == 0xFFFFFFFF)
            {
              atomicAdd (fragment_data_length, 1);
              break;
            }
          }

          vec2 fragTexCoord = vec2 (floor(gl_FragCoord.x - ii[j].ii_x)/(ii[j].ii_w-1)
                                    , floor(gl_FragCoord.y - ii[j].ii_y)/(ii[j].ii_h-1));
          vec4 color = texture(sampler2D(tex[j], samp), fragTexCoord);
          if (color.a == 1.0f)
            break;
        }
      }
    }
  }
}



