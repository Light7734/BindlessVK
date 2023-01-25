#include "Framework/Pools/StagingPool.hpp"

StagingPool::StagingPool(uint32_t count, size_t size, bvk::Device* device)
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
	}
}

StagingPool::StagingPool(StagingPool&& other)
{
	*this = std::move(other);
}

StagingPool& StagingPool::operator=(StagingPool&& rhs)
{
	this->staging_buffers = std::move(rhs.staging_buffers);
	return *this;
}

StagingPool::~StagingPool()
{
}

void StagingPool::destroy_buffers()
{
	staging_buffers.clear();
}
