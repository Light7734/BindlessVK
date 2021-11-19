#pragma once

#include "Debug/Exceptions.h"
#include "Debug/Logger.h"

#define BIT(x) 1 << x

#define ASSERT(x, ...)                             \
    if (!x)                                        \
    {                                              \
        LOG(critical, __VA_ARGS__);                \
        throw failedAssertion(__FILE__, __LINE__); \
    }