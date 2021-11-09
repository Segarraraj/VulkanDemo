#include "render.h"

#include "logger.h"
#include "utils.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "chrono"

static struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
};

static const int MAX_FRAMES_IN_FLIGHT = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData);

Render::Render() { }

Render::~Render() {
  cleanup();

#ifdef DEBUG
  auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");

  if (vkDestroyDebugUtilsMessengerEXT)
  {
    vkDestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);
  }
#endif // DEBUG

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    vkDestroySemaphore(_device, _image_ready_semaphores[i], nullptr);
    vkDestroySemaphore(_device, _render_finished_semaphores[i], nullptr);
    vkDestroyFence(_device, _frame_fences[i], nullptr);
  }

  vkDestroyDescriptorSetLayout(_device, _uniform_descriptor_layout, nullptr);

  vkDestroyBuffer(_device, _positions_vertex_buffer, nullptr);
  vkFreeMemory(_device, _positions_buffer_memory, nullptr);
  vkDestroyBuffer(_device, _colors_vertex_buffer, nullptr);
  vkFreeMemory(_device, _colors_buffer_memory, nullptr);
  vkDestroyBuffer(_device, _indices_buffer, nullptr);
  vkFreeMemory(_device, _indices_buffer_memory, nullptr);
  vkDestroyBuffer(_device, _uniform_buffer, nullptr);
  vkFreeMemory(_device, _uniform_buffer_memory, nullptr);

  vkDestroyCommandPool(_device, _command_pool, nullptr);
  vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
  
  vkDestroyDevice(_device, nullptr);
  vkDestroySurfaceKHR(_instance, _surface, nullptr);
  vkDestroyInstance(_instance, nullptr);
}

int Render::init(HWND window, HINSTANCE instance)
{
  _window = window;
  LOG_DEBUG("Render", "Initializing Vulkan :)");

  // CREATING APP INFO
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "Vulkan Demo";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "NotBadBoy";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_2;  

  // ENUMERATING AVAILABLE LAYERS AND EXTENSIONS
  uint32_t count;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, &availableExtensions[0]);
  
  std::cout << "\n";
  LOG_DEBUG("Render", "Available extensions:");
  LOG_DEBUG("Render", "---------------------");
  for (const VkExtensionProperties& propertie : availableExtensions)
  {
    LOG_DEBUG("Render", "%s, V%d", propertie.extensionName, propertie.specVersion);
  }

  vkEnumerateInstanceLayerProperties(&count, nullptr);
  std::vector<VkLayerProperties> availableLayers(count);
  vkEnumerateInstanceLayerProperties(&count, &(availableLayers[0]));

  std::cout << "\n";
  LOG_DEBUG("Render", "Available layers:");
  LOG_DEBUG("Render", "-----------------");
  for (const VkLayerProperties& propertie : availableLayers)
  {
    LOG_DEBUG("Render", "%s, V%d", propertie.layerName, propertie.implementationVersion);
  }

  // ADDING LAYERS AND EXTENSIONS
  std::vector<char*> extensions = std::vector<char*>();
  std::vector<char*> layers = std::vector<char*>();

  extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
  extensions.push_back("VK_KHR_win32_surface");

#ifdef DEBUG
  layers.push_back("VK_LAYER_KHRONOS_validation");
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // DEBUG

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = extensions.size();
  create_info.ppEnabledExtensionNames = &(extensions[0]);
  create_info.enabledLayerCount = layers.size();
  create_info.ppEnabledLayerNames = &(layers[0]);

  // CREATING VULKAN INSTANCE
  VkResult result = vkCreateInstance(&create_info, nullptr, &_instance);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create Vulkan instance");
    return 0;
  }

  createDebuger();

  if (!createSurface(window, instance))
  {
    return 0;
  }

  std::vector<char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  if (!pickPhysicalDevice(device_extensions)) {
    return 0;
  }

  if (!createLogicalDevice(device_extensions)) {
    return 0;
  }

  RECT rect;
  GetClientRect(window, &rect);
  if (!createSwapChain(rect.right - rect.left, rect.bottom - rect.top))
  {
    return 0;
  }

  if (!createRenderPass())
  {
    return 0;
  }

  if (!createGraphicsPipeline())
  {
    return 0;
  }

  if (!createVertexBuffers())
  {
    return 0;
  }

  if (!createCommandBuffer())
  {
    return 0;
  }

  return 1;
}

void Render::drawFrame()
{
  static size_t frame = 0;
  uint32_t image_index = 0;

  vkWaitForFences(_device, 1, &_frame_fences[frame], VK_TRUE, UINT64_MAX);

  VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _image_ready_semaphores[frame], VK_NULL_HANDLE, &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    recreateSwapChain();
    return;
  }
  else if (result == VK_SUBOPTIMAL_KHR)
  {
    LOG_WARNING("Render", "Running with suboptimal swapchain");
  }
  else if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to get image from swapchain");
    return;
  }

  if (_image_fences[image_index] != VK_NULL_HANDLE)
  {
    vkWaitForFences(_device, 1, &_image_fences[image_index], VK_TRUE, UINT64_MAX);
  }

  _image_fences[image_index] = _frame_fences[frame];

  VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &_image_ready_semaphores[frame];
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &_command_buffers[image_index];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &_render_finished_semaphores[frame];

  vkResetFences(_device, 1, &_frame_fences[frame]);

  update();

  result = vkQueueSubmit(_graphics_queue, 1, &submit_info, _frame_fences[frame]);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed submiting command buffer");
    return;
  }

  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &_render_finished_semaphores[frame];
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &_swapchain;
  present_info.pImageIndices = &image_index;
  present_info.pResults = nullptr;

  result = vkQueuePresentKHR(_present_queue, &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _resize) {
    _resize = false;
    recreateSwapChain();
  }
  else if (result != VK_SUCCESS) {
    LOG_ERROR("Render", "Failed to present swapchain image");
    return;
  }

  frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {

  switch (messageSeverity)
  {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
    LOG_DEBUG("VALIDATION LAYER", pCallbackData->pMessage);
    break;
  }
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
    LOG_WARNING("VALIDATION LAYER", pCallbackData->pMessage);
    break;
  }
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
    LOG_ERROR("VALIDATION LAYER", pCallbackData->pMessage);
    break;
  }
  //default: {
  //  LOG_DEBUG("VALIDATION LAYER", pCallbackData->pMessage);
  //  break;
  //}
  }

  return VK_FALSE;
}

void Render::createDebuger()
{
#if DEBUG
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating debuger");
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
  debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_create_info.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_create_info.messageType =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_create_info.pfnUserCallback = debugCallback;
  debug_create_info.pUserData = nullptr;

  // Get extension funtion pointer
  auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");

  // check if it's valid
  if (!vkCreateDebugUtilsMessengerEXT) {
    LOG_WARNING("Render", "Failed to set up debug messenger");
    return;
  }
   
  // Create the debug messenger
  VkResult result = vkCreateDebugUtilsMessengerEXT(_instance, &debug_create_info, nullptr, &_debug_messenger);
  if (result != VK_SUCCESS)
  {
    LOG_WARNING("Render", "Failed to create debug messenger");
    return;
  }

  LOG_DEBUG("Render", "Debuger created succesfully");
#endif // DEBUG
}

int Render::createSurface(HWND window, HINSTANCE instance)
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating surface");
  VkWin32SurfaceCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  create_info.hwnd = window;
  create_info.hinstance = instance;

  VkResult result = vkCreateWin32SurfaceKHR(_instance, &create_info, nullptr, &_surface);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create window surface");
    return 0;
  }

  LOG_DEBUG("Render", "Surface created succesfully");
  return 1;
}

int Render::createLogicalDevice(const std::vector<char*>& device_extensions)
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating logical device");

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  int32_t queues[] = { _queue_indices.graphics_family, _queue_indices.present_family };

  for (size_t i = 0; i < sizeof(queues) / sizeof(queues[0]); i++)
  {
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueCount = 1; 
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_info.queueFamilyIndex = queues[i];
    queue_create_infos.push_back(queue_create_info);
  }

  // Any mandatory features required
  VkPhysicalDeviceFeatures device_features = {};

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = (uint32_t) queue_create_infos.size();
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledLayerCount = 0;
  create_info.ppEnabledLayerNames = nullptr;
  create_info.enabledExtensionCount = device_extensions.size();
  create_info.ppEnabledExtensionNames = device_extensions.data();

  VkResult result = vkCreateDevice(_physical_device, &create_info, nullptr, &_device);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create logical device!");
    return 0;
  }

  vkGetDeviceQueue(_device, _queue_indices.graphics_family, 0, &_graphics_queue);
  vkGetDeviceQueue(_device, _queue_indices.present_family, 0, &_present_queue);

  LOG_DEBUG("Render", "Logical device created succesfully");

  return 1;
}

int Render::createSwapChain(int width, int height)
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating swapchain");
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> available_formats;
  std::vector<VkPresentModeKHR> available_present_modes;

  VkSurfaceFormatKHR surface_format = {};
  VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
  VkExtent2D surface_extent = { (uint32_t) width, (uint32_t) height };

  uint32_t count;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &capabilities);

  vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &count, nullptr);
  if (count != 0) 
  {
    available_formats.resize(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &count, available_formats.data());
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &count, nullptr);
  if (count != 0) 
  {
    available_present_modes.resize(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &count, available_present_modes.data());
  }

  surface_format = available_formats[0];
  for (size_t i = 0; i < available_formats.size(); i++)
  {
    if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
      available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      surface_format = available_formats[i];
    }
  }

  for (size_t i = 0; i < available_present_modes.size(); i++)
  {
    if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      surface_present_mode = available_present_modes[i];
    }
  }

  if (capabilities.currentExtent.width != UINT32_MAX) 
  {
    surface_extent = capabilities.currentExtent;
  }
  else {
    surface_extent.width = glm::clamp(surface_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    surface_extent.height = glm::clamp(surface_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
  }

  uint32_t image_count = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount != 0 && image_count > capabilities.maxImageCount)
  {
    image_count = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.preTransform = capabilities.currentTransform;
  create_info.surface = _surface;
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = surface_extent;
  create_info.presentMode = surface_present_mode;
  create_info.imageArrayLayers = 1;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;
  
  uint32_t queue_family_indices[] = { _queue_indices.graphics_family, _queue_indices.present_family };
  if (_queue_indices.graphics_family != _queue_indices.present_family)
  {
    // VK_SHARING_MODE_EXCLUSIVE should be better for performance
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = sizeof(queue_family_indices) / sizeof(queue_family_indices[0]);
    create_info.pQueueFamilyIndices = queue_family_indices;
  }
  else
  {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = nullptr;
  }

  VkResult result = vkCreateSwapchainKHR(_device, &create_info, nullptr, &_swapchain);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create SwapChain");
    return 0;
  }

  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, nullptr);
  _swapchain_images.resize(image_count);
  vkGetSwapchainImagesKHR(_device, _swapchain, &image_count, _swapchain_images.data());

  _swapchain_extent = surface_extent;
  _swapchain_format = surface_format.format;

  _swapchain_image_views.resize(image_count);
  for (uint32_t i = 0; i < image_count; i++)
  {
    VkImageViewCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.image = _swapchain_images[i];
    create_info.format = surface_format.format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
  
    VkResult result = vkCreateImageView(_device, &create_info, nullptr, &(_swapchain_image_views[i]));
    if (result != VK_SUCCESS)
    {
      LOG_DEBUG("Render", "Failed creating image view %d", i);
      return 0;
    }
  }

  LOG_DEBUG("Render", "Swapchain created succesfully");
  return 1;
}

int Render::createRenderPass()
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating render pass");

  VkAttachmentDescription color_attachment = {};
  color_attachment.format = _swapchain_format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_attachment_ref;

  VkSubpassDependency subpass_dependency = {};
  subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency.dstSubpass = 0;
  subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.srcAccessMask = 0;
  subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.attachmentCount = 1;
  create_info.pAttachments = &color_attachment;
  create_info.subpassCount = 1;
  create_info.pSubpasses = &subpass;
  create_info.dependencyCount = 1;
  create_info.pDependencies = &subpass_dependency;

  VkResult result = vkCreateRenderPass(_device, &create_info, nullptr, &_render_pass);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating render pass");
    return 0;
  }

  _swapchain_framebuffers.resize(_swapchain_image_views.size());
  for (size_t i = 0; i < _swapchain_image_views.size(); i++) {
    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = _render_pass;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments = &(_swapchain_image_views[i]);
    framebuffer_info.width = _swapchain_extent.width;
    framebuffer_info.height = _swapchain_extent.height;
    framebuffer_info.layers = 1;

    result = vkCreateFramebuffer(_device, &framebuffer_info, nullptr, &_swapchain_framebuffers[i]);
    if (result != VK_SUCCESS)
    {
      LOG_ERROR("Render", "Failed creating framebuffer");
      return 0;
    }
  }

  LOG_DEBUG("Render", "Render pass created succesfully");
  return 1;
}

int Render::createGraphicsPipeline()
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating ghrapic pipeline");
  
  std::vector<char> vertex_shader_code = Utils::readFile("../../shaders/vert.spv");
  std::vector<char> fragment_shader_code = Utils::readFile("../../shaders/frag.spv");

  if (vertex_shader_code.size() == 0 || fragment_shader_code.size() == 0)
  {
    LOG_ERROR("Render", "Failed opening shader files");
    return 0;
  }

  VkShaderModule vertex_shader_module = createShaderModule(vertex_shader_code);
  VkShaderModule fragment_shader_module = createShaderModule(fragment_shader_code);

  VkPipelineShaderStageCreateInfo vertex_stage_create_info = {};
  vertex_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertex_stage_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_stage_create_info.module = vertex_shader_module;
  vertex_stage_create_info.pName = "main";

  VkPipelineShaderStageCreateInfo fragment_stage_create_info = {};
  fragment_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragment_stage_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_stage_create_info.module = fragment_shader_module;
  fragment_stage_create_info.pName = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_stage_create_info, fragment_stage_create_info };

  VkVertexInputBindingDescription positions_binding_description = {};
  positions_binding_description.binding = 0;
  positions_binding_description.stride = sizeof(glm::vec2);
  positions_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputBindingDescription colors_binding_description = {};
  colors_binding_description.binding = 1;
  colors_binding_description.stride = sizeof(glm::vec3);
  colors_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription positions_attribute_description = {};
  positions_attribute_description.binding = 0;
  positions_attribute_description.location = 0;
  positions_attribute_description.offset = 0;
  positions_attribute_description.format = VK_FORMAT_R32G32_SFLOAT;

  VkVertexInputAttributeDescription colors_attribute_description = {};
  colors_attribute_description.binding = 1;
  colors_attribute_description.location = 1;
  colors_attribute_description.offset = 0;
  colors_attribute_description.format = VK_FORMAT_R32G32B32_SFLOAT;

  VkVertexInputBindingDescription bindings[] = { positions_binding_description, colors_binding_description };
  VkVertexInputAttributeDescription attributes[] = { positions_attribute_description, colors_attribute_description };

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = sizeof(bindings) / sizeof(VkVertexInputBindingDescription);
  vertex_input_info.pVertexBindingDescriptions = bindings;
  vertex_input_info.vertexAttributeDescriptionCount = sizeof(attributes) / sizeof(VkVertexInputAttributeDescription);;
  vertex_input_info.pVertexAttributeDescriptions = attributes;

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = _swapchain_extent.width;
  viewport.height = _swapchain_extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = { 0, 0 };
  scissor.extent = _swapchain_extent;

  VkPipelineViewportStateCreateInfo viewport_info = {};
  viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_info.viewportCount = 1;
  viewport_info.pViewports = &viewport;
  viewport_info.scissorCount = 1;
  viewport_info.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampler = {};
  multisampler.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampler.sampleShadingEnable = VK_FALSE;
  multisampler.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampler.minSampleShading = 1.0f;
  multisampler.pSampleMask = nullptr;
  multisampler.alphaToCoverageEnable = VK_FALSE;
  multisampler.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState color_blend_attachment = {};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;

  VkDescriptorSetLayoutBinding uniform_layour_binding = {};
  uniform_layour_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniform_layour_binding.binding = 0;
  uniform_layour_binding.descriptorCount = 1;
  uniform_layour_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo uniform_layout_create_info = {};
  uniform_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  uniform_layout_create_info.bindingCount = 1;
  uniform_layout_create_info.pBindings = &uniform_layour_binding;

  VkResult result = vkCreateDescriptorSetLayout(_device, &uniform_layout_create_info, nullptr, &_uniform_descriptor_layout);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating uniform descriptor layuout");
    return 0;
  }

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.setLayoutCount = 1;
  pipeline_layout_create_info.pSetLayouts = &_uniform_descriptor_layout;

  result = vkCreatePipelineLayout(_device, &pipeline_layout_create_info, nullptr, &_pipeline_layout);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating pipeline layout");
    return 0;
  }

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_info;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampler;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.layout = _pipeline_layout;
  pipeline_info.renderPass = _render_pass;
  pipeline_info.subpass = 0;

  result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &_graphics_pipeline);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating graphics pipeline");
    return 0;
  }

  vkDestroyShaderModule(_device, vertex_shader_module, nullptr);
  vkDestroyShaderModule(_device, fragment_shader_module, nullptr);

  LOG_DEBUG("Render", "Ghrapic pipeline created succesfully");
  return 1;
}

int Render::createVertexBuffers()
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating vertex buffer");

  void* buffer_memory;
  const std::vector<glm::vec2> positions = {
    {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f},
  };

  const std::vector<glm::vec3> colors = {
    {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
  };

  const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
  };

  //////////////////
  // POSITIONS
  VkBufferCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.size = sizeof(glm::vec2) * positions.size();
  create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(_device, &create_info, nullptr, &_positions_vertex_buffer);
  if (result != VK_SUCCESS) 
  {
    LOG_ERROR("Render", "Failed creating vertex buffer");
    return 0;
  }

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(_device, _positions_vertex_buffer, &memory_requirements);

  VkMemoryAllocateInfo allocate_info{};
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = memory_requirements.size;
  allocate_info.memoryTypeIndex = findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  result = vkAllocateMemory(_device, &allocate_info, nullptr, &_positions_buffer_memory);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Render", "Failed to allocate vertex buffer memory!");
    return 0;
  }

  vkBindBufferMemory(_device, _positions_vertex_buffer, _positions_buffer_memory, 0);

  vkMapMemory(_device, _positions_buffer_memory, 0, create_info.size, 0, &buffer_memory);
  memcpy(buffer_memory, positions.data(), (size_t) create_info.size);
  vkUnmapMemory(_device, _positions_buffer_memory);

  //////////////////
  // COLORS
  create_info.size = sizeof(glm::vec3) * colors.size();
  result = vkCreateBuffer(_device, &create_info, nullptr, &_colors_vertex_buffer);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating vertex buffer");
    return 0;
  }

  vkGetBufferMemoryRequirements(_device, _colors_vertex_buffer, &memory_requirements);
  allocate_info.allocationSize = memory_requirements.size;

  result = vkAllocateMemory(_device, &allocate_info, nullptr, &_colors_buffer_memory);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Render", "Failed to allocate vertex buffer memory!");
    return 0;
  }

  vkBindBufferMemory(_device, _colors_vertex_buffer, _colors_buffer_memory, 0);

  vkMapMemory(_device, _colors_buffer_memory, 0, create_info.size, 0, &buffer_memory);
  memcpy(buffer_memory, colors.data(), (size_t) create_info.size);
  vkUnmapMemory(_device, _colors_buffer_memory);

  // --------------------------------
  // INDICES
  create_info.size = sizeof(uint16_t) * indices.size();
  create_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  result = vkCreateBuffer(_device, &create_info, nullptr, &_indices_buffer);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating indices buffer");
    return 0;
  }

  vkGetBufferMemoryRequirements(_device, _indices_buffer, &memory_requirements);
  allocate_info.allocationSize = memory_requirements.size;

  result = vkAllocateMemory(_device, &allocate_info, nullptr, &_indices_buffer_memory);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Render", "Failed to allocate indices buffer memory!");
    return 0;
  }

  vkBindBufferMemory(_device, _indices_buffer, _indices_buffer_memory, 0);

  vkMapMemory(_device, _indices_buffer_memory, 0, create_info.size, 0, &buffer_memory);
  memcpy(buffer_memory, indices.data(), (size_t) create_info.size);
  vkUnmapMemory(_device, _indices_buffer_memory);
  
  ////////////////////
  // UNIFORM BUFFER
  create_info.size = sizeof(UniformBufferObject);
  create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  result = vkCreateBuffer(_device, &create_info, nullptr, &_uniform_buffer);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating unifrom buffer");
    return 0;
  }

  vkGetBufferMemoryRequirements(_device, _uniform_buffer, &memory_requirements);
  allocate_info.allocationSize = memory_requirements.size;

  result = vkAllocateMemory(_device, &allocate_info, nullptr, &_uniform_buffer_memory);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Render", "Failed to allocate uniform buffer memory!");
    return 0;
  }

  vkBindBufferMemory(_device, _uniform_buffer, _uniform_buffer_memory, 0);

  ////////////////////
  // DESCRIPTORS
  VkDescriptorPoolSize pool_size = {};
  pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_size.descriptorCount = 1;

  VkDescriptorPoolCreateInfo pool_create_info = {};
  pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_create_info.poolSizeCount = 1;
  pool_create_info.pPoolSizes = &pool_size;
  pool_create_info.maxSets = 1;

  result = vkCreateDescriptorPool(_device, &pool_create_info, nullptr, &_descriptor_pool);
  if (result != VK_SUCCESS) {
    LOG_ERROR("Render", "Failed to create descriptor pool!");
    return 0;
  }

  VkDescriptorSetAllocateInfo descriptor_allocate_info = {};
  descriptor_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_allocate_info.descriptorPool = _descriptor_pool;
  descriptor_allocate_info.descriptorSetCount = 1;
  descriptor_allocate_info.pSetLayouts = &_uniform_descriptor_layout;

  result = vkAllocateDescriptorSets(_device, &descriptor_allocate_info, &_descriptor_set);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create descriptor set!");
    return 0;
  }

  VkDescriptorBufferInfo descriptor_buffer_info = {};
  descriptor_buffer_info.buffer = _uniform_buffer;
  descriptor_buffer_info.offset = 0;
  descriptor_buffer_info.range = sizeof(UniformBufferObject);

  VkWriteDescriptorSet descriptor_write = {};
  descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet = _descriptor_set;
  descriptor_write.dstBinding = 0;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_write.descriptorCount = 1;
  descriptor_write.pBufferInfo = &descriptor_buffer_info;

  vkUpdateDescriptorSets(_device, 1, &descriptor_write, 0, nullptr);  

  LOG_DEBUG("Render", "Vertex buffers created succesfully");
  return 1;
}

int Render::createCommandBuffer()
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating command buffer");

  VkCommandPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = _queue_indices.graphics_family;
  pool_info.flags = 0;

  VkResult result = vkCreateCommandPool(_device, &pool_info, nullptr, &_command_pool);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating command pool");
    return 0;
  }

  _command_buffers.resize(_swapchain_framebuffers.size());

  VkCommandBufferAllocateInfo allocate_info = {};
  allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocate_info.commandPool = _command_pool;
  allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocate_info.commandBufferCount = (uint32_t) _command_buffers.size();

  result = vkAllocateCommandBuffers(_device, &allocate_info, _command_buffers.data());
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to allocate command buffers");
    return 0;
  }

  for (size_t i = 0; i < _command_buffers.size(); i++)
  {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    result = vkBeginCommandBuffer(_command_buffers[i], &begin_info);
    if (result != VK_SUCCESS)
    {
      LOG_ERROR("Render", "Failed to begin recording command buffer %d", i);
      return 0;
    }

    VkClearValue clear_color = {{{ 0.06f, 0.06f, 0.06f, 1.0f }}};
    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = _render_pass;
    render_pass_info.framebuffer = _swapchain_framebuffers[i];
    render_pass_info.renderArea.offset = { 0, 0 };
    render_pass_info.renderArea.extent = _swapchain_extent;
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(_command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphics_pipeline);

    VkBuffer vertex_buffers[] = { _positions_vertex_buffer, _colors_vertex_buffer };
    VkDeviceSize offsets[] = { 0, 0 };
    vkCmdBindVertexBuffers(_command_buffers[i], 0, 2, vertex_buffers, offsets);
    vkCmdBindDescriptorSets(_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout, 0, 1, &_descriptor_set, 0, nullptr);
    vkCmdBindIndexBuffer(_command_buffers[i], _indices_buffer, 0, VK_INDEX_TYPE_UINT16);

    // HARDCODED INDEX COUNT
    vkCmdDrawIndexed(_command_buffers[i], 6, 1, 0, 0, 0);
    vkCmdEndRenderPass(_command_buffers[i]);

    result = vkEndCommandBuffer(_command_buffers[i]);
    if (result != VK_SUCCESS)
    {
      LOG_ERROR("Render", "Failed to record command buffer");
      return 0;
    }
  }

  _image_ready_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
  _frame_fences.resize(MAX_FRAMES_IN_FLIGHT);
  _image_fences.resize(_swapchain_images.size(), VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info = {};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    if (vkCreateSemaphore(_device, &semaphore_info, nullptr, &_image_ready_semaphores[i]) != VK_SUCCESS ||
      vkCreateSemaphore(_device, &semaphore_info, nullptr, &_render_finished_semaphores[i]) != VK_SUCCESS ||
      vkCreateFence(_device, &fence_info, nullptr, &_frame_fences[i]) != VK_SUCCESS)
    {
      LOG_ERROR("Render", "Failed creating semaphore");
      return 0;
    }
  }

  LOG_DEBUG("Render", "Command buffer created succesfully");
  return 1;
}

int Render::recreateSwapChain()
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Recreating swapchain");

  vkDeviceWaitIdle(_device);

  cleanup();

  RECT rect;
  GetClientRect(_window, &rect);
  if (!createSwapChain(rect.right - rect.left, rect.bottom - rect.top))
  {
    return 0;
  }

  if (!createRenderPass())
  {
    return 0;
  }

  if (!createGraphicsPipeline())
  {
    return 0;
  }

  if (!createCommandBuffer())
  {
    return 0;
  }

  LOG_DEBUG("Render", "Swapchain recreated susccefully");
  return 1;
}

void Render::update()
{
  static std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();

  std::chrono::steady_clock::time_point current_time = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

  UniformBufferObject uniform = {};
  uniform.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  uniform.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  uniform.projection = glm::perspective(glm::radians(45.0f), _swapchain_extent.width / (float) _swapchain_extent.height, 0.1f, 10.0f);

  uniform.projection[1][1] *= -1;

  void* data;
  vkMapMemory(_device, _uniform_buffer_memory, 0, sizeof(uniform), 0, &data);
  memcpy(data, &uniform, sizeof(uniform));
  vkUnmapMemory(_device, _uniform_buffer_memory);
}

int Render::pickPhysicalDevice(const std::vector<char*>& device_extensions)
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating physical device");

  uint32_t count;
  std::vector<VkPhysicalDevice> devices;
  
  // Query number of available devices
  vkEnumeratePhysicalDevices(_instance, &count, nullptr);

  if (count < 1)
  {
    LOG_ERROR("Render", "Failed to find GPUs with Vulkan support");
    return 0;
  }

  // Query the "real"devices
  devices.resize(count);
  vkEnumeratePhysicalDevices(_instance, &count, &(devices[0]));

  for (size_t i = 0; i < devices.size() && _physical_device == VK_NULL_HANDLE && !_queue_indices.isValid(); i++)
  {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(devices[i], &properties);

    // We just want dedicated GPUs
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {

      uint32_t count;
      QueueFamilyIndices indices = {};
      std::vector<VkQueueFamilyProperties> queues;

      // Query count of queue families
      vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &count, nullptr);

      // Query the queue family properties
      queues.resize(count);
      vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &count, &(queues[0]));

      for (int32_t j = 0; j < count; j++)
      {
        // Check if the queue supports graphic commands
        if (queues[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
          indices.graphics_family = j;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, _surface, &present_support);

        if (present_support)
        {
          indices.present_family = j;
        }
      }

      if (indices.isValid())
      {
        _queue_indices = indices;
        _physical_device = devices[i];
        LOG_DEBUG("Render", "Running on: %s", properties.deviceName);
      }
    }
  }

  if (_physical_device == VK_NULL_HANDLE)
  {
    LOG_ERROR("Render", "Failed to find any suitable GPU");
    return 0;
  }

  return 1;
}

uint32_t Render::findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(_physical_device, &memory_properties);

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
    if ((filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  LOG_ERROR("Render", "Unable to fins right memory type");
  return UINT32_MAX;
}

VkShaderModule Render::createShaderModule(const std::vector<char>& code) const
{
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shader_module = {};
  VkResult result = vkCreateShaderModule(_device, &create_info, nullptr, &shader_module);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed creating shader module");
    return VkShaderModule();
  }

  return shader_module;
}

void Render::cleanup()
{
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    vkDestroyImageView(_device, _swapchain_image_views[i], nullptr);
    vkDestroyFramebuffer(_device, _swapchain_framebuffers[i], nullptr);
  }

  vkFreeCommandBuffers(_device, _command_pool, static_cast<uint32_t>(_command_buffers.size()), _command_buffers.data());

  vkDestroyPipeline(_device, _graphics_pipeline, nullptr);
  vkDestroyPipelineLayout(_device, _pipeline_layout, nullptr);
  vkDestroyRenderPass(_device, _render_pass, nullptr);

  vkDestroySwapchainKHR(_device, _swapchain, nullptr);
}
