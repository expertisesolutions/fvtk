#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

const int tex_max_size = 4096;
const int screen_width = 2000;
const int screen_height = 1500;
const uint num_segments = 20;

//layout(set = 0, binding = 0) uniform texture2D tex[tex_max_size];
//layout(set = 1, binding = 0) uniform sampler samp;

#include "set01.layout"
#include "set2.layout"
const uint arc_quadractic_component_type = 3;
const uint arc_cubic_component_type = 4;

layout(std430, push_constant) uniform PushConstants
{
  uint image_length;
} constants;

// // output
// layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord;

void main()
{
  // in bool gl_HelperInvocation
  if (!gl_HelperInvocation)
  {
  for (uint i = constants.image_length; i != 0; --i)
  {
    const uint value = i-1;
    if (indirect_draw.buffers_to_draw[value] == 0)
    {
      uint unorm_x = uint(floor(gl_FragCoord.x));
      uint unorm_y = uint(floor(gl_FragCoord.y));
      if (unorm_x >= component_information.array[value].ii_x && unorm_x < component_information.array[value].ii_x + component_information.array[value].ii_w
          && unorm_y >= component_information.array[value].ii_y && unorm_y < component_information.array[value].ii_y + component_information.array[value].ii_h)
      {
        // found correct image
        if (atomicExchange(indirect_draw.buffers_to_draw[value], 1) == 0)
        {
          const uint segments =
            ((component_information.array[value].component_type == arc_quadractic_component_type
              || component_information.array[value].component_type == arc_cubic_component_type)
             ? num_segments : 1);
          atomicAdd (indirect_draw.instance_count, segments);

          uint counter = 0;
          uint next_index = tex_max_size;
          while (counter != segments)
          {
            uint value = i-1;
            uint old_value = 0xFFFFFFFF;
            uint cur_index = next_index == tex_max_size ? 0 : next_index;

            while (true)
            {
              uint length = atomicAdd(indirect_draw.fragment_data_length, 0);
              while (cur_index != length)
              {
                old_value = atomicMin (indirect_draw.component_id[cur_index], value);
                ++cur_index;
                if (value < old_value)
                {
                  value = old_value;
                  if (next_index > cur_index)
                    next_index = cur_index;
                }
              }

              // theoretically after sequence
              if (atomicCompSwap (indirect_draw.component_id[cur_index], 0xFFFFFFFF, value) == 0xFFFFFFFF)
              {
                atomicAdd (indirect_draw.fragment_data_length, 1);
                indirect_draw.component_id[cur_index] = value;
                //indirect_draw.instance_id_start[cur_index] = cur_index - counter;
                ++counter;
                if (next_index > cur_index)
                  next_index = cur_index;
                break;
              }
            }
          }

          // vec2 fragTexCoord = vec2 (floor(gl_FragCoord.x - component_information.array[value].ii_x)/(component_information.array[value].ii_w-1)
          //                           , floor(gl_FragCoord.y - component_information.array[value].ii_y)/(component_information.array[value].ii_h-1));
          // vec4 color = texture(sampler2D(tex[value], samp), fragTexCoord);
          // if (color.a == 1.0f)
          break;
        }
      }
    }
  }
  }
}



