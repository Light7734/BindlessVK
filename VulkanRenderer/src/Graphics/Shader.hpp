#pragma once

#include "Core/Base.hpp"

#include <shaderc/shaderc.hpp>
#include <volk.h>

struct ShaderCreateInfo
{
	VkDevice logicalDevice;
	shaderc_optimization_level optimizationLevel;
	const std::string vertexPath;
	const std::string pixelPath;
};

class Shader
{
public:
	Shader(ShaderCreateInfo& createInfo);
	~Shader();

private:
	// Shader stages enum
	enum class Stage : uint8_t
	{
		None = 0,
		Vertex,
		Pixel,
		Geometry,
	};

	// Device
	VkDevice m_LogicalDevice;

	// Modules
	VkShaderModule m_VertexShaderModule;
	VkShaderModule m_PixelShaderModule;

	// Pipeline
	VkPipelineShaderStageCreateInfo m_PipelineShaderCreateInfos[2];

	shaderc::SpvCompilationResult CompileGlslToSpv(const std::string& path, Stage stage, shaderc_optimization_level optimizationLevel);
};
