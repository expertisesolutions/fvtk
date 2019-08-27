///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_DRAW_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_DRAW_HPP

#include <ftk/ui/backend/vulkan_fwd.hpp>
#include <ftk/ui/ui_fwd.hpp>

#include <fastdraw/output/vulkan/vulkan_draw.hpp>
#include <fastdraw/object/dmabuf_image.hpp>
#include <fastdraw/output/vulkan/add_dmabuf_image.hpp>

namespace ftk { namespace ui { namespace backend {

template <typename Backend>
void draw (toplevel_window<Backend>& toplevel)
{
  auto const image_pipeline0 = fastdraw::output::vulkan::create_image_pipeline (toplevel->window.voutput, 0);
  auto static const vkCmdPushDescriptorSetKHR_ptr
    = vkGetDeviceProcAddr (toplevel->window.voutput.device, "vkCmdPushDescriptorSetKHR");
  auto static const vkCmdPushDescriptorSetKHR
    = reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(vkCmdPushDescriptorSetKHR_ptr);
  assert (vkCmdPushDescriptorSetKHR != nullptr);
  auto const indirect_pipeline = ftk::ui::vulkan
    ::create_indirect_draw_buffer_filler_pipeline (toplevel->window.voutput);
  
}

} } }

#endif
