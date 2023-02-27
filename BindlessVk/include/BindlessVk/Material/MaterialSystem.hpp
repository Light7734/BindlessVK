#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Shader/Shader.hpp"

#include <glm/glm.hpp>

namespace BINDLESSVK_NAMESPACE {
/// @todo
/** @brief Shaders and pipeline layout for creating ShaderPasses
 * usually a pair of vertex and fragment shaders
 */
class ShaderEffect
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
	ShaderEffect(
	    VkContext *vk_context,
	    vec<Shader *> const &shaders,
	    ShaderEffect::Configuration configuration,
	    c_str debug_name = ""
	);
	~ShaderEffect();

	ShaderEffect(ShaderEffect &&);
	ShaderEffect &operator=(ShaderEffect &&);

	ShaderEffect() = default;
	ShaderEffect(const ShaderEffect &) = delete;

	ShaderEffect &operator=(const ShaderEffect &) = delete;

	inline vk::Pipeline get_pipeline() const
	{
		return pipeline;
	}

	inline vk::PipelineLayout get_pipeline_layout() const
	{
		return pipeline_layout;
	}

	inline const arr<vk::DescriptorSetLayout, 2> &get_descriptor_set_layouts() const
	{
		return descriptor_sets_layout;
	}

private:
	ShaderEffect &move(ShaderEffect &&effect);

	void create_descriptor_sets_layout(vec<Shader *> const &shaders);

	auto combine_descriptor_sets_bindings(vec<Shader *> const &shaders)
	    -> arr<vec<vk::DescriptorSetLayoutBinding>, 2>;

	auto create_pipeline_layout() -> vk::PipelineLayout;

	auto create_pipeline_shader_stage_infos(vec<Shader *> shaders)
	    -> vec<vk::PipelineShaderStageCreateInfo>;

	auto create_graphics_pipeline(
	    vec<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos,
	    vk::PipelineRenderingCreateInfoKHR rendering_info,
	    ShaderEffect::Configuration configuration
	) -> vk::Pipeline;

private:
	VkContext *vk_context = {};
	str debug_name;

	vk::Pipeline pipeline = {};
	vk::PipelineLayout pipeline_layout = {};
	arr<vk::DescriptorSetLayout, 2> descriptor_sets_layout = {};

private:
};

class Material
{
public:
	struct Parameters
	{
		arr<f32, 4> albedo;
		arr<f32, 4> emissive;
		arr<f32, 4> diffuse;
		arr<f32, 4> specular;
		f32 metallic;
		f32 roughness;
	};

public:
	Material(VkContext *vk_context, ShaderEffect *effect, vk::DescriptorPool descriptor_pool);

	~Material() = default;

	inline auto *get_effect() const
	{
		return effect;
	}

	inline auto get_descriptor_set() const
	{
		return descriptor_set;
	}

private:
	ShaderEffect *effect = {};
	Parameters parametes = {};
	vk::DescriptorSet descriptor_set = {};
};

} // namespace BINDLESSVK_NAMESPACE
