#pragma once

#include "Debug/Logger.h"

#include <exception>

#define VKC(x)                       VKC_NO_REDIFINITION(x, __LINE__)
#define VKC_NO_REDIFINITION(x, line) VKC_NO_REDIFINITION2(x, line)
#define VKC_NO_REDIFINITION2(x, line)  \
	VkResult vkr##line;                \
	if ((vkr##line = x) != VK_SUCCESS) \
	throw vkException(vkr##line, __FILE__, line)

struct failedAssertion: std::exception
{
	failedAssertion(const char* file, int line)
	{
		LOG(critical, "FailedAssertion thrown at: {}:{}", file, line);
	}
};

struct vkException: std::exception
{
	vkException(int errorCode, const char* file, int line)
	{
		LOG(critical, "vkException: thrown with code {} at {}:{}", errorCode, file, line);
	}
};