#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

const int tex_max_size = 4096;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 2) in flat uint InstanceID;

#include "set01.layout"
#include "set2.layout"

// output
layout(location = 0) out vec4 outColor;

in vec4 gl_FragCoord;

#include "image_ssbo.frag"
#include "button_ssbo.frag"
#include "rectangle_ssbo.frag"
#include "arc_quadractic_ssbo.frag"

void main() {
  uint component_index = indirect_draw.zindex[InstanceID];
  if (component_information.array[component_index].component_type == image_component_type)
  {
    image_draw_fragment (component_index);
    //outColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  }
  else if (component_information.array[component_index].component_type == button_component_type)
  {
    button_draw_fragment (component_index);
    //outColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
  }
  else if (component_information.array[component_index].component_type == rectangle_component_type)
  {
    rectangle_draw_fragment (component_index);
    //outColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
  }
  else if (component_information.array[component_index].component_type == 3)
  {
    arc_quadractic_draw_fragment (component_index);
  }
}

