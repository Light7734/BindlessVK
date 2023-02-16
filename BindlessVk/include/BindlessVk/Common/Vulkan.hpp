#pragma once

#define VULKAN_HPP_USE_REFLECT             1
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Common/Aliases.hpp"

#include <vk_mem_alloc.hpp>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

static_assert(VK_SUCCESS == false, "VK_SUCCESS was supposed to be 0 (false), but it isn't");

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

	inline operator vk::Image() const
	{
		return image;
	}

	inline operator vma::Allocation() const
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
