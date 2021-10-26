#include "render.h"

#include "logger.h"

#include "glm/common.hpp"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData);

Render::Render() { }

Render::~Render() {
#ifdef DEBUG
  auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");

  if (vkDestroyDebugUtilsMessengerEXT)
  {
    vkDestroyDebugUtilsMessengerEXT(_instance, _debug_messenger, nullptr);
  }
#endif // DEBUG
  vkDestroySwapchainKHR(_device, _swap_chain, nullptr);
  vkDestroyDevice(_device, nullptr);
  vkDestroySurfaceKHR(_instance, _surface, nullptr);
  vkDestroyInstance(_instance, nullptr);
}

int Render::init(HWND window, HINSTANCE instance)
{
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

  std::vector<char*> device_extensions = std::vector<char*>();
  device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  if (!createPhysicalDevice(device_extensions)) {
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

  return 1;
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

  auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)
    vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");

  if (!vkCreateDebugUtilsMessengerEXT) {
    LOG_WARNING("Render", "Failed to set up debug messenger");
    return;
  }
   
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
  QueueFamilyIndices indices = findQueueFamilyIndices(_physical_device);

  if (!indices.isValid())
  {
    return 0;
  }

  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  int32_t queues[] = { indices.graphics_family, indices.present_family };

  for (size_t i = 0; i < sizeof(queues) / sizeof(int32_t); i++)
  {
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_info.queueFamilyIndex = queues[i];
    queue_create_infos.push_back(queue_create_info);
  }

  // Any mandatory feature is required
  VkPhysicalDeviceFeatures device_features = {};

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = (uint32_t) queue_create_infos.size();
  create_info.pQueueCreateInfos = queue_create_infos.data();
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledLayerCount = 0;
  create_info.enabledExtensionCount = device_extensions.size();
  create_info.ppEnabledExtensionNames = device_extensions.data();

  VkResult result = vkCreateDevice(_physical_device, &create_info, nullptr, &_device);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create logical device!");
    return 0;
  }

  vkGetDeviceQueue(_device, indices.graphics_family, 0, &_graphics_queue);
  vkGetDeviceQueue(_device, indices.present_family, 0, &_graphics_queue);

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
  
  QueueFamilyIndices indices = findQueueFamilyIndices(_physical_device);
  uint32_t queue_family_indices[] = { indices.graphics_family, indices.present_family };
  if (indices.graphics_family != indices.present_family)
  {
    // VK_SHARING_MODE_EXCLUSIVE should be better for performance
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = sizeof(queue_family_indices) / sizeof(uint32_t);
    create_info.pQueueFamilyIndices = queue_family_indices;
  }
  else
  {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices = nullptr;
  }

  VkResult result = vkCreateSwapchainKHR(_device, &create_info, nullptr, &_swap_chain);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create SwapChain");
    return 0;
  }

  vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, nullptr);
  _swapchain_images.resize(image_count);
  vkGetSwapchainImagesKHR(_device, _swap_chain, &image_count, _swapchain_images.data());

  _swapchain_extent = surface_extent;
  _swapchain_format = surface_format.format;

  LOG_DEBUG("Render", "Swapchain created succesfully");
  return 1;
}

int Render::createPhysicalDevice(const std::vector<char*>& device_extensions)
{
  std::cout << "\n";
  LOG_DEBUG("Render", "Creating physical device");

  uint32_t count;
  vkEnumeratePhysicalDevices(_instance, &count, nullptr);

  if (count < 1)
  {
    LOG_ERROR("Render", "Failed to find GPUs with Vulkan support");
    return 0;
  }

  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(_instance, &count, &(devices[0]));

  for (int i = 0; i < devices.size() && _physical_device == VK_NULL_HANDLE; i++)
  {
    if (isDeviceSuitable(devices[i], device_extensions))
    {
      _physical_device = devices[i];
    }
  }

  if (_physical_device == VK_NULL_HANDLE)
  {
    LOG_ERROR("Render", "Failed to find any suitable GPU");
    return 0;
  }

  return 1;
}

bool Render::isDeviceSuitable(const VkPhysicalDevice& device, const std::vector<char*>& device_extensions) const
{
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceFeatures(device, &features);
  vkGetPhysicalDeviceProperties(device, &properties);

  // We just want dedicated GPUs
  if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
  {
    return false;
  }

  // Check if the GPU supports our required queues
  if (!findQueueFamilyIndices(device).isValid())
  {
    return false;
  }

  // Check for device extensions
  uint32_t count;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
  std::vector<VkExtensionProperties> available_extensions(count);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available_extensions.data());

  std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
  for (const VkExtensionProperties& propertie : available_extensions)
  {
    required_extensions.erase(propertie.extensionName);
  }

  if (!required_extensions.empty())
  {
    return false;
  }

  LOG_DEBUG("Render", "Running on: %s", properties.deviceName);

  return true;
}

QueueFamilyIndices Render::findQueueFamilyIndices(const VkPhysicalDevice& device) const
{
  QueueFamilyIndices indices = {};
  uint32_t count;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
  std::vector<VkQueueFamilyProperties> queues(count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &count, &(queues[0]));

  for (int32_t i = 0; i < count; i++)
  {
    if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphics_family = i;
    }

    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &present_support);

    if (present_support)
    {
      indices.present_family = i;
    }
  }

  return indices;
}
