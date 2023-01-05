#include "BindlessVk/Buffer.hpp"

#include "BindlessVk/BindlessVkConfig.hpp"
#include "BindlessVk/Model.hpp"

namespace BINDLESSVK_NAMESPACE {

Buffer::Buffer(
  const char* name,
  Device* device,
  vk::BufferUsageFlags usage,
  vk::DeviceSize min_block_size,
  uint32_t block_count,
  const void* initial_data /* = {} */
)
  : m_device(device)
  , m_block_count(block_count)
  , m_min_block_size(min_block_size)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer and write the initial data to it(if any)
	{
		const vk::DeviceSize min_alignment = m_device->physical.getProperties()
		                                       .limits
		                                       .minUniformBufferOffsetAlignment;

		// Round up minBlockSize to be the next multiple of
		// minUniformBufferOffsetAlignment
		m_block_size = (min_block_size + min_alignment - 1) & -min_alignment;
		m_whole_size = m_block_size * m_block_count;

		vk::BufferCreateInfo buffer_info {
			{},                          // flags
			m_whole_size,                // size
			usage,                       // usage
			vk::SharingMode::eExclusive, // sharingMode
		};

		m_buffer = m_device->allocator.createBuffer(
		  buffer_info,
		  {
		    {},
		    vma::MemoryUsage::eCpuToGpu,
		  }
		);

		if (initial_data)
		{
			memcpy(
			  m_device->allocator.mapMemory(m_buffer),
			  initial_data,
			  static_cast<size_t>(m_whole_size)
			);
			m_device->allocator.unmapMemory(m_buffer);
		}
	}

	m_descriptor_info = {
		m_buffer,      // buffer
		0u,            // offset
		VK_WHOLE_SIZE, // range
	};
}

Buffer::~Buffer()
{
	m_device->allocator.destroyBuffer(m_buffer, m_buffer);
}

void* Buffer::map_block(uint32_t block_index)
{
	return ((uint8_t*)m_device->allocator.mapMemory(m_buffer))
	       + (m_block_size * block_index);
}

void Buffer::unmap()
{
	m_device->allocator.unmapMemory(m_buffer);
}

StagingBuffer::StagingBuffer(
  const char* name,
  Device* device,
  vk::BufferUsageFlags usage,
  vk::DeviceSize min_block_size,
  uint32_t block_count,
  const void* initial_data /* = {} */
)
  : m_device(device)
{
	// @todo: Merge 2 buffer classes into 1
	BVK_ASSERT(
	  block_count != 1,
	  "Block count for staging buffers SHOULD be 1 (this will be fixed)"
	);

	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer & staging buffer, then write the initial data to it(if any)
	{
		vk::BufferCreateInfo buffer_info {
			{},                                            // flags
			min_block_size,                                // size
			vk::BufferUsageFlagBits::eTransferDst | usage, // usage
			vk::SharingMode::eExclusive,                   // sharingMode
		};

		vk::BufferCreateInfo staging_buffer_info {
			{},                                    // flags
			min_block_size,                        // size
			vk::BufferUsageFlagBits::eTransferSrc, // usage
			vk::SharingMode::eExclusive,           // sharingMode
		};

		m_buffer = m_device->allocator.createBuffer(
		  buffer_info,
		  { {}, vma::MemoryUsage::eGpuOnly }
		);
		m_staging_buffer = m_device->allocator.createBuffer(
		  staging_buffer_info,
		  { {}, vma::MemoryUsage::eCpuOnly }
		);

		if (initial_data)
		{
			// Copy starting data to staging buffer
			memcpy(
			  m_device->allocator.mapMemory(m_staging_buffer),
			  initial_data,
			  static_cast<size_t>(min_block_size)
			);

			m_device->allocator.unmapMemory(m_staging_buffer);

			m_device->immediate_submit([&](vk::CommandBuffer cmd) {
				vk::BufferCopy buffer_copy {
					0u,             // srcOffset
					0u,             // dstOffset
					min_block_size, // size
				};

				cmd.copyBuffer(m_staging_buffer, m_buffer, 1u, &buffer_copy);
			});
		}
	}
}

StagingBuffer::~StagingBuffer()
{
	m_device->logical.waitIdle();
	if (m_buffer.buffer)
	{
		m_device->allocator.destroyBuffer(m_buffer, m_buffer);
		m_device->allocator.destroyBuffer(m_staging_buffer, m_staging_buffer);
	}
}

} // namespace BINDLESSVK_NAMESPACE
