#pragma once

#include "Core/Base.hpp"
#include "Graphics/Shader.hpp"

#include <volk.h>

struct PipelineCreateInfo
{
	VkDevice logicalDevice;
	VkExtent2D viewportExtent;

	const std::string vertexShaderPath;
	const std::string pixelShaderPath;
};

class Pipeline
{
public:
	Pipeline(PipelineCreateInfo& createInfo);
	~Pipeline();

private:
	VkDevice m_LogicalDevice;

	VkPipeline m_Pipeline;
	VkPipelineLayout m_PipelineLayout;
	VkRenderPass m_RenderPass;

	std::unique_ptr<Shader> m_Shader;
};
