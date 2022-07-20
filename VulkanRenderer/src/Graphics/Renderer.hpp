#pragma once

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/Renderable.hpp"
#include "Graphics/Texture.hpp"

#include <volk.h>

struct RendererCreateInfo
{
	VkDevice logicalDevice;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkSampleCountFlagBits sampleCount;
	SurfaceInfo surfaceInfo;
	QueueInfo queueInfo;
};

class Renderer
{
public:
	Renderer(const RendererCreateInfo& createInfo);
	~Renderer();

	void BeginFrame();
	void Draw();
	void EndFrame();

	inline VkCommandPool GetCommandPool() const { return m_CommandPool; }
	inline uint32_t GetImageCount() const { return m_Images.size(); }

	inline bool IsSwapchainInvalidated() const { return m_SwapchainInvalidated; }

private:
	VkDevice m_LogicalDevice  = VK_NULL_HANDLE;
	QueueInfo m_QueueInfo     = {};
	SurfaceInfo m_SurfaceInfo = {};

	PushConstants m_ViewProjection;
	bool m_SwapchainInvalidated = false;

	// Swapchain
	VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

	std::vector<VkImage> m_Images             = {};
	std::vector<VkImageView> m_ImageViews     = {};
	std::vector<VkFramebuffer> m_Framebuffers = {};

	// Multisampling
	VkImage m_ColorImage;
	VkDeviceMemory m_ColorImageMemory;
	VkImageView m_ColorImageView;

	// RenderPass
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;

	VkSampleCountFlagBits m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
	// Commands
	VkCommandPool m_CommandPool                   = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers = {};

	// Synchronization
	std::vector<VkSemaphore> m_AquireImageSemaphores = {};
	std::vector<VkSemaphore> m_RenderSemaphores      = {};
	std::vector<VkFence> m_FrameFences               = {};
	const uint32_t m_MaxFramesInFlight               = 2u;
	uint32_t m_CurrentFrame                          = 0u;

	// Descriptor sets
	VkDescriptorSetLayout m_DescriptorSetLayout   = VK_NULL_HANDLE;
	VkDescriptorPool m_DescriptorPool             = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_DescriptorSets = {};

	// Depth buffer
	VkFormat m_DepthFormat;
	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;

	// Pipelines
	std::vector<std::shared_ptr<Pipeline>> m_Pipelines = {};
	std::shared_ptr<Texture> m_StatueTexture; // #TEMP
};
