///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_BUFFER_HPP
#define FASTDRAW_OUTPUT_VULKAN_BUFFER_HPP

#include <vulkan/vulkan_core.h>

namespace fastdraw { namespace output { namespace vulkan {

uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties
                          , VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  
  throw std::runtime_error("failed to find suitable memory type!");
}

std::pair<VkBuffer, VkDeviceMemory> create_buffer
  (VkDevice device, std::size_t size, VkPhysicalDevice physicalDevice
   , VkBufferUsageFlags usage_flags
   , VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE)
{
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;

  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage_flags;
  bufferInfo.sharingMode = sharing_mode;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to create vertex buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = output::find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate vertex buffer memory!");
  }
  vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

  return {vertexBuffer, vertexBufferMemory};
}
    
} } }

#endif
