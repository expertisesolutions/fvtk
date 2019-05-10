///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_ADD_TEXT_HPP
#define FASTDRAW_OUTPUT_VULKAN_ADD_TEXT_HPP

#include <fastdraw/output/vulkan/vulkan_output_info.hpp>
#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>
#include <fastdraw/object/text.hpp>

#include <vector>
#include <stdexcept>
#include <cstring>

#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */

namespace fastdraw { namespace output { namespace vulkan {

template <typename Point, typename String, typename Color, typename WindowingBase>
vulkan_draw_info create_output_specific_object (vulkan_output_info<WindowingBase>& output, object::fill_text<Point, String, Color> const& text
                                               , VkPipelineRasterizationStateCreateInfo rasterizer
                                               = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO // sType
                                                  , nullptr                                                  // pNext
                                                  , 0u                                                       // flags
                                                  , VK_FALSE                                                 // depthClampEnable
                                                  , VK_FALSE                                                 // rasterizerDiscardEnable
                                                  , VK_POLYGON_MODE_FILL                                     // polygonMode
                                                  , VK_CULL_MODE_BACK_BIT                                    // cullMode
                                                  , VK_FRONT_FACE_CLOCKWISE                                  // frontFace
                                                  , VK_FALSE                                                 // depthBiasEnable
                                                  , 0.0f                                                     // depthBiasConstantFactor
                                                  , 0.0f                                                     // depthBiasClamp
                                                  , 0.0f                                                     // depthBiasSlopeFactor
                                                  , 1.0f                                                     // lineWidth
                                               }
                                               , VkPipelineMultisampleStateCreateInfo multisampling
                                               = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO // sType
                                                   , nullptr                                               // pNext
                                                   , 0u                                                    // flags
                                                   , VK_SAMPLE_COUNT_1_BIT                                 // rasterizationSamples
                                                   , VK_FALSE                                              // sampleShadingEnable
                                                   , 1.0f                                                  // minSampleShading
                                                   , nullptr                                               // pSampleMask
                                                   , VK_FALSE                                              // alphaToCoverageEnable
                                                   , VK_FALSE                                              // alphaToOneEnable
                                               }
                                               , VkPipelineColorBlendAttachmentState colorBlendAttachment
                                               = {VK_TRUE                                                  // blendEnable
                                                  , VK_BLEND_FACTOR_ONE                                    // srcColorBlendFactor
                                                  , VK_BLEND_FACTOR_ZERO                                   // dstColorBlendFactor
                                                  , VK_BLEND_OP_ADD                                        // colorBlendOp
                                                  , VK_BLEND_FACTOR_ONE                                    // srcAlphaBlendFactor
                                                  , VK_BLEND_FACTOR_ZERO                                   // dstAlphaBlendFactor
                                                  , VK_BLEND_OP_ADD                                        // alphaBlendOp
                                                  , VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT    // colorWriteMask
                                                  | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT    // colorWriteMask
                                               }
                                               , VkPipelineColorBlendStateCreateInfo colorBlending
                                               = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO // sType
                                                   , nullptr                                               // pNext
                                                   , 0u                                                    // flags
                                                   , VK_FALSE                                              // logicOpEnable
                                                   , VK_LOGIC_OP_COPY                                      // logicOp
                                                   , 1                                                     // attachmentCount
                                                   , nullptr                                               // pAttachments = &colorBlendAttachment 
                                                   , {1.0f, 1.0f, 1.0f, 0.5f}                              // blendConstants
                                               }
                                               )
{
  // hb_font_t* font = hb_ft_font_create ();

  static FT_Library  library;
  static bool library_initialized;

  if (!library_initialized)
  {
    auto error = FT_Init_FreeType( &library );
    if (error)
    {
      throw -1;
    }
  }

  FT_Face face;
  auto error = FT_New_Face (library, text.face.c_str()
                            , 0, &face);
  if (error)
  {
    std::cout << "error " << error << std::endl;
    throw float{};
  }

  error = FT_Set_Pixel_Sizes(face, 0, 40); /* set character size */
  if (error)
    throw uint32_t{};

  // hb_face_t* hb_face = ::hb_ft_face_create (face, nullptr);
  // if (!hb_face)
  //   throw -1;

  hb_font_t* hb_font = ::hb_ft_font_create (face, nullptr);
  if (!hb_font)
    throw long{};

  hb_buffer_t* buffer;
  buffer = hb_buffer_create ();

  hb_buffer_add_utf8 (buffer,
                      text.text.c_str(),
                      text.text.size(),
                      0,
                      text.text.size());
  hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);

  hb_feature_t feature;
  // The leak most probably doesn't depend on the type of the feature.
  feature.tag = hb_tag_from_string("kern", 4);
  feature.value = 0;
  feature.start = 0;
  feature.end = (unsigned int) -1;
  //int num_features = 1;
  hb_shape(hb_font, buffer, &feature, 1);


  int glyph_count = hb_buffer_get_length(buffer);
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, 0);

  std::cout << "glyph count " << glyph_count << std::endl;

  if (FT_HAS_KERNING (face))
  {
    std::cout << "has kerning" << std::endl;
  }
  
  int pen_x = 0;
  
  for (int i = 0; i != glyph_count; ++i) 
  {
    FT_UInt glyph_index = glyph_info[i].codepoint;
    FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    std::cout << "glyph index " << glyph_index << std::endl;
    //OutputDebugString(dbg_info);
 
    /* convert to an anti-aliased bitmap */
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    // FreeTypeDrawBitmap(dc, &face->glyph->bitmap, pen_x +
    //                    face->glyph->bitmap_left,
    //                    yBaseLine - ft_face->glyph->bitmap_top);
    pen_x += face->glyph->advance.x >> 6;
  }

  
  
  return {};
}

// template <typename Point, typename Color>
// vulkan_draw_info replace_push_constants (vulkan_draw_info& info, object::fill_triangle<Point, Color> const& triangle)
// {
//   // std::cout << "replace_push_constants" << std::endl;
//     std::array<float, 16> values
//       ({triangle.p1.x, triangle.p1.y, 0.0f, triangle.fill_color.r
//         , triangle.p2.x, triangle.p2.y, 0.0f, triangle.fill_color.g
//         , triangle.p3.x, triangle.p3.y, 0.0f, triangle.fill_color.b
//         , /*triangle.fill_color.a*/0.0f});
    
//     std::memcpy(info.push_constants.data(), values.data(), info.push_constants.size());
//     return info;
// }

} } }
  

#endif
