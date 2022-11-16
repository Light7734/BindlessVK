#pragma once

#include "BindlessVk/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

struct RenderContext
{
	class RenderGraph* graph;
	class RenderPass* pass;
	void* userPointer;

	vk::Device logicalDevice;
	vk::CommandBuffer cmd;

	uint32_t imageIndex;
	uint32_t frameIndex;
};

struct AllocatedImage
{
	AllocatedImage() = default;

	AllocatedImage(std::pair<vk::Image, vma::Allocation> pair)
	    : image(pair.first)
	    , allocation(pair.second)
	{
	}

	inline operator vk::Image() { return image; }
	inline operator vma::Allocation() { return allocation; }

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

	inline operator vk::Buffer() { return buffer; }
	inline operator vma::Allocation() { return allocation; }


	vk::Buffer buffer          = {};
	vma::Allocation allocation = {};
};

} // namespace BINDLESSVK_NAMESPACE
