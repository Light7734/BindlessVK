#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Shader
{
	vk::ShaderModule module;
	vk::ShaderStageFlagBits stage;
	arr<vec<vk::DescriptorSetLayoutBinding>, 2> descriptor_sets_bindings;
};

class ShaderPipeline
{
public:
	struct Configuration
	{
		vk::PipelineVertexInputStateCreateInfo vertex_input_state;
		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;
		vk::PipelineTessellationStateCreateInfo tesselation_state;
		vk::PipelineViewportStateCreateInfo viewport_state;
		vk::PipelineRasterizationStateCreateInfo rasterization_state;
		vk::PipelineMultisampleStateCreateInfo multisample_state;
		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;

		vk::PipelineColorBlendStateCreateInfo color_blend_state;

		vec<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
		vec<vk::DynamicState> dynamic_states;
	};

public:
	ShaderPipeline(
	    VkContext *vk_context,
	    vec<Shader *> const &shaders,
	    ShaderPipeline::Configuration configuration,
	    str_view debug_name = ""
	);

	~ShaderPipeline();

	ShaderPipeline(ShaderPipeline &&);
	ShaderPipeline &operator=(ShaderPipeline &&);

	ShaderPipeline(const ShaderPipeline &) = delete;
	ShaderPipeline &operator=(const ShaderPipeline &) = delete;

	inline auto get_name() const
	{
		return str_view(debug_name);
	}

	inline auto get_pipeline() const
	{
		return pipeline;
	}

	inline auto get_pipeline_layout() const
	{
		return pipeline_layout;
	}

	inline auto &get_descriptor_set_layouts() const
	{
		return descriptor_set_layouts;
	}

private:
	void create_descriptor_sets_layout(vec<Shader *> const &shaders);

	void create_pipeline_layout();

	void create_pipeline(vec<Shader *> const &shaders, ShaderPipeline::Configuration configuration);

	auto combine_descriptor_sets_bindings(vec<Shader *> const &shaders) const
	    -> arr<vec<vk::DescriptorSetLayoutBinding>, 2>;

	auto create_shader_stage_create_infos(vec<Shader *> const &shaders) const
	    -> vec<vk::PipelineShaderStageCreateInfo>;


private:
	VkContext *vk_context = {};

	vk::Pipeline pipeline = {};
	vk::PipelineLayout pipeline_layout = {};
	arr<vk::DescriptorSetLayout, 2> descriptor_set_layouts = {};

	str debug_name = {};
};

} // namespace BINDLESSVK_NAMESPACE
