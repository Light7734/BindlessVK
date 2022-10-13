
#pragma once

#include "Core/Base.hpp"
#include "Graphics/Texture.hpp"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

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
		vk::RenderPass renderPass;
		uint32_t subpass;
	};

	ShaderEffect* effect;

	vk::RenderPass renderPass;
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
		std::vector<Texture*> textures;
	};

	MasterMaterial* base;
	MaterialParameters parameters;

	vk::DescriptorSet descriptorSet;
	std::vector<Texture*> textures;

	uint32_t sortKey;
};

class MaterialSystem
{
public:
	struct CreateInfo
	{
		vk::Device logicalDevice;
	};

public:
	MaterialSystem(const MaterialSystem::CreateInfo& info);

	MaterialSystem() = default;
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
	vk::Device m_LogicalDevice          = {};
	vk::DescriptorPool m_DescriptorPool = {};

	std::unordered_map<uint64_t, Shader> m_Shaders             = {};
	std::unordered_map<uint64_t, ShaderEffect> m_ShaderEffects = {};
	std::unordered_map<uint64_t, ShaderPass> m_ShaderPasses    = {};

	std::unordered_map<uint64_t, MasterMaterial> m_MasterMaterials = {};
	std::unordered_map<uint64_t, Material> m_Materials             = {};
};
