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

#include <ftk/ui/backend/vulkan_backend_fwd.hpp>
#include <ftk/ui/backend/vulkan/indirect_draw.hpp>
#include <ftk/ui/toplevel_window.hpp>
#include <ftk/ui/ui_fwd.hpp>

#include <fastdraw/output/vulkan/vulkan_draw.hpp>
#include <fastdraw/object/dmabuf_image.hpp>
#include <fastdraw/output/vulkan/add_dmabuf_image.hpp>

namespace ftk { namespace ui { namespace backend { namespace vulkan {

template <typename Backend>
void fill_buffer (VkSemaphore semaphore, toplevel_window<Backend>& toplevel
                  , uint32_t image_index)
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;
  
  VkFence initialization_fence;
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  auto r = from_result (vkCreateFence (toplevel.window.voutput.device, &fenceInfo, nullptr, &initialization_fence));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));

  VkCommandPool command_pool = toplevel.window.voutput.command_pool;
  VkCommandBuffer command_buffer;

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = command_pool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  r = from_result (vkAllocateCommandBuffers(toplevel.window.voutput.device, &allocInfo
                                            , &command_buffer));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code (r));

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  r = from_result (vkBeginCommandBuffer(command_buffer, &beginInfo));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));

  // fill everything with 0's first
  vkCmdFillBuffer (command_buffer, toplevel.swapchain_info[image_index].indirect_draw_buffer
                   , 0 /* offset */, (6 + 4096 + 4096) * sizeof(uint32_t)
                   * toplevel.indirect_draw_info_array_size
                   , 0);
  for (std::size_t i = 0; i != toplevel.indirect_draw_info_array_size; ++i)
  {
    // vertex count filling
    vkCmdFillBuffer (command_buffer, toplevel.swapchain_info[image_index].indirect_draw_buffer
                     , (6 + 4096 + 4096) * sizeof(uint32_t) * i, sizeof (uint32_t), 6);
    // fill fg_zindex array
    vkCmdFillBuffer (command_buffer, toplevel.swapchain_info[image_index].indirect_draw_buffer
                     , (6 + 4096 + 4096) * sizeof(uint32_t) * i
                     + (6 + 4096) * sizeof(uint32_t)
                     , sizeof (uint32_t) * 4096, 0xFFFFFFFF);
  }
  
  r = from_result (vkEndCommandBuffer(command_buffer));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command_buffer;
  if (semaphore != VK_NULL_HANDLE)
  {
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphore;
  }

  {
    ftk::ui::backend::vulkan::queues::lock_graphic_queue lock_queue(toplevel.window.queues);
    r = from_result(vkQueueSubmit(lock_queue.get_queue().vkqueue, 1, &submitInfo, initialization_fence));
  }
  {
    r = from_result(vkWaitForFences(toplevel.window.voutput.device, 1, &initialization_fence, false, -1));
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code (r));
  }
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code (r));
}

// still damage calculation problems
template <typename Backend>
std::vector<toplevel_framebuffer_region>
calculate_regions (toplevel_window<Backend>& toplevel, int image_index)
{
  auto framebuffer_damaged_regions = std::move (toplevel.swapchain_info[image_index].framebuffers_damaged_regions);
  assert (toplevel.swapchain_info[image_index].framebuffers_damaged_regions.empty());

  std::cout << "damaged regions pre-recorded " << framebuffer_damaged_regions.size() << std::endl;

  if (framebuffer_damaged_regions.size() > 32)
  {
    auto first = std::next (framebuffer_damaged_regions.begin(), 32)
      , last = framebuffer_damaged_regions.end();
    for (;first != last; ++first)
    {
      toplevel.swapchain_info[image_index].framebuffers_damaged_regions.push_back (std::move(*first));
    }
    framebuffer_damaged_regions.erase (first, last);
  }

  unsigned int i = 0;
  for (auto&& component : toplevel.components)
  {
    if (component.must_draw[image_index])
    {
      std::cout << "found drawable component " << &component << " component.must_draw[0] "
                << component.must_draw[0] << " component.must_draw[1] " << component.must_draw[1] << std::endl;
      // images.push_back(image);
      component.must_draw[image_index] = false;
      component.framebuffers_regions[image_index] = {component.x, component.y, component.width, component.height};
      if (framebuffer_damaged_regions.size() < 32)
        framebuffer_damaged_regions.push_back
          ({component.x, component.y, component.width, component.height});
      else
        toplevel.swapchain_info[image_index].framebuffers_damaged_regions.push_back
          ({component.x, component.y, component.width, component.height});
    }
    i++;
  }
  return framebuffer_damaged_regions;
}

template <typename Backend>
std::vector<VkCommandBuffer>
record (toplevel_window<Backend>& toplevel
        , std::vector<toplevel_framebuffer_region> framebuffer_damaged_regions
        , uint32_t image_index)
{
  auto const static image_pipeline0 = fastdraw::output::vulkan::create_image_pipeline (toplevel.window.voutput, 0);
  auto static const vkCmdPushDescriptorSetKHR_ptr
    = vkGetDeviceProcAddr (toplevel.window.voutput.device, "vkCmdPushDescriptorSetKHR");
  auto static const vkCmdPushDescriptorSetKHR
    = reinterpret_cast<PFN_vkCmdPushDescriptorSetKHR>(vkCmdPushDescriptorSetKHR_ptr);
  assert (vkCmdPushDescriptorSetKHR != nullptr);
  auto const indirect_pipeline = ftk::ui::backend::vulkan
    ::create_indirect_draw_buffer_filler_pipeline (toplevel.window.voutput);

  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;
  
  VkCommandPool commandPool = toplevel.window.voutput.command_pool;

  std::vector<VkCommandBuffer> damaged_command_buffers;

  damaged_command_buffers.resize (framebuffer_damaged_regions.size());
         
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = damaged_command_buffers.size();

  if (!damaged_command_buffers.empty())
  {
    auto r = from_result (vkAllocateCommandBuffers(toplevel.window.voutput.device, &allocInfo
                                                   , &damaged_command_buffers[0]));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));
  }

  std::cout << "recording " << framebuffer_damaged_regions.size() << " regions" << std::endl;
  // draw damage areas
  int i = 0;
  for (auto&& region : framebuffer_damaged_regions)
  {
    auto damaged_command_buffer = damaged_command_buffers[i];

    auto x = region.x;
    auto y = region.y;
    auto width = static_cast<uint32_t>(region.width);
    auto height = static_cast<uint32_t>(region.height);

    std::cout << "damaged rect " << x << "x" << y << "-" << width << "x" << height << std::endl;
           
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = toplevel.window.swapChainFramebuffers[image_index];
    renderPassInfo.renderArea.offset = {x, y};
    {
      auto w = static_cast<uint32_t>(x) + width <= toplevel.window.voutput.swapChainExtent.width
        ? width : toplevel.window.voutput.swapChainExtent.width - static_cast<uint32_t>(x);
      auto h = static_cast<uint32_t>(y) + height <= toplevel.window.voutput.swapChainExtent.height
        ? height : toplevel.window.voutput.swapChainExtent.height - static_cast<uint32_t>(y);
      renderPassInfo.renderArea.extent = {w/* + image.x*/, h/* + image.y*/};

      std::cout << "rendering to " << renderPassInfo.renderArea.offset.x
                << "x" << renderPassInfo.renderArea.offset.y
                << " size " << renderPassInfo.renderArea.extent.width
                << "x" << renderPassInfo.renderArea.extent.height << std::endl;
    }
    renderPassInfo.renderPass = toplevel.window.voutput.renderpass;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    auto r = from_result (vkBeginCommandBuffer(damaged_command_buffer, &beginInfo));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code(r));

    {
      VkDescriptorBufferInfo ssboInfo = {};
      ssboInfo.buffer = toplevel.swapchain_info[image_index].component_ssbo_buffer;
      ssboInfo.range = VK_WHOLE_SIZE;

      VkDescriptorBufferInfo indirect_draw_info = {};
      indirect_draw_info.buffer = toplevel.swapchain_info[image_index].indirect_draw_buffer;
      indirect_draw_info.range = VK_WHOLE_SIZE;
      indirect_draw_info.offset = i*sizeof(typename ftk::ui::toplevel_window<Backend>::indirect_draw_info);

      std::cout << "offset of indirect draw info " << indirect_draw_info.offset << std::endl;
             
      VkWriteDescriptorSet descriptorWrites[2] = {};
                             
      descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[0].dstSet = 0;
      descriptorWrites[0].dstBinding = 0;
      descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorWrites[0].descriptorCount = 1;
      descriptorWrites[0].pBufferInfo = &ssboInfo;
      
      descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[1].dstSet = 0;
      descriptorWrites[1].dstBinding = 2;
      descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      descriptorWrites[1].descriptorCount = 1;
      descriptorWrites[1].pBufferInfo = &indirect_draw_info;
             
      vkCmdBindDescriptorSets (damaged_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS
                               , indirect_pipeline.pipeline_layout
                               , 0, 1, &toplevel.swapchain_info[image_index].texture_descriptors.set
                               , 0, 0);

      vkCmdBindDescriptorSets (damaged_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS
                               , indirect_pipeline.pipeline_layout
                               , 1, 1, &toplevel.sampler_descriptors.set
                               , 0, 0);

      vkCmdPushDescriptorSetKHR
        (damaged_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS
         , indirect_pipeline.pipeline_layout
         , 2 /* from 1 */, sizeof(descriptorWrites)/sizeof(descriptorWrites[0]), &descriptorWrites[0]);

      uint32_t component_size = toplevel.components.size();
      vkCmdPushConstants(damaged_command_buffer
                         , indirect_pipeline.pipeline_layout
                         , VK_SHADER_STAGE_VERTEX_BIT
                         | VK_SHADER_STAGE_COMPUTE_BIT
                         | VK_SHADER_STAGE_FRAGMENT_BIT
                         , 0, sizeof(uint32_t), &component_size);
    }

    vkCmdBeginRenderPass(damaged_command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(damaged_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, indirect_pipeline.pipeline);
    VkRect2D scissor = renderPassInfo.renderArea;
    vkCmdSetScissor (damaged_command_buffer, 0, 1, &scissor);

    vkCmdDraw(damaged_command_buffer, 6, 1, 0, 0);

    VkMemoryBarrier memory_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    memory_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    
    vkCmdPipelineBarrier (damaged_command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                          , VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
                          | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
                          | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                          | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT
                          , 1, &memory_barrier, 0, /*&buffer_barrier*/nullptr, 0, nullptr);

    vkCmdEndRenderPass(damaged_command_buffer);
    vkCmdBeginRenderPass(damaged_command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(damaged_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pipeline0.pipeline);
    vkCmdDrawIndirect (damaged_command_buffer, toplevel.swapchain_info[image_index].indirect_draw_buffer
                       , sizeof(typename ftk::ui::toplevel_window<Backend>::indirect_draw_info)*i, 1, 0);

    vkCmdEndRenderPass(damaged_command_buffer);

    // vkCmdFillBuffer (damaged_command_buffer, toplevel.swapchain_info[image_index].indirect_draw_buffer
    //                  , sizeof(uint32_t) /* offset */, (5 + 4096) * sizeof(uint32_t), 0);

    // vkCmdFillBuffer (damaged_command_buffer, toplevel.swapchain_info[image_index].indirect_draw_buffer
    //                  , sizeof(uint32_t) * (6 + 4096) /* offset */, 4096 * sizeof(uint32_t), 0xFFFFFFFF);

    r = from_result (vkEndCommandBuffer(damaged_command_buffer));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code(r));
    
    ++i;
  }

  return damaged_command_buffers;
}

template <typename Backend>
void draw (toplevel_window<Backend>& toplevel, uint32_t image_index
           /*, VkSemaphore image_available, VkSemaphore render_finished*/);

template <typename Backend>
void present (toplevel_window<Backend>& toplevel, uint32_t image_index
              /*, VkSemaphore render_finished*/);

template <typename Backend>
void debug_ssbo_buffer (toplevel_window<Backend>& toplevel, uint32_t image_index);

template <typename Backend>
void draw_and_present (toplevel_window<Backend>& toplevel, bool debug_ssbo = false)
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;

  VkSemaphore image_available = render_thread_create_semaphore (toplevel.window.voutput.device);

  uint32_t image_index = -1;
  auto r = from_result(vkAcquireNextImageKHR(toplevel.window.voutput.device, toplevel.window.swapChain
                                             , std::numeric_limits<uint64_t>::max(), image_available
                                             , nullptr, &image_index));
  if (r != vulkan_error_code::success)
    throw std::system_error (make_error_code (r));

  std::swap(toplevel.swapchain_info[image_index].image_available_sem, image_available);
  if (image_available != VK_NULL_HANDLE)
    vkDestroySemaphore (toplevel.window.voutput.device, image_available, nullptr);

  //VkSemaphore render_finished = render_thread_create_semaphore (toplevel.window.voutput.device);
  draw (toplevel, image_index/*, render_finished*/);
  present (toplevel, image_index/*, render_finished*/);
  if (debug_ssbo)
    debug_ssbo_buffer (toplevel, image_index);
}

template <typename Backend>
void draw (toplevel_window<Backend>& toplevel, uint32_t image_index
           /*, VkSemaphore image_available, VkSemaphore render_finished*/)
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;

  // in the wrong place
  std::unique_lock<std::mutex> l (toplevel.swapchain_info[image_index].in_use_mutex);
  {
    toplevel.swapchain_info[image_index].is_in_use = true;
    for (auto&& op : toplevel.swapchain_info[image_index].transfer_pending_operations)
    {
      std::visit([&] (auto&& op)
                 {
                   toplevel.transfer_operation (op, toplevel.swapchain_info[image_index], image_index);
                 }, op.operation);
    }
    toplevel.swapchain_info[image_index].transfer_pending_operations.clear();
  }

  if (toplevel.swapchain_info[image_index].render_finished_sem != VK_NULL_HANDLE)
    vkDestroySemaphore (toplevel.window.voutput.device, toplevel.swapchain_info[image_index].render_finished_sem, nullptr);

  toplevel.swapchain_info[image_index].render_finished_sem
    = render_thread_create_semaphore (toplevel.window.voutput.device);

  if (toplevel.swapchain_info[image_index].buffer_is_dirty)
  {
    //toplevel.swapchain_info[image_index].buffer_is_dirty = false;
    fill_buffer (VK_NULL_HANDLE, toplevel, image_index);
  }

  auto framebuffer_damaged_regions = calculate_regions (toplevel, image_index);
  l.unlock();
  std::vector<VkCommandBuffer> buffers = record (toplevel, framebuffer_damaged_regions, image_index);

  //toplevel.swapchain_info[image_index].buffer_is_dirty = false;
  //fill_buffer (VK_NULL_HANDLE, toplevel, image_index);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                             
  VkSemaphore waitSemaphores[] = {toplevel.swapchain_info[image_index].image_available_sem};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = buffers.size();
  submitInfo.pCommandBuffers = &buffers[0];

  VkSemaphore signalSemaphores[] = {toplevel.swapchain_info[image_index].render_finished_sem};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;

  VkFence execution_finished;
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  auto r = from_result (vkCreateFence (toplevel.window.voutput.device, &fenceInfo, nullptr, &execution_finished));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));

  auto queue_begin = std::chrono::high_resolution_clock::now();
  {
    queues::lock_graphic_queue lock_queue(toplevel.window.queues);

    auto now = std::chrono::high_resolution_clock::now();
    auto diff = now - queue_begin;
    std::cout << "Time locking queue "
              << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
              << "ms" << std::endl;
         
    //std::cout << "submit graphics " << buffers.size() << std::endl;
    auto r = from_result(vkQueueSubmit(lock_queue.get_queue().vkqueue, 1, &submitInfo, execution_finished/*/VK_NULL_HANDLE/*/));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));

    auto now2 = std::chrono::high_resolution_clock::now();
    auto diff2 = now2 - now;
    std::cout << "Time submitting to queue "
              << std::chrono::duration_cast<std::chrono::milliseconds>(diff2).count()
              << "ms" << std::endl;
  }

  {
    r = from_result(vkWaitForFences(toplevel.window.voutput.device, 1, &execution_finished, false, -1));
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code (r));
  }

}

template <typename Backend>
void present (toplevel_window<Backend>& toplevel, uint32_t image_index
              /*, VkSemaphore render_finished*/)
{
  {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore waitSemaphores[] = {toplevel.swapchain_info[image_index].render_finished_sem};
         
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = waitSemaphores;

    VkSwapchainKHR swapChains[] = {toplevel.window.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &image_index;
    presentInfo.pResults = nullptr; // Optional

    {
      queues::lock_presentation_queue lock_queue(toplevel.window.queues);
         
      using fastdraw::output::vulkan::from_result;
      using fastdraw::output::vulkan::vulkan_error_code;
      
      auto now = std::chrono::high_resolution_clock::now();
      
      auto r = from_result (vkQueuePresentKHR(lock_queue.get_queue().vkqueue, &presentInfo));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));
      
      auto now2 = std::chrono::high_resolution_clock::now();
      auto diff = now2 - now;
      std::cout << "Time submitting presentation queue "
                << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
                << "ms" << std::endl;

      /// still synchronization problems
      r = from_result(vkQueueWaitIdle (lock_queue.get_queue().vkqueue));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));
    }
           
    // {
    //   auto now = std::chrono::high_resolution_clock::now();
    //   auto diff = now - queue_begin;
    //   std::cout << "Time running drawing command "
    //             << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()
    //             << "ms" << std::endl;
    // }
    
  }
}

template <typename Backend>
void debug_ssbo_buffer (toplevel_window<Backend>& toplevel, uint32_t image_index)
{
  {
    auto data = toplevel.buffer_allocator.map (toplevel.swapchain_info[image_index].component_ssbo_buffer);
    auto iterator = static_cast<typename toplevel_window<Backend>::component_info*>(data)
      , last = iterator + toplevel.components.size();
    for (int i = 0; iterator != last; ++iterator,++i)
    {
      std::cout << "==== " << i << " showing components descriptor_index " << iterator->descriptor_index
                << " x " << iterator->x
                << " y " << iterator->y
                << " w " << iterator->w
                << " h " << iterator->h
                << " found alpha " << iterator->found_alpha
                << " compnent type " << iterator->component_type
                << " padding0 " << iterator->padding
                << " padding.color[0] " << iterator->component_data.color[0]
                << " padding.color[1] " << iterator->component_data.color[1]
                << " padding.color[2] " << iterator->component_data.color[2]
                << " padding.color[3] " << iterator->component_data.color[3]
                << std::endl;
    };
    toplevel.buffer_allocator.unmap (toplevel.swapchain_info[image_index].component_ssbo_buffer);
  }
  {
    auto data = toplevel.buffer_allocator.map (toplevel.swapchain_info[image_index].indirect_draw_buffer);
    auto iterator = static_cast<typename toplevel_window<Backend>::indirect_draw_info*>(data)
      , last = iterator + toplevel_window<Backend>::indirect_draw_info_array_size;
    for (int i = 0; iterator != last; ++iterator,++i)
    {
      std::cout << "finished indirect draw info vertexCount " << iterator->indirect.vertexCount
                << " instanceCount " << iterator->indirect.instanceCount
                << " firstVertex " << iterator->indirect.firstVertex
                << " firstInstance " << iterator->indirect.firstInstance
                << " component length " << iterator->component_length
                << " fragment_data_length " << iterator->fragment_data_length
        ;
      std::cout << " zindexes ";
      for (unsigned int j = 0; j != iterator->fragment_data_length; ++j)
      {
        std::cout << " " << j;
      }
      std::cout << std::endl;
    }
    toplevel.buffer_allocator.unmap (toplevel.swapchain_info[image_index].indirect_draw_buffer);
  }

  {
    std::unique_lock<std::mutex> l (toplevel.swapchain_info[image_index].in_use_mutex);
    assert (toplevel.swapchain_info[image_index].is_in_use);
    toplevel.swapchain_info[image_index].is_in_use = false;
    if (!toplevel.swapchain_info[image_index].transfer_pending_operations.empty())
    {
      std::cout << "there are queued operations" << std::endl;
    }
  }
}

} } } }

#endif
