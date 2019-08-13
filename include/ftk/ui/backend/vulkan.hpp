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
#include <fastdraw/output/vulkan/mt_buffer_pool.hpp>

#include <ftk/ui/backend/x11_base_fwd.hpp>
#include <ftk/ui/backend/uv/uv_loop_fwd.hpp>
#include <ftk/ui/backend/vulkan/draw.hpp>
#include <ftk/ui/backend/vulkan/queues.hpp>

#include <cstring>
#include <array>
#include <vector>
#include <optional>
#include <cassert>
#include <iostream>
#include <mutex>

namespace ftk { namespace ui { namespace backend { namespace vulkan {

template <typename Loop, typename WindowingBase>
struct vulkan : WindowingBase
{
  vulkan (Loop loop) : WindowingBase(loop)
  {
  }

  using window_base = typename WindowingBase::window;
  
  struct window : window_base
  {
    typedef int coord_type;
    typedef fastdraw::point<coord_type> point_type;
    typedef fastdraw::color::color_premultiplied_rgba<std::uint8_t> color_type;
    fastdraw::output::vulkan::shader_loader shader_loader;
    fastdraw::output::vulkan::vulkan_output<int, point_type, color_type> voutput;
    //int graphicsFamilyIndex;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSwapchainKHR swapChain;
    VkFence executionFinished;
    fastdraw::output::vulkan::mt_buffer_pool command_buffer_pool;
    struct queues queues;

    window (window_base base, fastdraw::output::vulkan::shader_loader shader_loader
            , fastdraw::output::vulkan::vulkan_output<int, point_type, color_type> voutput
            , int graphicsFamilyIndex, std::vector<VkFramebuffer> swapChainFramebuffers
            , VkSwapchainKHR swapChain, VkFence executionFinished
            , VkCommandPool command_pool
            , struct queues queues)
      : window_base (base), shader_loader (shader_loader)
      , voutput (voutput)//, graphicsFamilyIndex (graphicsFamilyIndex)
      , swapChainFramebuffers (swapChainFramebuffers)
      , swapChain (swapChain), executionFinished (executionFinished)
      , command_buffer_pool {command_pool}, queues (std::move(queues))
    {}

    window (window && other)
      : window_base (std::move(static_cast<window_base&>(*this)))
      , shader_loader (std::move(other.shader_loader))
      , voutput (std::move(other.voutput))
        //, graphicsFamilyIndex (std::move(other.graphicsFamilyIndex))
      , swapChainFramebuffers (std::move(other.swapChainFramebuffers))
      , swapChain (std::move(other.swapChain))
      , executionFinished (std::move(other.executionFinished))
      , command_buffer_pool (std::move(other.command_buffer_pool))
      , queues (std::move(other.queues))
    {}
  };

  window create_window (int width, int height, std::filesystem::path resource_path) const;
};

} } } }

#endif
