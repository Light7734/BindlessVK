#pragma once

#include "BindlessVk/Buffer.hpp"
#include "Framework/Common/Common.hpp"

class StagingPool
{
public:
	StagingPool() = default;

	StagingPool(uint32_t count, size_t size, bvk::Device* device);

	StagingPool(StagingPool&& other);
	StagingPool(const StagingPool& rhs) = delete;

	StagingPool& operator=(StagingPool&& rhs);
	StagingPool& operator=(const StagingPool& rhs) = delete;

	~StagingPool();

	void destroy_buffers();

	inline bvk::Buffer* get_by_index(uint32_t index)
	{
		assert_true(staging_buffers.size() > index);
		return &staging_buffers[index];
	}

private:
	void move(StagingPool&& other);

	std::vector<bvk::Buffer> staging_buffers = {};
};
