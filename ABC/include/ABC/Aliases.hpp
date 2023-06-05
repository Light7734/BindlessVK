#pragma once

#include <array>
#include <cinttypes>
#include <functional>
#include <map>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// primitve type aliases
using u8 = ::std::uint8_t;
using u16 = ::std::uint16_t;
using u32 = ::std::uint32_t;
using u64 = ::std::uint64_t;
using i8 = ::std::int8_t;
using i16 = ::std::int8_t;
using i32 = ::std::int32_t;
using i64 = ::std::int64_t;
using f32 = float;
using f64 = double;
using usize = ::std::size_t;

// c-style string alias
using c_str = char const *;

// standard type aliases
using str = ::std::string;
using str_view = ::std::string_view;

template<typename T>
using vec = ::std::vector<T>;

template<typename T>
using vec_it = typename ::std::vector<T>::iterator;

template<typename T>
using span = ::std::span<T>;

template<typename T, size_t N>
using arr = ::std::array<T, N>;

template<typename F>
using fn = ::std::function<F>;

template<typename K, typename V>
using map = ::std::map<K, V>;

template<typename K, typename V>
using hash_map = ::std::unordered_map<K, V>;

template<typename K, typename V>
using set = ::std::set<K, V>;

template<typename K, typename V>
using hash_set = ::std::unordered_set<K, V>;

template<typename T1, typename T2>
using pair = ::std::pair<T1, T2>;

template<typename... Ts>
using tuple = ::std::tuple<Ts...>;
