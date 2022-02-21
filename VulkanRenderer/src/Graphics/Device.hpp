#pragma once

#include "Core/Base.hpp"

#include <volk.h>

class Window;

struct DeviceCreateInfo
{
	std::vector<const char*> layers;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> deviceExtensions;

	bool enableDebugging;
	VkDebugUtilsMessageSeverityFlagBitsEXT minMessageSeverity;
	VkDebugUtilsMessageTypeFlagsEXT messageTypes;
};

class Device
{
public:
	Device(DeviceCreateInfo& createInfo, Window& window);
	~Device();

	inline VkDevice GetLogicalDevice() const { return m_LogicalDevice; }

private:
	// Instance
	VkInstance m_Instance = VK_NULL_HANDLE;

	// Layers & Extensions
	std::vector<const char*> m_Layers;
	std::vector<const char*> m_Extensions;

	// Device
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties = {};

	VkDevice m_LogicalDevice = VK_NULL_HANDLE;

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
};
