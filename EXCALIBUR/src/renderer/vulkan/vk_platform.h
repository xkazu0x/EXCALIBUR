#pragma once

#include <vector>

struct window_state;
struct vulkan_context;

namespace vk_platform {
	void get_required_extension_names(std::vector<const char *> *extensions);
	bool create_surface(struct window_state *state, struct vulkan_context *context);
}