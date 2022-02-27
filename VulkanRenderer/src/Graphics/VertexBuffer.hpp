#pragma once

#include "Core/Base.hpp"

#include <volk.h>

struct VertexBufferCreateInfo
{
	VkDevice logicalDevice;
	VkPhysicalDevice physicalDevice;
	VkDeviceSize size;
	void* startingData = nullptr;
};

class VertexBuffer
{
public:
	VertexBuffer(VertexBufferCreateInfo& createInfo);
	~VertexBuffer();

	inline VkBuffer* GetBuffer() { return &m_Buffer; }

private:
	VkDevice m_LogicalDevice;
	VkBuffer m_Buffer;
	VkDeviceMemory m_DeviceMemory;
};
