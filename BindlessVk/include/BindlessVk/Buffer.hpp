#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Types.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

class Buffer
{
public:
	Buffer(
	    const char* name,
	    Device* device,
	    vk::BufferUsageFlags usage,
	    vk::DeviceSize min_block_size,
	    uint32_t block_count,
	    const void* initial_data = {}
	);

	~Buffer();

	inline vk::Buffer* get_buffer()
	{
		return &buffer.buffer;
	}

	inline vk::DescriptorBufferInfo* get_descriptor_info()
	{
		return &descriptor_info;
	}

	inline vk::DeviceSize get_block_size() const
	{
		return block_size;
	}

	inline vk::DeviceSize get_whole_size() const
	{
		return whole_size;
	}

	inline vk::DeviceSize get_valid_block_size() const
	{
		return min_block_size;
	}

	void* map_block(uint32_t block_index);

	void unmap();

private:
	Device* device = {};

	vk::DescriptorBufferInfo descriptor_info = {};

	AllocatedBuffer buffer = {};

	uint32_t block_count = {};

	vk::DeviceSize block_size = {};
	vk::DeviceSize whole_size = {};
	vk::DeviceSize min_block_size = {};
};

// @todo: Merge 2 buffer classes into 1
class StagingBuffer
{
public:
	StagingBuffer(
	    const char* name,
	    Device* device,
	    vk::BufferUsageFlags usage,
	    vk::DeviceSize min_block_size,
	    uint32_t block_count,
	    const void* initial_data = {}
	);

	StagingBuffer() = default;

	~StagingBuffer();

	inline vk::Buffer* get_buffer()
	{
		return &buffer.buffer;
	}

private:
	Device* device = {};

	AllocatedBuffer buffer = {};
	AllocatedBuffer staging_buffer = {};
};

} // namespace BINDLESSVK_NAMESPACE
