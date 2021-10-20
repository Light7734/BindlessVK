#pragma once

#include "Base.h"

#include "SharedContext.h"

#include <volk.h>

#include <shaderc/shaderc.hpp>

class Shader
{
private:
	enum Stage
	{
		NONE = 0,
		VERTEX = 1,
		PIXEL = 2,
		GEOMETRY = 3
	};

private:
	SharedContext m_SharedContext;

	VkShaderModule m_PixelShaderModule;
	VkShaderModule m_VertexShaderModule;

	std::array<VkPipelineShaderStageCreateInfo, 2> m_PipelineShaderStageCreateInfos;

public:
	Shader(const std::string& vertexPath, const std::string& pixelPath, SharedContext sharedContext);
	~Shader();

	inline std::array<VkPipelineShaderStageCreateInfo, 2> GetShaderStages() { return m_PipelineShaderStageCreateInfos; }

private:
	shaderc::SpvCompilationResult CompileGlslToSpv(const std::string& path, Stage stage);
};