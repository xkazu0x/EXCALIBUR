#pragma once

#include "vk_types.h"

namespace vk_swapchain {
	void create(
		vulkan_context *context, 
		uint32_t width, 
		uint32_t height, 
		vulkan_swapchain *out_swapchain);
	void recreate(
		vulkan_context *context,
		uint32_t width,
		uint32_t height,
		vulkan_swapchain *swapchain);
	void destroy(vulkan_context *context, vulkan_swapchain *swapchain);
	bool acquire_next_image_index(
		vulkan_context *context,
		vulkan_swapchain *swapchain,
		uint64_t timeout_ns,
		VkSemaphore image_available_semaphore,
		VkFence fence,
		uint32_t *out_image_index);
	void present(
		vulkan_context *context,
		vulkan_swapchain *swapchain,
		VkQueue graphics_queue,
		VkQueue present_queue,
		VkSemaphore render_complete_semaphore,
		uint32_t present_image_index);
}