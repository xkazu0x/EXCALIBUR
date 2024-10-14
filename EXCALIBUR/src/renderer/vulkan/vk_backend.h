#pragma once

#include "vk_types.h"

struct platform_state;

namespace vk_backend {
	bool initialize(platform_state *platform);
	void shutdown();
}