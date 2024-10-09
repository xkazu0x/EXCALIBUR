#pragma once

#include "vk_types.h"

struct platform_state;

namespace vk_backend {
	bool initialize(vulkan_context *context, platform_state *platform);
	void shutdown(vulkan_context *context);
}