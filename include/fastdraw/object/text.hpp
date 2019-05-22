///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAWCXX_OBJECTS_TEXT_HH
#define FASTDRAWCXX_OBJECTS_TEXT_HH

#include <fastdraw/point.hpp>

#include <string>
#include <variant>

namespace fastdraw { namespace object {

struct text_scale
{
  bool vertically, horizontally;
};

struct text_point_size
{
  int point_size;
};

enum class text_align
{
  left_to_right,
  right_to_left,
  center
};

template <typename Point, typename String = std::string>
struct text_base
{
  typedef Point point_type;
  point_type p1;
  point_type size;
  String face;
  String text;

  std::variant
  <
    text_scale
    , text_point_size
  >
  size_information;
  text_align horizontal_alignment, vertical_alignment;
};

template <typename Point, typename String, typename Color>
struct fill_text : text_base<Point>
{
  Color fill_color;
};
  
} }

#endif
