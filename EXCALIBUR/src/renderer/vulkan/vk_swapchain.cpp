#include "vk_swapchain.h"
#include "vk_device.h"
#include "vk_image.h"
#include "core/sxlogger.h"
#include "core/sxmemory.h"

#include "defines.h"

void create_swapchain(vulkan_context *context, uint32_t width, uint32_t height, vulkan_swapchain *swapchain);
void destroy_swapchain(vulkan_context *context, vulkan_swapchain *swapchain);

void vk_swapchain::create(
	vulkan_context *context,
	uint32_t width,
	uint32_t height,
	vulkan_swapchain *out_swapchain) {
	create_swapchain(context, width, height, out_swapchain);
}

void vk_swapchain::recreate(
	vulkan_context *context,
	uint32_t width,
	uint32_t height,
	vulkan_swapchain *swapchain) {
	destroy_swapchain(context, swapchain);
	create_swapchain(context, width, height, swapchain);
}

void vk_swapchain::destroy(vulkan_context *context, vulkan_swapchain *swapchain) {
	destroy_swapchain(context, swapchain);
}

bool vk_swapchain::acquire_next_image_index(
	vulkan_context *context,
	vulkan_swapchain *swapchain,
	uint64_t timeout_ns,
	VkSemaphore image_available_semaphore,
	VkFence fence,
	uint32_t *out_image_index) {
	VkResult result = vkAcquireNextImageKHR(
		context->device.logical_device, 
		swapchain->handle,
		timeout_ns,
		image_available_semaphore,
		fence,
		out_image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		vk_swapchain::recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
		return false;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		SXERROR("FAILED TO ACQUIRE SWAPCHAIN IMAGE");
		return false;
	}

	return true;
}

void vk_swapchain::present(
	vulkan_context *context,
	vulkan_swapchain *swapchain,
	VkQueue graphics_queue,
	VkQueue present_queue,
	VkSemaphore render_complete_semaphore,
	uint32_t present_image_index) {
	VkPresentInfoKHR present_info = {};	
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_complete_semaphore;
	present_info.swapchainCount = 1;
	//present_info.pSwapchains = &swapchain->handle;
	present_info.pImageIndices = &present_image_index;
	present_info.pResults = 0;

	VkResult result = vkQueuePresentKHR(present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		vk_swapchain::recreate(context, context->framebuffer_width, context->framebuffer_height, swapchain);
	} else if (result != VK_SUCCESS) {
		SXERROR("FAILED TO PRESENT SWAPCHAIN IMAGE");
	}
}

void create_swapchain(vulkan_context *context, uint32_t width, uint32_t height, vulkan_swapchain *swapchain) {
	SXDEBUG("CREATING VULKAN SWAPCHAIN");
	VkExtent2D swapchain_extent = { width, height };
	swapchain->max_frames_in_flight = 2;

	bool found = false;
	for (uint32_t i = 0; i < context->device.swapchain_support.format_count; ++i) {
		VkSurfaceFormatKHR format = context->device.swapchain_support.formats[i];
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			swapchain->image_format = format;
			found = true;
			break;
		}
	}
	if (!found) {
		swapchain->image_format = context->device.swapchain_support.formats[0];
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < context->device.swapchain_support.present_mode_count; ++i) {
		VkPresentModeKHR mode = context->device.swapchain_support.present_modes[i];
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = mode;
			break;
		}
	}

	vk_device::query_swapchain_support(
		context->device.physical_device, 
		context->surface, 
		&context->device.swapchain_support);
	if (context->device.swapchain_support.capabilities.currentExtent.width != UINT32_MAX) {
		swapchain_extent = context->device.swapchain_support.capabilities.currentExtent;
	}

	VkExtent2D min = context->device.swapchain_support.capabilities.minImageExtent;
	VkExtent2D max = context->device.swapchain_support.capabilities.maxImageExtent;
	swapchain_extent.width = SXCLAMP(swapchain_extent.width, min.width, max.width);
	swapchain_extent.height = SXCLAMP(swapchain_extent.height, min.height, max.height);

	uint32_t image_count = context->device.swapchain_support.capabilities.minImageCount + 1;
	if (context->device.swapchain_support.capabilities.maxImageCount > 0 &&
		image_count > context->device.swapchain_support.capabilities.maxImageCount) {
		image_count = context->device.swapchain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.surface = context->surface;
	swapchain_create_info.minImageCount = image_count;
	swapchain_create_info.imageFormat = swapchain->image_format.format;
	swapchain_create_info.imageColorSpace = swapchain->image_format.colorSpace;
	swapchain_create_info.imageExtent = swapchain_extent;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (context->device.graphics_queue_index != context->device.present_queue_index) {
		uint32_t queue_family_indices[] = { 
			static_cast<uint32_t>(context->device.graphics_queue_index), 
			static_cast<uint32_t>(context->device.present_queue_index) };
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_create_info.queueFamilyIndexCount = 0;
		swapchain_create_info.pQueueFamilyIndices = 0;
	}
	swapchain_create_info.preTransform = context->device.swapchain_support.capabilities.currentTransform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = 0;

	VKCHECK(vkCreateSwapchainKHR(
		context->device.logical_device, 
		&swapchain_create_info, 
		context->allocator, 
		&swapchain->handle));

	context->current_frame = 0;

	// images
	swapchain->image_count = 0;
	VKCHECK(vkGetSwapchainImagesKHR(
		context->device.logical_device, 				
		swapchain->handle, 
		&swapchain->image_count, 
		0));
	if (!swapchain->images) {
		swapchain->images = static_cast<VkImage *>(
			sxmemory::allocate(
				sizeof(VkImage) * swapchain->image_count,
				MEMORY_TAG_RENDERER));
	}
	if (!swapchain->views) {
		swapchain->views = static_cast<VkImageView *>(
			sxmemory::allocate(
				sizeof(VkImageView) * swapchain->image_count,
				MEMORY_TAG_RENDERER));
	}

	VKCHECK(vkGetSwapchainImagesKHR(
		context->device.logical_device,
		swapchain->handle,
		&swapchain->image_count,
		swapchain->images));

	// views
	for (uint32_t i = 0; i < swapchain->image_count; ++i) {
		VkImageViewCreateInfo view_info = {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = swapchain->images[i];
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = swapchain->image_format.format;
		view_info.components.r = VK_COMPONENT_SWIZZLE_R;
		view_info.components.g = VK_COMPONENT_SWIZZLE_G;
		view_info.components.b = VK_COMPONENT_SWIZZLE_B;
		view_info.components.a = VK_COMPONENT_SWIZZLE_A;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;
		view_info.flags = 0;
		VKCHECK(vkCreateImageView(
			context->device.logical_device, 
			&view_info, 
			context->allocator,
			&swapchain->views[i]));
	}

	// depth resources
	if (!vk_device::detect_depth_format(&context->device)) {
		context->device.depth_format = VK_FORMAT_UNDEFINED;
		SXERROR("FAILED TO FIND A SUPPORTED FORMAT");
	}

	// depth image
	vk_image::create(
		context,
		VK_IMAGE_TYPE_2D,
		swapchain_extent.width,
		swapchain_extent.height,
		context->device.depth_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		true,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		&swapchain->depth_attachment);
	SXDEBUG("VULKAN SWAPCHAIN CREATED");
}

void destroy_swapchain(vulkan_context *context, vulkan_swapchain *swapchain) {
	SXDEBUG("DESTROYING VULKAN SWAPCHAIN");
	vk_image::destroy(context, &swapchain->depth_attachment);
	for (uint32_t i = 0; i < swapchain->image_count; ++i) {
		vkDestroyImageView(context->device.logical_device, swapchain->views[i], context->allocator);
	}
	sxmemory::free_memory(swapchain->images, sizeof(VkImage) * swapchain->image_count, MEMORY_TAG_RENDERER);
	sxmemory::free_memory(swapchain->views, sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);

	vkDestroySwapchainKHR(context->device.logical_device, swapchain->handle, context->allocator);
}