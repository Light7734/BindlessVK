#pragma once

#include "Debug/Exceptions.h"
#include "Debug/Logger.h"

#define BIT(x) 1 << x

#ifdef _MSC_VER
#define ASSERT(x, ...)                             \
    if (!x)                                        \
    {                                              \
        __debugbreak();                            \
        LOG(critical, __VA_ARGS__);                \
        throw failedAssertion(__FILE__, __LINE__); \
    }
#else
#define ASSERT(x, ...)                             \
    if (!x)                                        \
    {                                              \
        LOG(critical, __VA_ARGS__);                \
        throw failedAssertion(__FILE__, __LINE__); \
    }
#endif