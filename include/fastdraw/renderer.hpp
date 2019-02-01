///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_RENDERER_HH
#define FASTDRAW_RENDERER_HH

#include <vector>

namespace fastdraw {

template <typename Coord, typename T>
struct renderer
{
  Coord x, y, w, h;
  
  std::vector<T> active_objects, render_objects;
};

template <typename T>
void recalc_clip(T const& object)
{
}
  
template <typename Coord, typename T, typename Output>
void render(renderer<Coord, T>& r, Output& out)
{
  for (auto&& i : r.render_objects)
  {
    draw (i, out);
  }
}    
  
}

#endif



