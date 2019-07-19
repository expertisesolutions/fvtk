///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_LOAD_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_LOAD_HPP

#include <ftk/ui/backend/vulkan_fwd.hpp>
#include <ftk/ui/backend/vulkan_image.hpp>
#include <ftk/ui/ui_fwd.hpp>

#include <fastdraw/output/vulkan/vulkan_draw.hpp>
#include <fastdraw/object/dmabuf_image.hpp>
#include <fastdraw/output/vulkan/add_dmabuf_image.hpp>

#include <chrono>
#include <future>

#undef CHRONO_START
#undef CHRONO_COMPARE
#undef CHRONO_COMPARE_FORCE
#define CHRONO_START()  auto before = std::chrono::high_resolution_clock::now();
#define CHRONO_COMPARE()  {auto after = std::chrono::high_resolution_clock::now(); auto diff = after - before; if (diff > std::chrono::microseconds(100)) { std::cout << "Passed " << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << " at " <<  __FILE__ << ":" << __LINE__ << std::endl; before = after;} }
#define CHRONO_COMPARE_FORCE()  {auto after = std::chrono::high_resolution_clock::now(); auto diff = after - before; std::cout << "Passed " << std::chrono::duration_cast<std::chrono::microseconds>(diff).count() << " at " <<  __FILE__ << ":" << __LINE__ << std::endl; before = after; }

namespace ftk { namespace ui { namespace backend {

template <typename U>
struct vulkan_buffer_token
{
  struct buffer
  {
    void* pointer;
    std::int32_t width;
    std::int32_t height;
    std::uint32_t stride;
    bool loaded;
    VkImage vulkan_image;
    VkImageView vulkan_image_view;
    //VkDescriptorSet descriptor;
    U user_value;
    
    buffer (void* pointer, std::int32_t width, std::int32_t height, std::uint32_t stride, U user_value)
      : pointer(pointer), width(width), height(height), stride(stride), user_value(user_value) {}
  };

  vulkan_buffer_token () : token(nullptr) {}
  vulkan_buffer_token (buffer* token) : token(token) {}

  U& user_value() { return token->user_value; }
  U const& user_value() const { return token->user_value; }
  
  buffer* token;  
};

template <typename U>
bool operator==(vulkan_buffer_token<U> lhs, vulkan_buffer_token<U> rhs)
{
  return lhs.token == rhs.token;
}

template <typename U>
bool operator!=(vulkan_buffer_token<U> lhs, vulkan_buffer_token<U> rhs)
{
  return lhs.token != rhs.token;
}

template <typename U>
bool operator<(vulkan_buffer_token<U> lhs, vulkan_buffer_token<U> rhs)
{
  return std::less<typename vulkan_buffer_token<U>::buffer*>()(lhs.token, rhs.token);
}

template <typename U>
bool operator<=(vulkan_buffer_token<U> lhs, vulkan_buffer_token<U> rhs)
{
  return lhs.token == rhs.token || lhs < rhs;
}

template <typename U>
bool operator>(vulkan_buffer_token<U> lhs, vulkan_buffer_token<U> rhs)
{
  return std::less<typename vulkan_buffer_token<U>::buffer*>()(rhs.token, lhs.token);
}
template <typename U>
bool operator>=(vulkan_buffer_token<U> lhs, vulkan_buffer_token<U> rhs)
{
  return !(lhs.token < rhs.token);
}

template <typename I>
pc::future<vulkan_image_loader::output_image_type> vulkan_image_loader::load (std::filesystem::path path, I image_loader) const
{
  return
  graphic_thread_pool->run
    (
     [path, image_loader, this] (VkCommandBuffer command_buffer, unsigned int, auto submitted) -> pc::future<output_image_type>
     {
       using fastdraw::output::vulkan::from_result;
       using fastdraw::output::vulkan::vulkan_error_code;

       std::cout << "running in a thread loop!" << std::endl;

       auto image = image_loader.load (path);

       auto size = image.height() * image.stride();

       std::cout << "png image " << image.width() << "x" << image.height() << " image  stride " << image.stride()
                 << " format " << (int)image.format() << std::endl;

       auto staging_pair = fastdraw::output::vulkan::create_buffer
         (device, size, physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

       void* data;
       auto r = from_result(vkMapMemory(device, staging_pair.second, 0, size, 0, &data));
       if (r != vulkan_error_code::success)
         throw std::system_error (make_error_code(r));

       image.write_to (static_cast<char*>(data), size);
       // for (std::size_t i = 3; i < size; i += 4)
       //   static_cast<char*>(data)[i] = 255;
       
       vkUnmapMemory(device, staging_pair.second);

       auto format = VK_FORMAT_B8G8R8A8_UNORM; //VK_FORMAT_R8G8B8A8_UNORM; RGBA vs ARGB
  
       VkImageCreateInfo imageInfo = {};
       imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
       imageInfo.imageType = VK_IMAGE_TYPE_2D;
       imageInfo.extent.width = static_cast<uint32_t>(image.width());
       imageInfo.extent.height = static_cast<uint32_t>(image.height());
       imageInfo.extent.depth = 1;
       imageInfo.mipLevels = 1;
       imageInfo.arrayLayers = 1;
       imageInfo.format = format;
       imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
       imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
       imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
       imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
       imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
       //imageInfo.flags = 0; // Optional

       VkImage vulkan_image;
       r = from_result(vkCreateImage(device, &imageInfo, nullptr, &vulkan_image));
       if (r != vulkan_error_code::success)
         throw std::system_error (make_error_code(r));

       VkMemoryRequirements memRequirements;
       vkGetImageMemoryRequirements(device, vulkan_image, &memRequirements);

       VkDeviceMemory textureImageMemory;
       {
         VkMemoryAllocateInfo allocInfo = {};
         allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
         allocInfo.allocationSize = memRequirements.size;
         allocInfo.memoryTypeIndex = fastdraw::output::vulkan::find_memory_type
           (memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device);

         r = from_result(vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory));
         if (r != vulkan_error_code::success)
           throw std::system_error(make_error_code(r));
       }

       vkBindImageMemory(device, vulkan_image, textureImageMemory, 0);
       std::cout << __FILE__ << ":" << __LINE__ << std::endl;

       VkImageView vulkan_image_view;
       VkImageViewCreateInfo viewInfo = {};
       viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
       viewInfo.image = vulkan_image;
       viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
       viewInfo.format = format;
       viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
       viewInfo.subresourceRange.baseMipLevel = 0;
       viewInfo.subresourceRange.levelCount = 1;
       viewInfo.subresourceRange.baseArrayLayer = 0;
       viewInfo.subresourceRange.layerCount = 1;
       
       if (vkCreateImageView(device, &viewInfo, nullptr, &vulkan_image_view) != VK_SUCCESS) {
         throw std::runtime_error("failed to create texture image view!");
       }

       std::cout << __FILE__ << ":" << __LINE__ << std::endl;

       VkCommandBufferBeginInfo beginInfo = {};
       beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
       beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
       r = from_result(vkBeginCommandBuffer(command_buffer, &beginInfo));
       if (r != vulkan_error_code::success)
         throw std::system_error (make_error_code(r));

       std::cout << __FILE__ << ":" << __LINE__ << std::endl;
       VkImageMemoryBarrier barrier = {};
       barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
       barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_UNDEFINED;
       barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

       barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
       barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

       barrier.image = vulkan_image;
       barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
       barrier.subresourceRange.baseMipLevel = 0;
       barrier.subresourceRange.levelCount = 1;
       barrier.subresourceRange.baseArrayLayer = 0;
       barrier.subresourceRange.layerCount = 1;

       barrier.srcAccessMask = 0;
       barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

       barrier.srcAccessMask = 0; // TODO
       barrier.dstAccessMask = 0; // TODO

       std::cout << __FILE__ << ":" << __LINE__ << std::endl;
       vkCmdPipelineBarrier
         (
          command_buffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT /* TODO */, VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */,
          0,
          0, nullptr,
          0, nullptr,
          1, &barrier
         );
       std::cout << __FILE__ << ":" << __LINE__ << std::endl;
       
       VkBufferImageCopy region = {};
       region.bufferOffset = 0;
       region.bufferRowLength = 0;
       region.bufferImageHeight = 0;

       region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
       region.imageSubresource.mipLevel = 0;
       region.imageSubresource.baseArrayLayer = 0;
       region.imageSubresource.layerCount = 1;

       region.imageOffset = {0, 0, 0};
       region.imageExtent = {
                             image.width(),
                             image.height(),
                             1
                            };
       std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  
       vkCmdCopyBufferToImage
         (
          command_buffer,
          staging_pair.first,
          vulkan_image,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          1,
          &region
         );  

       barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
       barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

       barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
       barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
       std::cout << __FILE__ << ":" << __LINE__ << std::endl;

       vkCmdPipelineBarrier
         (
          command_buffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /* TODO */,
          0,
          0, nullptr,
          0, nullptr,
          1, &barrier
         );

       std::cout << __FILE__ << ":" << __LINE__ << std::endl;
       vkEndCommandBuffer(command_buffer);
       std::cout << __FILE__ << ":" << __LINE__ << std::endl;


       return submitted.then
         ([device = device, staging_pair, vulkan_image, vulkan_image_view] (auto&&)
           -> vulkan_image_loader::output_image_type
          {
            std::cout << __FILE__ << ":" << __LINE__ << std::endl;
            vkDestroyBuffer(device, staging_pair.first, nullptr);
            std::cout << __FILE__ << ":" << __LINE__ << std::endl;
            vkFreeMemory(device, staging_pair.second, nullptr);
            std::cout << __FILE__ << ":" << __LINE__ << std::endl;
            return {vulkan_image, vulkan_image_view};
          });
     });
}

template <typename Loop, typename WindowingBase, typename U, typename Handler>
inline vulkan_buffer_token<U> load_buffer
(backend::vulkan<Loop, WindowingBase>& backend, toplevel_window<vulkan<Loop, WindowingBase>>& window
 , void* buffer, std::int32_t width, std::int32_t height, std::uint32_t stride
 , U user_value, Handler handler)
{
  CHRONO_START()
  // detail::on_expose (backend, window, fastdraw::object::dmabuf_image<fastdraw::point<int>>{{50,100}, {300,100}, fd, width, height
  //                                                                                                                 , stride, format
  //                                                                                                                 , modifier_hi, modifier_lo});

  using buffer_type = typename vulkan_buffer_token<U>::buffer;
  buffer_type* pb = new buffer_type {buffer, width, height, stride, std::move(user_value)};
  vulkan_buffer_token<U> token {pb};

  auto future =
  std::async([pb, backend = &backend, window = &window, handler, token]
             {
               // using fastdraw::output::vulkan::from_result;
               // using fastdraw::output::vulkan::vulkan_error_code;
               // CHRONO_START()
               // auto size = pb->stride * pb->height;

               // auto staging_pair = fastdraw::output::vulkan::create_buffer
               //   (window->window.voutput.device, size
               //    , window->window.voutput.physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

               // void* data;
               // CHRONO_COMPARE()
               // auto r = from_result(vkMapMemory(window->window.voutput.device, staging_pair.second, 0, size, 0, &data));
               // if (r != vulkan_error_code::success)
               // {
               //   handler(make_error_code(r), token);
               //   return;
               // }
               // CHRONO_COMPARE()
               // std::cout << __FILE__ ":" << __LINE__ << std::endl;

               // std::cout << __FILE__ ":" << __LINE__ << std::endl;
               // CHRONO_COMPARE()
               // std::cout << "data " << data << " pb->pointer " << pb->pointer << " size " << size << std::endl;
               // std::memcpy (data, pb->pointer, size);
               // CHRONO_COMPARE()

               // std::cout << __FILE__ ":" << __LINE__ << std::endl;
               // CHRONO_COMPARE()
               // vkUnmapMemory(window->window.voutput.device, staging_pair.second);
               // CHRONO_COMPARE()

               // VkDeviceMemory textureImageMemory;

               // auto format = VK_FORMAT_B8G8R8A8_UNORM; //VK_FORMAT_R8G8B8A8_UNORM; RGBA vs ARGB
  
               // VkImageCreateInfo imageInfo = {};
               // imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
               // imageInfo.imageType = VK_IMAGE_TYPE_2D;
               // imageInfo.extent.width = static_cast<uint32_t>(pb->width);
               // imageInfo.extent.height = static_cast<uint32_t>(pb->height);
               // imageInfo.extent.depth = 1;
               // imageInfo.mipLevels = 1;
               // imageInfo.arrayLayers = 1;
               // imageInfo.format = format;
               // imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
               // imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
               // imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
               // imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
               // imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
               // //imageInfo.flags = 0; // Optional

               // CHRONO_COMPARE()
               // std::cout << __FILE__ ":" << __LINE__ << std::endl;
               // r = from_result(vkCreateImage(window->window.voutput.device, &imageInfo, nullptr, &pb->vulkan_image));
               // if (r != vulkan_error_code::success)
               // {
               //   handler(make_error_code(r), token);
               //   return;
               // }
               // CHRONO_COMPARE()

               // VkMemoryRequirements memRequirements;
               // vkGetImageMemoryRequirements(window->window.voutput.device, pb->vulkan_image, &memRequirements);

               // {
               //   VkMemoryAllocateInfo allocInfo = {};
               //   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
               //   allocInfo.allocationSize = memRequirements.size;
               //   allocInfo.memoryTypeIndex = fastdraw::output::vulkan::find_memory_type
               //     (memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
               //      , window->window.voutput.physical_device);

               //   CHRONO_COMPARE()
               //   r= from_result(vkAllocateMemory(window->window.voutput.device, &allocInfo, nullptr, &textureImageMemory));
               //   if (r != vulkan_error_code::success)
               //   {
               //     handler(make_error_code(r), token);
               //     return;
               //   }
               //   CHRONO_COMPARE()
               // }

               // CHRONO_COMPARE()
               // vkBindImageMemory(window->window.voutput.device, pb->vulkan_image, textureImageMemory, 0);
               // CHRONO_COMPARE()
               // std::cout << __FILE__ ":" << __LINE__ << std::endl;

               // VkImageViewCreateInfo viewInfo = {};
               // viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
               // viewInfo.image = pb->vulkan_image;
               // viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
               // viewInfo.format = format;
               // viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
               // viewInfo.subresourceRange.baseMipLevel = 0;
               // viewInfo.subresourceRange.levelCount = 1;
               // viewInfo.subresourceRange.baseArrayLayer = 0;
               // viewInfo.subresourceRange.layerCount = 1;

               // CHRONO_COMPARE()
               // if (vkCreateImageView(window->window.voutput.device, &viewInfo, nullptr, &pb->vulkan_image_view) != VK_SUCCESS) {
               //   throw std::runtime_error("failed to create texture image view!");
               // }
               // CHRONO_COMPARE()
                 
               
               // VkCommandPool command_pool;
               // VkCommandPoolCreateInfo poolInfo = {};
               // poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
               // poolInfo.queueFamilyIndex = window->window.graphicsFamilyIndex;
               // poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

               // r = from_result(vkCreateCommandPool(window->window.voutput.device, &poolInfo, nullptr, &command_pool));
               // if (r != vulkan_error_code::success)
               // {
               //   handler(make_error_code(r), token);
               //   return;
               // }

               // VkCommandBuffer commandBuffer;
               // {
               //   VkCommandBufferAllocateInfo allocInfo = {};
               //   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
               //   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
               //   allocInfo.commandPool = command_pool;
               //   allocInfo.commandBufferCount = 1;

               //   CHRONO_COMPARE()
               //   r = from_result(vkAllocateCommandBuffers(window->window.voutput.device, &allocInfo, &commandBuffer));
               //   if (r != vulkan_error_code::success)
               //   {
               //     handler(make_error_code(r), token);
               //     return;
               //   }
                 
               //   CHRONO_COMPARE()
               // }

               // std::cout << __FILE__ ":" << __LINE__ << std::endl;
               // VkCommandBufferBeginInfo beginInfo = {};
               // beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
               // beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
               // CHRONO_COMPARE()
               // r = from_result(vkBeginCommandBuffer(commandBuffer, &beginInfo));
               // if (r != vulkan_error_code::success)
               // {
               //   handler(make_error_code(r), token);
               //   return;
               // }
               // CHRONO_COMPARE()

               //   // VkBufferCopy copyRegion = {};
               //   // copyRegion.size = size;
               //   // vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

               // VkImageMemoryBarrier barrier = {};
               // barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
               // barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_UNDEFINED;
               // barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

               // barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
               // barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

               // barrier.image = pb->vulkan_image;
               // barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
               // barrier.subresourceRange.baseMipLevel = 0;
               // barrier.subresourceRange.levelCount = 1;
               // barrier.subresourceRange.baseArrayLayer = 0;
               // barrier.subresourceRange.layerCount = 1;

               // barrier.srcAccessMask = 0;
               // barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

               // barrier.srcAccessMask = 0; // TODO
               // barrier.dstAccessMask = 0; // TODO

               // CHRONO_COMPARE()
               // vkCmdPipelineBarrier
               //   (
               //    commandBuffer,
               //    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT /* TODO */, VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */,
               //    0,
               //    0, nullptr,
               //    0, nullptr,
               //    1, &barrier
               //   );
               // CHRONO_COMPARE()

               //   std::cout << __FILE__ ":" << __LINE__ << std::endl;
               // VkBufferImageCopy region = {};
               // region.bufferOffset = 0;
               // region.bufferRowLength = 0;
               // region.bufferImageHeight = 0;

               // region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
               // region.imageSubresource.mipLevel = 0;
               // region.imageSubresource.baseArrayLayer = 0;
               // region.imageSubresource.layerCount = 1;

               // region.imageOffset = {0, 0, 0};
               // region.imageExtent = {
               //                       pb->width,
               //                       pb->height,
               //                       1
               //                      };
  
               // CHRONO_COMPARE()
               // vkCmdCopyBufferToImage
               //   (
               //    commandBuffer,
               //    staging_pair.first,
               //    pb->vulkan_image,
               //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               //    1,
               //    &region
               //   );  
               // CHRONO_COMPARE()

               // barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
               // barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

               // barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
               // barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

               // CHRONO_COMPARE()
               // vkCmdPipelineBarrier
               //   (
               //    commandBuffer,
               //    VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /* TODO */,
               //    0,
               //    0, nullptr,
               //    0, nullptr,
               //    1, &barrier
               //   );
               // CHRONO_COMPARE()
               //   std::cout << __FILE__ ":" << __LINE__ << std::endl;

               // vkEndCommandBuffer(commandBuffer);
               // CHRONO_COMPARE()
               
               // VkSubmitInfo submitInfo = {};
               // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
               // submitInfo.commandBufferCount = 1;
               // submitInfo.pCommandBuffers = &commandBuffer;

               // // {
               // //   std::unique_lock<std::mutex> l(window->window.copy_buffer_queue_mutex);
               
               // //   CHRONO_COMPARE()
               // //   r = from_result(vkQueueSubmit(window->window.copy_buffer_queue, 1, &submitInfo, VK_NULL_HANDLE));

               // //   if (r != vulkan_error_code::success)
               // //   {
               // //     handler (make_error_code(r), token);
               // //     return;
               // //   }
                 
               // //   CHRONO_COMPARE()
               // //   vkQueueWaitIdle(window->window.copy_buffer_queue);
               // //   CHRONO_COMPARE()

               // //   // if (submit_error)
               // //   //   throw -1;
               // // }
               // CHRONO_COMPARE()
               // vkFreeCommandBuffers(window->window.voutput.device, command_pool, 1, &commandBuffer);
               // vkDestroyBuffer(window->window.voutput.device, staging_pair.first, nullptr);
               // vkFreeMemory(window->window.voutput.device, staging_pair.second, nullptr);
               // CHRONO_COMPARE()

               // // how do I warn that it is finished?
               // handler ({}, token);
             }
    );
  static_cast<void>(future);
  CHRONO_COMPARE()

  return token;
}

} } }

#endif
