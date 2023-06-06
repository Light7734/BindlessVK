#include "BindlessVk/Buffers/Buffer.hpp"

#include "Amender/Amender.hpp"

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
	ScopeProfiler _;

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
	device->set_object_name(buffer, "{}", this->debug_name);
}

Buffer::~Buffer()
{
	ScopeProfiler _;

	if (!device)
		return;

	device->vk().waitIdle();

	auto const &[buffer, allocation] = allocated_buffer;

	unmap();
	memory_allocator->vma().destroyBuffer(buffer, allocation);
}

void Buffer::write_data(
    void const *const src_data,
    usize const src_data_size,
    u32 const block_index
)
{
	ScopeProfiler _;

	if (auto *map = map_block(block_index))
	{
		memcpy(map, src_data, src_data_size);
		unmap();
	}
}

void Buffer::write_buffer(Buffer const &src_buffer, vk::BufferCopy const &src_copy)
{
	ScopeProfiler _;

	device->immediate_submit([&](vk::CommandBuffer cmd) {
		auto &[buffer, allocation] = allocated_buffer;
		cmd.copyBuffer(*src_buffer.vk(), buffer, 1u, &src_copy);
	});
}

auto Buffer::map_memory() -> u8 *
{
	ScopeProfiler _;

	if (mapped)
	{
		log_err("Failed to map a previously mapped buffer: {}", debug_name);
		return {};
	}

	mapped = true;

	auto &[buffer, allocation] = allocated_buffer;
	return (u8 *)memory_allocator->vma().mapMemory(allocation);
}

void Buffer::calculate_block_size(Gpu const *const gpu)
{
	ScopeProfiler _;

	auto const gpu_properties = gpu->vk().getProperties();
	auto const min_alignment = gpu_properties.limits.minUniformBufferOffsetAlignment;

	// Round up minBlockSize to be the next multiple of minUniformBufferOffsetAlignment
	block_size = (valid_block_size + min_alignment - 1) & -min_alignment;
	whole_size = block_size * block_count;
}

void *Buffer::map_block(u32 const block_index)
{
	ScopeProfiler _;

	if (auto *map = map_memory())
		return map + (block_size * block_index);

	return {};
}

auto Buffer::map_all() -> vec<void *>
{
	ScopeProfiler _;

	if (auto const map = map_memory())
		return blockify_all_map(map);

	return {};
}

auto Buffer::map_all_zeroed() -> vec<void *>
{
	ScopeProfiler _;

	if (auto const map = map_memory())
	{
		memset(map, {}, block_size * block_count);
		return blockify_all_map(map);
	}

	return {};
}

auto Buffer::blockify_all_map(u8 *map) -> vec<void *>
{
	ScopeProfiler _;

	auto maps = vec<void *>(block_count);
	for (u32 i = 0; i < block_count; ++i)
		maps[i] = map + (block_size * i);

	return maps;
}

void *Buffer::map_block_zeroed(u32 const block_index)
{
	ScopeProfiler _;

	if (auto map = map_block(block_index))
	{
		memset(map, {}, get_block_size());
		return map;
	}

	return {};
}

void Buffer::unmap()
{
	ScopeProfiler _;

	if (!mapped)
		return;

	auto &[buffer, allocation] = allocated_buffer;
	mapped = false;

	memory_allocator->vma().unmapMemory(allocation);
}

} // namespace BINDLESSVK_NAMESPACE
