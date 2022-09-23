#pragma once

#include "Core/Base.hpp"
#include "Core/DeletionQueue.hpp"

#include <vulkan/vulkan.hpp>

struct BufferCreateInfo
{
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	vk::CommandPool commandPool;
	vk::Queue graphicsQueue;
	vk::BufferUsageFlags usage;
	vk::DeviceSize size;
	const void* initialData = nullptr;
};

class Buffer
{
public:
	Buffer(BufferCreateInfo& createInfo);
	~Buffer();

	inline vk::Buffer* GetBuffer() { return &m_Buffer; }

	void* Map();
	void Unmap();

private:
	vk::Device m_LogicalDevice;

	vk::Buffer m_Buffer;
	vk::DeviceSize m_BufferSize;
	vk::DeviceMemory m_BufferMemory;

	DeletionQueue m_DeletionQueue;
};

class StagingBuffer
{
public:
	StagingBuffer(BufferCreateInfo& createInfo);
	~StagingBuffer();

	inline vk::Buffer* GetBuffer() { return &m_Buffer; }

private:
	vk::Device m_LogicalDevice;

	vk::Buffer m_Buffer;
	vk::DeviceMemory m_BufferMemory;

	vk::Buffer m_StagingBuffer;
	vk::DeviceMemory m_StagingBufferMemory;

	DeletionQueue m_DeletionQueue;
};
