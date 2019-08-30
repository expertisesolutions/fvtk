///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_DESCRIPTOR_FIXED_ARRAY_HPP
#define FTK_FTK_UI_BACKEND_DESCRIPTOR_FIXED_ARRAY_HPP

#include <array>

namespace ftk { namespace ui { namespace backend { namespace vulkan {

namespace detail {

template <VkDescriptorType>
struct descriptor_type_traits;

template <>
struct descriptor_type_traits<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>
{
  typedef VkDescriptorImageInfo info_type;

  static void update_info (VkWriteDescriptorSet& set, info_type const* info)
  {
    set.pImageInfo = info;
  }
};

template <>
struct descriptor_type_traits<VK_DESCRIPTOR_TYPE_SAMPLER>
{
  typedef VkDescriptorImageInfo info_type;

  static void update_info (VkWriteDescriptorSet& set, info_type const* info)
  {
    set.pImageInfo = info;
  }
};
  
}
      
template <VkDescriptorType DescriptorType, std::size_t Size = 4096>
struct descriptor_fixed_array
{
  typedef detail::descriptor_type_traits<DescriptorType> descriptor_type_traits;
  typedef typename descriptor_type_traits::info_type info_type;
  
  descriptor_fixed_array ()
    : device (nullptr), descriptor_pool (nullptr), current_size (0)
  {
  }
  descriptor_fixed_array (VkDevice device, VkDescriptorSetLayout set_layout
                          , std::optional<info_type> empty_info = {}, VkDescriptorPoolCreateFlags flags = 0)
    : device (device), current_size (0), empty_info (empty_info)
  {
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    VkDescriptorPoolSize pool_sizes = {};
    pool_sizes.type = DescriptorType;
    pool_sizes.descriptorCount = Size;
    VkDescriptorPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.maxSets = 1;
    create_info.poolSizeCount = 1;
    create_info.pPoolSizes = &pool_sizes;
    create_info.flags = flags;

    std::cout << "creating descriptor pool" << std::endl;
    auto r = from_result (vkCreateDescriptorPool (device, &create_info, nullptr, &descriptor_pool));
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code(r));
    std::cout << "created descriptor pool" << std::endl;

    VkDescriptorSetAllocateInfo set_info = {};
    set_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    set_info.descriptorPool = descriptor_pool;
    set_info.descriptorSetCount = 1;
    set_info.pSetLayouts = &set_layout;
    std::cout << "allocating descriptor" << std::endl;
    r = from_result (vkAllocateDescriptorSets (device, &set_info, &set));
    if (r != vulkan_error_code::success)
      throw std::system_error (make_error_code(r));
    std::cout << "allocated descriptor" << std::endl;

    if (empty_info)
    {
      VkWriteDescriptorSet write_set = {};
      write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_set.dstSet = this->set;
      write_set.dstBinding = 0;
      write_set.descriptorType = DescriptorType;
      write_set.dstArrayElement = 0;
      std::array <info_type, Size> types;
      for (auto&& t : types)
        t = *empty_info;
      write_set.descriptorCount = Size;
      descriptor_type_traits::update_info (write_set, &types[0]);
      vkUpdateDescriptorSets (device, 1, &write_set, 0, nullptr);
    }
  }

  void replace (int index, info_type type)
  {
    std::cout << "push_back " << current_size << std::endl;
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    std::cout << "image_view " << type.imageView << std::endl;

    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = this->set;
    write_set.dstBinding = 0;
    write_set.descriptorType = DescriptorType;
    write_set.dstArrayElement = index;
    std::cout << "count " << write_set.descriptorCount << " dstArrayElement "
              << write_set.dstArrayElement << std::endl;
    if (index == 0) // update all
    {
      std::array <info_type, Size> types;
      for (auto&& t : types)
        t = type;
      write_set.descriptorCount = Size;
      descriptor_type_traits::update_info (write_set, &types[0]);
    }
    else
    {
      write_set.descriptorCount = 1;
      descriptor_type_traits::update_info (write_set, &type);
    }
    std::cout << "writing descriptorset" << std::endl;
    vkUpdateDescriptorSets (device, 1, &write_set, 0, nullptr);
    std::cout << "wrote descriptorset" << std::endl;
  }

  void push_back (info_type type)
  {
    if (current_size == Size)
      throw std::runtime_error ("Max descriptors");
    
    std::cout << "push_back " << current_size << std::endl;
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    std::cout << "image_view " << type.imageView << std::endl;

    VkWriteDescriptorSet write_set = {};
    write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_set.dstSet = this->set;
    write_set.dstBinding = 0;
    write_set.descriptorType = DescriptorType;
    write_set.dstArrayElement = current_size;
    std::cout << "count " << write_set.descriptorCount << " dstArrayElement "
              << write_set.dstArrayElement << std::endl;
    if (current_size == 0) // update all
    {
      std::array <info_type, Size> types;
      for (auto&& t : types)
        t = type;
      write_set.descriptorCount = Size;
      descriptor_type_traits::update_info (write_set, &types[0]);
    }
    else
    {
      write_set.descriptorCount = 1;
      descriptor_type_traits::update_info (write_set, &type);
    }
    std::cout << "writing descriptorset" << std::endl;
    vkUpdateDescriptorSets (device, 1, &write_set, 0, nullptr);
    std::cout << "wrote descriptorset" << std::endl;
    ++current_size;
  }

  VkDevice device;
  VkDescriptorPool descriptor_pool;
  VkDescriptorSet set;
  std::size_t current_size;
  std::optional<info_type> empty_info;
};
      
} } } }

#endif
