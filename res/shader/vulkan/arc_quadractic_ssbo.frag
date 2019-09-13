
const uint arc_quadractic_component_type = 3;
#ifndef VERTEX_SHADER
const int screen_width = 1280;
const int screen_height = 1000;

float scale (uint v, uint size)
{
  return float(v)/float(size-1)*2 - 1.0f;
}  
#endif
struct arc_quadractic_info
{
  uint unused0;
  uint ii_x, ii_y, ii_w, ii_h;
  //uint alpha_compositing;
  uint found_alpha;
  uint component_type;
  uint padding0;
  ivec2 p0, p1, p2;  
};

layout (std430, set = 2, binding = 0) buffer arc_quadractic_infos
{
  arc_quadractic_info array[];
} arc_quadractic_information;

#ifdef VERTEX_SHADER
layout (location = 3) out flat float arc_quadractic_t1;
layout (location = 4) out flat float arc_quadractic_t2;
#else
layout (location = 3) in flat float arc_quadractic_t1;
layout (location = 4) in flat float arc_quadractic_t2;
#endif

vec4 arc_quadractic_vertex (uint instance_id, uint component_id, uint vid)
{
    uint instance_id_first = instance_id;
    while (instance_id_first != 0 && component_id == indirect_draw.component_id[instance_id_first])
      -- instance_id_first;
    uint segment_id = instance_id - instance_id_first;
    float t1 = float(segment_id)/10.f;
    float t2 = float(segment_id+1)/10.0f;

    vec2 p0 = arc_quadractic_information.array[component_id].p0;
    vec2 p1 = arc_quadractic_information.array[component_id].p1;
    vec2 p2 = arc_quadractic_information.array[component_id].p2;

    float px1 = (1-t1) * (1-t1) * float(p0.x)
      + 2 * (1 - t1) * t1*float(p1.x)
      + t1 * t1 * float(p2.x);
    float py1 = (1-t1) * (1-t1) * float(p0.y)
      + 2 * (1 - t1) * t1*float(p1.y)
      + t1 * t1 * float(p2.y);

    float px2 = (1-t2) * (1-t2) * float(p0.x)
      + 2 * (1 - t2) * t2*float(p1.x)
      + t2 * t2 * float(p2.x);
    float py2 = (1-t2) * (1-t2) * float(p0.y)
      + 2 * (1 - t2) * t2*float(p1.y)
      + t2 * t2 * float(p2.y);

    arc_quadractic_t1 = t1;
    arc_quadractic_t2 = t2;

    // uint x = component_information.array[component_id].ii_x;
    // uint y = component_information.array[component_id].ii_y;
    // uint w = component_information.array[component_id].ii_w;
    // uint h = component_information.array[component_id].ii_h;
    // uint x2 = x + w-1;
    // uint y2 = x + h-1;
    float scaled_x1 = scale (uint(px1), screen_width);
    float scaled_y1 = scale (uint(py1), screen_height);
    float scaled_x2 = scale (uint(px2), screen_width);
    float scaled_y2 = scale (uint(py2), screen_height);
  
    vec4 positions[6] =
    {
       {scaled_x1, scaled_y1, 0.0f, 1.0f}
     , {scaled_x2, scaled_y1, 0.0f, 1.0f}
     , {scaled_x2, scaled_y2, 0.0f, 1.0f}
     , {scaled_x2, scaled_y2, 0.0f, 1.0f}
     , {scaled_x1, scaled_y2, 0.0f, 1.0f}
     , {scaled_x1, scaled_y1, 0.0f, 1.0f}
    };

    return positions[vid];
}
#ifndef VERTEX_SHADER
void arc_quadractic_draw_fragment(uint zindex) {
  //outColor = arc_quadractic_information.array[zindex].fill_color;
  //outColor = 
  outColor = vec4 (1.0f, 0.0f, 0.0f, 1.0f);
}
#endif
