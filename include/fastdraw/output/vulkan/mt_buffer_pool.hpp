///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_FASTDRAW_OUTPUT_VULKAN_MT_BUFFER_POOL_HPP
#define FASTDRAW_FASTDRAW_OUTPUT_VULKAN_MT_BUFFER_POOL_HPP

#include <mutex>

#include <fastdraw/output/vulkan/error.hpp>

namespace fastdraw { namespace output { namespace vulkan {

struct mt_buffer_pool
{
  VkCommandPool pool;
  std::mutex mutex;

  mt_buffer_pool (VkCommandPool pool) : pool(pool) {}
  mt_buffer_pool (mt_buffer_pool&& other)
    : pool(other.pool), mutex{}
  {}

  void allocate_buffers (VkDevice device, VkCommandBufferAllocateInfo info, VkCommandBuffer* buffers)
  {
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;
    std::unique_lock<std::mutex> l(mutex);
    auto r = fastdraw::output::vulkan::from_result(vkAllocateCommandBuffers (device, &info, buffers));
    l.unlock();
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code(r));
  }

  VkCommandBuffer allocate_buffers (VkDevice device, VkCommandBufferAllocateInfo info)
  {
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;
    info.commandBufferCount = 1;
    std::unique_lock<std::mutex> l(mutex);
    VkCommandBuffer buffer;

    auto r = fastdraw::output::vulkan::from_result(vkAllocateCommandBuffers (device, &info, &buffer));
    l.unlock();
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code(r));
    return buffer;
  }
};
      
} } }

#endif

