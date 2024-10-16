#pragma once

#include <vulkan/vulkan.h>
#include <assert.h>
#include <vector>

#define VKCHECK(expr) { assert(expr == VK_SUCCESS); }

struct vulkan_swapchain_support_info {
	VkSurfaceCapabilitiesKHR capabilities;
	unsigned int format_count;
	VkSurfaceFormatKHR *formats;
	unsigned int present_mode_count;
	VkPresentModeKHR *present_modes;
};

struct vulkan_device {
	VkPhysicalDevice physical_device;
	VkDevice logical_device;

	vulkan_swapchain_support_info swapchain_support;
	int graphics_queue_index;
	int present_queue_index;
	int compute_queue_index;
	int transfer_queue_index;

	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;

	VkCommandPool graphics_command_pool;

	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceMemoryProperties memory;

	VkFormat depth_format;
};

struct vulkan_image {
	VkImage handle;
	VkDeviceMemory memory;
	VkImageView view;
	uint32_t width;
	uint32_t height;
};

enum vulkan_renderpass_state {
	READY,
	RECORDING,
	IN_RENDER_PASS,
	RECORDING_ENDED,
	SUBMITTED,
	NOT_ALLOCATED
};

struct vulkan_renderpass {
	VkRenderPass handle;
	float x, y, w, h;
	float r, g, b, a;
	float depth;
	uint32_t stencil;
};

struct vulkan_swapchain {
	VkSurfaceFormatKHR image_format;
	uint8_t max_frames_in_flight;
	VkSwapchainKHR handle;
	uint32_t image_count;
	VkImage *images;
	VkImageView *views;
	vulkan_image depth_attachment;
};

enum vulkan_command_buffer_state {
	COMMAND_BUFFER_STATE_READY,
	COMMAND_BUFFER_STATE_RECORDING,
	COMMAND_BUFFER_STATE_IN_RENDER_PASS,
	COMMAND_BUFFER_STATE_RECORDING_ENDED,
	COMMAND_BUFFER_STATE_SUBMITTED,
	COMMAND_BUFFER_STATE_NOT_ALLOCATED,
};

struct vulkan_command_buffer {
	VkCommandBuffer handle;
	vulkan_command_buffer_state state;
};

struct vulkan_context {
	VkInstance instance;
	VkAllocationCallbacks *allocator;
#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
	VkSurfaceKHR surface;
	vulkan_device device;

	vulkan_swapchain swapchain;
	vulkan_renderpass main_renderpass;

	std::vector<vulkan_command_buffer> graphics_command_buffers;

	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint32_t image_index;
	uint32_t current_frame;

	bool recreating_swapchain;

	int32_t(*find_memory_index)(uint32_t type_filter, uint32_t property_flags);
};