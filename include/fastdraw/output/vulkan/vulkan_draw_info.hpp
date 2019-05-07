///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_DRAW_INFO_HPP
#define FASTDRAW_OUTPUT_VULKAN_DRAW_INFO_HPP

#include <vulkan/vulkan_core.h>
#include <vector>
#include <iostream>

namespace fastdraw { namespace output {

struct vulkan_draw_info
{
  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;
  VkRenderPass render_pass;

  uint32_t vertexCount;
  uint32_t instanceCount;
  uint32_t firstVertex;
  uint32_t firstInstance;

  std::vector<char> push_constants;
};

} }
  

#endif
