
const uint arc_quadractic_component_type = 3;
const uint arc_cubic_component_type = 4;
const int line_thickness = 1;
const uint num_segments = 20;
#ifndef VERTEX_SHADER
const int screen_width = 1280;
const int screen_height = 1000;
#endif
float coord_scale (uint v, uint size)
{
  return float(v)/float(size-1)*2 - 1.0f;
}  

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

float linear_bezier_coordinate (float x, float y, float t)
{
  float mt = (1-t);
  return
      mt * x
    +  t * y
    ;
}

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
layout (location = 3) out noperspective float arc_t;
layout (location = 4) out flat float t1t1;
layout (location = 5) out flat float t1t2;

uint get_instance_id_first (uint instance_id, uint component_id)
{
  uint instance_id_first = instance_id;
  while (instance_id_first != 0 && component_id == indirect_draw.component_id[instance_id_first])
    -- instance_id_first;
  return instance_id_first;
}

vec4 arc_vertex (uint instance_id, uint component_id, uint vid)
{
  bool is_cubic = arc_information.array[component_id].component_type == arc_cubic_component_type;
  uint instance_id_first = get_instance_id_first (instance_id, component_id);
  uint segment_id = instance_id - instance_id_first;

  float t1 = float(segment_id)  /float(num_segments);
  t1t1 = t1;
  float t2 = float(segment_id+1)/float(num_segments);
  t1t2 = t2;

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

  vec2 src_normal, dst_normal;

  if (is_cubic)
  {
    vec2 dp0 = vec2(3*(p1.x - p0.x), 3*(p1.y - p0.y));
    vec2 dp1 = vec2(3*(p2.x - p1.x), 3*(p2.y - p1.y));
    vec2 dp2 = vec2(3*(p3.x - p2.x), 3*(p3.y - p2.y));
    src_normal =
     normalize(vec2(
       - quadractic_bezier_coordinate (dp0.y, dp1.y, dp2.y, t1)
       , quadractic_bezier_coordinate (dp0.x, dp1.x, dp2.x, t1)
     ));
    dst_normal =
      normalize(vec2(
       - quadractic_bezier_coordinate (dp0.y, dp1.y, dp2.y, t2)
       , quadractic_bezier_coordinate (dp0.x, dp1.x, dp2.x, t2)
      ));
  }
  else
  {
    vec2 dp0 = vec2(2*(p1.x - p0.x), 2*(p1.y - p0.y));
    vec2 dp1 = vec2(2*(p2.x - p1.x), 2*(p2.y - p1.y));
    src_normal = normalize(vec2 (-linear_bezier_coordinate (dp0.y, dp1.y, t1)
                                 , linear_bezier_coordinate (dp0.x, dp1.x, t1)));
    dst_normal = normalize(vec2 (-linear_bezier_coordinate (dp0.y, dp1.y, t2)
                                 , linear_bezier_coordinate (dp0.x, dp1.x, t2)));
  }

  vec2 scaled_src = vec2 (coord_scale (uint(src.x), screen_width), coord_scale (uint(src.y), screen_height));
  vec2 scaled_dst = vec2 (coord_scale (uint(dst.x), screen_width), coord_scale (uint(dst.y), screen_height));

  vec2 scaled_lt = vec2 (line_thickness*100/float(screen_width-1), line_thickness*100/float(screen_height-1));
  vec2 clockwise_positions[6] =
      {
         scaled_src + scaled_lt *   src_normal , scaled_dst + scaled_lt *   dst_normal , scaled_dst + scaled_lt * (-dst_normal)
       , scaled_dst + scaled_lt * (-dst_normal), scaled_src + scaled_lt * (-src_normal), scaled_src + scaled_lt *   src_normal
      };
  float arct_positions[6] = {t1, t2, t2, t2, t1, t1};
  arc_t = arct_positions[vid];
  return vec4(clockwise_positions[vid], 0.0f, 1.0f);
}
#else
layout (location = 3) in noperspective float arc_t;
layout (location = 4) in flat float t1t1;
layout (location = 5) in flat float t1t2;

vec2 intersect(vec2 p, vec2 v, vec2 q, vec2 u)
{
  vec3 a = cross(vec3(v, 0), vec3(u, 0));

  if(a.x == 0 && a.y == 0 && a.z == 0)
    return vec2(0.0,0.0);

  vec3 b = cross (vec3(q - p, 0), vec3(u, 0));

  float t;
  if(a.x != 0)
    t = b.x / a.x;
  else if(a.y != 0)
    t = b.y / a.y;
  else if(a.z != 0)
    t = b.z / a.z;

  return p + (t * v);
}

void arc_draw_fragment(uint component_id)
{
  bool is_cubic = arc_information.array[component_id].component_type == arc_cubic_component_type;

  vec2 p0 = arc_information.array[component_id].p0;
  vec2 p1 = arc_information.array[component_id].p1;
  vec2 p2 = arc_information.array[component_id].p2;
  vec2 p3 = arc_information.array[component_id].p3;

  float t = arc_t;
  vec2 pos =
    vec2 (
       generic_bezier_coordinate (is_cubic, p0.x, p1.x, p2.x, p3.x, t)
     , generic_bezier_coordinate (is_cubic, p0.y, p1.y, p2.y, p3.y, t)
    );

  vec2 tangent, normal;

  if (is_cubic)
  {
    vec2 dp0 = vec2(3*(p1.x - p0.x), 3*(p1.y - p0.y));
    vec2 dp1 = vec2(3*(p2.x - p1.x), 3*(p2.y - p1.y));
    vec2 dp2 = vec2(3*(p3.x - p2.x), 3*(p3.y - p2.y));
    tangent =
     vec2(
          quadractic_bezier_coordinate (dp0.x, dp1.x, dp2.x, t)
          , quadractic_bezier_coordinate (dp0.y, dp1.y, dp2.y, t)
     );
    normal =
     vec2(
          - quadractic_bezier_coordinate (dp0.y, dp1.y, dp2.y, t)
          , quadractic_bezier_coordinate (dp0.x, dp1.x, dp2.x, t)
     );
  }
  else
  {
    vec2 dp0 = vec2(2*(p1.x - p0.x), 2*(p1.y - p0.y));
    vec2 dp1 = vec2(2*(p2.x - p1.x), 2*(p2.y - p1.y));
    tangent = vec2 (linear_bezier_coordinate (dp0.x, dp1.x, t)
                    , linear_bezier_coordinate (dp0.y, dp1.y, t));
    normal = vec2 (- linear_bezier_coordinate (dp0.y, dp1.y, t)
                   , linear_bezier_coordinate (dp0.x, dp1.x, t));
  }

  vec2 normal_intersect = intersect (pos, tangent, gl_FragCoord.xy, normal);

  if (normal_intersect.x >= gl_FragCoord.x - 0.5 && normal_intersect.x < gl_FragCoord.x + 0.5
      && normal_intersect.y >= gl_FragCoord.y -0.5 && normal_intersect.y < gl_FragCoord.y + 0.5)
    outColor = vec4 (0.0f, 1.0f, 0.0f, 1.0f);
  else/* if (abs(t - t1t1) <= 0.001f)
    outColor = vec4 (0.0f, 0.0f, 1.0f, 1.0f);
    else*/
    //outColor = vec4((t - t1t1) / (t1t2 - t1t1), 0.0f, 1- ( (t - t1t1) / (t1t2 - t1t1)), 1.0f);
    discard;
}
#endif
