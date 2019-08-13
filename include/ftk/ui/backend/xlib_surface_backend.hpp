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

#include <ftk/ui/backend/x11_base_backend.hpp>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_core.h>

namespace ftk { namespace ui { namespace backend {

template <typename Loop>
struct xlib_surface_backend : x11_base_backend<Loop>
{
  using base = x11_base_backend<Loop>;
  xlib_surface_backend (Loop const& loop) : base(loop) {}
  
  struct window : x11_base_backend<Loop>::window
  {
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface;
  };

  window create_window(int width, int height, std::filesystem::path resource_path) const;
};

      
} } }

#endif
