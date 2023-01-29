#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/VkContext.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

class Buffer
{
public:
	Buffer(
	    c_str debug_name,
	    VkContext const *vk_context,
	    vk::BufferUsageFlags buffer_usage,
	    vma::AllocationCreateInfo const &vma_info,
	    vk::DeviceSize min_block_size,
	    u32 block_count
	);

	~Buffer();

	void write_data(void const *src_data, usize src_data_size, u32 block_index);
	void write_buffer(Buffer const &src_buffer, vk::BufferCopy const &copy_info);

	void *map_block(u32 block_index);

	void unmap();

	inline auto *get_buffer()
	{
		return &buffer.buffer;
	}

	inline auto *get_descriptor_info()
	{
		return &descriptor_info;
	}

	inline auto get_whole_size() const
	{
		return whole_size;
	}

	inline auto get_block_size() const
	{
		return block_size;
	}

	inline auto get_block_valid_size() const
	{
		return valid_block_size;
	}

	inline auto get_block_padding_size() const
	{
		return block_size - valid_block_size;
	}

	inline auto get_block_count() const
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
	vk::DeviceSize const valid_block_size = {};

	u32 const block_count = {};

	vk::DescriptorBufferInfo descriptor_info = {};
};

} // namespace BINDLESSVK_NAMESPACE
