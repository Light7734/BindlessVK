#pragma once

#include "Core/Base.hpp"

#include <volk.h>

struct VertexBufferCreateInfo
{
	VkDevice logicalDevice;
	VkPhysicalDevice physicalDevice;
	VkCommandPool commandPool;
	VkQueue graphicsQueue;
	VkDeviceSize size;
	void* startingData = nullptr;
};

class VertexBuffer
{
public:
	VertexBuffer(VertexBufferCreateInfo& createInfo);
	~VertexBuffer();

	inline VkBuffer* GetBuffer() { return &m_Buffer; }

	VkDevice m_LogicalDevice;

	VkBuffer m_Buffer;
	VkDeviceMemory m_BufferMemory;

	VkBuffer m_StagingBuffer;
	VkDeviceMemory m_StagingBufferMemory;
};
