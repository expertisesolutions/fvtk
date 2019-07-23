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
#include <X11/XKBlib.h>

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
  boost::signals2::signal<void (XKeyEvent)> key_signal;
  boost::signals2::signal<void (XMotionEvent)> motion_signal;
  boost::signals2::signal<void (XButtonEvent)> button_signal;
  
  struct window
  {
    Window x11_window;
    Display* x11_display;

    window () : x11_window{}, x11_display(nullptr) {}
    window (Window win, Display* display)
      : x11_window(win), x11_display(display)
    {
      XMapWindow(x11_display, x11_window);
      XStoreName(x11_display, x11_window, "VERY SIMPLE APPLICATION");
    }
  };

  x11_base(Loop loop)
    : loop(loop), display(nullptr)
  {
    connect();
  }

  void fd_readable () const
  {
    // std::cout << "fd_readable" << std::endl;
    XEvent xev;
    for (auto&& w : windows)
    {
      bool check;
      bool last_was_keypress = false;
      bool last_was_buttonpress = false;
      while ((check = XCheckWindowEvent (display, w, KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &xev)))
      {
        // std::cout << " Event Type " << (int)xev.type << std::endl;
        // std::cout << "is Expose ? " << (xev.type == Expose) << " is KeyPress? " << (xev.type == KeyPress)
        //           << " is KeyRelease? " << (xev.type == KeyRelease)
        //           << " is button press? " << (xev.type == ButtonPress)
        //           << " is button release? " << (xev.type == ButtonRelease) << std::endl;
        if (xev.type == Expose)
          exposure_signal();
        else if ((xev.type == KeyPress || xev.type == KeyRelease) && !(xev.type == KeyPress && last_was_keypress))
        {
          last_was_keypress = xev.type == KeyPress;
          key_signal (xev.xkey);
        }
        else if ((xev.type == ButtonPress || xev.type == ButtonRelease) && !(xev.type == ButtonPress && last_was_buttonpress))
        {
          last_was_buttonpress = xev.type == ButtonPress;
          button_signal (xev.xbutton);
        }
        else if (xev.type == MotionNotify)
        {
          ///std::cout << "Is motion? " << std::endl;
          motion_signal (xev.xmotion);
        }
      }
    }
  }
    
  void connect()
  {
    XInitThreads();
    display = XOpenDisplay(nullptr);
    if (display)
    {
      root = DefaultRootWindow(display);
      cmap = DefaultColormap(display, 0);
      int fd = XConnectionNumber(display);
      display_fd = loop.create_fd (fd, static_cast<int>(Loop::readable)
                                   , std::bind(&self_type::fd_readable, this));
      Bool r = True;
      XkbSetDetectableAutoRepeat (display, r, &r);
      assert (r == True);
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
    swa.event_mask = ExposureMask | KeyPressMask | ButtonPressMask | KeyReleaseMask | ButtonReleaseMask | PointerMotionMask;
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
