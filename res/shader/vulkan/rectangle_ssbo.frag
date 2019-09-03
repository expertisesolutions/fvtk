
const uint rectangle_component_type = 2;

struct rectangle_info
{
  uint unused0;
  uint ii_x, ii_y, ii_w, ii_h;
  //uint alpha_compositing;
  uint found_alpha;
  uint component_type;
  uint padding0;
  vec4 fill_color;  
};

layout (std430, set = 2, binding = 0) buffer rectangle_infos
{
  rectangle_info array[];
} rectangle_information;

void rectangle_draw_fragment(uint zindex) {
  outColor = rectangle_information.array[zindex].fill_color;
  //outColor = vec4 (1.0f, 0.0f, 0.0f, 1.0f);
}
