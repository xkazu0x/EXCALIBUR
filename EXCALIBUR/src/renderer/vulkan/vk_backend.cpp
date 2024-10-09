#include "vk_backend.h"
#include "vk_instance.h"
#include "vk_platform.h"
#include "vk_device.h"

bool vk_backend::initialize(vulkan_context *context, platform_state *platform) {
	if (!vk_instance::create(context)) 
		return false;

	if (!vk_platform::create_surface(platform, context)) 
		return false;

	if (!vk_device::create(context))
		return false;

	return true;
}

void vk_backend::shutdown(vulkan_context *context) {
	vk_device::destroy(context);

	if (context->surface)
		vkDestroySurfaceKHR(context->instance, context->surface, context->allocator);

	vk_instance::destroy(context);
}