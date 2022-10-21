#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "Core/Base.hpp"
#include "Graphics/Buffer.hpp"
#include "Graphics/Device.hpp"
#include "Graphics/Model.hpp"
#include "Scene/Camera.hpp"
// #include "Graphics/Pipeline.hpp"
#include "Graphics/Texture.hpp"
#include "Graphics/Types.hpp"

#include <functional>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif

class Window;

struct CameraData
{
	std::unique_ptr<Buffer> buffer;
	glm::mat4 projection;
	glm::mat4 view;
	glm::vec4 lightPos;
	glm::vec4 viewPos;
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

	std::array<CmdBuffers, MAX_FRAMES_IN_FLIGHT> cmdBuffers; // Command buffers for each frame

	std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets; // Binding 1

	vk::CommandPool cmdPool; // Pools for threads @todo Multi-threading

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;

	std::unique_ptr<Buffer> storageBuffer;
};

class Renderer
{
public:
	struct CreateInfo
	{
		DeviceContext deviceContext;
		Window* window;
		Texture* defaultTexture;
		Texture* skyboxTexture;
	};

public:
	Renderer(const Renderer::CreateInfo& createInfo);
	~Renderer();

	void RecreateSwapchainResources(Window* window, DeviceContext context);
	void DestroySwapchain();

	void BeginFrame();
	void Draw();
	void DrawScene(class Scene* scene, const Camera& camera);

	void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& function);

	inline QueueInfo GetQueueInfo() const { return m_QueueInfo; }

	inline vk::CommandPool GetCommandPool() const { return m_CommandPool; }
	inline uint32_t GetImageCount() const { return m_Images.size(); }

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
	void CreateImageViews();
	void CreateResolveColorImage();
	void CreateDepthImage();
	void CreateCommandPool();
	void CreateDescriptorResources(vk::PhysicalDevice physicalDevice);
	void CreateFramePass();
	void CreateForwardPass(const DeviceContext& deviceContext);
	void CreateUIPass(Window* window, const DeviceContext& deviceContext);

	void UpdateFrameDescriptorSet(const FrameData& frame, const Camera& camera);
	void UpdateForwardPassDescriptorSet(Scene* scene, uint32_t frameIndex);

	void SetupRenderBarriers(vk::CommandBuffer cmd, uint32_t imageIndex);
	void SetupPresentBarriers(vk::CommandBuffer cmd, uint32_t imageIndex);
	void RenderForwardPass(Scene* scene, vk::CommandBuffer cmd, uint32_t imageIndex);
	void RenderUIPass(vk::CommandBuffer cmd, uint32_t imageIndex);

	void SubmitQueue(const FrameData& frame, vk::CommandBuffer cmd);
	void PresentFrame(const FrameData& frame, uint32_t imageIndex);

	void WriteDescriptorSetsDefaultValues();

private:
	UploadContext m_UploadContext = {};
	vk::Device m_LogicalDevice    = {};
	QueueInfo m_QueueInfo         = {};
	SurfaceInfo m_SurfaceInfo     = {};
	vk::Format m_DepthFormat      = {};

	vma::Allocator m_Allocator = {};

	std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames = {};
	vk::DescriptorSetLayout m_FramesDescriptorSetLayout  = {};

	// Render Passes
	RenderPassData m_ForwardPass = {};

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

	Texture* m_DefaultTexture = {};
	Texture* m_SkyboxTexture  = {};

	// Descriptor sets
	vk::DescriptorPool m_DescriptorPool = {};

	// Depth buffer
	AllocatedImage m_DepthImage = {};

	vk::ImageView m_DepthImageView = {};

	// Pipelines
	vk::PipelineLayout m_FramePipelineLayout = {};
};
