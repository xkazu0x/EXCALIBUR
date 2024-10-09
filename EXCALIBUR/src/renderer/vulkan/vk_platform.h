#pragma once

#include <vector>

struct platform_state;
struct vulkan_context;

namespace vk_platform {
	void get_required_extension_names(std::vector<const char *> *extensions);
	bool create_surface(struct platform_state *platform, struct vulkan_context *context);
}