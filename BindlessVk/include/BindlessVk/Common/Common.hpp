#pragma once

#ifndef BINDLESSVK_NAMESPACE
	#define BINDLESSVK_NAMESPACE bvk
#endif

#ifndef BVK_MAX_FRAMES_IN_FLIGHT
	#define BVK_MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef BVK_DESIRED_SWAPCHAIN_IMAGES
	#define BVK_DESIRED_SWAPCHAIN_IMAGES 3
#endif

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Assertions.hpp"
#include "BindlessVk/Common/Types/Mat4.hpp"
#include "BindlessVk/Common/Types/Vec2.hpp"
#include "BindlessVk/Common/Types/Vec3.hpp"
#include "BindlessVk/Common/Types/Vec4.hpp"
#include "BindlessVk/Common/Vulkan.hpp"

#include <exception>
#include <fmt/format.h>

namespace BINDLESSVK_NAMESPACE {

str_view constexpr default_debug_name = "unnamed\0";

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

template<typename T>
u64 hash_t(u64 s, T const &v)
{
	std::hash<T> h;
	return h(v) + 0x9e37779b9 + (s << 6) + (s >> 2);
}

} // namespace BINDLESSVK_NAMESPACE
