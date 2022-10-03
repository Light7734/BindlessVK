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

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

class Window;

struct RendererCreateInfo
{
	Window* window;
	DeviceContext deviceContext;
};

struct CameraData
{
	std::unique_ptr<Buffer> buffer;
	glm::mat4 projection;
	glm::mat4 view;
};

struct UploadContext
{
	vk::CommandBuffer cmdBuffer;
	vk::CommandPool cmdPool;
	vk::Fence fence;
};

struct FrameData
{
	vk::Semaphore renderSemaphore;
	vk::Semaphore presentSemaphore;
	vk::Fence renderFence;

	CameraData cameraData;

	vk::DescriptorSet descriptorSet; // Binding 0
};

// @todo Multi-threading
struct RenderPassData
{
	struct CmdBuffers
	{
		vk::CommandBuffer primary;
		std::vector<vk::CommandBuffer> secondaries; // secondary command buffers for each thread
	};

	vk::RenderPass renderpass;
	std::vector<vk::Framebuffer> framebuffers;

	vk::CommandPool cmdPool;                                 // Pools for threads @todo Multi-threading
	std::array<CmdBuffers, MAX_FRAMES_IN_FLIGHT> cmdBuffers; // Command buffers for each frame

	std::unique_ptr<Buffer> storageBuffer;

	std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets; // Binding 1
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;
};

class Renderer
{
public:
	Renderer(const RendererCreateInfo& createInfo);
	~Renderer();

	void BeginFrame();
	void Draw();
	void EndFrame();

	void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function);

	inline vk::CommandPool GetCommandPool() const { return m_CommandPool; }
	inline uint32_t GetImageCount() const { return m_Images.size(); }

	inline bool IsSwapchainInvalidated() const { return m_SwapchainInvalidated; }

private:
	vk::Device m_LogicalDevice = {};
	QueueInfo m_QueueInfo      = {};
	SurfaceInfo m_SurfaceInfo  = {};

	vma::Allocator m_Allocator;

	std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames = {};
	vk::DescriptorSetLayout m_FramesDescriptorSetLayout;

	// Render Passes
	RenderPassData m_ForwardPass = {};
	// RenderPassData m_UIPass;

	bool m_SwapchainInvalidated = false;

	// Swapchain
	vk::SwapchainKHR m_Swapchain = {};

	std::vector<vk::Image> m_Images         = {};
	std::vector<vk::ImageView> m_ImageViews = {};

	// Multisampling
	AllocatedImage m_ColorImage    = {};
	vk::ImageView m_ColorImageView = {};

	vk::SampleCountFlagBits m_SampleCount = vk::SampleCountFlagBits::e1;

	// Commands
	vk::CommandPool m_CommandPool                   = {};
	std::vector<vk::CommandBuffer> m_CommandBuffers = {};

	uint32_t m_CurrentFrame = 0ul;

	// Descriptor sets
	vk::DescriptorPool m_DescriptorPool = {};

	// Depth buffer
	vk::Format m_DepthFormat    = {};
	AllocatedImage m_DepthImage = {};

	vk::ImageView m_DepthImageView = {};

	// ImGui
	vk::DescriptorPool m_ImguiPool                   = {};
	std::vector<vk::CommandBuffer> m_ImguiCmdBuffers = {};

	UploadContext m_UploadContext = {};

	// Pipelines
	std::vector<std::shared_ptr<Pipeline>> m_Pipelines = {};
	vk::PipelineLayout m_FramePipelineLayout           = {};
	std::shared_ptr<Texture> m_TempTexture             = {}; // #TEMP
	                                                         //
};
