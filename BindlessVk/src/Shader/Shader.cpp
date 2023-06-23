#include "BindlessVk/Shader/Shader.hpp"



namespace BINDLESSVK_NAMESPACE {
ShaderPipeline::ShaderPipeline(
    VkContext const *const vk_context,
    LayoutAllocator *const layout_allocator,
    Type const type,
    vec<Shader *> const &shaders,
    ShaderPipeline::Configuration const &configuration,
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout,
    str_view const debug_name /* = default_debug_name */
)
    : device(vk_context->get_device())
    , surface(vk_context->get_surface())
    , layout_allocator(layout_allocator)
    , debug_name(debug_name)
{
	ZoneScoped;

	assert_false(shaders.empty(), "No shaders provided to shader pipeline {}", debug_name);

	create_descriptor_set_layout(shaders);
	create_pipeline_layout(graph_descriptor_set_layout, pass_descriptor_set_layout);

	if (type == Type::eCompute)
		create_compute_pipeline(shaders);

	else if (type == Type::eGraphics)
		create_graphics_pipeline(shaders, configuration);

	else
		assert_fail("Invalid shader pipeline type {}: {}", debug_name, (int)type);
}

ShaderPipeline::~ShaderPipeline()
{
	ZoneScoped;

	if (!device)
		return;

	device->vk().destroyPipeline(pipeline);
}

void ShaderPipeline::create_descriptor_set_layout(vec<Shader *> const &shaders)
{
	ZoneScoped;

	auto const shader_set_bindings = combine_descriptor_sets_bindings(shaders);

	if (shader_set_bindings.empty())
		return;

	descriptor_set_layout = layout_allocator->goc_descriptor_set_layout(
	    {},
	    shader_set_bindings,
	    vec<vk::DescriptorBindingFlags>(
	        shader_set_bindings.size(),
	        vk::DescriptorBindingFlagBits::ePartiallyBound
	    )
	);
	device->set_object_name(descriptor_set_layout.vk(), "{}_descriptor_set_layout", debug_name);
}

void ShaderPipeline::create_pipeline_layout(
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout
)
{
	ZoneScoped;

	pipeline_layout = layout_allocator->goc_pipeline_layout(
	    {},
	    graph_descriptor_set_layout, // set = 0 -> per graph(frame)
	    pass_descriptor_set_layout,  // set = 1 -> per pass
	    this->descriptor_set_layout  // set = 2 -> per shader
	);
	device->set_object_name(pipeline_layout, "{}_pipeline_layout", debug_name);
}

void ShaderPipeline::create_graphics_pipeline(
    vec<Shader *> const &shaders,
    ShaderPipeline::Configuration const configuration
)
{
	ZoneScoped;

	auto const surface_color_format = surface->get_color_format();

	auto const pipeline_rendering_info = vk::PipelineRenderingCreateInfo {
		{},
		surface_color_format,
		surface->get_depth_format(),
		{},
	};

	auto const shader_stage_create_infos = create_shader_stage_create_infos(shaders);

	auto const color_blend_state = vk::PipelineColorBlendStateCreateInfo {
		{},
		{},
		{},
		static_cast<u32>(configuration.color_blend_attachments.size()),
		configuration.color_blend_attachments.data()
	};

	auto const dynamic_state = vk::PipelineDynamicStateCreateInfo {
		{},
		static_cast<u32>(configuration.dynamic_states.size()),
		configuration.dynamic_states.data(),
	};

	auto const graphics_pipeline_info = vk::GraphicsPipelineCreateInfo {
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
		&pipeline_rendering_info,
	};

	auto const [result, graphics_pipeline] = device->vk().createGraphicsPipeline(
	    {},
	    graphics_pipeline_info
	);

	assert_false(result);
	pipeline = graphics_pipeline;

	device->set_object_name(pipeline, "{}_graphics_pipeline", debug_name);
}

void ShaderPipeline::create_compute_pipeline(vec<Shader *> const &shaders)
{
	ZoneScoped;

	auto const shader_stage_create_infos = create_shader_stage_create_infos(shaders);

	auto const compute_pipeline_info = vk::ComputePipelineCreateInfo {
		{}, //
		shader_stage_create_infos[0],
		pipeline_layout,
		{},
		{},
		{},
	};

	auto const [result, compute_pipeline] = device->vk().createComputePipeline(
	    {},
	    compute_pipeline_info
	);

	assert_false(result);
	pipeline = compute_pipeline;

	device->set_object_name(pipeline, "{}_compute_pipeline", debug_name);
}

auto ShaderPipeline::combine_descriptor_sets_bindings(vec<Shader *> const &shaders) const
    -> vec<vk::DescriptorSetLayoutBinding>
{
	ZoneScoped;

	auto combined_bindings = vec<vk::DescriptorSetLayoutBinding> {};

	for (Shader *const shader : shaders)
		for (auto const &descriptor_set_binding : shader->descriptor_set_bindings)
		{
			auto const binding_index = descriptor_set_binding.binding;

			if (combined_bindings.size() <= binding_index)
				combined_bindings.resize(binding_index + 1);

			combined_bindings[binding_index] = descriptor_set_binding;
		}

	return combined_bindings;
}

auto ShaderPipeline::create_shader_stage_create_infos(vec<Shader *> const &shaders) const
    -> vec<vk::PipelineShaderStageCreateInfo>
{
	ZoneScoped;

	auto shader_stage_create_infos = vec<vk::PipelineShaderStageCreateInfo> {};
	shader_stage_create_infos.resize(shaders.size());

	for (u32 i = 0; auto const &shader : shaders)
		shader_stage_create_infos[i++] = {
			{},
			shader->stage,
			shader->module,
			"main",
		};

	return shader_stage_create_infos;
}

} // namespace BINDLESSVK_NAMESPACE
