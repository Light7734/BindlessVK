#pragma once

#ifndef BINDLESSVK_NAMESPACE
	#define BINDLESSVK_NAMESPACE bvk
#endif

#define VULKAN_HPP_USE_REFLECT             1
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vk_mem_alloc.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

namespace BINDLESSVK_NAMESPACE {

/** Maximum frames allowed to be in flight */
auto constexpr max_frames_in_flight = usize { 3 };

/** Default debug name for objects and vulkan debug utils */
auto constexpr default_debug_name = str_view { "unnamed" };

/** Hashes T with a @a seed
 *
 * @param seed A seed, intented to be a previously generated hash_t value to combine hash a struct
 * @param value A T value, requires to be hashable by std::hash<T>
 */
template<typename T>
u64 hash_t(u64 seed, T const &value)
{
	std::hash<T> h;
	return h(value) + 0x9e37779b9 + (seed << 6) + (seed >> 2);
}

} // namespace BINDLESSVK_NAMESPACE
