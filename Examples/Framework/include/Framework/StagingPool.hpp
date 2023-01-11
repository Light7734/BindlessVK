#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/Types.hpp"
#include "Framework/Core/Common.hpp"
#include "Framework/Scene/Components.hpp"
#include "Framework/Scene/Scene.hpp"

class StagingPool
{
public:
	StagingPool(uint32_t count, size_t size, bvk::Device* device)
	{
		staging_buffers.reserve(count);

		for (uint32_t i = 0; i < count; ++i) {
			staging_buffers.emplace_back(
			  fmt::format("staging_buffer_{}", i).c_str(),
			  device,
			  vk::BufferUsageFlagBits::eTransferSrc,
			  vma::AllocationCreateInfo {
			    vma::AllocationCreateFlagBits::eHostAccessRandom,
			    vma::MemoryUsage::eAutoPreferHost,
			  },
			  size,
			  1u
			);
            // 8 + 4 + 2
		}
	}

	StagingPool()  = default;
	~StagingPool() = default;

	void destroy_buffers()
	{
		staging_buffers.clear();
	}

	bvk::Buffer* get_by_index(uint32_t index)
	{
		ASSERT(staging_buffers.size() > index);
		return &staging_buffers[index];
	}

private:
	std::vector<bvk::Buffer> staging_buffers = {};
};
