#version 450
#extension GL_ARB_separate_shader_objects : enable

const int tex_max_size = 4096;
const int screen_width = 1280;
const int screen_height = 1000;

layout(set = 0, binding = 0) uniform texture2D tex[tex_max_size];
layout(set = 1, binding = 0) uniform sampler samp;

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

  uint padding_was_image_length;
  uint fragment_data_length;
  uint buffers_to_draw [tex_max_size];
  uint fg_zindex[tex_max_size];
};

layout(std430, push_constant) uniform PushConstants
{
  uint image_length;
} constants;

// output
layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord;

void main()
{
  // in bool gl_HelperInvocation
  if (!gl_HelperInvocation)
  {
  uint error = 0;
  for (uint i = constants.image_length; i != 0; --i)
  {
    uint value = i-1;
    if (buffers_to_draw[value] == 0)
    {
      uint unorm_x = uint(floor(gl_FragCoord.x));
      uint unorm_y = uint(floor(gl_FragCoord.y));
      if (unorm_x >= ii[value].ii_x && unorm_x < ii[value].ii_x + ii[value].ii_w
          && unorm_y >= ii[value].ii_y && unorm_y < ii[value].ii_y + ii[value].ii_h)
      {
        // found correct image
        if (atomicExchange(buffers_to_draw[value], 1) == 0)
        {
          atomicAdd (instance_count, 1);

          uint old_value = 0xFFFFFFFF;
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
            if (atomicCompSwap (fg_zindex[cur_index], 0xFFFFFFFF, value) == 0xFFFFFFFF)
            {
              atomicAdd (fragment_data_length, 1);
              fg_zindex[cur_index] = value;
              break;
            }
          }

          vec2 fragTexCoord = vec2 (floor(gl_FragCoord.x - ii[value].ii_x)/(ii[value].ii_w-1)
                                    , floor(gl_FragCoord.y - ii[value].ii_y)/(ii[value].ii_h-1));
          vec4 color = texture(sampler2D(tex[value], samp), fragTexCoord);
          if (color.a == 1.0f)
            break;
        }
      }
    }
  }
  }
}



