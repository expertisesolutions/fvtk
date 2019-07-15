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
  std::vector<queue> queues;
  //std::vector<queue> presentation_queues;

  std::unique_ptr<std::atomic_flag[]> graphics_in_use;
  std::unique_ptr<std::atomic_flag[]> presentations_in_use;

  vulkan_queues (vulkan_queues const&) = delete;
  vulkan_queues (vulkan_queues&&) = default;

  vulkan_queues& operator= (vulkan_queues const&) = delete;
  vulkan_queues& operator= (vulkan_queues&&) = default;
  
  vulkan_queues () {}
  
  vulkan_queues (std::vector<std::vector<queue>> swapchains_queues
                 , std::vector<queue> graphic_queues
                 , std::vector<queue> presentation_queues
                 , std::vector<queue> shared_queues)
    : graphic_queues (graphic_queues), presentation_queues (presentation_queues)
  {
    graphic_queues.insert(graphic_queues.end(), shared_queues.begin(), shared_queues.end());
    presentation_queues.insert(presentation_queues.end(), shared_queues.begin(), shared_queues.end());

    graphics_in_use.reset (new std::atomic_flag[graphic_queues.size()]);
    presentations_in_use.reset (new std::atomic_flag[presentation_queues.size()]);

    for (auto g = &graphics_in_use[0], last = g + graphic_queues.size(); g != last; ++g)
      g->clear();
    for (auto p = &presentations_in_use[0], last = p + presentation_queues.size(); p != last; ++p)
      p->clear();
  }

  struct lock_graphic_queue
  {
    vulkan_queues* queues;
    unsigned int i;
    
    lock_graphic_queue(vulkan_queues& queues)
      : queues (&queues), i (0)
    {
      for (; i != queues.graphic_queues.size(); ++i)
      {
        if (queues.graphics_in_use[i].test_and_set () == false /* old value */)
          break;
      }

      if (i == queues.graphic_queues.size())
        throw -1;
    }

    queue get_queue () const { return queues->graphic_queues[i]; }

    ~lock_graphic_queue()
    {
      queues->graphics_in_use[i].clear();
    }
  };

  struct lock_presentation_queue
  {
    vulkan_queues* queues;
    unsigned int i;
    
    lock_presentation_queue(vulkan_queues& queues)
      : queues (&queues), i (0)
    {
      for (; i != queues.presentation_queues.size(); ++i)
      {
        if (queues.presentations_in_use[i].test_and_set () == false /* old value */)
          break;
      }

      if (i == queues.presentation_queues.size())
        throw -1;
    }

    queue get_queue () const { return queues->presentation_queues[i]; }

    ~lock_presentation_queue()
    {
      queues->presentations_in_use[i].clear();
    }
  };
  
};
      
} } }

#endif
