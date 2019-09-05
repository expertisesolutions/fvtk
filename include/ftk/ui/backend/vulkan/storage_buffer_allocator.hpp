///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_BUFFER_ALLOCATOR_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_BUFFER_ALLOCATOR_HPP

#include <list>
#include <map>

namespace ftk { namespace ui { namespace backend { namespace vulkan {

struct memory_device
{
  VkDevice device;
  VkDeviceMemory memory;
  VkDeviceSize allocated_size;

  memory_device (VkDevice device, VkDeviceMemory memory, VkDeviceSize size)
    : device (device), memory (memory), allocated_size (size) {}
  memory_device () : device (nullptr), memory (nullptr), allocated_size (0u)
  {}
  memory_device (memory_device&& other)
    : device (other.device)
    , memory (other.memory)
    , allocated_size (other.allocated_size)
  {
    other.device = nullptr;
    other.memory = nullptr;
    other.allocated_size = 0;
  }
  memory_device& operator= (memory_device&& other)
  {
    if (memory)
      vkFreeMemory (device, memory, nullptr);
    device = other.device;
    memory = other.memory;
    other.device = nullptr;
    other.memory = nullptr;
    return *this;
  }
  ~memory_device ()
  {
    if (!memory)
      vkFreeMemory (device, memory, nullptr);
  }
};

uint32_t get_memory_index (VkPhysicalDevice physical_device, uint32_t memory_type_bits
                           , uint32_t memory_flags)
{
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
  {
    if ((memory_type_bits & (1 << i))
        && ((mem_properties.memoryTypes[i].propertyFlags
             & (memory_flags))) ==  (memory_flags))
    {
      return i;
    }
  }
  throw std::runtime_error ("Memory not supported");
}
      
memory_device create_device_memory (VkDevice device, VkPhysicalDevice physical_device, VkBuffer buffer
                                     , VkMemoryPropertyFlagBits memory_flags)
{
  using fastdraw::output::vulkan::vulkan_error_code;
  using fastdraw::output::vulkan::from_result;
  
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

  std::cout << "alignment " << mem_requirements.alignment << " size " << mem_requirements.size << std::endl;

  VkMemoryAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  info.allocationSize = mem_requirements.size;

  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
  {
    if ((mem_requirements.memoryTypeBits & (1 << i))
        && ((mem_properties.memoryTypes[i].propertyFlags
             & (memory_flags))) ==  (memory_flags))
    {
      info.memoryTypeIndex = i;
      break;
    }
  }

  VkDeviceMemory memory;
  auto r = from_result(vkAllocateMemory(device, &info, nullptr, &memory));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code (r));

  return {device, memory, mem_requirements.size};
}          

struct buffer_allocator
{
  buffer_allocator
    (VkDevice device, VkPhysicalDevice physical_device
     , VkMemoryPropertyFlags memory_flags)
      : device (device), physical_device (physical_device)
      , memory_flags (memory_flags)
  {
  }

  void allocate (VkBuffer buffer)
  {
    std::cout << "buffer_allocator::allocate" << std::endl;
    using fastdraw::output::vulkan::vulkan_error_code;
    using fastdraw::output::vulkan::from_result;
    
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

    std::cout << "alignment " << mem_requirements.alignment << " size " << mem_requirements.size << std::endl;

    auto memory_index = get_memory_index (physical_device, mem_requirements.memoryTypeBits, memory_flags);

    std::list<memory_keep>::iterator memory_iterator = memories.end();
    if (memories.size() <= memory_index || *(memory_iterator = std::next(memories.begin(), memory_index)) == memory_keep{})
    {
      if (memory_iterator == memories.end())
      {
        memories.resize(memory_index + 1);
        memory_iterator = std::prev(memories.end());
        memory_iterator->memory_index = memory_index;
      }
    
      VkMemoryAllocateInfo info = {};
      info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      info.allocationSize = 256*1024*1024;
      info.memoryTypeIndex = memory_index;

      VkDeviceMemory memory;
      auto r = from_result(vkAllocateMemory(device, &info, nullptr, &memory));
      if (r != vulkan_error_code::success)
        throw std::system_error(make_error_code (r));

      vkBindBufferMemory(device, buffer, memory, 0);

      memory_iterator->vulkan_allocations
        .push_back (/*memory_keep::vulkan_allocation*/
                    {memory, info.allocationSize,
                       {/*memory_keep::vulkan_allocation::buffer_allocation*/
                          {buffer, 0u /*offset*/, mem_requirements.size}}});
      buffer_map.insert ({buffer, {memory_index, std::prev(memory_iterator->vulkan_allocations.end())
                                   , memory_iterator->vulkan_allocations.back().buffer_allocations.begin()}});
    }
    else
    {
      memory_iterator = std::next (memories.begin(), memory_index);
      auto buffer_hole = find_hole (*memory_iterator, mem_requirements.size, mem_requirements.alignment);

      if (buffer_hole != memory_iterator->vulkan_allocations.back().buffer_allocations.end())
      {
        std::cout << "something else" << std::endl;
        throw std::runtime_error ("not implemented yet");
      }
      else
      {
        auto before = std::prev (buffer_hole);
        auto size = memory_iterator->vulkan_allocations.back().vulkan_allocated_size;
        if (size - (before->offset + before->size) >= mem_requirements.size)
        {
          std::cout << "found space" << std::endl;

          auto new_offset = align(before->offset + before->size, mem_requirements.alignment);

          std::cout << "new offset " << new_offset << " old offset " << before->offset + before->size << std::endl;
          
          vkBindBufferMemory(device, buffer, memory_iterator->vulkan_allocations.back().vulkan_memory
                             , new_offset);

          memory_iterator->vulkan_allocations.back()
            .buffer_allocations.push_back ({buffer, new_offset, mem_requirements.size});
          buffer_map.insert ({buffer, {memory_index, std::prev(memory_iterator->vulkan_allocations.end())
                                       , std::prev(memory_iterator->vulkan_allocations.back().buffer_allocations.end())}});
        }
      }
    }
  }

  // VkDeviceMemory create_memory (VkBuffer buffer)
  // {
  //   using fastdraw::output::vulkan::from_result;
  //   using fastdraw::output::vulkan::vulkan_error_code;

  //   // VkBufferCreateInfo bufferInfo = {};
  //   // bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  //   // bufferInfo.size = sizeof(Z) * extent.width * extent.height;
  //   // bufferInfo.usage = buffer_usage;//VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  //   // bufferInfo.sharingMode = sharing;

  //   // auto r = from_result(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
  //   // if (r != vulkan_error_code::success)
  //   //   throw std::system_error(make_error_code (r));

  //   // std::cout << "create_memory " << size << std::endl;
  //   // void* map_pointer = nullptr;
  //   // vkMapMemory (device, memory, 0, size, 0, &map_pointer);
  //   // std::memset (map_pointer, 0, size);
  //   // vkUnmapMemory (device, memory);
    
    
  //   // vkBindBufferMemory(device, buffer, memory, 0);
  // }

  //VkBuffer get_buffer () const { return buffer; }
  struct buffer_allocation
  {
    VkBuffer buffer;
    std::size_t offset;
    std::size_t size;

    friend bool operator==(buffer_allocation const& lhs, buffer_allocation const& rhs)
    { return lhs.buffer == rhs.buffer
        && lhs.offset == rhs.offset
        && lhs.size == rhs.size; }
    friend bool operator!=(buffer_allocation const& lhs, buffer_allocation const& rhs)
    { return !(lhs == rhs); }
  };

  struct vulkan_allocation
  {
    VkDeviceMemory vulkan_memory;
    std::size_t vulkan_allocated_size;

    friend bool operator==(vulkan_allocation const& lhs, vulkan_allocation const& rhs)
    { return lhs.vulkan_memory == rhs.vulkan_memory
        && lhs.vulkan_allocated_size == rhs.vulkan_allocated_size
        && lhs.buffer_allocations == rhs.buffer_allocations; }
    friend bool operator!=(vulkan_allocation const& lhs, vulkan_allocation const& rhs)
    { return !(lhs == rhs); }
      
    std::list<buffer_allocation> buffer_allocations;
  };
  
  struct memory_keep
  {
    uint32_t memory_index;

    
    std::list<vulkan_allocation> vulkan_allocations;

    friend bool operator==(memory_keep const& lhs, memory_keep const& rhs)
    { return lhs.memory_index == rhs.memory_index
        && lhs.vulkan_allocations == rhs.vulkan_allocations; }
    friend bool operator!=(memory_keep const& lhs, memory_keep const& rhs)
    { return !(lhs == rhs); }
  };

  static std::size_t align (std::size_t size, std::size_t alignment)
  {
    if (size % alignment != 0)
      return size + (size % alignment);
    else
      return size;
  }

  std::list<buffer_allocation>::iterator find_hole
    (memory_keep& memory, std::size_t size, std::size_t alignment)
  {
    for (auto&& vulkan_allocation : memory.vulkan_allocations)
    {
      std::size_t last_end = 0;
      auto buffer_iterator = vulkan_allocation.buffer_allocations.begin()
        , buffer_last_iterator = vulkan_allocation.buffer_allocations.end();
      for (; buffer_iterator != buffer_last_iterator; ++buffer_iterator)
      {
        if (buffer_iterator->offset - last_end >= size)
        {
          std::cout << "found hole" << std::endl;
          return buffer_iterator;
        }
        else
          last_end = align(buffer_iterator->offset + buffer_iterator->size, alignment);
      }

      // search last
      if (vulkan_allocation.vulkan_allocated_size - last_end >= size)
        return buffer_iterator;
    }

    throw std::runtime_error ("hole not found");
    return {};
  }

  void* map (VkBuffer buffer)
  {
    assert (buffer_map.find(buffer) != buffer_map.end());
    auto path = buffer_map[buffer];

    void* map_pointer;
    std::cout << " mapping at offset " << path.buffer_allocation_iterator->offset
              << " size " << path.buffer_allocation_iterator->size << std::endl;
    vkMapMemory (device, path.vulkan_allocation_iterator->vulkan_memory
                 , path.buffer_allocation_iterator->offset
                 , path.buffer_allocation_iterator->size, 0, &map_pointer);

    return map_pointer;
  }

  void unmap (VkBuffer buffer)
  {
    assert (buffer_map.find(buffer) != buffer_map.end());
    auto path = buffer_map[buffer];

    vkUnmapMemory (device, path.vulkan_allocation_iterator->vulkan_memory);
    // VkMappedMemoryRange range = {};
    // range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    // range.memory = path.vulkan_allocation_iterator->vulkan_memory;
    // range.size = VK_WHOLE_SIZE;
  }

  struct path
  {
    uint32_t memory_index;
    std::list<vulkan_allocation>::iterator vulkan_allocation_iterator;
    std::list<buffer_allocation>::iterator buffer_allocation_iterator;
  };
  
  std::list<memory_keep> memories;
  std::map<VkBuffer, path> buffer_map;
  VkDevice device;
  VkPhysicalDevice physical_device;
  //VkBufferUsageFlags buffer_usage;
  // VkSharingMode sharing;
  VkMemoryPropertyFlags memory_flags;
};
    
} } } }

#endif
