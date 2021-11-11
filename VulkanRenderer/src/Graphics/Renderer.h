#pragma once

#include "Core/Base.h"

#include "Graphics/DeviceContext.h"
#include "Graphics/Buffers.h"

#include <glm/glm.hpp>

#include <volk.h>

struct GLFWwindow;
class Shader;

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;

	std::vector<uint32_t> indices;

	inline bool IsSuitableForRendering() const { return graphics.has_value() && present.has_value(); }

	operator bool() const { return IsSuitableForRendering(); };
};

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;

	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	inline bool IsSuitableForRendering() const { !formats.empty() && !presentModes.empty(); }

	operator bool() const { return IsSuitableForRendering(); }
};

class Renderer
{
private:
	// window
	GLFWwindow* m_WindowHandle;
	VkSurfaceKHR m_Surface;

	// device
	DeviceContext m_DeviceContext;
	VkInstance m_VkInstance;
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_LogicalDevice;

	// queue
	QueueFamilyIndices m_QueueFamilyIndices;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	// validation layers & extensions
	std::vector<const char*> m_ValidationLayers;
	std::vector<const char*> m_RequiredExtensions;
	std::vector<const char*> m_LogicalDeviceExtensions;

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

	// aka - framebuffer resized
	bool m_SwapchainInvalidated;

	// command pool
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	// synchronization
	const uint32_t m_FramesInFlight;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_Fences;
	std::vector<VkFence> m_ImagesInFlight;
	size_t m_CurrentFrame;

	// shaders
	std::unique_ptr<Shader> m_ShaderTriangle;

	// buffers
	std::unique_ptr<Buffer> m_VertexBuffer;
	std::unique_ptr<Buffer> m_VertexStagingBuffer;

	std::unique_ptr<Buffer> m_IndexBuffer;
	std::unique_ptr<Buffer> m_IndexStagingBuffer;

public:
	Renderer(GLFWwindow* windowHandle, uint32_t frames = 2u);
	~Renderer();

	void RenderFrame();

	inline void InvalidateSwapchain() { m_SwapchainInvalidated = true; }

private:
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateWindowSurface(GLFWwindow* windowHandle);

	void CreateSwapchain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreatePipelineLayout();
	void CreatePipeline(std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages);
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronizations();

	void RecreateSwapchain();

	void DestroySwapchain();

	void FilterValidationLayers();
	void FetchRequiredExtensions();
	void FetchLogicalDeviceExtensions();
	void FetchSupportedQueueFamilies();
	void FetchSwapchainSupportDetails();
	uint32_t FetchMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

	VkDebugUtilsMessengerCreateInfoEXT SetupDebugMessageCallback();
};