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
#include <ftk/ui/backend/vulkan_queues.hpp>

#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>
#include <fastdraw/output/vulkan/add_image.hpp>

#include <functional>

#include <iostream>
#include <variant>
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

struct image_component
{
  VkImageView image_view;
  std::uint32_t texture_descriptor_index;
};

struct button_component
{
};
struct rectangle_component
{
  float color[4];
};
    
struct toplevel_window_component
{
  std::int32_t x, y, width, height;
  std::variant<image_component, button_component, rectangle_component> component_data;

  bool must_draw[2] = {true, true};

  toplevel_framebuffer_region framebuffers_regions[2];
};

template <typename Backend>
struct toplevel_window
{
  toplevel_window (Backend& backend, std::filesystem::path resource_path)
    : window (backend.create_window(1280, 1000, resource_path))
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
      bufferInfo.size = sizeof(component_info) * /*extent.width * extent.height*/ 4096;
      bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      auto r = from_result(vkCreateBuffer(window.voutput.device, &bufferInfo, nullptr, &component_ssbo_buffer));
      if (r != vulkan_error_code::success)
        throw std::system_error(make_error_code (r));
    }
    buffer_allocator.allocate (component_ssbo_buffer);

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

  typedef std::list<toplevel_window_component>::iterator component_iterator;

  component_iterator append_component (toplevel_window_component component)
  {
    std::cout << "appending component in " << component.x << "x" << component.y << std::endl;
    auto whole_width = window.voutput.swapChainExtent.width;
    auto whole_height = window.voutput.swapChainExtent.height;
    auto it = components.insert(components.begin(), component);
    auto x = it->x;
    auto y = it->y;
    auto width = it->width;
    auto height = it->height;
    auto texture_descriptor_index = create_new_zindex ();
    auto ssbo_component_index = texture_descriptor_index;
    //it->zindex = zindex;
    //std::cout << "it->zindex " << it->zindex << std::endl;
    auto ssbo_data = buffer_allocator.map (component_ssbo_buffer);

    auto component_info_ptr = static_cast<component_info*>(ssbo_data) + ssbo_component_index;
    *component_info_ptr = {static_cast<uint32_t>(texture_descriptor_index)
                           , static_cast<uint32_t>(x)
                           , static_cast<uint32_t>(y)
                           , static_cast<uint32_t>(width)
                           , static_cast<uint32_t>(height), 1, 0, get_component_type (component.component_data)};
    buffer_allocator.unmap (component_ssbo_buffer);

    if (image_component* p = std::get_if<image_component>(&component.component_data))
    {
      VkDescriptorImageInfo imageInfo = {};
      imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      imageInfo.imageView = p->image_view;
      texture_descriptors.push_back (imageInfo);
      p->texture_descriptor_index = texture_descriptor_index;
    }

    return it;
  }
  void replace_image_view (component_iterator component, VkImageView view)
  {
    image_component* p = std::get_if<image_component>(&component->component_data);
    assert (p != nullptr);

    component->must_draw[0] = true;
    component->must_draw[1] = true;
    p->image_view = view;

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view;
    texture_descriptors.replace (p->texture_descriptor_index, imageInfo);
  }
  void remove_component (component_iterator component)
  {
    toplevel_framebuffer_region empty{};

    std::cout << "removing component to " << component->x << "x" << component->y << " damage on fb[0]? "
              << (component->framebuffers_regions[0] != empty)
              << " fb[1]? " << (component->framebuffers_regions[1] != empty) << std::endl;

    if (/*!component->must_draw[0] && */component->framebuffers_regions[0] != empty)
    {
      std::cout << "pushing damage region fb[0] " << component->framebuffers_regions[0].x << "x"
                << component->framebuffers_regions[0].y << std::endl;
      framebuffers_damaged_regions[0].push_back (component->framebuffers_regions[0]);
      component->framebuffers_regions[0] = {};
    }
    if (/*!component->must_draw[1] && */component->framebuffers_regions[1] != empty)
    {
      std::cout << "pushing damage region fb[1] " << component->framebuffers_regions[1].x << "x"
                << component->framebuffers_regions[1].y << std::endl;
      framebuffers_damaged_regions[1].push_back (component->framebuffers_regions[1]);
      component->framebuffers_regions[1] = {};
    }
    component->must_draw[0] = false;
    component->must_draw[1] = false;
    component->width = 0;
    component->height = 0;

    {
      auto ssbo_component_index = std::distance(components.begin(), component);
      auto texture_descriptor_index = ssbo_component_index;
      if (image_component* p = std::get_if<image_component>(&component->component_data))
        texture_descriptor_index = p->texture_descriptor_index;

      auto ssbo_data = buffer_allocator.map (component_ssbo_buffer);
      auto component_info_ptr = static_cast<component_info*>(ssbo_data) + ssbo_component_index;
      *component_info_ptr = {static_cast<uint32_t>(texture_descriptor_index)
                             , static_cast<uint32_t>(component->x)
                             , static_cast<uint32_t>(component->y)
                             , static_cast<uint32_t>(component->width)
                             , static_cast<uint32_t>(component->height), 1, 0, 0, get_component_type (component->component_data)};
      buffer_allocator.unmap (component_ssbo_buffer);
    }

    // if (image_component* p = std::get_if<image_component>(&component->component_data))
    // {
    //   vkDestroyImageView(window.voutput.device, p->image_view, nullptr);
    //   p->image_view = VK_NULL_HANDLE;

    //   // VkDescriptorImageInfo imageInfo = {};
    //   // imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    //   // imageInfo.imageView = components.back().image_view;
    //   // texture_descriptors.replace (p->texture_descriptor_index, imageInfo);
    // }
  }
  void move_component (component_iterator component, std::int32_t x, std::int32_t y)
  {
    toplevel_framebuffer_region empty{};

    std::cout << "moving component to " << x << "x" << y << " damage on fb[0]? "
              << (component->framebuffers_regions[0] != empty)
              << " fb[1]? " << (component->framebuffers_regions[1] != empty) << std::endl;

    if (!component->must_draw[0] && component->framebuffers_regions[0] != empty)
    {
      std::cout << "pushing damage region fb[0] " << component->framebuffers_regions[0].x << "x"
                << component->framebuffers_regions[0].y << std::endl;
      framebuffers_damaged_regions[0].push_back (component->framebuffers_regions[0]);
      component->framebuffers_regions[0] = {};
    }
    if (!component->must_draw[1] && component->framebuffers_regions[1] != empty)
    {
      std::cout << "pushing damage region fb[1] " << component->framebuffers_regions[1].x << "x"
                << component->framebuffers_regions[1].y << std::endl;
      framebuffers_damaged_regions[1].push_back (component->framebuffers_regions[1]);
      component->framebuffers_regions[1] = {};
    }
    component->must_draw[0] = true;
    component->must_draw[1] = true;
    component->x = x;
    component->y = y;
    auto whole_width = window.voutput.swapChainExtent.width;
    auto whole_height = window.voutput.swapChainExtent.height;
    auto width = component->width;
    auto height = component->height;

    {
      auto ssbo_data = buffer_allocator.map (component_ssbo_buffer);
      auto ssbo_component_index = std::distance (components.begin(), component);
      auto texture_descriptor_index = ssbo_component_index;
      if (image_component* p = std::get_if<image_component>(&component->component_data))
        texture_descriptor_index = p->texture_descriptor_index;

      auto component_info_ptr = static_cast<component_info*>(ssbo_data) + ssbo_component_index;
      *component_info_ptr = {static_cast<uint32_t>(texture_descriptor_index)
                         , static_cast<uint32_t>(x)
                         , static_cast<uint32_t>(y)
                         , static_cast<uint32_t>(width)
                         , static_cast<uint32_t>(height), 1, 0, 0};
      buffer_allocator.unmap (component_ssbo_buffer);
    }
  }

  VkRenderPass create_compatible_render_pass ();

  int create_new_zindex ()
  {
    static int next_zindex = 0;
    return next_zindex ++;
  }
  
  void load_background (toplevel_window_component bg)
  {
    append_component (bg);
  }

  enum class component_type
  {
    image, button, rectangle
  };

  struct image_data {};
  struct button_data {};
  struct rectangle_data
  {
    float color[4];
  };

  uint32_t get_component_type (std::variant<image_component, button_component, rectangle_component> const& v)
  {
    return v.index();
  }
  
  struct component_info
  {
    uint32_t descriptor_index;
    uint32_t x, y, w, h;
    uint32_t has_alpha;
    uint32_t found_alpha;
    uint32_t component_type;
    std::variant <image_data, button_data, rectangle_data> component_data;
  };

  struct indirect_draw_info
  {
    VkDrawIndirectCommand indirect;
    uint32_t component_length;
    uint32_t fragment_data_length;
    uint32_t buffers_to_draw[4096];
    uint fg_zindex[4096];
  };

  std::list<toplevel_window_component> components;
  mutable typename Backend::window window;
  fastdraw::output::vulkan::vulkan_draw_info image_pipeline;
  vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096> texture_descriptors;
  vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLER, 1> sampler_descriptors;
  std::vector<toplevel_framebuffer_region> framebuffers_damaged_regions[2];
  vulkan::buffer_allocator buffer_allocator;
  VkBuffer component_ssbo_buffer;
  VkBuffer indirect_draw_buffer;
  //std::mutex image_mutex; // for images vector and command_buffers cache
  VkSampler const sampler = detail::render_thread_create_sampler (window.voutput.device);

  std::size_t const static constexpr indirect_draw_info_array_size = 32u;
};
    
} }

#endif
