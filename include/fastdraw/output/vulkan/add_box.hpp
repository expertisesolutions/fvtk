///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_ADD_BOX_HPP
#define FASTDRAW_OUTPUT_VULKAN_ADD_BOX_HPP

#include <fastdraw/output/vulkan/vulkan_output.hpp>
#include <fastdraw/object/box.hpp>
#include <fastdraw/coordinates.hpp>
#include <fastdraw/color.hpp>

#include <vector>
#include <stdexcept>

namespace fastdraw { namespace output { namespace vulkan {

template <typename Point, typename Color, typename WindowingBase>
vulkan_draw_info create_output_specific_object (vulkan_output_info<WindowingBase>& output, object::fill_box<Point, Color> const& box
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
  if(colorBlending.attachmentCount == 1 && colorBlending.pAttachments == nullptr)
    colorBlending.pAttachments = &colorBlendAttachment;

  auto vertex_shader = vulkan::shader::triangle_bind_vertex;
  auto fragment_shader = vulkan::shader::fill_solid_color_bind_frag;
  
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = output.shader_loader->pipeline_stage(vertex_shader);
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = output.shader_loader->pipeline_stage(fragment_shader);

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkVertexInputBindingDescription bindingDescriptions[2] = {{}, {}};
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(float)*2;
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  bindingDescriptions[1].binding = 1;
  bindingDescriptions[1].stride = sizeof(float)*4;
  bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
  
  VkVertexInputAttributeDescription attributeDescriptions[2] = {{}, {}};
  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescriptions[0].offset = 0;
  attributeDescriptions[1].binding = 1;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
  attributeDescriptions[1].offset = sizeof(float)*2*6;

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

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = output.swapChainExtent;    

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

    VkPipelineLayout pipelineLayout;
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

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

    typedef color::color_traits<Color> color_traits;
    typedef color::color_channel_traits<typename color_traits::channel> channel_traits;
    typedef color::color_channel_traits<typename color_traits::alpha_channel> alpha_channel_traits;

    auto whole_width = output.swapChainExtent.width;
    auto whole_height = output.swapChainExtent.height;

    auto r = channel_traits::ratio (color_traits::red(box.fill_color));
    auto g = channel_traits::ratio (color_traits::green(box.fill_color));
    auto b = channel_traits::ratio (color_traits::blue(box.fill_color));
    auto a = alpha_channel_traits::ratio (color_traits::alpha(box.fill_color));
    
    const float vertices_and_color[] =
      {     coordinates::ratio(box.p1.x, whole_width)             , coordinates::ratio(box.p1.y, whole_height)
          , coordinates::ratio(box.p1.x + box.size.x, whole_width), coordinates::ratio(box.p1.y, whole_height)              
          , coordinates::ratio(box.p1.x + box.size.x, whole_width), coordinates::ratio(box.p1.y + box.size.y, whole_height)
          , coordinates::ratio(box.p1.x + box.size.x, whole_width), coordinates::ratio(box.p1.y + box.size.y, whole_height)
          , coordinates::ratio(box.p1.x, whole_width)             , coordinates::ratio(box.p1.y + box.size.y, whole_height)
          , coordinates::ratio(box.p1.x, whole_width)             , coordinates::ratio(box.p1.y, whole_height)
          , r, g, b, a
          , r, g, b, a
    };

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    std::tie(vertexBuffer, vertexBufferMemory) = create_vertex_buffer (output.device, sizeof(vertices_and_color), output.physical_device);

    void* data;
    vkMapMemory(output.device, vertexBufferMemory, 0, sizeof(vertices_and_color), 0, &data);

    std::memcpy(data, vertices_and_color, sizeof(vertices_and_color));

    vkUnmapMemory(output.device, vertexBufferMemory);
    
    return {graphicsPipeline, pipelineLayout, output.renderpass, 6, 2, 0, 0, /*push_constants*/{}, {{0ul, vertexBuffer}, {0ul, vertexBuffer}}};
}

} } }
  

#endif
