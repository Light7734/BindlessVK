#pragma once

#include "BindlessVk/Common.hpp"

#include <vector>

namespace BINDLESSVK_NAMESPACE {

// primitve type aliases
using u8  = uint8_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i32 = int32_t;
using i64 = int64_t;

// standard type aliases
using string = std::string;

template<typename T>
using vec = std::vector<T>;

template<typename T, size_t N>
using arr = std::array<T, N>;

template<typename F>
using func = std::function<F>;

/** @brief
 */
struct AllocatedImage
{
	AllocatedImage() = default;

	AllocatedImage(vk::Image image): image(image)
	{
	}

	AllocatedImage(std::pair<vk::Image, vma::Allocation> pair)
	    : image(pair.first)
	    , allocation(pair.second)
	{
	}

	inline operator vk::Image()
	{
		return image;
	}
	inline operator vma::Allocation()
	{
		return allocation;
	}

	inline bool has_allocation() const
	{
		return !!allocation;
	}

	vk::Image image            = {};
	vma::Allocation allocation = {};
};

struct AllocatedBuffer
{
	AllocatedBuffer() = default;

	AllocatedBuffer(std::pair<vk::Buffer, vma::Allocation> pair)
	    : buffer(pair.first)
	    , allocation(pair.second)
	{
	}

	inline operator vk::Buffer()
	{
		return buffer;
	}
	inline operator vma::Allocation()
	{
		return allocation;
	}


	vk::Buffer buffer          = {};
	vma::Allocation allocation = {};
};

} // namespace BINDLESSVK_NAMESPACE
