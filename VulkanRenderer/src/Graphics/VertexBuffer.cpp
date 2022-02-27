#include "Graphics/VertexBuffer.hpp"

VertexBuffer::VertexBuffer(VertexBufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer
	{
		VkBufferCreateInfo bufferCreateInfo {
			.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size        = createInfo.size,
			.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VKC(vkCreateBuffer(m_LogicalDevice, &bufferCreateInfo, nullptr, &m_Buffer));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Allocate memory
	{
		// Fetch requirements & properties
		VkMemoryRequirements memReqs;
		VkPhysicalDeviceMemoryProperties physicalMemProps;
		vkGetPhysicalDeviceMemoryProperties(createInfo.physicalDevice, &physicalMemProps);
		vkGetBufferMemoryRequirements(m_LogicalDevice, m_Buffer, &memReqs);

		uint32_t memTypeIndex = UINT32_MAX;

		for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
		{
			if (memReqs.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			{
				memTypeIndex = i;
				break;
			}
		}

		ASSERT(memTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

		VkMemoryAllocateInfo memoryAllocInfo {
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize  = memReqs.size,
			.memoryTypeIndex = memTypeIndex,
		};
		VKC(vkAllocateMemory(m_LogicalDevice, &memoryAllocInfo, nullptr, &m_DeviceMemory));
		vkBindBufferMemory(m_LogicalDevice, m_Buffer, m_DeviceMemory, 0u);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Write data to buffer
	{
		void* data;
		VKC(vkMapMemory(m_LogicalDevice, m_DeviceMemory, 0u, createInfo.size, 0u, &data));
		memcpy(data, createInfo.startingData, static_cast<size_t>(createInfo.size));
		vkUnmapMemory(m_LogicalDevice, m_DeviceMemory);
	}
}

VertexBuffer::~VertexBuffer()
{
	vkDestroyBuffer(m_LogicalDevice, m_Buffer, nullptr);
	vkFreeMemory(m_LogicalDevice, m_DeviceMemory, nullptr);
}
