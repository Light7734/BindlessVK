#pragma once

#include "Core/Base.hpp"
#include "Graphics/Model.hpp"
#include "Graphics/Pipeline.hpp"
#include "Graphics/Texture.hpp"

#include <volk.h>

#include <glm/glm.hpp>

class Window;

struct UniformMVP
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct SurfaceInfo
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;

	std::vector<VkSurfaceFormatKHR> supportedFormats;
	std::vector<VkPresentModeKHR> supportedPresentModes;
};

struct QueueInfo
{
	// WARN!: These should be coherent in memory
	uint32_t graphicsQueueIndex;
	uint32_t presentQueueIndex;

	VkQueue graphicsQueue;
	VkQueue presentQueue;
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

	void LogDebugInfo();

	SurfaceInfo FetchSurfaceInfo();

	void DrawFrame();

	inline QueueInfo GetQueueInfo() const { return m_QueueInfo; }

	inline VkDevice GetLogicalDevice() const { return m_LogicalDevice; }
	inline VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
	inline VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const { return m_PhysicalDeviceProperties; }
	inline VkSampleCountFlagBits GetMaxSupportedSampleCount() const { return m_MaxSupportedSampleCount; }

private:
	// Instance
	VkInstance m_Instance = VK_NULL_HANDLE;

	// Layers & Extensions
	std::vector<const char*> m_Layers;
	std::vector<const char*> m_Extensions;

	// Device
	VkDevice m_LogicalDevice                              = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice                     = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties = {};
	VkSampleCountFlagBits m_MaxSupportedSampleCount       = VK_SAMPLE_COUNT_1_BIT;

	// Queue & Surface
	QueueInfo m_QueueInfo;
	SurfaceInfo m_SurfaceInfo;
};
