#pragma once

u64 constexpr hash_str(str_view const value)
{
	return *value.begin() ? static_cast<u64>(*value.begin()) + 33 * hash_str(value.begin() + 1) :
	                        5381;
}
