#include "vk_renderpass.h"

#include "core/sxlogger.h"
#include "core/sxmemory.h"

void vk_renderpass::create(
	vulkan_context *context,
	vulkan_renderpass *out_renderpass,
	float x, float y, float w, float h,
	float r, float g, float b, float a,
	float depth,
	uint32_t stencil) {
	SXDEBUG("CREATING VULKAN RENDERPASS");
	
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// TODO: make this configurable
	const uint32_t attachment_description_count = 2;
	VkAttachmentDescription attachment_descriptions[attachment_description_count];

	VkAttachmentDescription color_attachment = {};
	color_attachment.flags = 0;
	color_attachment.format = context->swapchain.image_format.format; // TODO: configurable
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachment_descriptions[0] = color_attachment;

	VkAttachmentReference color_attachment_reference = {};
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_reference;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.flags = 0;
	depth_attachment.format = context->device.depth_format;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment_descriptions[1] = depth_attachment;

	VkAttachmentReference depth_attachment_reference = {};
	depth_attachment_reference.attachment = 1;
	depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	subpass.pDepthStencilAttachment = &depth_attachment_reference;
	
	// TODO: other attachment types (input, resolve, preserve)
	// input from shader
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = 0;

	// used for multisampling
	subpass.pResolveAttachments = 0;

	// attachment not used in this subpass, but must be preserved for the next
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = 0;

	// TODO: make this confiurable
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo renderpass_create_info = {};
	renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpass_create_info.flags = 0;
	renderpass_create_info.attachmentCount = attachment_description_count;
	renderpass_create_info.pAttachments = attachment_descriptions;
	renderpass_create_info.subpassCount = 1;
	renderpass_create_info.pSubpasses = &subpass;
	renderpass_create_info.dependencyCount = 1;
	renderpass_create_info.pDependencies = &dependency;

	VKCHECK(vkCreateRenderPass(
		context->device.logical_device, 
		&renderpass_create_info, 
		context->allocator, 
		&out_renderpass->handle));

	SXDEBUG("VULKAN RENDERPASS CREATED");
}

void vk_renderpass::destroy(vulkan_context *context, vulkan_renderpass *renderpass) {
	SXDEBUG("DESTROYING VULKAN RENDERPASS");
	if (renderpass && renderpass->handle) {
		vkDestroyRenderPass(context->device.logical_device, renderpass->handle, context->allocator);
		renderpass->handle = 0;
	}
}

void vk_renderpass::begin(
	vulkan_command_buffer *command_buffer,
	vulkan_renderpass *renderpass,
	VkFramebuffer framebuffer) {
	VkRenderPassBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.renderPass = renderpass->handle;
	begin_info.framebuffer = framebuffer;
	begin_info.renderArea.offset.x = static_cast<int32_t>(renderpass->x);
	begin_info.renderArea.offset.y = static_cast<int32_t>(renderpass->y);
	begin_info.renderArea.extent.width = static_cast<uint32_t>(renderpass->w);
	begin_info.renderArea.extent.height = static_cast<uint32_t>(renderpass->h);

	VkClearValue clear_values[2];
	sxmemory::zero_memory(clear_values, sizeof(VkClearValue) * 2);
	clear_values[0].color.float32[0] = renderpass->r;
	clear_values[0].color.float32[1] = renderpass->g;
	clear_values[0].color.float32[2] = renderpass->b;
	clear_values[0].color.float32[3] = renderpass->a;
	clear_values[1].depthStencil.depth = renderpass->depth;
	clear_values[1].depthStencil.stencil = renderpass->stencil;

	begin_info.clearValueCount = 2;
	begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(command_buffer->handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
	command_buffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vk_renderpass::end(vulkan_command_buffer *command_buffer, vulkan_renderpass *renderpass) {
	vkCmdEndRenderPass(command_buffer->handle);
	command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}