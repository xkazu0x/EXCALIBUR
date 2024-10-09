#pragma once

#include "vk_types.h"

namespace vk_device {
	bool create(vulkan_context *context);
	void destroy(vulkan_context *context);
	void query_swapchain_support(
		VkPhysicalDevice physical_device, 
		VkSurfaceKHR surface, 
		vulkan_swapchain_support_info *out_support_info);
}