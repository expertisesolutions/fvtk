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

template <typename Executor>
template <typename I>
pc::future<typename vulkan_image_loader<Executor>::output_image_type> vulkan_image_loader<Executor>
  ::load (std::filesystem::path path, I image_loader) const
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;

  std::cout << "running in a thread loop!" << std::endl;

  auto image = image_loader.load (path);

  uint32_t src_size = height(image) * stride(image);
  uint32_t buffer_size = width (image) * height(image) * 4;

  std::cout << "image " << width(image) << "x" << height(image) << " image  stride " << stride(image)
            << " format " << (int)format(image) << std::endl;

  auto staging_pair = fastdraw::output::vulkan::create_buffer
    (device, buffer_size, physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  void* data;
  auto r = from_result(vkMapMemory(device, staging_pair.second, 0, buffer_size, 0, &data));
  if (r != vulkan_error_code::success)
    throw std::system_error (make_error_code(r));

  if (src_size == buffer_size)
    write_to (image, static_cast<char*>(data), src_size);
  else
  {
    std::vector<char> raw (src_size);
    write_to (image, &raw[0], src_size);
    unsigned int channels = stride (image) / width (image);
    assert (channels == 3);
    char* cdata = static_cast<char*>(data);
    for (unsigned int i = 0, j = 0; i != src_size;/* i += channels, j += 4*/)
    {
      //i++;i++;
      //std::cout << "R " << (int)raw[i] << " G " << (int)raw[i+1] << " R " << (int)raw[i+2] << std::endl;
      cdata[j++] = raw[i++];
      cdata[j++] = raw[i++];
      cdata[j++] = raw[i++];
      cdata[j++] = 255;
    }
  }
       
  vkUnmapMemory(device, staging_pair.second);

  return load (staging_pair.first, width(image), height(image));
}

template <typename Executor>
pc::future<typename vulkan_image_loader<Executor>::output_image_type> vulkan_image_loader<Executor>
  ::load (VkBuffer buffer, int32_t width, int32_t height) const
{
  return
  graphic_thread_pool->run
    (
     [buffer, width, height, this] (VkCommandBuffer command_buffer, unsigned int, auto submitted)
       -> pc::future<output_image_type>
     {
       using fastdraw::output::vulkan::from_result;
       using fastdraw::output::vulkan::vulkan_error_code;
       
       //auto format = VK_FORMAT_B8G8R8A8_UNORM; //VK_FORMAT_R8G8B8A8_UNORM; RGBA vs ARGB
       auto format = VK_FORMAT_R8G8B8A8_UNORM; //VK_FORMAT_R8G8B8A8_UNORM; RGBA vs ARGB
  
       VkImageCreateInfo imageInfo = {};
       imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
       imageInfo.imageType = VK_IMAGE_TYPE_2D;
       imageInfo.extent.width = static_cast<uint32_t>(width);
       imageInfo.extent.height = static_cast<uint32_t>(height);
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
       auto r = from_result(vkCreateImage(device, &imageInfo, nullptr, &vulkan_image));
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
                             static_cast<uint32_t>(width),
                             static_cast<uint32_t>(height),
                             1
                            };
       std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  
       vkCmdCopyBufferToImage
         (
          command_buffer,
          buffer,
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
         ([device = device, buffer, vulkan_image, vulkan_image_view] (auto&&)
           -> vulkan_image_loader::output_image_type
          {
            std::cout << __FILE__ << ":" << __LINE__ << std::endl;
            vkDestroyBuffer(device, buffer, nullptr);
            std::cout << __FILE__ << ":" << __LINE__ << std::endl;
            // vkFreeMemory(device, staging_pair.second, nullptr);
            // std::cout << __FILE__ << ":" << __LINE__ << std::endl;
            return {vulkan_image, vulkan_image_view};
          });
     });
}

template <typename Executor>
pc::future<typename vulkan_image_loader<Executor>::output_image_type> vulkan_image_loader<Executor>
  ::load (const void* buffer, int32_t width, int32_t height, uint32_t stride) const
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;

  std::cout << "running in a thread loop!" << std::endl;

  std::cout << "image " << width << "x" << height << " image  stride " << stride << std::endl;

  auto buffer_size = height*stride;
       
  auto staging_pair = fastdraw::output::vulkan::create_buffer
    (device, buffer_size, physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  void* data;
  auto r = from_result(vkMapMemory(device, staging_pair.second, 0, buffer_size, 0, &data));
  if (r != vulkan_error_code::success)
    throw std::system_error (make_error_code(r));

  std::memcpy (data, buffer, buffer_size);

  vkUnmapMemory(device, staging_pair.second);

  return load (staging_pair.first, width, height);
}

} } }

#endif
