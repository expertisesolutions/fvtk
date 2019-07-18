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

#include <variant>

namespace ftk { namespace ui { namespace backend {

struct queue
{
  VkQueue vkqueue;
  unsigned int family;
};

struct family_ref
{
  unsigned int index;
};

inline
std::array<std::vector<family_ref>, 3>
vulkan_queues_separate_queue_families (VkDevice device, VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  std::vector<family_ref> graphic_families, presentation_families, shared_families;

  unsigned int family_index = 0;
  for (const auto& queue_family : queue_families)
  {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, family_index, surface, &presentSupport);
    if (queue_family.queueCount > 0 && presentSupport)
    {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        shared_families.push_back({family_index});
      else
        presentation_families.push_back({family_index});
    }
    else if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      graphic_families.push_back({family_index});

    ++family_index;
  }

  return {graphic_families, presentation_families, shared_families};
}
      
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
          vkGetDeviceQueue (device, family_index, i, &queues[index].vkqueue);
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
  struct swapchain
  {
    std::vector<std::variant<family, family_ref>>  graphic_families
      , presentation_families, shared_families;
  };
  std::vector<family> global_graphic_families, global_presentation_families
    , global_shared_families;
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
              families.push_back ({q.family, {q.vkqueue}});
              mapping.insert ({q.family, i});
            }
            else
              families[it->second].queues.push_back(q.vkqueue);
          }
          for (auto && f : families)
          {
            f.in_use.reset(new std::atomic_flag[f.queues.size()]);
            for (auto p = &f.in_use[0], last = p + f.queues.size()
                   ;p != last; ++p)
              p->clear();
          }
          std::sort (families.begin(), families.end(), [] (auto&& lhs, auto&& rhs)
                                                       {
                                                         return lhs.index < rhs.index;
                                                       });
          return families;
        };

    global_graphic_families = fill (graphic_queues);
    global_presentation_families = fill (presentation_queues);
    global_shared_families = fill (shared_queues);
  }

  template <typename Derived>
  struct lock_queue_base
  {
    family* family_ptr;
    unsigned int queue_index;

    Derived& get_derived() { return static_cast<Derived&>(*this); }
    Derived const& get_derived() const { return static_cast<Derived const&>(*this); }

    bool search_queue (vulkan_queues& queues)
    {
      for (queue_index = 0; queue_index != family_ptr->queues.size(); ++queue_index)
        if (family_ptr->in_use[queue_index].test_and_set () == false /* old value */)
          return true;
      return false;
    }

    bool search_family (vulkan_queues& queues, std::vector<family>::iterator first
                        , std::vector<family>::iterator last)
    {
      for (;first != last; ++first)
      {
        family_ptr = &*first;
        if (search_queue (queues))
          return true;
      }
      return false;
    }

    bool search_specific_family (vulkan_queues& queues, unsigned int family)
    {
      auto last = get_derived().get_main_families(queues).end();
      auto iterator = std::find_if (get_derived().get_main_families(queues).begin(), last
                                    , [family] (auto&& f) { return f.index == family; });
      if (iterator == last)
      {
        last = queues.global_shared_families.end();
        iterator = std::find_if (queues.global_shared_families.begin(), last
                                 , [family] (auto&& f) { return f.index == family; });
        if (iterator == last)
          return false;
      }
      family_ptr = &*iterator;

      if (!search_queue (queues))
        return false;
      return true;
    }

    lock_queue_base()
    {
    }
    
    lock_queue_base(vulkan_queues& queues)
      : queue_index(0)
    {
      if (!search_family (queues, get_derived().get_main_families(queues).begin()
                          , get_derived().get_main_families(queues).end()))
      {
        if (!search_family (queues, queues.global_shared_families.begin()
                            , queues.global_shared_families.end()))
          throw -1;
      }
    }

    lock_queue_base(vulkan_queues& queues, unsigned int family)
      : queue_index (0)
    {
      search_specific_family (queues, family);
    }

    queue get_queue () const
    {
      return {family_ptr->queues[queue_index], family_ptr->index};
    }

    ~lock_queue_base()
    {
      family_ptr->in_use[queue_index].clear();
    }
  };

  struct lock_graphic_queue : lock_queue_base<lock_graphic_queue>
  {
    std::vector<family>& get_main_families (vulkan_queues& queues) const { return queues.global_graphic_families; }

    typedef lock_queue_base<lock_graphic_queue> base_t;
    using base_t::base_t;
  };

  struct lock_presentation_queue : lock_queue_base<lock_presentation_queue>
  {
    std::vector<family>& get_main_families (vulkan_queues& queues) const { return queues.global_presentation_families; }
    typedef lock_queue_base<lock_presentation_queue> base_t;
    using base_t::base_t;
  };

  template <typename Derived>
  struct lock_swapchain_queue : lock_queue_base<Derived>
  {
    typedef lock_queue_base<Derived> base_t;

    Derived& get_derived() { return static_cast<Derived&>(*this); }
    Derived const& get_derived() const { return static_cast<Derived&>(*this); }

    lock_swapchain_queue (vulkan_queues& queues, unsigned int swapchain_index)
      : base_t {}
    {
      swapchain& sw = swapchains[swapchain_index];
      auto&& sw_families = get_derived().get_swapchain_families(sw);
      bool found = false;

      for (auto&& sw_family : sw_families)
      {
        if (family_ref* ref = std::get_if<family_ref>(&sw_family))
        {
          if (found = this->search_specific_family (queues, ref->index))
            break;
        }
        else if (family* family = std::get_if<family>(&sw_family))
        {
          
        }
      }
    }
  };

  struct lock_graphic_swapchain_queue : lock_swapchain_queue<lock_graphic_swapchain_queue>
  {
    std::vector<family>& get_main_families (vulkan_queues& queues) const { return queues.global_graphic_families; }
    std::vector<std::variant<family, family_ref>>& get_swapchain_families (swapchain& sw) const { return sw.graphic_families; }

    using base_t = lock_swapchain_queue<lock_graphic_swapchain_queue>;
    using base_t::base_t;
  };
  
};
      
} } }

#endif
