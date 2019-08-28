///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_COLOR_CHANNEL_HPP
#define FASTDRAW_COLOR_CHANNEL_HPP

#include <type_traits>

namespace fastdraw { namespace color {

template <typename T>
struct color_channel_traits;

template <>
struct color_channel_traits<float>
{
  constexpr static float max()
  {
    return 1.0f;
  }

  constexpr static float parametric_ratio(float n, float whole, float whole2)
  {
    assert (whole == whole2);
    return n;
  }
  constexpr static float ratio(float n, float whole)
  {
    return n/whole;
  }
  constexpr static float ratio(float n)
  {
    return n/max();
  }
};
    
template <typename T>
struct color_channel_traits
{
  constexpr static T max()
  {
    return std::numeric_limits<T>::max();
  }

  template <typename U>
  constexpr static U  parametric_domain_ratio (T n, T whole, U max = color_channel_traits<U>::max())
  {
    throw -1;
  }

  constexpr static float parametric_domain_ratio (T n, T whole, float max = color_channel_traits<float>::max())
  {
    float nn = n;
    return (max/1.0f)*(nn/whole);
  }

  constexpr static float ratio (T n, T whole, float max = color_channel_traits<float>::max())
  {
    return ((float)n)/whole;
  }

  constexpr static float ratio (T n)
  {
    return ((float)n)/max();
  }
};

} }

#endif
