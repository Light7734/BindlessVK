#include "BindlessVk/Buffer.hpp"

namespace BINDLESSVK_NAMESPACE {

Buffer::Buffer(
    c_str const debug_name,
    VkContext const *const vk_context,
    vk::BufferUsageFlags const buffer_usage,
    vma::AllocationCreateInfo const &vma_info,
    vk::DeviceSize const desired_block_size,
    u32 const block_count
)
    : vk_context(vk_context)
    , block_count(block_count)
    , valid_block_size(desired_block_size)
{
	calculate_block_size();

	const auto device = vk_context->get_device();
	const auto allocator = vk_context->get_allocator();

	buffer = allocator.createBuffer(
	    {
	        {},
	        whole_size,
	        buffer_usage,
	        vk::SharingMode::eExclusive,
	    },
	    vma_info
	);

	descriptor_info = {
		buffer,
		0u,
		VK_WHOLE_SIZE,
	};

	allocator.setAllocationName(buffer.allocation, debug_name);
	vk_context->set_object_name(buffer.buffer, debug_name);
}

Buffer::~Buffer()
{
	if (!vk_context)
		return;

	vk_context->get_allocator().destroyBuffer(buffer, buffer);
}

void Buffer::write_data(
    void const *const src_data,
    usize const src_data_size,
    u32 const block_index
)
{
	memcpy(map_block(block_index), src_data, src_data_size);
	unmap();
}

void Buffer::write_buffer(Buffer const &src_buffer, vk::BufferCopy const &src_copy)
{
	vk_context->immediate_submit([&](vk::CommandBuffer cmd) {
		cmd.copyBuffer(*const_cast<Buffer &>(src_buffer).get_buffer(), buffer, 1u, &src_copy);
	});
}

void Buffer::calculate_block_size()
{
	const auto gpu_properties = vk_context->get_gpu().getProperties();
	const auto min_alignment = gpu_properties.limits.minUniformBufferOffsetAlignment;

	// Round up minBlockSize to be the next multiple of minUniformBufferOffsetAlignment
	block_size = (valid_block_size + min_alignment - 1) & -min_alignment;
	whole_size = block_size * block_count;
}

void *Buffer::map_block(u32 const block_index)
{
	return ((u8 *)vk_context->get_allocator().mapMemory(buffer)) + (block_size * block_index);
}

void Buffer::unmap()
{
	vk_context->get_allocator().unmapMemory(buffer);
}

} // namespace BINDLESSVK_NAMESPACE
