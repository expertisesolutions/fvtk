#include <fastdraw/scene.hpp>
#include <fastdraw/output/vulkan/vulkan_output.hpp>
#include <fastdraw/output/vulkan/add_triangle.hpp>
#include <fastdraw/output/vulkan/add_box.hpp>
#include <fastdraw/object/triangle.hpp>
#include <fastdraw/color.hpp>
#include <fastdraw/output/vulkan/shader_loader.hpp>
#include <fastdraw/output/vulkan/vulkan_draw.hpp>
#include <fastdraw/output/vulkan/add_text.hpp>

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>
#include <optional>
#include <fstream>
#include <chrono>
#include <thread>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>
#include <vulkan/vulkan_core.h>

typedef float coord_type;
typedef fastdraw::point<coord_type> point_type;
typedef float color_channel_type;
typedef fastdraw::color::color_premultiplied_rgba<color_channel_type> color_type;

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice physicalDevice);

std::pair<VkBuffer, VkDeviceMemory> create_vertex_buffer (VkDevice device, std::size_t size, VkPhysicalDevice physicalDevice)
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
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties
                        , VkPhysicalDevice physicalDevice) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  
  throw std::runtime_error("failed to find suitable memory type!");
}

fastdraw::output::vulkan::vulkan_output<coord_type, point_type, color_type> on_animation
(fastdraw::scene<float, point_type, color_type>& scene
 , fastdraw::scene_difference<float, point_type, color_type>& scene_diff
 , fastdraw::output::vulkan::vulkan_output<coord_type, point_type, color_type>& difference_output)
{
  using triangle_type = fastdraw::object::fill_triangle<point_type, color_type>;

  // auto before = std::chrono::high_resolution_clock::now();
  
  // std::cout << "animation step" << std::endl;
  assert (scene.objects.size() > 0);

  try
  {
    triangle_type triangle = std::get<triangle_type>(scene.objects[0].object);
    if (triangle.p1.x >= -1.0)
    {
      triangle.p1.x -= 0.0016;
      triangle.p2.x -= 0.0016;
      triangle.p3.x -= 0.0016;
      replace (scene, scene_diff, 0, triangle);
    }

    //std::cout << "replace " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before).count() << "micros" << std::endl;

    auto difference_output1 = output (scene, scene_diff, difference_output);

    //std::cout << "output conversion " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before).count() << "micros" << std::endl;
    
    merge_scene_difference (scene, scene_diff);
    scene_diff.clear();

    //std::cout << "merge " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before).count() << "micros" << std::endl;

    // std::cout << "finished animation step" << std::endl;
    
    return difference_output1;
  }
  catch (std::exception const& e)
  {
    std::cout << "exception " << e.what() << std::endl;
    exit(-1);
  }
}

void submit_graphic_queue (VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore
                           , VkFence executionFinishedFence
                           , VkQueue graphicsQueue
                           , std::vector<VkCommandBuffer>& commandBuffers
                           , int imageIndex)
{
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, executionFinishedFence) != VK_SUCCESS) {
          throw std::runtime_error("failed to submit draw command buffer!");
        }
}

void record_command_buffer(VkRenderPass renderPass, VkCommandBuffer commandBuffer
                           , int imageIndex
                           , std::vector<VkFramebuffer>& swapChainFramebuffers
                           , VkExtent2D swapChainExtent
                           , fastdraw::output::vulkan::vulkan_output<coord_type, point_type, color_type>& diff_output)
                           // , VkBuffer vertexBuffer)
{
        {
          VkCommandBufferBeginInfo beginInfo = {};
          beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

          if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
          }

          VkRenderPassBeginInfo renderPassInfo = {};
          renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
          renderPassInfo.renderPass = renderPass;
          renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
          renderPassInfo.renderArea.offset = {0, 0};
          renderPassInfo.renderArea.extent = swapChainExtent;

          vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

          for (auto&& object_output : diff_output.object_outputs)
          {
            for (auto&& pipeline : object_output.draw_infos)
            {
              // VkBuffer vertexBuffers[] = {vertexBuffer};
              // VkDeviceSize offsets[] = {0};
              // vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
              // for (int i = 0; i != 500; ++i)
                fastdraw::output::vulkan::draw (pipeline, commandBuffers[imageIndex]);

            }
          }
        
          vkCmdEndRenderPass(commandBuffers[imageIndex]);

          if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
          }
        }
}

void acquire_image (VkDevice device, VkSwapchainKHR swapChain, VkSemaphore imageAvailableSemaphore, uint32_t& imageIndex)
{
  vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
}

int main()
{
  Display                 *dpy;
  Window                  root;
  Colormap                cmap;
  XSetWindowAttributes    swa;
  Window                  win;
  //XWindowAttributes       gwa;
  XEvent                  xev = {};
  int width = 1280, height = 1000;

  dpy = XOpenDisplay(NULL);
 
  if(dpy == NULL) {
    printf("\n\tcannot connect to X server\n\n");
    return -1;
  }

  root = DefaultRootWindow(dpy);
  cmap = DefaultColormap(dpy, 0);
  //cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

  swa.colormap = cmap;
  swa.event_mask = ExposureMask | KeyPressMask;
 
  win = XCreateWindow(dpy, root, 0, 0, width, height, 0, /*vi->depth*/24, InputOutput, /*vi->visual*/0, CWColormap | CWEventMask, &swa);

  XMapWindow(dpy, win);
  XStoreName(dpy, win, "VERY SIMPLE APPLICATION");
  
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

  VkInstance instance;
  if (vkCreateInstance(&cinfo, NULL, &instance) != VK_SUCCESS)

    {
      std::cout << "error instance" << std::endl;
      return -10;
    }

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  physicalDevice = devices[0];

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

  std::optional<unsigned int> graphicsFamilyIndex, presentationFamilyIndex;

  VkXlibSurfaceCreateInfoKHR info;
  std::memset(&info, 0, sizeof(info));

  info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  info.dpy = dpy;
  info.window = win;

  VkSurfaceKHR surface;
  if (vkCreateXlibSurfaceKHR(instance, &info, NULL, &surface) != VK_SUCCESS)
  {
    std::cout << "error surface" << std::endl;
    return -10;
  }
  
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
  assert(*graphicsFamilyIndex == *presentationFamilyIndex);
  
  VkPhysicalDeviceFeatures deviceFeatures = {};
  
  VkDeviceCreateInfo deviceCInfo = {};
  deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCInfo.pQueueCreateInfos = &queueInfo[0];
  deviceCInfo.queueCreateInfoCount = queueInfo.size();
  deviceCInfo.pEnabledFeatures = &deviceFeatures;
  std::array<const char*, 1> requiredDeviceExtensions({"VK_KHR_swapchain"});
  deviceCInfo.enabledExtensionCount = requiredDeviceExtensions.size();
  deviceCInfo.ppEnabledExtensionNames = &requiredDeviceExtensions[0];
  
  VkDevice device;
  if (vkCreateDevice(physicalDevice, &deviceCInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  VkQueue graphicsQueue, presentQueue;
  vkGetDeviceQueue(device, *graphicsFamilyIndex, 0, &graphicsQueue);
  vkGetDeviceQueue(device, *presentationFamilyIndex, 0, &presentQueue);

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  if (formatCount != 0) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data());
  }
  assert(formatCount != 0);
  uint32_t imageCount = 2;
  
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
  swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
  swapInfo.queueFamilyIndexCount = 2;
  swapInfo.pQueueFamilyIndices = &indices[0];
  // swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // swapInfo.queueFamilyIndexCount = 0;
  // swapInfo.pQueueFamilyIndices = nullptr;

  VkSwapchainKHR swapChain;

  if (vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapChain) != VK_SUCCESS) {
    throw std::runtime_error("failed to create swap chain!");
  }

  VkFormat swapChainImageFormat = formats[0].format;
  VkExtent2D swapChainExtent = capabilities.currentExtent;
  
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

  
  auto load_shader = [&] (const char* name)
  {
    std::ifstream file(name, std::ios::ate | std::ios::binary);

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
     return shaderModule;
   };

    VkShaderModule vertShaderModule = load_shader("tests/vert.spv");
    VkShaderModule fragShaderModule = load_shader("tests/frag.spv");

    fastdraw::output::vulkan::shader_loader shader_loader("res/shader/vulkan", device);

    VkRenderPass renderPass;
    {
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
    }

    std::vector<VkFramebuffer> swapChainFramebuffers(imageCount);
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

    VkCommandPool commandPool;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = *graphicsFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
    }

    std::vector<VkCommandBuffer> commandBuffers(imageCount);
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }

    fastdraw::scene<float, point_type, color_type> scene;
    fastdraw::scene_difference<float, point_type, color_type> scene_diff;
    typedef fastdraw::object::fill_triangle<point_type, color_type> triangle_type;
    typedef fastdraw::object::fill_text<point_type, std::string, color_type> text_type;
    typedef fastdraw::object::fill_box<point_type, color_type> box_type;

    color_type red {1.0, 0.0, 0.0, 1.0}, blue {0.0, 0.0, 1.0, 1.0}, transparent_blue {0.0, 0.0, 0.5, 0.5}
      , transparent_red {0.0, 0.25, 0.0, 0.25};;
    triangle_type triangle{{{0.0, -0.75}, {0.75, 0.75}, {-0.75, 0.75}}, blue};
    // fastdraw::object::fill_triangle<point_type, color_type> triangle2{{{0.0, -0.25}, {0.25, 0.25}, {-0.25, 0.25}}, blue};

    push_back(scene, scene_diff, triangle
              , text_type
              {
                {
                  {-0.5f, -0.5f}, {1.0f, 0.3f}, "/usr/share/fonts/TTF/DejaVuSans.ttf", "Hello World"
                  , {fastdraw::object::text_scale{true, true}}
                  , fastdraw::object::text_align::center
                  , fastdraw::object::text_align::center
                }, transparent_blue
              }
              , box_type
              {
                {{-0.5f, -0.5f}, {1.0f, 0.3f}}, transparent_red
              }

              /*, triangle2
              , triangle_type{{{0.1, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.0, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.1, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.2, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.3, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.4, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.5, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.6, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.7, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.8, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.9, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.10, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.11, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.12, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.13, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.14, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.15, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.16, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.17, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.18, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.19, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.20, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.21, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.22, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.23, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.24, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.25, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.26, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.27, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.28, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.29, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.30, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.31, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.32, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.33, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.34, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.35, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.36, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.37, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.38, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.39, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.40, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.41, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.42, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.43, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.44, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.45, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.46, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.47, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.48, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.49, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.50, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.51, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.52, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.53, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.54, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.55, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.56, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.57, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.58, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.59, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.60, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.61, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.62, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.63, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.64, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.65, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.66, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.67, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.68, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.69, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.70, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.71, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.72, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.73, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.74, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.75, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.76, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.77, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.78, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.79, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.80, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.81, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}
              , triangle_type{{{0.82, -0.5}, {0.5, 0.5}, {-0.5, 0.5}}, blue}*/);

    std::cout << "diff size " << scene_diff.operations.size() << std::endl;
    
    fastdraw::output::vulkan::vulkan_output<coord_type, point_type
                                            , color_type> voutput
      {{{}, graphicsQueue, presentQueue, vertShaderModule, fragShaderModule
        , swapChainImageFormat, swapChainExtent, device, physicalDevice
        , renderPass, commandPool, &shader_loader}};

    auto intermediate = fastdraw::output::vulkan::output (scene, scene_diff, voutput);

    merge_scene_difference (scene, scene_diff);
    scene_diff.clear();

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence executionFinishedFence;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {

      throw std::runtime_error("failed to create semaphores!");
    }

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence (device, &fenceInfo, nullptr, &executionFinishedFence) != VK_SUCCESS)
      throw std::runtime_error("failed to create semaphores!");

    // VkBuffer vertexBuffer;
    // VkDeviceMemory vertexBufferMemory;
    // std::tie(vertexBuffer, vertexBufferMemory) = create_vertex_buffer (device, sizeof(float)*16, physicalDevice);

    // void* data;
    // vkMapMemory(device, vertexBufferMemory, 0, sizeof(float)*16, 0, &data);
    
    // const float vertices[] = { triangle.p1.x, triangle.p1.y, 0.0f
    //                            , triangle.p2.x, triangle.p2.y, 0.0f
    //                            , triangle.p3.x, triangle.p3.y, 0.0f };

    // std::memcpy(data, vertices, sizeof(vertices));

    // vkUnmapMemory(device, vertexBufferMemory);


    while(1) {
      bool check = XCheckWindowEvent(dpy, win, KeyPressMask | ExposureMask, &xev);

      //std::cout << "check " << check << " xev.type " << xev.type << std::endl;
        
      if(!check || xev.type == Expose)
      {
        //std::cout << "check == " << check << " xev.type == Expose " << (xev.type == Expose) << std::endl;
        xev.type = 0;

        //XGetWindowAttributes(dpy, win, &gwa);
      {
        //std::cout << "waiting on fence" << std::endl;
        if (vkWaitForFences (device, 1, &executionFinishedFence, VK_FALSE, -1) == VK_TIMEOUT)
        {
          std::cout << "Timeout waiting for fence" << std::endl;
          throw -1;
        }
        vkResetFences (device, 1, &executionFinishedFence);
        //std::cout << "fence time " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before_draw).count() << "micros" << std::endl;
        
        // std::cout << "redrawing" << std::endl;
        uint32_t imageIndex = 0;
        //std::this_thread::sleep_for(std::chrono::microseconds(16600));
        
        //vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        //auto before_draw = std::chrono::high_resolution_clock::now();
        acquire_image (device, swapChain, imageAvailableSemaphore, imageIndex);
        //std::cout << "acquire next image " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before_draw).count() << "micros" << std::endl;

        auto before_draw = std::chrono::high_resolution_clock::now();
        
        // std::cout << "doing animation " << imageIndex << std::endl;
        //fastdraw::output::vulkan::vulkan_output<coord_type, point_type, color_type> output = voutput;
        fastdraw::output::vulkan::vulkan_output<coord_type, point_type, color_type> diff_output;
        // static bool first = true;
        //if (first)
          {
            diff_output = voutput;
            // first = false;
          }
        // else
        //   diff_output = on_animation (scene, scene_diff, voutput);

        //std::cout << "animation " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before_draw).count() << "micros" << std::endl;

        // static bool recorded[2] = {false, false};
        // if (!recorded[imageIndex])
          {
            record_command_buffer(renderPass, commandBuffers, imageIndex, swapChainFramebuffers, swapChainExtent, diff_output);
            // recorded[imageIndex] = true;
          }
        
        //std::cout << "saving buffer " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before_draw).count() << "micros" << std::endl;

        submit_graphic_queue (imageAvailableSemaphore, renderFinishedSemaphore, executionFinishedFence
                              , graphicsQueue, commandBuffers, imageIndex);

        //std::cout << "queue graphics " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - before_draw).count() << "micros" << std::endl;
        
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        vkQueuePresentKHR(presentQueue, &presentInfo);

        auto after_draw = std::chrono::high_resolution_clock::now();
        auto int_ms = std::chrono::duration_cast<std::chrono::microseconds>(after_draw - before_draw);
        std::cout << "drawing time total " << int_ms.count() << "micros" << std::endl;
 
      }
    }
    else if(check && xev.type == KeyPress) {
      std::cout << "keypressed" << std::endl;
      XDestroyWindow(dpy, win);
      XCloseDisplay(dpy);
      return -1;
    }

  }

  return 0;
}
