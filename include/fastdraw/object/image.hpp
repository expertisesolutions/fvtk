///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OBJECTS_IMAGE_HH
#define FASTDRAW_OBJECTS_IMAGE_HH

namespace fastdraw { namespace object {

template <typename Coord>
struct image
{
  typedef point<Coord> point_type;
  point_type pos, size;
  //  point_type prev_p1, prev_p2;
};
    
} }

#endif
