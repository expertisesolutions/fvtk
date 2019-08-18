///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_UI_BACKEND_VULKAN_INDIRECT_DRAW_HPP
#define FTK_UI_BACKEND_VULKAN_INDIRECT_DRAW_HPP

namespace ftk { namespace ui { namespace vulkan {

template </*typename Point, */typename WindowingBase>
fastdraw::output::vulkan::vulkan_draw_info
create_indirect_draw_buffer_filler_pipeline
(fastdraw::output::vulkan::vulkan_output_info<WindowingBase>& output/*, object::image<Point> const& image*/
                                  )
{
  using fastdraw::output::vulkan::from_result;
  using fastdraw::output::vulkan::vulkan_error_code;

  VkPipelineRasterizationStateCreateInfo rasterizer
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
      };
  VkPipelineMultisampleStateCreateInfo multisampling
    = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO // sType
       , nullptr                                               // pNext
       , 0u                                                    // flags
       , VK_SAMPLE_COUNT_1_BIT                                 // rasterizationSamples
       , VK_FALSE                                              // sampleShadingEnable
       , 1.0f                                                  // minSampleShading
       , nullptr                                               // pSampleMask
       , VK_FALSE                                              // alphaToCoverageEnable
       , VK_FALSE                                              // alphaToOneEnable
      };
  VkPipelineColorBlendAttachmentState colorBlendAttachment
    = {VK_FALSE                                                 // blendEnable
       , VK_BLEND_FACTOR_ONE                                    // srcColorBlendFactor
       , VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA                    // dstColorBlendFactor
       , VK_BLEND_OP_ADD                                        // colorBlendOp
       , VK_BLEND_FACTOR_ONE                                    // srcAlphaBlendFactor
       , VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA                    // dstAlphaBlendFactor
       , VK_BLEND_OP_ADD                                        // alphaBlendOp
       // , VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT    // colorWriteMask
       // | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT    // colorWriteMask
       , 0
      };
  VkPipelineColorBlendStateCreateInfo colorBlending
    = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO // sType
       , nullptr                                               // pNext
       , 0u                                                    // flags
       , VK_FALSE                                              // logicOpEnable
       , VK_LOGIC_OP_COPY                                      // logicOp
       , 1                                                     // attachmentCount
       , &colorBlendAttachment                                 // pAttachments = &colorBlendAttachment 
       , {1.0f, 1.0f, 1.0f, 1.0f}                              // blendConstants
     };

  CHRONO_START()
  
  {
    constexpr const int tex_max_size = 4096;
    VkDescriptorSetLayoutBinding textureLayoutBinding = {};
    textureLayoutBinding.binding = 0;
    textureLayoutBinding.descriptorCount = tex_max_size;
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureLayoutBinding.pImmutableSamplers = nullptr;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding ssboLayoutBinding = {};
    ssboLayoutBinding.binding = 0;
    ssboLayoutBinding.descriptorCount = 1;
    ssboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding ssboZIndexLayoutBinding = {};
    ssboZIndexLayoutBinding.binding = 1;
    ssboZIndexLayoutBinding.descriptorCount = 1;
    ssboZIndexLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboZIndexLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutBinding indirectDrawLayoutBinding = {};
    indirectDrawLayoutBinding.binding = 2;
    indirectDrawLayoutBinding.descriptorCount = 1;
    indirectDrawLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indirectDrawLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding indirectDrawSharedStateLayoutBinding = {};
    indirectDrawSharedStateLayoutBinding.binding = 3;
    indirectDrawSharedStateLayoutBinding.descriptorCount = 1;
    indirectDrawSharedStateLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    indirectDrawSharedStateLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    
    std::array<VkDescriptorSetLayoutBinding, 1> texture_bindings = {textureLayoutBinding//, samplerLayoutBinding
                                                            /*, zindexPixelSboLayoutBinding
                                                              , zindexArraySboLayoutBinding*/};
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(texture_bindings.size());
    layoutInfo.pBindings = texture_bindings.data();
    // layoutInfo.bindingCount = 1; //static_cast<uint32_t>(bindings.size());
    // layoutInfo.pBindings = /*bindings.data()*/&samplerLayoutBinding;
    //layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    
  CHRONO_COMPARE()
    VkDescriptorSetLayout texture_layout_set;
    auto r = from_result(vkCreateDescriptorSetLayout(output.device, &layoutInfo, nullptr, &texture_layout_set));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code(r));

    std::array<VkDescriptorSetLayoutBinding, 1> sampler_bindings = {samplerLayoutBinding
                                                            /*, zindexPixelSboLayoutBinding
                                                              , zindexArraySboLayoutBinding*/};
    VkDescriptorSetLayoutCreateInfo samplerLayoutInfo = {};
    samplerLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    samplerLayoutInfo.bindingCount = static_cast<uint32_t>(sampler_bindings.size());
    samplerLayoutInfo.pBindings = sampler_bindings.data();
    // samplerLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    
    VkDescriptorSetLayout sampler_layout_set;
    r = from_result(vkCreateDescriptorSetLayout(output.device, &samplerLayoutInfo, nullptr, &sampler_layout_set));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code(r));

    std::array<VkDescriptorSetLayoutBinding, 4> ssbo_bindings = {ssboLayoutBinding
                                                                 , ssboZIndexLayoutBinding
                                                                 , indirectDrawLayoutBinding
                                                                 , indirectDrawSharedStateLayoutBinding
                                                            /*, zindexPixelSboLayoutBinding
                                                              , zindexArraySboLayoutBinding*/};
    VkDescriptorSetLayoutCreateInfo ssboLayoutInfo = {};
    ssboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ssboLayoutInfo.bindingCount = static_cast<uint32_t>(ssbo_bindings.size());
    ssboLayoutInfo.pBindings = ssbo_bindings.data();
    ssboLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    
    VkDescriptorSetLayout ssbo_layout_set;
    r = from_result(vkCreateDescriptorSetLayout(output.device, &ssboLayoutInfo, nullptr, &ssbo_layout_set));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code(r));

    // ////

    // std::array<VkDescriptorSetLayoutBinding, 2> indirect_draw_bindings
    //   = {
    //     };
    // VkDescriptorSetLayoutCreateInfo indirectDrawLayoutInfo = {};
    // indirectDrawLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // indirectDrawLayoutInfo.bindingCount = static_cast<uint32_t>(indirect_draw_bindings.size());
    // indirectDrawLayoutInfo.pBindings = indirect_draw_bindings.data();
    // indirectDrawLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    
    // VkDescriptorSetLayout indirect_draw_layout_set;
    // r = from_result(vkCreateDescriptorSetLayout(output.device, &indirectDrawLayoutInfo, nullptr
    //                                             , &indirect_draw_layout_set));
    // if (r != vulkan_error_code::success)
    //   throw std::system_error(make_error_code(r));
    
    VkVertexInputBindingDescription bindingDescriptions[4] = {};
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(float)*4;
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bindingDescriptions[1].binding = 1;
    bindingDescriptions[1].stride = sizeof(float)*2;
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    bindingDescriptions[2].binding = 2;
    bindingDescriptions[2].stride = 0;
    bindingDescriptions[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    bindingDescriptions[3].binding = 3;
    bindingDescriptions[3].stride = 0;
    bindingDescriptions[3].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    
    VkVertexInputAttributeDescription attributeDescriptions[7] = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    attributeDescriptions[1].binding = 1;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float)*24;

    attributeDescriptions[2].binding = 2;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = sizeof(float)*24 + sizeof(float)*12;

    attributeDescriptions[3].binding = 2;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[3].offset = sizeof(float)*24 + sizeof(float)*12 + sizeof(float)*4;

    attributeDescriptions[4].binding = 2;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = sizeof(float)*24 + sizeof(float)*12 + sizeof(float)*4*2;

    attributeDescriptions[5].binding = 2;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = sizeof(float)*24 + sizeof(float)*12 + sizeof(float)*4*3;

    attributeDescriptions[6].binding = 3;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[6].offset = sizeof(float)*24 + sizeof(float)*12 + sizeof(float)*4*4;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = sizeof(bindingDescriptions)/sizeof(bindingDescriptions[0]);
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = sizeof(attributeDescriptions)/sizeof(attributeDescriptions[0]);
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions; // Optional
    
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
    CHRONO_COMPARE()
    vertShaderStageInfo.module = output.shader_loader->load(fastdraw::output::vulkan::shader::indirect_draw_vertex);
    CHRONO_COMPARE()
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    CHRONO_COMPARE()
    fragShaderStageInfo.module = output.shader_loader->load(fastdraw::output::vulkan::shader::indirect_draw_frag);
    CHRONO_COMPARE()
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    VkPipelineLayout pipelineLayout;

    // std::size_t desc_layout_size = 1;
    // std::unique_ptr<VkDescriptorSetLayout[]> descriptor_set_layouts{new VkDescriptorSetLayout[desc_layout_size]};
    // for(auto first = &descriptor_set_layouts[0], last = first + desc_layout_size; first != last; ++first)
    //   *first = descriptorSetLayout;          

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineLayoutInfo.setLayoutCount = desc_layout_size; // Optional
    // pipelineLayoutInfo.pSetLayouts = &descriptor_set_layouts[0]; // Optional
    std::array<VkDescriptorSetLayout, 3> descriptor_sets {texture_layout_set
                                                          , sampler_layout_set
                                                          , ssbo_layout_set
                                                          /*, indirect_draw_layout_set*/};
    pipelineLayoutInfo.setLayoutCount = descriptor_sets.size(); // Optional
    pipelineLayoutInfo.pSetLayouts = &descriptor_sets[0]; // Optional
  CHRONO_COMPARE()
    r = from_result(vkCreatePipelineLayout(output.device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code(r));
  CHRONO_COMPARE()

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    //depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
    //depthStencil.depthBoundsTestEnable = VK_FALSE;
    //depthStencil.minDepthBounds = 0.0f; // Optional
    //depthStencil.maxDepthBounds = 1.0f; // Optional
    //depthStencil.stencilTestEnable = VK_FALSE;
    //depthStencil.front = {}; // Optional
    //depthStencil.back = {}; // Optional

    VkDynamicState scissorDynamicState = VK_DYNAMIC_STATE_SCISSOR;
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 1;
    dynamicState.pDynamicStates = &scissorDynamicState;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState; // Optional
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = output.renderpass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VkPipeline graphicsPipeline;
  CHRONO_COMPARE()
    r = from_result(vkCreateGraphicsPipelines(output.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline));
    if (r != vulkan_error_code::success)
      throw std::system_error(make_error_code(r));
  CHRONO_COMPARE()

  //   VkBuffer vertexBuffer;
  //   {
  //     auto whole_width = output.swapChainExtent.height;
  //     auto whole_height = output.swapChainExtent.width;
  //     // auto whole_width = output.swapChainExtent.width;
  //     // auto whole_height = output.swapChainExtent.height;

  //     auto scale = 4;
      
  //     const float vertices[] =
  //       {     coordinates::ratio(image.pos.x, whole_width)              , coordinates::ratio(image.pos.y, whole_height)
  //           , coordinates::ratio(image.pos.x + image.size.x*scale, whole_width), coordinates::ratio(image.pos.y, whole_height)              
  //           , coordinates::ratio(image.pos.x + image.size.x*scale, whole_width), coordinates::ratio(image.pos.y + image.size.y*scale, whole_height)
  //           , coordinates::ratio(image.pos.x + image.size.x*scale, whole_width), coordinates::ratio(image.pos.y + image.size.y*scale, whole_height)
  //           , coordinates::ratio(image.pos.x, whole_width)              , coordinates::ratio(image.pos.y + image.size.y*scale, whole_height)
  //           , coordinates::ratio(image.pos.x, whole_width)              , coordinates::ratio(image.pos.y, whole_height)
  //       };
  //     const float coordinates[] =
  //       {   0.0f, 0.0f
  //           , 1.0f, 0.0f
  //           , 1.0f, 1.0f
  //           , 1.0f, 1.0f
  //           , 0.0f, 1.0f
  //           , 0.0f, 0.0f
  //       };
  //     const float transform_matrix[] =
  //       {
  //           1.0f, 1.0f, 1.0f, 1.0f
  //         , 1.0f, 1.0f, 1.0f, 1.0f
  //         , 1.0f, 1.0f, 1.0f, 1.0f
  //         , 1.0f, 1.0f, 1.0f, 1.0f
  //         //   1.0f, 0.0f, 0.0f, 0.0f
  //         // , 0.0f, 1.0f, 0.0f, 0.0f
  //         // , 0.0f, 0.0f, 1.0f, 0.0f
  //         // , 0.0f, 0.0f, 0.0f, 1.0f
  //       };

  //     for (int i = 0; i != 12; i += 2)
  //     {        
  //       std::cout << "x: " << vertices[i] << " y: " << vertices[i+1] << std::endl;
  //     }

  //     auto findMemoryType =
  //       [] (uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice) -> uint32_t
  //       { 
  //         VkPhysicalDeviceMemoryProperties memProperties;
  //         vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  //         for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
  //           if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
  //             return i;
  //           }
  //         }
  
  //         throw std::runtime_error("failed to find suitable memory type!");
  //       };


  //     auto create_vertex_buffer =
  //       [findMemoryType] (VkDevice device, std::size_t size, VkPhysicalDevice physicalDevice) -> std::pair<VkBuffer, VkDeviceMemory>
  //       {
  //        VkBuffer vertexBuffer;
  //        VkDeviceMemory vertexBufferMemory;

  //        VkBufferCreateInfo bufferInfo = {};
  //        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  //        bufferInfo.size = /*sizeof(vertices[0]) * vertices.size()*/size;
  //        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  //        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  //        if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
  //          throw std::runtime_error("failed to create vertex buffer!");
  //        }

  //        VkMemoryRequirements memRequirements;
  //        vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

  //        VkMemoryAllocateInfo allocInfo = {};
  //        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  //        allocInfo.allocationSize = memRequirements.size;
  //        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, physicalDevice);

  //        if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
  //          throw std::runtime_error("failed to allocate vertex buffer memory!");
  //        }
  //        vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

  //        return {vertexBuffer, vertexBufferMemory};
  //       };

      
  //     VkDeviceMemory vertexBufferMemory;
  //     std::tie(vertexBuffer, vertexBufferMemory) = create_vertex_buffer (output.device, sizeof(vertices) + sizeof(coordinates)
  //                                                                        + sizeof(transform_matrix)
  //                                                                        , output.physical_device);

  // CHRONO_COMPARE()
  //     void* data;
  //     vkMapMemory(output.device, vertexBufferMemory, 0, sizeof(vertices) + sizeof(coordinates) + sizeof(transform_matrix), 0, &data);
    
  // CHRONO_COMPARE()
  //     std::memcpy(data, vertices, sizeof(vertices));
  //     std::memcpy(&static_cast<char*>(data)[sizeof(vertices)], coordinates, sizeof(coordinates));
  //     std::memcpy(&static_cast<char*>(data)[sizeof(vertices) + sizeof(coordinates)], transform_matrix, sizeof(transform_matrix));

  // CHRONO_COMPARE()
  //     vkUnmapMemory(output.device, vertexBufferMemory);
  // CHRONO_COMPARE()
  //   }
    
  CHRONO_COMPARE()
    // return {graphicsPipeline, pipelineLayout, output.renderpass, 6, 1, 0, 0, /*push_constants*/{}, {{0, vertexBuffer}, {0, vertexBuffer}}
    //         , descriptorSet, descriptorSetLayout};
    return {graphicsPipeline, pipelineLayout, output.renderpass, 6, 1, 0, 0, /*push_constants*/{}, {}
            , {}/*descriptorSet*/, {texture_layout_set, sampler_layout_set, ssbo_layout_set}};
  }
  
  // return {};
}

} } }

#endif
