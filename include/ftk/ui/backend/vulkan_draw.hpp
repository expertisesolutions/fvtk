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

#include <chrono>

#undef CHRONO_START
#undef CHRONO_COMPARE
#undef CHRONO_COMPARE_FORCE
#define CHRONO_START()  auto before = std::chrono::high_resolution_clock::now();
#define CHRONO_COMPARE()  {auto after = std::chrono::high_resolution_clock::now(); auto diff = after - before; if (diff > std::chrono::microseconds(100)) { std::cout << "Passed " << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << " at " <<  __FILE__ << ":" << __LINE__ << std::endl; before = after;} }
#define CHRONO_COMPARE_FORCE()  {auto after = std::chrono::high_resolution_clock::now(); auto diff = after - before; std::cout << "Passed " << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << " at " <<  __FILE__ << ":" << __LINE__ << std::endl; before = after; }

namespace ftk { namespace ui { namespace backend {

namespace detail {

inline
void submit_presentation_queue (/*VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore
                           , */VkQueue presentQueue
                           , VkSwapchainKHR swapChain
                           , uint32_t imageIndex
			   , VkSemaphore renderFinished)
{
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  VkSemaphore waitSemaphores[] = {renderFinished};

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = waitSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;
  presentInfo.pResults = nullptr; // Optional

  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;
  auto r = from_result (vkQueuePresentKHR(presentQueue, &presentInfo));
  if (r != vulkan_error_code::success)
    throw std::system_error (make_error_code (r));
}
  
inline
void submit_graphic_queue (/*VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore
                             ,*/ VkFence executionFinishedFence
                           , VkQueue graphicsQueue
                           , std::vector<VkCommandBuffer> commandBuffers
			     , int imageIndex
			     , VkSemaphore imageAvailable
			     , VkSemaphore renderFinished)
{
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  std::cout << "submit graphics" << std::endl;
      
  VkSemaphore waitSemaphores[] = {imageAvailable};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[/*imageIndex*/0];

  VkSemaphore signalSemaphores[] = {renderFinished};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;
  auto r = from_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo, executionFinishedFence));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code (r));
}

// template <typename Coord, typename Point, typename Color>
void record_command_buffer(VkRenderPass renderPass, VkCommandBuffer commandBuffer
                           , int imageIndex
                           , std::vector<VkFramebuffer>& swapChainFramebuffers
                           , VkExtent2D swapChainExtent
                           , fastdraw::output::vulkan::vulkan_draw_info draw_info)
                           // , std::vector<ftk::ui::toplevel_window_buffer>>* buffers)
                           // , fastdraw::output::vulkan::vulkan_output<Coord, Point, Color>& diff_output)
                           // , VkBuffer vertexBuffer)
{
  std::cout << "record command" << std::endl;
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

	  // VkSubpassDependency dependency = {};
	  // dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	  // dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	  // dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	  // dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
	  //   | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	  // renderPassInfo.dependencyCount = 1;
	  // renderPassInfo.pDependencies = &dependency;
	  
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
  
template <typename Loop, typename WindowingBase>
void on_expose (backend::vulkan<Loop, WindowingBase> const& backend, toplevel_window<vulkan<Loop, WindowingBase>> const& window
                , void* buffer, std::size_t buffer_size, int32_t width, int32_t height, int32_t stride)

// template <typename Loop, typename WindowingBase>
// void on_expose (backend::vulkan<Loop, WindowingBase> const& backend, toplevel_window<vulkan<Loop, WindowingBase>> const& window
//                 , fastdraw::object::dmabuf_image<fastdraw::point<int>> image)
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;
  
  CHRONO_START()
  std::cout << "window ptr " << &window << std::endl;
  std::cout << "w width " << window.window.voutput.swapChainExtent.width << std::endl;
  typedef int coord_type;
  typedef fastdraw::point<coord_type> point_type;
  // typedef fastdraw::color::color_premultiplied_rgba<uint8_t> color_type;
  // typedef fastdraw::object::fill_text<point_type, std::string, color_type> text_type;
  // typedef fastdraw::object::fill_box<point_type, color_type> box_type;
  // color_type transparent_blue {0, 0, 128, 128};
  // color_type transparent_red {128, 0, 0, 128};
  // text_type text { {{50, 100}, {1000, 200}, "/usr/share/fonts/TTF/DejaVuSans.ttf", "Hello World"
  //                   , {fastdraw::object::text_scale{true, true}}
  //                   , fastdraw::object::text_align::center
  //                   , fastdraw::object::text_align::center
  //                   }, transparent_red };

  // box_type box { {{50, 100}, {300, 100}}, transparent_blue };

  // static_cast<void>(box);
  typedef fastdraw::object::image<point_type> image_type;

  image_type image { {0, 0}, {width, height}, buffer, buffer_size, width, height, stride};
  CHRONO_COMPARE()
  
  // do something
  // auto output_info = fastdraw::output::vulkan::create_output_specific_object
  //   (window.window.voutput, image);
  auto output_info = fastdraw::output::vulkan::create_image_pipeline (window.window.voutput);
  CHRONO_COMPARE()

  // record buffer
  VkCommandPool commandPool;

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = window.window.graphicsFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

  if (vkCreateCommandPool(window.window.voutput.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command pool!");
  }
  CHRONO_COMPARE()

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(window.window.voutput.device, &allocInfo, &commandBuffer) != VK_SUCCESS) { 
   throw std::runtime_error("failed to allocate command buffers!");
  }
  CHRONO_COMPARE()

  std::cout << "acquiring" << std::endl;
  // if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  // {
  //   std::cout << "Timeout waiting for fence" << std::endl;
  //   throw -1;
  // }
  // vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);
  // submit

  VkSemaphore imageAvailable, renderFinished;

  {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    auto r = from_result(vkCreateSemaphore (window.window.voutput.device
					    , &semaphoreInfo
					    , nullptr, &imageAvailable));
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code (r));

    r = from_result(vkCreateSemaphore (window.window.voutput.device
				       , &semaphoreInfo
				       , nullptr, &renderFinished));
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code (r));
  }
  CHRONO_COMPARE()
  uint32_t imageIndex;
  vkAcquireNextImageKHR(window.window.voutput.device, window.window.swapChain
                        , std::numeric_limits<uint64_t>::max(), imageAvailable
                        , /*window.window.executionFinished*/nullptr, &imageIndex);

  CHRONO_COMPARE()

  std::cout << "recording buffer" << std::endl;
  CHRONO_COMPARE()

  
  record_command_buffer (window.window.voutput.renderpass
                         , commandBuffer, imageIndex
                         , window.window.swapChainFramebuffers
                         , window.window.voutput.swapChainExtent
                         , output_info/*,  buffers*/);
  CHRONO_COMPARE()

  std::cout << "image index " << imageIndex << std::endl;
  // if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  // {
  //   std::cout << "Timeout waiting for fence" << std::endl;
  //   throw -1;
  // }
  // vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);
  
  // std::cout << "image index " << imageIndex << std::endl;
  // if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  // {
  //   std::cout << "Timeout waiting for fence" << std::endl;
  //   throw -1;
  // }
  // vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);

  CHRONO_COMPARE()
  submit_graphic_queue (window.window.executionFinished, window.window.voutput.graphics_queue, {commandBuffer}, imageIndex, imageAvailable, renderFinished);
  CHRONO_COMPARE_FORCE()

  std::cout << "graphic" << std::endl;
  if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  {
    std::cout << "Timeout waiting for fence" << std::endl;
    throw -1;
  }
  vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);
  
  std::cout << "submit presentation" << std::endl;
  CHRONO_COMPARE()
  submit_presentation_queue (window.window.presentation_queue, window.window.swapChain, imageIndex, renderFinished);
  CHRONO_COMPARE()

  // if (vkWaitForFences (window.window.voutput.device, 1, &window.window.executionFinished, VK_FALSE, -1) == VK_TIMEOUT)
  // {
  //   std::cout << "Timeout waiting for fence" << std::endl;
  //   throw -1;
  // }
  // vkResetFences (window.window.voutput.device, 1, &window.window.executionFinished);
}

}
      
template <typename Loop, typename WindowingBase>
inline void on_draw (backend::vulkan<Loop, WindowingBase>& backend, toplevel_window<vulkan<Loop, WindowingBase>> const& window)
{
  // std::cout << "backend.display " << backend.display << std::endl;
  // std::cout << "window ptr " << &window << std::endl;

  //backend.exposure_signal.connect (std::bind(&detail::on_expose<Loop>, std::ref(backend), std::ref(window)));
}

template <typename Loop, typename WindowingBase>
inline void draw (backend::vulkan<Loop, WindowingBase>& backend, toplevel_window<vulkan<Loop, WindowingBase>> const& window)
{
  // std::cout << "backend.display " << backend.display << std::endl;
  // std::cout << "window ptr " << &window << std::endl;

  // backend.exposure_signal.connect (std::bind(&detail::on_expose<Loop>, std::ref(backend), std::ref(window)));
  //detail::on_expose (backend, window);
  auto const output_info = fastdraw::output::vulkan::create_image_pipeline (window.window.voutput);

  
}

template <typename Loop, typename WindowingBase>
inline void draw_buffer (backend::vulkan<Loop, WindowingBase>& backend, toplevel_window<vulkan<Loop, WindowingBase>> const& window
                         , void* buffer, std::size_t size
                         , int32_t width, int32_t height, int32_t stride)
{
  // std::cout << "backend.display " << backend.display << std::endl;
  // std::cout << "window ptr " << &window << std::endl;

  // backend.exposure_signal.connect (std::bind(&detail::on_expose<Loop>, std::ref(backend), std::ref(window)));
  CHRONO_START()
  detail::on_expose (backend, window, buffer, size, width, height, stride);
  CHRONO_COMPARE()
}

template <typename Loop, typename WindowingBase>
inline void draw_buffer (backend::vulkan<Loop, WindowingBase>& backend, toplevel_window<vulkan<Loop, WindowingBase>> const& window
                         , int fd
                         , int32_t width, int32_t height, std::uint32_t format, std::uint32_t offset, int32_t stride
                         , std::uint32_t modifier_hi, std::uint32_t modifier_lo)
{
  // std::cout << "backend.display " << backend.display << std::endl;
  // std::cout << "window ptr " << &window << std::endl;

  // backend.exposure_signal.connect (std::bind(&detail::on_expose<Loop>, std::ref(backend), std::ref(window)));
  CHRONO_START()
  // detail::on_expose (backend, window, fastdraw::object::dmabuf_image<fastdraw::point<int>>{{50,100}, {300,100}, fd, width, height
  //                                                                                                                 , stride, format
  //                                                                                                                 , modifier_hi, modifier_lo});
  CHRONO_COMPARE()
}

} } }

#endif
