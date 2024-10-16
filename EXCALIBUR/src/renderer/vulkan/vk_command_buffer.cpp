#include "vk_command_buffer.h"

#include "core/sxlogger.h"
#include "core/sxmemory.h"

void vk_command_buffer::allocate(
	vulkan_context *context,
	VkCommandPool pool,
	bool is_primary,
	vulkan_command_buffer *out_command_buffer) {
	sxmemory::zero_memory(out_command_buffer, sizeof(vulkan_command_buffer));

	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = pool;
	allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocate_info.commandBufferCount = 1;

	out_command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
	VKCHECK(vkAllocateCommandBuffers(
		context->device.logical_device,
		&allocate_info,
		&out_command_buffer->handle));
	out_command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vk_command_buffer::free(
	vulkan_context *context,
	VkCommandPool pool,
	vulkan_command_buffer *command_buffer) {
	vkFreeCommandBuffers(context->device.logical_device, pool, 1, &command_buffer->handle);
	command_buffer->handle = 0;
	command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

void vk_command_buffer::begin(
	vulkan_command_buffer *command_buffer,
	bool is_single_use,
	bool is_renderpass_continue,
	bool is_simultaneous_use) {
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	if (is_single_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (is_renderpass_continue) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	if (is_simultaneous_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VKCHECK(vkBeginCommandBuffer(command_buffer->handle, &begin_info));
	command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void vk_command_buffer::end(vulkan_command_buffer *command_buffer) {
	VKCHECK(vkEndCommandBuffer(command_buffer->handle));
	command_buffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
}

void vk_command_buffer::update_submitted(vulkan_command_buffer *command_buffer) {
	command_buffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
}

void vk_command_buffer::reset(vulkan_command_buffer *command_buffer) {
	command_buffer->state = COMMAND_BUFFER_STATE_READY;
}

void vk_command_buffer::allocate_and_begin_single_use(
	vulkan_context *context,
	VkCommandPool pool,
	vulkan_command_buffer *out_command_buffer) {
	vk_command_buffer::allocate(context, pool, true, out_command_buffer);
	vk_command_buffer::begin(out_command_buffer, true, false, false);
}

void vk_command_buffer::end_single_use(
	vulkan_context *context,
	VkCommandPool pool,
	vulkan_command_buffer *command_buffer,
	VkQueue queue) {
	vk_command_buffer::end(command_buffer);
	
	// submit the queue
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer->handle;

	VKCHECK(vkQueueSubmit(queue, 1, &submit_info, 0));

	// wait for it to finish
	VKCHECK(vkQueueWaitIdle(queue));

	vk_command_buffer::free(context, pool, command_buffer);
}