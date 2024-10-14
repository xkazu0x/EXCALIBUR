#include "vk_image.h"
#include "vk_device.h"
#include "core/sxlogger.h"

void vk_image::create(
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
	vulkan_image *out_image) {
	out_image->width = width;
	out_image->height = height;

	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = format;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1; // TODO: support configurable depth
	image_create_info.mipLevels = 4;	// TODO: mip mapping
	image_create_info.arrayLayers = 1;	// TODO: support number of layers in the image
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;			// TODO: configurable sample count
	image_create_info.tiling = tiling;
	image_create_info.usage = usage;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// TODO: configurable sharing mode
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VKCHECK(vkCreateImage(
		context->device.logical_device, 
		&image_create_info, 
		context->allocator, 
		&out_image->handle));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(
		context->device.logical_device, 
		out_image->handle, 
		&memory_requirements);

	int32_t memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memory_flags);
	if (memory_type == -1) {
		SXERROR("REQUIRED MEMORY TYPE NOT FOUND");
	}

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = memory_type;
	VKCHECK(vkAllocateMemory(
		context->device.logical_device, 
		&memory_allocate_info, 
		context->allocator, 
		&out_image->memory));
	VKCHECK(vkBindImageMemory(
		context->device.logical_device, 
		out_image->handle, 
		out_image->memory, 
		0));	// TODO: configurable memory offset
	if (create_view) {
		out_image->view = 0;
		vk_image::view_create(context, format, out_image, view_aspect_flags);
	}
}

void vk_image::view_create(
	vulkan_context *context,
	VkFormat format,
	vulkan_image *image,
	VkImageAspectFlags aspect_flags) {
	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = image->handle;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D; // TODO: make configurable
	view_create_info.format = format;
	view_create_info.subresourceRange.aspectMask = aspect_flags;
	// TODO: make configurable
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;
	VKCHECK(vkCreateImageView(
		context->device.logical_device, 
		&view_create_info, 
		context->allocator, 
		&image->view));
}

void vk_image::destroy(vulkan_context *context, vulkan_image *image) {
	if (image->view) {
		vkDestroyImageView(context->device.logical_device, image->view, context->allocator);
		image->view = 0;
	}
	if (image->memory) {
		vkFreeMemory(context->device.logical_device, image->memory, context->allocator);
		image->memory = 0;
	}
	if (image->handle) {
		vkDestroyImage(context->device.logical_device, image->handle, context->allocator);
		image->handle = 0;
	}
}