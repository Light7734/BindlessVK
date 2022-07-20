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

	// Shader
	VkDescriptorPool descriptorPool;
	const std::string vertexShaderPath;
	const std::string pixelShaderPath;

	std::vector<VkVertexInputBindingDescription> vertexBindingDescs;
	std::vector<VkVertexInputAttributeDescription> vertexAttribDescs;
};

struct PushConstants
{
	glm::mat4 projection;
	glm::mat4 view;
};

struct CommandBufferStartInfo
{
	VkDescriptorSet* descriptorSet;
	VkFramebuffer framebuffer;
	VkExtent2D extent;
	uint32_t frameIndex;
	PushConstants* pushConstants;
};

class Pipeline
{
public:
	Pipeline(PipelineCreateInfo& createInfo);
	~Pipeline();

	VkCommandBuffer RecordCommandBuffer(CommandBufferStartInfo& startInfo);

	UUID CreateRenderable(RenderableCreateInfo& createInfo);
	void RemoveRenderable(UUID renderableId);
	void RecreateBuffers();

private:
	VkDevice m_LogicalDevice          = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

	VkRenderPass m_RenderPass = VK_NULL_HANDLE;

	VkPipeline m_Pipeline             = VK_NULL_HANDLE;
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	QueueInfo m_QueueInfo        = {};
	uint32_t m_MaxFramesInFlight = 0u;

	// Shader
	std::unique_ptr<Shader> m_Shader = nullptr;

	std::unique_ptr<StagingBuffer> m_VertexBuffer = nullptr;
	std::unique_ptr<StagingBuffer> m_IndexBuffer  = nullptr;

	std::unique_ptr<Buffer> m_StorageBuffer = {};

	VkCommandPool m_CommandPool                   = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers = {};
	std::vector<VkDescriptorSet> m_DescriptorSets = {};
	VkDescriptorSetLayout m_DescriptorSetLayout   = VK_NULL_HANDLE;

	std::vector<Renderable> m_Renderables;
	uint32_t m_IndicesCount       = 0;
	uint32_t m_CurrentObjectLimit = 64u;
	uint32_t m_MaxDeadObjects     = std::floor(64 / 10.0);
	uint32_t m_MaxObjectLimit     = 2048;
};
