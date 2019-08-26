///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_TOPLEVEL_WINDOW_HPP
#define FTK_FTK_UI_TOPLEVEL_WINDOW_HPP

#include <ftk/ui/backend/vulkan_vertex_buffer.hpp>
// #include <ftk/ui/backend/vulkan_storage_zindex.hpp>
// #include <ftk/ui/backend/vulkan_storage_zindex_array.hpp>
// #include <ftk/ui/backend/vulkan_storage_vector.hpp>
#include <ftk/ui/backend/vulkan_command_buffer_cache.hpp>
#include <ftk/ui/backend/vulkan_storage_buffer_allocator.hpp>
#include <ftk/ui/backend/descriptor_fixed_array.hpp>

#include <functional>

#include <iostream>

#include <mutex>
#include <list>

namespace ftk { namespace ui {

namespace detail {

// template <>
// struct zindex_traits<std::uint32_t>
// {
//   static constexpr std::uint32_t zero () { return 0u; }
//   static constexpr std::uint32_t next (std::uint32_t v)
//   {
//     return v+1;
//   }
// };

VkSampler render_thread_create_sampler (VkDevice device)
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;

  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 0;

  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

  samplerInfo.unnormalizedCoordinates = VK_FALSE;

  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
       
  std::cout << __FILE__ ":" << __LINE__ << std::endl;
  VkSampler texture_sampler;
  //CHRONO_COMPARE()
  auto r = from_result(vkCreateSampler(device, &samplerInfo, nullptr, &texture_sampler));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));

  return texture_sampler;
}

}      

struct toplevel_framebuffer_region
{
  std::int32_t x, y, width, height;

  friend bool operator== (toplevel_framebuffer_region const& lhs
                          , toplevel_framebuffer_region const& rhs)
  {
    return lhs.x == rhs.x && lhs.y == rhs.y
      && lhs.width == rhs.width && lhs.height == rhs.height;
  }
  friend bool operator!= (toplevel_framebuffer_region const& lhs
                          , toplevel_framebuffer_region const& rhs)
  {
    return !(lhs == rhs);
  }
};
    
struct toplevel_window_image
{
  VkImageView image_view;
  std::int32_t x, y, width, height;
  std::uint32_t zindex;

  bool must_draw[2] = {true, true};

  toplevel_framebuffer_region framebuffers_regions[2];
};

template <typename Backend>
struct toplevel_window
{
  toplevel_window (Backend& backend)
    : window (backend.create_window(1280, 1000))
    , image_pipeline (fastdraw::output::vulkan::create_image_pipeline (window.voutput, 0))
    , texture_descriptors (window.voutput.device, image_pipeline.descriptorSetLayouts[0])
    , sampler_descriptors (window.voutput.device, image_pipeline.descriptorSetLayouts[1])
    , buffer_allocator (window.voutput.device, window.voutput.physical_device
                        //, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
  {
    using fastdraw::output::vulkan::vulkan_error_code;
    using fastdraw::output::vulkan::from_result;
    
    //backend_->exposure_signal.connect (std::bind(&toplevel_window<Backend>::exposure, this));
    {
      VkBufferCreateInfo bufferInfo = {};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = sizeof(image_info) * /*extent.width * extent.height*/ 4096;
      bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      auto r = from_result(vkCreateBuffer(window.voutput.device, &bufferInfo, nullptr, &image_ssbo_buffer));
      if (r != vulkan_error_code::success)
        throw std::system_error(make_error_code (r));
    }
    buffer_allocator.allocate (image_ssbo_buffer);

    {
      VkBufferCreateInfo bufferInfo = {};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = sizeof(indirect_draw_info)*indirect_draw_info_array_size;
      bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
        | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      auto r = from_result(vkCreateBuffer(window.voutput.device, &bufferInfo, nullptr, &indirect_draw_buffer));
      if (r != vulkan_error_code::success)
        throw std::system_error(make_error_code (r));
    }
    buffer_allocator.allocate (indirect_draw_buffer);

    VkDescriptorImageInfo samplerInfo = {};
    samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerInfo.sampler = sampler;
    sampler_descriptors.push_back (samplerInfo);
  }

  toplevel_window (toplevel_window&& other) = delete; // for now

  typedef std::list<toplevel_window_image>::iterator image_iterator;

  image_iterator append_image (toplevel_window_image image)
  {
    std::cout << "appending image in " << image.x << "x" << image.y << std::endl;
    auto whole_width = window.voutput.swapChainExtent.width;
    auto whole_height = window.voutput.swapChainExtent.height;
    auto it = images.insert(images.begin(), image);
    auto x = it->x;
    auto y = it->y;
    auto width = it->width;
    auto height = it->height;
    auto zindex = create_new_zindex ();
    it->zindex = zindex;
    std::cout << "it->zindex " << it->zindex << std::endl;
    auto ssbo_data = buffer_allocator.map (image_ssbo_buffer);

    auto image_info_ptr = static_cast<image_info*>(ssbo_data) + zindex;
    *image_info_ptr = {static_cast<uint32_t>(zindex)
                       , static_cast<uint32_t>(x)
                       , static_cast<uint32_t>(y)
                       , static_cast<uint32_t>(width)
                       , static_cast<uint32_t>(height), 1, 0, 0};
    buffer_allocator.unmap (image_ssbo_buffer);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image.image_view;
    texture_descriptors.push_back (imageInfo);

    return it;
  }
  void replace_image_view (image_iterator image, VkImageView view)
  {
    image->must_draw[0] = true;
    image->must_draw[1] = true;
    image->image_view = view;

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view;
    texture_descriptors.replace (image->zindex, imageInfo);
  }
  void remove_image (image_iterator image)
  {
    toplevel_framebuffer_region empty{};

    std::cout << "removing image to " << image->x << "x" << image->y << " damage on fb[0]? "
              << (image->framebuffers_regions[0] != empty)
              << " fb[1]? " << (image->framebuffers_regions[1] != empty) << std::endl;

    if (/*!image->must_draw[0] && */image->framebuffers_regions[0] != empty)
    {
      std::cout << "pushing damage region fb[0] " << image->framebuffers_regions[0].x << "x"
                << image->framebuffers_regions[0].y << std::endl;
      framebuffers_damaged_regions[0].push_back (image->framebuffers_regions[0]);
      image->framebuffers_regions[0] = {};
    }
    if (/*!image->must_draw[1] && */image->framebuffers_regions[1] != empty)
    {
      std::cout << "pushing damage region fb[1] " << image->framebuffers_regions[1].x << "x"
                << image->framebuffers_regions[1].y << std::endl;
      framebuffers_damaged_regions[1].push_back (image->framebuffers_regions[1]);
      image->framebuffers_regions[1] = {};
    }
    image->must_draw[0] = false;
    image->must_draw[1] = false;
    image->width = 0;
    image->height = 0;

    {
      auto ssbo_data = buffer_allocator.map (image_ssbo_buffer);

      auto image_info_ptr = static_cast<image_info*>(ssbo_data) + image->zindex;
      *image_info_ptr = {static_cast<uint32_t>(image->zindex)
                         , static_cast<uint32_t>(image->x)
                         , static_cast<uint32_t>(image->y)
                         , static_cast<uint32_t>(image->width)
                         , static_cast<uint32_t>(image->height), 1, 0, 0};
      buffer_allocator.unmap (image_ssbo_buffer);
    }

    vkDestroyImageView(window.voutput.device, image->image_view, nullptr);
    image->image_view = VK_NULL_HANDLE;

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = images.back().image_view;
    texture_descriptors.replace (image->zindex, imageInfo);
  }
  void move_image (image_iterator image, std::int32_t x, std::int32_t y)
  {
    toplevel_framebuffer_region empty{};

    std::cout << "moving image to " << x << "x" << y << " damage on fb[0]? "
              << (image->framebuffers_regions[0] != empty)
              << " fb[1]? " << (image->framebuffers_regions[1] != empty) << std::endl;

    if (!image->must_draw[0] && image->framebuffers_regions[0] != empty)
    {
      std::cout << "pushing damage region fb[0] " << image->framebuffers_regions[0].x << "x"
                << image->framebuffers_regions[0].y << std::endl;
      framebuffers_damaged_regions[0].push_back (image->framebuffers_regions[0]);
      image->framebuffers_regions[0] = {};
    }
    if (!image->must_draw[1] && image->framebuffers_regions[1] != empty)
    {
      std::cout << "pushing damage region fb[1] " << image->framebuffers_regions[1].x << "x"
                << image->framebuffers_regions[1].y << std::endl;
      framebuffers_damaged_regions[1].push_back (image->framebuffers_regions[1]);
      image->framebuffers_regions[1] = {};
    }
    image->must_draw[0] = true;
    image->must_draw[1] = true;
    image->x = x;
    image->y = y;
    auto whole_width = window.voutput.swapChainExtent.width;
    auto whole_height = window.voutput.swapChainExtent.height;
    auto width = image->width;
    auto height = image->height;

    {
      auto ssbo_data = buffer_allocator.map (image_ssbo_buffer);

      auto image_info_ptr = static_cast<image_info*>(ssbo_data) + image->zindex;
      *image_info_ptr = {static_cast<uint32_t>(image->zindex)
                         , static_cast<uint32_t>(x)
                         , static_cast<uint32_t>(y)
                         , static_cast<uint32_t>(width)
                         , static_cast<uint32_t>(height), 1, 0, 0};
      buffer_allocator.unmap (image_ssbo_buffer);
    }
  }

  VkRenderPass create_compatible_render_pass ();

  int create_new_zindex ()
  {
    static int next_zindex = 0;
    return next_zindex ++;
  }
  
  void load_background (toplevel_window_image bg)
  {
    append_image (bg);
  }

  // toplevel_window_image background;

  struct image_info
  {
    uint32_t zindex;
    uint32_t x, y, w, h;
    uint32_t has_alpha;
    uint32_t found_alpha;
    uint32_t padding;
    
  };

  struct indirect_draw_info
  {
    VkDrawIndirectCommand indirect;
    uint32_t image_length;
    uint32_t fragment_data_length;
    uint32_t buffers_to_draw[4096];
    uint fg_zindex[4096];
  };

  std::list<toplevel_window_image> images;
  mutable typename Backend::window window;
  fastdraw::output::vulkan::vulkan_draw_info image_pipeline;
  vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096> texture_descriptors;
  vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLER, 1> sampler_descriptors;
  std::vector<toplevel_framebuffer_region> framebuffers_damaged_regions[2];
  vulkan::buffer_allocator buffer_allocator;
  VkBuffer image_ssbo_buffer;
  VkBuffer indirect_draw_buffer;
  std::mutex image_mutex; // for images vector and command_buffers cache
  VkSampler const sampler = detail::render_thread_create_sampler (window.voutput.device);

  std::size_t const static constexpr indirect_draw_info_array_size = 32u;
};
    
} }

#endif
