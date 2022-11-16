#include "BindlessVk/Buffer.hpp"

#include "BindlessVk/BindlessVkConfig.hpp"

namespace BINDLESSVK_NAMESPACE {

Buffer::Buffer(BufferCreateInfo& info)
    : m_Device(info.device)
    , m_BlockCount(info.blockCount)
    , m_MinBlockSize(info.minBlockSize)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer and write the initial data to it(if any)
	{
		const vk::DeviceSize minAlignment = m_Device->physical.getProperties().limits.minUniformBufferOffsetAlignment;

		// Round up minBlockSize to be the next multiple of minUniformBufferOffsetAlignment
		m_BlockSize = (info.minBlockSize + minAlignment - 1) & -minAlignment;
		m_WholeSize = m_BlockSize * m_BlockCount;

		vk::BufferCreateInfo bufferCreateInfo {
			{},                          //flags
			m_WholeSize,                 // size
			info.usage,                  // usage
			vk::SharingMode::eExclusive, // sharingMode
		};

		m_Buffer = m_Device->allocator.createBuffer(bufferCreateInfo, { {}, vma::MemoryUsage::eCpuToGpu });

		if (info.initialData)
		{
			memcpy(m_Device->allocator.mapMemory(m_Buffer), info.initialData, static_cast<size_t>(m_WholeSize));
			m_Device->allocator.unmapMemory(m_Buffer);
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
	m_Device->allocator.destroyBuffer(m_Buffer, m_Buffer);
}

void* Buffer::MapBlock(uint32_t blockIndex)
{
	return ((uint8_t*)m_Device->allocator.mapMemory(m_Buffer)) + (m_BlockSize * blockIndex);
}

void Buffer::Unmap()
{
	m_Device->allocator.unmapMemory(m_Buffer);
}

StagingBuffer::StagingBuffer(const BufferCreateInfo& info)
    : m_Device(info.device)
{
	// @todo: Merge 2 buffer classes into 1
	BVK_ASSERT(info.blockCount == 1, "Block count for staging buffers SHOULD be 1 (this will be fixed)");

	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer & staging buffer, then write the initial data to it(if any)
	{
		vk::BufferCreateInfo bufferCreateInfo {
			{},                                                 //flags
			info.minBlockSize,                                  // size
			vk::BufferUsageFlagBits::eTransferDst | info.usage, // usage
			vk::SharingMode::eExclusive,                        // sharingMode
		};

		vk::BufferCreateInfo stagingBufferCrateInfo {
			{},                                    //flags
			info.minBlockSize,                     // size
			vk::BufferUsageFlagBits::eTransferSrc, // usage
			vk::SharingMode::eExclusive,           // sharingMode
		};

		m_Buffer        = m_Device->allocator.createBuffer(bufferCreateInfo, { {}, vma::MemoryUsage::eGpuOnly });
		m_StagingBuffer = m_Device->allocator.createBuffer(stagingBufferCrateInfo, { {}, vma::MemoryUsage::eCpuOnly });

		if (info.initialData)
		{
			// Copy starting data to staging buffer
			memcpy(m_Device->allocator.mapMemory(m_StagingBuffer), info.initialData, static_cast<size_t>(info.minBlockSize));
			m_Device->allocator.unmapMemory(m_StagingBuffer);

			// #TODO: Make simpler by calling an InstantSubmit function
			// Allocate cmd buffer
			vk::CommandBufferAllocateInfo allocInfo {
				info.commandPool,                 // commandPool
				vk::CommandBufferLevel::ePrimary, // level
				1u,                               // commandBufferCount
			};
			vk::CommandBuffer cmdBuffer = m_Device->logical.allocateCommandBuffers(allocInfo)[0];

			// Record cmd buffer
			vk::CommandBufferBeginInfo beginInfo {
				vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // flags
			};

			cmdBuffer.begin(beginInfo);
			vk::BufferCopy bufferCopy {
				0u,                // srcOffset
				0u,                // dstOffset
				info.minBlockSize, // size
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


			// Wait for and free cmd buffer
			m_Device->graphicsQueue.waitIdle();
			m_Device->logical.freeCommandBuffers(info.commandPool, 1u, &cmdBuffer);
		}
	}
}

StagingBuffer::~StagingBuffer()
{
	if (m_Buffer.buffer)
	{
		m_Device->allocator.destroyBuffer(m_Buffer, m_Buffer);
		m_Device->allocator.destroyBuffer(m_StagingBuffer, m_StagingBuffer);
	}
}

} // namespace BINDLESSVK_NAMESPACE
