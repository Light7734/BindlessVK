#pragma once

#include "Core/Base.hpp"

#include <glm/glm.hpp>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

struct Device
{
	struct CreateInfo
	{
		class Window* window;

		std::vector<const char*> layers;
		std::vector<const char*> instanceExtensions;
		std::vector<const char*> deviceExtensions;

		bool enableDebugging;
		vk::DebugUtilsMessageSeverityFlagsEXT debugMessageSeverity;
		vk::DebugUtilsMessageTypeFlagsEXT debugMessageTypes;

		std::function<VkBool32(VkDebugUtilsMessageSeverityFlagBitsEXT,
		                       VkDebugUtilsMessageTypeFlagsEXT,
		                       const VkDebugUtilsMessengerCallbackDataEXT*,
		                       void*)>
		    debugMessageCallback;
	};

	// Layers & Extensions
	std::vector<const char*> layers;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> deviceExtensions;

	// instance
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	vk::Instance instance;

	// device
	vk::Device logical;
	vk::PhysicalDevice physical;

	vk::PhysicalDeviceProperties properties;

	vk::Format depthFormat;
	bool hasStencil;

	vk::SampleCountFlagBits maxDepthColorSamples;
	vk::SampleCountFlagBits maxColorSamples;
	vk::SampleCountFlagBits maxDepthSamples;

	// queue
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;

	uint32_t graphicsQueueIndex;
	uint32_t presentQueueIndex;

	// surface
	vk::SurfaceKHR surface;
	vk::SurfaceCapabilitiesKHR surfaceCapabilities;

	vk::SurfaceFormatKHR surfaceFormat;
	vk::PresentModeKHR presentMode;

	// allocator
	vma::Allocator allocator;
};

class DeviceSystem
{
public:
	DeviceSystem(const Device::CreateInfo& info);
	~DeviceSystem();

	void DrawFrame();

	inline Device* GetDevice()
	{
		return &m_Device;
	}

private:
	void CheckLayerSupport();
	void CreateVulkanInstance(const Device::CreateInfo& info);
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateAllocator();

	void FetchSurfaceInfo();
	void FetchDepthFormat();

private:
	Device m_Device;

	vk::DynamicLoader m_DynamicLoader;
	vk::DebugUtilsMessengerEXT m_DebugUtilMessenger = {};
};
