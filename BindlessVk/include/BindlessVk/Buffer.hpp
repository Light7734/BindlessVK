#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/Types.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

struct BufferCreateInfo
{
	const char* name;

	Device* device;
	vk::CommandPool commandPool;
	vk::BufferUsageFlags usage;

	vk::DeviceSize minBlockSize;
	uint32_t blockCount;

	const void* initialData = {};
};

class Buffer
{
public:
	Buffer(BufferCreateInfo& info);
	~Buffer();

	inline vk::Buffer* GetBuffer() { return &m_Buffer.buffer; }

	inline vk::DescriptorBufferInfo* GetDescriptorInfo()
	{
		return &m_DescriptorInfo;
	}

	inline vk::DeviceSize GetBlockSize() const
	{
		return m_BlockSize;
	}

	inline vk::DeviceSize GetWholeSize() const
	{
		return m_WholeSize;
	}

	inline vk::DeviceSize GetValidBlockSize() const
	{
		return m_MinBlockSize;
	}

	void* MapBlock(uint32_t blockIndex);

	void Unmap();

private:
	Device* m_Device = {};

	vk::DescriptorBufferInfo m_DescriptorInfo = {};

	AllocatedBuffer m_Buffer = {};

	uint32_t m_BlockCount = {};

	vk::DeviceSize m_BlockSize    = {};
	vk::DeviceSize m_WholeSize    = {};
	vk::DeviceSize m_MinBlockSize = {};
};

// @todo: Merge 2 buffer classes into 1
class StagingBuffer
{
public:
	StagingBuffer() = default;
	StagingBuffer(const BufferCreateInfo& createInfo);
	~StagingBuffer();

	inline vk::Buffer* GetBuffer() { return &m_Buffer.buffer; }

private:
	Device* m_Device = {};

	AllocatedBuffer m_Buffer        = {};
	AllocatedBuffer m_StagingBuffer = {};
};

} // namespace BINDLESSVK_NAMESPACE
