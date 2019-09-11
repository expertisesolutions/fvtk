
const uint arc_quadractic_component_type = 2;

struct arc_quadractic_info
{
  uint unused0;
  uint ii_x, ii_y, ii_w, ii_h;
  //uint alpha_compositing;
  uint found_alpha;
  uint component_type;
  uint padding0;
  vec2 p0, p1, p2;  
};

layout (std430, set = 2, binding = 0) buffer arc_quadractic_infos
{
  arc_quadractic_info array[];
} arc_quadractic_information;

void arc_quadractic_draw_fragment(uint zindex) {
  //outColor = arc_quadractic_information.array[zindex].fill_color;
  //outColor = 
  outColor = vec4 (1.0f, 0.0f, 0.0f, 1.0f);
}
