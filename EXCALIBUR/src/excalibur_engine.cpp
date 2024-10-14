#include "core/sxlogger.h"
#include "core/sxmemory.h"
#include "platform/platform.h"
#include "renderer/vulkan/vk_backend.h"

int main() {
	sxmemory::initialize();

	if (!sxlogger::initialize())
		return -1;

	platform_info info = {};
	info.screen_width = 1920 / 2;
	info.screen_height = 1080 / 2;
	info.title = "EXCALIBUR";

	platform_state platform = {};
	if (!platform::initialize(&platform, &info)) 
		return -1;

	if (!vk_backend::initialize(&platform)) 
		return -1;

	SXINFO(sxmemory::get_memory_usage_str());
	while (!platform::should_stop(&platform)) {
		platform::pump_messages(&platform);
	}

	vk_backend::shutdown();
	platform::shutdown(&platform);
	sxlogger::shutdown();

	SXINFO(sxmemory::get_memory_usage_str());
	sxmemory::shutdown();

	return 0;
}
