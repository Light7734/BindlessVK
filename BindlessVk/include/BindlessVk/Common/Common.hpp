#pragma once

#ifndef BINDLESSVK_NAMESPACE
	#define BINDLESSVK_NAMESPACE bvk
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

/** Maximum frames allowed to be in flight */
auto constexpr max_frames_in_flight = usize { 3 };

/** Default debug name for objects and vulkan debug utils */
auto constexpr default_debug_name = str_view { "unnamed" };

/** @brief Severity of a log message.
 *
 * @note Values reflect spdlog::lvl
 */
enum class LogLvl : u8
{
	/** Lowest and most vebose log level, for tracing execution paths and events */
	eTrace = 0,

	/** Vebose log level, for enabling temporarily to debug */
	eDebug = 1,

	/** General information */
	eInfo = 2,

	/** Things we need to be aware of and edge cases */
	eWarn = 3,

	/** Defects, bugs and undesired behaviour */
	eError = 4,

	/** Unrecoverable errors */
	eCritical = 5,

	/** No logging */
	eOff = 6,

	nCount,
};

/** Hashes T with a @a seed
 *
 * @param seed A seed, intented to be a previously generated hash_t value to combine hash a struct
 * @param value A T value, needs to be hashable by std:hash<T>
 */
template<typename T>
u64 hash_t(u64 seed, T const &value)
{
	std::hash<T> h;
	return h(value) + 0x9e37779b9 + (seed << 6) + (seed >> 2);
}

} // namespace BINDLESSVK_NAMESPACE
