#include "Utils/Stringifier.hpp"

const char* Stringifier::VkMsgSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity)
	{
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: return "VERBOSE";
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: return "INFO";
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: return "WARNING";
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: return "ERROR";
	default: return "Unknown";
	}
}

const char* Stringifier::VkMsgType(vk::DebugUtilsMessageTypeFlagBitsEXT type)
{
	switch (type)
	{
	case vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral: return "GENERAL";
	case vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation: return "VALIDATION";
	case vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance: return "PERFORMANCE";
	default: return "Unknown";
	}
}
