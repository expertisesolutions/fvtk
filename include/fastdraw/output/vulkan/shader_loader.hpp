///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_OUTPUT_VULKAN_SHADER_LOADER_HPP
#define FASTDRAW_OUTPUT_VULKAN_SHADER_LOADER_HPP

#include <fastdraw/output/vulkan/shaders.hpp>

#include <map>
#include <stdexcept>
#include <fstream>
#include <filesystem>

namespace fastdraw { namespace output { namespace vulkan {

enum class shader
{
  fill_solid_color_bind_frag,
  triangle_bind_vertex,
  image_vertex,
  image_frag
};

const char* name (shader s)
{
  switch (s)
  {
  case shader::fill_solid_color_bind_frag: return "fill_solid_color_bind.frag.spv";
  case shader::triangle_bind_vertex:       return "triangle_bind.vert.spv";
  case shader::image_vertex:               return "image.vert.spv";
  case shader::image_frag:                 return "image.frag.spv";
  default:                                 throw std::runtime_error ("Shader not found");
  }
}

VkShaderStageFlagBits stage_bits (shader s)
{
  switch (s)
  {
  case shader::fill_solid_color_bind_frag: return VK_SHADER_STAGE_FRAGMENT_BIT;
  case shader::triangle_bind_vertex:       return VK_SHADER_STAGE_VERTEX_BIT;
  case shader::image_vertex:               return VK_SHADER_STAGE_VERTEX_BIT;
  case shader::image_frag:                 return VK_SHADER_STAGE_FRAGMENT_BIT;
  default:                                 throw std::runtime_error ("Shader not found");
  };      
}

struct shader_loader
{
  shader_loader () = default;
  shader_loader (std::filesystem::path path
                 , VkDevice device)
    : path (path), device(device) {}

  VkPipelineShaderStageCreateInfo pipeline_stage (shader s) const
  {
    VkPipelineShaderStageCreateInfo stage_info = {};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = stage_bits (s);;
    stage_info.module = load (s);
    stage_info.pName = "main";

    return stage_info;
  }
  
  VkShaderModule load(shader s) const
  {
    auto iterator = map.find(s);
    if (iterator == map.end())
    {
      auto name = path / vulkan::name (s);
      std::ifstream file(name.c_str(), std::ios::ate | std::ios::binary);

      if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
      }
      size_t fileSize = (size_t) file.tellg();
      std::vector<char> buffer(fileSize);
      file.seekg(0);
      file.read(buffer.data(), fileSize);

      VkShaderModuleCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = buffer.size();
      createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
    
      VkShaderModule shaderModule;
      if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
      }

      map.insert(std::make_pair(s, shaderModule));
      return shaderModule;
    }
    else
      return iterator->second;
  }

  std::filesystem::path path;
  VkDevice device;
  mutable std::map<shader, VkShaderModule> map;
};

} } }

#endif
