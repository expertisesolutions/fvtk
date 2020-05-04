///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_REGION_UNION_HPP
#define FTK_FTK_UI_REGION_UNION_HPP

#include <set>
#include <algorithm>

#include <iostream>

#include <algorithm/rectangles_partition.hpp>

namespace ftk { namespace ui {

template <typename T>
struct region_union
{
  typedef std::pair<int, int> interval;
  typedef exp::algorithm::rectangle<interval, interval> rectangle;

  struct region
  {
    T x, y, w, h;

    friend std::ostream& operator<<(std::ostream& os, region const& r)
    {
      return os << "[x: " << r.x << " y: " << r.y << " w: " << r.w << " h: " << r.h << "]";
    }
    friend bool operator<(region const& left, region const& right)
    {
      return left.x == right.x
        ? left.y == right.y
        ? left.w == right.w
        ? left.h < right.h
        : left.w < right.w
        : left.y < right.y
        : left.x < right.x
        ;
    }
  };

  std::set <rectangle> rectangles;

  void add_region (region r)
  {
    rectangles.insert ({{r.x, r.w + r.x}, {r.y, r.h + r.y}});
  }
  
  std::vector<region> non_overlapping_regions () const
  {
    std::vector<region> r;
    auto new_rects = exp::algorithm::rectangle_partition (rectangles);
    std::transform (new_rects.begin(), new_rects.end()
                    , std::back_inserter (r)
                    , [] (auto&& r) -> region { return {r.i0.first, r.i1.first
                                                        , r.i0.second - r.i0.first, r.i1.second - r.i1.first}; });
    return r;
  }
  
};
    
} }

#endif
