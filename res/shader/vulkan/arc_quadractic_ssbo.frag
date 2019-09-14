
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
layout (location = 3) out float arc_quadractic_t;

vec4 arc_quadractic_vertex (uint instance_id, uint component_id, uint vid)
{
    uint instance_id_first = instance_id;
    while (instance_id_first != 0 && component_id == indirect_draw.component_id[instance_id_first])
      -- instance_id_first;
    uint segment_id = instance_id - instance_id_first;
    float t1 = float(segment_id)/10.f;
    float t2 = float(segment_id+1)/10.0f;

    float border = 4;
    
    vec2 p0 = arc_quadractic_information.array[component_id].p0;
    vec2 p1 = arc_quadractic_information.array[component_id].p1;
    vec2 p2 = arc_quadractic_information.array[component_id].p2;

    float px1 = (1-t1) * (1-t1) * float(p0.x)
      + 2 * (1 - t1) * t1*float(p1.x)
      + t1 * t1 * float(p2.x) /*+ border*/;
    float py1 = (1-t1) * (1-t1) * float(p0.y)
      + 2 * (1 - t1) * t1*float(p1.y)
      + t1 * t1 * float(p2.y) /*+ border*/;

    float px2 = (1-t2) * (1-t2) * float(p0.x)
      + 2 * (1 - t2) * t2*float(p1.x)
      + t2 * t2 * float(p2.x) /*- border*/;
    float py2 = (1-t2) * (1-t2) * float(p0.y)
      + 2 * (1 - t2) * t2*float(p1.y)
      + t2 * t2 * float(p2.y) /*- border*/;

    float position_ratio [6] =
      {
         0.0f, 0.5f, 1.0f
       , 1.0f, 0.5f, 0.0f
      };
    
    arc_quadractic_t = position_ratio [vid] * (t2 - t1) + t1;

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
#else
layout (location = 3) in float arc_quadractic_t;

void arc_quadractic_draw_fragment(uint component_id) {

    vec2 p0 = arc_quadractic_information.array[component_id].p0;
    vec2 p1 = arc_quadractic_information.array[component_id].p1;
    vec2 p2 = arc_quadractic_information.array[component_id].p2;

  float t = arc_quadractic_t;
  vec2 pos =
    vec2
    (
     (1-t) * (1-t) * float(p0.x)
     + 2 * (1 - t) * t*float(p1.x)
     + t * t * float(p2.x)
     , (1-t) * (1-t) * float(p0.y)
     + 2 * (1 - t) * t*float(p1.y)
     + t * t * float(p2.y)
    );

  float dist = distance (pos, gl_FragCoord.xy);
  float border = 4;
  float color = dist > border ? 0.0 : (1 - dist/border);

  outColor = vec4 (color, 0.0f, 0.0f, 1.0f);
}
#endif
