#pragma once

#include "Core/Base.h"
#include "DeviceContext.h"

#include <glm/glm.hpp>
#include <volk.h>

class Buffer
{
private:
	class Device* m_Device;

	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;

	bool m_Mapped = false;

public:
	Buffer(class Device* device, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
	~Buffer();

	void CopyBufferToSelf(Buffer* src, uint32_t size, VkCommandPool commandPool, VkQueue graphicsQueue);

	void* Map(uint32_t size);
	void Unmap();

	inline const VkBuffer* GetBuffer() { return &m_Buffer; }

private:
	uint32_t FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);
};