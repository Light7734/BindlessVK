#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

class Buffer
{
public:
	Buffer(
	    const char* debug_name,
	    Device* device,
	    vk::BufferUsageFlags buffer_usage,
	    const vma::AllocationCreateInfo& vma_info,
	    vk::DeviceSize min_block_size,
	    u32 block_count
	);

	~Buffer();

	void write_data(const void* src_data, usize src_data_size, u32 block_index);
	void write_buffer(const Buffer& src_buffer, const vk::BufferCopy& copy_info);

	void* map_block(u32 block_index);

	void unmap();

	inline vk::Buffer* get_buffer()
	{
		return &buffer.buffer;
	}

	inline vk::DescriptorBufferInfo* get_descriptor_info()
	{
		return &descriptor_info;
	}

	inline vk::DeviceSize get_whole_size() const
	{
		return whole_size;
	}

	inline vk::DeviceSize get_block_size() const
	{
		return block_size;
	}

	inline vk::DeviceSize get_block_valid_size() const
	{
		return valid_block_size;
	}

	inline vk::DeviceSize get_block_padding_size() const
	{
		return block_size - valid_block_size;
	}

	inline u32 get_block_count() const
	{
		return block_count;
	}

private:
	void calculate_block_size();

private:
	Device* device = {};

	AllocatedBuffer buffer = {};

	vk::DeviceSize whole_size = {};
	vk::DeviceSize block_size = {};
	vk::DeviceSize valid_block_size = {};

	u32 block_count = {};

	vk::DescriptorBufferInfo descriptor_info = {};
};

} // namespace BINDLESSVK_NAMESPACE
