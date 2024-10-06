#pragma once

#include <vulkan/vulkan.h>
#include <assert.h>

#define VKCHECK(expr) { assert(expr == VK_SUCCESS); }

struct vulkan_context {
	VkInstance instance;
	VkAllocationCallbacks *allocator;
#if defined(_DEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
};