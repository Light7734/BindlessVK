#include "BindlessVk/Shader/Shader.hpp"

template<typename T>
void build_layout()
{
	auto const bindings = T::get_descriptor_set_bindings();
}

namespace BINDLESSVK_NAMESPACE {
ShaderPipeline::ShaderPipeline(
    VkContext *const vk_context,
    vec<Shader *> const &shaders,
    ShaderPipeline::Configuration const &configuration,
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout,
    str_view const debug_name /* = "" */
)
    : vk_context(vk_context)
    , debug_name(debug_name)
{
	vk_context->log(LogLvl::eTrace, "creating pipeline: {}", debug_name);
	create_descriptor_set_layout(shaders);
	create_pipeline_layout(graph_descriptor_set_layout, pass_descriptor_set_layout);

	create_pipeline(shaders, configuration);
}

ShaderPipeline::ShaderPipeline(ShaderPipeline &&effect)
{
	*this = std::move(effect);
}

ShaderPipeline &ShaderPipeline::operator=(ShaderPipeline &&effect)
{
	this->vk_context = effect.vk_context;
	this->pipeline = effect.pipeline;
	this->pipeline_layout = effect.pipeline_layout;
	this->descriptor_set_layout = effect.descriptor_set_layout;

	effect.vk_context = {};

	return *this;
}

ShaderPipeline::~ShaderPipeline()
{
	if (!vk_context)
		return;

	auto const device = vk_context->get_device();

	device.destroyPipeline(pipeline);
}

void ShaderPipeline::create_descriptor_set_layout(vec<Shader *> const &shaders)
{
	auto *const layout_allocator = vk_context->get_layout_allocator();

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
	vk_context->set_object_name(
	    descriptor_set_layout,
	    fmt::format("{}_descriptor_set_layout", debug_name)
	);
}

void ShaderPipeline::create_pipeline_layout(
    DescriptorSetLayoutWithHash graph_descriptor_set_layout,
    DescriptorSetLayoutWithHash pass_descriptor_set_layout
)
{
	auto *layout_allocator = vk_context->get_layout_allocator();
	pipeline_layout = layout_allocator->goc_pipeline_layout(
	    {},
	    graph_descriptor_set_layout, // set = 0 -> per graph(frame)
	    pass_descriptor_set_layout,  // set = 1 -> per pass
	    this->descriptor_set_layout  // set = 2 -> per shader
	);

	vk_context->set_object_name(pipeline_layout, fmt::format("{}_pipeline_layout", debug_name));
}

void ShaderPipeline::create_pipeline(
    vec<Shader *> const &shaders,
    ShaderPipeline::Configuration const configuration
)
{
	auto const device = vk_context->get_device();
	auto const surface = vk_context->get_surface();
	auto const surface_color_format = surface.get_color_format();

	auto const pipeline_rendering_info = vk::PipelineRenderingCreateInfo {
		{},
		surface_color_format,
		vk_context->get_depth_format(),
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
	    device.createGraphicsPipeline({}, graphics_pipeline_info);

	assert_false(result);
	pipeline = graphics_pipeline;

	vk_context->set_object_name(pipeline, fmt::format("{}_pipeline", debug_name));
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
