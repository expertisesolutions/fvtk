///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_STORAGE_VECTOR_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_STORAGE_VECTOR_HPP

namespace ftk { namespace ui { namespace backend { namespace vulkan {
      
template <typename T, std::size_t MinimumSizePerBuffer = 100 /* to avoid relocations */>
struct storage_vector
{
  typedef storage_vector<T, MinimumSizePerBuffer> self_type;
  
  storage_vector (VkDevice device, VkPhysicalDevice physical_device)
    : map_pointer (nullptr), size_ (0u)
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
      unmap();
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

    allocated_ = mem_requirements.size;
    map(allocated_);
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
    return allocated_ / sizeof(T);
  }
  
  void reserve (unsigned int size)
  {
    if (allocated_ < size * sizeof(T))
      grow (size * sizeof(T));
  }

  void push_back (T value)
  {
    if (capacity () == size())
    {
      grow ((allocated_ + sizeof(T)) * 2);
    }
    *(begin() + size_++) = std::move(value);
  }

  void create_buffer (unsigned int size)
  {
    using fastdraw::output::vulkan::vulkan_error_code;
    using fastdraw::output::vulkan::from_result;

    std::cout << "create_buffer " << size << std::endl;
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto r = from_result(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));
  }

  typedef T* iterator;
  typedef T const* const_iterator;

  iterator begin () { return static_cast<T*>(map_pointer); }
  const_iterator begin () const { return static_cast<T const*>(map_pointer); }
  iterator end () { return static_cast<T*>(map_pointer) + size(); }
  const_iterator end () const { return static_cast<T const*>(map_pointer) + size(); }
  
  VkBuffer get_buffer ()
  {
    return buffer;
  }

  unsigned int size() const { return size_; }

  void* map_pointer;
  unsigned int size_;
  unsigned int allocated_;
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkBuffer buffer;
  VkDeviceMemory memory;
};
    
} } } }

#endif
