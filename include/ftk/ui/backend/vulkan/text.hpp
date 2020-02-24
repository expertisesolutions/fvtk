///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_BACKEND_VULKAN_TEXT_HPP
#define FTK_FTK_BACKEND_VULKAN_TEXT_HPP

#include <ftk/ui/backend/vulkan/image.hpp>
#include <ftk/ui/backend/vulkan/submission_pool.hpp>

#include <future>
#include <filesystem>

#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */

namespace ftk { namespace ui { namespace backend { namespace vulkan {

pc::future<vulkan_image> load_text (FT_Library freetype, FT_Face face
                                    , int32_t height
                                    , VkDevice device, VkPhysicalDevice physical_device, std::string_view text)
{
  if (FT_HAS_KERNING (face))
  {
    std::cout << "face has kerning support" << std::endl;
  }
  else
  {
    std::cout << "NOT face has NOT kerning support" << std::endl;
  }
  // should do binary search
  auto fixup = height / 10;
  do
  {
    FT_Error error;
      error = FT_Set_Pixel_Sizes(face, 0, height - fixup); /* set character size */
    // else
    //   error = FT_Set_Char_Size (face, 20 << 6, 0, 300, 0);
    if (error)
      throw uint32_t{};

    std::cout << "Face height " << (face->size->metrics.height >> 6) << std::endl;

    fixup += height / 20;
  }
  while ( (face->size->metrics.height >> 6) > height - 2);

  hb_font_t* hb_font = ::hb_ft_font_create (face, nullptr);
  if (!hb_font)
    throw -1;

  hb_buffer_t* buffer;
  buffer = hb_buffer_create ();

  hb_buffer_add_utf8 (buffer,
                      text.data(),
                      text.size(),
                      0,
                      text.size());
  hb_buffer_guess_segment_properties (buffer);

  hb_feature_t feature;
  feature.tag = hb_tag_from_string("kern", 4);
  feature.value = 0;
  feature.start = 0;
  feature.end = (unsigned int) -1;
  //int num_features = 1;

  // calculate harfbuzz text position
  hb_shape(hb_font, buffer, &feature, 1);

  unsigned int glyph_info_count = 0;
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_info_count);
  unsigned int glyph_pos_count = 0;
  hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_pos_count);

  unsigned int total_size = 0;

  for (unsigned int i = 0; i != glyph_info_count; ++i) 
  {
    FT_UInt glyph_index = glyph_info[i].codepoint;
    FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    std::cout << "glyph index " << glyph_index << std::endl;

    std::cout << "position {advance_x: " << (glyph_pos[i].x_advance >> 6)
              << ", advance_y: " << (glyph_pos[i].y_advance >> 6)
              << ", offset_x: " << (glyph_pos[i].x_offset >> 6)
              << ", offset_y: " << (glyph_pos[i].y_offset >> 6)
              << ", advance.x: " << (face->glyph->advance.x >> 6)
              << ", advance.y: " << (face->glyph->advance.y >> 6)
              << ", lsb_delta: " << face->glyph->lsb_delta
              << ", rsb_delta: " << face->glyph->rsb_delta
              << "}\n";

    total_size += glyph_pos[i].x_advance >> 6;
  }

  /*
  {
    double current_x = 0;
    double current_y = 0;
    for (unsigned int i = 0; i != glyph_info_count; i++)
    {
      hb_codepoint_t gid   = glyph_info[i].codepoint;
      unsigned int cluster = glyph_info[i].cluster;
      double x_position = current_x + glyph_pos[i].x_offset / 64.;
      double y_position = current_y + glyph_pos[i].y_offset / 64.;


      char glyphname[32];
      hb_font_get_glyph_name (hb_font, gid, glyphname, sizeof (glyphname));

      printf ("glyph='%s'	cluster=%d	position=(%g,%g)\n",
	      glyphname, cluster, x_position, y_position);

      current_x += glyph_pos[i].x_advance / 64.;
      current_y += glyph_pos[i].y_advance / 64.;
    }
  }
  */

  
  
  std::cout << "total size " << total_size << std::endl;
  return {};
}

} } } }

#endif
