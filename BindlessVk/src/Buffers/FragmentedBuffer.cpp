#include "BindlessVk/Buffers/FragmentedBuffer.hpp"

namespace BINDLESSVK_NAMESPACE {

FragmentedBuffer::FragmentedBuffer(
    VkContext const *vk_context,
    MemoryAllocator const *const memory_allocator,
    Type type,
    usize size,
    str_view debug_name /** = default_debug_name */
)
    : debug_utils(vk_context->get_debug_utils())
    , fragments({ { 0, size } })
    , buffer(

          vk_context,
          memory_allocator,

          vk::BufferUsageFlagBits::eTransferDst |
              (type == Type::eVertex ? vk::BufferUsageFlagBits::eVertexBuffer :
                                       vk::BufferUsageFlagBits::eIndexBuffer),

          vma::AllocationCreateInfo {
              vma::AllocationCreateFlagBits::eHostAccessRandom,
              vma::MemoryUsage::eAutoPreferDevice,
          },

          size,
          1u,
          debug_name
      )
    , map(buffer.map_block(0))
    , type(type)
{
}

FragmentedBuffer::FragmentedBuffer(FragmentedBuffer &&other)
{
	*this = std::move(other);
}

FragmentedBuffer &FragmentedBuffer::operator=(FragmentedBuffer &&other)
{
	this->device = other.device;
	this->fragments = other.fragments;
	this->buffer = std::move(other.buffer);
	this->map = other.map;
	this->debug_utils = other.debug_utils;

	other.buffer = {};

	return *this;
}

FragmentedBuffer::~FragmentedBuffer()
{
}

void FragmentedBuffer::copy_staging_to_subregion(Buffer *staging_buffer, Fragment subregion)
{
	buffer.write_buffer(
	    *staging_buffer,
	    vk::BufferCopy {
	        0,
	        subregion.offset,
	        subregion.length,
	    }
	);
}

void FragmentedBuffer::bind(vk::CommandBuffer cmd, u32 binding /** = 0 */) const
{
	auto static offset = vk::DeviceSize {};

	switch (type)
	{
	case Type::eVertex:
	{
		cmd.bindVertexBuffers(binding, 1, buffer.vk(), &offset);
		break;
	}
	case Type::eIndex:
	{
		cmd.bindIndexBuffer(*buffer.vk(), 0u, vk::IndexType::eUint32);
		break;
	}
	default: assert_fail("Invalid fragmented buffer ({}) type: {} ", buffer.get_name(), type);
	}
}

[[nodiscard]] auto FragmentedBuffer::grab_fragment(u32 size) -> Fragment
{
	for (auto &region : fragments)
		if (region.length >= size)
		{
			region.offset += size;
			return Fragment {
				region.offset - size,
				size,
			};
		}

	assert_fail(
	    "Failed to grab a fragment of size {}bytes out of fragmented buffer {}",
	    size,
	    buffer.get_name()
	);

	return {};
}

void FragmentedBuffer::return_fragment(Fragment returned_fragment)
{
	// if returned_fragment is contiguous to any free region then
	// simply expand the non-occupied region
	for (auto &fragment : fragments)
	{
		// returned_fragment is contiguous in FRONT
		if (fragment.offset + fragment.length == (returned_fragment.offset - 1))
		{
			fragment.length += returned_fragment.length;
			return;
		}

		// returned_fragment is contiguous in BACK
		else if (fragment.offset - 1 == (returned_fragment.offset + returned_fragment.length))
		{
			fragment.offset -= returned_fragment.length;
			return;
		}
	}

	// returned_fragment is not contiguous to any free regions
	fragments.push_back(returned_fragment);
}

} // namespace BINDLESSVK_NAMESPACE
