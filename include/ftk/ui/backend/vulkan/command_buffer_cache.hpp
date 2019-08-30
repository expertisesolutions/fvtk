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

namespace ftk { namespace ui { namespace backend { namespace vulkan {

template <typename PipelineInfo>
VkCommandBuffer create_command_buffer (VkCommandPool pool
                                       , VkDevice device
                                       , VkFramebuffer framebuffer
                                       , VkExtent2D swapChainExtent
                                       , VkRenderPass renderpass
                                       , VkBuffer vertex_buffer
                                       , VkSampler sampler
                                       , std::vector<VkImageView>const& image_views
                                       , int x, int y
                                       , int width, int height
                                       , unsigned int buffer_cache_offset
                                       , PipelineInfo image_pipeline
                                       , VkBuffer zindex_pixel_buffer
                                       , VkBuffer zindex_array_buffer)
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
  VkRect2D scissor = renderPassInfo.renderArea;
  vkCmdSetScissor (commandBuffer, 0, 1, &scissor);

  std::vector<VkDeviceSize> offsets;
  std::vector<VkBuffer> buffers;

  offsets.push_back(buffer_cache_offset);
  buffers.push_back(vertex_buffer);
  offsets.push_back(buffer_cache_offset);
  buffers.push_back(vertex_buffer);
  offsets.push_back(buffer_cache_offset);
  buffers.push_back(vertex_buffer);
  offsets.push_back(buffer_cache_offset);
  buffers.push_back(vertex_buffer);
  
  vkCmdBindVertexBuffers(commandBuffer, 0, 4, &buffers[0], &offsets[0]);

  VkDescriptorImageInfo samplerInfo = {};
  samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  samplerInfo.sampler = sampler;

  assert (image_views.size() <= 10);
  VkDescriptorImageInfo imageInfos[10] = {};
  unsigned int i = 0;
  for (auto&& imageInfo : imageInfos)
  {
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = i < image_views.size() ? image_views[i] : image_views[0];
    ++i;
  }
  
  VkWriteDescriptorSet descriptorWrites[2] = {};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = 0;
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  descriptorWrites[0].descriptorCount = 10;
  descriptorWrites[0].pImageInfo = &imageInfos[0];
  
  descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet = 0;
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo = &samplerInfo;

  // VkDescriptorBufferInfo zindex_pixel_buffer_info {};
  // zindex_pixel_buffer_info.buffer = zindex_pixel_buffer;
  // zindex_pixel_buffer_info.range = VK_WHOLE_SIZE;
    
  // descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  // descriptorWrites[1].dstSet = 0;
  // descriptorWrites[1].dstBinding = 2;
  // descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  // descriptorWrites[1].descriptorCount = 1;
  // descriptorWrites[1].pBufferInfo = &zindex_pixel_buffer_info;

  // VkDescriptorBufferInfo zindex_array_buffer_info {};
  // zindex_array_buffer_info.buffer = zindex_array_buffer;
  // zindex_array_buffer_info.range = VK_WHOLE_SIZE;
    
  // descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  // descriptorWrites[2].dstSet = 0;
  // descriptorWrites[2].dstBinding = 3;
  // descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  // descriptorWrites[2].descriptorCount = 1;
  // descriptorWrites[2].pBufferInfo = &zindex_array_buffer_info;
  
  auto static const vkCmdPushDescriptorSetKHR
    = vkGetDeviceProcAddr (device, "vkCmdPushDescriptorSetKHR");
  assert (vkCmdPushDescriptorSetKHR != nullptr);
  reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(vkCmdPushDescriptorSetKHR)
    (commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
     , image_pipeline.pipeline_layout
     , 0, sizeof (descriptorWrites)/sizeof(descriptorWrites[0]), &descriptorWrites[0]);

  vkCmdDraw(commandBuffer, 6, 1, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  r = from_result (vkEndCommandBuffer(commandBuffer));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));

  return commandBuffer;
}
    
} } } }

#endif
