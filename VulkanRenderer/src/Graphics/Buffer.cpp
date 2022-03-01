
#include "Graphics/Buffer.hpp"
Buffer::Buffer(BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice), m_BufferSize(createInfo.size)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create vertex and staging buffers
	{
		VkBufferCreateInfo bufferCreateInfo {
			.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size        = createInfo.size,
			.usage       = createInfo.usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VKC(vkCreateBuffer(m_LogicalDevice, &bufferCreateInfo, nullptr, &m_Buffer));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Fetch buffer's memory requirements and allocate memory
	{
		// Fetch memory requirements
		VkMemoryRequirements bufferMemReq;
		vkGetBufferMemoryRequirements(m_LogicalDevice, m_Buffer, &bufferMemReq);

		// Fetch device memory properties
		VkPhysicalDeviceMemoryProperties physicalMemProps;
		vkGetPhysicalDeviceMemoryProperties(createInfo.physicalDevice, &physicalMemProps);

		// Find adequate memory type indices
		uint32_t memTypeIndex = UINT32_MAX;

		for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
		{
			if (bufferMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				memTypeIndex = i;
		}

		ASSERT(memTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

		// Allocate memory
		VkMemoryAllocateInfo memoryAllocInfo {
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize  = bufferMemReq.size,
			.memoryTypeIndex = memTypeIndex,
		};

		VKC(vkAllocateMemory(m_LogicalDevice, &memoryAllocInfo, nullptr, &m_BufferMemory));
		VKC(vkBindBufferMemory(m_LogicalDevice, m_Buffer, m_BufferMemory, 0u));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Write starting data to buffer
	{
		if (createInfo.startingData)
		{
			void* data;
			VKC(vkMapMemory(m_LogicalDevice, m_BufferMemory, 0u, createInfo.size, 0x0, &data));
			memcpy(data, createInfo.startingData, static_cast<size_t>(createInfo.size));
			vkUnmapMemory(m_LogicalDevice, m_BufferMemory);
		}
	}
}

void* Buffer::Map()
{
	void* data;
	VKC(vkMapMemory(m_LogicalDevice, m_BufferMemory, 0u, m_BufferSize, 0x0, &data));
	return data;
}

void Buffer::Unmap()
{
	vkUnmapMemory(m_LogicalDevice, m_BufferMemory);
}

Buffer::~Buffer()
{
	vkDestroyBuffer(m_LogicalDevice, m_Buffer, nullptr);
	vkFreeMemory(m_LogicalDevice, m_BufferMemory, nullptr);
}

StagingBuffer::StagingBuffer(BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create vertex and staging buffers
	{
		VkBufferCreateInfo bufferCreateInfo {
			.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size        = createInfo.size,
			.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT | createInfo.usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VkBufferCreateInfo stagingBufferCrateInfo {
			.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size        = createInfo.size,
			.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};

		VKC(vkCreateBuffer(m_LogicalDevice, &bufferCreateInfo, nullptr, &m_Buffer));
		VKC(vkCreateBuffer(m_LogicalDevice, &stagingBufferCrateInfo, nullptr, &m_StagingBuffer));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Fetch buffer's memory requirements and allocate memory
	{
		// Fetch memory requirements
		VkMemoryRequirements bufferMemReq;
		VkMemoryRequirements stagingBufferMemReq;
		vkGetBufferMemoryRequirements(m_LogicalDevice, m_Buffer, &bufferMemReq);
		vkGetBufferMemoryRequirements(m_LogicalDevice, m_StagingBuffer, &stagingBufferMemReq);

		// Fetch device memory properties
		VkPhysicalDeviceMemoryProperties physicalMemProps;
		vkGetPhysicalDeviceMemoryProperties(createInfo.physicalDevice, &physicalMemProps);

		// Find adequate memory type indices
		uint32_t memTypeIndex = UINT32_MAX;
		uint32_t stagingMemTypeIndex = UINT32_MAX;

		for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
		{
			if (bufferMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				memTypeIndex = i;

			if (bufferMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				stagingMemTypeIndex = i;
		}

		ASSERT(memTypeIndex != UINT32_MAX, "Failed to find suitable memory type");
		ASSERT(stagingMemTypeIndex != UINT32_MAX, "Failed to find suitable staging memory type");

		// Allocate memory
		VkMemoryAllocateInfo memoryAllocInfo {
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize  = bufferMemReq.size,
			.memoryTypeIndex = memTypeIndex,
		};

		VkMemoryAllocateInfo stagingMemoryAllocInfo {
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize  = stagingBufferMemReq.size,
			.memoryTypeIndex = stagingMemTypeIndex,
		};

		VKC(vkAllocateMemory(m_LogicalDevice, &memoryAllocInfo, nullptr, &m_BufferMemory));
		VKC(vkBindBufferMemory(m_LogicalDevice, m_Buffer, m_BufferMemory, 0u));

		VKC(vkAllocateMemory(m_LogicalDevice, &stagingMemoryAllocInfo, nullptr, &m_StagingBufferMemory));
		VKC(vkBindBufferMemory(m_LogicalDevice, m_StagingBuffer, m_StagingBufferMemory, 0u));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Write starting data to buffer
	{
		if (createInfo.startingData)
		{
			void* data;
			VKC(vkMapMemory(m_LogicalDevice, m_StagingBufferMemory, 0u, createInfo.size, 0u, &data));
			memcpy(data, createInfo.startingData, static_cast<size_t>(createInfo.size));
			vkUnmapMemory(m_LogicalDevice, m_StagingBufferMemory);

			VkCommandBufferAllocateInfo allocInfo {
				.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.commandPool        = createInfo.commandPool,
				.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1u,
			};
			VkCommandBuffer cmdBuffer;
			VKC(vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &cmdBuffer));

			VkCommandBufferBeginInfo beginInfo {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			};

			VKC(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
			VkBufferCopy bufferCopy {
				.srcOffset = 0u,
				.dstOffset = 0u,
				.size      = createInfo.size,
			};
			vkCmdCopyBuffer(cmdBuffer, m_StagingBuffer, m_Buffer, 1u, &bufferCopy);
			vkEndCommandBuffer(cmdBuffer);

			VkSubmitInfo submitInfo {
				.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.commandBufferCount = 1u,
				.pCommandBuffers    = &cmdBuffer,
			};
			VKC(vkQueueSubmit(createInfo.graphicsQueue, 1u, &submitInfo, VK_NULL_HANDLE));
			VKC(vkQueueWaitIdle(createInfo.graphicsQueue));
			vkFreeCommandBuffers(m_LogicalDevice, createInfo.commandPool, 1u, &cmdBuffer);
		}
	}
}

StagingBuffer::~StagingBuffer()
{
	vkDestroyBuffer(m_LogicalDevice, m_Buffer, nullptr);
	vkFreeMemory(m_LogicalDevice, m_BufferMemory, nullptr);
	vkDestroyBuffer(m_LogicalDevice, m_StagingBuffer, nullptr);
	vkFreeMemory(m_LogicalDevice, m_StagingBufferMemory, nullptr);
}
