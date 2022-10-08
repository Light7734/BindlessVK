#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Mesh.hpp"
#include "Graphics/Shader.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct PipelineCreateInfo
{
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	vma::Allocator allocator;
	uint32_t maxFramesInFlight;
	QueueInfo queueInfo;
	vk::Extent2D viewportExtent;
	vk::CommandPool commandPool;
	uint32_t imageCount;
	vk::SampleCountFlagBits sampleCount;
	vk::RenderPass renderPass;
	std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;

	// Shader
	vk::DescriptorPool descriptorPool;
	const std::string vertexShaderPath;
	const std::string pixelShaderPath;

	std::vector<vk::VertexInputBindingDescription> vertexBindingDescs;
	std::vector<vk::VertexInputAttributeDescription> vertexAttribDescs;
};


struct CommandBufferStartInfo
{
	vk::DescriptorSet* descriptorSet;
	vk::Framebuffer framebuffer;
	vk::Extent2D extent;
	uint32_t frameIndex;
};

class Pipeline
{
public:
	Pipeline(PipelineCreateInfo& createInfo);
	~Pipeline();

	void RecordCommandBuffer(vk::CommandBuffer cmd);

	UUID CreateRenderable(MeshCreateInfo& createInfo);
	void RemoveRenderable(UUID renderableId);
	void RecreateBuffers();

private:
	vk::Device m_LogicalDevice          = {};
	vk::PhysicalDevice m_PhysicalDevice = {};

	vma::Allocator m_Allocator = {};

	vk::RenderPass m_RenderPass = {};

	vk::Pipeline m_Pipeline             = {};
	vk::PipelineLayout m_PipelineLayout = {};

	QueueInfo m_QueueInfo        = {};
	uint32_t m_MaxFramesInFlight = {};

	// Shader
	std::unique_ptr<Shader> m_Shader = {};

	std::unique_ptr<StagingBuffer> m_VertexBuffer = {};
	std::unique_ptr<StagingBuffer> m_IndexBuffer  = {};

	std::unique_ptr<Buffer> m_StorageBuffer = {};

	vk::CommandPool m_CommandPool                   = {};
	std::vector<vk::CommandBuffer> m_CommandBuffers = {};
	std::vector<vk::DescriptorSet> m_DescriptorSets = {};
	vk::DescriptorSetLayout m_DescriptorSetLayout   = {};

	std::vector<Renderable> m_Renderables;
	uint32_t m_IndicesCount       = 0;
	uint32_t m_CurrentObjectLimit = 64u;
	uint32_t m_MaxDeadObjects     = std::floor(64 / 10.0);
	uint32_t m_MaxObjectLimit     = 2048;
};
