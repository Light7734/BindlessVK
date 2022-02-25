#pragma once

#include "Core/Base.hpp"
#include "Graphics/Shader.hpp"

#include <volk.h>

struct PipelineCreateInfo
{
	VkDevice logicalDevice;
	VkExtent2D viewportExtent;
	VkCommandPool commandPool;
	uint32_t imageCount;
	VkRenderPass renderPass;

	const std::string vertexShaderPath;
	const std::string pixelShaderPath;
};

struct CommandBufferStartInfo
{
	VkFramebuffer framebuffer;
	VkExtent2D extent;
	uint32_t frameIndex;
};

class Pipeline
{
public:
	Pipeline(PipelineCreateInfo& createInfo);
	~Pipeline();

	VkCommandBuffer RecordCommandBuffer(CommandBufferStartInfo& startInfo);

private:
	VkDevice m_LogicalDevice;

	VkPipeline m_Pipeline;
	VkPipelineLayout m_PipelineLayout;
	VkRenderPass m_RenderPass;

	std::unique_ptr<Shader> m_Shader;
	std::vector<VkCommandBuffer> m_CommandBuffers;
};
