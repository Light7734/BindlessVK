#pragma once

#include "BindlessVk/Common/Aliases.hpp"
#include "BindlessVk/Common/VulkanIncludes.hpp"

#include <vector>

namespace BINDLESSVK_NAMESPACE {

struct AllocatedImage
{
	AllocatedImage() = default;

	AllocatedImage(vk::Image image): image(image)
	{
	}

	AllocatedImage(pair<vk::Image, vma::Allocation> allocated_image)
	    : image(allocated_image.first)
	    , allocation(allocated_image.second)
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

	AllocatedBuffer(pair<vk::Buffer, vma::Allocation> allocated_buffer)
	    : buffer(allocated_buffer.first)
	    , allocation(allocated_buffer.second)
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
