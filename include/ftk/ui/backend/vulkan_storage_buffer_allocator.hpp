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

namespace ftk { namespace ui { namespace vulkan { namespace detail {

struct memory_device
{
  VkDeviceMemory memory;
  VkDeviceSize allocated_size;

  memory_device (VkDeviceMemory memory, VkDeviceSize size)
    : memory (memory), allocated_size (size) {}
  memory_device () : memory (nullptr), allocated_size (0u) {}
  memory_device (memory_device&& other)
    : memory (other.memory)
  {
    other.memory = nullptr;
  }
  memory_device& operator= (memory_device&& other)
  {
    if (memory)
      VkFreeMemory (memory);
    memory = other.memory;
    other.memory = nullptr;
    return *this;
  }
  ~memory_device ()
  {
    if (!memory)
      vkFreeMemory (memory);
  }
};

memory_device create_device_memory (VkDevice device, VkPhysicalDevice physical_device, VkBuffer buffer
                                     , VkMemoryPropertyFlagBits memory_flags)
{
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
             & (memory_flags))) ==  (memory_flags))
    {
      info.memoryTypeIndex = i;
      break;
    }
  }

  VkDeviceMemory memory;
  r = from_result(vkAllocateMemory(device, &info, nullptr, &memory));
  if (r != vulkan_error_code::success)
    throw std::system_error(make_error_code (r));

  return {memory, mem_requirements.size}
}          

struct buffer_allocator
{
  buffer_allocator
    (VkDevice device, VkPhysicalDevice physical_device//, VkExtent2D extent
     , VkBufferUsageFlags buffer_usage, VkSharingMode sharing
     , VkMemoryPropertyFlagBits memory_flags)
      : device (device), physical_device (physical_device)
        //, extent (extent)
      , buffer_usage (buffer_usage), sharing (sharing)
      , memory_flags (memory_flags)
  {
  }

  VkDeviceMemory create_memory (VkBuffer buffer)
  {
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Z) * extent.width * extent.height;
    bufferInfo.usage = buffer_usage;//VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.sharingMode = sharing;

    auto r = from_result(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));


    std::cout << "create_memory " << size << std::endl;
    void* map_pointer = nullptr;
    vkMapMemory (device, memory, 0, size, 0, &map_pointer);
    std::memset (map_pointer, 0, size);
    vkUnmapMemory (device, memory);
    
    
    vkBindBufferMemory(device, buffer, memory, 0);
  }

  VkBuffer get_buffer () const { return buffer; }

    VkDevice device;
    VkPhysicalDevice physical_device;
    VkBufferUsageFlags buffer_usage;
    VkSharingMode sharing;
    VkMemoryPropertyFlagBits memory_flags;
};
    
} } } }

#endif
