#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

class Buffer
{
public:
	Buffer(
	    VkContext const *vk_context,
	    vk::BufferUsageFlags buffer_usage,
	    vma::AllocationCreateInfo const &vma_info,
	    vk::DeviceSize min_block_size,
	    u32 block_count,
	    str_view debug_name = default_debug_name
	);

	Buffer(Buffer &&other) noexcept
	{
		*this = std::move(other);
	}

	Buffer &operator=(Buffer &&other) noexcept
	{
		this->vk_context = other.vk_context;
		this->buffer = other.buffer;
		this->whole_size = other.whole_size;
		this->valid_block_size = other.valid_block_size;
		this->block_count = other.block_count;
		this->descriptor_info = other.descriptor_info;

		other.vk_context = nullptr;

		return *this;
	}

	Buffer(Buffer const &other) = delete;
	Buffer &operator=(Buffer const &other) = delete;

	~Buffer();

	void write_data(void const *src_data, usize src_data_size, u32 block_index);
	void write_buffer(Buffer const &src_buffer, vk::BufferCopy const &copy_info);

	void *map_block(u32 block_index);

	void unmap();

	auto get_buffer()
	{
		return &buffer.buffer;
	}

	auto get_descriptor_info()
	{
		return &descriptor_info;
	}

	auto get_whole_size() const
	{
		return whole_size;
	}

	auto get_block_size() const
	{
		return block_size;
	}

	auto get_block_valid_size() const
	{
		return valid_block_size;
	}

	auto get_block_padding_size() const
	{
		return block_size - valid_block_size;
	}

	auto get_block_count() const
	{
		return block_count;
	}

private:
	void calculate_block_size();

private:
	VkContext const *vk_context = {};

	AllocatedBuffer buffer = {};

	vk::DeviceSize whole_size = {};
	vk::DeviceSize block_size = {};
	vk::DeviceSize valid_block_size = {};

	u32 block_count = {};

	vk::DescriptorBufferInfo descriptor_info = {};

	str debug_name = {};
};

} // namespace BINDLESSVK_NAMESPACE
