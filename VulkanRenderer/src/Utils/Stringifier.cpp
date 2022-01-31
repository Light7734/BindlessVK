#include "Utils/Stringifier.hpp"

const char* Stringifier::VkMsgSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERBOSE";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "INFO";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "ERROR";
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARNING";
	default: return "Unknown";
	}
}

const char* Stringifier::VkMsgType(VkDebugUtilsMessageTypeFlagBitsEXT type)
{
	switch (type)
	{
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "GENERAL";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "VALIDATION";
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "PERFORMANCE";
	default: return "Unknown";
	}
}
