#include "vk_backend.h"
#include "vk_instance.h"
#include "vk_platform.h"
#include "vk_device.h"
#include "vk_swapchain.h"
#include "vk_renderpass.h"
#include "vk_command_buffer.h"

#include "core/sxlogger.h"
#include "core/sxmemory.h"

static vulkan_context context;

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags);
void create_command_buffers();

bool vk_backend::initialize(platform_state *platform) {
	context.find_memory_index = find_memory_index;
	context.allocator = 0;

	SXDEBUG("INITIALIZING VULKAN RENDERER");
	if (!vk_instance::create(&context)) {
		SXFATAL("FAILED TO CREATE VULKAN INSTANCE");
		return false;
	}

	if (!vk_platform::create_surface(platform, &context)) {
		SXFATAL("FAILED TO CREATE VULKAN SURFACE");
		return false;
	}

	if (!vk_device::create(&context)) {
		SXFATAL("FAILED TO CREATE VULKAN DEVICE");
		return false;
	}

	vk_swapchain::create(
		&context,
		context.framebuffer_width,
		context.framebuffer_height,
		&context.swapchain);

	vk_renderpass::create(
		&context,
		&context.main_renderpass,
		0, 0, (float)context.framebuffer_width, (float)context.framebuffer_height,
		0.2f, 0.2f, 0.3f, 1.0f,
		1.0f,
		0);

	create_command_buffers();

	SXDEBUG("VULKAN RENDERER INITIALIZED SUCCESSFULLY");
	return true;
}

void vk_backend::shutdown() {
	SXDEBUG("SHUTTING DOWN VULKAN BACKEND");

	SXDEBUG("RELEASING VULKAN COMMAND BUFFERS");
	for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
		if (context.graphics_command_buffers[i].handle) {
			vk_command_buffer::free(
				&context,
				context.device.graphics_command_pool,
				&context.graphics_command_buffers[i]);
			context.graphics_command_buffers[i].handle = 0;
		}
	}
	// NOTE: destroy command buffers array

	vk_renderpass::destroy(&context, &context.main_renderpass);

	vk_swapchain::destroy(&context, &context.swapchain);

	vk_device::destroy(&context);

	if (context.surface) {
		SXDEBUG("DESTROYING VULKAN SURFACE");
		vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
		context.surface = 0;
	}

	vk_instance::destroy(&context);
}

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(context.device.physical_device, &memory_properties);
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
		if (type_filter & (1 << i) &&
			(memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
			return i;
		}
	}
	SXWARN("UNABLE TO FIND SUITABLE MEMORY TYPE");
	return -1;
}

void create_command_buffers() {
	SXDEBUG("CREATING VULKAN COMMAND BUFFERS");

	if (!context.graphics_command_buffers.data()) {
		context.graphics_command_buffers.resize(context.swapchain.image_count);
		for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
			sxmemory::zero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
		}
	}

	for (uint32_t i = 0; i < context.swapchain.image_count; ++i) {
		if (context.graphics_command_buffers[i].handle) {
			vk_command_buffer::free(
				&context,
				context.device.graphics_command_pool,
				&context.graphics_command_buffers[i]);
		}
		sxmemory::zero_memory(&context.graphics_command_buffers[i], sizeof(vulkan_command_buffer));
		vk_command_buffer::allocate(&context, context.device.graphics_command_pool, true, &context.graphics_command_buffers[i]);
	}

	SXDEBUG("VULKAN COMMAND BUFFERS CREATED");
}