#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

const int tex_max_size = 4096;
const int screen_width = 1280;
const int screen_height = 1000;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 2) out flat uint InstanceID;

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

float scale (uint v, uint size)
{
  return float(v)/float(size-1)*2 - 1.0f;
}  

#include "set2.layout"
#include "arc_ssbo.frag"

void main()
{
  uint component_id = indirect_draw.component_id[gl_InstanceIndex];

  switch (component_information.array[component_id].component_type)
  {
  default:
    {
      uint x = component_information.array[component_id].ii_x;
      uint y = component_information.array[component_id].ii_y;
      uint w = component_information.array[component_id].ii_w;
      uint h = component_information.array[component_id].ii_h;
      uint x2 = x + w-1;
      uint y2 = x + h-1;
      float scaled_x1 = scale (x, screen_width);
      float scaled_y1 = scale (y, screen_height);
      float scaled_x2 = scale (x + w, screen_width);
      float scaled_y2 = scale (y + h, screen_height);
  
      vec4 positions[6] =
        {
           {scaled_x1, scaled_y1, 0.0f, 1.0f}
         , {scaled_x2, scaled_y1, 0.0f, 1.0f}
         , {scaled_x2, scaled_y2, 0.0f, 1.0f}
         , {scaled_x2, scaled_y2, 0.0f, 1.0f}
         , {scaled_x1, scaled_y2, 0.0f, 1.0f}
         , {scaled_x1, scaled_y1, 0.0f, 1.0f}
        };
      uint vid = gl_VertexIndex;
      gl_Position = positions[vid];
      fragTexCoord = texcoord[vid];
      if (gl_VertexIndex == 0)
        InstanceID = gl_InstanceIndex;
    }
    break;
  case arc_quadractic_component_type:
  case arc_cubic_component_type:
    {
      uint vid = gl_VertexIndex;
      gl_Position = arc_vertex (gl_InstanceIndex, component_id, vid);

      if (gl_VertexIndex == 0)
        InstanceID = gl_InstanceIndex;
    }
    break;
  };
}
