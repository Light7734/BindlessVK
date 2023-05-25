#pragma once

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

/** A (big) buffer to be used fragment by fragment meant to be used bindlessly */
class FragmentedBuffer
{
public:
	struct Fragment
	{
		usize offset;
		usize length;
	};

	enum Type
	{
		eVertex,
		eIndex,
	};

public:
	/** Default constructor */
	FragmentedBuffer() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 * @param memory_allocator The memory allocator
	 * @param type The type of the buffer, ie. Vertex/Index
	 * @param size The maximum size of the buffer in bytes
	 * @param debug_name Name of the buffer, a null-terminated str view
	 */
	FragmentedBuffer(
	    VkContext const *vk_context,
	    MemoryAllocator const *memory_allocator,
	    Type type,
	    usize size,
	    str_view debug_name = default_debug_name
	);

	/** Default move constructor */
	FragmentedBuffer(FragmentedBuffer &&other) = default;

	/** Default move assignment operator */
	FragmentedBuffer &operator=(FragmentedBuffer &&other) = default;

	/** Deleted copy constructor */
	FragmentedBuffer(const FragmentedBuffer &) = delete;

	/** Deleted copy assignment operator */
	FragmentedBuffer &operator=(const FragmentedBuffer &) = delete;

	/** Default destructor */
	~FragmentedBuffer() = default;

	void copy_staging_to_fragment(Buffer *staging_buffer, Fragment fragment);

	void bind(vk::CommandBuffer cmd, u32 binding = 0) const;

	void write_buffer(Buffer const &src_buffer, vk::BufferCopy const &copy_info);

	auto get_buffer() -> vk::Buffer;

	/** Grabs a fragment from the buffer
	 *
	 * @param size The  size of the grabbed fragment
	 * @returns A buffer fragment
	 */
	[[nodiscard]] //
	auto grab_fragment(u32 size) -> Fragment;

	void return_fragment(Fragment returned_fragment);

private:
	DebugUtils *debug_utils = {};

	Type type = {};

	vec<Fragment> fragments = {};

	Buffer buffer = {};

	void *map = {};
};

} // namespace BINDLESSVK_NAMESPACE
