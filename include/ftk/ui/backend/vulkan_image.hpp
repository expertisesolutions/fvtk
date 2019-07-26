///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_BACKEND_VULKAN_IMAGE_HPP
#define FTK_FTK_BACKEND_VULKAN_IMAGE_HPP

#include <ftk/ui/backend/vulkan_submission_pool.hpp>

#include <future>

namespace ftk { namespace ui { namespace backend {

struct vulkan_image
{
  VkImage image;
  VkImageView image_view;
};

template <typename Executor>
struct vulkan_image_loader
{
  VkDevice device;
  VkPhysicalDevice physical_device;

  ftk::ui::backend::vulkan_submission_pool<Executor>* graphic_thread_pool;

  typedef vulkan_image output_image_type;

  template <typename I>
  pc::future<output_image_type> load (std::filesystem::path path, I image_loader) const;
  pc::future<output_image_type> load (const void* buffer, int32_t width, int32_t height
                                      , uint32_t stride) const;
  pc::future<output_image_type> load (VkBuffer, int32_t width, int32_t height) const;
};
      
} } }
      
#endif

