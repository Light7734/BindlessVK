#pragma once
#ifndef BINDLESSVK_NAMESPACE
	#define BINDLESSVK_NAMESPACE bvk
#endif

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Assertions.hpp"
#include "BindlessVk/Common/Vulkan.hpp"

#include <exception>
#include <fmt/format.h>

#if !defined(BINDLESSVK_NAMESPACE)
	#define BINDLESSVK_NAMESPACE bvk
#endif

#ifndef BVK_MAX_FRAMES_IN_FLIGHT
	#define BVK_MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef BVK_DESIRED_SWAPCHAIN_IMAGES
	#define BVK_DESIRED_SWAPCHAIN_IMAGES 3
#endif

namespace BINDLESSVK_NAMESPACE {

c_str constexpr default_debug_name = "unnamed";

enum class LogLvl : u8
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

// simple hash function
u64 constexpr hash_str(c_str value)
{
	return *value ? static_cast<u64>(*value) + 33 * hash_str(value + 1) : 5381;
}

} // namespace BINDLESSVK_NAMESPACE
