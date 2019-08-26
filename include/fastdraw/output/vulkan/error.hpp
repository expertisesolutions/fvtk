///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAW_FASTDRAW_OUTPUT_VULKAN_ERROR_HPP
#define FASTDRAW_FASTDRAW_OUTPUT_VULKAN_ERROR_HPP

#include <cstring>
#include <array>
#include <vector>
#include <optional>
#include <cassert>
#include <iostream>

#include <vulkan/vulkan.h>

namespace fastdraw { namespace output { namespace vulkan {

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
    return false;
  default:
    return true;
  }
}

vulkan_error_code from_result (VkResult r)
{
  return static_cast<vulkan_error_code>(r);
}

struct vulkan_category : std::error_category
{
  const char* name () const noexcept { return "vulkan_category"; }
  std::string message (int condition) const
  {
    switch (condition)
    {
  case VK_SUCCESS: return "Command successfully completed";
  case VK_NOT_READY: return "A fence or query has not yet completed";
  case VK_TIMEOUT: return "A wait operation has not completed in the specified time";
  case VK_EVENT_SET: return "An event is signaled";
  case VK_EVENT_RESET: return "An event is unsignaled";
  case VK_INCOMPLETE: return "A return array was too small for the result";
  case VK_SUBOPTIMAL_KHR: return "A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
  case VK_ERROR_OUT_OF_HOST_MEMORY: return "A host memory allocation has failed.";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "A device memory allocation has failed.";
  case VK_ERROR_INITIALIZATION_FAILED: return "Initialization of an object could not be completed for implementation-specific reasons.";
  case VK_ERROR_DEVICE_LOST: return "The logical or physical device has been lost. See Lost Device";
  case VK_ERROR_MEMORY_MAP_FAILED: return "Mapping of a memory object has failed.";
  case VK_ERROR_LAYER_NOT_PRESENT: return "A requested layer is not present or could not be loaded.";
  case VK_ERROR_EXTENSION_NOT_PRESENT: return "A requested extension is not supported.";
  case VK_ERROR_FEATURE_NOT_PRESENT: return "A requested feature is not supported.";
  case VK_ERROR_INCOMPATIBLE_DRIVER: return "The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
  case VK_ERROR_TOO_MANY_OBJECTS: return "Too many objects of the type have already been created.";
  case VK_ERROR_FORMAT_NOT_SUPPORTED: return "A requested format is not supported on this device.";
  case VK_ERROR_FRAGMENTED_POOL: return "A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
  case VK_ERROR_SURFACE_LOST_KHR: return "A surface is no longer available.";
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
  case VK_ERROR_OUT_OF_DATE_KHR: return "A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
  case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
  case VK_ERROR_INVALID_SHADER_NV: return "One or more shaders failed to compile or link. More details are reported back to the application via https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VK_EXT_debug_report if enabled.";
  case VK_ERROR_OUT_OF_POOL_MEMORY: return "A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.";
  case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "An external handle is not a valid handle of the specified type.";
  case VK_ERROR_FRAGMENTATION_EXT: return "A descriptor pool creation has failed due to fragmentation.";
  case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: return "A buffer creation failed because the requested address is not available.";
  case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.";
    default: return "Unknown";
    }
  }
};

inline std::error_category const& get_vulkan_category() { static vulkan_category const category; return category; }

} } }

namespace std {
template <>
struct is_error_code_enum< ::fastdraw::output::vulkan::vulkan_error_code> : true_type {};
}

namespace fastdraw { namespace output { namespace vulkan {

std::error_code make_error_code(vulkan_error_code ec)
{
  return {(int)ec, get_vulkan_category()};
}

} } }

#endif
