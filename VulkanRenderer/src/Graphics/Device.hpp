#pragma once

#include "Core/Base.hpp"

#include <volk.h>

class Window;

struct DeviceCreateInfo
{
	std::vector<const char*> layers;
	std::vector<const char*> extensions;

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

	// Queues
	uint32_t m_GraphicsQueueIndex = UINT32_MAX;
	uint32_t m_PresentQueueIndex  = UINT32_MAX;

	// Layers & Extensions
	std::vector<const char*> m_Layers;
	std::vector<const char*> m_Extensions;
};
