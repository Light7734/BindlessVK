#pragma once

#include "Core/Base.h"
#include "DeviceContext.h"

#include <volk.h>
#include <glm/glm.hpp>

class Buffer
{
private:
	DeviceContext m_DeviceContext;

	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;

public:
	Buffer(DeviceContext deviceContext, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
	~Buffer();

	void CopyBufferToSelf(Buffer* src, uint32_t size, VkCommandPool commandPool, VkQueue graphicsQueue);

	void* Map(uint32_t size);
	void Unmap();

	inline const VkBuffer* GetBuffer() { return &m_Buffer; }

private:
	uint32_t FetchMemoryType(uint32_t filter, VkMemoryPropertyFlags flags);
};