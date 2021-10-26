#ifndef __RENDER_H__
#define __RENDER_H__ 1

#include "vulkan/vulkan.h"

class Render
{
public:
	Render();
	~Render();

	int init();

private:
	void createDebuger();
	int createPhysicalDevice();
	bool isDeviceSuitable(const VkPhysicalDevice& device) const;

	VkInstance _instance = VK_NULL_HANDLE;
	VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;
};

#endif // !__RENDER_H__
