///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_X11_BASE_HPP
#define FTK_FTK_UI_BACKEND_X11_BASE_HPP

#include <ftk/ui/backend/x11_base.hpp>

#include <system_error>
#include <functional>

#include <iostream>

#include <boost/signals2.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace ftk { namespace ui { namespace backend {

enum class xlib_error_code
{
  open_display_error
};

} } }

namespace std {
template <>
struct is_error_code_enum< ::ftk::ui::backend::xlib_error_code> : true_type {};
}

namespace ftk { namespace ui { namespace backend {

std::error_code make_error_code(xlib_error_code ec)
{
  return {ec};
}

template <typename Loop>
struct x11_base
{
  Loop loop;
  Display* display;
  Window root;
  Colormap cmap;
  typename Loop::fd display_fd;
  mutable std::vector<Window> windows;

  using self_type = x11_base<Loop>;

  boost::signals2::signal<void ()> exposure_signal;
  
  struct window
  {
    Window win;
    Display* display;

    window (Window win, Display* display)
      : win(win), display(display)
    {
      XMapWindow(display, win);
      XStoreName(display, win, "VERY SIMPLE APPLICATION");
    }
  };

  x11_base(Loop loop)
    : loop(loop), display(nullptr)
  {
    connect();
  }

  void fd_readable () const
  {
    std::cout << "fd_readable" << std::endl;
    XEvent xev;
    // which window ??
    for (auto&& w : windows)
    {
      bool check = XCheckWindowEvent(display, w, KeyPressMask | ExposureMask | ButtonPressMask, &xev);
      if (check)
      {
        std::cout << "is Expose ? " << (xev.type == Expose) << " is KeyPress? " << (xev.type == KeyPress)
                  << " is button press? " << (xev.type == ButtonPress) << std::endl;
        if (xev.type == Expose)
          exposure_signal();
        else if (xev.type == KeyPress)
          {}
        else if (xev.type == ButtonPress)
          {}
      }
      else
      {
        std::cout << "no event" << std::endl;
      }
    }
  }
    
  void connect()
  {
    display = XOpenDisplay(nullptr);
    if (display)
    {
      root = DefaultRootWindow(display);
      cmap = DefaultColormap(display, 0);
      int fd = XConnectionNumber(display);
      display_fd = loop.create_fd (fd, static_cast<int>(Loop::readable)
                                   , std::bind(&self_type::fd_readable, this));
    }
    else
    {
      throw std::system_error(make_error_code(xlib_error_code::open_display_error));
    }
  }

  window create_window(int width, int height) const
  {
    XSetWindowAttributes swa = {};
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | ButtonPressMask;
    std::cout << "creating window with w: " << width << " h: " << height << std::endl;
    Window w = XCreateWindow(display, root, 0, 0, width, height, 0
                             , /*vi->depth*/24, InputOutput
                             , /*vi->visual*/0, CWColormap | CWEventMask, &swa);
    windows.push_back(w);
    return { w, display };
  }

};

} } }

#endif
