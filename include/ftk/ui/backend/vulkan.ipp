///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FTK_FTK_UI_BACKEND_VULKAN_IPP
#define FTK_FTK_UI_BACKEND_VULKAN_IPP

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_core.h>

#include <ftk/ui/backend/vulkan_queues.hpp>
#include <ftk/ui/backend/khr_display.hpp>
#include <ftk/ui/backend/xlib_surface.hpp>

namespace ftk { namespace ui { namespace backend {

std::vector<unsigned int> get_graphics_family (VkPhysicalDevice physical_device)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  std::vector<unsigned int> graphic_families;
    
  unsigned int i = 0;
  for (const auto& queue_family : queue_families)
  {
    if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      graphic_families.push_back(i);
    ++i;
  }
  return graphic_families;
}

std::vector<unsigned int> get_presentation_family (VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

  std::vector<unsigned int> highest_priority_presentation_families;
  std::vector<unsigned int> lower_priority_presentation_families;
  unsigned int i = 0;
  for (const auto& queue_family : queue_families)
  {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentSupport);
    if (queue_family.queueCount > 0 && presentSupport)
    {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) // let give priority to non graphic family
        lower_priority_presentation_families.push_back(i);
      else
        highest_priority_presentation_families.push_back(i);
    }
    ++i;
  }

  std::vector<unsigned int> presentation_families (highest_priority_presentation_families.begin()
                                                   , highest_priority_presentation_families.end());
  std::copy (lower_priority_presentation_families.begin()
             , lower_priority_presentation_families.end()
             , std::back_inserter(presentation_families));
  return presentation_families;
}

template <typename Loop>
typename xlib_surface<Loop>::window xlib_surface<Loop>::create_window(int width, int height) const
{
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    window w;
    static_cast<base::window&>(w) = base::create_window (width, height);

    {
      uint32_t count;
      vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
      std::vector<VkExtensionProperties> extensions(count);
      vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
      for (auto & extension : extensions) {
        std::cout << "extension: " << extension.extensionName << std::endl;
      }
    }
    {
      VkApplicationInfo ApplicationInfo;
      std::memset(&ApplicationInfo, 0, sizeof(ApplicationInfo));
      ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      ApplicationInfo.pNext = nullptr;
      ApplicationInfo.pApplicationName = "My Application";
      ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.pEngineName = "My Engine";
      ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

      std::array<const char*, 2> extensions({"VK_KHR_surface", "VK_KHR_xlib_surface"});
  
      VkInstanceCreateInfo cinfo;

      std::memset(&cinfo, 0, sizeof(cinfo));

      std::array<const char*, 1> layers({"VK_LAYER_KHRONOS_validation"});
  
      cinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      cinfo.pNext = NULL;
      cinfo.flags = 0;
      cinfo.pApplicationInfo = &ApplicationInfo;
      cinfo.enabledLayerCount = layers.size();
      cinfo.ppEnabledLayerNames = &layers[0];
      cinfo.enabledExtensionCount = extensions.size();;
      cinfo.ppEnabledExtensionNames = &extensions[0];

      vulkan_error_code r = from_result(vkCreateInstance(&cinfo, NULL, &w.instance));
      if (r != vulkan_error_code::success)
      {
        std::cout << "error? " << static_cast<int>(r) << std::endl;
        std::cout << "error? " << make_error_code(r).message() << std::endl;
        throw std::system_error (make_error_code (r));
      }
      std::cout << "VkCreateInstance OK" << std::endl;

      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, nullptr);

      assert(deviceCount != 0);

      std::cout << "There are " << deviceCount << " physical devices" << std::endl;
      
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, devices.data());

      w.physicalDevice = devices[0];

      VkPhysicalDeviceProperties physical_properties;
      vkGetPhysicalDeviceProperties (w.physicalDevice, &physical_properties);

      std::cout << "Device name "  << physical_properties.deviceName << std::endl;

      {
        std::cout << "get device extensions" << std::endl;
        std::uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, NULL);
        std::vector<VkExtensionProperties> properties(count);
        if (!properties.empty())
        {
          vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, &properties[0]);
          for (auto&& property : properties)
          {
            std::cout << "device extension: " << property.extensionName << std::endl;
          }
        }
      }
      
      VkXlibSurfaceCreateInfoKHR info = {};
      std::memset(&info, 0, sizeof(info));
      info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
      info.dpy = w.x11_display;
      info.window = w.x11_window;

      r = from_result (vkCreateXlibSurfaceKHR(w.instance, &info, NULL, &w.surface));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));
      }

    return w;
}
      
khr_display::window khr_display::create_window(int width, int height) const
{
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;
    window w;
    {
      uint32_t count;
      vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr); //get number of extensions
      std::vector<VkExtensionProperties> extensions(count);
      vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()); //populate buffer
      for (auto & extension : extensions) {
        std::cout << "extension: " << extension.extensionName << std::endl;
      }
    }
    {
      VkApplicationInfo ApplicationInfo;
      std::memset(&ApplicationInfo, 0, sizeof(ApplicationInfo));
      ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      ApplicationInfo.pNext = nullptr;
      ApplicationInfo.pApplicationName = "My Application";
      ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.pEngineName = "My Engine";
      ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
      ApplicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

      std::array<const char*, 2> extensions({"VK_KHR_display", "VK_KHR_surface",/* "VK_KHR_get_display_properties2", "VK_KHR_get_physical_device_properties2", "VK_KHR_external_memory_capabilities", "VK_EXT_direct_mode_display", "VK_KHR_get_surface_capabilities2", "VK_KHR_external_fence_capabilities", "VK_KHR_external_semaphore_capabilities", "VK_KHR_device_group_creation", "VK_KHR_surface_protected_capabilities", "VK_EXT_display_surface_counter", "VK_EXT_debug_report", "VK_EXT_debug_utils"*/});
  
      VkInstanceCreateInfo cinfo;

      std::memset(&cinfo, 0, sizeof(cinfo));

      std::array<const char*, 1> layers({"VK_LAYER_KHRONOS_validation"});
  
      cinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      cinfo.pNext = NULL;
      cinfo.flags = 0;
      cinfo.pApplicationInfo = &ApplicationInfo;
      cinfo.enabledLayerCount = layers.size();
      cinfo.ppEnabledLayerNames = &layers[0];
      cinfo.enabledExtensionCount = extensions.size();;
      cinfo.ppEnabledExtensionNames = &extensions[0];

      vulkan_error_code r = from_result(vkCreateInstance(&cinfo, NULL, &w.instance));
      if (r != vulkan_error_code::success)
      {
        std::cout << "error? " << static_cast<int>(r) << std::endl;
        std::cout << "error? " << make_error_code(r).message() << std::endl;
        throw std::system_error (make_error_code (r));
      }
      std::cout << "VkCreateInstance OK" << std::endl;

      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, nullptr);

      assert(deviceCount != 0);

      std::cout << "There are " << deviceCount << " physical devices" << std::endl;
      
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(w.instance, &deviceCount, devices.data());

      w.physicalDevice = devices[0];

      {
        std::cout << "get device extensions" << std::endl;
        std::uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, NULL);
        std::vector<VkExtensionProperties> properties(count);
        if (!properties.empty())
        {
          vkEnumerateDeviceExtensionProperties (w.physicalDevice, NULL, &count, &properties[0]);
          for (auto&& property : properties)
          {
            std::cout << "device extension: " << property.extensionName << std::endl;
          }
        }
      }
      
      VkDisplayPropertiesKHR* display_properties = nullptr;
      {
        uint32_t count = 0;
        r = from_result(vkGetPhysicalDeviceDisplayPropertiesKHR (w.physicalDevice, &count, nullptr));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        std::cout << "count " << count << std::endl;

        display_properties = new VkDisplayPropertiesKHR[count];

        r = from_result(vkGetPhysicalDeviceDisplayPropertiesKHR (w.physicalDevice, &count, display_properties));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        for (unsigned int i = 0; i != count; ++i )
        {
          std::cout << "display name " << display_properties[i].displayName << std::endl;
        }
        std::cout << "transform supported " << (int)display_properties[0].supportedTransforms << std::endl;
        
        assert (count != 0);
      }

      VkDisplayModePropertiesKHR* display_mode_properties = nullptr;
      {
        uint32_t count = 0;        
        r = from_result(vkGetDisplayModePropertiesKHR (w.physicalDevice, display_properties[0].display, &count, nullptr));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        display_mode_properties = new VkDisplayModePropertiesKHR[count];
        std::cout << "display modes " << count << std::endl;

        r = from_result(vkGetDisplayModePropertiesKHR (w.physicalDevice, display_properties[0].display, &count, display_mode_properties));
        if (r != vulkan_error_code::success)
          throw std::system_error (make_error_code (r));

        assert(count != 0);
      }

      VkDisplaySurfaceCreateInfoKHR info = {};

      info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
      info.displayMode = display_mode_properties[0].displayMode;
      info.imageExtent = display_mode_properties[0].parameters.visibleRegion;
      info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
      // info.transform = VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR;
      info.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
      
      r = from_result (vkCreateDisplayPlaneSurfaceKHR(w.instance, &info, NULL, &w.surface));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));
    }
    return w;
  }
      
template <typename Loop, typename WindowingBase>
typename vulkan<Loop, WindowingBase>::window vulkan<Loop, WindowingBase>::create_window (int width, int height) const
{
    using fastdraw::output::vulkan::from_result;
    using fastdraw::output::vulkan::vulkan_error_code;

    window_base wb = WindowingBase::create_window(width, height);

    VkDevice device;
    VkRenderPass renderPass;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkCommandPool commandPool;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSwapchainKHR swapChain;
    VkFence executionFinishedFence;

    std::cout << "Creating vulkan surface " << std::endl;
    std::vector<unsigned int> graphic_families = get_graphics_family (wb.physicalDevice);
    std::vector<unsigned int> presentation_families = get_presentation_family (wb.physicalDevice, wb.surface);

    std::cout << "graphic families: " << graphic_families.size() << " presentation families " << presentation_families.size()
              << " highest priority " << presentation_families[0] << std::endl;
    vulkan_queues queues;
    {
      std::vector<VkDeviceQueueCreateInfo> queue_info;
      std::unique_ptr<float[]> queue_priorities;

      std::tie (queue_info, queue_priorities)
        = vulkan_queues_create_queue_create_info (wb.physicalDevice, wb.surface);

      VkPhysicalDeviceFeatures deviceFeatures = {};

      // std::cout << "creating device" << std::endl;
      
      VkDeviceCreateInfo deviceCInfo = {};
      deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      deviceCInfo.pQueueCreateInfos = &queue_info[0];
      deviceCInfo.queueCreateInfoCount = queue_info.size();
      deviceCInfo.pEnabledFeatures = &deviceFeatures;
      std::array<const char*, 1> requiredDeviceExtensions({"VK_KHR_swapchain"/*, "VK_EXT_external_memory_dma_buf"*/
                                                           /*  , "VK_KHR_external_memory_fd", "VK_KHR_external_memory"*/});
      deviceCInfo.enabledExtensionCount = requiredDeviceExtensions.size();
      deviceCInfo.ppEnabledExtensionNames = &requiredDeviceExtensions[0];
  
      auto r = from_result(vkCreateDevice(wb.physicalDevice, &deviceCInfo, nullptr, &device));
      if (r != vulkan_error_code::success)
      {
        std::cout << "failed creating device " << static_cast<int>(r) << std::endl;
        throw std::system_error (make_error_code (r));
      }

      std::cout << "created device" << std::endl;

      auto separated_queues = vulkan_queues_create_queues (device, wb.physicalDevice, wb.surface);
      queues = vulkan_queues {separated_queues[0], separated_queues[1], separated_queues[2]};

      VkSurfaceCapabilitiesKHR capabilities;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(wb.physicalDevice, wb.surface, &capabilities);

      std::cout << "cap w: " << capabilities.currentExtent.width << " h: " << capabilities.currentExtent.height << std::endl;
      
      uint32_t formatCount;
      r = from_result(vkGetPhysicalDeviceSurfaceFormatsKHR(wb.physicalDevice, wb.surface, &formatCount, nullptr));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      std::vector<VkSurfaceFormatKHR> formats(formatCount);
      if (formatCount != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(wb.physicalDevice, wb.surface, &formatCount, formats.data());
      }
      assert(formatCount != 0);
      uint32_t imageCount = 2;

      VkSwapchainCreateInfoKHR swapInfo = {};
      swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      swapInfo.surface = wb.surface;
      swapInfo.minImageCount = imageCount;
      swapInfo.imageFormat = formats[0].format;//VK_FORMAT_B8G8R8A8_UNORM;//format.format;
      swapInfo.imageColorSpace = formats[0].colorSpace;//VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;//surfaceFormat.colorSpace;
      swapInfo.imageExtent = capabilities.currentExtent;//VkExtent2D {gwa.width, gwa.height};
      swapInfo.imageArrayLayers = 1;
      swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
      swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
      swapInfo.clipped = VK_TRUE;
      swapInfo.oldSwapchain = VK_NULL_HANDLE;
      swapInfo.preTransform = capabilities.currentTransform;

      std::vector<uint32_t> indices({graphic_families[0], presentation_families[0]});
      swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;//VK_SHARING_MODE_CONCURRENT;
      swapInfo.queueFamilyIndexCount = 2;
      swapInfo.pQueueFamilyIndices = &indices[0];
      swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      // swapInfo.queueFamilyIndexCount = 0;
      // swapInfo.pQueueFamilyIndices = nullptr;


      r = from_result(vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapChain));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      swapChainImageFormat = formats[0].format;
      swapChainExtent = capabilities.currentExtent;

      std::cout << "swapChainExtent.width " << swapChainExtent.width << " swapChainExtent.height " << swapChainExtent.height << std::endl;
      
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
      std::vector<VkImage> swapChainImages(imageCount);
      vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
      
      std::vector<VkImageView> swapChainImageViews(imageCount);
      for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create image views!");
        }
    
      }

      VkAttachmentDescription colorAttachment = {};
      colorAttachment.format = swapChainImageFormat;
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      VkAttachmentReference colorAttachmentRef = {};
      colorAttachmentRef.attachment = 0;
      colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  
      VkSubpassDescription subpass = {};
      subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      subpass.colorAttachmentCount = 1;
      subpass.pColorAttachments = &colorAttachmentRef;

      VkSubpassDependency dependency = {};
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.dstSubpass = 0;
      dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.srcAccessMask = 0;
      dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
      dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

      VkRenderPassCreateInfo renderPassInfo = {};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      renderPassInfo.attachmentCount = 1;
      renderPassInfo.pAttachments = &colorAttachment;
      renderPassInfo.subpassCount = 1;
      renderPassInfo.pSubpasses = &subpass;
      renderPassInfo.dependencyCount = 1;
      renderPassInfo.pDependencies = &dependency;
      
      if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
      }

      swapChainFramebuffers.resize(imageCount);
      for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
                                     swapChainImageViews[i]
        };
      
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;
        
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
          throw std::runtime_error("failed to create framebuffer!");
        }
      }

      VkCommandPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      poolInfo.queueFamilyIndex = /**graphicsFamilyIndex*/graphic_families[0];
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

      if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
      }

      VkFenceCreateInfo fenceInfo = {};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = 0/*VK_FENCE_CREATE_SIGNALED_BIT*/;
      if (vkCreateFence (device, &fenceInfo, nullptr, &executionFinishedFence) != VK_SUCCESS)
        throw std::runtime_error("failed to create semaphores!");
    }

    VkCommandPool mt_buffer_pool;
    {
      VkCommandPoolCreateInfo alloc_info {};
      alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      alloc_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      alloc_info.queueFamilyIndex = /**graphicsFamilyIndex*/graphic_families[0];

      
    }

    window w {{wb}, {}, {{{}/*, graphicsQueue, presentQueue*/, {}, {}
              , swapChainImageFormat, swapChainExtent, device, wb.physicalDevice
              , renderPass, commandPool, &w.shader_loader}}, /**graphicsFamilyIndex*/graphic_families[0]
              , swapChainFramebuffers, swapChain
              , executionFinishedFence, mt_buffer_pool, std::move(queues)};
    w.shader_loader = {"../fastdraw/res/shader/vulkan", device};
    // for faster loading later
    w.shader_loader.load(fastdraw::output::vulkan::shader::image_vertex);
    w.shader_loader.load(fastdraw::output::vulkan::shader::image_frag);
    return w;
}
      
} } }

#endif
