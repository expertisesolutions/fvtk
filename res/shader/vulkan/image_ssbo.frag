
const uint image_component_type = 0;

struct image_info
{
  uint texture_descriptor_index;
  uint ii_x, ii_y, ii_w, ii_h;
  uint alpha_compositing;
  uint found_alpha;
  uint component_type;
  uint padding[4];
};

layout (std430, set = 2, binding = 0) buffer image_infos
{
  image_info array[];
} image_information;

void image_draw_fragment(uint zindex) {
  outColor = texture(sampler2D(tex[image_information.array[zindex].texture_descriptor_index], samp), fragTexCoord);
}
