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

	inline operator bool() const
	{
		return descriptor_pool && descriptor_set;
	}
};

/**
 * @todo Make this more efficient
 */
class DescriptorAllocator
{
private:
	struct Pool
	{
		Pool(vk::DescriptorPool const pool): pool_object(pool), set_count(0), out_of_memory(false)
		{
		}

		vk::DescriptorPool pool_object;
		u32 set_count;
		bool out_of_memory;

		inline auto is_eligable_for_destruction() const -> bool
		{
			return set_count == 0 && out_of_memory;
		}

		inline operator vk::DescriptorPool() const
		{
			return pool_object;
		}

		inline auto operator==(Pool const other) const -> bool
		{
			return this->pool_object == other.pool_object;
		}

		inline auto operator==(vk::DescriptorPool const other) const -> bool
		{
			return this->pool_object == other;
		}
	};

public:
	void init(vk::Device device);
	void destroy();

	auto allocate_descriptor_set(vk::DescriptorSetLayout layout) -> AllocatedDescriptorSet;
	void release_descriptor_set(AllocatedDescriptorSet const &set);

private:
	auto try_allocate_descriptor_set(vk::DescriptorSetAllocateInfo alloc_info)
	    -> AllocatedDescriptorSet;

	void expire_current_pool();

	void grab_or_create_new_pool();
	void grab_pool();

	void create_new_pools();

	void destroy_pool(vec<Pool>::iterator pool_it);

	auto find_pool(vk::DescriptorPool pool) -> vec<Pool>::iterator;


private:
	vk::Device device = {};

	vec<Pool> active_pools = {};
	vec<Pool> free_pools = {};

	vec<Pool>::iterator current_pool = {};

	u32 pool_counter = {};
};

} // namespace BINDLESSVK_NAMESPACE
