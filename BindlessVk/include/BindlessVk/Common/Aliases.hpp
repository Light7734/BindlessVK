#pragma once

#include <array>
#include <functional>
#include <inttypes.h>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace BINDLESSVK_NAMESPACE {

// primitve type aliases
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int8_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
using usize = size_t;

// c-style string alias
using c_str = const char*;

// standard type aliases
using str = std::string;

template<typename T>
using vec = std::vector<T>;

template<typename T, size_t N>
using arr = std::array<T, N>;

template<typename F>
using fn = std::function<F>;

template<typename K, typename V>
using map = std::map<K, V>;

template<typename K, typename V>
using hash_map = std::unordered_map<K, V>;

template<typename K, typename V>
using set = std::set<K, V>;

template<typename K, typename V>
using hash_set = std::unordered_set<K, V>;

template<typename T1, typename T2>
using pair = std::pair<T1, T2>;

} // namespace BINDLESSVK_NAMESPACE
