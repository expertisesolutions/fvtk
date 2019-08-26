///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_VERTEX_BUFFER_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_VERTEX_BUFFER_HPP

#include <fastdraw/output/vulkan/error.hpp>

#include <vulkan/vulkan.h>

#include <vector>

namespace ftk { namespace ui {

template <typename...T>
struct vertex_buffer
{
  constexpr static const unsigned int elements_size = (sizeof (T) + ...);
  
  vertex_buffer (VkDevice device, VkPhysicalDevice physical_device)
    : map_pointer (nullptr)
    , allocated_ (0u), device (device), physical_device (physical_device)
  {
  }

  void grow (unsigned int to)
  {
    using fastdraw::output::vulkan::vulkan_error_code;
    using fastdraw::output::vulkan::from_result;

    std::cout << "grow to at least to " << to << std::endl;
    if (allocated_ != 0)
    {
      vkDestroyBuffer (device, buffer, nullptr);
      vkFreeMemory (device, memory, nullptr);
    }

    create_buffer (to);

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = mem_requirements.size;

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
      if ((mem_requirements.memoryTypeBits & (1 << i))
          && ((mem_properties.memoryTypes[i].propertyFlags
               & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
               ==  (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
      {
        info.memoryTypeIndex = i;
        break;
      }
    }
    
    auto r = from_result(vkAllocateMemory(device, &info, nullptr, &memory));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));

    std::cout << "create_memory " << mem_requirements.size << std::endl;
    
    vkBindBufferMemory(device, buffer, memory, 0);

    map(to);
    if (!cpu_memory.empty())
      std::memcpy (map_pointer, &cpu_memory[0], cpu_memory.size() * elements_size);
    allocated_ = to;
  }

  void map(unsigned int size)
  {
    assert (map_pointer == nullptr);
    vkMapMemory (device, memory, 0, size, 0, &map_pointer);
  }
  
  void unmap ()
  {
    vkUnmapMemory (device, memory);
    map_pointer = nullptr;
  }

  uint32_t capacity () const
  {
    return allocated_ / elements_size;
  }
  
  // void reserve (unsigned int size)
  // {
  //   if (row_size < size)
  //   {
  //     grow (size);
  //     unmap();
  //   }
  // }

  void replace (unsigned int offset, T... ts)
  {
    std::cout << "replacing at offset " << offset << std::endl;
    map (allocated_);
    std::tuple <T...> t {ts...};
    std::memcpy (static_cast<char*>(map_pointer) + offset * elements_size, &t, elements_size);
    cpu_memory[offset] = t;
    unmap ();
  }

  unsigned int push_back (T... ts) // returns offset
  {
    std::cout << "elements_size " << elements_size << std::endl;
    auto offset = cpu_memory.size() * elements_size;
    std::cout << "offset " << offset << " allocated_ " << allocated_ << std::endl;
    if (allocated_ == cpu_memory.size()*elements_size)
      grow((allocated_ + elements_size) << 1);
    else
      map (allocated_);
    std::tuple <T...> t {ts...};
    std::memcpy (static_cast<char*>(map_pointer) + offset, &t, elements_size);
    cpu_memory.push_back (t);
    std::cout << "new size " << cpu_memory.size() << std::endl;
    unmap ();
    return offset;
  }

  void create_buffer (unsigned int size)
  {
    using fastdraw::output::vulkan::vulkan_error_code;
    using fastdraw::output::vulkan::from_result;

    std::cout << "create_buffer " << size << std::endl;
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto r = from_result(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));
  }
  
  VkBuffer get_buffer ()
  {
    assert (!cpu_memory.empty());
    return buffer;
  }

  unsigned int size() const { return cpu_memory.size(); }

  std::vector<std::tuple<T...>> cpu_memory;
  void* map_pointer;
  unsigned int allocated_;
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkBuffer buffer;
  VkDeviceMemory memory;
};
    
} }

#endif
