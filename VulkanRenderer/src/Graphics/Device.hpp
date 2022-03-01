#pragma once

#include "Core/Base.hpp"
#include "Graphics/Pipeline.hpp"

#include <glm/glm.hpp>
#include <volk.h>

class Window;


struct UniformMVP
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct DeviceCreateInfo
{
	Window* window;
	std::vector<const char*> layers;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> logicalDeviceExtensions;

	bool enableDebugging;
	VkDebugUtilsMessageSeverityFlagBitsEXT debugMessageSeverity;
	VkDebugUtilsMessageTypeFlagsEXT debugMessageTypes;
};

class Device
{
public:
	Device(DeviceCreateInfo& createInfo);
	~Device();

	void DrawFrame();

	inline VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
	inline VkCommandPool GetCommandPool() const { return m_CommandPool; }
	inline uint32_t GetImageCount() const { return m_Images.size(); }

private:
	void CreateSwapchain();
	void DestroySwapchain();

private:
	// Window
	Window* m_Window;

	// Instance
	VkInstance m_Instance = VK_NULL_HANDLE;

	// Layers & Extensions
	std::vector<const char*> m_Layers;
	std::vector<const char*> m_Extensions;

	// Device
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties = {};

	VkDevice m_LogicalDevice = VK_NULL_HANDLE;

	// Queue
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	// Surface
	VkSurfaceKHR m_Surface;

	// Queues #WARNING: Don't change the order of these 2 variables, they should be coherent in the memory
	uint32_t m_GraphicsQueueIndex = UINT32_MAX;
	uint32_t m_PresentQueueIndex  = UINT32_MAX;

    // Swapchain
	VkSwapchainKHR m_Swapchain;

	VkSurfaceFormatKHR m_SurfaceFormat;
	VkPresentModeKHR m_PresentMode;
	VkExtent2D m_SwapchainExtent;

	VkSurfaceCapabilitiesKHR m_SurfcaCapabilities;
	std::vector<VkSurfaceFormatKHR> m_SupportedSurfaceFormats;
	std::vector<VkPresentModeKHR> m_SupportedPresentModes;

	// Swapcain images & framebuffers
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;

	std::vector<VkFramebuffer> m_Framebuffers;

	// RenderPass
	VkRenderPass m_RenderPass;

	// Commands
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	//Synchronization
	std::vector<VkSemaphore> m_AquireImageSemaphores;
	std::vector<VkSemaphore> m_RenderSemaphores;
	std::vector<VkFence> m_FrameFences;
	const uint32_t m_MaxFramesInFlight = 2u;
	uint32_t m_CurrentFrame            = 0u;

	// Descriptor sets
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;

	std::vector<std::unique_ptr<Buffer>> m_MVPUniBuffer;

	// Pipelines
	std::unique_ptr<Pipeline> m_TrianglePipeline;
};
