#pragma once

#include "vk_types.h"

namespace vk_renderpass {
	void create(
		vulkan_context *context,
		vulkan_renderpass *out_renderpass,
		float x, float y, float width, float height,
		float r, float g, float b, float a,
		float depth,
		uint32_t stencil);
	void destroy(vulkan_context *context, vulkan_renderpass *renderpass);
	void begin(
		vulkan_command_buffer *command_buffer,
		vulkan_renderpass *renderpass,
		VkFramebuffer framebuffer);
	void end(vulkan_command_buffer *command_buffer, vulkan_renderpass *renderpass);
}