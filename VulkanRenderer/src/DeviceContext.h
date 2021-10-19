#pragma once

#include "Base.h"

#include "SharedContext.h"

#include <volk.h>
#include <glfw/glfw3.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

class DeviceContext
{
private:
	// window
	GLFWwindow* m_WindowHandle;
	VkSurfaceKHR m_Surface;

	// instance
	VkInstance m_VkInstance;

	// device
	VkPhysicalDevice m_PhysicalDevice;
	VkDevice m_LogicalDevice;

	SharedContext m_SharedContext;

	// queue
	QueueFamilyIndices m_QueueFamilyIndices;

	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	// validation layers & extensions
	std::vector<const char*> m_ValidationLayers;
	std::vector<const char*> m_GlobalExtensions;
	std::vector<const char*> m_DeviceExtensions;

public:
	DeviceContext(GLFWwindow* windowHandle);
	~DeviceContext();

	inline SharedContext GetSharedContext() { return m_SharedContext; }

private:
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateWindowSurface(GLFWwindow* windowHandle);

	void FilterValidationLayers();
	void FetchGlobalExtensions();
	void FetchDeviceExtensions();
	void FetchSupportedQueueFamilies();

	VkDebugUtilsMessengerCreateInfoEXT SetupDebugMessageCallback();
};