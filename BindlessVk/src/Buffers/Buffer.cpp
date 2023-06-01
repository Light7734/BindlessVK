#include "BindlessVk/Buffers/Buffer.hpp"

namespace BINDLESSVK_NAMESPACE {

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

	allocated_buffer = memory_allocator->vma().createBuffer(
	    vk::BufferCreateInfo {
	        {},
	        whole_size,
	        buffer_usage,
	        vk::SharingMode::eExclusive,
	    },
	    vma_info
	);

	auto &[buffer, allocation] = allocated_buffer;

	descriptor_info = vk::DescriptorBufferInfo {
		buffer,
		0u,
		VK_WHOLE_SIZE,
	};

	memory_allocator->vma().setAllocationName(allocation, this->debug_name.c_str());
	vk_context->get_debug_utils()->set_object_name(device->vk(), buffer, this->debug_name);
}

Buffer::~Buffer()
{
	if (!device)
		return;

	auto const &[buffer, allocation] = allocated_buffer;

	memory_allocator->vma().destroyBuffer(buffer, allocation);
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
		auto &[buffer, allocation] = allocated_buffer;
		cmd.copyBuffer(*src_buffer.vk(), buffer, 1u, &src_copy);
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
	auto &[buffer, allocation] = allocated_buffer;
	auto const map = (u8 *)memory_allocator->vma().mapMemory(allocation);
	return map + (block_size * block_index);
}

auto Buffer::map_all() -> vec<void *>
{
	auto maps = vec<void *>(block_count);

	auto &[buffer, allocation] = allocated_buffer;
	auto const vk_map = (u8 *)memory_allocator->vma().mapMemory(allocation);

	for (u32 i = 0; i < block_count; ++i)
		maps[i] = vk_map + (block_size * i);

	return maps;
}

auto Buffer::map_all_zeroed() -> vec<void *>
{
	auto maps = vec<void *>(block_count);

	auto &[buffer, allocation] = allocated_buffer;
	auto const vk_map = (u8 *)memory_allocator->vma().mapMemory(allocation);

	memset(vk_map, {}, block_size * block_count);

	for (u32 i = 0; i < block_count; ++i)
		maps[i] = vk_map + (block_size * i);

	return maps;
}

void *Buffer::map_block_zeroed(u32 const block_index)
{
	auto map = map_block(block_index);
	memset(map, {}, get_block_size());
	return map;
}

void Buffer::unmap() const
{
	auto &[buffer, allocation] = allocated_buffer;

	memory_allocator->vma().unmapMemory(allocation);
}

} // namespace BINDLESSVK_NAMESPACE
