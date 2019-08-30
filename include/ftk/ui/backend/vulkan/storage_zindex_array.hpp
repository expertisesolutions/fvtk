///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_STORAGE_ZINDEX_ARRAY_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_STORAGE_ZINDEX_ARRAY_HPP

namespace ftk { namespace ui { namespace backend { namespace vulkan { namespace detail {

template <typename Z, typename Enable = void>
struct zindex_traits;

}
      
template <typename Z, std::size_t MinimumSizePerBuffer = 100 /* to avoid relocations */>
struct storage_zindex_array
{
  typedef storage_zindex_array<Z, MinimumSizePerBuffer> self_type;
  
  constexpr static const unsigned int elements_size = sizeof(Z);

  typedef detail::zindex_traits<Z> traits;

  struct zindex_info
  {
    Z zindex;
    unsigned int index;
    unsigned int buffer_offset;
  };
  
  storage_zindex_array (VkDevice device, VkPhysicalDevice physical_device)
    : array_size_per_element (MinimumSizePerBuffer)
    , map_pointer (nullptr)
    , allocated_ (0u), device (device), physical_device (physical_device)
  {
    zindexes.push_back ({traits::zero(), true});
    reserve (std::max(zindexes.size(), MinimumSizePerBuffer));
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

    map(to);
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
  
  void reserve (unsigned int size)
  {
    if (allocated_ < size * array_size_per_element)
      grow (size * array_size_per_element);
  }

  // void replace (unsigned int offset, Z)
  // {
  //   std::cout << "replacing at offset " << offset << std::endl;
  //   map (allocated_);
  //   std::tuple <T...> t {ts...};
  //   std::memcpy (map_pointer + offset * elements_size, &t, elements_size);
  //   cpu_memory[offset] = t;
  //   unmap ();
  // }

  zindex_info create_new_zindex () // returns offset
  {
    assert (!zindexes.empty());
    auto index = zindexes.size();
    zindexes.push_back ({traits::next(zindexes[index - 1].first), true});
    std::cout << "creating new zindex next from " << zindexes[index - 1].first
              << " of value " << zindexes[index].first << std::endl;

    reserve (zindexes.size());
    
    return {zindexes.back().first, index, index * array_size_per_element};
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
  
  VkBuffer get_buffer ()
  {
    return buffer;
  }

  struct value_type
  {
    typedef Z const* iterator;
    typedef Z const* const_iterator;
    const_iterator begin() const
    {
      return first;
    }
    const_iterator end() const
    {
      return last;
    }

    const_iterator first, last;
  };

  struct iterator
  {
    std::vector<Z>::const_iterator z_first, z_last;
    void* map_pointer;
    std::size_t array_size_per_element;

    typedef self_type::value_type value_type;
    typedef std::ptrdiff_t difference_type;
    typedef value_type* pointer;
    typedef value_type reference;
    typedef std::random_access_iterator_tag iterator_category;
  };

  unsigned int size() const { return zindexes.size(); }

  zindex_info get_zindex_info (std::size_t index) const
  {
    return {zindexes[index].first, index, index * array_size_per_element};
  }

  std::size_t array_size_per_element;
  std::vector<std::pair<Z, bool>> zindexes;
  void* map_pointer;
  unsigned int allocated_;
  VkDevice device;
  VkPhysicalDevice physical_device;
  VkBuffer buffer;
  VkDeviceMemory memory;
};
    
} } } }

#endif
