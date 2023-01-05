#pragma once

#include "BindlessVk/BindlessVkConfig.hpp"

#include <exception>

#define VULKAN_HPP_USE_REFLECT             1
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vk_mem_alloc.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

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
	eTrace    = 0,
	eDebug    = 1,
	eInfo     = 2,
	eWarn     = 3,
	eError    = 4,
	eCritical = 5,
	eOff      = 6,

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

enum class ErrorCodes : int
{
	None = 0,

	eDefault,

	eUnsupported,
	eUnimplemented,
	eInvalidInpuut,

	nCount,
};

uint64_t constexpr HashStr(const char* str)
{
	return *str ? static_cast<uint64_t>(*str) + 33 * HashStr(str + 1) : 5381;
}

} // namespace BINDLESSVK_NAMESPACE
