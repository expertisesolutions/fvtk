///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_QUEUES_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_QUEUES_HPP

namespace ftk { namespace ui { namespace backend {

struct queue
{
  VkQueue queue_;
  unsigned int family;
};

inline
std::array<std::vector<queue>, 3>
vulkan_queues_create_queues (VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  std::vector<queue> graphic_queues, presentation_queues, shared_queues;

  unsigned int family_index = 0;

  auto add_to_queue
    = [&] (std::vector<queue>& queues, VkQueueFamilyProperties properties)
      {
        unsigned int index = queues.size();
        queues.resize (index + properties.queueCount);
        for (unsigned int i = 0;index != queues.size(); ++index, ++i)
        {
          vkGetDeviceQueue (device, family_index, i, &queues[index].queue_);
          queues[index].family = family_index;
        }
      };
  
  for (const auto& queue_family : queue_families)
  {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family_index, surface, &presentSupport);
    if (queue_family.queueCount > 0 && presentSupport)
    {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        add_to_queue (shared_queues, queue_family);
      else
        add_to_queue (presentation_queues, queue_family);
    }
    else if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        add_to_queue (graphic_queues, queue_family);

    ++family_index;
  }

  return {graphic_queues, presentation_queues, shared_queues};
}

std::pair<std::vector<VkDeviceQueueCreateInfo>
          , std::unique_ptr<float[]>> vulkan_queues_create_queue_create_info (VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  unsigned int max_queues = 0;
  std::vector<VkDeviceQueueCreateInfo> r;

  unsigned int i = 0;
  for (const auto& queue_family : queue_families)
  {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentSupport);
    if (queue_family.queueCount > 0 && presentSupport)
    {
      max_queues = std::max(max_queues, queue_family.queueCount);

      r.push_back({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO});
      r.back().queueFamilyIndex = i;
      r.back().queueCount = queue_family.queueCount;
    }
    else if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      max_queues = std::max(max_queues, queue_family.queueCount);

      r.push_back({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO});
      r.back().queueFamilyIndex = i;
      r.back().queueCount = queue_family.queueCount;
    }
    ++i;
  }

  assert (max_queues > 0);
  
  std::unique_ptr<float[]> priorities {new float[max_queues]};
  for (auto it = &priorities[0], last = &priorities[0] + max_queues
         ; it != last; ++it)
    *it = 1.0f;

  for (auto&& info : r)
    info.pQueuePriorities = &priorities[0];
  
  return {r, std::move(priorities)};
}

struct vulkan_queues
{
  struct family
  {
    unsigned int index;

    std::vector<VkQueue> queues;
    std::unique_ptr<std::atomic_flag[]> in_use;
  };
  std::vector<family> global_graphic_families, global_presentation_families
    , global_shared_families;
  struct family_ref
  {
    unsigned int index;
  };
  struct swapchain
  {
    std::vector<std::variant<family, family_ref>>  graphic_families
      , presentation_families;
  };
  std::vector<swapchain> swapchains;
  
  vulkan_queues (vulkan_queues const&) = delete;
  vulkan_queues (vulkan_queues&&) = default;

  vulkan_queues& operator= (vulkan_queues const&) = delete;
  vulkan_queues& operator= (vulkan_queues&&) = default;
  
  vulkan_queues () {}

  void push_back_swapchain (swapchain s)
  {
    swapchains.push_back(std::move(s));
  }
  
  vulkan_queues (std::vector<queue> graphic_queues
                 , std::vector<queue> presentation_queues
                 , std::vector<queue> shared_queues)
  {

    auto fill
      = [] (std::vector<queue> queues)
        {
          std::map<unsigned int, unsigned int> mapping;
          std::vector<family> families;
          for (auto&& q : queues)
          {
            auto it = mapping.find (q.family);
            if (it == mapping.end ())
            {
              auto i = families.size();
              families.push_back ({q.family, {q.queue_}});
              auto pair = mapping.insert ({q.family, i});
            }
            else
              families[it->second].queues.push_back(q.queue_);
          }
          for (auto && f : families)
          {
            f.in_use.reset(new std::atomic_flag[f.queues.size()]);
            for (auto p = &f.in_use[0], last = p + f.queues.size()
                   ;p != last; ++p)
              p->clear();
          }
          return families;
        };

    global_graphic_families = fill (graphic_queues);
    global_presentation_families = fill (presentation_queues);
    global_shared_families = fill (shared_queues);
  }

  struct lock_graphic_queue
  {
    vulkan_queues* queues;
    unsigned int i, j;
    
    lock_graphic_queue(vulkan_queues& queues)
      : queues (&queues), i (0)
    {
      for (; i != queues.global_graphic_families.size(); ++i)
      {
        for (j = 0; j != queues.global_graphic_families[i].queues.size(); ++j)
          if (queues.global_graphic_families[i].in_use[j].test_and_set () == false /* old value */)
            break;
      }

      if (i == queues.global_graphic_families.size())
        throw -1;
    }

    queue get_queue () const
    {
      return {queues->global_graphic_families[i].queues[j], queues->global_graphic_families[i].index};
      }

    ~lock_graphic_queue()
    {
      //queues->graphic[i].clear();
    }
  };

  struct lock_presentation_queue
  {
  };
  
};
      
} } }

#endif
