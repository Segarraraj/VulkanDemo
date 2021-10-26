#ifndef __RENDER_H__
#define __RENDER_H__ 1

#include <Windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"

struct QueueFamilyIndices
{
	int32_t graphics_family = -1;
	int32_t present_family = -1;

	bool isValid() {
		return graphics_family != -1 && present_family != -1;
	}
};

class Render
{
public:
	Render();
	~Render();

	int init(HWND window, HINSTANCE instance);

private:
	void createDebuger();
	int createSurface(HWND window, HINSTANCE instance);
	int createPhysicalDevice();
	int createLogicalDevice();

	bool isDeviceSuitable(const VkPhysicalDevice& device) const;
	QueueFamilyIndices findQueueFamilyIndices(const VkPhysicalDevice& device) const;

	VkInstance _instance = VK_NULL_HANDLE;
	
	VkQueue _graphics_queue = VK_NULL_HANDLE;
	VkQueue _present_queue = VK_NULL_HANDLE;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
	VkPhysicalDevice _physical_device = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
};

#endif // !__RENDER_H__
