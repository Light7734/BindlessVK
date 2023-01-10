#pragma once

#include "BindlessVk/Common.hpp"

#include <vector>

namespace BINDLESSVK_NAMESPACE {

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

	vk::Image image = {};
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


	vk::Buffer buffer = {};
	vma::Allocation allocation = {};
};

} // namespace BINDLESSVK_NAMESPACE
