#pragma once

#ifndef BINDLESSVK_NAMESPACE
	#define BINDLESSVK_NAMESPACE bvk
#endif

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/Assertions.hpp"
#include "BindlessVk/Common/PtrTypes.hpp"
#include "BindlessVk/Common/Vulkan.hpp"

#include <exception>

namespace BINDLESSVK_NAMESPACE {

u64 constexpr hash_str(str_view const value)
{
	return *value.begin() ? static_cast<u64>(*value.begin()) + 33 * hash_str(value.begin() + 1) :
	                        5381;
}


/** Maximum frames allowed to be in flight */
auto constexpr max_frames_in_flight = usize { 3 };

/** Default debug name for objects and vulkan debug utils */
auto constexpr default_debug_name = str_view { "unnamed" };

/** Hashes T with a @a seed
 *
 * @param seed A seed, intented to be a previously generated hash_t value to combine hash a struct
 * @param value A T value, needs to be hashable by std::hash<T>
 */
template<typename T>
u64 hash_t(u64 seed, T const &value)
{
	std::hash<T> h;
	return h(value) + 0x9e37779b9 + (seed << 6) + (seed >> 2);
}

} // namespace BINDLESSVK_NAMESPACE
