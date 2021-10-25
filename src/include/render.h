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
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;

};

#endif // !__RENDER_H__
