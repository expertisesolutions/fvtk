///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_STORAGE_BUFFER_ALLOCATOR_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_STORAGE_BUFFER_ALLOCATOR_HPP

namespace ftk { namespace ui {

struct storage_buffer_allocator
{
  storage_zindex (VkDevice device, VkPhysicalDevice physical_device
                  , VkExtent2D extent)
  {
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Z) * extent.width * extent.height;
    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto r = from_result(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code (r));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

    VkMemoryAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    info.allocationSize = size = mem_requirements.size;

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
    
    r = from_result(vkAllocateMemory(device, &info, nullptr, &memory));
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

  unsigned int size;
  VkBuffer buffer;
  VkDeviceMemory memory;
};
    
} }

#endif
