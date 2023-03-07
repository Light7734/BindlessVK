#include "BindlessVk/Allocators/DescriptorAllocator.hpp"

namespace BINDLESSVK_NAMESPACE {

void DescriptorAllocator::init(vk::Device const device)
{
	this->device = device;
	grab_or_create_new_pool();
}

void DescriptorAllocator::destroy()
{
	for (auto pool : active_pools)
		device.destroyDescriptorPool(pool);
}

AllocatedDescriptorSet DescriptorAllocator::allocate_descriptor_set(
    vk::DescriptorSetLayout const layout
)
{
	auto set = try_allocate_descriptor_set(vk::DescriptorSetAllocateInfo {
	    *current_pool,
	    1u,
	    &layout,
	});

	if (!set)
	{
		expire_current_pool();
		grab_or_create_new_pool();

		set = try_allocate_descriptor_set({ *current_pool, 1u, &layout });
		assert_true(set, "Something went terribly wrong");
	}

	current_pool->set_count++;
	return { set, *current_pool };
}

void DescriptorAllocator::release_descriptor_set(AllocatedDescriptorSet const &set)
{
	auto const pool = find_pool(set.descriptor_pool);
	pool->set_count--;

	if (pool->is_eligable_for_destruction())
		destroy_pool(pool);
}

auto DescriptorAllocator::try_allocate_descriptor_set(vk::DescriptorSetAllocateInfo const alloc_info
) -> AllocatedDescriptorSet
{
	auto set = vk::DescriptorSet {};
	auto const result = device.allocateDescriptorSets(&alloc_info, &set);

	return result == vk::Result::eSuccess ? AllocatedDescriptorSet { set, *current_pool } :
	                                        AllocatedDescriptorSet {};
}

void DescriptorAllocator::expire_current_pool()
{
	current_pool->out_of_memory = true;
	if (current_pool->is_eligable_for_destruction())
		destroy_pool(current_pool);
}

void DescriptorAllocator::grab_or_create_new_pool()
{
	if (free_pools.empty())
		create_new_pools();

	grab_pool();
}

void DescriptorAllocator::grab_pool()
{
	active_pools.push_back(free_pools.back());
	current_pool = active_pools.end() - 1;

	free_pools.pop_back();
}

void DescriptorAllocator::create_new_pools()
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

	auto const flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;

	free_pools.emplace_back(device.createDescriptorPool({ flags, 100, pool_sizes }));
	free_pools.emplace_back(device.createDescriptorPool({ flags, 100, pool_sizes }));
	free_pools.emplace_back(device.createDescriptorPool({ flags, 100, pool_sizes }));
}

void DescriptorAllocator::destroy_pool(vec_it<Pool> pool)
{
	device.destroyDescriptorPool(*pool);
	active_pools.erase(pool);
	current_pool = active_pools.end() - 1;
}

auto DescriptorAllocator::find_pool(vk::DescriptorPool const pool) -> vec_it<Pool>
{
	return std::find(active_pools.begin(), active_pools.end(), pool);
}

} // namespace BINDLESSVK_NAMESPACE
