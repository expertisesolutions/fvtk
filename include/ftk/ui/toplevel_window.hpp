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

#include <ftk/ui/backend/vulkan/vertex_buffer.hpp>
#include <ftk/ui/backend/vulkan/command_buffer_cache.hpp>
#include <ftk/ui/backend/vulkan/storage_buffer_allocator.hpp>
#include <ftk/ui/backend/vulkan/descriptor_fixed_array.hpp>
#include <ftk/ui/backend/vulkan/queues.hpp>
#include <ftk/ui/backend/vulkan/load.hpp>

#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>
#include <fastdraw/output/vulkan/add_image.hpp>

#include <functional>

#include <iostream>
#include <variant>
#include <mutex>
#include <list>
#include <type_traits>

namespace ftk { namespace ui {

namespace backend { namespace vulkan {

VkSemaphore render_thread_create_semaphore (VkDevice device)
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;
  VkSemaphore semaphore;
  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  auto r = from_result(vkCreateSemaphore (device, &semaphoreInfo, nullptr, &semaphore));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code(r));
  return semaphore;
}

  
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

} }      

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
  fastdraw::color::color_premultiplied_rgba<uint8_t> color;
};

template <std::size_t swapchain_image_count>
struct toplevel_window_component
{
  std::int32_t x, y, width, height;
  std::variant<image_component, button_component, rectangle_component> component_data;

  bool must_draw[swapchain_image_count] = {true, true};

  toplevel_framebuffer_region framebuffers_regions[swapchain_image_count];
};

template <typename Backend, std::size_t swapchain_image_count>
struct toplevel_window
{
  typedef ui::toplevel_window_component<swapchain_image_count> window_component;
  
  static constexpr const bool backend_is_reference = std::is_reference<Backend>::type::value;
  using backend_type = typename std::conditional
    <backend_is_reference
     , Backend
     , typename std::remove_reference<Backend>::type>::type;
  using backend_window_type = typename std::conditional
    <backend_is_reference
     , typename std::add_lvalue_reference<typename std::remove_reference<Backend>::type::window>::type
     , typename std::remove_reference<Backend>::type::window>::type;
  
  toplevel_window (Backend& backend, std::filesystem::path resource_path
                   , VkImageView empty_image_view)
    : window (backend.create_window(1280, 1000, resource_path))
    , image_pipeline (fastdraw::output::vulkan::create_image_pipeline (window.voutput, 0))
    , sampler_descriptors (window.voutput.device, image_pipeline.descriptorSetLayouts[1])
    , buffer_allocator (window.voutput.device, window.voutput.physical_device
                        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    , swapchain_info ({swapchain_initialize(empty_image_view), swapchain_initialize(empty_image_view)})
  {
    initialize_buffers ();
  }

  toplevel_window (backend_window_type& window, std::filesystem::path resource_path
                   , VkImageView empty_image_view)
    : window (window)
    , image_pipeline (fastdraw::output::vulkan::create_image_pipeline (window.voutput, 0))
    , sampler_descriptors (window.voutput.device, image_pipeline.descriptorSetLayouts[1])
    , buffer_allocator (window.voutput.device, window.voutput.physical_device
                        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    , swapchain_info ({swapchain_initialize(empty_image_view), swapchain_initialize(empty_image_view)})
  {
    initialize_buffers ();
  }

  VkBuffer allocate_buffer (std::size_t size, VkBufferUsageFlags usage)
  {
    using fastdraw::output::vulkan::vulkan_error_code;
    using fastdraw::output::vulkan::from_result;
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    auto r = from_result(vkCreateBuffer(window.voutput.device, &bufferInfo, nullptr, &buffer));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));

    buffer_allocator.allocate (buffer);
    return buffer;
  }

  struct swapchain_specific_information;
  swapchain_specific_information swapchain_initialize (VkImageView empty_image_view)
  {
    swapchain_specific_information v
      {
       {window.voutput.device, image_pipeline.descriptorSetLayouts[0]
                 , VkDescriptorImageInfo
                 {nullptr, empty_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}}
        , allocate_buffer (sizeof(component_info)*4096, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        , allocate_buffer (sizeof(indirect_draw_info)*indirect_draw_info_array_size
                           , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
                           | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
       , {}
       , backend::vulkan::render_thread_create_semaphore (window.voutput.device)
      };
    return v;
  }
  
  void initialize_buffers ()
  {
    VkDescriptorImageInfo samplerInfo = {};
    samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerInfo.sampler = sampler;
    sampler_descriptors.push_back (samplerInfo);
  }

  toplevel_window (toplevel_window&& other) = delete; // for now

  typedef typename std::list<window_component>::iterator component_iterator;

  // modifies components, ssbo buffer and texture descriptor
  component_iterator append_image (std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h, VkImageView image)
  {
    auto texture_descriptor_index = create_new_zindex();
    window_component component
      = {x, y, w, h, image_component{image, texture_descriptor_index}};

    std::cout << "appending image in " << component.x << "x" << component.y << std::endl;
    auto it = components.insert(components.begin(), component);

    typename component_operation::append_image_operation op
      {x, y, w, h, image, texture_descriptor_index
       , std::distance (it, components.end())-1};
    queue_operation(op);
    
    return it;
  }

  component_iterator append_rectangle (std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h
                                       , fastdraw::color::color_premultiplied_rgba<uint8_t> color)
  {
    //auto texture_descriptor_index = create_new_zindex();
    window_component component
      = {x, y, w, h, rectangle_component{color}};

    std::cout << "appending rectangle in " << component.x << "x" << component.y << std::endl;
    auto it = components.insert(components.begin(), component);

    fastdraw::color::color_premultiplied_rgba<float> dst_color {0.0f, 0.0f, 0.0f, 0.0f};
    dst_color = dst_color.blend_with_src (color);
    typename component_operation::append_rectangle_operation op
      {x, y, w, h, {dst_color.red(), dst_color.green(), dst_color.blue(), dst_color.alpha()}
       , /*reverse index */ std::distance(it, components.end())-1};
    queue_operation(op);

    return it;
  }
  void replace_image_view (component_iterator component, VkImageView view)
  {
    // image_component* p = std::get_if<image_component>(&component->component_data);
    // assert (p != nullptr);

    // component->must_draw[0] = true;
    // component->must_draw[1] = true;
    // p->image_view = view;

    // VkDescriptorImageInfo imageInfo = {};
    // imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // imageInfo.imageView = view;
    // texture_descriptors.replace (p->texture_descriptor_index, imageInfo);
  }
  void remove_component (component_iterator component)
  {
    // toplevel_framebuffer_region empty{};

    // std::cout << "removing component to " << component->x << "x" << component->y << " damage on fb[0]? "
    //           << (component->framebuffers_regions[0] != empty)
    //           << " fb[1]? " << (component->framebuffers_regions[1] != empty) << std::endl;

    // if (/*!component->must_draw[0] && */component->framebuffers_regions[0] != empty)
    // {
    //   std::cout << "pushing damage region fb[0] " << component->framebuffers_regions[0].x << "x"
    //             << component->framebuffers_regions[0].y << std::endl;
    //   framebuffers_damaged_regions[0].push_back (component->framebuffers_regions[0]);
    //   component->framebuffers_regions[0] = {};
    // }
    // if (/*!component->must_draw[1] && */component->framebuffers_regions[1] != empty)
    // {
    //   std::cout << "pushing damage region fb[1] " << component->framebuffers_regions[1].x << "x"
    //             << component->framebuffers_regions[1].y << std::endl;
    //   framebuffers_damaged_regions[1].push_back (component->framebuffers_regions[1]);
    //   component->framebuffers_regions[1] = {};
    // }
    // component->must_draw[0] = false;
    // component->must_draw[1] = false;
    // component->width = 0;
    // component->height = 0;

    // {
    //   auto ssbo_component_index = std::distance(components.begin(), component);
    //   auto texture_descriptor_index = ssbo_component_index;
    //   if (image_component* p = std::get_if<image_component>(&component->component_data))
    //     texture_descriptor_index = p->texture_descriptor_index;

    //   auto ssbo_data = buffer_allocator.map (component_ssbo_buffer);
    //   auto component_info_ptr = static_cast<component_info*>(ssbo_data) + ssbo_component_index;
    //   *component_info_ptr = {static_cast<uint32_t>(texture_descriptor_index)
    //                          , static_cast<uint32_t>(component->x)
    //                          , static_cast<uint32_t>(component->y)
    //                          , static_cast<uint32_t>(component->width)
    //                          , static_cast<uint32_t>(component->height), 1, 0, get_component_type (component->component_data)};
    //   buffer_allocator.unmap (component_ssbo_buffer);
    // }

    // // if (image_component* p = std::get_if<image_component>(&component->component_data))
    // // {
    // //   vkDestroyImageView(window.voutput.device, p->image_view, nullptr);
    // //   p->image_view = VK_NULL_HANDLE;

    // //   // VkDescriptorImageInfo imageInfo = {};
    // //   // imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // //   // imageInfo.imageView = components.back().image_view;
    // //   // texture_descriptors.replace (p->texture_descriptor_index, imageInfo);
    // // }
  }
  void move_component (component_iterator component, std::int32_t x, std::int32_t y)
  {
    std::int32_t old_x = component->x
      , old_y = component->y;
    component->must_draw[0] = true;
    component->must_draw[1] = true;
    component->x = x;
    component->y = y;

    typename component_operation::move_component_operation op {component};
    queue_operation (op);
    
  }

  VkRenderPass create_compatible_render_pass ();

  unsigned int create_new_zindex ()
  {
    static unsigned int next_zindex = 0;
    return next_zindex ++;
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
    //uint32_t has_alpha;
    uint32_t found_alpha;
    uint32_t component_type;
    //std::variant <image_data, button_data, rectangle_data> component_data;
    uint32_t padding;
    rectangle_data component_data;
    //static_assert (sizeof (std::variant <image_data, button_data, rectangle_data>) == sizeof (float)*4 + sizeof(uint32_t), "");

    struct dummy
    {
      uint descriptor_index;
      uint ii_x, ii_y, ii_w, ii_h;
      //uint alpha_compositing;
      uint found_alpha;
      uint component_type;
      uint padding0;
      uint padding[4];
    };
  };
  static_assert (sizeof (component_info) == sizeof (typename component_info::dummy), "");

  struct indirect_draw_info
  {
    VkDrawIndirectCommand indirect;
    uint32_t component_length;
    uint32_t fragment_data_length;
    uint32_t buffers_to_draw[4096];
    uint fg_zindex[4096];
  };

  struct component_operation
  {
    struct append_image_operation
    {
      std::int32_t x, y, w, h;
      VkImageView view;
      std::uint32_t texture_descriptor_index;
      std::uint32_t component_index;
    };
    struct append_rectangle_operation
    {
      std::int32_t x, y, w, h;
      float color[4];
      std::uint32_t component_index;
    };
    struct replace_image_view_operation
    {
      component_iterator component;
      VkImageView view;
    };
    struct remove_component_operation
    {
      //component_iterator component;
    };
    struct move_component_operation
    {
      component_iterator component;
      //std::int32_t old_x, old_y;
    };
    
    std::variant<append_image_operation, append_rectangle_operation
                 //, replace_image_view_operation
                 //, remove_component_operation
                 , move_component_operation> operation;
  };

  void transfer_operation (typename component_operation::append_image_operation op, swapchain_specific_information& swapchain_info
                           , unsigned int swapchain_index)
  {
    auto ssbo_data = buffer_allocator.map (swapchain_info.component_ssbo_buffer);

    std::cout << "image component_index " << op.component_index << std::endl;
    auto component_info_ptr = static_cast<component_info*>(ssbo_data) + op.component_index;
    *component_info_ptr = {static_cast<uint32_t>(op.texture_descriptor_index)
                           , static_cast<uint32_t>(op.x)
                           , static_cast<uint32_t>(op.y)
                           , static_cast<uint32_t>(op.w)
                           , static_cast<uint32_t>(op.h), 1/*, 0*/, static_cast<int>(component_type::image) /* image */};

    buffer_allocator.unmap (swapchain_info.component_ssbo_buffer);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = op.view;
    swapchain_info.texture_descriptors.push_back (imageInfo);
  }

  void transfer_operation (typename component_operation::append_rectangle_operation op, swapchain_specific_information& swapchain_info
                           , unsigned int swapchain_index)
  {
    auto ssbo_data = buffer_allocator.map (swapchain_info.component_ssbo_buffer);

    std::cout << "rectangle component_index " << op.component_index << std::endl;
    auto component_info_ptr = static_cast<component_info*>(ssbo_data) + op.component_index;
    *component_info_ptr = {0ul
                           , static_cast<uint32_t>(op.x)
                           , static_cast<uint32_t>(op.y)
                           , static_cast<uint32_t>(op.w)
                           , static_cast<uint32_t>(op.h), 1/*, 0*/, static_cast<int>(component_type::rectangle)};

    component_info_ptr->component_data = rectangle_data {op.color[0], op.color[1], op.color[2], op.color[3]};

    buffer_allocator.unmap (swapchain_info.component_ssbo_buffer);
  }

  void transfer_operation (typename component_operation::move_component_operation op, swapchain_specific_information& swapchain_info
                           , unsigned int swapchain_index)
  {
    toplevel_framebuffer_region empty{};

    std::cout << "moving component to " << op.component->x << "x" << op.component->y << " damage on fb[" << swapchain_index << "]? "
              << (op.component->framebuffers_regions[swapchain_index] != empty) << std::endl;

    if (!op.component->must_draw[swapchain_index] && op.component->framebuffers_regions[swapchain_index] != empty)
    {
      std::cout << "pushing damage region fb[" << swapchain_index << "] " << op.component->framebuffers_regions[swapchain_index].x << "x"
                << op.component->framebuffers_regions[swapchain_index].y << std::endl;
      swapchain_info.framebuffers_damaged_regions.push_back (op.component->framebuffers_regions[swapchain_index]);
      op.component->framebuffers_regions[swapchain_index] = {};
      op.component->must_draw[swapchain_index] = true;
    }

    {
      auto ssbo_data = buffer_allocator.map (swapchain_info.component_ssbo_buffer);
      auto ssbo_component_index = std::distance (op.component, components.end())-1;
      auto texture_descriptor_index = 0;
      if (image_component* p = std::get_if<image_component>(&op.component->component_data))
        texture_descriptor_index = p->texture_descriptor_index;

      std::cout << "move operation component index " << ssbo_component_index
                <<  " descriptor index " << texture_descriptor_index << std::endl;

      auto component_info_ptr = static_cast<component_info*>(ssbo_data) + ssbo_component_index;
      *component_info_ptr = {static_cast<uint32_t>(texture_descriptor_index)
                         , static_cast<uint32_t>(op.component->x)
                         , static_cast<uint32_t>(op.component->y)
                         , static_cast<uint32_t>(op.component->width)
                             , static_cast<uint32_t>(op.component->height), 1/*, 0*/, get_component_type (op.component->component_data)};
      buffer_allocator.unmap (swapchain_info.component_ssbo_buffer);
    }
  }

  template <typename Op>
  void queue_operation (Op op)
  {
    // which is available?
    for (unsigned int i = 0; i != swapchain_image_count; ++i)
    {
      std::unique_lock<std::mutex> l(swapchain_info[i].in_use_mutex);
      if (swapchain_info[i].is_in_use)
      {
        std::cout << "queueing op" << std::endl;
        swapchain_info[i].transfer_pending_operations.push_back ({op});
      }
      else
      {
        std::cout << "transfering op" << std::endl;
        transfer_operation (op, swapchain_info[i], i);
      }
    }
    
  }
  
  struct swapchain_specific_information
  {
    std::vector<toplevel_framebuffer_region> framebuffers_damaged_regions;
    std::vector<component_operation> transfer_pending_operations;

    // can't be modified while command buffer is used
    VkCommandBuffer command_buffer_in_use;
    backend::vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096> texture_descriptors;
    VkBuffer component_ssbo_buffer;
    VkBuffer indirect_draw_buffer;
    VkFence execution_finished;
    bool buffer_is_dirty;   /// do we need this?
    //VkSemaphore render_finished;
    
    std::mutex in_use_mutex;
    bool is_in_use;

    swapchain_specific_information (backend::vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096> texture_descriptors
                                    , VkBuffer component_ssbo_buffer, VkBuffer indirect_draw_buffer
                                    , VkFence execution_finished, VkSemaphore render_finished)
      : texture_descriptors (texture_descriptors)
      , component_ssbo_buffer (component_ssbo_buffer)
      , indirect_draw_buffer (indirect_draw_buffer)
      , execution_finished (execution_finished)
        //, render_finished (render_finished)
      , in_use_mutex {}
      , is_in_use (false)
    {}
    
    swapchain_specific_information (swapchain_specific_information&& other)
      : framebuffers_damaged_regions (std::move(other.framebuffers_damaged_regions))
      , transfer_pending_operations (std::move(other.transfer_pending_operations))
      , command_buffer_in_use (std::move(other.command_buffer_in_use))
      , texture_descriptors (std::move(other.texture_descriptors))
      , component_ssbo_buffer (std::move(other.component_ssbo_buffer))
      , indirect_draw_buffer (std::move(other.indirect_draw_buffer))
      , execution_finished (std::move(other.execution_finished))
      , buffer_is_dirty (std::move(other.buffer_is_dirty))
        //, render_finished (std::move(other.render_finished))
      , in_use_mutex {}
      , is_in_use (other.is_in_use)
    {}
  };

  std::list<window_component> components;
  backend_window_type window;
  fastdraw::output::vulkan::vulkan_draw_info image_pipeline;
  backend::vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLER, 1> sampler_descriptors;
  backend::vulkan::buffer_allocator buffer_allocator;
  swapchain_specific_information swapchain_info[swapchain_image_count];
  //std::mutex image_mutex; // for images vector and command_buffers cache
  VkSampler const sampler = backend::vulkan::render_thread_create_sampler (window.voutput.device);

  std::size_t const static constexpr indirect_draw_info_array_size = 32u;
};
    
} }

#endif
