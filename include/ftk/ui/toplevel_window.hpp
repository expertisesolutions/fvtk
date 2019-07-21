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
#include <ftk/ui/backend/vulkan_command_buffer_cache.hpp>

#include <functional>

#include <iostream>

#include <mutex>
#include <list>

namespace ftk { namespace ui {

namespace detail {

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
  VkCommandBuffer command_buffer[2]; // for each framebuffer
  unsigned int vertex_buffer_offset;
};
    
struct toplevel_window_image
{
  VkImageView image_view;
  std::int32_t x, y, width, height;

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
    , vbuffer (window.voutput.device, window.voutput.physical_device)
  {
    //backend_->exposure_signal.connect (std::bind(&toplevel_window<Backend>::exposure, this));
    
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
    auto whole_width = window.voutput.swapChainExtent.width - 1;
    auto whole_height = window.voutput.swapChainExtent.height - 1;
    auto it = images.insert(images.begin(), image);
    auto x = it->x;
    auto y = it->y;
    auto width = it->width;
    auto height = it->height;

    vertex_info vinfo =
      {
       {
        fastdraw::coordinates::ratio(x, whole_width)
        , fastdraw::coordinates::ratio(y, whole_height)
        , fastdraw::coordinates::ratio(x + width, whole_width)
        , fastdraw::coordinates::ratio(y, whole_height)
        , fastdraw::coordinates::ratio(x + width, whole_width)
        , fastdraw::coordinates::ratio(y + height, whole_height)
        , fastdraw::coordinates::ratio(x + width, whole_width)
        , fastdraw::coordinates::ratio(y + height, whole_height)
        , fastdraw::coordinates::ratio(x, whole_width)
        , fastdraw::coordinates::ratio(y + height, whole_height)
        , fastdraw::coordinates::ratio(x, whole_width)
        , fastdraw::coordinates::ratio(y, whole_height)
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
      };

    auto invalidated_vbuffer = vbuffer.size() == vbuffer.capacity();
    auto buffer_cache_offset =  vbuffer.push_back (vinfo);
    if (invalidated_vbuffer)
    {
      auto buffer_cache_offset =  vbuffer.push_back (vinfo);

      for (auto&& image : images)
      {
        if (image.cache)
        {
          vkFreeCommandBuffers (window.voutput.device, window.voutput.command_pool
                                , 2, image.cache->command_buffer);
          image.cache =
            toplevel_window_command_buffer_cache
            {{create_command_buffer
              (window.voutput.command_pool
               , window.voutput.device
               , window.swapChainFramebuffers[0]
               , window.voutput.swapChainExtent
               , renderPass
               , vbuffer.get_buffer()
               , sampler
               , image.image_view
               , image.x, image.y, image.width, image.height
               , image.cache->vertex_buffer_offset
               , image_pipeline
               )
              , create_command_buffer
              (window.voutput.command_pool
               , window.voutput.device
               , window.swapChainFramebuffers[1]
               , window.voutput.swapChainExtent
               , renderPass
               , vbuffer.get_buffer()
               , sampler
               , image.image_view
               , image.x, image.y, image.width, image.height
               , image.cache->vertex_buffer_offset
               , image_pipeline
               )}
             , image.cache->vertex_buffer_offset};
        }
      }
    }

    it->cache = toplevel_window_command_buffer_cache
        {{create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[0]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image.image_view
           , x, y, width, height
           , buffer_cache_offset
           , image_pipeline
           )
          , create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[1]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image.image_view
           , x, y, width, height
           , buffer_cache_offset
           , image_pipeline
           )}
         , buffer_cache_offset};
    return it;
  }
  void replace_image_view (image_iterator image, VkImageView view)
  {
    vkFreeCommandBuffers (window.voutput.device, window.voutput.command_pool
                          , 2, image->cache->command_buffer);
    image->must_draw[0] = true;
    image->must_draw[1] = true;
    image->image_view = view;
    image->cache = toplevel_window_command_buffer_cache
        {{create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[0]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image->image_view
           , image->x, image->y, image->width, image->height
           , image->cache->vertex_buffer_offset
           , image_pipeline
           )
          , create_command_buffer
          (window.voutput.command_pool
           , window.voutput.device
           , window.swapChainFramebuffers[1]
           , window.voutput.swapChainExtent
           , renderPass
           , vbuffer.get_buffer()
           , sampler
           , image->image_view
           , image->x, image->y, image->width, image->height
           , image->cache->vertex_buffer_offset
           , image_pipeline
           )}
         , image->cache->vertex_buffer_offset};
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
    auto whole_width = window.voutput.swapChainExtent.width - 1;
    auto whole_height = window.voutput.swapChainExtent.height - 1;
    auto width = image->width;
    auto height = image->height;

    vbuffer.replace(image->cache->vertex_buffer_offset / sizeof(vertex_info)
                    , vertex_info
                       {
                         {
                             fastdraw::coordinates::ratio(x, whole_width)
                           , fastdraw::coordinates::ratio(y, whole_height)
                           , fastdraw::coordinates::ratio(x + width, whole_width)
                           , fastdraw::coordinates::ratio(y, whole_height)
                           , fastdraw::coordinates::ratio(x + width, whole_width)
                           , fastdraw::coordinates::ratio(y + height, whole_height)
                           , fastdraw::coordinates::ratio(x + width, whole_width)
                           , fastdraw::coordinates::ratio(y + height, whole_height)
                           , fastdraw::coordinates::ratio(x, whole_width)
                           , fastdraw::coordinates::ratio(y + height, whole_height)
                           , fastdraw::coordinates::ratio(x, whole_width)
                           , fastdraw::coordinates::ratio(y, whole_height)
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
                       });
  }

  VkRenderPass create_compatible_render_pass ();
  
  // should be a fastdraw something
  std::list<toplevel_window_image> images;

  void load_background (toplevel_window_image bg)
  {
    background = bg;
    
    unsigned int offset
      = vbuffer.push_back (vertex_info
                       {
                         {
                             -1.0f, -1.0f
                           ,  1.0f, -1.0f
                           ,  1.0f,  1.0f
                           ,  1.0f,  1.0f
                           , -1.0f, 1.0f
                           , -1.0f, -1.0f
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
                       });
    std::cout << "background offset " << offset << std::endl;
  }

  toplevel_window_image background;

  struct vertex_info
  {
    float vertices [12];
    float tex_coordinates [12];
    float transform_matrix [16];
  };
  
  // Backend* backend_;
  mutable typename Backend::window window;
  std::vector<toplevel_framebuffer_region> framebuffers_damaged_regions[2];
  vertex_buffer <vertex_info> vbuffer;
  //std::vector<toplevel_window_command_buffer_cache> buffer_cache;
  std::mutex image_mutex; // for images vector and command_buffers cache
  fastdraw::output::vulkan::vulkan_draw_info const image_pipeline
    = fastdraw::output::vulkan::create_image_pipeline (window.voutput);
  VkSampler const sampler = detail::render_thread_create_sampler (window.voutput.device);
  VkRenderPass const renderPass = create_compatible_render_pass();
};
    
} }

#endif
