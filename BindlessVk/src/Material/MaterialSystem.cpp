#include "BindlessVk/Material/MaterialSystem.hpp"

#include <ranges>

namespace BINDLESSVK_NAMESPACE {

Material::Material(
    DescriptorAllocator *const descriptor_allocator,
    ShaderPipeline *const shader_pipeline,
    vk::DescriptorPool const descriptor_pool
)
    : shader_pipeline(shader_pipeline)
{
	if (shader_pipeline->uses_shader_descriptor_set_slot())
	{
		auto const descriptor_set_layout = shader_pipeline->get_descriptor_set_layout();
		descriptor_set = DescriptorSet(descriptor_allocator, descriptor_set_layout.vk());
	}
}

} // namespace BINDLESSVK_NAMESPACE
