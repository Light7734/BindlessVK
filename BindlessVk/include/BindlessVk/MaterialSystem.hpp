
#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"

#include <glm/glm.hpp>

struct SpvReflectDescriptorSet;
struct SpvReflectDescriptorBinding;

namespace BINDLESSVK_NAMESPACE {
struct PipelineConfiguration
{
	vk::PipelineVertexInputStateCreateInfo vertex_input_state;
	vk::PipelineInputAssemblyStateCreateInfo input_assembly_state;
	vk::PipelineTessellationStateCreateInfo tesselation_state;
	vk::PipelineViewportStateCreateInfo viewport_state;
	vk::PipelineRasterizationStateCreateInfo rasterization_state;
	vk::PipelineMultisampleStateCreateInfo multisample_state;
	vk::PipelineDepthStencilStateCreateInfo depth_stencil_state;

	vec<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
	vk::PipelineColorBlendStateCreateInfo color_blend_state;

	vec<vk::DynamicState> dynamic_states;
	vk::PipelineDynamicStateCreateInfo dynamic_state;
};

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

/** @brief A vulkan shader module & it's code, for building ShaderEffects
 * @todo Don't store shader code
 */
struct Shader
{
	vk::ShaderModule module;
	vk::ShaderStageFlagBits stage;
	vec<u32> code;
};

/** @brief Shaders and pipeline layout for creating ShaderPasses
 * usually a pair of vertex and fragment shaders
 */
struct ShaderEffect
{
	vec<Shader*> shaders;
	vk::PipelineLayout pipeline_layout;
	arr<vk::DescriptorSetLayout, 2> sets_layout;
};

/** @brief Full shader & pipeline state needed for creating a material */
struct ShaderPass
{
	ShaderEffect* effect;
	vk::Pipeline pipeline;
	PipelineConfiguration pipeline_configuration;
};

/** @brief Graphics state needed to render an object during a renderpass */
struct Material
{
	ShaderPass* shader_pass;
	vk::DescriptorSet descriptor_set;
	vec<class Texture*> textures;
	u32 sort_key;
};

/** @brief System for loading/destroying materials
 * @todo #docs: Complete docs
 * @todo #refactor: Tidy up code
 * @todo #responsibility: This class should not be responsible for creating descriptor pools
 */
class MaterialSystem
{
public:
	/** @brief Main constructor */
	MaterialSystem(Device* device);

	/** @brief Main destructor */
	~MaterialSystem();

	/** @brief default constructo */
	MaterialSystem() = default;

	/** Loads compiled shader named @p name from @p path
	 * @param name name of the shader
	 * @param path path to the compiled shader
	 * @param stage shader stage (eg. vertex)
	 */
	Shader load_shader(c_str name, c_str path, vk::ShaderStageFlagBits stage);

	/** Caches pipeline states for future pipeline creation (used in shader pass)
	 * @param name name of the pipeline configuration
	 * @param vertex_input_state vulkan pipeline's vertex input state
	 * @param input_assembly_state vulkan pipeline's input assembly state
	 * @param viewport_state vulkan pipeline's viewport state
	 * @param rasterization_state vulkan pipeilne's rasterization state
	 * @param multisample_state vulkan pipeline's multi-sample state
	 * @param depth_stencil_state vulkan pipeline's depth-stencil state
	 * @param color_blend_attachments vulkan pipeline's color-blend attachments
	 * @param color_blend_state vulkan pipeline's color blend state
	 * @param dynamic_states vulkan dynamic pipeline's states, usually viewports & scissors
	 */
	PipelineConfiguration create_pipeline_configuration(
	    c_str name,
	    vk::PipelineVertexInputStateCreateInfo vertex_input_state,
	    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state,
	    vk::PipelineTessellationStateCreateInfo tessellation_state,
	    vk::PipelineViewportStateCreateInfo viewport_state,
	    vk::PipelineRasterizationStateCreateInfo rasterization_state,
	    vk::PipelineMultisampleStateCreateInfo multisample_state,
	    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state,
	    vec<vk::PipelineColorBlendAttachmentState> color_blend_attachments,
	    vk::PipelineColorBlendStateCreateInfo color_blend_state,
	    vec<vk::DynamicState> dynamic_states
	);

	/** Creates shader effect named @p name
	 * @param name name of the shader effect
	 * @param shaders shader programs making up the effect
	 */
	ShaderEffect create_shader_effect(c_str name, vec<Shader*> shaders);

	/** Creates shader pass named @p name, vulkan graphics pipeline is created
	 * here
	 * @param name name of the shader pass
	 * @param effect shader effect of the pass
	 * @param color_attachment_format format of the color attachment of shader
	 * pass
	 * @param depth_attachment_format format of the depth attachment of shader
	 * pass
	 * @param pipeline_configuration vulkan pipeline states
	 */
	ShaderPass create_shader_pass(
	    c_str name,
	    ShaderEffect* effect,
	    vk::Format color_attachment_format,
	    vk::Format depth_attachment_format,
	    PipelineConfiguration pipeline_configuration
	);

	/** Creates material named @p name shader_pass
	 * @param name name of the material
	 * @param shader_pass the shader pass used in the material
	 * @param parameters inital material parameters
	 * @param textures textures used in the material
	 */
	Material create_material(c_str name, ShaderPass* shader_pass, vec<class Texture*> textures);

private:
	vec<u32> load_shader_code(c_str path);

	arr<vec<vk::DescriptorSetLayoutBinding>, 2> reflect_shader_effect_bindings(vec<Shader*> shader);

	vec<SpvReflectDescriptorSet*> reflect_spv_descriptor_sets(Shader* shader);
	arr<vk::DescriptorSetLayout, 2u> create_descriptor_sets_layout(
	    arr <vec<vk::DescriptorSetLayoutBinding>, 2> sets_bindings
	);

	vec<vk::DescriptorSetLayoutBinding> extract_spv_descriptor_set_bindings(
	    SpvReflectDescriptorSet* spv_set
	);

	vk::DescriptorSetLayoutBinding extract_descriptor_binding(
	    SpvReflectDescriptorBinding* spv_binding,
	    Shader* shader
	);

private:
	Device* device = {};
	vk::DescriptorPool descriptor_pool = {};
};

} // namespace BINDLESSVK_NAMESPACE
