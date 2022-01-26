// Note: files that are included by this file should not #include "Base.hpp"
#include "Debug/Logger.hpp"


// Exceptions
#include <exception>

struct FailedAsssertionException: std::exception
{
	FailedAsssertionException(const char* statement, const char* file, int line)
	{
		LOG(critical, "Assertion failed: {}", statement);
		LOG(critical, "FailedAssertion thrown at {}:{}", file, line);
	}
};


// Platform specific
#if defined(VULKANRENDERER_PLATFORM_LINUX)
	#define VULKANRENDERER_PLATFORM "Linux"
	#include <signal.h>
	#define DEBUG_TRAP() raise(SIGTRAP)

#elif defined(VULKANRENDERER_PLATFORM_WINDOWS)
	#define VULKANRENDERER_PLATFORM "Windows"
	#define DEBUG_TRAP()            __debugbreak()

#else
	#error "Unsupported platform"
	#define DEBUG_TRPA()
#endif

// Assertions
#define ASSERT(x, ...)                                           \
	if (!(x))                                                    \
	{                                                            \
		LOG(critical, __VA_ARGS__);                              \
		DEBUG_TRAP();                                            \
		throw FailedAsssertionException(#x, __FILE__, __LINE__); \
	}
