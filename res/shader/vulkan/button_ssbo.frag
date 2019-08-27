
const uint button_component_type = 1;

struct button_info
{
  uint text_texture_descriptor_index;
  uint ii_x, ii_y, ii_w, ii_h;
  uint alpha_compositing;
  uint found_alpha;
  uint component_type;

  
};

layout (std430, set = 2, binding = 0) buffer button_infos
{
  button_info array[];
} button_information;

void button_draw_fragment(uint zindex) {
  outColor = vec4 (1.0f, 0.0f, 0.0f, 1.0f);
}
