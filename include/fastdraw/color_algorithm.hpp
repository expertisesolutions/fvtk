///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_COLOR_ALGORITHM_HPP
#define FASTDRAW_COLOR_ALGORITHM_HPP

#include <fastdraw/color_channel.hpp>

#include <type_traits>

namespace fastdraw { namespace color {

template <typename D, typename S, typename A>
D premultiplied_blend (D dst, S src
                       , A src_alpha
                       , D dst_max = color_channel_traits<D>::max()
                       , S src_max = color_channel_traits<S>::max()
                       , A alpha_max = color_channel_traits<A>::max())
{
  return color_channel_traits<D>::parametric_ratio (dst, dst_max, dst_max) + color_channel_traits<S>::ratio (src, src_max, dst_max)
    * color_channel_traits<A>::ratio (src_alpha, alpha_max, dst_max);
}

template <typename T, typename U>
T apply_occlusion (U ch, T ratio
                   , typename std::enable_if<(std::is_floating_point<U>::type::value || std::is_floating_point<T>::type::value)>::type* = nullptr)
{
  return ch * ratio;
}
    
} }

#endif
