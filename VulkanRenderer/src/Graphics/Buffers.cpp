#include "Graphics/Buffers.h"

#include "Graphics/Device.h"
#include "Graphics/RendererCommand.h"

Buffer::Buffer(class Device* device, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties)
    : m_Device(device)
    , m_Buffer(VK_NULL_HANDLE)
    , m_Memory(VK_NULL_HANDLE)
{
	// buffer create-info
	VkBufferCreateInfo bufferCreateInfo {
		.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size        = size,
		.usage       = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	// create buffer
	VKC(vkCreateBuffer(m_Device->logical(), &bufferCreateInfo, nullptr, &m_Buffer));

	VkMemoryRequirements memoryRequirments;
	vkGetBufferMemoryRequirements(m_Device->logical(), m_Buffer, &memoryRequirments);

	VkMemoryAllocateInfo allocateInfo = {
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize  = memoryRequirments.size,
		.memoryTypeIndex = FetchMemoryType(memoryRequirments.memoryTypeBits, memoryProperties),
	};

	VKC(vkAllocateMemory(m_Device->logical(), &allocateInfo, nullptr, &m_Memory));
	VKC(vkBindBufferMemory(m_Device->logical(), m_Buffer, m_Memory, 0u));
}

Buffer::~Buffer()
{
	vkDestroyBuffer(m_Device->logical(), m_Buffer, nullptr);
	vkFreeMemory(m_Device->logical(), m_Memory, nullptr);
}

void Buffer::CopyBufferToSelf(Buffer* src, uint32_t size, VkCommandPool commandPool, VkQueue graphicsQueue)
{
	VkCommandBuffer commandBuffer = RendererCommand::BeginOneTimeCommand();

	VkBufferCopy copyRegion {
		.srcOffset = 0u,
		.dstOffset = 0u,
		.size      = size
	};

	vkCmdCopyBuffer(commandBuffer, *src->GetBuffer(), m_Buffer, 1u, &copyRegion);

	RendererCommand::EndOneTimeCommand(&commandBuffer);
}

void* Buffer::Map(uint32_t size)
{
	if (m_Mapped)
	{
		LOG(warn, "Buffer already mapped");
		return nullptr;
	}

	void* map;
	VKC(vkMapMemory(m_Device->logical(), m_Memory, NULL, size, NULL, &map));
	m_Mapped = true;
	return map;
}

void Buffer::Unmap()
{
	if (!m_Mapped)
	{
		LOG(warn, "Buffer not mapped");
		return;
	}

	vkUnmapMemory(m_Device->logical(), m_Memory);
	m_Mapped = false;
}

uint32_t Buffer::FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_Device->physical(), &physicalMemoryProperties);

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
