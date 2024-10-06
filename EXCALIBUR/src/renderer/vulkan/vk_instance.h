#pragma once

#include "vk_types.h"

namespace vk_instance {
	bool create(vulkan_context *context);
	void destroy(vulkan_context *context);
}