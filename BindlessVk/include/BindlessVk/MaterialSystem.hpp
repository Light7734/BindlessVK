
#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"

#include <glm/glm.hpp>

namespace BINDLESSVK_NAMESPACE {

struct PipelineConfiguration
{
	struct CreateInfo
	{
		const char* name;

		vk::PipelineVertexInputStateCreateInfo vertexInputState;
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
		vk::PipelineTessellationStateCreateInfo tessellationState;
		vk::PipelineViewportStateCreateInfo viewportState;
		vk::PipelineRasterizationStateCreateInfo rasterizationState;
		vk::PipelineMultisampleStateCreateInfo multisampleState;
		vk::PipelineDepthStencilStateCreateInfo depthStencilState;

		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
		vk::PipelineColorBlendStateCreateInfo colorBlendState;

		std::vector<vk::DynamicState> dynamicStates;
	};

	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
	vk::PipelineTessellationStateCreateInfo tessellationState;
	vk::PipelineViewportStateCreateInfo viewportState;
	vk::PipelineRasterizationStateCreateInfo rasterizationState;
	vk::PipelineMultisampleStateCreateInfo multisampleState;
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;

	std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
	vk::PipelineColorBlendStateCreateInfo colorBlendState;

	std::vector<vk::DynamicState> dynamicStates;
	vk::PipelineDynamicStateCreateInfo dynamicState;
};

/// @todo
struct MaterialParameters
{
	glm::vec4 baseColorFactor;
	glm::vec4 emissiveFactor;
	glm::vec4 diffuseFactor;
	glm::vec4 specularFactor;
	float metallicFactor;
	float roughnessFactor;
};

/// @brief A vulkan shader module & it's code, for building ShaderEffects
struct Shader
{
	struct CreateInfo
	{
		const char* name;
		const char* path;
		vk::ShaderStageFlagBits stage;
	};

	vk::ShaderModule module;
	vk::ShaderStageFlagBits stage;
	std::vector<uint32_t> code;
};

/// @brief Shaders and pipeline layout for creating ShaderPasses
/// usually a pair of vertex and fragment <Shader>s
struct ShaderEffect
{
	struct CreateInfo
	{
		const char* name;
		std::vector<Shader*> shaders;
	};

	std::vector<Shader*> shaders;

	vk::PipelineLayout pipelineLayout;
	std::array<vk::DescriptorSetLayout, 2> setsLayout;
};

/// @brief Full shader & pipeline state needed for creating a master material
struct ShaderPass
{
	struct CreateInfo
	{
		const char* name;
		ShaderEffect* effect;
		vk::Format colorAttachmentFormat;
		vk::Format depthAttachmentFormat;
		PipelineConfiguration pipelineConfiguration;
	};

	ShaderEffect* effect;
	vk::Pipeline pipeline;
};

/// @brief Graphics state needed to render an object during a renderpass
struct Material
{
	struct CreateInfo
	{
		const char* name;
		ShaderPass* shaderPass;
		MaterialParameters parameters;
		std::vector<class Texture*> textures;
	};

	ShaderPass* shaderPass;
	MaterialParameters parameters;

	vk::DescriptorSet descriptorSet;
	std::vector<class Texture*> textures;

	uint32_t sortKey;
};

class MaterialSystem
{
public:
	struct CreateInfo
	{
		Device* device;
	};

public:
	MaterialSystem() = default;

	void Init(const MaterialSystem::CreateInfo& info);

	void Reset();

	void DestroyAllMaterials();

	inline Shader* GetShader(const char* name) { return &m_Shaders[HashStr(name)]; }
	inline ShaderEffect* GetShaderEffect(const char* name) { return &m_ShaderEffects[HashStr(name)]; }
	inline ShaderPass* GetShaderPass(const char* name) { return &m_ShaderPasses[HashStr(name)]; }
	inline Material* GetMaterial(const char* name) { return &m_Materials[HashStr(name)]; }

	inline const PipelineConfiguration& GetPipelineConfiguration(const char* name) const { return m_PipelineConfigurations.at(HashStr(name)); }

	void LoadShader(const Shader::CreateInfo& info);

	void CreatePipelineConfiguration(const PipelineConfiguration::CreateInfo& info);

	void CreateShaderEffect(const ShaderEffect::CreateInfo& info);

	void CreateShaderPass(const ShaderPass::CreateInfo& info);

	void CreateMaterial(const Material::CreateInfo& info);

private:
	Device* m_Device;
	vk::DescriptorPool m_DescriptorPool = {};

	std::unordered_map<uint64_t, Shader> m_Shaders             = {};
	std::unordered_map<uint64_t, ShaderEffect> m_ShaderEffects = {};
	std::unordered_map<uint64_t, ShaderPass> m_ShaderPasses    = {};

	std::unordered_map<uint64_t, PipelineConfiguration> m_PipelineConfigurations = {};

	std::unordered_map<uint64_t, Material> m_Materials = {};
};

} // namespace BINDLESSVK_NAMESPACE
