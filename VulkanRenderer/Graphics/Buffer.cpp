#include "Graphics/Buffer.hpp"

Buffer::Buffer(BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
    , m_Allocator(createInfo.allocator)
    , m_BlockCount(createInfo.blockCount)
    , m_MinBlockSize(createInfo.minBlockSize)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer and write the initial data to it(if any)
	{
		const vk::DeviceSize minAlignment = createInfo.physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;

		// Round up minBlockSize to be the next multiple of minUniformBufferOffsetAlignment
		m_BlockSize = (createInfo.minBlockSize + minAlignment - 1) & -minAlignment;
		m_WholeSize = m_BlockSize * m_BlockCount;

		vk::BufferCreateInfo bufferCreateInfo {
			{},                          //flags
			m_WholeSize,                 // size
			createInfo.usage,            // usage
			vk::SharingMode::eExclusive, // sharingMode
		};

		m_Buffer = m_Allocator.createBuffer(bufferCreateInfo, { {}, vma::MemoryUsage::eCpuToGpu });

		if (createInfo.initialData)
		{
			memcpy(m_Allocator.mapMemory(m_Buffer), createInfo.initialData, static_cast<size_t>(m_WholeSize));
			m_Allocator.unmapMemory(m_Buffer);
		}
	}

	m_DescriptorInfo = {
		m_Buffer,      // buffer
		0u,            // offset
		VK_WHOLE_SIZE, // range
	};
}

Buffer::~Buffer()
{
	m_Allocator.destroyBuffer(m_Buffer, m_Buffer);
}

void* Buffer::MapBlock(uint32_t blockIndex)
{
	return ((uint8_t*)m_Allocator.mapMemory(m_Buffer)) + (m_BlockSize * blockIndex);
}

void Buffer::Unmap()
{
	m_Allocator.unmapMemory(m_Buffer);
}

StagingBuffer::StagingBuffer(const BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice)
    , m_Allocator(createInfo.allocator)
{
	// @todo: Merge 2 buffer classes into 1
	ASSERT(createInfo.blockCount == 1, "Block count for staging buffers SHOULD be 1 (this will be fixed)");

	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer & staging buffer, then write the initial data to it(if any)
	{
		vk::BufferCreateInfo bufferCreateInfo {
			{},                                                       //flags
			createInfo.minBlockSize,                                  // size
			vk::BufferUsageFlagBits::eTransferDst | createInfo.usage, // usage
			vk::SharingMode::eExclusive,                              // sharingMode
		};

		vk::BufferCreateInfo stagingBufferCrateInfo {
			{},                                    //flags
			createInfo.minBlockSize,               // size
			vk::BufferUsageFlagBits::eTransferSrc, // usage
			vk::SharingMode::eExclusive,           // sharingMode
		};

		m_Buffer        = m_Allocator.createBuffer(bufferCreateInfo, { {}, vma::MemoryUsage::eGpuOnly });
		m_StagingBuffer = m_Allocator.createBuffer(stagingBufferCrateInfo, { {}, vma::MemoryUsage::eCpuOnly });

		if (createInfo.initialData)
		{
			// Copy starting data to staging buffer
			memcpy(m_Allocator.mapMemory(m_StagingBuffer), createInfo.initialData, static_cast<size_t>(createInfo.minBlockSize));
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
				0u,                      // srcOffset
				0u,                      // dstOffset
				createInfo.minBlockSize, // size
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
