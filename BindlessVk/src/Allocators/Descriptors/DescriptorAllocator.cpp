#include "BindlessVk/Allocators/Descriptors/DescriptorAllocator.hpp"

namespace BINDLESSVK_NAMESPACE {

DescriptorAllocator::DescriptorAllocator(VkContext const *vk_context)
    : device(vk_context->get_device())
{
	grab_or_create_new_pool();
}

DescriptorAllocator::DescriptorAllocator(DescriptorAllocator &&other)
{
	*this = std::move(other);
}

DescriptorAllocator &DescriptorAllocator::operator=(DescriptorAllocator &&other)
{
	this->device = other.device;
	this->active_pools = std::move(other.active_pools);
	this->free_pools = std::move(other.free_pools);
	this->pool_counter = other.pool_counter;
	this->current_pool = this->active_pools.end() - 1;

	other.device = {};

	return *this;
}

DescriptorAllocator::~DescriptorAllocator()
{
	if (!device)
		return;

	for (auto &pool : active_pools)
		device->vk().destroyDescriptorPool(pool.vk());
}

pair<vk::DescriptorSet, vk::DescriptorPool> DescriptorAllocator::allocate_descriptor_set(
    vk::DescriptorSetLayout const layout
)
{
	auto descriptor_set = current_pool->try_allocate_descriptor_set(layout);

	if (!descriptor_set)
	{
		try_destroy_pool(current_pool);
		grab_or_create_new_pool();

		descriptor_set = current_pool->try_allocate_descriptor_set(layout);
	}

	assert_true(descriptor_set, "Something went terribly wrong");
	return { descriptor_set, current_pool->vk() };
}

void DescriptorAllocator::release_descriptor_set(vk::DescriptorPool descriptor_pool)
{
	auto const pool = find_pool(descriptor_pool);
	pool->release_descriptor_set();
	try_destroy_pool(pool);
}

void DescriptorAllocator::grab_or_create_new_pool()
{
	if (free_pools.empty())
		create_new_pools();

	grab_pool();
}

void DescriptorAllocator::grab_pool()
{
	active_pools.emplace_back(std::move(free_pools.back()));
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

	free_pools.emplace_back(
	    device,
	    vk::DescriptorPoolCreateInfo {
	        flags,
	        100,
	        pool_sizes,
	    }
	);

	free_pools.emplace_back(
	    device,
	    vk::DescriptorPoolCreateInfo {
	        flags,
	        100,
	        pool_sizes,
	    }
	);
}

void DescriptorAllocator::try_destroy_pool(vec_it<DescriptorPool> pool)
{
	if (!pool->is_eligable_for_destruction())
		return;

	active_pools.erase(pool);
	current_pool = active_pools.end() - 1;
}

auto DescriptorAllocator::find_pool(vk::DescriptorPool const pool) -> vec_it<DescriptorPool>
{
	return std::find(active_pools.begin(), active_pools.end(), pool);
}

} // namespace BINDLESSVK_NAMESPACE
