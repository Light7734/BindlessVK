#include "BindlessVk/MaterialSystem.hpp"

#include <ranges>

namespace BINDLESSVK_NAMESPACE {

ShaderEffect::ShaderEffect(
    VkContext *vk_context,
    vec<Shader *> shaders,
    ShaderEffect::Configuration configuration
)
    : vk_context(vk_context)
{
	create_descriptor_sets_layout(shaders);

	const auto device = vk_context->get_device();
	const auto surface = vk_context->get_surface();
	const auto pipeline_shader_stage_create_infos = create_pipeline_shader_stage_infos(shaders);

	pipeline_layout = device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
	    {},
	    static_cast<u32>(descriptor_sets_layout.size()),
	    descriptor_sets_layout.data(),
	});

	const auto pipeline_rendering_info = vk::PipelineRenderingCreateInfo {
		{},
		1u,
		&surface.color_format,
		vk_context->get_depth_format(), //
		{},
	};

	pipeline = create_graphics_pipeline(
	    pipeline_shader_stage_create_infos,
	    pipeline_rendering_info,
	    configuration
	);
}

ShaderEffect::ShaderEffect(ShaderEffect &&effect)
{
	*this = std::move(effect);
}

ShaderEffect &ShaderEffect::operator=(ShaderEffect &&effect)
{
	this->vk_context = effect.vk_context;
	this->pipeline = effect.pipeline;
	this->pipeline_layout = effect.pipeline_layout;
	this->descriptor_sets_layout = effect.descriptor_sets_layout;

	effect.vk_context = {};
	effect.pipeline = VK_NULL_HANDLE;
	effect.pipeline_layout = VK_NULL_HANDLE;
	effect.descriptor_sets_layout = {};

	return *this;
}

ShaderEffect::~ShaderEffect()
{
	if (vk_context)
	{
		const auto device = vk_context->get_device();

		device.destroyPipeline(pipeline);
		device.destroyPipelineLayout(pipeline_layout);
		device.destroyDescriptorSetLayout(descriptor_sets_layout[0]);
		device.destroyDescriptorSetLayout(descriptor_sets_layout[1]);
	}
}

void ShaderEffect::create_descriptor_sets_layout(vec<Shader *> shaders)
{
	const auto device = vk_context->get_device();
	const auto sets_bindings = combine_descriptor_sets_bindings(shaders);

	for (u32 i = 0u; const auto &set_bindings : sets_bindings)
	{
		descriptor_sets_layout[i++] =
		    device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {
		        {},
		        static_cast<u32>(set_bindings.size()),
		        set_bindings.data(),
		    });
	}
}

arr<vec<vk::DescriptorSetLayoutBinding>, 2> ShaderEffect::combine_descriptor_sets_bindings(
    vec<Shader *> shaders
)
{
	auto combined_bindings = arr<vec<vk::DescriptorSetLayoutBinding>, 2> {};

	for (const auto &shader : shaders)
	{
		for (u32 i = 0; const auto &descriptor_set_bindings : shader->descriptor_sets_bindings)
		{
			for (const auto &descriptor_set_binding : descriptor_set_bindings)
			{
				const u32 set_index = i;
				const u32 binding_index = descriptor_set_binding.binding;

				if (combined_bindings[i].size() <= binding_index)
					combined_bindings[i].resize(binding_index + 1);

				combined_bindings[i][binding_index] = descriptor_set_binding;
			}

			i++;
		}
	}

	return combined_bindings;
}

vec<vk::PipelineShaderStageCreateInfo> ShaderEffect::create_pipeline_shader_stage_infos(
    vec<Shader *> shaders
)
{
	auto pipeline_shader_stage_infos = vec<vk::PipelineShaderStageCreateInfo> {};
	pipeline_shader_stage_infos.resize(shaders.size());

	for (u32 i = 0; const auto &shader : shaders)
	{
		pipeline_shader_stage_infos[i++] = {
			{},
			shader->stage,
			shader->module,
			"main",
		};
	}

	return pipeline_shader_stage_infos;
}

vk::Pipeline ShaderEffect::create_graphics_pipeline(
    vec<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos,
    vk::PipelineRenderingCreateInfoKHR rendering_info,
    ShaderEffect::Configuration configuration
)
{
	const auto device = vk_context->get_device();

	const auto color_blend_state = vk::PipelineColorBlendStateCreateInfo {
		{},
		{},
		{},
		static_cast<u32>(configuration.color_blend_attachments.size()),
		configuration.color_blend_attachments.data()
	};

	const auto dynamic_state = vk::PipelineDynamicStateCreateInfo {
		{},
		static_cast<u32>(configuration.dynamic_states.size()),
		configuration.dynamic_states.data(),
	};

	const auto graphics_pipeline_info = vk::GraphicsPipelineCreateInfo {
		{},
		static_cast<u32>(shader_stage_create_infos.size()),
		shader_stage_create_infos.data(),
		&configuration.vertex_input_state,
		&configuration.input_assembly_state,
		&configuration.tesselation_state,
		&configuration.viewport_state,
		&configuration.rasterization_state,
		&configuration.multisample_state,
		&configuration.depth_stencil_state,
		&color_blend_state,
		&dynamic_state,
		pipeline_layout,
		{},
		{},
		{},
		{},
		&rendering_info,
	};

	const auto pipeline = device.createGraphicsPipeline({}, graphics_pipeline_info);
	assert_false(pipeline.result);

	return pipeline.value;
}

} // namespace BINDLESSVK_NAMESPACE
