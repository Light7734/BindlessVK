#include "Framework/Pools/StagingPool.hpp"

StagingPool::StagingPool(u32 count, size_t size, ref<bvk::VkContext const> vk_context)
  : vk_context(vk_context)
{
	staging_buffers.reserve(count);

	for (u32 i = 0; i < count; ++i) {
		staging_buffers.emplace_back(
		  fmt::format("staging_buffer_{}", i).c_str(),
		  vk_context.get(),
		  vk::BufferUsageFlagBits::eTransferSrc,
		  vma::AllocationCreateInfo {
		    vma::AllocationCreateFlagBits::eHostAccessRandom,
		    vma::MemoryUsage::eAutoPreferHost,
		  },
		  size,
		  1u
		);
	}
}

StagingPool::StagingPool(StagingPool &&other)
{
	*this = std::move(other);
}

StagingPool &StagingPool::operator=(StagingPool &&rhs)
{
	this->staging_buffers = std::move(rhs.staging_buffers);
	return *this;
}

StagingPool::~StagingPool()
{
	staging_buffers.clear();
}

void StagingPool::destroy_buffers()
{
	staging_buffers.clear();
}
