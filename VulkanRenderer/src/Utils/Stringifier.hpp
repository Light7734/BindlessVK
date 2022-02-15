#pragma once

#include "Core/Base.hpp"

#include <volk.h>

    class Stringifier
{
public:
	static const char* VkMsgSeverity(VkDebugUtilsMessageSeverityFlagBitsEXT severity);
	static const char* VkMsgType(VkDebugUtilsMessageTypeFlagBitsEXT type);
};
