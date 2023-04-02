#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

struct AllocatedDescriptorSet
{
	/** Trivial accessor for the underlying descriptor set */
	auto vk() const
	{
		return descriptor_set;
	}

	/** Trivial accessor for descriptor pool */
	auto get_pool() const
	{
		return descriptor_pool;
	}

	/** Validity :Lwqa*/
	operator bool() const
	{
		return descriptor_pool && descriptor_set;
	}

private:
	vk::DescriptorSet descriptor_set;
	vk::DescriptorPool descriptor_pool;
};

class DescriptorAllocator
{
private:
	class Pool;

public:
	/** Default constructor */
	DescriptorAllocator() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 */
	DescriptorAllocator(VkContext const *vk_context);

	/** Move constructor */
	DescriptorAllocator(DescriptorAllocator &&other);

	/** Move assignment operator */
	DescriptorAllocator &operator=(DescriptorAllocator &&other);

	/** Deleted copy constructor */
	DescriptorAllocator(DescriptorAllocator const &) = delete;

	/** Deleted copy assignment operator */
	DescriptorAllocator &operator=(DescriptorAllocator const &other) = delete;

	/** Destructor */
	~DescriptorAllocator();

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
	struct Pool
	{
		Pool(vk::DescriptorPool const pool): pool_object(pool), set_count(0), out_of_memory(false)
		{
		}

		vk::DescriptorPool pool_object;
		u32 set_count;
		bool out_of_memory;

		auto is_eligable_for_destruction() const -> bool
		{
			return set_count == 0 && out_of_memory;
		}

		auto operator==(Pool const other) const -> bool
		{
			return this->pool_object == other.pool_object;
		}

		auto operator==(vk::DescriptorPool const other) const -> bool
		{
			return this->pool_object == other;
		}

		operator vk::DescriptorPool() const
		{
			return pool_object;
		}
	};

private:
	Device *device = {};

	vec<Pool> active_pools = {};
	vec<Pool> free_pools = {};

	vec<Pool>::iterator current_pool = {};

	u32 pool_counter = {};
};

} // namespace BINDLESSVK_NAMESPACE
