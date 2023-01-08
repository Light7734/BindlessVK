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
    : device(device)
    , block_count(block_count)
    , min_block_size(min_block_size)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Create buffer and write the initial data to it(if any)
	{
		const vk::DeviceSize min_alignment =
		    device->physical.getProperties().limits.minUniformBufferOffsetAlignment;

		// Round up minBlockSize to be the next multiple of
		// minUniformBufferOffsetAlignment
		block_size = (min_block_size + min_alignment - 1) & -min_alignment;
		whole_size = block_size * block_count;

		vk::BufferCreateInfo buffer_info {
			{},                          // flags
			whole_size,                  // size
			usage,                       // usage
			vk::SharingMode::eExclusive, // sharingMode
		};

		buffer = device->allocator.createBuffer(
		    buffer_info,
		    {
		        {},
		        vma::MemoryUsage::eCpuToGpu,
		    }
		);

		if (initial_data)
		{
			memcpy(
			    device->allocator.mapMemory(buffer),
			    initial_data,
			    static_cast<size_t>(whole_size)
			);
			device->allocator.unmapMemory(buffer);
		}
	}

	descriptor_info = {
		buffer,        // buffer
		0u,            // offset
		VK_WHOLE_SIZE, // range
	};


	device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
	    vk::ObjectType::eBuffer,
	    (u64)(VkBuffer)(buffer.buffer),
	    name,
	});
}

Buffer::~Buffer()
{
	device->allocator.destroyBuffer(buffer, buffer);
}

void* Buffer::map_block(uint32_t block_index)
{
	return ((uint8_t*)device->allocator.mapMemory(buffer)) + (block_size * block_index);
}

void Buffer::unmap()
{
	device->allocator.unmapMemory(buffer);
}

StagingBuffer::StagingBuffer(
    const char* name,
    Device* device,
    vk::BufferUsageFlags usage,
    vk::DeviceSize min_block_size,
    uint32_t block_count,
    const void* initial_data /* = {} */
)
    : device(device)
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

		buffer = device->allocator.createBuffer(buffer_info, { {}, vma::MemoryUsage::eGpuOnly });
		staging_buffer =
		    device->allocator.createBuffer(staging_buffer_info, { {}, vma::MemoryUsage::eCpuOnly });

		if (initial_data)
		{
			// Copy starting data to staging buffer
			memcpy(
			    device->allocator.mapMemory(staging_buffer),
			    initial_data,
			    static_cast<size_t>(min_block_size)
			);

			device->allocator.unmapMemory(staging_buffer);

			device->immediate_submit([&](vk::CommandBuffer cmd) {
				vk::BufferCopy buffer_copy {
					0u,             // srcOffset
					0u,             // dstOffset
					min_block_size, // size
				};

				cmd.copyBuffer(staging_buffer, buffer, 1u, &buffer_copy);
			});
		}
	}

	device->logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
	    vk::ObjectType::eBuffer,
	    (u64)(VkBuffer)(buffer.buffer),
	    name,
	});
}

StagingBuffer::~StagingBuffer()
{
	device->logical.waitIdle();
	if (buffer.buffer)
	{
		device->allocator.destroyBuffer(buffer, buffer);
		device->allocator.destroyBuffer(staging_buffer, staging_buffer);
	}
}

} // namespace BINDLESSVK_NAMESPACE
