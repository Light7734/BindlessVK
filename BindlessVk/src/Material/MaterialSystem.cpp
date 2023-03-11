#include "BindlessVk/Material/MaterialSystem.hpp"

#include <ranges>

namespace BINDLESSVK_NAMESPACE {

Material::Material(
    VkContext *const vk_context,
    ShaderPipeline *const effect,
    vk::DescriptorPool const descriptor_pool
)
    : vk_context(vk_context)
    , effect(effect)
{
	auto *const descriptor_allocator = vk_context->get_descriptor_allocator();

	if (effect->uses_shader_descriptor_set_slot())
	{
		auto const descriptor_set_layout = effect->get_descriptor_set_layout();
		descriptor_set = descriptor_allocator->allocate_descriptor_set(descriptor_set_layout);
	}
}

Material::~Material()
{
	auto *const descriptor_allocator = vk_context->get_descriptor_allocator();
	descriptor_allocator->release_descriptor_set(descriptor_set);
}


} // namespace BINDLESSVK_NAMESPACE
