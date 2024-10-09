#include "vk_backend.h"
#include "vk_instance.h"
#include "vk_platform.h"
#include "vk_device.h"
#include "core/sxlogger.h"

bool vk_backend::initialize(vulkan_context *context, platform_state *platform) {
	if (!vk_instance::create(context)) {
		SXFATAL("FAILED TO CREATE VULKAN INSTANCE");
		return false;
	}

	if (!vk_platform::create_surface(platform, context)) {
		SXFATAL("FAILED TO CREATE VULKAN SURFACE");
		return false;
	}

	if (!vk_device::create(context)) {
		SXFATAL("FAILED TO CREATE VULKAN DEVICE");
		return false;
	}

	return true;
}

void vk_backend::shutdown(vulkan_context *context) {
	SXDEBUG("SHUTTING DOWN VULKAN BACKEND");
	vk_device::destroy(context);
	if (context->surface) {
		SXDEBUG("DESTROYING VULKAN SURFACE");
		vkDestroySurfaceKHR(context->instance, context->surface, context->allocator);
		context->surface = 0;
	}
	vk_instance::destroy(context);
}