#include "Logger.h"
#include "Exceptions.h"

#define BIT(x) 1 << x

#define ASSERT(x, ...) if(!x) { LOG(critical, __VA_ARGS__); throw failedAssertion(__FILE__, __LINE__); }