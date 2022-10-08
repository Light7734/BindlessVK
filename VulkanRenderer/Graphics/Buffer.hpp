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
	vk::DeviceSize size;
	const void* initialData = {};
};

class Buffer
{
public:
	Buffer(BufferCreateInfo& createInfo);
	~Buffer();

	inline vk::Buffer* GetBuffer() { return &m_Buffer.buffer; }

	void* Map();
	void Unmap();

private:
	vk::Device m_LogicalDevice = {};

	vma::Allocator m_Allocator = {};

	AllocatedBuffer m_Buffer    = {};
	vk::DeviceSize m_BufferSize = {};
};

class StagingBuffer
{
public:
	StagingBuffer(BufferCreateInfo& createInfo);
	StagingBuffer()                          = default;
	StagingBuffer(StagingBuffer&& buffer)
	    : m_Buffer(buffer.m_Buffer)
	    , m_StagingBuffer(buffer.m_StagingBuffer)
	    , m_Allocator(buffer.m_Allocator)
	    , m_LogicalDevice(buffer.m_LogicalDevice)
	{
		buffer.m_Buffer        = {};
		buffer.m_StagingBuffer = {};
	}
	~StagingBuffer();

	inline vk::Buffer* GetBuffer() { return &m_Buffer.buffer; }

private:
	vk::Device m_LogicalDevice = {};

	vma::Allocator m_Allocator = {};

	AllocatedBuffer m_Buffer        = {};
	AllocatedBuffer m_StagingBuffer = {};
};
