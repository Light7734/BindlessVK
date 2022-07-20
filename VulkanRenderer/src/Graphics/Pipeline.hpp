#pragma once

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Renderable.hpp"
#include "Graphics/Shader.hpp"

#include <volk.h>

#include <glm/glm.hpp>

struct PipelineCreateInfo
{
	VkDevice logicalDevice;
	VkPhysicalDevice physicalDevice;
	uint32_t maxFramesInFlight;
	QueueInfo queueInfo;
	VkExtent2D viewportExtent;
	VkCommandPool commandPool;
	uint32_t imageCount;
	VkSampleCountFlagBits sampleCount;
	VkRenderPass renderPass;
	std::vector<std::shared_ptr<Buffer>> viewProjectionBuffers;

	// Shader
	VkDescriptorPool descriptorPool;
	const std::string vertexShaderPath;
	const std::string pixelShaderPath;

	std::vector<VkVertexInputBindingDescription> vertexBindingDescs;
	std::vector<VkVertexInputAttributeDescription> vertexAttribDescs;
};

struct CommandBufferStartInfo
{
	VkDescriptorSet* descriptorSet;
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

	void CreateRenderable(RenderableCreateInfo& createInfo);
	void RemoveRenderable(UUID renderableId);
	void RecreateBuffers();

private:
	VkDevice m_LogicalDevice;
	VkPhysicalDevice m_PhysicalDevice;
	VkCommandPool m_CommandPool;
	QueueInfo m_QueueInfo;

	uint32_t m_MaxFramesInFlight = 0u;

	VkRenderPass m_RenderPass;

	VkPipeline m_Pipeline;

	// Layout
	VkPipelineLayout m_PipelineLayout;

	// Shader
	std::unique_ptr<Shader> m_Shader;
	std::vector<std::shared_ptr<Buffer>> m_ViewProjectionBuffers;
	std::unique_ptr<StagingBuffer> m_VertexBuffer;
	std::unique_ptr<StagingBuffer> m_IndexBuffer;
	std::unique_ptr<Buffer> m_StorageBuffer;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;

	std::vector<Renderable> m_Renderables;
	uint32_t m_IndicesCount = 0;
	// Renderable* m_Model;
};
