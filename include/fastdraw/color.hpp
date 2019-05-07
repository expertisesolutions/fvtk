///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_COLOR_HPP
#define FASTDRAW_COLOR_HPP

namespace fastdraw { namespace color {

template <typename Channel>
struct color_rgb
{
  Channel r, g, b;
};
    
} }

#endif
