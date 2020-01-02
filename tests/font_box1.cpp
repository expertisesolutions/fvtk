//////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
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

#include <filesystem>

#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */
#include FT_GLYPH_H
#include FT_IMAGE_H
#include FT_OUTLINE_H

template <typename MoveTo, typename LineTo, typename ConicTo, typename CubicTo>
struct funcs
{
  MoveTo moveto;
  LineTo lineto;
  ConicTo conicto;
  CubicTo cubicto;
};

template <typename MoveTo, typename LineTo, typename ConicTo, typename CubicTo>
std::pair<FT_Outline_Funcs, void*> make_funcs (MoveTo&& moveto, LineTo&& lineto, ConicTo&& conicto, CubicTo&& cubicto)
{
  typedef struct funcs<MoveTo, LineTo, ConicTo, CubicTo> funcs_type;
  FT_Outline_Funcs f
    =
    {
       [] (FT_Vector const* to, void* user) -> int
       {
         auto& f = *static_cast<funcs_type*>(user);
         f.moveto (*to);
         return 0;
       }
     , [] (FT_Vector const* to, void* user) -> int
       {
         auto& f = *static_cast<funcs_type*>(user);
         f.lineto (*to);
         return 0;
       }
     , [] (FT_Vector const* control, FT_Vector const* to, void* user) -> int
       {
         auto& f = *static_cast<funcs_type*>(user);
         f.conicto (*control, *to);
         return 0;
       }
     , [] (FT_Vector const* control1, FT_Vector const* control2, FT_Vector const* to, void* user) -> int
       {
         auto& f = *static_cast<funcs_type*>(user);
         f.cubicto (*control1, *control2, *to);
         return 0;
       }
       , 0
       , -100
    };

  return std::make_pair(f, new funcs_type{std::forward<MoveTo>(moveto)
                                            , std::forward<LineTo>(lineto)
                                            , std::forward<ConicTo>(conicto)
                                            , std::forward<CubicTo>(cubicto)});
}

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

  FT_Face face;
  //std::string face_name ("/usr/share/fonts/liberation/LiberationSerif-Bold.ttf");
  std::string face_name ("/usr/share/fonts/TTF/DejaVuSans-Bold.ttf");
  auto error = FT_New_Face (library, face_name.c_str()
                            , 0, &face);
  if (error)
  {
    std::cout << "error " << error << std::endl;
    throw float{};
  }

  // error = FT_Set_Pixel_Sizes(face, 0, 200); /* set character size */
  
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
    unsigned int glyph_pos_count = 0;
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buffer, &glyph_pos_count);

    std::cout << "glyph count " << glyph_info_count << std::endl;

    if (FT_HAS_KERNING (face))
    {
      std::cout << "has kerning" << std::endl;
    }
    
    glyph_index = glyph_info[0].codepoint;
  }

  std::cout << "glyph_index is " << glyph_index << std::endl;

  if (FT_Error err = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP))
  {
    std::cout << "Error is " << (unsigned long)err << std::endl;
    throw -1;
  }

  {
    const FT_Fixed multiplier = 65536L;

    FT_Matrix matrix;

    matrix.xx = 1L * multiplier;
    matrix.xy = 0L * multiplier;
    matrix.yx = 0L * multiplier;
    matrix.yy = -1L * multiplier;

    FT_GlyphSlot slot = face->glyph;
    FT_Outline &outline = slot->outline;

    // FT_Outline_Transform(&outline, &matrix);
  }

  // FT_Glyph glyph;
  // if (FT_Get_Glyph (face->glyph, &glyph))
  //   throw -1.0;

  if (argc < 2)
  {
    std::cout << "Pass directory for resource path" << std::endl;
    return 1;
  }

  std::filesystem::path res_path (argv[1]);

  std::cout << "resource path " << res_path << std::endl;

  typedef ftk::ui::backend::vulkan_backend<ftk::ui::backend::uv::uv_loop, ftk::ui::backend::xlib_surface_backend<ftk::ui::backend::uv::uv_loop>> backend_type;
  backend_type backend({&loop});

  auto vulkan_window = backend.create_window(2000, 1500, res_path);
  
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

  {
  std::cout << "glyph->format: " << (unsigned int)face->glyph->format << std::endl;
  FT_Vector initial_pos = {0,0};

  unsigned int segments = 0;
  if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
  {
    FT_Outline_Funcs funcs;
    void* user;
    std::tie (funcs, user) = ::make_funcs
      (
         [&] (FT_Vector to)
         {
           std::cout << "moveto " << to.x << 'x' << to.y << std::endl;
           initial_pos = to;
         }
       , [&] (FT_Vector to)
         {
           //if (segments++ < 1)
           {
             std::cout << "lineto " << to.x << 'x' << to.y << std::endl;
             auto a = initial_pos, b = to;
             if (a.y > b.y)
               std::swap (a, b);
             w.append_arc_quadractic ({a.x, a.y}, {a.x, a.y}, {b.x, b.y});
             initial_pos = to;
           }
         }
       , [&] (FT_Vector control, FT_Vector to)
         {
           std::cout << "conicto " << to.x << 'x' << to.y << " with control " << control.x << 'x' << control.y << std::endl;
           w.append_arc_quadractic ({initial_pos.x, initial_pos.y}, {control.x, control.y}, {to.x, to.y});
           initial_pos = to;
         }
       , [&] (FT_Vector control1, FT_Vector control2, FT_Vector to)
         {
           std::cout << "conicto " << to.x << 'x' << to.y << " with control1 " << control1.x << 'x' << control1.y
                     << " and control2 " << control2.x << 'x' << control2.y << std::endl;
           w.append_arc_cubic ({initial_pos.x, initial_pos.y}, {control1.x, control1.y}, {control2.x, control2.y}, {to.x, to.y});
           initial_pos = to;
         }
       );

    std::cout << "decomposing" << std::endl;
    
    if (auto err = FT_Outline_Decompose (&face->glyph->outline
                                         , &funcs
                                         , user
                                         ))
    {
      std::cout << "Error decomposing " << (unsigned int)err << std::endl;
    }
    std::cout << "decomposed" << std::endl;
  }  
  }

  //w.append_arc_quadractic ({397, 0}, {397, 0}, {492, 272});
  //w.append_arc_quadractic ({0, 0}, {0, 0}, {1280, 1000});
  
  ftk::ui::backend::vulkan::draw_and_present (w, true);
  
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
