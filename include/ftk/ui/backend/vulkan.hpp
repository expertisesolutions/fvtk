///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_HPP

#include <fastdraw/output/vulkan/vulkan_output.hpp>

#include <ftk/ui/backend/x11_base_fwd.hpp>
#include <ftk/ui/backend/uv_fwd.hpp>
#include <ftk/ui/backend/vulkan_draw.hpp>

#include <cstring>
#include <array>
#include <vector>
#include <optional>
#include <cassert>
#include <iostream>
#include <mutex>

namespace ftk { namespace ui { namespace backend {

template <typename Loop, typename WindowingBase>
struct vulkan : WindowingBase
{
  vulkan (Loop loop, int additional_graphic_queues)
    : WindowingBase(loop), additional_graphic_queues(additional_graphic_queues)
  {
  }

  int additional_graphic_queues;
  
  using window_base = typename WindowingBase::window;
  
  struct window : window_base
  {
    typedef int coord_type;
    typedef fastdraw::point<coord_type> point_type;
    typedef fastdraw::color::color_premultiplied_rgba<std::uint8_t> color_type;
    fastdraw::output::vulkan::shader_loader shader_loader;
    fastdraw::output::vulkan::vulkan_output<int, point_type, color_type> voutput;
    int graphicsFamilyIndex;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkQueue presentation_queue;
    VkSwapchainKHR swapChain;
    VkFence executionFinished;
    VkQueue copy_buffer_queue;
    std::unique_ptr<std::mutex> copy_buffer_queue_mutex {new std::mutex};
  };

  window create_window (int width, int height) const;
};

using any = vulkan<>;
    
} } }

#endif
