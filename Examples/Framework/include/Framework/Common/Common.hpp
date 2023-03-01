#pragma once

#include "Framework/Common/Aliases.hpp"
#include "Framework/Common/Assertions.hpp"

#include <BindlessVk/Common/Common.hpp>
#include <exception>
#include <functional>

u64 constexpr hash_str(str_view const value)
{
	return *value.begin() ? static_cast<u64>(*value.begin()) + 33 * hash_str(value.begin() + 1) :
	                        5381;
}
