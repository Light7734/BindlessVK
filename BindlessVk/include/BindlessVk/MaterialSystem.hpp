
#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"

#include <glm/glm.hpp>

namespace BINDLESSVK_NAMESPACE {

struct PipelineConfiguration
{
	vk::PipelineVertexInputStateCreateInfo vertexInputState;
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
	vk::PipelineTessellationStateCreateInfo tessellationState;
	vk::PipelineViewportStateCreateInfo viewportState;
	vk::PipelineRasterizationStateCreateInfo rasterizationState;
	vk::PipelineMultisampleStateCreateInfo multisampleState;
	vk::PipelineDepthStencilStateCreateInfo depthStencilState;
	vk::PipelineColorBlendStateCreateInfo colorBlendState;
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
		PipelineConfiguration pipelineConfiguration;
		ShaderEffect* effect;
		vk::Format colorAttachmentFormat;
		vk::Format depthAttachmentFormat;
	};

	ShaderEffect* effect;
	vk::Pipeline pipeline;
};

/// @brief Template for materials to base themselves off of
struct MasterMaterial
{
	struct CreateInfo
	{
		const char* name;
		ShaderPass* shaderPass;
		MaterialParameters defaultParameters;
	};

	ShaderPass* shader;
	MaterialParameters defaultParameters;
};

/// @brief Graphics state needed to render an object during a renderpass
struct Material
{
	struct CreateInfo
	{
		const char* name;
		MasterMaterial* base;
		MaterialParameters parameters;
		std::vector<class Texture*> textures;
	};

	MasterMaterial* base;
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

	~MaterialSystem();

	void DestroyAllMaterials();

	inline Shader* GetShader(const char* name) { return &m_Shaders[HashStr(name)]; }
	inline ShaderEffect* GetShaderEffect(const char* name) { return &m_ShaderEffects[HashStr(name)]; }
	inline ShaderPass* GetShaderPass(const char* name) { return &m_ShaderPasses[HashStr(name)]; }
	inline MasterMaterial* GetMasterMaterial(const char* name) { return &m_MasterMaterials[HashStr(name)]; }
	inline Material* GetMaterial(const char* name) { return &m_Materials[HashStr(name)]; }

	void LoadShader(const Shader::CreateInfo& info);

	void CreateShaderEffect(const ShaderEffect::CreateInfo& info);

	void CreateShaderPass(const ShaderPass::CreateInfo& info);

	void CreateMasterMaterial(const MasterMaterial::CreateInfo& info);

	void CreateMaterial(const Material::CreateInfo& info);

private:
	Device* m_Device;
	vk::DescriptorPool m_DescriptorPool = {};

	std::unordered_map<uint64_t, Shader> m_Shaders             = {};
	std::unordered_map<uint64_t, ShaderEffect> m_ShaderEffects = {};
	std::unordered_map<uint64_t, ShaderPass> m_ShaderPasses    = {};

	std::unordered_map<uint64_t, MasterMaterial> m_MasterMaterials = {};
	std::unordered_map<uint64_t, Material> m_Materials             = {};
};

} // namespace BINDLESSVK_NAMESPACE
