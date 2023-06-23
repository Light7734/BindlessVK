#include "BindlessVk/Shader/DescriptorSet.hpp"



namespace BINDLESSVK_NAMESPACE {

DescriptorSet::DescriptorSet(
    DescriptorAllocator *const descriptor_allocator,
    vk::DescriptorSetLayout const descriptor_set_layout
)
    : descriptor_allocator(descriptor_allocator)
    , allocated_descriptor_set(descriptor_allocator->allocate_descriptor_set(descriptor_set_layout))
{
	ZoneScoped;
}

DescriptorSet::~DescriptorSet()
{
	ZoneScoped;

	if (!descriptor_allocator)
		return;

	descriptor_allocator->release_descriptor_set(allocated_descriptor_set.second);
}

} // namespace BINDLESSVK_NAMESPACE
