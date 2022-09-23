#include "Graphics/Buffer.hpp"

Buffer::Buffer(BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice), m_BufferSize(createInfo.size), m_DeletionQueue("Buffer")
{
	LOG(warn, "Buffer size: {}", createInfo.size);
	/////////////////////////////////////////////////////////////////////////////////
	// Create vertex and staging buffers
	{
		vk::BufferCreateInfo bufferCreateInfo {
			{},                          //flags
			createInfo.size,             // size
			createInfo.usage,            // usage
			vk::SharingMode::eExclusive, // sharingMode
		};

		m_Buffer = m_LogicalDevice.createBuffer(bufferCreateInfo);
		m_DeletionQueue.Enqueue([=]() {
			m_LogicalDevice.destroyBuffer(m_Buffer, nullptr);
		});
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Fetch buffer's memory requirements and allocate memory
	{
		// Fetch memory requirements
		vk::MemoryRequirements bufferMemReq = m_LogicalDevice.getBufferMemoryRequirements(m_Buffer);

		// Fetch device memory properties
		vk::PhysicalDeviceMemoryProperties physicalMemProps = createInfo.physicalDevice.getMemoryProperties();

		// Find adequate memory type indices
		uint32_t memTypeIndex = UINT32_MAX;

		for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
		{
			if (bufferMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))
				memTypeIndex = i;
		}

		ASSERT(memTypeIndex != UINT32_MAX, "Failed to find suitable memory type");

		// Allocate memory
		vk::MemoryAllocateInfo memoryAllocInfo {
			bufferMemReq.size, // allocationSize
			memTypeIndex,      // memoryTypeIndex
		};

		m_BufferMemory = m_LogicalDevice.allocateMemory(memoryAllocInfo, nullptr);
		m_DeletionQueue.Enqueue([=]() {
			m_LogicalDevice.freeMemory(m_BufferMemory, nullptr);
		});


		m_LogicalDevice.bindBufferMemory(m_Buffer, m_BufferMemory, 0u);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Write initial data to the buffer
	{
		if (createInfo.initialData)
		{
			void* data = m_LogicalDevice.mapMemory(m_BufferMemory, 0u, createInfo.size);

			memcpy(data, createInfo.initialData, static_cast<size_t>(createInfo.size));
			m_LogicalDevice.unmapMemory(m_BufferMemory);
		}
	}
}

Buffer::~Buffer()
{
	m_DeletionQueue.Flush();
}

void* Buffer::Map()
{
	return m_LogicalDevice.mapMemory(m_BufferMemory, 0u, m_BufferSize);
}

void Buffer::Unmap()
{
	m_LogicalDevice.unmapMemory(m_BufferMemory);
}

StagingBuffer::StagingBuffer(BufferCreateInfo& createInfo)
    : m_LogicalDevice(createInfo.logicalDevice), m_DeletionQueue("StagingBuffer")
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create vertex and staging buffers
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

		m_Buffer = m_LogicalDevice.createBuffer(bufferCreateInfo);
		m_DeletionQueue.Enqueue([=]() {
			m_LogicalDevice.destroyBuffer(m_Buffer, nullptr);
		});

		m_StagingBuffer = m_LogicalDevice.createBuffer(stagingBufferCrateInfo);
		m_DeletionQueue.Enqueue([=]() {
			m_LogicalDevice.destroyBuffer(m_StagingBuffer, nullptr);
		});
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Fetch buffer's memory requirements and allocate memory
	{
		// Fetch memory requirements
		vk::MemoryRequirements bufferMemReq        = m_LogicalDevice.getBufferMemoryRequirements(m_Buffer);
		vk::MemoryRequirements stagingBufferMemReq = m_LogicalDevice.getBufferMemoryRequirements(m_Buffer);

		// Fetch device memory properties
		vk::PhysicalDeviceMemoryProperties physicalMemProps = createInfo.physicalDevice.getMemoryProperties();

		// Find adequate memories' indices
		uint32_t memTypeIndex        = UINT32_MAX;
		uint32_t stagingMemTypeIndex = UINT32_MAX;

		for (uint32_t i = 0; i < physicalMemProps.memoryTypeCount; i++)
		{
			if (bufferMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
				memTypeIndex = i;

			if (stagingBufferMemReq.memoryTypeBits & (1 << i) && physicalMemProps.memoryTypes[i].propertyFlags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))
				stagingMemTypeIndex = i;
		}

		ASSERT(memTypeIndex != UINT32_MAX, "Failed to find suitable memory type");
		ASSERT(stagingMemTypeIndex != UINT32_MAX, "Failed to find suitable staging memory type");

		// Allocate memory
		vk::MemoryAllocateInfo memoryAllocInfo {
			bufferMemReq.size, // allocationSize
			memTypeIndex,      // memoryTypeIndex
		};

		vk::MemoryAllocateInfo stagingMemoryAllocInfo {
			stagingBufferMemReq.size, // allocationSize
			stagingMemTypeIndex,      // memoryTypeIndex
		};

		// Bind memory
		m_BufferMemory = m_LogicalDevice.allocateMemory(memoryAllocInfo, nullptr);
		m_DeletionQueue.Enqueue([=]() {
			m_LogicalDevice.freeMemory(m_BufferMemory, nullptr);
		});

		m_LogicalDevice.bindBufferMemory(m_Buffer, m_BufferMemory, 0u);

		m_StagingBufferMemory = m_LogicalDevice.allocateMemory(memoryAllocInfo, nullptr);
		m_DeletionQueue.Enqueue([=]() {
			m_LogicalDevice.freeMemory(m_StagingBufferMemory, nullptr);
		});

		m_LogicalDevice.bindBufferMemory(m_StagingBuffer, m_StagingBufferMemory, 0u);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Write initial data to the host local buffer
	{
		if (createInfo.initialData) // #TODO: Optimize
		{
			// Copy starting data to staging buffer
			void* data = m_LogicalDevice.mapMemory(m_StagingBufferMemory, 0u, createInfo.size);
			memcpy(data, createInfo.initialData, static_cast<size_t>(createInfo.size));
			m_LogicalDevice.unmapMemory(m_StagingBufferMemory);

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
    m_DeletionQueue.Flush();
}
