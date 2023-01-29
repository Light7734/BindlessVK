#pragma once

#include "BindlessVk/Buffer.hpp"
#include "Framework/Common/Common.hpp"

class StagingPool
{
public:
	StagingPool(u32 count, usize size, bvk::VkContext* vk_context);

	StagingPool() = default;

	StagingPool(StagingPool&& other);
	StagingPool& operator=(StagingPool&& rhs);

	StagingPool(const StagingPool& rhs) = delete;
	StagingPool& operator=(const StagingPool& rhs) = delete;

	~StagingPool();

	void destroy_buffers();

	inline bvk::Buffer* get_by_index(u32 index)
	{
		assert_true(staging_buffers.size() > index);
		return &staging_buffers[index];
	}

private:
	std::vector<bvk::Buffer> staging_buffers = {};
};
