#pragma once

#define VULKAN_HPP_USE_REFLECT             1
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

// Note: files that are included by this file should not #include "Base.hpp"
#include "Debug/Logger.hpp"

// Platform specific
#if VULKANRENDERER_PLATFORM == Linux
	#include <signal.h>
	#define DEBUG_TRAP() raise(SIGTRAP)

#elif VULKANRENDERER_PLATFORM == Windows
	#define DEBUG_TRAP() __debugbreak()

#else
	#error "Unsupported platform"
	#define DEBUG_TRAP()
#endif

// Exceptions
#include <exception>

struct FailedAsssertionException: std::exception
{
	FailedAsssertionException(const char* statement, const char* file, int line)
	{
		LOG(critical, "Assertion failed: {}", statement);
		LOG(critical, "FailedAssertion thrown at {}:{}", file, line);
		DEBUG_TRAP();
	}
};

struct vkException: std::exception
{
	vkException(const char* statement, int errorCode, const char* file, int line)
	{
		// #todo: Stringify vkException
		LOG(critical, "Vulkan check failed: {}", statement);
		LOG(critical, "vkException thrown with code {} at {}:{}", errorCode, file, line);
		DEBUG_TRAP();
	}
};


// Assertions
#define ASSERT(x, ...)                                           \
	if (!(x))                                                    \
	{                                                            \
		LOG(critical, __VA_ARGS__);                              \
		throw FailedAsssertionException(#x, __FILE__, __LINE__); \
	}

#define SPVASSERT(x, ...)                                            \
	{                                                                \
		SpvReflectResult res = x;                                    \
		if (res)                                                     \
		{                                                            \
			LOG(critical, "{}", (int)res);                           \
			LOG(critical, __VA_ARGS__);                              \
			throw FailedAsssertionException(#x, __FILE__, __LINE__); \
		}                                                            \
	}

// ye wtf C++
#define VKC(x)                       VKC_NO_REDIFINITION(x, __LINE__)
#define VKC_NO_REDIFINITION(x, line) VKC_NO_REDIFINITION2(x, line)
#define VKC_NO_REDIFINITION2(x, line)                                       \
	vk::Result vkr##line;                                                   \
	if ((vkr##line = x) != vk::Result::eSuccess)                            \
	{                                                                       \
		throw vkException(#x, static_cast<int>(vkr##line), __FILE__, line); \
	}


uint64_t constexpr HashStr(const char* str)
{
	return *str ? static_cast<uint64_t>(*str) + 33 * HashStr(str + 1) : 5381;
}
