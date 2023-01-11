#include "Framework/Pools/StagingPool.hpp"

#include "Framework/Core/Common.hpp"

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
	this->move(std::move(other));
}

StagingPool& StagingPool::operator=(StagingPool&& rhs)
{
	this->move(std::move(rhs));
	return *this;
}

StagingPool::~StagingPool()
{
}


void StagingPool::move(StagingPool&& other)
{
	this->staging_buffers = std::move(other.staging_buffers);
}

void StagingPool::destroy_buffers()
{
	staging_buffers.clear();
}
