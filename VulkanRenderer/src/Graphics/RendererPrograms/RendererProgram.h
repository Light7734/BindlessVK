#pragma once

#include "Core/Base.h"
#include "Graphics/Buffers.h"
#include "Graphics/DeviceContext.h"
#include "Graphics/Shader.h"

#include <volk.h>

class RendererProgram
{
protected:
	class Device* m_Device = nullptr;

	VkPipeline m_Pipeline             = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	VkDescriptorPool m_DescriptorPool           = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	VkViewport m_Viewport = {};
	VkRect2D m_Scissors   = {};

	uint32_t m_SwapchainImageCount = 0u;

	std::unique_ptr<Shader> m_Shader = nullptr;

	std::unique_ptr<Buffer> m_VertexBuffer = nullptr;
	std::unique_ptr<Buffer> m_IndexBuffer  = nullptr;

	std::unique_ptr<Buffer> m_StagingVertexBuffer = nullptr;
	std::unique_ptr<Buffer> m_StagingIndexBuffer  = nullptr;

public:
	RendererProgram(class Device* device, uint32_t swapchainImageCount);

	virtual ~RendererProgram();
};