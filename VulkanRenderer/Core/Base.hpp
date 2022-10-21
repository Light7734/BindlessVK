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

// Concatinate tokens
#define CAT_INDIRECTION(a, b) a##b

// Token indirection
#define TOKEN_INDIRECTION(token) token
#define TKNIND(token)            TOKEN_INDIRECTION(token)

#define CAT_TOKEN(a, b) CAT_INDIRECTION(a, b)

// Assertions
#define ASSERT(x, ...)                                                           \
	if (!(x))                                                                    \
	{                                                                            \
		LOG(critical, __VA_ARGS__);                                              \
		throw FailedAsssertionException(#x, TKNIND(__FILE__), TKNIND(__LINE__)); \
	}

#define SPVASSERT(x, ...)                                                                                \
	{                                                                                                    \
		SpvReflectResult CAT_TOKEN(spvresult, __LINE__) = x;                                             \
		if (CAT_TOKEN(spvresult, __LINE__))                                                              \
		{                                                                                                \
			LOG(critical, "SPV assertion failed: {}", static_cast<int>(CAT_TOKEN(spvresult, __LINE__))); \
			LOG(critical, __VA_ARGS__);                                                                  \
			throw FailedAsssertionException(#x, TKNIND(__FILE__), TKNIND(__LINE__));                     \
		}                                                                                                \
	}


#define KTXASSERT(x, ...)                                                                                \
	{                                                                                                    \
		ktxResult CAT_TOKEN(ktxresult, __LINE__) = x;                                                    \
		if (CAT_TOKEN(ktxresult, __LINE__) != KTX_SUCCESS)                                               \
		{                                                                                                \
			LOG(critical, "KTX assertion failed: {}", static_cast<int>(CAT_TOKEN(ktxresult, __LINE__))); \
			LOG(critical, __VA_ARGS__);                                                                  \
			throw FailedAsssertionException(#x, TKNIND(__FILE__), TKNIND(__LINE__));                     \
		}                                                                                                \
	}

#define VKC(x)                                                                                     \
	vk::Result CAT_TOKEN(vkresult, __LINE__);                                                      \
	if ((CAT_TOKEN(vkresult, __LINE__) = x) != vk::Result::eSuccess)                               \
	{                                                                                              \
		LOG(critical, "VK assertion failed: {}", static_cast<int>(CAT_TOKEN(vkresult, __LINE__))); \
		throw FailedAsssertionException(#x, TKNIND(__FILE__), TKNIND(__LINE__));                   \
	}


uint64_t constexpr HashStr(const char* str)
{
	return *str ? static_cast<uint64_t>(*str) + 33 * HashStr(str + 1) : 5381;
}
