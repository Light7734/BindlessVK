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
		return &m_buffer.buffer;
	}

	inline vk::DescriptorBufferInfo* get_descriptor_info()
	{
		return &m_descriptor_info;
	}

	inline vk::DeviceSize get_block_size() const
	{
		return m_block_size;
	}

	inline vk::DeviceSize get_whole_size() const
	{
		return m_whole_size;
	}

	inline vk::DeviceSize get_valid_block_size() const
	{
		return m_min_block_size;
	}

	void* map_block(uint32_t block_index);

	void unmap();

private:
	Device* m_device = {};

	vk::DescriptorBufferInfo m_descriptor_info = {};

	AllocatedBuffer m_buffer = {};

	uint32_t m_block_count = {};

	vk::DeviceSize m_block_size     = {};
	vk::DeviceSize m_whole_size     = {};
	vk::DeviceSize m_min_block_size = {};
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
		return &m_buffer.buffer;
	}

private:
	Device* m_device = {};

	AllocatedBuffer m_buffer         = {};
	AllocatedBuffer m_staging_buffer = {};
};

} // namespace BINDLESSVK_NAMESPACE
