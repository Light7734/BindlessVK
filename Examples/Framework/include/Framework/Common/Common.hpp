#pragma once

#include "Framework/Common/Aliases.hpp"
#include "Framework/Common/Assertions.hpp"

#include <BindlessVk/Common/Common.hpp>
#include <exception>

// simple hash function
u64 constexpr hash_str(c_str value)
{
	return *value ? static_cast<u64>(*value) + 33 * hash_str(value + 1) : 5381;
}
