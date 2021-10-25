#include "render.h"

#include <vector>
#include <iostream>

#include "logger.h"

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

  vkDestroyInstance(_instance, nullptr);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {

  switch (messageSeverity)
  {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
    LOG_WARNING("VALIDATION LAYER", pCallbackData->pMessage);

    break;
  }
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
    LOG_ERROR("VALIDATION LAYER", pCallbackData->pMessage);
    break;
  }
  default: {
    LOG_DEBUG("VALIDATION LAYER", pCallbackData->pMessage);
    break;
  }
  }

  return VK_FALSE;
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

#if DEBUG
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
  }
  else
  {
    vkCreateDebugUtilsMessengerEXT(_instance, &debug_create_info, nullptr, &_debug_messenger);
  }

#endif // DEBUG

  return 1;
}
