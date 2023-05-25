#include "BindlessVk/Allocators/Descriptors/DescriptorPool.hpp"

namespace BINDLESSVK_NAMESPACE {

DescriptorPool::DescriptorPool(Device const *device, vk::DescriptorPoolCreateInfo info)
    : device(device)
{
	descriptor_pool = device->vk().createDescriptorPool(info);
}

auto DescriptorPool::is_eligable_for_destruction() const -> bool
{
	return descriptor_set_count == 0 && out_of_memory;
}

auto DescriptorPool::operator==(DescriptorPool const &other) const -> bool
{
	return this->descriptor_pool == other.descriptor_pool;
}

auto DescriptorPool::operator==(vk::DescriptorPool const other) const -> bool
{
	return this->descriptor_pool == other;
}

auto DescriptorPool::try_allocate_descriptor_set(vk::DescriptorSetLayout layout)
    -> vk::DescriptorSet
{
	auto const alloc_info = vk::DescriptorSetAllocateInfo {
		descriptor_pool,
		layout,
	};

	auto descriptor_set = vk::DescriptorSet {};
	auto const result = device->vk().allocateDescriptorSets(&alloc_info, &descriptor_set);

	if (result != vk::Result::eSuccess)
	{
		out_of_memory = true;
		return vk::DescriptorSet {};
	}

	++descriptor_set_count;
	return descriptor_set;
}

void DescriptorPool::release_descriptor_set()
{
	--descriptor_set_count;
}


} // namespace BINDLESSVK_NAMESPACE
