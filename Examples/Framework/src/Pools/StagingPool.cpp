#include "Framework/Pools/StagingPool.hpp"

StagingPool::StagingPool(
    u32 count,
    size_t size,
    bvk::VkContext const *const vk_context,
    bvk::MemoryAllocator const *const memory_allocator
)
{
	staging_buffers.reserve(count);

	for (u32 i = 0; i < count; ++i)
		staging_buffers.push_back({
		    vk_context,
		    memory_allocator,
		    vk::BufferUsageFlagBits::eTransferSrc,
		    {
		        vma::AllocationCreateFlagBits::eHostAccessRandom,
		        vma::MemoryUsage::eAutoPreferHost,
		    },
		    size,
		    1u,
		    fmt::format("staging_buffer_{}", i),
		});
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
