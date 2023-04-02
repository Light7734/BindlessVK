#pragma once

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

// class VertexBuffer
// {
// public:
// 	struct Subregion
// 	{
// 		usize offset;
// 		usize length;
// 	};
//
// public:
// 	VertexBuffer(VkContext const *vk_context, usize size)
// 	{
// 		buffer = new Buffer {
// 			vk_context,
// 			vk::BufferUsageFlagBits::eVertexBuffer,
// 			vma::AllocationCreateInfo {
// 			    {},
// 			    vma::MemoryUsage::eAutoPreferDevice,
// 			},
// 			size,
// 			1u,
// 			fmt::format("vertex_buffer"),
// 		};
//
// 		map = buffer->map_block(0);
// 	}
//
// 	~VertexBuffer()
// 	{
// 	}
//
// 	VertexBuffer(const VertexBuffer &) = delete;
// 	VertexBuffer &operator=(const VertexBuffer &) = delete;
//
// 	void write_buffer(Buffer const &src_buffer, vk::BufferCopy const &copy_info)
// 	{
// 	}
//
// 	auto get_buffer() -> vk::Buffer
// 	{
// 		return *buffer->get_buffer();
// 	}
//
// 	[[nodiscard]] auto grab_region(u32 size) -> Subregion
// 	{
// 		for (auto &region : regions)
// 			if (region.length >= size)
// 			{
// 				region.length -= size; // OR -> region.offset += size;
//
// 				return Subregion {
// 					region.offset + region.length + 1,
// 					size,
// 				};
// 			}
//
// 		assert_fail("full of shit");
// 		return {};
// 	}
//
// 	void return_region(Subregion returned_region)
// 	{
// 		// if returned_region is contiguous any free region then
// 		// simply expand the non-occupied region
// 		for (auto &region : regions)
// 		{
// 			// returned region is contiguous in FRONT
// 			if (region.offset + region.length == (returned_region.offset - 1))
// 			{
// 				region.length += returned_region.length;
// 				return;
// 			}
//
// 			// returned region is contiguous in BACK
// 			else if (region.offset - 1 == (returned_region.offset + returned_region.length))
// 			{
// 				region.offset -= returned_region.length;
// 				return;
// 			}
// 		}
//
// 		// returned region is not contiguous to any free regions
// 		regions.push_back(returned_region);
// 	}
//
// private:
// 	vec<Subregion> regions;
// 	Buffer *buffer;
//
// 	void *map;
// };

} // namespace BINDLESSVK_NAMESPACE
