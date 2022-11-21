#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/RenderGraph.hpp"

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif

namespace BINDLESSVK_NAMESPACE {

class Renderer
{
public:
	struct CreateInfo
	{
		Device* device;
	};

public:
	Renderer() = default;
	void Init(const Renderer::CreateInfo& createInfo);

	void Reset();

	void OnSwapchainInvalidated();
	void DestroySwapchainResources();

	void BeginFrame(void* userPointer);
	void EndFrame(void* userPointer);

	void BuildRenderGraph(RenderGraph::BuildInfo buildInfo);

	void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function);

	inline vk::DescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

	inline vk::CommandPool GetCommandPool() const { return m_CommandPool; }
	inline uint32_t GetImageCount() const { return m_SwapchainImages.size(); }

	inline bool IsSwapchainInvalidated() const { return m_SwapchainInvalidated; }


private:
	struct UploadContext
	{
		vk::CommandBuffer cmdBuffer;
		vk::CommandPool cmdPool;
		vk::Fence fence;
	};

private:
	void CreateSyncObjects();
	void CreateDescriptorPools();

	void CreateSwapchain();

	void CreateCommandPool();

	void SubmitQueue(vk::Semaphore waitSemaphore, vk::Semaphore signalSemaphore, vk::Fence signalFence, vk::CommandBuffer cmd);
	void PresentFrame(vk::Semaphore waitSemaphore, uint32_t imageIndex);

private:
	UploadContext m_UploadContext = {};

	Device* m_Device          = {};
	RenderGraph m_RenderGraph = {};

	// Sync objects
	std::array<vk::Fence, MAX_FRAMES_IN_FLIGHT> m_RenderFences          = {};
	std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_RenderSemaphores  = {};
	std::array<vk::Semaphore, MAX_FRAMES_IN_FLIGHT> m_PresentSemaphores = {};

	bool m_SwapchainInvalidated = false;

	// Swapchain
	vk::SwapchainKHR m_Swapchain = {};

	std::vector<vk::Image> m_SwapchainImages         = {};
	std::vector<vk::ImageView> m_SwapchainImageViews = {};

	// Pools
	vk::CommandPool m_CommandPool       = {};
	vk::DescriptorPool m_DescriptorPool = {};

	std::vector<vk::CommandBuffer> m_CommandBuffers = {};

	uint32_t m_CurrentFrame = 0ul;
};

} // namespace BINDLESSVK_NAMESPACE
