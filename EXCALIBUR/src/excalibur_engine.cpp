#include "platform/platform.h"
#include "renderer/vulkan/vk_backend.h"

#include <stdio.h>

int main() {
	platform_info info = {};
	info.screen_width = 1920 / 2;
	info.screen_height = 1080 / 2;
	info.title = "EXCALIBUR";

	platform_state platform = {};
	if (!platform::initialize(&platform, &info)) 
		return -1;

	vulkan_context vk_context = {};
	if (!vk_backend::initialize(&vk_context, &platform)) 
		return -1;

	while (!platform::should_stop(&platform)) {
		platform::pump_messages(&platform);
	}

	vk_backend::shutdown(&vk_context);
	platform::shutdown(&platform);

	return 0;
}
