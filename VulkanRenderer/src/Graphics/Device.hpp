#pragma once

#include "Core/Base.hpp"

#include <volk.h>

class Window;

struct DeviceCreateInfo
{
	std::vector<const char*> layers;
	std::vector<const char*> extensions;
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

private:
	// Instance
	VkInstance m_Instance = VK_NULL_HANDLE;

	// Device
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties = {};

	VkDevice m_LogicalDevice = VK_NULL_HANDLE;

	// Surface
	VkSurfaceKHR m_Surface;

	// Queues #WARNING: Don't change the order of these 2 variables, they should be coherent in the memory
	uint32_t m_GraphicsQueueIndex = UINT32_MAX;
	uint32_t m_PresentQueueIndex  = UINT32_MAX;

	// Swap chain
	VkSwapchainKHR m_Swapchain;
	std::vector<VkImage> m_SwapchainImages;

	VkSurfaceFormatKHR m_SwapchainFormat;
	VkPresentModeKHR m_SwapchainPresentMode;
	VkExtent2D m_SwapchainExtent;

	VkSurfaceCapabilitiesKHR m_SwapchainCapabilities;
	std::vector<VkSurfaceFormatKHR> m_SupportedSwapchainFormats;
	std::vector<VkPresentModeKHR> m_SupportedSwapchainPresentModes;

	// Layers & Extensions
	std::vector<const char*> m_Layers;
	std::vector<const char*> m_Extensions;
};
