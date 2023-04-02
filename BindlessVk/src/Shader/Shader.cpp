#include "BindlessVk/Shader/Shader.hpp"

template<typename T>
void build_layout()
{
	auto const bindings = T::get_descriptor_set_bindings();
}

namespace BINDLESSVK_NAMESPACE {
ShaderPipeline::ShaderPipeline(
    VkContext const *const vk_context,
    LayoutAllocator *const layout_allocator,
    vec<Shader *> const &shaders,
    ShaderPipeline::Configuration const &configuration,
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout,
    str_view const debug_name /* = default_debug_name */
)
    : device(vk_context->get_device())
    , surface(vk_context->get_surface())
    , debug_utils(vk_context->get_debug_utils())
    , layout_allocator(layout_allocator)
    , debug_name(debug_name)
{
	create_descriptor_set_layout(shaders);
	create_pipeline_layout(graph_descriptor_set_layout, pass_descriptor_set_layout);

	create_pipeline(shaders, configuration);
}

ShaderPipeline::ShaderPipeline(ShaderPipeline &&effect)
{
	*this = std::move(effect);
}

ShaderPipeline &ShaderPipeline::operator=(ShaderPipeline &&other)
{
	this->device = other.device;
	this->surface = other.surface;
	this->debug_utils = other.debug_utils;
	this->layout_allocator = other.layout_allocator;
	this->pipeline = other.pipeline;
	this->pipeline_layout = other.pipeline_layout;
	this->descriptor_set_layout = other.descriptor_set_layout;

	other.device = {};

	return *this;
}

ShaderPipeline::~ShaderPipeline()
{
	if (!device)
		return;

	device->vk().destroyPipeline(pipeline);
}

void ShaderPipeline::create_descriptor_set_layout(vec<Shader *> const &shaders)
{
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

	debug_utils->set_object_name(
	    device->vk(),
	    descriptor_set_layout.vk(),
	    fmt::format("{}_descriptor_set_layout", debug_name)
	);
}

void ShaderPipeline::create_pipeline_layout(
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout
)
{
	pipeline_layout = layout_allocator->goc_pipeline_layout(
	    {},
	    graph_descriptor_set_layout, // set = 0 -> per graph(frame)
	    pass_descriptor_set_layout,  // set = 1 -> per pass
	    this->descriptor_set_layout  // set = 2 -> per shader
	);

	debug_utils->set_object_name(
	    device->vk(),
	    pipeline_layout,
	    fmt::format("{}_pipeline_layout", debug_name)
	);
}

void ShaderPipeline::create_pipeline(
    vec<Shader *> const &shaders,
    ShaderPipeline::Configuration const configuration
)
{
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

	auto const [result, graphics_pipeline] =
	    device->vk().createGraphicsPipeline({}, graphics_pipeline_info);

	assert_false(result);
	pipeline = graphics_pipeline;

	debug_utils->set_object_name(device->vk(), pipeline, fmt::format("{}_pipeline", debug_name));
}

auto ShaderPipeline::combine_descriptor_sets_bindings(vec<Shader *> const &shaders) const
    -> vec<vk::DescriptorSetLayoutBinding>
{
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
