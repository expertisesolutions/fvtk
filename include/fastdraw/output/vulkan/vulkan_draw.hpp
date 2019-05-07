///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_DRAW_HPP
#define FASTDRAW_OUTPUT_VULKAN_DRAW_HPP

#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>

namespace fastdraw { namespace output {

void draw (vulkan_draw_info const& info, VkCommandBuffer commandBuffer)
{
  if (info.push_constants.size())
  {
    vkCmdPushConstants(commandBuffer,
                       info.pipeline_layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       info.push_constants.size(),
                       info.push_constants.data());
  }
              
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);
  vkCmdDraw(commandBuffer, info.vertexCount, info.instanceCount, info.firstVertex, info.firstInstance);
}
    
} }

#endif
