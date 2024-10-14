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

struct vulkan_swapchain {
	VkSurfaceFormatKHR image_format;
	uint8_t max_frames_in_flight;
	VkSwapchainKHR handle;
	uint32_t image_count;
	VkImage *images;
	VkImageView *views;
	vulkan_image depth_attachment;
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
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint32_t image_index;
	uint32_t current_frame;

	bool recreating_swapchain;

	int32_t(*find_memory_index)(uint32_t type_filter, uint32_t property_flags);
};