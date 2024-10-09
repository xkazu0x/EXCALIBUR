#include "platform/platform.h"
#include "renderer/vulkan/vk_types.h"
#include "renderer/vulkan/vk_instance.h"
#include "renderer/vulkan/vk_platform.h"
#include "renderer/vulkan/vk_device.h"

#include <stdio.h>

int main() {
	platform_info info = {};
	info.screen_width = 1920 / 2;
	info.screen_height = 1080 / 2;
	info.title = "EXCALIBUR";

	platform_state platform;
	if (!platform::initialize(&platform, &info)) {
		return -1;
	}

	vulkan_context vk_context = {};
	vk_context.allocator = 0;

	if (!vk_instance::create(&vk_context)) {
		printf("FAILED TO CREATE VULKAN INSTANCE\n");
		return -1;
	}

	if (!vk_platform::create_surface(&platform, &vk_context)) {
		printf("FAILED TO CREATE VULKAN SURFACE\n");
		return -1;
	}

	if (!vk_device::create(&vk_context)) {
		printf("FAILED TO CREATE VULKAN DEVICE\n");
		return -1;
	}

	while (!platform::should_stop(&platform)) {
		platform::pump_messages(&platform);
	}

	vk_device::destroy(&vk_context);
	vkDestroySurfaceKHR(vk_context.instance, vk_context.surface, vk_context.allocator);
	vk_instance::destroy(&vk_context);

	platform::shutdown(&platform);

	return 0;
}
