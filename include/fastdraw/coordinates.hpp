///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_COORDINATES_HPP
#define FASTDRAW_COORDINATES_HPP

namespace fastdraw {

namespace coordinates {

template <typename T>
T proportion (float v, T whole)
{
  return v*whole;
}

template <typename T, typename U>
T proportion (T value, U whole
              , typename std::enable_if<std::is_integral<T>::value && std::is_integral<U>::value>::type* = nullptr)
{
  return value;
}
  
template <typename T>
float ratio (float v, T whole)
{
  return v;
}

template <typename T, typename U>
float ratio (T v, U whole
             , typename std::enable_if<std::is_integral<T>::value && std::is_integral<U>::value>::type* = nullptr)
{
  return 2*(((float)v)/whole) - 1.0f;
}
  
}
    
}

#endif
