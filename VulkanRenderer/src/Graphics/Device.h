#pragma once

#include "Core/Base.h"

#include <volk.h>

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;

	std::vector<uint32_t> indices;

	inline bool IsSuitableForRendering() const { return graphics.has_value() && present.has_value(); }

	operator bool() const { return IsSuitableForRendering(); };
};

class Device
{
public:
	Device(class Window* window);
	~Device();

	// Not copyable/movable
	Device(Device&&)      = delete;
	Device(const Device&) = delete;
	Device operator=(Device&&) = delete;
	Device operator=(const Device&) = delete;

	// NOTE: We would normally use CamelCase for functions, but since
	//       these getters are frequently used, I wrote them like
	//       struct member variables.
	//
	// Getters
	inline VkDevice logical() { return m_LogicalDevice; }
	inline VkPhysicalDevice physical() { return m_PhysicalDevice; }
	inline VkSurfaceKHR surface() { return m_Surface; }

	inline VkSampleCountFlagBits sampleCount() { return m_MSAASamples; }

	// Getters - Queue
	inline QueueFamilyIndices queueIndices() const { return m_QueueFamilyIndices; }
	inline uint32_t graphicsQueueIndex() const { return m_QueueFamilyIndices.graphics.value(); }
	inline uint32_t presentQueueIndex() const { return m_QueueFamilyIndices.present.value(); }
	inline VkQueue graphicsQueue() const { return m_GraphicsQueue; }
	inline VkQueue presentQueue() const { return m_PresentQueue; }

private:
	// window
	class Window* m_Window;

	// device
	VkInstance m_VkInstance = VK_NULL_HANDLE;

	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_LogicalDevice          = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface            = VK_NULL_HANDLE;

	// queue
	QueueFamilyIndices m_QueueFamilyIndices;
	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue  = VK_NULL_HANDLE;

	// validation layers & extensions
	std::vector<const char*> m_ValidationLayers        = {};
	std::vector<const char*> m_RequiredExtensions      = {};
	std::vector<const char*> m_LogicalDeviceExtensions = {};

	VkSampleCountFlagBits m_MSAASamples = VK_SAMPLE_COUNT_1_BIT;

private:
	void CreateVkInstance(VkDebugUtilsMessengerCreateInfoEXT debugMessageCreateInfo);
	void CreateWindowSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();

	void FilterValidationLayers();
	void FetchRequiredExtensions();
	void FetchSupportedQueueFamilies();
	void FetchMaxUsableSampleCount();


	VkDebugUtilsMessengerCreateInfoEXT SetupDebugMessageCallback();
};