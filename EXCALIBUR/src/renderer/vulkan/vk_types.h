#pragma once

#include <vulkan/vulkan.h>
#include <assert.h>
#include <vector>

#define VKCHECK(expr) { assert(expr == VK_SUCCESS); }

struct vulkan_swapchain_support_info {
	VkSurfaceCapabilitiesKHR capabilities;
	unsigned int format_count;
	std::vector<VkSurfaceFormatKHR> formats;
	unsigned int present_mode_count;
	std::vector<VkPresentModeKHR> present_modes;
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
};

struct vulkan_context {
	VkInstance instance;
	VkAllocationCallbacks *allocator;
#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
	VkSurfaceKHR surface;
	vulkan_device device;
};