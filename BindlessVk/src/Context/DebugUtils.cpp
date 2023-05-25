#include "BindlessVk/Context/DebugUtils.hpp"

namespace BINDLESSVK_NAMESPACE {

DebugUtils::DebugUtils(Instance *const instance, Callback const &callback, Filter const &filter)
    : instance(instance)
    , callback(std::make_unique<Callback>(callback))

{
	auto const messenger_info = vk::DebugUtilsMessengerCreateInfoEXT {
		{},
		filter.severity_flags,
		filter.type_flags,
		&DebugUtils::validation_layers_callback, //
		this->callback.get(),
	};

	messenger = instance->vk().createDebugUtilsMessengerEXT(messenger_info);
}

DebugUtils::~DebugUtils()
{
	if (!instance)
		return;

	instance->vk().destroyDebugUtilsMessengerEXT(messenger);
}

auto DebugUtils::validation_layers_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT const message_severity,
    VkDebugUtilsMessageTypeFlagsEXT const message_types,
    VkDebugUtilsMessengerCallbackDataEXT const *const callback_data,
    void *const vulkan_user_data
) -> VkBool32
{
	auto const [callback, user_data] = *static_cast<Callback *>(vulkan_user_data);

	auto const type = DebugUtils::parse_message_type(message_types);
	auto const level = DebugUtils::parse_message_severity(message_severity);

	callback(DebugCallbackSource::eValidationLayers, level, callback_data->pMessage, user_data);

	return static_cast<VkBool32>(VK_FALSE);
}

auto DebugUtils::parse_message_type(VkDebugUtilsMessageTypeFlagsEXT const message_types) -> c_str
{
	if (message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		return "GENERAL";

	if (message_types == (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
		return "VALIDATION | PERFORMANCE";

	if (message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		return "VALIDATION";

	return "PERFORMANCE";
}

auto DebugUtils::parse_message_severity(
    VkDebugUtilsMessageSeverityFlagBitsEXT const message_severity
) -> LogLvl
{
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

} // namespace BINDLESSVK_NAMESPACE
