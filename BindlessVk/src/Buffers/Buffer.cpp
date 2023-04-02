#include "BindlessVk/Buffers/Buffer.hpp"

namespace BINDLESSVK_NAMESPACE {

AllocatedBuffer::AllocatedBuffer(
    MemoryAllocator const *const memory_allocator,
    vk::BufferCreateInfo const &create_info,
    vma::AllocationCreateInfo const &allocate_info
)
    : memory_allocator(memory_allocator)
    , allocated_buffer(memory_allocator->vma().createBuffer(create_info, allocate_info))
{
}

AllocatedBuffer::AllocatedBuffer(AllocatedBuffer &&other)
{
	*this = std::move(other);
}

AllocatedBuffer &AllocatedBuffer::operator=(AllocatedBuffer &&other)
{
	this->memory_allocator = other.memory_allocator;
	this->allocated_buffer = other.allocated_buffer;

	other.memory_allocator = {};

	return *this;
}

AllocatedBuffer::~AllocatedBuffer()
{
	auto const &[buffer, allocation] = allocated_buffer;

	if (memory_allocator)
		memory_allocator->vma().destroyBuffer(buffer, allocation);
}

Buffer::Buffer(
    VkContext const *const vk_context,
    MemoryAllocator const *const memory_allocator,
    vk::BufferUsageFlags const buffer_usage,
    vma::AllocationCreateInfo const &vma_info,
    vk::DeviceSize const desired_block_size,
    u32 const block_count,
    str_view const debug_name /* = default_debug_name */
)
    : device(vk_context->get_device())
    , memory_allocator(memory_allocator)
    , block_count(block_count)
    , valid_block_size(desired_block_size)
    , debug_name(debug_name)
{
	calculate_block_size(vk_context->get_gpu());

	buffer = AllocatedBuffer(
	    memory_allocator,
	    vk::BufferCreateInfo {
	        {},
	        whole_size,
	        buffer_usage,
	        vk::SharingMode::eExclusive,
	    },
	    vma_info
	);

	descriptor_info = vk::DescriptorBufferInfo {
		*buffer.vk(),
		0u,
		VK_WHOLE_SIZE,
	};

	memory_allocator->vma().setAllocationName(buffer.get_allocation(), this->debug_name.c_str());
	vk_context->get_debug_utils()->set_object_name(device->vk(), *buffer.vk(), this->debug_name);
}

Buffer::Buffer(Buffer &&other) noexcept
{
	*this = std::move(other);
}

Buffer &Buffer::operator=(Buffer &&other) noexcept
{
	this->device = other.device;
	this->memory_allocator = other.memory_allocator;
	this->buffer = std::move(other.buffer);
	this->whole_size = other.whole_size;
	this->block_size = other.block_size;
	this->block_count = other.block_count;
	this->valid_block_size = other.valid_block_size;
	this->descriptor_info = other.descriptor_info;

	other.memory_allocator = {};

	return *this;
}

Buffer::~Buffer()
{
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
	device->immediate_submit([&](vk::CommandBuffer cmd) {
		cmd.copyBuffer(*src_buffer.vk(), *buffer.vk(), 1u, &src_copy);
	});
}

void Buffer::calculate_block_size(Gpu const *const gpu)
{
	auto const gpu_properties = gpu->vk().getProperties();
	auto const min_alignment = gpu_properties.limits.minUniformBufferOffsetAlignment;

	// Round up minBlockSize to be the next multiple of minUniformBufferOffsetAlignment
	block_size = (valid_block_size + min_alignment - 1) & -min_alignment;
	whole_size = block_size * block_count;
}

void *Buffer::map_block(u32 const block_index)
{
	auto const map = (u8 *)memory_allocator->vma().mapMemory(buffer.get_allocation());
	return map + (block_size * block_index);
}

void Buffer::unmap() const
{
	memory_allocator->vma().unmapMemory(buffer.get_allocation());
}

} // namespace BINDLESSVK_NAMESPACE
