//////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_VECTOR_GLYPH_CACHE_HPP

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */
#include FT_GLYPH_H
#include FT_IMAGE_H
#include FT_OUTLINE_H

namespace ftk { namespace ui { namespace ft {

namespace detail {

template <typename MoveTo, typename LineTo, typename ConicTo, typename CubicTo>
struct funcs
{
  MoveTo moveto;
  LineTo lineto;
  ConicTo conicto;
  CubicTo cubicto;
};

template <typename Func>
void funcs_free (void* func)
{
  delete static_cast<Func*>(func);
}

template <typename MoveTo, typename LineTo, typename ConicTo, typename CubicTo>
std::pair<FT_Outline_Funcs, std::unique_ptr<void, void(*)(void*)>> make_funcs (MoveTo&& moveto, LineTo&& lineto, ConicTo&& conicto, CubicTo&& cubicto)
{
  typedef struct funcs<MoveTo, LineTo, ConicTo, CubicTo> funcs_type;
  FT_Outline_Funcs f
    =
    {
       [] (FT_Vector const* to, void* user) -> int
       {
         std::cout << "moveto" << std::endl;
         auto& f = *static_cast<funcs_type*>(user);
         f.moveto (*to);
         return 0;
       }
     , [] (FT_Vector const* to, void* user) -> int
       {
         std::cout << "lineto" << std::endl;
         auto& f = *static_cast<funcs_type*>(user);
         f.lineto (*to);
         return 0;
       }
     , [] (FT_Vector const* control, FT_Vector const* to, void* user) -> int
       {
         std::cout << "conicto" << std::endl;
         auto& f = *static_cast<funcs_type*>(user);
         f.conicto (*control, *to);
         return 0;
       }
     , [] (FT_Vector const* control1, FT_Vector const* control2, FT_Vector const* to, void* user) -> int
       {
         std::cout << "cubicto" << std::endl;
         auto& f = *static_cast<funcs_type*>(user);
         f.cubicto (*control1, *control2, *to);
         return 0;
       }
       , 0
       , 0
    };

  std::unique_ptr<void, void(*)(void*)> p(new funcs_type{std::forward<MoveTo>(moveto)
                                                           , std::forward<LineTo>(lineto)
                                                           , std::forward<ConicTo>(conicto)
                                                           , std::forward<CubicTo>(cubicto)}
    , static_cast<void(*)(void*)>(&detail::funcs_free<funcs_type>));
  return std::make_pair(f, std::move(p));
}

}  
      
struct glyph_cache
{
  struct glyph
  {
    struct line
    {
      fastdraw::point<int> from, to;
    };
    struct quadractic
    {
      fastdraw::point<int> from, control, to;
    };
    struct cubic
    {
      fastdraw::point<int> from, control1, control2, to;
    };
    typedef std::variant<line, quadractic, cubic> subcontour_type;
    struct contours
    {
      std::vector<subcontour_type> subcontours;
    };

    bool empty () const
    {
      return contours.empty();
    }
    
    std::vector<contours> contours;
  };
  
  mutable std::vector<glyph> glyphs;

  bool exists (FT_UInt glyph_index) const
  {
    return glyphs.size() > glyph_index
      && !glyphs[glyph_index].empty();
  }

  glyph& operator[] (FT_UInt glyph_index)
  {
    if (glyphs.size() <= glyph_index)
      glyphs.resize (glyph_index + 1);
    return glyphs[glyph_index];
  }
  glyph const& operator[] (FT_UInt glyph_index) const
  {
    if (glyphs.size() <= glyph_index)
      glyphs.resize (glyph_index + 1);
    return glyphs[glyph_index];
  }
};

namespace detail {

template <typename Toplevel>
struct subc
{
  Toplevel* toplevel;
  typedef void result_type;

  void operator()(glyph_cache::glyph::line line) const
  {
    std::cout << "appending line" << std::endl;
    toplevel->append_arc_quadractic (line.from, line.from, line.to);
  }
  void operator()(glyph_cache::glyph::quadractic quad) const
  {
    std::cout << "appending quadractic" << std::endl;
    toplevel->append_arc_quadractic (quad.from, quad.control, quad.to);
  }
  void operator()(glyph_cache::glyph::cubic cubic) const
  {
    std::cout << "appending cubic" << std::endl;
    toplevel->append_arc_cubic (cubic.from, cubic.control1, cubic.control2, cubic.to);
  }
};
  
}
      
template <typename Toplevel>
void draw_glyph (glyph_cache::glyph gl, Toplevel& w, fastdraw::point<int> position
                 , fastdraw::point<int> size)
{
  const detail::subc<Toplevel> subc_set{&w};
  for (auto&& contour : gl.contours)
  {
    for (auto&& subc : contour.subcontours)
    {
      std::visit(subc_set, subc);
    }
  }
}

namespace detail {

template <typename T, typename U>
T transform_x (U pos)
{
  assert (static_cast<U>(std::numeric_limits<T>::max()) >= pos);
  std::cout << " xpos "<< pos << " ";
  return static_cast<T>(pos);
}

template <typename T, typename U>
T transform_y (U pos)
{
  assert (static_cast<U>(std::numeric_limits<T>::max()) >= pos);
  static_assert (std::numeric_limits<U>::min () < 0);
  //assert (pos <= 0);
  std::cout << " ypos "<< pos << " ";
  return static_cast<T>(pos);
}

}

void load_glyph (glyph_cache& cache, FT_Face& face, FT_UInt glyph_index)
{
  if (!cache.exists (glyph_index))
  {
    if (FT_Error err = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP))
    {
      std::cout << "Error is " << (unsigned long)err << std::endl;
      throw -1;
    }

    glyph_cache::glyph& gl = cache[glyph_index];

    assert (gl.empty());

    assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);
    assert (face->glyph->outline.n_contours != 0
            && face->glyph->outline.n_points != 0);

    FT_Vector initial_position = {0,0};
    
    FT_Outline_Funcs funcs;
    std::unique_ptr<void, void(*)(void*)> user {nullptr, & ::free};
    
    std::tie (funcs, user) = detail::make_funcs
      (
         [&] (FT_Vector to)
         {
           std::cout << "moveto" << std::endl;
           initial_position = to;
           gl.contours.push_back({});
         }
       , [&] (FT_Vector to)
         {
           assert (!gl.contours.empty());
           std::cout << "lineto" << std::endl;
           gl.contours.back().subcontours.push_back
             (glyph_cache::glyph::line{{detail::transform_x<int>(initial_position.x), detail::transform_y<int>(initial_position.y)}
                                       , {detail::transform_x<int>(to.x), detail::transform_y<int>(to.y)}});
           initial_position = to;
         }
       , [&] (FT_Vector control, FT_Vector to)
         {
           assert (!gl.contours.empty());
           std::cout << "conicto" << std::endl;
           gl.contours.back().subcontours.push_back
             (glyph_cache::glyph::quadractic{{detail::transform_x<int>(initial_position.x), detail::transform_y<int>(initial_position.y)}
                                             , {detail::transform_x<int>(control.x), detail::transform_y<int>(control.y)}
                                             , {detail::transform_x<int>(to.x), detail::transform_y<int>(to.y)}});
           initial_position = to;
         }
       , [&] (FT_Vector control1, FT_Vector control2, FT_Vector to)
         {
           assert (!gl.contours.empty());
           std::cout << "cubicto" << std::endl;
           gl.contours.back().subcontours.push_back
             (glyph_cache::glyph::cubic{{detail::transform_x<int>(initial_position.x), detail::transform_y<int>(initial_position.y)}
                                        , {detail::transform_x<int>(control1.x), detail::transform_y<int>(control1.y)}
                                        , {detail::transform_x<int>(control2.x), detail::transform_y<int>(control2.y)}
                                        , {detail::transform_x<int>(to.x), detail::transform_y<int>(to.y)}});
           initial_position = to;
         }
       );
    
    if (auto err = FT_Outline_Decompose (&face->glyph->outline, &funcs, user.get()))
    {
      std::cout << "Error decomposing " << (unsigned int)err << std::endl;
      
      throw -1;
    }

    
  }
  else
  {
  }
}

      
} } }

#endif
