///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_ADD_TEXT_HPP
#define FASTDRAW_OUTPUT_VULKAN_ADD_TEXT_HPP

#include <fastdraw/output/vulkan/vulkan_output_info.hpp>
#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>
#include <fastdraw/output/vulkan/buffer.hpp>
#include <fastdraw/object/text.hpp>
#include <fastdraw/coordinates.hpp>
#include <fastdraw/color.hpp>

#include <vector>
#include <stdexcept>
#include <cstring>

#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H /* Freetype2 OS/2 font table. */

namespace fastdraw { namespace output { namespace vulkan {

template <typename Point, typename String, typename Color, typename WindowingBase>
vulkan_draw_info create_output_specific_object (vulkan_output_info<WindowingBase>& output, object::fill_text<Point, String, Color> const& text
                                               , VkPipelineRasterizationStateCreateInfo rasterizer
                                               = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO // sType
                                                  , nullptr                                                  // pNext
                                                  , 0u                                                       // flags
                                                  , VK_FALSE                                                 // depthClampEnable
                                                  , VK_FALSE                                                 // rasterizerDiscardEnable
                                                  , VK_POLYGON_MODE_FILL                                     // polygonMode
                                                  , VK_CULL_MODE_BACK_BIT                                    // cullMode
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

  static FT_Library  library;
  static bool library_initialized;

  if (!library_initialized)
  {
    auto error = FT_Init_FreeType( &library );
    if (error)
    {
      throw -1;
    }
  }

  FT_Face face;
  auto error = FT_New_Face (library, text.face.c_str()
                            , 0, &face);
  if (error)
  {
    std::cout << "error " << error << std::endl;
    throw float{};
  }

  auto whole_width = output.swapChainExtent.width;
  auto whole_height = output.swapChainExtent.height;

  auto texture_width = coordinates::proportion(text.size.x, whole_width);
  auto texture_height = coordinates::proportion(text.size.y, whole_height);
  unsigned int pen_y = (texture_height / 3) * 2;

  std::cout << "width " << texture_width << " height " << texture_height << " pen_y " << pen_y << std::endl;

  auto fixup = texture_height / 10;

  // should do binary search
  do
  {
    if (object::text_scale const* scale = std::get_if<object::text_scale>(&text.size_information))
      error = FT_Set_Pixel_Sizes(face, 0, texture_height - fixup); /* set character size */
    else
      error = FT_Set_Char_Size (face, 20 << 6, 0, 300, 0);
    if (error)
      throw uint32_t{};

    std::cout << "Face height " << (face->size->metrics.height >> 6) << std::endl;

    fixup += texture_height / 20;
  }
  while ( (face->size->metrics.height >> 6) > texture_height - 2);

  // hb_face_t* hb_face = ::hb_ft_face_create (face, nullptr);
  // if (!hb_face)
  //   throw -1;

  hb_font_t* hb_font = ::hb_ft_font_create (face, nullptr);
  if (!hb_font)
    throw long{};

  hb_buffer_t* buffer;
  buffer = hb_buffer_create ();

  hb_buffer_add_utf8 (buffer,
                      text.text.c_str(),
                      text.text.size(),
                      0,
                      text.text.size());
  hb_buffer_set_direction(buffer, HB_DIRECTION_LTR);

  hb_feature_t feature;
  // The leak most probably doesn't depend on the type of the feature.
  feature.tag = hb_tag_from_string("kern", 4);
  feature.value = 0;
  feature.start = 0;
  feature.end = (unsigned int) -1;
  //int num_features = 1;
  hb_shape(hb_font, buffer, &feature, 1);


  int glyph_count = hb_buffer_get_length(buffer);
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buffer, 0);

  std::cout << "glyph count " << glyph_count << std::endl;

  if (FT_HAS_KERNING (face))
  {
    std::cout << "has kerning" << std::endl;
  }
  
  int pen_x = 0;

  ///
  // calculate the optimal texture size
  
  auto size = texture_width * texture_height * sizeof(float);

  std::cout << "texture size w: " << texture_width << " h: " << texture_height << std::endl;

  auto staging_pair = vulkan::create_buffer(output.device, size, output.physical_device, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  void* data;
  vkMapMemory(output.device, staging_pair.second, 0, size, 0, &data);

  std::fill (static_cast<uint32_t*>(data), static_cast<uint32_t*>(data) + texture_width * texture_height
             , 0x00000000);

  for (int i = 0; i != glyph_count; ++i) 
  {
    FT_UInt glyph_index = glyph_info[i].codepoint;
    FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    std::cout << "glyph index " << glyph_index << std::endl;
    
    /* convert to an anti-aliased bitmap */
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    FT_Bitmap bitmap = face->glyph->bitmap;
    std::cout << "size " << bitmap.width << 'x' << bitmap.rows << std::endl;
    std::cout << "advance " << (face->glyph->advance.x >> 6)<< std::endl;
    std::cout << "bitmap type " << (int)bitmap.pixel_mode << " bitmap_left " << face->glyph->bitmap_left
              << " bitmap top " << face->glyph->bitmap_top << std::endl;
    
    {
      int di = (pen_y - face->glyph->bitmap_top)*texture_width*sizeof(float) + pen_x*sizeof(float) + face->glyph->bitmap_left*sizeof(float);
      std::cout << "di " << di << std::endl;
      uint8_t* current = bitmap.buffer;
      for (decltype(bitmap.rows) j = 0; j != bitmap.rows; ++j)
      {
        for (auto i = static_cast<decltype(bitmap.width)>(0); i != bitmap.width; ++i)
        {
          typedef color::color_traits<Color> color_traits;
          typedef typename color_traits::channel color_channel;
          typedef color::color_channel_traits<color_channel> color_channel_traits;
          typedef color::color_channel_traits<uint8_t> dst_color_channel_traits;

          auto pcolor = color_traits::to_premultiplied_alpha(text.fill_color);
          color::color_premultiplied_rgba<uint8_t> current_color;

          current_color = color::apply_occlusion(pcolor, current[i]);

          // little or big?
          char* data_ = static_cast<char*>(data);
          std::memcpy (&data_[di + i * 4], &current_color, sizeof(uint32_t));
        }

        current += bitmap.pitch;
        di += texture_width*sizeof(float);
      }
    }
  
    pen_x += face->glyph->advance.x >> 6 /* 26.6 FF */;
    if (pen_x + (face->size->metrics.max_advance >> 6) >= texture_width)
      break;
  }

  vkUnmapMemory(output.device, staging_pair.second);

  VkImage textureImage;
  VkDeviceMemory textureImageMemory;
  
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(texture_width);
  imageInfo.extent.height = static_cast<uint32_t>(texture_height);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  //imageInfo.flags = 0; // Optional

  if (vkCreateImage(output.device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(output.device, textureImage, &memRequirements);

  {
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vulkan::find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                                         , output.physical_device);

    if (vkAllocateMemory(output.device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate image memory!");
    }
  }

  vkBindImageMemory(output.device, textureImage, textureImageMemory, 0);

  VkCommandBuffer commandBuffer;
  {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = output.command_pool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(output.device, &allocInfo, &commandBuffer);
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  // VkBufferCopy copyRegion = {};
  // copyRegion.size = size;
  // vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = textureImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  barrier.srcAccessMask = 0; // TODO
  barrier.dstAccessMask = 0; // TODO

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT /* TODO */, VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {
                        texture_width,
                        texture_height,
                        1
  };
  
  vkCmdCopyBufferToImage(
                         commandBuffer,
                         staging_pair.first,
                         textureImage,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1,
                         &region
                         );  

  barrier.oldLayout = /*oldLayout*/ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = /*newLayout*/ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT /* TODO */, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /* TODO */,
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
  );


  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  
  auto submit_error = vkQueueSubmit(output.graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(output.graphics_queue);

  if (submit_error)
    throw -1;

  vkFreeCommandBuffers(output.device, output.command_pool, 1, &commandBuffer);
  vkDestroyBuffer(output.device, staging_pair.first, nullptr);
  vkFreeMemory(output.device, staging_pair.second, nullptr);

  // image view
  VkImageView textureImageView;
  VkSampler textureSampler;

  {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
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

    if (vkCreateSampler(output.device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture sampler!");
    }
    
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
    bindingDescriptions[1].stride = sizeof(float)*2;
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributeDescriptions[2] = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float)*12;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
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
    vertShaderStageInfo.module = output.shader_loader->load(shader::image_vertex);
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = output.shader_loader->load(shader::image_frag);
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
      const float vertices[] =
        {     coordinates::ratio(text.p1.x, whole_width)              , coordinates::ratio(text.p1.y, whole_height)
            , coordinates::ratio(text.p1.x + text.size.x, whole_width), coordinates::ratio(text.p1.y, whole_height)              
            , coordinates::ratio(text.p1.x + text.size.x, whole_width), coordinates::ratio(text.p1.y + text.size.y, whole_height)
            , coordinates::ratio(text.p1.x + text.size.x, whole_width), coordinates::ratio(text.p1.y + text.size.y, whole_height)
            , coordinates::ratio(text.p1.x, whole_width)              , coordinates::ratio(text.p1.y + text.size.y, whole_height)
            , coordinates::ratio(text.p1.x, whole_width)              , coordinates::ratio(text.p1.y, whole_height)
        };
      const float coordinates[] =
        {   0.0f, 0.0f
            , 1.0f, 0.0f
            , 1.0f, 1.0f
            , 1.0f, 1.0f
            , 0.0f, 1.0f
            , 0.0f, 0.0f
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
      std::tie(vertexBuffer, vertexBufferMemory) = create_vertex_buffer (output.device, sizeof(vertices) + sizeof(coordinates), output.physical_device);

      void* data;
      vkMapMemory(output.device, vertexBufferMemory, 0, sizeof(vertices), 0, &data);
    
      std::memcpy(data, vertices, sizeof(vertices));
      std::memcpy(&static_cast<char*>(data)[sizeof(vertices)], coordinates, sizeof(coordinates));

      vkUnmapMemory(output.device, vertexBufferMemory);
    }
    
    return {graphicsPipeline, pipelineLayout, output.renderpass, 6, 2, 0, 0, /*push_constants*/{}, {{0, vertexBuffer}}
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
