
#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Device.hpp"

#include <glm/glm.hpp>

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

	std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
	vk::PipelineColorBlendStateCreateInfo color_blend_state;

	std::vector<vk::DynamicState> dynamic_states;
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

/** @brief A vulkan shader module & it's code, for building ShaderEffects */
struct Shader
{
	vk::ShaderModule module;
	vk::ShaderStageFlagBits stage;
	std::vector<uint32_t> code;
};

/** @brief Shaders and pipeline layout for creating ShaderPasses
 * usually a pair of vertex and fragment shaders
 */
struct ShaderEffect
{
	std::vector<Shader*> shaders;
	vk::PipelineLayout pipeline_layout;
	std::array<vk::DescriptorSetLayout, 2> sets_layout;
};

/// @brief Full shader & pipeline state needed for creating a master material
struct ShaderPass
{
	ShaderEffect* effect;
	vk::Pipeline pipeline;
};

/// @brief Graphics state needed to render an object during a renderpass
struct Material
{
	ShaderPass* shader_pass;
	MaterialParameters parameters;
	vk::DescriptorSet descriptor_set;
	std::vector<class Texture*> textures;
	uint32_t sort_key;
};

/** @brief System for loading/destroying materials
 * @todo: complete docs
 * @todo: refactor code
 */
class MaterialSystem
{
public:
	MaterialSystem() = default;

	/** @return shader named @p name */
	inline Shader* get_shader(const char* name)
	{
		return &shaders[hash_str(name)];
	}

	/** @return shader effect named @p name */
	inline ShaderEffect* get_shader_effect(const char* name)
	{
		return &shader_effects[hash_str(name)];
	}

	/** @return shader pass named @p name */
	inline ShaderPass* get_shader_pass(const char* name)
	{
		return &shader_passes[hash_str(name)];
	}

	/** @return material named @p name */
	inline Material* get_material(const char* name)
	{
		return &materials[hash_str(name)];
	}

	/** @return pipeline configuration named @p name */
	inline const PipelineConfiguration& get_pipeline_configuration( // Nvr-gnna-gv-u-up
	    const char* name
	) const
	{
		return pipline_configurations.at(hash_str(name));
	}

	/** Initializes the material system
	 * @param device the bindlessvk device
	 */
	void init(Device* device);

	/** Destroys the material system */
	void reset();

	/** Destroys all pipelines and resets the descriptor pool  */
	void destroy_all_materials();

	/** Loads compiled shader named @p name from @p path
	 * @param name name of the shader
	 * @param path path to the compiled shader
	 * @param stage shader stage (eg. vertex)
	 */
	void load_shader(const char* name, const char* path, vk::ShaderStageFlagBits stage);

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
	 * @param dynamic_states vulkan dynamic pipeline's states, usually viewports &
	 * scissors
	 */
	void create_pipeline_configuration(
	    const char* name,
	    vk::PipelineVertexInputStateCreateInfo vertex_input_state,
	    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state,
	    vk::PipelineTessellationStateCreateInfo tessellation_state,
	    vk::PipelineViewportStateCreateInfo viewport_state,
	    vk::PipelineRasterizationStateCreateInfo rasterization_state,
	    vk::PipelineMultisampleStateCreateInfo multisample_state,
	    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state,
	    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments,
	    vk::PipelineColorBlendStateCreateInfo color_blend_state,
	    std::vector<vk::DynamicState> dynamic_states
	);

	/** Creates shader effect named @p name
	 * @param name name of the shader effect
	 * @param shaders shader programs making up the effect
	 */
	void create_shader_effect(const char* name, std::vector<Shader*> shaders);

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
	void create_shader_pass(
	    const char* name,
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
	void create_material(
	    const char* name,
	    ShaderPass* shader_pass,
	    MaterialParameters parameters,
	    std::vector<class Texture*> textures
	);

private:
	Device* device;
	vk::DescriptorPool descriptor_pool = {};

	hash_map<u64, Shader> shaders = {};
	hash_map<u64, ShaderEffect> shader_effects = {};
	hash_map<u64, ShaderPass> shader_passes = {};

	hash_map<u64, PipelineConfiguration> pipline_configurations = {};

	hash_map<u64, Material> materials = {};
};

} // namespace BINDLESSVK_NAMESPACE
