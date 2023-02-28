#include "BindlessVk/Material/MaterialSystem.hpp"

#include <ranges>

namespace BINDLESSVK_NAMESPACE {

Material::Material(
    VkContext *const vk_context,
    ShaderPipeline *const effect,
    vk::DescriptorPool const descriptor_pool
)
    : effect(effect)
{
	auto *const descriptor_allocator = vk_context->get_descriptor_allocator();
	vk::DescriptorSetAllocateInfo allocate_info {
		descriptor_pool,
		1,
		&effect->get_descriptor_set_layouts().back(),
	};

	descriptor_allocator->allocate_descriptor_set(effect->get_descriptor_set_layouts().back());
	const auto device = vk_context->get_device();
	assert_false(device.allocateDescriptorSets(&allocate_info, &descriptor_set));
}


} // namespace BINDLESSVK_NAMESPACE
