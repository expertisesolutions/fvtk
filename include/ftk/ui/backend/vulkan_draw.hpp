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
#include <ftk/ui/backend/vulkan_error.hpp>
#include <ftk/ui/ui_fwd.hpp>

#include <fastdraw/output/vulkan/vulkan_draw.hpp>

namespace ftk { namespace ui { namespace backend {

namespace detail {

inline
void submit_presentation_queue (/*VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore
                           , */VkQueue presentQueue
                           , VkSwapchainKHR swapChain
                           , uint32_t imageIndex)
                           // , VkFence executionFinishedFence)
{
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  // VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};

  // presentInfo.waitSemaphoreCount = 1;
  // presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional

  vkQueuePresentKHR(presentQueue, &presentInfo);
}
  
inline
void submit_graphic_queue (/*VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore
                             ,*/ VkFence executionFinishedFence
                           , VkQueue graphicsQueue
                           , std::vector<VkCommandBuffer> commandBuffers
                           , int imageIndex)
{
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  // VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  // submitInfo.waitSemaphoreCount = 1;
  // submitInfo.pWaitSemaphores = waitSemaphores;
  // submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

  // VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  // submitInfo.signalSemaphoreCount = 1;
  // submitInfo.pSignalSemaphores = signalSemaphores;
  auto r = backend::from_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo, executionFinishedFence));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code (r));
}

// template <typename Coord, typename Point, typename Color>
void record_command_buffer(VkRenderPass renderPass, VkCommandBuffer commandBuffer
                           , int imageIndex
                           , std::vector<VkFramebuffer>& swapChainFramebuffers
                           , VkExtent2D swapChainExtent
                           , fastdraw::output::vulkan::vulkan_draw_info draw_info)
                           // , fastdraw::output::vulkan::vulkan_output<Coord, Point, Color>& diff_output)
                           // , VkBuffer vertexBuffer)
{
        {
          VkCommandBufferBeginInfo beginInfo = {};
          beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

          if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
          }

          VkRenderPassBeginInfo renderPassInfo = {};
          renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
          renderPassInfo.renderPass = renderPass;
          renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
          renderPassInfo.renderArea.offset = {0, 0};
          renderPassInfo.renderArea.extent = swapChainExtent;

          vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

          fastdraw::output::vulkan::draw (draw_info, commandBuffer);
          // for (auto&& object_output : diff_output.object_outputs)
          // {
          //   for (auto&& pipeline : object_output.draw_infos)
          //   {
          //     // VkBuffer vertexBuffers[] = {vertexBuffer};
          //     // VkDeviceSize offsets[] = {0};
          //     // vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
          //     // for (int i = 0; i != 500; ++i)
          //       fastdraw::output::vulkan::draw (pipeline, commandBuffer);

          //   }
          // }
        
          vkCmdEndRenderPass(commandBuffer);

          if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
          }
        }
}
  
template <typename Loop>
void on_expose (backend::vulkan<Loop> const& backend, toplevel_window<vulkan<Loop>> const& window)
{
  std::cout << "window ptr " << &window << std::endl;
  std::cout << "w width " << window.window.voutput.swapChainExtent.width << std::endl;
  typedef int coord_type;
  typedef fastdraw::point<coord_type> point_type;
  typedef fastdraw::color::color_premultiplied_rgba<uint8_t> color_type;
  typedef fastdraw::object::fill_text<point_type, std::string, color_type> text_type;
  typedef fastdraw::object::fill_box<point_type, color_type> box_type;
  color_type transparent_blue {0, 0, 128, 128};
  color_type transparent_red {128, 0, 0, 128};
  text_type text { {{50, 100}, {1000, 200}, "/usr/share/fonts/TTF/DejaVuSans.ttf", "Hello World"
                    , {fastdraw::object::text_scale{true, true}}
                    , fastdraw::object::text_align::center
                    , fastdraw::object::text_align::center
                    }, transparent_red };

  box_type box { {{50, 100}, {300, 100}}, transparent_blue };

  static_cast<void>(box);
  
  // do something
  auto output_info = fastdraw::output::vulkan::create_output_specific_object
    (window.window.voutput, text);

  // record buffer
  VkCommandPool commandPool;

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = window.window.graphicsFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

  if (vkCreateCommandPool(window.window.voutput.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(window.window.voutput.device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate command buffers!");
  }

  std::cout << "recording buffer" << std::endl;
  
  record_command_buffer (window.window.voutput.renderpass
                         , commandBuffer, 0
                         , window.window.swapChainFramebuffers
                         , window.window.voutput.swapChainExtent
                         , output_info);

  if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  {
    std::cout << "Timeout waiting for fence" << std::endl;
    throw -1;
  }
  vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);
  // submit

  std::cout << "acquiring" << std::endl;

  uint32_t imageIndex;
  vkAcquireNextImageKHR(window.window.voutput.device, window.window.swapChain
                        , std::numeric_limits<uint64_t>::max(), VK_NULL_HANDLE/*imageAvailableSemaphore*/
                        , window.window.executionFinished, &imageIndex);

  std::cout << "image index " << imageIndex << std::endl;
  if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  {
    std::cout << "Timeout waiting for fence" << std::endl;
    throw -1;
  }
  vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);

  submit_graphic_queue (window.window.executionFinished, window.window.voutput.graphics_queue, {commandBuffer}, imageIndex);

  std::cout << "graphic" << std::endl;
  if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  {
    std::cout << "Timeout waiting for fence" << std::endl;
    throw -1;
  }
  vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);
  
  std::cout << "submit presentation" << std::endl;
  submit_presentation_queue (window.window.presentation_queue, window.window.swapChain, imageIndex);
}

}
      
template <typename Loop>
inline void on_draw (backend::vulkan<Loop>& backend, toplevel_window<vulkan<Loop>> const& window)
{
  std::cout << "backend.display " << backend.display << std::endl;
  std::cout << "window ptr " << &window << std::endl;

  backend.exposure_signal.connect (std::bind(&detail::on_expose<Loop>, std::ref(backend), std::ref(window)));
}

} } }

#endif
