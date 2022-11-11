#pragma once

#include "Core/Base.hpp"
#include "Graphics/Types.hpp"

#include <vulkan/vulkan.hpp>

struct BufferCreateInfo
{
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	vma::Allocator allocator;
	vk::CommandPool commandPool;
	vk::Queue graphicsQueue;
	vk::BufferUsageFlags usage;

	vk::DeviceSize minBlockSize;
	uint32_t blockCount;

	const void* initialData = {};
};

class Buffer
{
public:
	Buffer(BufferCreateInfo& createInfo);
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

	inline vk::DeviceSize GetBlockStride() const
	{
		return m_Stride;
	}

	void* MapBlock(uint32_t blockIndex);

	void Unmap();

private:
	vk::DescriptorBufferInfo m_DescriptorInfo;
	vk::Device m_LogicalDevice = {};

	vma::Allocator m_Allocator = {};

	AllocatedBuffer m_Buffer = {};

	uint32_t m_BlockCount = {};

	vk::DeviceSize m_BlockSize = {};
	vk::DeviceSize m_WholeSize = {};
	vk::DeviceSize m_Stride    = {};
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
	vk::Device m_LogicalDevice = {};

	vma::Allocator m_Allocator = {};

	AllocatedBuffer m_Buffer        = {};
	AllocatedBuffer m_StagingBuffer = {};
};
