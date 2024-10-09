#include "vk_instance.h"
#include "vk_platform.h"

#include <stdio.h>
#include <vector>

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_types,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data);

bool vk_instance::create(vulkan_context *context) {
	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pApplicationName = "EXCALIBUR";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "EXCALIBUR_ENGINE";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instance_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instance_info.pApplicationInfo = &app_info;

	std::vector<const char *> required_extensions;
	std::vector<const char *> required_validation_layers;

	// Extensions
	required_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	vk_platform::get_required_extension_names(&required_extensions);

#if defined(_DEBUG)
	// Debug extension
	required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	printf("REQUIRED EXTENSIONS:\n");
	for (int i = 0; i < required_extensions.size(); i++) {
		printf("%s\n", required_extensions[i]);
	}

	// Validation layers
	required_validation_layers.push_back("VK_LAYER_KHRONOS_validation");

	uint32_t available_layer_count = 0;
	VKCHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));

	std::vector<VkLayerProperties> available_layers(available_layer_count);
	VKCHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data()));

	for (int i = 0; i < required_validation_layers.size(); i++) {
		printf("SEARCHING FOR LAYER: %s\n", required_validation_layers[i]);
		bool found = false;
		for (const auto &layer : available_layers) {
			if (strcmp(layer.layerName, required_validation_layers[i]) == 0) {
				found = true;
				printf("FOUND LAYER: %s\n", required_validation_layers[i]);
				break;
			}
		}

		if (!found) {
			printf("REQUIRED VALIDATION LAYER IS MISSING: %s\n", required_validation_layers[i]);
			return false;
		}
	}
#endif

	instance_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
	instance_info.ppEnabledLayerNames = required_validation_layers.data();
	instance_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
	instance_info.ppEnabledExtensionNames = required_extensions.data();

	VKCHECK(vkCreateInstance(&instance_info, context->allocator, &context->instance));

#if defined(_DEBUG)
	uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT; //|
		//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

	VkDebugUtilsMessengerCreateInfoEXT debug_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debug_info.messageSeverity = log_severity;
	debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debug_info.pfnUserCallback = vk_debug_callback;

	PFN_vkCreateDebugUtilsMessengerEXT function =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");
	VKCHECK(function(context->instance, &debug_info, context->allocator, &context->debug_messenger));
#endif

	return true;
}

void vk_instance::destroy(vulkan_context *context) {
	if (context->debug_messenger) {
		PFN_vkDestroyDebugUtilsMessengerEXT function =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");
		function(context->instance, context->debug_messenger, context->allocator);
	}

	if (context->instance) {
		vkDestroyInstance(context->instance, context->allocator);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_types,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data) {
	switch (message_severity) {
		default:
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			printf("[ERROR] %s",callback_data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			printf("[WARN] %s", callback_data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			printf("[INFO] %s", callback_data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			printf("[TRACE] %s", callback_data->pMessage);
			break;
	}
	return VK_FALSE;
}