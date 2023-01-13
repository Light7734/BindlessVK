#pragma once

#include "BindlessVk/BindlessVkConfig.hpp"
#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Types.hpp"
#include "BindlessVk/Common/VulkanIncludes.hpp"

#include <exception>
#ifndef BINDLESSVK_NAMESPACE
	#define BINDLESSVK_NAMESPACE bvk
#endif

#ifndef BVK_MAX_FRAMES_IN_FLIGHT
	#define BVK_MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef BVK_DESIRED_SWAPCHAIN_IMAGES
	#define BVK_DESIRED_SWAPCHAIN_IMAGES 3
#endif

namespace BINDLESSVK_NAMESPACE {

//@note: If BVK_LOG is not overriden by the user
// then values should reflect spdlog::level::level_enum
enum class LogLvl
{
	eTrace = 0,
	eDebug = 1,
	eInfo = 2,
	eWarn = 3,
	eError = 4,
	eCritical = 5,
	eOff = 6,

	nCount,
};

struct BindlessVkException: std::exception
{
	BindlessVkException(const char* statement, const char* file, int line)
	    : statement(statement)
	    , file(file)
	    , line(line)
	{
	}

	const char* statement;
	const char* file;
	int line;
};

// simple hash function
u64 constexpr hash_str(c_str value)
{
	return *value ? static_cast<u64>(*value) + 33 * hash_str(value + 1) : 5381;
}

} // namespace BINDLESSVK_NAMESPACE
