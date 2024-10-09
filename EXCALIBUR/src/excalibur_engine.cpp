#include "core/sxlogger.h"
#include "platform/platform.h"
#include "renderer/vulkan/vk_backend.h"

#include <stdio.h>

int main() {
	if (!sxlogger::initialize()) {
		return -1;
	}

	SXFATAL("EXCALIBUR ENGINE %i", 999);
	SXERROR("EXCALIBUR ENGINE %i", 999);
	SXWARN("EXCALIBUR ENGINE %i", 999);
	SXINFO("EXCALIBUR ENGINE %i", 999);
	SXDEBUG("EXCALIBUR ENGINE %i", 999);
	SXTRACE("EXCALIBUR ENGINE %i", 999);

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
	sxlogger::shutdown();

	return 0;
}
