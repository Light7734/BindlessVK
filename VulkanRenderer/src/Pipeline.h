#pragma once

#include "Base.h"

#include "Shader.h"

#include <volk.h>

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	inline bool IsSuitableForRendering() const { !formats.empty() && !presentModes.empty(); }

	operator bool() const { return IsSuitableForRendering(); }
};

class Pipeline
{
private:
	// constants
	const uint32_t m_FramesInFlight;

	// context
	SharedContext m_SharedContext;

	// pipeline
	VkPipeline m_Pipeline;
	VkPipelineLayout m_PipelineLayout;
	VkRenderPass m_RenderPass;

	// swap chain
	VkSwapchainKHR m_Swapchain;

	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;
	std::vector<VkFramebuffer> m_SwapchainFramebuffers;

	VkFormat m_SwapchainImageFormat;
	VkExtent2D m_SwapchainExtent;

	SwapchainSupportDetails m_SwapChainDetails;

	// commnad
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	// synchronization
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_Fences;
	std::vector<VkFence> m_ImagesInFlight;
	size_t m_CurrentFrame = 0ull;

public:
	Pipeline(SharedContext sharedContext, std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages, uint32_t frames = 2u);
	~Pipeline();

	uint32_t AquireNextImage();
	void SubmitCommandBuffer(uint32_t imageIndex);

private:
	void CreateSwapchain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreatePipelineLayout();
	void CreatePipeline(std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages);
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronizations();

	void FetchSwapchainSupportDetails();

};