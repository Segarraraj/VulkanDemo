#include "render.h"

#include <vector>
#include <iostream>

#include "logger.h"



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
  vkDestroyDevice(_device, nullptr);
  vkDestroyInstance(_instance, nullptr);
}

int Render::init()
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

  if (!createPhysicalDevice()) {
    return 0;
  }

  if (!createLogicalDevice()) {
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

int Render::createLogicalDevice()
{
  QueueFamilyIndices indices = findQueueFamilyIndices(_physical_device);

  if (!indices.isValid())
  {
    return 0;
  }

  float queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_create_info = {};
  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueCount = 1;
  queue_create_info.pQueuePriorities = &queue_priority;
  queue_create_info.queueFamilyIndex = indices.graphics_family;

  // Any mandatory feature is required
  VkPhysicalDeviceFeatures device_features = {};

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.queueCreateInfoCount = 1;
  create_info.pQueueCreateInfos = &queue_create_info;
  create_info.pEnabledFeatures = &device_features;
  create_info.enabledLayerCount = 0;

  VkResult result = vkCreateDevice(_physical_device, &create_info, nullptr, &_device);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Render", "Failed to create logical device!");
    return 0;
  }

  vkGetDeviceQueue(_device, indices.graphics_family, 0, &_graphics_queue);

  return 1;
}

int Render::createPhysicalDevice()
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
    if (isDeviceSuitable(devices[i]))
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

bool Render::isDeviceSuitable(const VkPhysicalDevice& device) const
{
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceFeatures(device, &features);
  vkGetPhysicalDeviceProperties(device, &properties);

  if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
  {
    return false;
  }

  if (!findQueueFamilyIndices(device).isValid())
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
  }

  return indices;
}
