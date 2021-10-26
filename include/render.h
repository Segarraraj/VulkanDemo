#ifndef __RENDER_H__
#define __RENDER_H__ 1

#include "vulkan/vulkan.h"

struct QueueFamilyIndices
{
	int32_t graphics_family = -1;

	bool isValid() {
		return graphics_family != -1;
	}
};

class Render
{
public:
	Render();
	~Render();

	int init();

private:
	void createDebuger();
	int createLogicalDevice();
	int createPhysicalDevice();

	bool isDeviceSuitable(const VkPhysicalDevice& device) const;
	QueueFamilyIndices findQueueFamilyIndices(const VkPhysicalDevice& device) const;

	VkInstance _instance = VK_NULL_HANDLE;
	
	VkQueue _graphics_queue = VK_NULL_HANDLE;

	VkDevice _device = VK_NULL_HANDLE;
	VkPhysicalDevice _physical_device = VK_NULL_HANDLE;

	VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
};

#endif // !__RENDER_H__
