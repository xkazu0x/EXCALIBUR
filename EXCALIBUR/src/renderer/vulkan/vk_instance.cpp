#include "vk_instance.h"
#include "vk_platform.h"
#include "core/sxlogger.h"

#include <vector>

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_types,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data);

bool vk_instance::create(vulkan_context *context) {
	SXDEBUG("CREATING VULKAN INSTANCE");

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "EXCALIBUR";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "EXCALIBUR_ENGINE";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;

	std::vector<const char *> required_extensions;
	required_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	vk_platform::get_required_extension_names(&required_extensions);
#if defined(_DEBUG)
	required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	SXINFO("REQUIRED EXTENSIONS:");
	for (int i = 0; i < required_extensions.size(); i++) {
		SXINFO(required_extensions[i]);
	}
#endif

	instance_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
	instance_info.ppEnabledExtensionNames = required_extensions.data();

	std::vector<const char *> required_validation_layers;
#if defined(_DEBUG)
	required_validation_layers.push_back("VK_LAYER_KHRONOS_validation");

	uint32_t available_layer_count = 0;
	VKCHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));

	std::vector<VkLayerProperties> available_layers(available_layer_count);
	VKCHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data()));

	for (int i = 0; i < required_validation_layers.size(); i++) {
		SXINFO("SEARCHING FOR LAYER: %s", required_validation_layers[i]);
		bool found = false;
		for (const auto &layer : available_layers) {
			if (strcmp(layer.layerName, required_validation_layers[i]) == 0) {
				found = true;
				SXINFO("FOUND LAYER: %s", required_validation_layers[i]);
				break;
			}
		}

		if (!found) {
			SXERROR("REQUIRED VALIDATION LAYER IS MISSING: %s", required_validation_layers[i]);
			return false;
		}
	}
#endif

	instance_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
	instance_info.ppEnabledLayerNames = required_validation_layers.data();

	VKCHECK(vkCreateInstance(&instance_info, context->allocator, &context->instance));
	SXDEBUG("VULKAN INSTANCE CREATED");

#if defined(_DEBUG)
	SXDEBUG("CREATING VULKAN DEBUG MESSENGER");
	uint32_t log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

	VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = log_severity;
	debug_info.messageType = 
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | 
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debug_info.pfnUserCallback = vk_debug_callback;

	PFN_vkCreateDebugUtilsMessengerEXT function =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");
	VKCHECK(function(context->instance, &debug_info, context->allocator, &context->debug_messenger));
	SXDEBUG("VULKAN DEBUG MESSENGER CREATED");
#endif

	return true;
}

void vk_instance::destroy(vulkan_context *context) {
	SXDEBUG("DESTROYING VULKAN DEBUG MESSENGER");
	if (context->debug_messenger) {
		PFN_vkDestroyDebugUtilsMessengerEXT function =
			(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");
		function(context->instance, context->debug_messenger, context->allocator);
		context->debug_messenger = 0;
	}

	SXDEBUG("DESTROYING VULKAN INSTANCE");
	if (context->instance) {
		vkDestroyInstance(context->instance, context->allocator);
		context->instance = 0;
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
			SXERROR(callback_data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			SXWARN(callback_data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			SXINFO(callback_data->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			SXTRACE(callback_data->pMessage);
			break;
	}
	return VK_FALSE;
}