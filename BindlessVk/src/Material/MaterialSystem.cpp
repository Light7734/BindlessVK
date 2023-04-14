#include "BindlessVk/Material/MaterialSystem.hpp"

#include <ranges>

namespace BINDLESSVK_NAMESPACE {

Material::Material(
    DescriptorAllocator *const descriptor_allocator,
    ShaderPipeline *const shader_pipeline,
    vk::DescriptorPool const descriptor_pool
)
    : descriptor_allocator(descriptor_allocator)
    , shader_pipeline(shader_pipeline)
{
	if (shader_pipeline->uses_shader_descriptor_set_slot())
	{
		auto const descriptor_set_layout = shader_pipeline->get_descriptor_set_layout();
		descriptor_set = DescriptorSet(descriptor_allocator, descriptor_set_layout.vk());
	}
}

Material::Material(Material &&other)
{
	*this = std::move(other);
}

Material &Material::operator=(Material &&other)
{
	this->descriptor_allocator = other.descriptor_allocator;
	this->shader_pipeline = other.shader_pipeline;
	this->parameters = other.parameters;
	this->descriptor_set = std::move(other.descriptor_set);

	other.descriptor_allocator = {};

	return *this;
}

Material::~Material()
{
}


} // namespace BINDLESSVK_NAMESPACE
