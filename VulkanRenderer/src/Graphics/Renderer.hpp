#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/Renderable.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Types.hpp"

#include <functional>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

struct RendererCreateInfo
{
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	vk::PhysicalDeviceProperties physicalDeviceProperties;
	vma::Allocator allocator;
	vk::SampleCountFlagBits sampleCount;
	SurfaceInfo surfaceInfo;
	QueueInfo queueInfo;
};

struct FrameInfo
{
};

class Renderer
{
public:
	Renderer(const RendererCreateInfo& createInfo);
	~Renderer();

	void BeginFrame();
	void Draw();
	void EndFrame();

	inline vk::CommandPool GetCommandPool() const { return m_CommandPool; }
	inline uint32_t GetImageCount() const { return m_Images.size(); }

	inline bool IsSwapchainInvalidated() const { return m_SwapchainInvalidated; }

private:
	vk::Device m_LogicalDevice = {};
	QueueInfo m_QueueInfo      = {};
	SurfaceInfo m_SurfaceInfo  = {};

	vma::Allocator m_Allocator;

	PushConstants m_ViewProjection = {};
	bool m_SwapchainInvalidated    = false;

	// Swapchain
	vk::SwapchainKHR m_Swapchain = {};

	std::vector<vk::Image> m_Images             = {};
	std::vector<vk::ImageView> m_ImageViews     = {};
	std::vector<vk::Framebuffer> m_Framebuffers = {};

	// Multisampling
	AllocatedImage m_ColorImage    = {};
	vk::ImageView m_ColorImageView = {};

	// RenderPass
	vk::RenderPass m_RenderPass = {};

	vk::SampleCountFlagBits m_SampleCount = vk::SampleCountFlagBits::e1;
	// Commands
	vk::CommandPool m_CommandPool                   = {};
	std::vector<vk::CommandBuffer> m_CommandBuffers = {};

	// Synchronization
	std::vector<vk::Semaphore> m_AquireImageSemaphores = {};
	std::vector<vk::Semaphore> m_RenderSemaphores      = {};
	std::vector<vk::Fence> m_FrameFences               = {};

	const uint32_t m_MaxFramesInFlight = 2u;
	uint32_t m_CurrentFrame            = 0u;

	// Descriptor sets
	vk::DescriptorPool m_DescriptorPool             = {};
	std::vector<vk::DescriptorSet> m_DescriptorSets = {};

	// Depth buffer
	vk::Format m_DepthFormat    = {};
	AllocatedImage m_DepthImage = {};

	vk::ImageView m_DepthImageView = {};

	// Pipelines
	std::vector<std::shared_ptr<Pipeline>> m_Pipelines = {};
	std::shared_ptr<Texture> m_StatueTexture           = {}; // #TEMP
};
