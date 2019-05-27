///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_ERROR_HPP
#define FTK_FTK_UI_BACKEND_VULKAN_ERROR_HPP

#include <cstring>
#include <array>
#include <vector>
#include <optional>
#include <cassert>
#include <iostream>

namespace ftk { namespace ui { namespace backend {

enum class vulkan_error_code
{
 success = 0,
 not_ready = 1,
 timeout = 2,
 event_set = 3,
 event_reset = 4,
 incomplete = 5,
 suboptimal_khr = 1000001003,
 // errors
 error_out_of_host_memory = -1,
 error_out_of_device_memory = -2,
 error_initialization_failed = -3,
 error_device_lost = -4,
 error_memory_map_failed = -5,
 error_layer_not_present = -6,
 error_extension_not_present = -7,
 error_feature_not_present = -8,
 error_incompatible_driver = -9,
 error_too_many_objects = -10,
 error_format_not_supported = -11,
 error_fragmented_pool = -12,
 error_out_of_pool_memory = -1000069000,
 error_invalid_external_handle = -1000072003,
 error_surface_lost_khr = -1000000000,
 error_native_window_in_use_khr = -1000000001,
 error_out_of_date_khr = -1000001004,
 error_incompatible_display_khr = -1000003001,
 error_validation_failed_ext = -1000011001,
 error_invalid_shader_nv = -1000012000,
 error_invalid_drm_format_modifier_plane_layout_ext = -1000158000,
 error_fragmentation_ext = -1000161000,
 error_not_permitted_ext = -1000174001,
 error_invalid_device_address_ext = -1000244000,
 error_full_screen_exclusive_mode_lost_ext = -1000255000,
};

bool is_error (vulkan_error_code c)
{
  switch (c)
  {
  case vulkan_error_code::success:
  case vulkan_error_code::not_ready:
  case vulkan_error_code::timeout:
  case vulkan_error_code::event_set:
  case vulkan_error_code::event_reset:
  case vulkan_error_code::incomplete:
  case vulkan_error_code::suboptimal_khr:
    return true;
  default:
    return false;
  }
}

vulkan_error_code from_result (VkResult r)
{
  return static_cast<vulkan_error_code>(r);
}

} } }

namespace std {
template <>
struct is_error_code_enum< ::ftk::ui::backend::vulkan_error_code> : true_type {};
}

namespace ftk { namespace ui { namespace backend {

std::error_code make_error_code(vulkan_error_code ec)
{
  return {ec};
}

} } }

#endif
