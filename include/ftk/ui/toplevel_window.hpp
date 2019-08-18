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
    
struct toplevel_window_command_buffer_cache
{
  //VkCommandBuffer command_buffer[2]; // for each framebuffer
  unsigned int vertex_buffer_offset;
};
    
struct toplevel_window_image
{
  VkImageView image_view;
  std::int32_t x, y, width, height;
  std::uint32_t zindex;

  bool must_draw[2] = {true, true};

  toplevel_framebuffer_region framebuffers_regions[2];

  std::optional<toplevel_window_command_buffer_cache> cache;
};

template <typename Backend>
struct toplevel_window
{
  toplevel_window (Backend& backend)
  //: backend_(&backend)
    : window (backend.create_window(1280, 1000))
    , image_pipeline (fastdraw::output::vulkan::create_image_pipeline (window.voutput, 0))
    , texture_descriptors (window.voutput.device, image_pipeline.descriptorSetLayouts[0])
    , sampler_descriptors (window.voutput.device, image_pipeline.descriptorSetLayouts[1])
    , vbuffer (window.voutput.device, window.voutput.physical_device)
    , buffer_allocator (window.voutput.device, window.voutput.physical_device
                        //, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                        | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    // , storage_zindex (window.voutput.device, window.voutput.physical_device
    //                  , window.voutput.swapChainExtent)
    // , zindex_array (window.voutput.device, window.voutput.physical_device)
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
      bufferInfo.size = sizeof(uint32_t)*window.voutput.swapChainExtent.width * window.voutput.swapChainExtent.height;
      bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      auto r = from_result(vkCreateBuffer(window.voutput.device, &bufferInfo, nullptr, &image_zindex_ssbo_buffer));
      if (r != vulkan_error_code::success)
        throw std::system_error(make_error_code (r));
    }
    buffer_allocator.allocate (image_zindex_ssbo_buffer);

    // zero initialize zindex buffer
    {
      auto data = buffer_allocator.map (image_zindex_ssbo_buffer);
      std::memset (data, 0, sizeof(uint32_t) * window.voutput.swapChainExtent.width
                   * window.voutput.swapChainExtent.height);
      
      buffer_allocator.unmap (image_zindex_ssbo_buffer);
    }

    {
      VkBufferCreateInfo bufferInfo = {};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = sizeof(indirect_draw_info)*indirect_draw_info_array_size;
      bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      auto r = from_result(vkCreateBuffer(window.voutput.device, &bufferInfo, nullptr, &indirect_draw_buffer));
      if (r != vulkan_error_code::success)
        throw std::system_error(make_error_code (r));
    }
    buffer_allocator.allocate (indirect_draw_buffer);

    // initialize indirect buffer
    {
      auto data = buffer_allocator.map (indirect_draw_buffer);

      std::memset (data, 0, sizeof (indirect_draw_info)*indirect_draw_info_array_size);

      for (indirect_draw_info* p = static_cast<indirect_draw_info*>(data)
             , *last = p + indirect_draw_info_array_size
             ;p != last; ++p)
      {
        std::memset (&p->fg_zindex[0], 0xFF, sizeof (p->fg_zindex));
        p->indirect.vertexCount = 6;
      }
      
      buffer_allocator.unmap (indirect_draw_buffer);
    }
    
    VkDescriptorImageInfo samplerInfo = {};
    samplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    samplerInfo.sampler = sampler;
    sampler_descriptors.push_back (samplerInfo);
  }

  toplevel_window (toplevel_window&& other) = delete; // for now

  // void exposure ()
  // {
  //   std::cout << "exposure event for toplevel window" << std::endl;

  //   // cached scene
    
  // }

  // std::size_t  add_on_top (void* buffer, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, std::uint32_t stride)
  // {
  //   shm_buffers.push_back ({buffer, x, y, width, height, stride});
  // }

  //std::mutex render_mutex;

  //void create_vertex_buffer();
  
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
    auto /*zinfo*/zindex = /*zindex_array.*/create_new_zindex ();
    it->zindex = /*zinfo.*/zindex;
    std::cout << "it->zindex " << it->zindex << std::endl;
    auto ssbo_data = buffer_allocator.map (image_ssbo_buffer);

    auto image_info_ptr = static_cast<image_info*>(ssbo_data) + zindex;
    *image_info_ptr = {static_cast<uint32_t>(zindex)
                       , static_cast<uint32_t>(x)
                       , static_cast<uint32_t>(y)
                       , static_cast<uint32_t>(width)
                       , static_cast<uint32_t>(height), 1, 0, 0};
    buffer_allocator.unmap (image_ssbo_buffer);
    {
      auto data = buffer_allocator.map (indirect_draw_buffer);
      // workaround for now
      for (indirect_draw_info* p = static_cast<indirect_draw_info*>(data)
             , *last = p + indirect_draw_info_array_size
             ; p != last; ++p)
      {
        std::cout << " p->image_length " << p->image_length << std::endl;
        p->image_length++;
      }
      
      buffer_allocator.unmap (indirect_draw_buffer);
    }

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = image.image_view;
    texture_descriptors.push_back (imageInfo);
    
    std::vector<VkImageView> image_views;// {background.image_view};
    for (auto&& image : images)
    {
      image_views.push_back (image.image_view);
    }
    
    vertex_info vinfo =
      {
       {
        // -1 because the positions are inclusive, and not past the end
        fastdraw::coordinates::ratio(x, whole_width)
        , fastdraw::coordinates::ratio(y, whole_height)
        , 0.0f, 1.0f
        , fastdraw::coordinates::ratio(x + width-1, whole_width)
        , fastdraw::coordinates::ratio(y, whole_height)
        , 0.0f, 1.0f
        , fastdraw::coordinates::ratio(x + width-1, whole_width)
        , fastdraw::coordinates::ratio(y + height-1, whole_height)
        , 0.0f, 1.0f
        , fastdraw::coordinates::ratio(x + width-1, whole_width)
        , fastdraw::coordinates::ratio(y + height-1, whole_height)
        , 0.0f, 1.0f
        , fastdraw::coordinates::ratio(x, whole_width)
        , fastdraw::coordinates::ratio(y + height-1, whole_height)
        , 0.0f, 1.0f
        , fastdraw::coordinates::ratio(x, whole_width)
        , fastdraw::coordinates::ratio(y, whole_height)
        , 0.0f, 1.0f
       }
       , {
            0.0f, 0.0f
          , 1.0f, 0.0f
          , 1.0f, 1.0f
          , 1.0f, 1.0f
          , 0.0f, 1.0f
          , 0.0f, 0.0f
       }
       , {
            1.0f, 0.0f, 0.0f, 0.0f
          , 0.0f, 1.0f, 0.0f, 0.0f
          , 0.0f, 0.0f, 1.0f, 0.0f
          , 0.0f, 0.0f, 0.0f, 1.0f
       }
       , /*zinfo.*/static_cast<unsigned int>(zindex)
       , {{}, {}, {}} // padding
      };

    auto invalidated_vbuffer = vbuffer.size() == vbuffer.capacity();
    auto buffer_cache_offset =  vbuffer.push_back (vinfo);
    // if (invalidated_vbuffer)
    // {
    //   //auto buffer_cache_offset =  vbuffer.push_back (vinfo);
    //   //static_cast<void>(buffer_cache_offset); // it is the last element

    //   for (auto&& image : images)
    //   {
    //     // if (image.cache)
    //     // {
    //     //   // vkFreeCommandBuffers (window.voutput.device, window.voutput.command_pool
    //     //   //                       , 2, image.cache->command_buffer);
    //     //   image.cache =
    //     //     toplevel_window_command_buffer_cache
    //     //     {/*{create_command_buffer
    //     //       (window.voutput.command_pool
    //     //        , window.voutput.device
    //     //        , window.swapChainFramebuffers[0]
    //     //        , window.voutput.swapChainExtent
    //     //        , renderPass
    //     //        , vbuffer.get_buffer()
    //     //        , sampler
    //     //        , image_views
    //     //        , image.x, image.y, image.width, image.height
    //     //        , image.cache->vertex_buffer_offset
    //     //        , image_pipeline
    //     //        , storage_zindex.get_buffer()
    //     //        , zindex_array.get_buffer()
    //     //        )
    //     //       , create_command_buffer
    //     //       (window.voutput.command_pool
    //     //        , window.voutput.device
    //     //        , window.swapChainFramebuffers[1]
    //     //        , window.voutput.swapChainExtent
    //     //        , renderPass
    //     //        , vbuffer.get_buffer()
    //     //        , sampler
    //     //        , image_views
    //     //        , image.x, image.y, image.width, image.height
    //     //        , image.cache->vertex_buffer_offset
    //     //        , image_pipeline
    //     //        , storage_zindex.get_buffer()
    //     //        , zindex_array.get_buffer()
    //     //        )}
    //     //        , */image.cache->vertex_buffer_offset};
    //     // }
    //   }
    // }

    it->cache = toplevel_window_command_buffer_cache
        {/*{create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[0]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image_views
           , x, y, width, height
           , buffer_cache_offset
           , image_pipeline
           , storage_zindex.get_buffer()
           , zindex_array.get_buffer()
           )
          , create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[1]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image_views
           , x, y, width, height
           , buffer_cache_offset
           , image_pipeline
           , storage_zindex.get_buffer()
           , zindex_array.get_buffer()
           )}
           , */buffer_cache_offset};
    return it;
  }
  void replace_image_view (image_iterator image, VkImageView view)
  {
    // vkFreeCommandBuffers (window.voutput.device, window.voutput.command_pool
    //                       , 2, image->cache->command_buffer);
    image->must_draw[0] = true;
    image->must_draw[1] = true;
    image->image_view = view;

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view;
    texture_descriptors.push_back (imageInfo);

    // std::vector<VkImageView> image_views {background.image_view};
    // for (auto&& image : images)
    // {
    //   image_views.push_back (image.image_view);
    // }
    image->cache = toplevel_window_command_buffer_cache
        {/*{create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[0]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image_views
           , image->x, image->y, image->width, image->height
           , image->cache->vertex_buffer_offset
           , image_pipeline
           , storage_zindex.get_buffer()
           , zindex_array.get_buffer()
           ) 
          , create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[1]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image_views
           , image->x, image->y, image->width, image->height
           , image->cache->vertex_buffer_offset
           , image_pipeline
           , storage_zindex.get_buffer()
           , zindex_array.get_buffer()
           )}
         , */image->cache->vertex_buffer_offset};
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
    
    vbuffer.replace(image->cache->vertex_buffer_offset / sizeof(vertex_info)
                    , vertex_info
                       {
                         {
                           // -1 because the positions are inclusive, and not past the end
                             fastdraw::coordinates::ratio(x, whole_width)
                           , fastdraw::coordinates::ratio(y, whole_height)
                           , 0.0f, 1.0f
                           , fastdraw::coordinates::ratio(x + width-1, whole_width)
                           , fastdraw::coordinates::ratio(y, whole_height)
                           , 0.0f, 1.0f
                           , fastdraw::coordinates::ratio(x + width-1, whole_width)
                           , fastdraw::coordinates::ratio(y + height-1, whole_height)
                           , 0.0f, 1.0f
                           , fastdraw::coordinates::ratio(x + width-1, whole_width)
                           , fastdraw::coordinates::ratio(y + height-1, whole_height)
                           , 0.0f, 1.0f
                           , fastdraw::coordinates::ratio(x, whole_width)
                           , fastdraw::coordinates::ratio(y + height-1, whole_height)
                           , 0.0f, 1.0f
                           , fastdraw::coordinates::ratio(x, whole_width)
                           , fastdraw::coordinates::ratio(y, whole_height)
                           , 0.0f, 1.0f
                         }
                         , {
                              0.0f, 0.0f
                            , 1.0f, 0.0f
                            , 1.0f, 1.0f
                            , 1.0f, 1.0f
                            , 0.0f, 1.0f
                            , 0.0f, 0.0f
                         }
                         , {
                              1.0f, 0.0f, 0.0f, 0.0f
                            , 0.0f, 1.0f, 0.0f, 0.0f
                            , 0.0f, 0.0f, 1.0f, 0.0f
                            , 0.0f, 0.0f, 0.0f, 1.0f
                         }
                         , image->zindex
                         , {{}, {}, {}} // padding
                       });
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
    return;
    //background = bg;
    //auto z = 1.0f;//16777215.0f;
    //auto z = 0.5f;
#if 0
    auto zindex = /*zindex_array.get_zindex_info (0);*/create_new_zindex();
    background.zindex = /*zinfo.*/zindex;
    std::cout << "background zindex " << background.zindex << std::endl;
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = background.image_view;
    texture_descriptors.push_back (imageInfo);
    
    unsigned int offset
      = vbuffer.push_back (vertex_info
                       {
                         {
                             -1.0f, -1.0f, 0.0f, 1.0f
                           ,  1.0f, -1.0f, 0.0f, 1.0f
                           ,  1.0f,  1.0f, 0.0f, 1.0f
                           ,  1.0f,  1.0f, 0.0f, 1.0f
                           , -1.0f,  1.0f, 0.0f, 1.0f
                           , -1.0f, -1.0f, 0.0f, 1.0f
                         }
                         , {
                              0.0f, 0.0f
                            , 1.0f, 0.0f
                            , 1.0f, 1.0f
                            , 1.0f, 1.0f
                            , 0.0f, 1.0f
                            , 0.0f, 0.0f
                         }
                         , {
                              1.0f, 0.0f, 0.0f, 0.0f
                            , 1.0f, 1.0f, 0.0f, 0.0f
                            , 0.0f, 0.0f, 1.0f, 0.0f
                            , 0.0f, 0.0f, 0.0f, 1.0f
                         }
                         , /*zinfo.*/zindex
                         , {{}, {}, {}} // padding
                       });
    std::cout << "background offset " << offset << std::endl;
#endif
  }

  // toplevel_window_image background;

  struct vertex_info
  {
    float vertices [24];
    float tex_coordinates [12];
    float transform_matrix [16];
    uint zindex;
    uint padding[3];
  };

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

  //VkDescriptorPool texture_descriptor_pool;
  // Backend* backend_;
  // should be a fastdraw something
  std::list<toplevel_window_image> images;
  mutable typename Backend::window window;
  fastdraw::output::vulkan::vulkan_draw_info image_pipeline;
  vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096> texture_descriptors;
  vulkan::descriptor_fixed_array<VK_DESCRIPTOR_TYPE_SAMPLER, 1> sampler_descriptors;
  std::vector<toplevel_framebuffer_region> framebuffers_damaged_regions[2];
  vertex_buffer <vertex_info> vbuffer;
  vulkan::buffer_allocator buffer_allocator;
  VkBuffer image_ssbo_buffer;
  VkBuffer image_zindex_ssbo_buffer;
  VkBuffer indirect_draw_buffer;
  //VkBuffer indirect_draw_shared_state_buffer;
  //storage_buffer <> ssbo_buffer;
  // struct storage_zindex<uint32_t> storage_zindex;
  // struct storage_zindex_array<uint32_t> zindex_array;
  //std::vector<toplevel_window_command_buffer_cache> buffer_cache;
  std::mutex image_mutex; // for images vector and command_buffers cache
  VkSampler const sampler = detail::render_thread_create_sampler (window.voutput.device);
  //VkRenderPass const renderPass = create_compatible_render_pass();

  std::size_t const static constexpr indirect_draw_info_array_size = 32u;
};
    
} }

#endif
