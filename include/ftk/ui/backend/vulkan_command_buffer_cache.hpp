///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_COMMAND_BUFFER_CACHE_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_COMMAND_BUFFER_CACHE_HPP

namespace ftk { namespace ui {

template <typename PipelineInfo>
VkCommandBuffer create_command_buffer (VkCommandPool pool
                                       , VkDevice device
                                       , VkFramebuffer framebuffer
                                       , VkExtent2D swapChainExtent
                                       , VkRenderPass renderpass
                                       , VkBuffer buffer
                                       , VkSampler sampler
                                       , VkImageView image_view
                                       , int x, int y
                                       , int width, int height
                                       , unsigned int buffer_cache_offset
                                       , PipelineInfo image_pipeline)
{
  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  using fastdraw::output::vulkan::vulkan_error_code;
  using fastdraw::output::vulkan::from_result;
    
  auto r = from_result (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));
  
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.framebuffer = framebuffer;
  // renderPassInfo.renderArea.offset = {x, y};
  // {
  //   auto w = x + width <= swapChainExtent.width
  //     ? width : swapChainExtent.width - x;
  //   auto h = y + height <= swapChainExtent.height
  //     ? height : swapChainExtent.height - y;
  //   renderPassInfo.renderArea.extent = {w/* + image.x*/, h/* + image.y*/};
  // }
  renderPassInfo.renderArea.offset = {0,0};
  renderPassInfo.renderArea.extent = {swapChainExtent.width, swapChainExtent.height};
  renderPassInfo.renderPass = renderpass;

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  r = from_result (vkBeginCommandBuffer(commandBuffer, &beginInfo));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));
    
  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pipeline.pipeline);

  std::vector<VkDeviceSize> offsets;
  std::vector<VkBuffer> buffers;

  offsets.push_back(buffer_cache_offset);
  buffers.push_back(buffer);
  offsets.push_back(buffer_cache_offset);
  buffers.push_back(buffer);
  
  vkCmdBindVertexBuffers(commandBuffer, 0, 2, &buffers[0], &offsets[0]);

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = image_view;
  imageInfo.sampler = sampler;

  VkWriteDescriptorSet descriptorWrites = {};
                             
  descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites.dstSet = 0;
  descriptorWrites.dstBinding = 1;
  //descriptorWrites.dstArrayElement = 0;
  descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites.descriptorCount = 1;
  descriptorWrites.pImageInfo = &imageInfo;

  auto static const vkCmdPushDescriptorSetKHR
    = vkGetDeviceProcAddr (device, "vkCmdPushDescriptorSetKHR");
  assert (vkCmdPushDescriptorSetKHR != nullptr);
  reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(vkCmdPushDescriptorSetKHR)
    (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
     , image_pipeline.pipeline_layout
     , 0, 1, &descriptorWrites);

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  r = from_result (vkEndCommandBuffer(commandBuffer));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));

  return commandBuffer;
}
    
} }

#endif
