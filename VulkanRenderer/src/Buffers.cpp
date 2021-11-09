#include "Buffers.h"

Buffer::Buffer(DeviceContext deviceContext, uint32_t size, VkBufferUsageFlags usage) :
	m_DeviceContext(deviceContext),
	m_Buffer(VK_NULL_HANDLE),
	m_Memory(VK_NULL_HANDLE)
{
	// buffer create-info
	VkBufferCreateInfo bufferCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	// create buffer
	VKC(vkCreateBuffer(m_DeviceContext.logical, &bufferCreateInfo, nullptr, &m_Buffer));

	VkMemoryRequirements memoryRequirments;
	vkGetBufferMemoryRequirements(m_DeviceContext.logical, m_Buffer, &memoryRequirments);

	VkMemoryAllocateInfo allocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirments.size,
		.memoryTypeIndex = FetchMemoryType(memoryRequirments.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	};

	VKC(vkAllocateMemory(m_DeviceContext.logical, &allocateInfo, nullptr, &m_Memory));
	VKC(vkBindBufferMemory(m_DeviceContext.logical, m_Buffer, m_Memory, 0u));
}

Buffer::~Buffer()
{
	vkDestroyBuffer(m_DeviceContext.logical, m_Buffer, nullptr);
	vkFreeMemory(m_DeviceContext.logical, m_Memory, nullptr);
}

void* Buffer::Map(uint32_t size)
{
	void* map;
	VKC(vkMapMemory(m_DeviceContext.logical, m_Memory, NULL, size, NULL, &map));
	return map;
}

void Buffer::Unmap()
{
	vkUnmapMemory(m_DeviceContext.logical, m_Memory);
}

uint32_t Buffer::FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_DeviceContext.physical, &physicalMemoryProperties);

	for (uint32_t i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (physicalMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to fetch required memory type!");
	return -1;
}
