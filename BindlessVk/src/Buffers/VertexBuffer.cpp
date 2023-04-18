#include "BindlessVk/Buffers/VertexBuffer.hpp"

namespace BINDLESSVK_NAMESPACE {

VertexBuffer::VertexBuffer(
    VkContext const *vk_context,
    MemoryAllocator const *const memory_allocator,
    usize size
)
    : debug_utils(vk_context->get_debug_utils())
{
	buffer = new Buffer {
		vk_context,
		memory_allocator,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vma::AllocationCreateInfo {
		    vma::AllocationCreateFlagBits::eHostAccessRandom,
		    vma::MemoryUsage::eAutoPreferDevice,
		},
		size,
		1u,
		fmt::format("vertex_buffer"),
	};

	regions.push_back(Subregion { 0, size });

	map = buffer->map_block(0);
}

VertexBuffer::VertexBuffer(VertexBuffer &&other)
{
	*this = std::move(other);
}

VertexBuffer &VertexBuffer::operator=(VertexBuffer &&other)
{
	this->device = other.device;
	this->regions = other.regions;
	this->buffer = other.buffer;
	this->map = other.map;
	this->debug_utils = other.debug_utils;

	other.buffer = {};

	return *this;
}

VertexBuffer::~VertexBuffer()
{
	delete buffer;
}

void VertexBuffer::copy_staging_to_subregion(Buffer *staging_buffer, Subregion subregion)
{
	buffer->write_buffer(
	    *staging_buffer,
	    vk::BufferCopy {
	        0,
	        subregion.offset,
	        subregion.length,
	    }
	);
}

void VertexBuffer::bind(vk::CommandBuffer cmd, u32 binding) const
{
	auto static offset = vk::DeviceSize {};
	cmd.bindVertexBuffers(binding, 1, buffer->vk(), &offset);
}

[[nodiscard]] auto VertexBuffer::grab_region(u32 size) -> Subregion
{
	for (auto &region : regions)
		if (region.length >= size)
		{
			region.offset += size; // OR -> region.length -= size;
			debug_utils->log(
			    LogLvl::eTrace,
			    "Grabbed vertex buffer region: [off: {}, len: {}]",
			    region.offset - size,
			    size
			);

			return Subregion {
				region.offset - size,
				size,
			};
		}

	assert_fail("full of shit");
	return {};
}

void VertexBuffer::return_region(Subregion returned_region)
{
	// if returned_region is contiguous to any free region then
	// simply expand the non-occupied region
	for (auto &region : regions)
	{
		// returned region is contiguous in FRONT
		if (region.offset + region.length == (returned_region.offset - 1))
		{
			region.length += returned_region.length;
			return;
		}

		// returned region is contiguous in BACK
		else if (region.offset - 1 == (returned_region.offset + returned_region.length))
		{
			region.offset -= returned_region.length;
			return;
		}
	}

	// returned region is not contiguous to any free regions
	regions.push_back(returned_region);
}

} // namespace BINDLESSVK_NAMESPACE
