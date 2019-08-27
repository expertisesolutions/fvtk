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

void main() {
  uint zindex = indirect_draw.zindex[InstanceID];
  if (component_information.array[zindex].component_type == image_component_type)
  {
    image_draw_fragment (zindex);
  }
  else if (component_information.array[zindex].component_type == button_component_type)
  {
    button_draw_fragment (zindex);
  }
  else if (component_information.array[zindex].component_type == rectangle_component_type)
  {
    rectangle_draw_fragment (zindex);
  }
}

