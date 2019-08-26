///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_KHR_DISPLAY_HPP
#define FTK_FTK_UI_BACKEND_KHR_DISPLAY_HPP

#include <vulkan/vulkan.h>
//#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_core.h>

namespace ftk { namespace ui { namespace backend {

struct khr_display
{
  template <typename Loop>
  khr_display (Loop const&) {}
  
  struct window
  {
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface;
  };

  window create_window(int width, int height, std::filesystem::path resource_path) const;
};

} } }
      
#endif
