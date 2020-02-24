//////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#include <ftk/ui/backend/uv/uv_loop.hpp>
#include <ftk/ui/toplevel_window.hpp>
#include <ftk/ui/backend/vulkan/text.hpp>

#include <ftk/ui/backend/vulkan_backend.hpp>
#include <ftk/ui/backend/x11_base_backend.hpp>
#include <ftk/ui/backend/uv/uv_loop.hpp>
#include <ftk/ui/backend/uv/timer.hpp>
#include <ftk/ui/backend/vulkan/draw.hpp>
#include <ftk/ui/backend/vulkan_backend.ipp>
#include <ftk/ui/text/fontconfig.hpp>
#include <ftk/ui/text/vector/glyph_cache.hpp>

#include <filesystem>

#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */
#include FT_GLYPH_H
#include FT_IMAGE_H
#include FT_OUTLINE_H


template <typename T, typename U>
auto transform_x_pos (T p, U width) { return p/2; }

template <typename T, typename U>
auto transform_y_pos (T p, U height) { return height - p/2; }

int main(int argc, char* argv[])
{
  uv_loop_t loop;
  uv_loop_init (&loop);

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

  ftk::ui::fontconfig fontconfig;
  auto face_name = fontconfig.find_font ();

  FT_Face face;
  auto error = FT_New_Face (library, face_name.c_str()
                            , 0, &face);
  if (error)
  {
    std::cout << "error " << error << std::endl;
    throw float{};
  }

  FT_UInt glyph_index = 0;
  {
    hb_font_t* hb_font = ::hb_ft_font_create (face, nullptr);
    if (!hb_font)
      throw long{};

    hb_buffer_t* buffer;
    buffer = hb_buffer_create ();

    const std::string text = "a";
    hb_buffer_add_utf8 (buffer,
                        text.c_str(),
                        text.size(),
                        0,
                        text.size());
    hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);

    hb_feature_t feature;
    // The leak most probably doesn't depend on the type of the feature.
    feature.tag = hb_tag_from_string("kern", 4);
    feature.value = 0;
    feature.start = 0;
    feature.end = (unsigned int) -1;
    //int num_features = 1;
    hb_shape(hb_font, buffer, &feature, 1);

    unsigned int glyph_info_count = 0;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, &glyph_info_count);
    //unsigned int glyph_pos_count = 0;
    //hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_pos_count);

    std::cout << "glyph count " << glyph_info_count << std::endl;

    if (FT_HAS_KERNING (face))
    {
      std::cout << "has kerning" << std::endl;
    }
    
    glyph_index = glyph_info[0].codepoint;
  }

  std::cout << "glyph_index is " << glyph_index << std::endl;

  ftk::ui::ft::glyph_cache glyph_cache;
  load_glyph (glyph_cache, face, glyph_index);

  if (argc < 2)
  {
    std::cout << "Pass directory for resource path" << std::endl;
    return 1;
  }

  std::filesystem::path res_path (argv[1]);

  std::cout << "resource path " << res_path << std::endl;

  typedef ftk::ui::backend::vulkan_backend<ftk::ui::backend::uv::uv_loop, ftk::ui::backend::xlib_surface_backend<ftk::ui::backend::uv::uv_loop>> backend_type;
  backend_type backend({&loop});

  const auto width = 2000, height = 1500;
  auto vulkan_window = backend.create_window(width, height, res_path);
  
  typedef pc::inplace_executor_t executor_type;
  ftk::ui::backend::vulkan::submission_pool<executor_type>
    vulkan_submission_pool (vulkan_window.voutput.device
                            , &vulkan_window.queues
                            , pc::inplace_executor
                            , 1 /* thread count */);

  auto empty_image = ftk::ui::backend::vulkan::load_empty_image_view
    (vulkan_window.voutput.device, vulkan_window.voutput.physical_device
     , vulkan_submission_pool).get();
  
  ftk::ui::toplevel_window<backend_type&> w(vulkan_window, res_path, empty_image.image_view);

  std::cout << "w width " << w.window.voutput.swapChainExtent.width << std::endl;

  draw_glyph (glyph_cache[glyph_index], w, {0,0}, {100, 100});
  
  ftk::ui::backend::vulkan::draw_and_present (w);

  ftk::ui::backend::uv::timer_wait
    (backend.loop
     , 205000
     , [&] (uv_timer_t* timer)
       {
         uv_stop (&loop);
       });  
  
  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);
}
