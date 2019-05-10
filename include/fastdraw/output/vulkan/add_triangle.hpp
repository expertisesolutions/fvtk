///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_ADD_TRIANGLE_HPP
#define FASTDRAW_OUTPUT_VULKAN_ADD_TRIANGLE_HPP

#include <fastdraw/output/vulkan/vulkan_output_info.hpp>
#include <fastdraw/output/vulkan/vulkan_draw_info.hpp>
#include <fastdraw/object/triangle.hpp>

#include <vector>
#include <stdexcept>
#include <cstring>

namespace fastdraw { namespace output { namespace vulkan {

template <typename Point, typename Color, typename WindowingBase>
vulkan_draw_info create_output_specific_object (vulkan_output_info<WindowingBase>& output, object::fill_triangle<Point, Color> const& triangle
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
                                                  , VK_BLEND_FACTOR_ZERO                                   // dstColorBlendFactor
                                                  , VK_BLEND_OP_ADD                                        // colorBlendOp
                                                  , VK_BLEND_FACTOR_ONE                                    // srcAlphaBlendFactor
                                                  , VK_BLEND_FACTOR_ZERO                                   // dstAlphaBlendFactor
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
                                                   , {1.0f, 1.0f, 1.0f, 0.5f}                              // blendConstants
                                               }
                                               )
{
  std::cout << "add_triangle" << std::endl;
  if(colorBlending.attachmentCount == 1 && colorBlending.pAttachments == nullptr)
    colorBlending.pAttachments = &colorBlendAttachment;
  
  /***

      add specialization

  */

  // const float vertices[] = { triangle.p1.x, triangle.p1.y, 0.0f
  //                            , triangle.p2.x, triangle.p2.y, 0.0f
  //                            , triangle.p3.x, triangle.p3.y, 0.0f };
    
  //   const VkSpecializationMapEntry vertSpecializationEntries[] =
  //     {
  //      {
  //       0,                                    // constantID
  //       sizeof(float)*0,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       1,                                   // constantID
  //       sizeof(float)*1,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       2,                                   // constantID
  //       sizeof(float)*2,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       3,                                   // constantID
  //       sizeof(float)*3,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       4,                                   // constantID
  //       sizeof(float)*4,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       5,                                   // constantID
  //       sizeof(float)*5,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       6,                                   // constantID
  //       sizeof(float)*6,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       7,                                   // constantID
  //       sizeof(float)*7,  // offset
  //       sizeof(float)     // size
  //      },
  //      {
  //       8,                                   // constantID
  //       sizeof(float)*8,  // offset
  //       sizeof(float)     // size
  //      }
  //     };

  //   const VkSpecializationInfo vertShaderSpecializationInfo =
  //     {
  //      9,                                  // mapEntryCount
  //      vertSpecializationEntries,                            // pMapEntries
  //      sizeof(vertices),                       // dataSize
  //      &vertices[0],                              // pData
  //     };
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = output.vertShaderModule;
    vertShaderStageInfo.pName = "main";
    //vertShaderStageInfo.pSpecializationInfo = &vertShaderSpecializationInfo;

    // const VkSpecializationMapEntry fragSpecializationEntries[] =
    //   {
    //    {
    //     0,                                    // constantID
    //     sizeof(float)*0,  // offset
    //     sizeof(float)     // size
    //    },
    //    {
    //     1,                                   // constantID
    //     sizeof(float)*1,  // offset
    //     sizeof(float)     // size
    //    },
    //    {
    //     2,                                   // constantID
    //     sizeof(float)*2,  // offset
    //     sizeof(float)     // size
    //    },
    //    {
    //     3,                                   // constantID
    //     sizeof(float)*3,  // offset
    //     sizeof(float)     // size
    //    }
    //   };

    // const float color[4] = {triangle.fill_color.r, triangle.fill_color.g, triangle.fill_color.b, 0.5f};
    
    // const VkSpecializationInfo fragShaderSpecializationInfo =
    //   {
    //    4,                                  // mapEntryCount
    //    fragSpecializationEntries,                            // pMapEntries
    //    sizeof(color),                       // dataSize
    //    &color[0],                              // pData
    //   };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = output.fragShaderModule;
    fragShaderStageInfo.pName = "main";
    // fragShaderStageInfo.pSpecializationInfo = &fragShaderSpecializationInfo;

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(float)*3;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescription = {};
    attributeDescription.binding = 0;
    attributeDescription.location = 0;
    attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescription.offset = 0;

    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription; // Optional

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

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = output.swapChainExtent;    

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    // uboLayoutBinding.binding = 0;
    // uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // uboLayoutBinding.descriptorCount = 1;

    std::array<float, 16> values
      ({triangle.p1.x, triangle.p1.y, 0.0f, triangle.fill_color.r
        , triangle.p2.x, triangle.p2.y, 0.0f, triangle.fill_color.g
        , triangle.p3.x, triangle.p3.y, 0.0f, triangle.fill_color.b
        , /*triangle.fill_color.a*/0.0f});
    
    // VkPushConstantRange pushConstantRange;
    // pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    // pushConstantRange.offset = 0;
    // pushConstantRange.size = sizeof(float)*values.size();

    // std::vector<char> push_constants(sizeof(float)*values.size());

    // std::memcpy(push_constants.data(), values.data(), push_constants.size());

    // VkDescriptorSetLayout descriptorSetLayout;    
    VkPipelineLayout pipelineLayout;

    // VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    // layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // layoutInfo.bindingCount = 1;
    // layoutInfo.pBindings = &uboLayoutBinding;

    // if (vkCreateDescriptorSetLayout(output.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    //   throw std::runtime_error("failed to create descriptor set layout!");
    // }
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    //pipelineLayoutInfo.setLayoutCount = 1; // Optional
    //pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
    // pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
    // pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional

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

    return {graphicsPipeline, pipelineLayout, output.renderpass, 3, 1, 0, 0, /*push_constants*/{}};
}

template <typename Point, typename Color>
vulkan_draw_info replace_push_constants (vulkan_draw_info& info, object::fill_triangle<Point, Color> const& triangle)
{
  // std::cout << "replace_push_constants" << std::endl;
    std::array<float, 16> values
      ({triangle.p1.x, triangle.p1.y, 0.0f, triangle.fill_color.r
        , triangle.p2.x, triangle.p2.y, 0.0f, triangle.fill_color.g
        , triangle.p3.x, triangle.p3.y, 0.0f, triangle.fill_color.b
        , /*triangle.fill_color.a*/0.0f});
    
    std::memcpy(info.push_constants.data(), values.data(), info.push_constants.size());
    return info;
}

} } }
  

#endif
