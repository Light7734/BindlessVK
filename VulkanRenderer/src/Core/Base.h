#pragma once

#include "Debug/Exceptions.h"
#include "Debug/Logger.h"

#ifdef VULKANRENDERER_PLATFORM_LINUX
    #include <signal.h>
#endif

#define BIT(x) 1 << x

#ifdef _MSC_VER
	#define ASSERT(x, ...)                             \
		if (!x)                                        \
		{                                              \
			LOG(critical, __VA_ARGS__);                \
			__debugbreak();                            \
			throw failedAssertion(__FILE__, __LINE__); \
		}
#else
	#define ASSERT(x, ...)                             \
		if (!x)                                        \
		{                                              \
			LOG(critical, __VA_ARGS__);                \
            raise(SIGTRAP);                            \
			throw failedAssertion(__FILE__, __LINE__); \
		}
#endif
