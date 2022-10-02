#pragma once

#include "Core/Base.hpp"

#include <vulkan/vulkan.hpp>

class Stringifier
{
public:
	static const char* VkMsgSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT severity);
	static const char* VkMsgType(vk::DebugUtilsMessageTypeFlagBitsEXT type);
};
