#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Renderable.hpp"
#include "Graphics/Shader.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct PipelineCreateInfo
{
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	uint32_t maxFramesInFlight;
	QueueInfo queueInfo;
	vk::Extent2D viewportExtent;
	vk::CommandPool commandPool;
	uint32_t imageCount;
	vk::SampleCountFlagBits sampleCount;
	vk::RenderPass renderPass;

	// Shader
	vk::DescriptorPool descriptorPool;
	const std::string vertexShaderPath;
	const std::string pixelShaderPath;

	std::vector<vk::VertexInputBindingDescription> vertexBindingDescs;
	std::vector<vk::VertexInputAttributeDescription> vertexAttribDescs;
};

struct PushConstants
{
	glm::mat4 projection;
	glm::mat4 view;
};

struct CommandBufferStartInfo
{
	vk::DescriptorSet* descriptorSet;
	vk::Framebuffer framebuffer;
	vk::Extent2D extent;
	uint32_t frameIndex;
	PushConstants* pushConstants;
};

class Pipeline
{
public:
	Pipeline(PipelineCreateInfo& createInfo);
	~Pipeline();

	vk::CommandBuffer RecordCommandBuffer(CommandBufferStartInfo& startInfo);

	UUID CreateRenderable(RenderableCreateInfo& createInfo);
	void RemoveRenderable(UUID renderableId);
	void RecreateBuffers();

private:
	vk::Device m_LogicalDevice          = VK_NULL_HANDLE;
	vk::PhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

	vk::RenderPass m_RenderPass = VK_NULL_HANDLE;

	vk::Pipeline m_Pipeline             = VK_NULL_HANDLE;
	vk::PipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

	QueueInfo m_QueueInfo        = {};
	uint32_t m_MaxFramesInFlight = 0u;

	// Shader
	std::unique_ptr<Shader> m_Shader = nullptr;

	std::unique_ptr<StagingBuffer> m_VertexBuffer = nullptr;
	std::unique_ptr<StagingBuffer> m_IndexBuffer  = nullptr;

	std::unique_ptr<Buffer> m_StorageBuffer = {};

	vk::CommandPool m_CommandPool                   = VK_NULL_HANDLE;
	std::vector<vk::CommandBuffer> m_CommandBuffers = {};
	std::vector<vk::DescriptorSet> m_DescriptorSets = {};
	vk::DescriptorSetLayout m_DescriptorSetLayout   = VK_NULL_HANDLE;

	std::vector<Renderable> m_Renderables;
	uint32_t m_IndicesCount       = 0;
	uint32_t m_CurrentObjectLimit = 64u;
	uint32_t m_MaxDeadObjects     = std::floor(64 / 10.0);
	uint32_t m_MaxObjectLimit     = 2048;
};
