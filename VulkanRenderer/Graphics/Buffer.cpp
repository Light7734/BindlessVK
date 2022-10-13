#include "Graphics/Buffer.hpp"

Buffer::Buffer(BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
    , m_BufferSize(createInfo.size)
    , m_Allocator(createInfo.allocator)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer and write the initial data to it(if any)
	{
		vk::BufferCreateInfo bufferCreateInfo {
			{},                          //flags
			createInfo.size,             // size
			createInfo.usage,            // usage
			vk::SharingMode::eExclusive, // sharingMode
		};

		m_Buffer = m_Allocator.createBuffer(bufferCreateInfo, { {}, vma::MemoryUsage::eCpuToGpu });

		if (createInfo.initialData)
		{
			memcpy(m_Allocator.mapMemory(m_Buffer), createInfo.initialData, static_cast<size_t>(createInfo.size));
			m_Allocator.unmapMemory(m_Buffer);
		}
	}
}

Buffer::~Buffer()
{
	m_Allocator.destroyBuffer(m_Buffer, m_Buffer);
}

void* Buffer::Map()
{
	return m_Allocator.mapMemory(m_Buffer);
}

void Buffer::Unmap()
{
	m_Allocator.unmapMemory(m_Buffer);
}

StagingBuffer::StagingBuffer(const BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
    , m_Allocator(createInfo.allocator)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer & staging buffer, then write the initial data to it(if any)
	{
		vk::BufferCreateInfo bufferCreateInfo {
			{},                                                       //flags
			createInfo.size,                                          // size
			vk::BufferUsageFlagBits::eTransferDst | createInfo.usage, // usage
			vk::SharingMode::eExclusive,                              // sharingMode
		};

		vk::BufferCreateInfo stagingBufferCrateInfo {
			{},                                    //flags
			createInfo.size,                       // size
			vk::BufferUsageFlagBits::eTransferSrc, // usage
			vk::SharingMode::eExclusive,           // sharingMode
		};

		m_Buffer        = m_Allocator.createBuffer(bufferCreateInfo, { {}, vma::MemoryUsage::eGpuOnly });
		m_StagingBuffer = m_Allocator.createBuffer(stagingBufferCrateInfo, { {}, vma::MemoryUsage::eCpuOnly });

		if (createInfo.initialData)
		{
			// Copy starting data to staging buffer
			memcpy(m_Allocator.mapMemory(m_StagingBuffer), createInfo.initialData, static_cast<size_t>(createInfo.size));
			m_Allocator.unmapMemory(m_StagingBuffer);

			// #TODO: Make simpler by calling an InstantSubmit function
			// Allocate cmd buffer
			vk::CommandBufferAllocateInfo allocInfo {
				createInfo.commandPool,           // commandPool
				vk::CommandBufferLevel::ePrimary, // level
				1u,                               // commandBufferCount
			};
			vk::CommandBuffer cmdBuffer = m_LogicalDevice.allocateCommandBuffers(allocInfo)[0];

			// Record cmd buffer
			vk::CommandBufferBeginInfo beginInfo {
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // flags
			};

			cmdBuffer.begin(beginInfo);
			vk::BufferCopy bufferCopy {
				0u,              // srcOffset
				0u,              // dstOffset
				createInfo.size, // size
			};
			cmdBuffer.copyBuffer(m_StagingBuffer, m_Buffer, 1u, &bufferCopy);
			cmdBuffer.end();

			// Submit cmd buffer
			vk::SubmitInfo submitInfo {
				0u,         // waitSemaphoreCount
				nullptr,    // pWaitSemaphores
				nullptr,    // pWaitDstStageMask
				1u,         // commandBufferCount
				&cmdBuffer, // pCommandBuffers
				0u,         // signalSemaphoreCount
				nullptr,    // pSignalSemaphores
			};
			VKC(createInfo.graphicsQueue.submit(1u, &submitInfo, VK_NULL_HANDLE));

			// Wait for and free cmd buffer
			createInfo.graphicsQueue.waitIdle();
			m_LogicalDevice.freeCommandBuffers(createInfo.commandPool, 1u, &cmdBuffer);
		}
	}
}

StagingBuffer::~StagingBuffer()
{
	if (m_Buffer.buffer)
	{
		m_Allocator.destroyBuffer(m_Buffer, m_Buffer);
		m_Allocator.destroyBuffer(m_StagingBuffer, m_StagingBuffer);
	}
}
