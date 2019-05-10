///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAWCXX_OBJECT_VARIANT_HH
#define FASTDRAWCXX_OBJECT_VARIANT_HH

#include <fastdraw/object/box.hpp>
#include <fastdraw/object/image.hpp>
#include <fastdraw/object/line.hpp>
#include <fastdraw/object/triangle.hpp>
#include <fastdraw/object/text.hpp>

#include <variant>

namespace fastdraw { namespace object {

template <typename Coord, typename Point, typename Color>
struct object_variant
{
  typedef std::variant<fill_box<Point, Color>
                       , fill_triangle<Point, Color>
                       , fill_text<Point, std::string, Color>
                       > variant_type;

  variant_type object;
};
  
} }

#endif
