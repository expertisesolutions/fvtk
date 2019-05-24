///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_COLOR_HPP
#define FASTDRAW_COLOR_HPP

#include <fastdraw/color_algorithm.hpp>

#include <type_traits>

namespace fastdraw { namespace color {

template <typename Channel>
struct opaque_alpha
{
  operator Channel() const { return 1; }
};

template <typename Channel>
Channel operator*(Channel x, opaque_alpha<Channel>) { return x; }
template <typename Channel>
Channel operator*(opaque_alpha<Channel>, Channel x) { return x; }
template <typename Channel>
Channel operator-(opaque_alpha<Channel>, Channel x) { return 1-x; }
template <typename Channel>
Channel operator-(Channel x, opaque_alpha<Channel>) { return x-1; }
    
template <typename Channel>
struct color_rgb
{
  Channel r, g, b;

  typedef color_rgb<Channel> premultiplied_alpha;
  typedef color_rgb<Channel> unassociated_alpha;
  typedef std::false_type has_alpha_channel;
  typedef Channel channel;
  typedef color::opaque_alpha<Channel> opaque_channel;

  color_rgb () = default;
  color_rgb (Channel r, Channel g, Channel b, opaque_channel a = {})
    : r(r), g(g), b(b)
  {}

};

template <typename Channel>
struct color_premultiplied_rgba;
    
template <typename Channel>
struct color_unassociated_rgba
{
  Channel r, g, b, a;

  color_unassociated_rgba () = default;
  color_unassociated_rgba (Channel r, Channel g, Channel b, Channel a)
    : r(r), g(g), b(b), a(a)
  {}

  typedef color_premultiplied_rgba<Channel> premultiplied_alpha;
  typedef color_unassociated_rgba<Channel> unassociated_alpha;
  typedef std::true_type has_alpha_channel;
  typedef Channel channel;
  typedef Channel alpha_channel;
};

template <typename Channel>
struct color_premultiplied_rgba
{
  Channel r, g, b, a;

  color_premultiplied_rgba () = default;
  color_premultiplied_rgba (Channel r, Channel g, Channel b, Channel a)
    : r(r), g(g), b(b), a(a)
  {}

  typedef color_premultiplied_rgba<Channel> premultiplied_alpha;
  typedef color_unassociated_rgba<Channel> unassociated_alpha;
  typedef std::true_type has_alpha_channel;
  typedef Channel channel;
  typedef Channel alpha_channel;

  constexpr channel red() const
  {
    return r;
  }
  constexpr channel green() const
  {
    return g;
  }
  constexpr channel blue() const
  {
    return b;
  }
  constexpr channel alpha() const
  {
    return a;
  }

  constexpr premultiplied_alpha to_premultiplied_alpha () const
  {
    return *this;
  }
  template <typename OtherChannel>
  color_premultiplied_rgba<Channel> blend_with_src (color_premultiplied_rgba<OtherChannel> src)
  {
    return {
              color::premultiplied_blend(r, src.r, src.a)
            , color::premultiplied_blend(g, src.g, src.a)
            , color::premultiplied_blend(b, src.b, src.a)
            , color::premultiplied_blend(a, src.a, src.a)
           };
  }
};

template <typename T>
struct color_traits
{
  typedef typename T::premultiplied_alpha premultiplied_alpha;
  typedef typename T::unassociated_alpha unassociated_alpha;
  typedef typename T::has_alpha_channel has_alpha_channel;
  typedef typename T::channel channel;
  typedef typename T::alpha_channel alpha_channel;

  static channel red(T const& c) { return c.red(); }
  static channel blue(T const& c) { return c.blue(); }
  static channel green(T const& c) { return c.green(); }
  static alpha_channel alpha(T const& c) { return c.alpha(); }

  static premultiplied_alpha to_premultiplied_alpha (T const& c)
  {
    return c.to_premultiplied_alpha();
  }
};

} }

#endif
