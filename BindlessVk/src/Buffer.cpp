#include "BindlessVk/Buffer.hpp"

#include "BindlessVk/BindlessVkConfig.hpp"

namespace BINDLESSVK_NAMESPACE {

Buffer::Buffer(
    const char* debug_name,
    Device* device,
    vk::BufferUsageFlags buffer_usage,
    const vma::AllocationCreateInfo& vma_info,
    vk::DeviceSize desired_block_size,
    u32 block_count
)
    : device(device)
    , block_count(block_count)
    , valid_block_size(desired_block_size)
{
	calculate_block_size();

	buffer = device->allocator.createBuffer(
	    {
	        {},
	        whole_size,
	        buffer_usage,
	        vk::SharingMode::eExclusive,
	    },
	    vma_info
	);

	BVK_LOG(
	    LogLvl::eTrace,
	    "{} created with {}",
	    debug_name,
	    (u32)device->allocator.getAllocationMemoryProperties(buffer.allocation)
	);

	descriptor_info = {
		buffer,
		0u,
		VK_WHOLE_SIZE,
	};

	device->allocator.setAllocationName(buffer.allocation, debug_name);
	device->set_object_name(buffer.buffer, debug_name);
}

Buffer::~Buffer()
{
	device->allocator.destroyBuffer(buffer, buffer);
}

void Buffer::write_data(const void* src_data, usize src_data_size, u32 block_index)
{
	memcpy(map_block(block_index), src_data, src_data_size);
	unmap();
}

void Buffer::write_buffer(const Buffer& src_buffer, const vk::BufferCopy& src_copy)
{
	device->immediate_submit([&](vk::CommandBuffer cmd) {
		cmd.copyBuffer(*const_cast<Buffer&>(src_buffer).get_buffer(), buffer, 1u, &src_copy);
	});
}

void Buffer::calculate_block_size()
{
	const auto min_alignment = device->properties.limits.minUniformBufferOffsetAlignment;

	// Round up minBlockSize to be the next multiple of minUniformBufferOffsetAlignment
	block_size = (valid_block_size + min_alignment - 1) & -min_alignment;
	whole_size = block_size * block_count;
}

void* Buffer::map_block(u32 block_index)
{
	return ((uint8_t*)device->allocator.mapMemory(buffer)) + (block_size * block_index);
}

void Buffer::unmap()
{
	device->allocator.unmapMemory(buffer);
}

} // namespace BINDLESSVK_NAMESPACE
