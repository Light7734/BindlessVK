#include "BindlessVk/Context/Instance.hpp"

#include "Amender/Amender.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace BINDLESSVK_NAMESPACE {

auto static parse_message_type(VkDebugUtilsMessageTypeFlagsEXT const message_types) -> c_str
{
	ScopeProfiler _;

	if (message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		return "GENERAL";

	if (message_types
	    == (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
		return "VALIDATION | PERFORMANCE";

	if (message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		return "VALIDATION";

	return "PERFORMANCE";
}

auto static parse_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT const message_severity)
    -> LogLvl
{
	ScopeProfiler _;

	switch (message_severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return LogLvl::eTrace;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return LogLvl::eInfo;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return LogLvl::eWarn;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return LogLvl::eError;
	default: assert_fail("Invalid message severity: {}", static_cast<int>(message_severity));
	}

	return {};
}


auto static validation_layers_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT const message_severity,
    VkDebugUtilsMessageTypeFlagsEXT const message_types,
    VkDebugUtilsMessengerCallbackDataEXT const *const callback_data,
    void *const vulkan_user_data
) -> VkBool32
{
	ScopeProfiler _;

	auto const type = parse_message_type(message_types);
	auto const level = parse_message_severity(message_severity);

	Logger::log(level, ":: <{}> :: {}", type, callback_data->pMessage);
	return static_cast<VkBool32>(VK_FALSE);
}

Instance::Instance(Requirements const &requirements, Filter const &filter)
    : requirements(requirements)
{
	ScopeProfiler _;

	load_functions();
	check_layer_support();
	create_instance();

	auto const messenger_info = vk::DebugUtilsMessengerCreateInfoEXT {
		{},
		filter.severity_flags,
		filter.type_flags,
		&validation_layers_callback, //
	};

	messenger = instance.createDebugUtilsMessengerEXT(messenger_info);
}

Instance::Instance(Instance &&other)
{
	ScopeProfiler _;

	*this = std::move(other);
}

Instance &Instance::operator=(Instance &&other)
{
	ScopeProfiler _;

	this->instance = other.instance;
	this->requirements = other.requirements;

	other.instance = vk::Instance {};

	return *this;
}

Instance::~Instance()
{
	ScopeProfiler _;

	if (!instance)
		return;

	instance.destroyDebugUtilsMessengerEXT(messenger);
	instance.destroy();
}

void Instance::load_functions()
{
	ScopeProfiler _;

	VULKAN_HPP_DEFAULT_DISPATCHER.init(
	    vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")
	);
}

void Instance::check_layer_support()
{
	ScopeProfiler _;

	for (auto const layer : requirements.layers)
		if (!has_layer(layer))
			assert_fail("Required layer: {} is not supported", layer);
}

void Instance::create_instance()
{
	ScopeProfiler _;

	auto const application_info = vk::ApplicationInfo {
		"BindlessVk",
		VK_MAKE_VERSION(1, 0, 0),
		"BindlessVk", //
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_3,
	};

	auto const instance_info = vk::InstanceCreateInfo {
		{},
		&application_info,
		requirements.layers,
		requirements.extensions,
	};

	assert_false(
	    vk::createInstance(&instance_info, nullptr, &instance),
	    "Failed to create vulkan instance"
	);

	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

auto Instance::has_layer(c_str layer) const -> bool
{
	ScopeProfiler _;

	for (auto const &layer_properties : vk::enumerateInstanceLayerProperties())
		if (strcmp(layer, layer_properties.layerName))
			return true;

	return false;
}

} // namespace BINDLESSVK_NAMESPACE
