#pragma once

#include "vk_types.h"

namespace vk_image {
	void create(
		vulkan_context *context,
		VkImageType image_type,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memory_flags,
		bool create_view,
		VkImageAspectFlags view_aspect_flags,
		vulkan_image *out_image);
	void view_create(
		vulkan_context *context,
		VkFormat format,
		vulkan_image *image,
		VkImageAspectFlags aspect_flags);
	void destroy(vulkan_context *context, vulkan_image *image);
}