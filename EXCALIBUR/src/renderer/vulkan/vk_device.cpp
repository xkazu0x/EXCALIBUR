#include "vk_device.h"
#include "core/sxlogger.h"
#include "core/sxmemory.h"

#include <vector>

struct vulkan_physical_device_requirements {
	bool graphics;
	bool present;
	bool compute;
	bool transfer;
	bool discrete_gpu;
	bool sampler_anisotrophy;
	std::vector<const char *> device_extensions;
};

struct vulkan_physical_device_queue_family_info {
	unsigned int graphics_family_index;
	unsigned int present_family_index;
	unsigned int compute_family_index;
	unsigned int transfer_family_index;
};

bool select_physical_device(vulkan_context *context);
bool physical_device_meets_requirements(
	VkPhysicalDevice device,
	VkSurfaceKHR surface,
	const VkPhysicalDeviceProperties *properties,
	const VkPhysicalDeviceFeatures *features,
	const vulkan_physical_device_requirements *requirements,
	vulkan_physical_device_queue_family_info *out_queue_info,
	vulkan_swapchain_support_info *out_swapchain_support);

bool vk_device::create(vulkan_context *context) {
	if (!select_physical_device(context)) { // TODO: refactor this function
		SXERROR("FAILED TO SELECT PHYSICAL DEVICE");
		return false;
	}

	SXDEBUG("CREATING VULKAN LOGICAL DEVICE");
	// NOTE: do not create additional queues for shared indices
	bool present_shares_graphics_queue = context->device.graphics_queue_index == context->device.present_queue_index;
	bool transfer_shares_graphics_queue = context->device.graphics_queue_index == context->device.transfer_queue_index;
	
	uint32_t index_count = 1;
	if (!present_shares_graphics_queue) index_count++;
	if (!transfer_shares_graphics_queue) index_count++;

	std::vector<uint32_t> indices(index_count);
	uint8_t index = 0;
	indices[index++] = context->device.graphics_queue_index;
	if (!present_shares_graphics_queue)
		indices[index++] = context->device.present_queue_index;
	if (!transfer_shares_graphics_queue)
		indices[index++] = context->device.transfer_queue_index;

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos(index_count);
	for (uint32_t i = 0; i < index_count; ++i) {
		queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[i].queueFamilyIndex = indices[i];
		queue_create_infos[i].queueCount = 1;
		float queue_priority = 1.0f;
		queue_create_infos[i].pQueuePriorities= &queue_priority;
	}

	// TODO: should be config driven
	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_FALSE;

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = index_count;
	device_info.pQueueCreateInfos = queue_create_infos.data();
	device_info.pEnabledFeatures = &device_features;
	device_info.enabledExtensionCount = 1;
	const char *extension_name = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	device_info.ppEnabledExtensionNames = &extension_name;
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = 0;

	VKCHECK(vkCreateDevice(
		context->device.physical_device, 
		&device_info, 
		context->allocator, 
		&context->device.logical_device));
	SXDEBUG("VULKAN LOGICAL DEVICE CREATED");

	vkGetDeviceQueue(
		context->device.logical_device, 
		context->device.graphics_queue_index, 
		0, 
		&context->device.graphics_queue);
	vkGetDeviceQueue(
		context->device.logical_device,
		context->device.present_queue_index,
		0,
		&context->device.present_queue);
	vkGetDeviceQueue(
		context->device.logical_device,
		context->device.transfer_queue_index,
		0,
		&context->device.transfer_queue);
	SXDEBUG("QUEUES OBTAINED");

	// create command pool for graphics queue
	VkCommandPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create_info.queueFamilyIndex = context->device.graphics_queue_index;
	pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VKCHECK(vkCreateCommandPool(
		context->device.logical_device, 
		&pool_create_info, 
		context->allocator, 
		&context->device.graphics_command_pool));
	SXDEBUG("GRAPHICS COMMAND POOL CREATED");

	return true;
}

void vk_device::destroy(vulkan_context *context) {
	SXDEBUG("DESTROYING VULKAN COMMAND POOL");
	if (context->device.graphics_command_pool) {
		vkDestroyCommandPool(
			context->device.logical_device,
			context->device.graphics_command_pool,
			context->allocator);
		context->device.graphics_command_pool = 0;
	}

	//context->device.graphics_queue = 0;
	//context->device.present_queue = 0;
	//context->device.compute_queue_index = 0;
	//context->device.transfer_queue = 0;

	SXDEBUG("DESTROYING VULKAN LOGICAL DEVICE");
	if (context->device.logical_device) {
		vkDestroyDevice(context->device.logical_device, context->allocator);
		context->device.logical_device = 0;
	}

	SXDEBUG("RELEASING PHYSICAL DEVICE RESOURCES");
	context->device.physical_device = 0;
	if (context->device.swapchain_support.formats) {
		sxmemory::free_memory(
			context->device.swapchain_support.formats, 
			sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.format_count, 
			MEMORY_TAG_RENDERER);
	}
	if (context->device.swapchain_support.present_modes) {
		sxmemory::free_memory(
			context->device.swapchain_support.present_modes,
			sizeof(VkPresentModeKHR) * context->device.swapchain_support.present_mode_count,
			MEMORY_TAG_RENDERER);
	}

	// TODO: free capabilities

	context->device.graphics_queue_index = -1;
	context->device.present_queue_index = -1;
	context->device.compute_queue_index = -1;
	context->device.transfer_queue_index = -1;
}

void vk_device::query_swapchain_support(
	VkPhysicalDevice physical_device,
	VkSurfaceKHR surface,
	vulkan_swapchain_support_info *out_support_info) {
	VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		physical_device, 
		surface, 
		&out_support_info->capabilities));

	VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
		physical_device, 
		surface, 
		&out_support_info->format_count, 
		0));
	if (out_support_info->format_count != 0) {
		if (!out_support_info->formats) {
			out_support_info->formats = static_cast<VkSurfaceFormatKHR*>(
				sxmemory::allocate(
					sizeof(VkSurfaceFormatKHR) * out_support_info->format_count, 
					MEMORY_TAG_RENDERER));
		}
		VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
			physical_device, 
			surface, 
			&out_support_info->format_count, 
			out_support_info->formats));
	}

	VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
		physical_device,
		surface,
		&out_support_info->present_mode_count,
		0));
	if (out_support_info->present_mode_count != 0) {
		if (!out_support_info->present_modes) {
			out_support_info->present_modes = static_cast<VkPresentModeKHR*>(
				sxmemory::allocate(
					sizeof(VkPresentModeKHR) * out_support_info->present_mode_count, 
					MEMORY_TAG_RENDERER));
		}
		VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(
			physical_device,
			surface,
			&out_support_info->present_mode_count,
			out_support_info->present_modes));
	}
}

bool vk_device::detect_depth_format(vulkan_device *device) {
	const uint64_t candidate_count = 3;
	VkFormat candidates[candidate_count] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT};

	uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	for (uint64_t i = 0; i < candidate_count; ++i) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i], &properties);
		if ((properties.linearTilingFeatures & flags) == flags) {
			device->depth_format = candidates[i];
			return true;
		} else if ((properties.optimalTilingFeatures & flags) == flags) {
			device->depth_format = candidates[i];
			return true;
		}
	}
	return false;
}

bool select_physical_device(vulkan_context *context) {
	SXDEBUG("SELECTING PHYSICAL DEVICE");
	uint32_t physical_device_count = 0;
	VKCHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, 0));
	if (physical_device_count == 0) {
		SXERROR("FAILED TO FIND DEVICES WHICH SUPPORT VULKAN\n");
		return false;
	}
	std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
	VKCHECK(vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices.data()));
	for (uint32_t i = 0; i < physical_device_count; ++i) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(physical_devices[i], &features);

		VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(physical_devices[i], &memory);

		// TODO: should be configurable
		vulkan_physical_device_requirements requirements = {};
		requirements.graphics = true;
		requirements.present = true;
		requirements.compute = false;
		requirements.transfer = true;
		requirements.discrete_gpu = true;
		requirements.sampler_anisotrophy = true;
		requirements.device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		vulkan_physical_device_queue_family_info queue_info = {};
		bool result = physical_device_meets_requirements(
			physical_devices[i],
			context->surface,
			&properties,
			&features,
			&requirements,
			&queue_info,
			&context->device.swapchain_support);
		if (result) {
			SXDEBUG("PHYSICAL DEVICE SELECTED");
			SXINFO("SELECTED DEVICE: %s", properties.deviceName);
			switch (properties.deviceType) {
				default:
				case VK_PHYSICAL_DEVICE_TYPE_OTHER:
					SXINFO("GPU TYPE: UNKNOWN");
					break;
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
					SXINFO("GPU TYPE: INTEGRATED");
					break;
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
					SXINFO("GPU TYPE: DISCRETE");
					break;
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
					SXINFO("GPU TYPE: VIRTUAL");
					break;
				case VK_PHYSICAL_DEVICE_TYPE_CPU:
					SXINFO("GPU TYPE: CPU");
					break;
			}
			SXINFO(
				"GPU DRIVER VERSION: %d.%d.%d",
				VK_VERSION_MAJOR(properties.driverVersion),
				VK_VERSION_MINOR(properties.driverVersion),
				VK_VERSION_PATCH(properties.driverVersion));
			SXINFO(
				"VULKAN API VERSION: %d.%d.%d",
				VK_VERSION_MAJOR(properties.apiVersion),
				VK_VERSION_MINOR(properties.apiVersion),
				VK_VERSION_PATCH(properties.apiVersion));

			for (uint32_t j = 0; j < memory.memoryHeapCount; ++j) {
				float memory_size_gib = (((float)memory.memoryHeaps[j].size) / 1024.0f / 1024.0f / 1024.0f);
				if (memory.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
					SXINFO("LOCAL GPU MEMORY: %.2f GiB", memory_size_gib);
				} else {
					SXINFO("SHARED GPU MEMORY: %.2f GiB", memory_size_gib);
				}
			}

			context->device.physical_device = physical_devices[i];
			context->device.graphics_queue_index = queue_info.graphics_family_index;
			context->device.present_queue_index = queue_info.present_family_index;
			context->device.compute_queue_index = queue_info.compute_family_index;
			context->device.transfer_queue_index = queue_info.transfer_family_index;

			context->device.properties = properties;
			context->device.features = features;
			context->device.memory = memory;
			break;
		}
	}
	if (!context->device.physical_device) {
		SXERROR("NO PHYSICAL DEVICE MEET THE REQUIREMENTS");
		return false;
	}
	return true;
}

bool physical_device_meets_requirements(
	VkPhysicalDevice device,
	VkSurfaceKHR surface,
	const VkPhysicalDeviceProperties *properties,
	const VkPhysicalDeviceFeatures *features,
	const vulkan_physical_device_requirements *requirements,
	vulkan_physical_device_queue_family_info *out_queue_info,
	vulkan_swapchain_support_info *out_swapchain_support) {
	out_queue_info->graphics_family_index = -1;
	out_queue_info->present_family_index = -1;
	out_queue_info->compute_family_index = -1;
	out_queue_info->transfer_family_index = -1;

	if (requirements->discrete_gpu) {
		if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			SXERROR("DEVICE IS NOT A DISCRETE GPU");
			return false;
		}
	}

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

	uint8_t min_transfer_score = 255;
	for (uint32_t i = 0; i < queue_family_count; ++i) {
		uint8_t current_transfer_score = 0;

		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			out_queue_info->graphics_family_index = i;
			++current_transfer_score;
		}

		if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			out_queue_info->compute_family_index = i;
			++current_transfer_score;
		}

		if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			if (current_transfer_score <= min_transfer_score) {
				min_transfer_score = current_transfer_score;
				out_queue_info->transfer_family_index = i;
			}
		}

		VkBool32 supports_present = VK_FALSE;
		VKCHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supports_present));
		if (supports_present) {
			out_queue_info->present_family_index = i;
		}
	}

	SXTRACE("GRAPHICS: %d", out_queue_info->graphics_family_index != -1);
	SXTRACE("PRESENT: %d", out_queue_info->present_family_index != -1);
	SXTRACE("COMPUTE: %d", out_queue_info->compute_family_index != -1);
	SXTRACE("TRANSFER: %d", out_queue_info->transfer_family_index != -1);
	SXTRACE("NAME: %s", properties->deviceName);

	if (
		(!requirements->graphics || (requirements->graphics && out_queue_info->graphics_family_index != -1)) &&
		(!requirements->present || (requirements->present && out_queue_info->present_family_index != -1)) &&
		(!requirements->compute || (requirements->compute && out_queue_info->compute_family_index != -1)) &&
		(!requirements->transfer || (requirements->transfer && out_queue_info->transfer_family_index != -1))) {
		SXDEBUG("DEVICE MEETS REQUIREMENTS");
		SXDEBUG("GRAPHICS FAMILY INDEX: %i", out_queue_info->graphics_family_index);
		SXDEBUG("PRESENT FAMILY INDEX: %i", out_queue_info->present_family_index);
		SXDEBUG("COMPUTE FAMILY INDEX: %i", out_queue_info->compute_family_index);
		SXDEBUG("TRANSFER FAMILY INDEX: %i", out_queue_info->transfer_family_index);

		vk_device::query_swapchain_support(device, surface, out_swapchain_support);
		if (out_swapchain_support->format_count < 1 || out_swapchain_support->present_mode_count < 1) {
			if (out_swapchain_support->formats) {
				sxmemory::free_memory(
					out_swapchain_support->formats, 
					sizeof(VkSurfaceFormatKHR) * out_swapchain_support->format_count, 
					MEMORY_TAG_RENDERER);
			}
			if (out_swapchain_support->present_modes) {
				sxmemory::free_memory(
					out_swapchain_support->present_modes,
					sizeof(VkPresentModeKHR) * out_swapchain_support->present_mode_count,
					MEMORY_TAG_RENDERER);
			}
			SXERROR("REQUIRED SWAPCHAIN SUPPORT NOT PRESENT");
			return false;
		}

		if (requirements->device_extensions.data()) {
			unsigned int available_extension_count = 0;
			VkExtensionProperties* available_extensions = 0;
			VKCHECK(vkEnumerateDeviceExtensionProperties(
				device, 
				nullptr, 
				&available_extension_count, 
				nullptr));
			if (available_extension_count != 0) {
				available_extensions = static_cast<VkExtensionProperties*>(
					sxmemory::allocate(sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER));
				VKCHECK(vkEnumerateDeviceExtensionProperties(
					device, 
					nullptr, 
					&available_extension_count, 
					available_extensions));
				for (int i = 0; i < requirements->device_extensions.size(); ++i) {
					bool found = false;
					for (unsigned int j = 0; j < available_extension_count; ++j) {
						if (strcmp(requirements->device_extensions[i], available_extensions[j].extensionName) == 0) {
							found = true;
							break;
						}
					}
					if (!found) {
						SXERROR("REQUIRED EXTENSION NOT FOUND: %s", requirements->device_extensions[i]);
						sxmemory::free_memory(available_extensions,sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
						return false;
					}
				}
			}
			sxmemory::free_memory(available_extensions, sizeof(VkExtensionProperties) * available_extension_count, MEMORY_TAG_RENDERER);
		}
		if (requirements->sampler_anisotrophy && !features->samplerAnisotropy) {
			SXWARN("DEVICE DOES NOT SUPPORT SAMPLER ANISOTROPY");
			return false;
		}
		return true;
	}
	return false;
}