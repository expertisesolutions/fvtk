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
#include <fastdraw/color.hpp>

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

template <typename T, typename U>
T apply_occlusion (U ch, T ratio
                   , typename std::enable_if<(std::is_integral<U>::type::value && std::is_integral<T>::type::value)>::type* = nullptr)
{
  typedef color_channel_traits<T> channel_traits;
  return ch * (channel_traits::ratio(ratio));
}

color_premultiplied_rgba<uint8_t> apply_occlusion (color_premultiplied_rgba<uint8_t> color, uint8_t ratio)
{
  static_assert (sizeof(uint32_t) == sizeof(color));
  uint32_t original;
  std::memcpy (&original, &color, sizeof(color));
  uint32_t rb = (original & 0xFF00FF00) >> 8;
  uint32_t ga = (original & 0x00FF00FF);
  rb = (rb * ratio) >> 8;
  ga = (ga * ratio) >> 8;
  uint32_t new_value = ((rb & 0xFF00FF) << 8) + (ga & 0xFF00FF);
  
  color_premultiplied_rgba<uint8_t> r;
  std::memcpy (&r, &new_value, sizeof(r));
  return r;
}

template <typename Channel>
template <typename OtherChannel>
color_premultiplied_rgba<Channel> color_premultiplied_rgba<Channel>::blend_with_src (color_premultiplied_rgba<OtherChannel> src)
{
  return {
          color::premultiplied_blend(r, src.r, src.a)
          , color::premultiplied_blend(g, src.g, src.a)
          , color::premultiplied_blend(b, src.b, src.a)
          , color::premultiplied_blend(a, src.a, src.a)
         };
}
    
} }

#endif
