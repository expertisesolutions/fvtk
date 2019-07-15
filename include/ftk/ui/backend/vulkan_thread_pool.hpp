///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_POOL_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_POOL_HPP

#include <mutex>
#include <future>
#include <condition_variable>

namespace ftk { namespace ui { namespace backend {

struct vulkan_thread_pool
{
  VkDevice device;
  int family_index;
  VkCommandPool command_pool;

  struct thread_group_context
  {
    VkQueue queue;
    std::unique_ptr<std::mutex> mutex {new std::mutex};
    bool locked;
    std::unique_ptr<std::condition_variable> condvar {new std::condition_variable};
  };
  
  std::unique_ptr<thread_group_context[]> thread_groups_context;
  std::unique_ptr<VkCommandBuffer[]> command_buffers;
  //std::vector<thread_context> threads_context;
  std::unique_ptr<std::atomic_flag[]> threads_in_use;
  std::atomic_uint32_t threads_in_use_total;
  int command_buffers_per_queue;
  unsigned int thread_pool_count;
  //unsigned int max_threads;

  vulkan_thread_pool (VkDevice device, int family_index
                      , int queue_index_first, int queue_index_last
                      , unsigned int threads
                      , unsigned int command_buffers_per_queue = 3)
    : device(device), family_index (family_index)
    , threads_in_use_total (0u), command_buffers_per_queue(command_buffers_per_queue)
    , thread_pool_count (threads)
  {
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = family_index;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT  | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
    }

    const int queue_count = queue_index_last - queue_index_first;
    //thread_pool_count = std::min((queue_count) * command_buffers_per_queue, max_threads);

    if (thread_pool_count == 0)
      throw -1;

    thread_groups_context.reset (new thread_group_context[queue_count]);
    command_buffers.reset (new VkCommandBuffer[thread_pool_count]);
    threads_in_use.reset (new std::atomic_flag[thread_pool_count]);
    for (auto first = &threads_in_use[0]
           , last = &threads_in_use[0] + thread_pool_count
           ; first != last; ++first)
      first->clear();

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool;
    allocInfo.commandBufferCount = (queue_index_last - queue_index_first) * command_buffers_per_queue;

    auto r = from_result(vkAllocateCommandBuffers(device, &allocInfo, static_cast<VkCommandBuffer*>(static_cast<void*>(&command_buffers[0]))));
    if (r != vulkan_error_code::success)
      throw -1;

  }

  struct queue_lockable
  {
    thread_group_context* group;

    struct lock
    {
      thread_group_context* group;
      std::unique_lock<std::mutex> lock_;

      lock (queue_lockable* queue)
        : group (queue->group), lock_ (*queue->group->mutex)
      {
        group->locked = true;
      }

      ~lock()
      {
        group->locked = false;
        group->condvar->notify_one();
      }

      VkQueue get_queue () const
      {
        return group->queue;
      }
    };
  };

  std::pair<thread_group_context*, VkCommandBuffer> allocate_thread_index ()
  {
    if (threads_in_use_total++ < thread_pool_count)
    {
      for (std::size_t i = 0; i != thread_pool_count; ++i)
      {
        if (threads_in_use[i].test_and_set () == false)
        {
          thread_group_context* context = &thread_groups_context[i / command_buffers_per_queue];
          return {context, command_buffers[i]};
        }
      }

      // can't happen
      std::abort();
    }
    else
    {
      // block?
      throw -1;
    }
  }

  template <typename F>
  std::future<typename std::result_of<F(VkCommandBuffer, queue_lockable)>::type> run (F function)
  {
    auto context = allocate_thread_index ();
    if (!context.first)
    {
      // should block or queue
      throw -1;
    }
    else
    {
      ::vkResetCommandBuffer (context.second, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
      // do we have to reset command_buffer ? Maybe reset after use, not before use
      auto result = std::async([function, context] {return function (context.second, queue_lockable{context.first});});

      //VkResetCommandBuffer (
      
      // not in use anymore
      return result;
    }
  }
};

} } }

#endif

