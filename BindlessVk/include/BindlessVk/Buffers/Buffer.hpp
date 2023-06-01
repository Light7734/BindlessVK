#pragma once

#include "BindlessVk/Allocators/MemoryAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

#include <vulkan/vulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around a vulkan buffer and it's allocation */
class Buffer
{
public:
	/** Default constructor */
	Buffer() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 * @param memory_allocator The memory allocator
	 * @param buffer_usage Usage behaviour of the buffer
	 * @param vma_info Parameters of new vma allocation
	 * @param min_block_size The minimum required size of a single block
	 * @param block_count The number of blocks
	 * @param debug_name Name of the buffer, a null terminated str view
	 *
	 * @note Final block size is min_block_size rounded up to the next multiple of
	 * minUniformBufferOffsetAlignment
	 */
	Buffer(
	    VkContext const *vk_context,
	    MemoryAllocator const *memory_allocator,
	    vk::BufferUsageFlags buffer_usage,
	    vma::AllocationCreateInfo const &vma_info,
	    vk::DeviceSize min_block_size,
	    u32 block_count,
	    str_view debug_name = default_debug_name
	);

	/** Default move constructor */
	Buffer(Buffer &&other) = default;

	/** Default move assignment operator */
	Buffer &operator=(Buffer &&other) = default;

	/** Deleted copy constructor */
	Buffer(Buffer const &other) = delete;

	/** Deleted copy assignment operator */
	Buffer &operator=(Buffer const &other) = delete;

	/** Destructor */
	~Buffer();

	/** Copies data over to the buffer
	 *
	 * @param src_data Source data
	 * @param src_data_size Size of the source data in bytes
	 * @param block_index Index of buffer's block to copy the data to
	 */
	void write_data(void const *src_data, usize src_data_size, u32 block_index);

	/** Copies data over to the buffer
	 *
	 * @param src_buffer Source data buffer
	 * @param copy_info Copy info
	 */
	void write_buffer(Buffer const &src_buffer, vk::BufferCopy const &copy_info);

	/** Maps the buffer and returns an offseted pointer to the beginning of a block
	 *
	 * @param block_index Index of the block to offset the pointer to the beginning of
	 *
	 * @warn Don't map twice without unmapping
	 */
	[[nodiscard]] void *map_block(u32 block_index);

	/** Maps the buffer and zeroes it, then returns an offseted pointer to the beginning of a block
	 *
	 * @param block_index Index of the block to offset the pointer to the beginning of
	 *
	 * @warn Don't map twice without unmapping
	 */
	[[nodiscard]] void *map_block_zeroed(u32 block_index);

	/** Maps the buffer and returns offseted pointers to the beginning of the blocks
	 *
	 * @warn Don't map twice without unmapping
	 */
	[[nodiscard]] auto map_all() -> vec<void *>;

	/** Maps the buffer and zeroes it, then returns offseted pointers to the beginning of the blocks
	 *
	 * @warn Don't map twice without unmapping
	 */
	[[nodiscard]] auto map_all_zeroed() -> vec<void *>;


	/** Unmaps a buffer, can be called without prior mapping */
	void unmap() const;

	/** Returns null terminated str view to debug_name */
	auto get_name() const
	{
		return str_view(debug_name);
	}

	/** Calculates block's padding size */
	auto get_block_padding_size() const
	{
		return block_size - valid_block_size;
	}

	/** Address accessor for the underlying buffer */
	auto vk() const
	{
		return &allocated_buffer.first;
	}

	/** Trivial accessor for descriptor_info */
	auto get_descriptor_info() const
	{
		return descriptor_info;
	}

	/** Trivial accessor for whole_size */
	auto get_whole_size() const
	{
		return whole_size;
	}

	/** Trivial accessor for block_size */
	auto get_block_size() const
	{
		return block_size;
	}

	/** Trivial accessor for valid_block_size */
	auto get_block_valid_size() const
	{
		return valid_block_size;
	}

	/** Trivial accessor for block_count */
	auto get_block_count() const
	{
		return block_count;
	}

private:
	void calculate_block_size(Gpu const *gpu);

private:
	tidy_ptr<Device const> device = {};
	MemoryAllocator const *memory_allocator = {};

	pair<vk::Buffer, vma::Allocation> allocated_buffer = {};

	vk::DeviceSize whole_size = {};
	vk::DeviceSize block_size = {};
	vk::DeviceSize valid_block_size = {};

	u32 block_count = {};

	vk::DescriptorBufferInfo descriptor_info = {};

	str debug_name = {};
};

} // namespace BINDLESSVK_NAMESPACE
