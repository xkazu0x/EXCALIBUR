#include "vk_backend.h"
#include "vk_instance.h"
#include "vk_platform.h"
#include "vk_device.h"
#include "vk_swapchain.h"

#include "core/sxlogger.h"

static vulkan_context context;

int32_t find_memory_index(uint32_t type_filter, uint32_t property_flags);

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

	SXDEBUG("VULKAN RENDERER INITIALIZED SUCCESSFULLY");
	return true;
}

void vk_backend::shutdown() {
	SXDEBUG("SHUTTING DOWN VULKAN BACKEND");
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