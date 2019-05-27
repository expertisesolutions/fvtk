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

namespace ftk { namespace ui { namespace backend {

template <typename Loop, typename WindowingBase>
typename vulkan<Loop, WindowingBase>::window vulkan<Loop, WindowingBase>::create_window (int width, int height) const
{
    window_base wb = WindowingBase::create_window(width, height);

    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue graphicsQueue, presentQueue;
    VkRenderPass renderPass;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkCommandPool commandPool;
    std::optional<unsigned int> graphicsFamilyIndex;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkSwapchainKHR swapChain;
    VkFence executionFinishedFence;
    
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

      vulkan_error_code r = from_result(vkCreateInstance(&cinfo, NULL, &instance));
      if (r != vulkan_error_code::success)
      {
        throw std::system_error (make_error_code (r));
      }

      VkXlibSurfaceCreateInfoKHR info = {};
      std::memset(&info, 0, sizeof(info));
      info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
      info.dpy = wb.display;
      info.window = wb.win;

      r = from_result (vkCreateXlibSurfaceKHR(instance, &info, NULL, &surface));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      uint32_t deviceCount = 0;
      vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

      assert(deviceCount != 0);
      
      std::vector<VkPhysicalDevice> devices(deviceCount);
      vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

      physicalDevice = devices[0];

      uint32_t queueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

      std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

      std::optional<unsigned int> presentationFamilyIndex;
    
      int i = 0;
      for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          graphicsFamilyIndex = i;
        }

        {
          VkBool32 presentSupport = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
          if (queueFamily.queueCount > 0 && presentSupport) {
            presentationFamilyIndex = i;
          }
        }

        if (graphicsFamilyIndex && presentationFamilyIndex)
          break;
        i++;
      }

      float queuePriority = 1.0f;
      std::array<VkDeviceQueueCreateInfo, 2> queueInfo
      ({
        VkDeviceQueueCreateInfo {
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
          , NULL
          , 0
          , *graphicsFamilyIndex
          , 1
          , &queuePriority
        },
        VkDeviceQueueCreateInfo {
          VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
          , NULL
          , 0
          , *presentationFamilyIndex
          , 1
          , &queuePriority
        }
      });
      //assert(*graphicsFamilyIndex == *presentationFamilyIndex);

      VkPhysicalDeviceFeatures deviceFeatures = {};
  
      VkDeviceCreateInfo deviceCInfo = {};
      deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      deviceCInfo.pQueueCreateInfos = &queueInfo[0];
      deviceCInfo.queueCreateInfoCount = queueInfo.size();
      deviceCInfo.pEnabledFeatures = &deviceFeatures;
      std::array<const char*, 1> requiredDeviceExtensions({"VK_KHR_swapchain"});
      deviceCInfo.enabledExtensionCount = requiredDeviceExtensions.size();
      deviceCInfo.ppEnabledExtensionNames = &requiredDeviceExtensions[0];
  
      r = from_result(vkCreateDevice(physicalDevice, &deviceCInfo, nullptr, &device));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      vkGetDeviceQueue(device, *graphicsFamilyIndex, 0, &graphicsQueue);
      vkGetDeviceQueue(device, *presentationFamilyIndex, 0, &presentQueue);

      VkSurfaceCapabilitiesKHR capabilities;
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

      std::cout << "cap w: " << capabilities.currentExtent.width << " h: " << capabilities.currentExtent.height << std::endl;
      
      uint32_t formatCount;
      r = from_result(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
      if (r != vulkan_error_code::success)
        throw std::system_error (make_error_code (r));

      std::vector<VkSurfaceFormatKHR> formats(formatCount);
      if (formatCount != 0) {
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
      }
      assert(formatCount != 0);
      uint32_t imageCount = 1;
  
      VkSwapchainCreateInfoKHR swapInfo = {};
      swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
      swapInfo.surface = surface;
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

      std::vector<uint32_t> indices({*graphicsFamilyIndex, *presentationFamilyIndex});
      swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;//VK_SHARING_MODE_CONCURRENT;
      swapInfo.queueFamilyIndexCount = 2;
      swapInfo.pQueueFamilyIndices = &indices[0];
      // swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
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
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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
      poolInfo.queueFamilyIndex = *graphicsFamilyIndex;
      poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

      if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
      }

      VkFenceCreateInfo fenceInfo = {};
      fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      if (vkCreateFence (device, &fenceInfo, nullptr, &executionFinishedFence) != VK_SUCCESS)
        throw std::runtime_error("failed to create semaphores!");
    }
    
    window w {{wb}, {}, {{{}, graphicsQueue, presentQueue, {}, {}
          , swapChainImageFormat, swapChainExtent, device, physicalDevice
              , renderPass, commandPool, &w.shader_loader}}, *graphicsFamilyIndex
              , swapChainFramebuffers, presentQueue, swapChain
              , executionFinishedFence};
    w.shader_loader = {"../fastdraw/res/shader/vulkan", device};
    return w;
}
      
} } }

#endif
