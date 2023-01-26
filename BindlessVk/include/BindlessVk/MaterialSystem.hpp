
#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Shader.hpp"
#include "BindlessVk/VkContext.hpp"

#include <glm/glm.hpp>

namespace BINDLESSVK_NAMESPACE {
/// @todo
struct MaterialParameters
{
	glm::vec4 base_color_factor;
	glm::vec4 emissive_factor;
	glm::vec4 diffuse_factor;
	glm::vec4 specular_factor;
	float metallic_factor;
	float roughness_factor;
};

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
	    VkContext* vk_context,
	    vec<Shader*> shaders,
	    ShaderEffect::Configuration configuration
	);
	~ShaderEffect();

	ShaderEffect(ShaderEffect&&);
	ShaderEffect& operator=(ShaderEffect&&);

	ShaderEffect() = default;
	ShaderEffect(const ShaderEffect&) = delete;

	ShaderEffect& operator=(const ShaderEffect&) = delete;

	inline vk::Pipeline get_pipeline() const
	{
		return pipeline;
	}

	inline vk::PipelineLayout get_pipeline_layout() const
	{
		return pipeline_layout;
	}

	inline const arr<vk::DescriptorSetLayout, 2>& get_descriptor_set_layouts() const
	{
		return descriptor_sets_layout;
	}

private:
	ShaderEffect& move(ShaderEffect&& effect);

	void create_descriptor_sets_layout(vec<Shader*> shaders);

	arr<vec<vk::DescriptorSetLayoutBinding>, 2> combine_descriptor_sets_bindings(
	    vec<Shader*> shaders
	);

	vk::PipelineLayout create_pipeline_layout();

	vec<vk::PipelineShaderStageCreateInfo> create_pipeline_shader_stage_infos(vec<Shader*> shaders);

	vk::Pipeline create_graphics_pipeline(
	    vec<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos,
	    vk::PipelineRenderingCreateInfoKHR rendering_info,
	    ShaderEffect::Configuration configuration
	);

private:
	VkContext* vk_context = {};

	vk::Pipeline pipeline = {};
	vk::PipelineLayout pipeline_layout = {};
	arr<vk::DescriptorSetLayout, 2> descriptor_sets_layout = {};

private:
};

class Material
{
public:
	Material(VkContext* vk_context, ShaderEffect* effect, vk::DescriptorPool descriptor_pool)
	    : effect(effect)
	{
		vk::DescriptorSetAllocateInfo allocate_info {
			descriptor_pool,
			1,
			&effect->get_descriptor_set_layouts().back(),
		};

		const auto device = vk_context->get_device();
		assert_false(device.allocateDescriptorSets(&allocate_info, &descriptor_set));
	}

	Material() = default;
	Material(const Material&) = default;
	Material(Material&&) = default;

	Material& operator=(const Material&) = default;
	Material& operator=(Material&&) = default;

	~Material() = default;

	inline ShaderEffect* get_effect() const
	{
		return effect;
	}

	inline vk::DescriptorSet get_descriptor_set() const
	{
		return descriptor_set;
	}

private:
	ShaderEffect* effect = {};
	vk::DescriptorSet descriptor_set = {};
};

} // namespace BINDLESSVK_NAMESPACE
