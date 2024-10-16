#pragma once

#include "vk_types.h"

namespace vk_command_buffer {
	void allocate(
		vulkan_context *context,
		VkCommandPool pool,
		bool is_primary,
		vulkan_command_buffer *out_command_buffer);
	void free(
		vulkan_context *context,
		VkCommandPool pool,
		vulkan_command_buffer *command_buffer);
	void begin(
		vulkan_command_buffer *command_buffer,
		bool is_single_use,
		bool is_renderpass_continue,
		bool is_simultaneous_use);
	void end(vulkan_command_buffer *command_buffer);
	void update_submitted(vulkan_command_buffer *command_buffer);
	void reset(vulkan_command_buffer *command_buffer);

	/**
	 * Allocates and begins recording to out_command_buffer
	 */
	void allocate_and_begin_single_use(
		vulkan_context *context,
		VkCommandPool pool,
		vulkan_command_buffer *out_command_buffer);

	/**
	 * Ends recording, submits to and waits for queue operation and frees the provided command buffer
	 */
	void end_single_use(
		vulkan_context *context,
		VkCommandPool pool,
		vulkan_command_buffer *command_buffer,
		VkQueue queue);
}