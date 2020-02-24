///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_ADD_DMABUF_IMAGE_HPP
#define FASTDRAW_OUTPUT_VULKAN_ADD_DMABUF_IMAGE_HPP

#include <fastdraw/output/vulkan/vulkan_output_info.hpp>
#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>
#include <fastdraw/output/vulkan/buffer.hpp>
#include <fastdraw/object/image.hpp>
#include <fastdraw/coordinates.hpp>
#include <fastdraw/color.hpp>

#include <vector>
#include <stdexcept>
#include <cstring>

#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */

#include <png.h>

// #define USE_vkCreateDmaBufImageINTEL
// #include <vulkan/vulkan_intel.h>
// #undef USE_vkCreateDmaBufImageINTEL

#error
namespace fastdraw { namespace output { namespace vulkan {

template <typename Point, typename WindowingBase>
vulkan_draw_info create_output_specific_object (vulkan_output_info<WindowingBase>& output, object::dmabuf_image<Point> const& image
                                               , VkPipelineRasterizationStateCreateInfo rasterizer
                                               = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO // sType
                                                  , nullptr                                                  // pNext
                                                  , 0u                                                       // flags
                                                  , VK_FALSE                                                 // depthClampEnable
                                                  , VK_FALSE                                                 // rasterizerDiscardEnable
                                                  , VK_POLYGON_MODE_FILL                                     // polygonMode
                                                  , VK_CULL_MODE_NONE                                    // cullMode
                                                  , VK_FRONT_FACE_CLOCKWISE                                  // frontFace
                                                  , VK_FALSE                                                 // depthBiasEnable
                                                  , 0.0f                                                     // depthBiasConstantFactor
                                                  , 0.0f                                                     // depthBiasClamp
                                                  , 0.0f                                                     // depthBiasSlopeFactor
                                                  , 1.0f                                                     // lineWidth
                                               }
                                               , VkPipelineMultisampleStateCreateInfo multisampling
                                               = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO // sType
                                                   , nullptr                                               // pNext
                                                   , 0u                                                    // flags
                                                   , VK_SAMPLE_COUNT_1_BIT                                 // rasterizationSamples
                                                   , VK_FALSE                                              // sampleShadingEnable
                                                   , 1.0f                                                  // minSampleShading
                                                   , nullptr                                               // pSampleMask
                                                   , VK_FALSE                                              // alphaToCoverageEnable
                                                   , VK_FALSE                                              // alphaToOneEnable
                                               }
                                               , VkPipelineColorBlendAttachmentState colorBlendAttachment
                                               = {VK_TRUE                                                  // blendEnable
                                                  , VK_BLEND_FACTOR_ONE                                    // srcColorBlendFactor
                                                  , VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA                    // dstColorBlendFactor
                                                  , VK_BLEND_OP_ADD                                        // colorBlendOp
                                                  , VK_BLEND_FACTOR_ONE                                    // srcAlphaBlendFactor
                                                  , VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA                    // dstAlphaBlendFactor
                                                  , VK_BLEND_OP_ADD                                        // alphaBlendOp
                                                  , VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT    // colorWriteMask
                                                  | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT    // colorWriteMask
                                               }
                                               , VkPipelineColorBlendStateCreateInfo colorBlending
                                               = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO // sType
                                                   , nullptr                                               // pNext
                                                   , 0u                                                    // flags
                                                   , VK_FALSE                                              // logicOpEnable
                                                   , VK_LOGIC_OP_COPY                                      // logicOp
                                                   , 1                                                     // attachmentCount
                                                   , nullptr                                               // pAttachments = &colorBlendAttachment 
                                                   , {1.0f, 1.0f, 1.0f, 1.0f}                              // blendConstants
                                               }
                                               )
{
  // hb_font_t* font = hb_ft_font_create ();
  if(colorBlending.attachmentCount == 1 && colorBlending.pAttachments == nullptr)
    colorBlending.pAttachments = &colorBlendAttachment;

  // auto size = image.stride*image.height;
  // std::cout << "size " << size << std::endl;
  
  // auto staging_pair = vulkan::create_buffer(output.device, size, output.physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  // std::cout << __FILE__ ":" << __LINE__ << std::endl;
  {
  }

  std::cout << __FILE__ ":" << __LINE__ << std::endl;
  
  auto texture_width = image.width, texture_height = image.height;

  std::cout << "width " << texture_width << " height " << image.height << std::endl;

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;

  auto format = VK_FORMAT_B8G8R8A8_UNORM; //VK_FORMAT_R8G8B8A8_UNORM; RGBA vs ARGB
  
  // VkImageCreateInfo imageInfo = {};
  // imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  // imageInfo.imageType = VK_IMAGE_TYPE_2D;
  // imageInfo.extent.width = static_cast<uint32_t>(texture_width);
  // imageInfo.extent.height = static_cast<uint32_t>(texture_height);
  // imageInfo.extent.depth = 1;
  // imageInfo.mipLevels = 1;
  // imageInfo.arrayLayers = 1;
  // imageInfo.format = format;
  // imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  // imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  // imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  // //imageInfo.flags = 0; // Optional

  // std::cout << __FILE__ ":" << __LINE__ << std::endl;
  // if (vkCreateImage(output.device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
  //   throw std::runtime_error("failed to create image!");
  // }

  // VkMemoryRequirements memRequirements;
  // vkGetImageMemoryRequirements(output.device, textureImage, &memRequirements);

  // {
  //   VkMemoryAllocateInfo allocInfo = {};
  //   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  //   allocInfo.allocationSize = memRequirements.size;
  //   allocInfo.memoryTypeIndex = vulkan::find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  //                                                        , output.physical_device);

  //   if (vkAllocateMemory(output.device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
  //     throw std::runtime_error("failed to allocate image memory!");
  //   }
  // }

  // vkBindImageMemory(output.device, textureImage, textureImageMemory, 0);
  // std::cout << __FILE__ ":" << __LINE__ << std::endl;

  {
    struct VkImageCreateInfo info {};
    //info.stride = image.stride;
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.format = format;
    info.extent = VkExtent3D{image.width, image.height, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = (VkSampleCountFlagBits)1;
    info.tiling = /*VK_IMAGE_TILING_OPTIONAL.*/VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageType = VK_IMAGE_TYPE_2D;
    // auto r = vkImageCreate(_device,
    //   &(struct VkImageCreateInfo) {
    //      .isl_tiling_flags = ISL_TILING_X_BIT,
    //      .stride = pCreateInfo->strideInBytes,
    //      .vk_info = &(VkImageCreateInfo) {
    //         .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    //         .imageType = VK_IMAGE_TYPE_2D,
    //         .format = pCreateInfo->format,
    //         .extent = pCreateInfo->extent,
    //         .mipLevels = 1,
    //         .arrayLayers = 1,
    //         .samples = 1,
    //         /* FIXME: Need a way to use X tiling to allow scanout */
    //         .tiling = VK_IMAGE_TILING_OPTIMAL,
    //         .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    //         .flags = 0,
    //      }
    //   }, pAllocator, &image_h);
    VkImage image;
    auto r = vkCreateImage (output.device, &info, nullptr, &image);
    std::cout << "vkImageCreate return " << r << std::endl;
  }
  
  {
    PFN_vkCreateDmaBufImageINTEL bs_vkCreateDmaBufImageINTEL =
      reinterpret_cast<PFN_vkCreateDmaBufImageINTEL>
      (vkGetDeviceProcAddr(output.device, "vkCreateDmaBufImageINTEL"));
    if (bs_vkCreateDmaBufImageINTEL == NULL) {
      //bs_debug_error("vkGetDeviceProcAddr(\"vkCreateDmaBufImageINTEL\') failed");
      std::cout << "Couldn't find intel extension" << std::endl;
      throw -1;
    }

    VkDmaBufImageCreateInfo info {};
    info.sType = (VkStructureType)VK_STRUCTURE_TYPE_DMA_BUF_IMAGE_CREATE_INFO_INTEL;
    info.fd = image.fd;
    info.format = format;
    info.extent = VkExtent3D{image.width, image.height, 1};
    info.strideInBytes = image.stride;
    auto r = bs_vkCreateDmaBufImageINTEL (output.device, &info, nullptr, &textureImageMemory, &textureImage);
    std::cout << "vkCreateDmaBufImageINTEL " << r << std::endl;
  }
  
  // VkCommandBuffer commandBuffer;
  // {
  //   VkCommandBufferAllocateInfo allocInfo = {};
  //   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  //   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  //   allocInfo.commandPool = output.command_pool;
  //   allocInfo.commandBufferCount = 1;

  //   vkAllocateCommandBuffers(output.device, &allocInfo, &commandBuffer);
  // }

  // std::cout << __FILE__ ":" << __LINE__ << std::endl;
  // VkCommandBufferBeginInfo beginInfo = {};
  // beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  // beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
  // vkBeginCommandBuffer(commandBuffer, &beginInfo);

  // // VkBufferCopy copyRegion = {};
  // // copyRegion.size = size;
  // // vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  // VkImageMemoryBarrier barrier = {};
  // barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  // barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_UNDEFINED;
  // barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  // barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  // barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  // barrier.image = textureImage;
  // barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // barrier.subresourceRange.baseMipLevel = 0;
  // barrier.subresourceRange.levelCount = 1;
  // barrier.subresourceRange.baseArrayLayer = 0;
  // barrier.subresourceRange.layerCount = 1;

  // barrier.srcAccessMask = 0;
  // barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  // barrier.srcAccessMask = 0; // TODO
  // barrier.dstAccessMask = 0; // TODO

  // vkCmdPipelineBarrier(
  //   commandBuffer,
  //   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT /* TODO */, VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */,
  //   0,
  //   0, nullptr,
  //   0, nullptr,
  //   1, &barrier
  // );

  // std::cout << __FILE__ ":" << __LINE__ << std::endl;
  // VkBufferImageCopy region = {};
  // region.bufferOffset = 0;
  // region.bufferRowLength = 0;
  // region.bufferImageHeight = 0;

  // region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  // region.imageSubresource.mipLevel = 0;
  // region.imageSubresource.baseArrayLayer = 0;
  // region.imageSubresource.layerCount = 1;

  // region.imageOffset = {0, 0, 0};
  // region.imageExtent = {
  //                       texture_width,
  //                       texture_height,
  //                       1
  // };
  
  // vkCmdCopyBufferToImage(
  //                        commandBuffer,
  //                        staging_pair.first,
  //                        textureImage,
  //                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  //                        1,
  //                        &region
  //                        );  

  // barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  // barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  // barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  // barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  // vkCmdPipelineBarrier(
  //   commandBuffer,
  //   VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /* TODO */,
  //   0,
  //   0, nullptr,
  //   0, nullptr,
  //   1, &barrier
  // );
  // std::cout << __FILE__ ":" << __LINE__ << std::endl;


  // vkEndCommandBuffer(commandBuffer);

  // VkSubmitInfo submitInfo = {};
  // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  // submitInfo.commandBufferCount = 1;
  // submitInfo.pCommandBuffers = &commandBuffer;
  
  // auto submit_error = vkQueueSubmit(output.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
  // vkQueueWaitIdle(output.graphics_queue);

  // if (submit_error)
  //   throw -1;

  // vkFreeCommandBuffers(output.device, output.command_pool, 1, &commandBuffer);
  // vkDestroyBuffer(output.device, staging_pair.first, nullptr);
  // vkFreeMemory(output.device, staging_pair.second, nullptr);

  // image view
  VkImageView textureImageView;
  VkSampler textureSampler;

  {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(output.device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture image view!");
    }

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 0;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

  std::cout << __FILE__ ":" << __LINE__ << std::endl;
    if (vkCreateSampler(output.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture sampler!");
    }
  std::cout << __FILE__ ":" << __LINE__ << std::endl;
    
  }

  {
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    // std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1; //static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = /*bindings.data()*/&samplerLayoutBinding;

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(output.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create descriptor set layout!");
    }
  std::cout << __FILE__ ":" << __LINE__ << std::endl;

    VkDescriptorPool descriptorPool;
    /*std::array<*/VkDescriptorPoolSize/*, 2>*/ poolSizes = {};
    // poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // poolSizes[0].descriptorCount = /*static_cast<uint32_t>(swapChainImages.size())*/1;
    poolSizes/*[1]*/.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes/*[1]*/.descriptorCount = /*static_cast<uint32_t>(swapChainImages.size())*/1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1/*static_cast<uint32_t>(poolSizes.size())*/;
    poolInfo.pPoolSizes = &poolSizes/*.data()*/;
    poolInfo.maxSets = 1/*static_cast<uint32_t>(swapChainImages.size())*/;

     if (vkCreateDescriptorPool(output.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
     }    
    // for (size_t i = 0; i < swapChainImages.size(); i++) {
    // VkDescriptorBufferInfo bufferInfo = {};
    // bufferInfo.buffer = uniformBuffers[i];
    // bufferInfo.offset = 0;
    // bufferInfo.range = sizeof(UniformBufferObject);

    VkVertexInputBindingDescription bindingDescriptions[2] = {};
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(float)*2;
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescriptions[1].binding = 1;
    bindingDescriptions[1].stride = 0;//sizeof(float)*4;
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    
    VkVertexInputAttributeDescription attributeDescriptions[6] = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float)*12;

    attributeDescriptions[2].binding = 1;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = sizeof(float)*12 + sizeof(float)*12;

    attributeDescriptions[3].binding = 1;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[3].offset = sizeof(float)*12 + sizeof(float)*12 + sizeof(float)*4;

    attributeDescriptions[4].binding = 1;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = sizeof(float)*12 + sizeof(float)*12 + sizeof(float)*4*2;

    attributeDescriptions[5].binding = 1;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = sizeof(float)*12 + sizeof(float)*12 + sizeof(float)*4*3;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = sizeof(bindingDescriptions)/sizeof(bindingDescriptions[0]);
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = sizeof(attributeDescriptions)/sizeof(attributeDescriptions[0]);
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions; // Optional
    
    // attributeDescriptions[2].binding = 0;
    // attributeDescriptions[2].location = 2;
    // attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    // attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

     // std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);

     VkDescriptorSet descriptorSet;

     VkDescriptorSetAllocateInfo allocInfo = {};
     allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
     allocInfo.descriptorPool = descriptorPool;
     allocInfo.descriptorSetCount = 1;
     allocInfo.pSetLayouts = /*layouts.data()*/&descriptorSetLayout;

     if (vkAllocateDescriptorSets(output.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
       throw std::runtime_error("failed to allocate descriptor sets!");
     }

     // VkDescriptorBufferInfo bufferInfo = {};
     // bufferInfo.buffer = uniformBuffers[i];
     // bufferInfo.offset = 0;
     // bufferInfo.range = sizeof(UniformBufferObject);

     VkDescriptorImageInfo imageInfo = {};
     imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
     imageInfo.imageView = textureImageView;
     imageInfo.sampler = textureSampler;

     // VkWriteDescriptorSet descriptorWrite = {};
     // descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
     // descriptorWrite.dstSet = descriptorSets[i];
     // descriptorWrite.dstBinding = 0;
     // descriptorWrite.dstArrayElement = 0;
     // descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
     // descriptorWrite.descriptorCount = 1;
     // descriptorWrite.pBufferInfo = &bufferInfo;
       
     // vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

     /*std::array<*/VkWriteDescriptorSet/*, 2>*/ descriptorWrites = {};

    // descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // descriptorWrites[0].dstSet = descriptorSets[i];
    // descriptorWrites[0].dstBinding = 0;
    // descriptorWrites[0].dstArrayElement = 0;
    // descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // descriptorWrites[0].descriptorCount = 1;
    // descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites/*[1]*/.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites/*[1]*/.dstSet = descriptorSet/*s[i]*/;
    descriptorWrites/*[1]*/.dstBinding = 1;
    descriptorWrites/*[1]*/.dstArrayElement = 0;
    descriptorWrites/*[1]*/.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites/*[1]*/.descriptorCount = 1;
    descriptorWrites/*[1]*/.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(output.device, 1/*static_cast<uint32_t>(descriptorWrites.size())*/, &descriptorWrites/*.data()*/, 0, nullptr);

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(output.swapChainExtent.width);
    viewport.height = static_cast<float>(output.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    std::cout << "output.swapChainExtent.width " << output.swapChainExtent.width << std::endl;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = output.swapChainExtent;    

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = output.shader_loader->load(shader::indirect_draw_component_vertex);
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = output.shader_loader->load(shader::indirect_draw_component_frag);
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkPipelineLayout pipelineLayout;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
    if (vkCreatePipelineLayout(output.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
      throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = output.renderpass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(output.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
      throw std::runtime_error("failed to create graphics pipeline!");
    }

    VkBuffer vertexBuffer;
    {
      auto whole_width = output.swapChainExtent.height;
      auto whole_height = output.swapChainExtent.width;
      // auto whole_width = output.swapChainExtent.width;
      // auto whole_height = output.swapChainExtent.height;

      auto scale = 4;
      
      const float vertices[] =
        {     coordinates::ratio(image.pos.x, whole_width)              , coordinates::ratio(image.pos.y, whole_height)
            , coordinates::ratio(image.pos.x + image.size.x*scale, whole_width), coordinates::ratio(image.pos.y, whole_height)              
            , coordinates::ratio(image.pos.x + image.size.x*scale, whole_width), coordinates::ratio(image.pos.y + image.size.y*scale, whole_height)
            , coordinates::ratio(image.pos.x + image.size.x*scale, whole_width), coordinates::ratio(image.pos.y + image.size.y*scale, whole_height)
            , coordinates::ratio(image.pos.x, whole_width)              , coordinates::ratio(image.pos.y + image.size.y*scale, whole_height)
            , coordinates::ratio(image.pos.x, whole_width)              , coordinates::ratio(image.pos.y, whole_height)
        };
      const float coordinates[] =
        {   0.0f, 0.0f
            , 1.0f, 0.0f
            , 1.0f, 1.0f
            , 1.0f, 1.0f
            , 0.0f, 1.0f
            , 0.0f, 0.0f
        };
      const float transform_matrix[] =
        {
            1.0f, 1.0f, 1.0f, 1.0f
          , 1.0f, 1.0f, 1.0f, 1.0f
          , 1.0f, 1.0f, 1.0f, 1.0f
          , 1.0f, 1.0f, 1.0f, 1.0f
          //   1.0f, 0.0f, 0.0f, 0.0f
          // , 0.0f, 1.0f, 0.0f, 0.0f
          // , 0.0f, 0.0f, 1.0f, 0.0f
          // , 0.0f, 0.0f, 0.0f, 1.0f
        };

      for (int i = 0; i != 12; i += 2)
      {        
        std::cout << "x: " << vertices[i] << " y: " << vertices[i+1] << std::endl;
      }

      auto findMemoryType =
        [] (uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) -> uint32_t
        { 
          VkPhysicalDeviceMemoryProperties memProperties;
          vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

          for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
              return i;
            }
          }
  
          throw std::runtime_error("failed to find suitable memory type!");
        };


      auto create_vertex_buffer =
        [findMemoryType] (VkDevice device, std::size_t size, VkPhysicalDevice physicalDevice) -> std::pair<VkBuffer, VkDeviceMemory>
        {
         VkBuffer vertexBuffer;
         VkDeviceMemory vertexBufferMemory;

         VkBufferCreateInfo bufferInfo = {};
         bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
         bufferInfo.size = /*sizeof(vertices[0]) * vertices.size()*/size;
         bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
         bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

         if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
           throw std::runtime_error("failed to create vertex buffer!");
         }

         VkMemoryRequirements memRequirements;
         vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

         VkMemoryAllocateInfo allocInfo = {};
         allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
         allocInfo.allocationSize = memRequirements.size;
         allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);

         if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
           throw std::runtime_error("failed to allocate vertex buffer memory!");
         }
         vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

         return {vertexBuffer, vertexBufferMemory};
        };

      
      VkDeviceMemory vertexBufferMemory;
      std::tie(vertexBuffer, vertexBufferMemory) = create_vertex_buffer (output.device, sizeof(vertices) + sizeof(coordinates)
                                                                         + sizeof(transform_matrix)
                                                                         , output.physical_device);

      void* data;
      vkMapMemory(output.device, vertexBufferMemory, 0, sizeof(vertices) + sizeof(coordinates) + sizeof(transform_matrix), 0, &data);
    
      std::memcpy(data, vertices, sizeof(vertices));
      std::memcpy(&static_cast<char*>(data)[sizeof(vertices)], coordinates, sizeof(coordinates));
      std::memcpy(&static_cast<char*>(data)[sizeof(vertices) + sizeof(coordinates)], transform_matrix, sizeof(transform_matrix));

      vkUnmapMemory(output.device, vertexBufferMemory);
    }
    
    return {graphicsPipeline, pipelineLayout, output.renderpass, 6, 1, 0, 0, /*push_constants*/{}, {{0, vertexBuffer}, {0, vertexBuffer}}
            , descriptorSet, descriptorSetLayout};
  }
  
  // return {};
}

// template <typename Point, typename Color>
// vulkan_draw_info replace_push_constants (vulkan_draw_info& info, object::fill_triangle<Point, Color> const& triangle)
// {
//   // std::cout << "replace_push_constants" << std::endl;
//     std::array<float, 16> values
//       ({triangle.p1.x, triangle.p1.y, 0.0f, triangle.fill_color.r
//         , triangle.p2.x, triangle.p2.y, 0.0f, triangle.fill_color.g
//         , triangle.p3.x, triangle.p3.y, 0.0f, triangle.fill_color.b
//         , /*triangle.fill_color.a*/0.0f});
    
//     std::memcpy(info.push_constants.data(), values.data(), info.push_constants.size());
//     return info;
// }

} } }
  

#endif
