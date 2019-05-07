///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAWCXX_OBJECTS_TRIANGLE_HH
#define FASTDRAWCXX_OBJECTS_TRIANGLE_HH

#include <fastdraw/point.hpp>

namespace fastdraw { namespace object {

template <typename Point>
struct triangle_base
{
  typedef Point point_type;
  point_type p1, p2, p3;
};

template <typename Point, typename Color>
struct fill_triangle : triangle_base<Point>
{
  Color fill_color;
};
  
} }

#endif
