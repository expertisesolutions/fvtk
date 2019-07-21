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
    // if (allocated_ != 0)
    // {
    //   vkDestroyBuffer (device, buffer, nullptr);
    //   vkFreeMemory (device, memory, nullptr);
    // }

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
          && (mem_properties.memoryTypes[i].propertyFlags
              ==  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
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
    std::memcpy (map_pointer + offset * elements_size, &t, elements_size);
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
    std::memcpy (map_pointer + offset, &t, elements_size);
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
    
// template <typename Backend>
// void toplevel_window<Backend>::create_vertex_buffer ()
// {
//   const unsigned int vertex_data_size = (12 + 12 + 16) * sizeof(float);

//   auto whole_width = window.voutput.swapChainExtent.width;
//   auto whole_height = window.voutput.swapChainExtent.height;
//   // auto whole_width = output.swapChainExtent.width;
//   // auto whole_height = output.swapChainExtent.height;

//   //auto scale = 2;

//   //auto& image = images[0];
      
//   //        for (int i = 0; i != 12; i += 2)
//   //          {     
//   //   std::cout << "x: " << vertices[i] << " y: " << vertices[i+1] << std::endl;
//   // }

//   auto findMemoryType =
//     [] (uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) -> uint32_t
//     { 
//       VkPhysicalDeviceMemoryProperties memProperties;
//       vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

//       for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
//         if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
//           return i;
//         }
//       }

//       throw std::runtime_error("failed to find suitable memory type!");
//     };

//   auto create_vertex_buffer =
//     [findMemoryType] (VkDevice device, std::size_t size, VkPhysicalDevice physicalDevice) -> std::pair<VkBuffer, VkDeviceMemory>
//     {
//      VkBuffer vertexBuffer;
//      VkDeviceMemory vertexBufferMemory;

//      VkBufferCreateInfo bufferInfo = {};
//      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//      bufferInfo.size = /*sizeof(vertices[0]) * vertices.size()*/size;
//      bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
     
//      if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create vertex buffer!");
//      }

//      VkMemoryRequirements memRequirements;
//      vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

//      VkMemoryAllocateInfo allocInfo = {};
//      allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//      allocInfo.allocationSize = memRequirements.size;
//      allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);

//      if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
//        throw std::runtime_error("failed to allocate vertex buffer memory!");
//      }
//      vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

//      return {vertexBuffer, vertexBufferMemory};
//     };

//   VkDeviceMemory vertexBufferMemory;
//   std::tie(vertex_buffer, vertexBufferMemory) = create_vertex_buffer
//     (window.voutput.device, vertex_data_size * images.size(), window.voutput.physical_device);

//   //CHRONO_COMPARE()
//   void* data;
//   vkMapMemory(window.voutput.device, vertexBufferMemory, 0, vertex_data_size * images.size(), 0, &data);

//   //std::cout << "whole_width " << whole_width << " whole_height " << whole_height << std::endl;
             
//   unsigned int i = 0;
//   for (auto&& image : images)
//   {
//     auto x = image.x;
//     auto y = image.y;
//     const float vertices[] =
//       {     fastdraw::coordinates::ratio(x, whole_width/*/scale*/)/**2-1.0f*/              , fastdraw::coordinates::ratio(y, whole_height/*/scale*/)/**2-1.0f*/
//               , fastdraw::coordinates::ratio(x + image.width, whole_width/*/scale*/)/**2-1.0f*/, fastdraw::coordinates::ratio(y, whole_height/*/scale*/)/**2-1.0f*/
//               , fastdraw::coordinates::ratio(x + image.width, whole_width/*/scale*/)/**2-1.0f*/, fastdraw::coordinates::ratio(y + image.height, whole_height/*/scale*/)/**2-1.0f*/
//               , fastdraw::coordinates::ratio(x + image.width, whole_width/*/scale*/)/**2-1.0f*/, fastdraw::coordinates::ratio(y + image.height, whole_height/*/scale*/)/**2-1.0f*/
//               , fastdraw::coordinates::ratio(x, whole_width/*/scale*/)/**2-1.0f*/              , fastdraw::coordinates::ratio(y + image.height, whole_height/*/scale*/)/**2-1.0f*/
//               , fastdraw::coordinates::ratio(x, whole_width/*/scale*/)/**2-1.0f*/              , fastdraw::coordinates::ratio(y, whole_height/*/scale*/)/**2-1.0f*/
//       };
//     // const float vertices[]
//     // {
//     //      -1.0f,-1.0f
//     //     , 1.0f,-1.0f
//     //     , 1.0f, 1.0f
//     //     , 1.0f, 1.0f
//     //     ,-1.0f, 1.0f
//     //     ,-1.0f,-1.0f
//     // };
//     const float coordinates[] =
//       {     0.0f, 0.0f
//             , 1.0f, 0.0f
//             , 1.0f, 1.0f
//             , 1.0f, 1.0f
//             , 0.0f, 1.0f
//             , 0.0f, 0.0f
//       };
//     const float transform_matrix[] =
//       {
//        /* 90 degrees */
//        //   0.0f, -1.0f, 0.0f, 0.0f
//        // , 1.0f,  0.0f, 0.0f, 0.0f
//        // , 0.0f,  0.0f, 1.0f, 0.0f
//        // , 0.0f,  0.0f, 0.0f, 1.0f
//        /* normal */
//        1.0f, 0.0f, 0.0f, 0.0f
//        , 0.0f, 1.0f, 0.0f, 0.0f
//        , 0.0f, 0.0f, 1.0f, 0.0f
//        , 0.0f, 0.0f, 0.0f, 1.0f
//       };
//     assert(vertex_data_size == sizeof(vertices) + sizeof(coordinates) + sizeof(transform_matrix));
//     char* buffer = static_cast<char*>(data) + i * vertex_data_size;
//     //CHRONO_COMPARE()
//     std::memcpy(buffer, vertices, sizeof(vertices));
//     std::memcpy(&buffer[sizeof(vertices)], coordinates, sizeof(coordinates));
//     std::memcpy(&buffer[sizeof(vertices) + sizeof(coordinates)], transform_matrix, sizeof(transform_matrix));
//     ++i;
//   }

//   //CHRONO_COMPARE()
//   vkUnmapMemory(window.voutput.device, vertexBufferMemory);
//   //CHRONO_COMPARE()
// }
    
} }

#endif
