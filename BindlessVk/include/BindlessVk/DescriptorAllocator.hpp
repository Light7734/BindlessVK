#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

struct AllocatedDescriptorSet
{
	vk::DescriptorSet descriptor_set;
	vk::DescriptorPool descriptor_pool;

	inline operator vk::DescriptorSet() const
	{
		return descriptor_set;
	}

	inline operator VkDescriptorSet() const
	{
		return static_cast<VkDescriptorSet>(descriptor_set);
	}
};

class DescriptorAllocator
{
public:
	void init(vk::Device device)
	{
		this->device = device;
		current_pool = grab_or_create_pools();
	}

	void destroy()
	{
		for (auto const &[pool, set_count] : active_pools)
			device.destroyDescriptorPool(pool);

		for (auto const pool : free_pools)
			device.destroyDescriptorPool(pool);
	}

	AllocatedDescriptorSet allocate_descriptor_set(vk::DescriptorSetLayout layout)
	{
		auto const alloc_info = vk::DescriptorSetAllocateInfo { current_pool, 1u, &layout };
		auto set = vk::DescriptorSet {};

		auto result = device.allocateDescriptorSets(&alloc_info, &set);

		switch (result)
		{
		case vk::Result::eSuccess:
			std::cout << "set: " << (u64)(VkDescriptorSet)(set) << std::endl;
			return { set, current_pool };

		case vk::Result::eErrorOutOfPoolMemory:
		case vk::Result::eErrorFragmentedPool:
		{
			break;
		}

		default: assert_fail("Something went terribly wrong");
		}

		// destroy currently active pool if it doesn't hold anything
		if (active_pools.back().second == 0)
		{
			device.destroyDescriptorPool(active_pools.back().first);
			active_pools.pop_back();
		}

		current_pool = grab_or_create_pools();
		result = device.allocateDescriptorSets(&alloc_info, &set);

		if (result != vk::Result::eSuccess)
			assert_fail("Something went terribly wrong");

		std::cout << "set: " << (u64)(VkDescriptorSet)(set) << std::endl;
		return { set, current_pool };
	}

	void release_descriptor_set(AllocatedDescriptorSet const &set)
	{
		for (auto &[pool, set_count] : active_pools)
		{
			if (set.descriptor_pool == pool)
			{
				set_count--;

				if (set_count == 0 && pool != current_pool)
					device.destroyDescriptorPool(pool);
			}
		}
	}

private:
	auto grab_or_create_pools() -> vk::DescriptorPool
	{
		if (free_pools.empty())
			create_new_pools();

		auto const pool = free_pools.back();
		free_pools.pop_back();
		return pool;
	}

	void create_new_pools()
	{
		auto const pool_sizes = vec<vk::DescriptorPoolSize> {
			{ vk::DescriptorType::eSampler, 8000 },
			{ vk::DescriptorType::eCombinedImageSampler, 8000 },
			{ vk::DescriptorType::eSampledImage, 8000 },
			{ vk::DescriptorType::eStorageImage, 8000 },
			{ vk::DescriptorType::eUniformTexelBuffer, 8000 },
			{ vk::DescriptorType::eStorageTexelBuffer, 8000 },
			{ vk::DescriptorType::eUniformBuffer, 8000 },
			{ vk::DescriptorType::eStorageBuffer, 8000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 8000 },
			{ vk::DescriptorType::eStorageBufferDynamic, 8000 },
			{ vk::DescriptorType::eInputAttachment, 8000 },
		};

		free_pools.push_back(device.createDescriptorPool({ {}, 100, pool_sizes }));
		free_pools.push_back(device.createDescriptorPool({ {}, 100, pool_sizes }));
		free_pools.push_back(device.createDescriptorPool({ {}, 100, pool_sizes }));
	}

private:
	vk::Device device = {};

	vec<pair<vk::DescriptorPool, u32>> active_pools = {};
	vec<vk::DescriptorPool> free_pools = {};

	vk::DescriptorPool current_pool = {};
};

} // namespace BINDLESSVK_NAMESPACE
