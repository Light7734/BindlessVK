#pragma once

#include "Base.h"
#include "DeviceContext.h"

#include <volk.h>
#include <glm/glm.hpp>

struct Vertex
{
	glm::vec2 position;
	glm::vec3 color;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription
		{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributesDescription()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributesDescription;

		attributesDescription[0] =
		{
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Vertex, position),
		};

		attributesDescription[1] =
		{
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(Vertex, color),
		};

		return attributesDescription;
	}
};

class Buffer
{
private:
	DeviceContext m_DeviceContext;

	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;

public:
	Buffer(DeviceContext deviceContext, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
	~Buffer();

	void* Map(uint32_t size);
	void Unmap();

	inline const VkBuffer* GetBuffer() { return &m_Buffer; }

private:
	uint32_t FetchMemoryType(uint32_t filter, VkMemoryPropertyFlags flags);
};