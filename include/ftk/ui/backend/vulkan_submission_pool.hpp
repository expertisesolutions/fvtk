///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_RECORD_POOL_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_RECORD_POOL_HPP

#include <mutex>
#include <future>
#include <condition_variable>

#include <ftk/ui/backend/vulkan_queues.hpp>

#include <portable_concurrency/future>

namespace ftk { namespace ui { namespace backend {

namespace pc = portable_concurrency;
      
struct vulkan_submission_pool
{
  VkDevice device;
  vulkan_queues* queues;

  struct queue
  {
    VkQueue queue;
    VkFence fence;
  };

  struct family_info
  {
    int family_index = -1;
    VkCommandPool pool;
    unsigned int max_queues;
    // std::vector<queue> reserved_queues;
    // std::unique_ptr<std::atomic_flag[]> reserved_queues_in_use;
    //////
    std::mutex submission_mutex;
    std::vector<VkCommandBuffer> submission_buffers;
    std::vector<pc::promise<void>> submission_promises;
    bool submission_winner = false;

    family_info () = default;
    family_info (family_info&& other)
      : family_index(other.family_index)
      , pool (other.pool)
      , max_queues (other.max_queues)
      , submission_mutex {}
      , submission_buffers (std::move(other.submission_buffers))
      , submission_winner (false)
      , submission_promises (std::move(other.submission_promises))
      // , reserved_queues (std::move(other.reserved_queues))
      // , reserved_queues_in_use (std::move(other.reserved_queues_in_use))
    {}
  };

  std::vector<family_info> families;

  struct thread_context
  {
    VkCommandBuffer command_buffer;
    unsigned int family_index;
  };

  std::vector<thread_context> thread_contexts;
  std::unique_ptr<std::atomic_flag[]> threads_in_use;
  std::atomic_uint32_t threads_in_use_total;
  int command_buffers_per_queue;

  // void add_reserved_queue ()
  // {
    
  // }

  vulkan_submission_pool (VkDevice device, struct vulkan_queues* queues
                          , unsigned int max_threads = 12
                          , unsigned int command_buffers_per_queue = 3)
    : device(device)//, family_index (family_index)
    , queues (queues)
    , threads_in_use_total (0u), command_buffers_per_queue(command_buffers_per_queue)
      //, thread_pool_count (threads)
  {
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;
    unsigned int max_queues = 0;

    auto family_initializer
      = [this, device, &max_queues] (std::vector<vulkan_queues::family>const& from)
      {
       for (auto&& family : from)
       {
        if (family.index >= families.size())
          families.resize(family.index + 1);
        if (families[family.index].family_index == -1)
        {
          families[family.index].family_index = family.index;
          VkCommandPoolCreateInfo poolInfo = {};
          poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
          poolInfo.queueFamilyIndex = family.index;
          poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT  | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
          auto r = from_result(vkCreateCommandPool(device, &poolInfo, nullptr, &families[family.index].pool));
          if (r != vulkan_error_code::success)
            throw std::system_error(make_error_code(r));
        }
        max_queues += family.queues.size();
        families[family.index].max_queues += family.queues.size();
       }
      };
    family_initializer (queues->global_graphic_families);
    family_initializer (queues->global_shared_families);

    auto threads = std::min (max_threads, max_queues * command_buffers_per_queue);

    thread_contexts.resize (threads);
    threads_in_use.reset (new std::atomic_flag[threads]);
    
    for (auto first = &threads_in_use[0]
           , last = &threads_in_use[0] + threads
           ; first != last; ++first)
      first->clear();

    unsigned int family_index = 0;
    unsigned int buffer_index = 0;
    unsigned int buffer_local_index = 0;
    for (auto&& context : thread_contexts)
    {
      if (buffer_local_index == families[family_index].max_queues*command_buffers_per_queue)
      {
        buffer_local_index = 0;
        family_index++;
      }
      
      VkCommandBufferAllocateInfo allocInfo = {};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandPool = families[family_index].pool;
      allocInfo.commandBufferCount = 1;

      auto r = from_result(vkAllocateCommandBuffers(device, &allocInfo, &thread_contexts[buffer_index].command_buffer));
      if (r != vulkan_error_code::success)
        throw -1;

      ++buffer_index;
      ++buffer_local_index;
    }
  }

  unsigned int allocate_thread_index ()
  {
    if (threads_in_use_total++ < thread_contexts.size())
    {
      for (unsigned int i = 0; i != thread_contexts.size(); ++i)
      {
        if (threads_in_use[i].test_and_set () == false)
          return i;
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

  template <typename T>
  struct is_future : std::false_type
  {};

  // template <typename T>
  // struct is_future<std::future<T>> : std::true_type {};

  // template <typename T>
  // struct is_future<std::shared_future<T>> : std::true_type {};

  template <typename T>
  struct is_future<pc::shared_future<T>> : std::true_type {};

  template <typename T>
  struct is_future<pc::future<T>> : std::true_type {};
  
  template <typename F>
  struct result_type_t
  {
    typedef typename std::result_of<F(VkCommandBuffer, unsigned int, pc::future<void>)>::type f_type;

    typedef typename
      std::conditional
    <is_future<f_type>::value
     , f_type
     , pc::future<f_type>>::type type;
  };

  template <typename F>
  typename result_type_t<F>::type run (F function)
  {
    auto id = allocate_thread_index ();
    return pc::async
      (pc::inplace_executor
       , [function, id, this]
       {
         using fastdraw::output::vulkan::from_result;
         using fastdraw::output::vulkan::vulkan_error_code;
         ::vkResetCommandBuffer (thread_contexts[id].command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
         // do we have to reset command_buffer ? Maybe reset after use, not before use

         auto family_id = thread_contexts[id].family_index;
         pc::promise<void> submission;
         auto r = function (thread_contexts[id].command_buffer, family_id, submission.get_future());

         auto winner = false;
         std::unique_lock<std::mutex> lock (families[family_id].submission_mutex);
         // CAS can push -- winner submits
         if (!families[family_id].submission_winner)
           winner = families[family_id].submission_winner = true;
         families[family_id].submission_buffers.push_back (thread_contexts[id].command_buffer);
         families[family_id].submission_promises.push_back (std::move(submission));
         lock.unlock();

         if (winner)
         {
           vulkan_queues::lock_graphic_queue queue (*this->queues, family_id);
           std::unique_lock<std::mutex> lock (families[family_id].submission_mutex);
           auto submission_buffers = std::move(families[family_id].submission_buffers);
           families[family_id].submission_buffers.clear();
           auto submission_promises = std::move(families[family_id].submission_promises);
           families[family_id].submission_promises.clear();
           // clear can push
           families[family_id].submission_winner = false;
           lock.unlock();

           VkFence fence;
           {
             VkFenceCreateInfo info = {};
             info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
             auto r = from_result(vkCreateFence(device, &info, nullptr, &fence));
             if (r != vulkan_error_code::success)
               throw std::system_error (make_error_code (r));
           }

           std::cout << "submitting " << submission_buffers.size() << " buffers" << std::endl;
           VkSubmitInfo submitInfo = {};
           submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
           submitInfo.commandBufferCount = submission_buffers.size();
           submitInfo.pCommandBuffers = &submission_buffers[0];

           auto vkqueue = queue.get_queue();
           auto r = from_result( ::vkQueueSubmit(vkqueue.vkqueue, 1, &submitInfo, fence));
           if (r != vulkan_error_code::success)
             throw std::system_error (make_error_code(r));
           
           r = from_result( ::vkWaitForFences (device, 1, &fence, VK_TRUE, -1));
           if (r != vulkan_error_code::success)
             throw std::system_error (make_error_code (r));

           for (auto&& promise : submission_promises)
             promise.set_value();
           threads_in_use[id].clear();
           threads_in_use_total--;
         }

         return r;
       });
  }
};

} } }

#endif

