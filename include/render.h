#ifndef __RENDER_H__
#define __RENDER_H__ 1

#include <Windows.h>

#include <set>
#include <vector>
#include <string>
#include <iostream>

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
	void drawFrame();

	bool _resize = false;
	VkDevice _device = VK_NULL_HANDLE;
private:
	void createDebuger();
	int createSurface(HWND window, HINSTANCE instance);
	int pickPhysicalDevice(const std::vector<char*>& device_extensions);
	int createLogicalDevice(const std::vector<char*>& device_extensions);
	int createSwapChain(int width, int height);
	int createRenderPass();
	int createGraphicsPipeline();
	int createVertexBuffers();
	int createCommandBuffer();

	int recreateSwapChain();
	void update();

	uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags properties);
	VkShaderModule createShaderModule(const std::vector<char>& code) const;
	void cleanup();

	VkInstance _instance = VK_NULL_HANDLE;
	
	VkQueue _graphics_queue = VK_NULL_HANDLE;
	VkQueue _present_queue = VK_NULL_HANDLE;

	std::vector<VkCommandBuffer> _command_buffers;
	VkCommandPool _command_pool = VK_NULL_HANDLE;
	VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
	VkDescriptorSet _descriptor_set = VK_NULL_HANDLE;

	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
	VkFormat _swapchain_format;
	VkExtent2D _swapchain_extent;
	std::vector<VkImage> _swapchain_images;
	std::vector<VkImageView> _swapchain_image_views;
	std::vector<VkFramebuffer> _swapchain_framebuffers;

	VkPhysicalDevice _physical_device = VK_NULL_HANDLE;
	
	VkPipeline _graphics_pipeline = VK_NULL_HANDLE;
	VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout _uniform_descriptor_layout = VK_NULL_HANDLE;
	VkRenderPass _render_pass = VK_NULL_HANDLE;

	VkSemaphore _image_ready_semaphore;
	VkSemaphore _render_finished_semaphore;
	VkFence _frame_fence;

	VkBuffer _positions_vertex_buffer;
	VkBuffer _colors_vertex_buffer;
	VkBuffer _indices_buffer;
	VkBuffer _uniform_buffer;
	VkDeviceMemory _positions_buffer_memory;
	VkDeviceMemory _colors_buffer_memory;
	VkDeviceMemory _indices_buffer_memory;
	VkDeviceMemory _uniform_buffer_memory;

	VkDebugUtilsMessengerEXT _debug_messenger = VK_NULL_HANDLE;

	QueueFamilyIndices _queue_indices = {};

	HWND _window;
};

#endif // !__RENDER_H__
