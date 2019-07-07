///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_XLIB_SURFACE_HPP
#define FTK_FTK_UI_BACKEND_XLIB_SURFACE_HPP

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_core.h>

namespace ftk { namespace ui { namespace backend {

struct xlib_surface
{
  template <typename Loop>
  xlib_surface (Loop const&) {}
  
  struct window
  {
    Display* x11_display;
    Window x11_window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface;
  };

  window create_window(int width, int height) const;
};

      
} } }
      
#endif
