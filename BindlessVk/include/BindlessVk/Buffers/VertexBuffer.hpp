#pragma once

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Represents a vertex buffer to be used bindlessly */
class VertexBuffer
{
public:
	struct Subregion
	{
		usize offset;
		usize length;
	};

public:
	/** Default constructor */
	VertexBuffer() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 * @param size The entire size of the vertex buffer in bytes
	 */
	VertexBuffer(VkContext const *vk_context, MemoryAllocator const *memory_allocator, usize size);

	/** Move constructor */
	VertexBuffer(VertexBuffer &&other);

	/** Move assignment operator */
	VertexBuffer &operator=(VertexBuffer &&other);

	/** Deleted copy constructor */
	VertexBuffer(const VertexBuffer &) = delete;

	/** Deleted copy assignment operator */
	VertexBuffer &operator=(const VertexBuffer &) = delete;

	/** Destructor */
	~VertexBuffer();

	void copy_staging_to_subregion(Buffer *staging_buffer, Subregion subregion);

	void bind(vk::CommandBuffer cmd, u32 binding) const;

	void write_buffer(Buffer const &src_buffer, vk::BufferCopy const &copy_info);

	auto get_buffer() -> vk::Buffer;

	[[nodiscard]] //
	auto grab_region(u32 size) -> Subregion;

	void return_region(Subregion returned_region);

private:
	Device *device = {};
    DebugUtils* debug_utils = {};

	vec<Subregion> regions = {};

	Buffer *buffer = {};

	void *map = {};
};

} // namespace BINDLESSVK_NAMESPACE
