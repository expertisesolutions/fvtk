
const uint arc_quadractic_component_type = 3;
const uint arc_cubic_component_type = 4;
#ifndef VERTEX_SHADER
const int screen_width = 1280;
const int screen_height = 1000;

float scale (uint v, uint size)
{
  return float(v)/float(size-1)*2 - 1.0f;
}  
#endif
struct arc_info
{
  uint unused0;
  uint ii_x, ii_y, ii_w, ii_h;
  //uint alpha_compositing;
  uint found_alpha;
  uint component_type;
  uint padding0;
  ivec2 p0, p1, p2, p3;
};

layout (std430, set = 2, binding = 0) READONLY buffer arc_infos
{
  arc_info array[];
} arc_information;

float quadractic_bezier_coordinate (float x, float y, float z, float t)
{
  float mt = (1-t);
  float mt2 = mt*mt;
  float t2 = t*t;
  return
          mt2      * x
    + 2 * mt  * t  * y
    +           t2 * z
    ;
}

float cubic_bezier_coordinate (float x, float y, float z, float w, float t)
{
  float mt = (1-t);
  float mt2 = mt*mt;
  float mt3 = mt2*mt;
  float t2 = t*t;
  float t3 = t2*t;
  float p =
          mt3 *      x
    + 3 * mt2 * t  * y
    + 3 * mt  * t2 * z
    +           t3 * w
    ;
  return p;
}

float generic_bezier_coordinate (bool cubic, float x, float y, float z, float w, float t)
{
  return cubic ? cubic_bezier_coordinate (x, y, z, w, t)
    : quadractic_bezier_coordinate (x, y, z, t);
}

#ifdef VERTEX_SHADER
layout (location = 3) out float arc_t;

uint get_instance_id_first (uint instance_id, uint component_id)
{
  uint instance_id_first = instance_id;
  while (instance_id_first != 0 && component_id == indirect_draw.component_id[instance_id_first])
    -- instance_id_first;
  return instance_id_first;
}

vec4 arc_vertex (uint instance_id, uint component_id, uint vid)
{
  bool is_cubic = component_id == arc_cubic_component_type;
  uint instance_id_first = get_instance_id_first (instance_id, component_id);
  uint segment_id = instance_id - instance_id_first;

  float t1 = float(segment_id/2)  /10.0f;
  float t2 = float(segment_id/2+1)/10.0f;

  vec2 p0 = arc_information.array[component_id].p0;
  vec2 p1 = arc_information.array[component_id].p1;
  vec2 p2 = arc_information.array[component_id].p2;
  vec2 p3 = arc_information.array[component_id].p3;

  vec2 src = vec2
    (
       generic_bezier_coordinate (is_cubic, p0.x, p1.x, p2.x, p3.x, t1)
     , generic_bezier_coordinate (is_cubic, p0.y, p1.y, p2.y, p3.y, t1)
    );
  vec2 dst = vec2
    (
       generic_bezier_coordinate (is_cubic, p0.x, p1.x, p2.x, p3.x, t2)
     , generic_bezier_coordinate (is_cubic, p0.y, p1.y, p2.y, p3.y, t2)
    );

  {
    if (abs(src.x - dst.x) <= 4)
    {
      if (src.x < dst.x) dst.x += 4;
      else src.x += 4;
    }
    else if (abs(src.y - dst.y) <= 4)
    {
      if (src.y < dst.y) dst.y += 4;
      else src.y += 4;
    }
  }

  vec2 scaled_src = vec2 (scale (uint(src.x), screen_width), scale (uint(src.y), screen_height));
  vec2 scaled_dst = vec2 (scale (uint(dst.x), screen_width), scale (uint(dst.y), screen_height));

  // draw left connection
  if (segment_id % 2 == 1 && segment_id != 1)
  {
    float t0 = float(segment_id/2-1)/10.0f;

    vec2 src0 = vec2
      (
         generic_bezier_coordinate (is_cubic, p0.x, p1.x, p2.x, p3.x, t0)
       , generic_bezier_coordinate (is_cubic, p0.y, p1.y, p2.y, p3.y, t0)
      );
    vec2 scaled_src0 = vec2 (scale (uint(src0.x), screen_width), scale (uint(src0.y), screen_height));
    
    float position_ratio [6] =
      {
         t1
       , 0.5f * (t1 - t0) + t0
       , 0.5f * (t2 - t1) + t1
       , t1
       , 0.5f * (t2 - t1) + t1
       , 0.5f * (t1 - t0) + t0
      };
    
    arc_t = position_ratio [vid];

    // src0, src
    // src , dst
    // src -> [src.x, src0.y] -> [dst .x, src.y]
    // src -> [src.x, dst .y] -> [src0.x, src.y]
    vec2 positions[6] =
     {
       {scaled_src .x, scaled_src .y}
     , {scaled_src .x, scaled_src0.y}
     , {scaled_dst .x, scaled_src .y}
     , {scaled_src .x, scaled_src .y}
     , {scaled_src .x, scaled_dst .y}
     , {scaled_src0.x, scaled_src .y}
     };

    return vec4(positions[vid], 0.0f, 1.0f);
  }
  else if (segment_id % 2 == 0)
  {
    float position_ratio [6] =
      {
         0.0f, 0.5f, 1.0f
       , 1.0f, 0.5f, 0.0f
      };
    
    arc_t = position_ratio [vid] * (t2 - t1) + t1;
  
    vec2 positions[6] =
     {
       {scaled_src.x, scaled_src.y}
     , {scaled_dst.x, scaled_src.y}
     , {scaled_dst.x, scaled_dst.y}
     , {scaled_dst.x, scaled_dst.y}
     , {scaled_src.x, scaled_dst.y}
     , {scaled_src.x, scaled_src.y}
     };

    return vec4(positions[vid], 0.0f, 1.0f);
  }
}
#else
layout (location = 3) in float arc_t;

void arc_quadractic_draw_fragment(uint component_id)
{
  vec2 p0 = arc_information.array[component_id].p0;
  vec2 p1 = arc_information.array[component_id].p1;
  vec2 p2 = arc_information.array[component_id].p2;

  float t = arc_t;
  vec2 pos =
    vec2
    (
       quadractic_bezier_coordinate (p0.x, p1.x, p2.x, arc_t)
     , quadractic_bezier_coordinate (p0.y, p1.y, p2.y, arc_t)
    );

  float dist = distance (pos, gl_FragCoord.xy);
  float border = 4;
  float color = dist > border ? 0.0 : (1 - dist/border);

  outColor = vec4 (color, 0.0f, 0.0f, 1.0f);
  //outColor = vec4 (arc_t, 0.0f, 0.5f, 1.0f);
}

void arc_cubic_draw_fragment(uint component_id)
{
  vec2 p0 = arc_information.array[component_id].p0;
  vec2 p1 = arc_information.array[component_id].p1;
  vec2 p2 = arc_information.array[component_id].p2;
  vec2 p3 = arc_information.array[component_id].p3;

  float t = arc_t;
  vec2 pos =
    vec2
    (
       cubic_bezier_coordinate (p0.x, p1.x, p2.x, p3.x, arc_t)
     , cubic_bezier_coordinate (p0.y, p1.y, p2.y, p3.y, arc_t)
    );

  float dist = distance (pos, gl_FragCoord.xy);
  float border = 4;
  float color = dist > border ? 0.0 : (1 - dist/border);

  //outColor = vec4 (color, 0.0f, 0.0f, 1.0f);
  outColor = vec4(arc_t, 0.0f, 0.5f, 1.0f);
}
#endif
