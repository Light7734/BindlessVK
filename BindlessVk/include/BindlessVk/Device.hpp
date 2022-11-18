#pragma once

#include "BindlessVk/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Device
{
	struct CreateInfo
	{
		std::vector<const char*> layers;
		std::vector<const char*> instanceExtensions;
		std::vector<const char*> deviceExtensions;
		std::vector<const char*> windowExtensions;

		std::function<vk::SurfaceKHR(vk::Instance)> createWindowSurfaceFunc;

		bool enableDebugging;
		vk::DebugUtilsMessageSeverityFlagsEXT debugMessengerSeverities;
		vk::DebugUtilsMessageTypeFlagsEXT debugMessengerTypes;
		PFN_vkDebugUtilsMessengerCallbackEXT debugMessengerCallback;
		void* debugMessengerUserPointer;
	};

	// Layers & Extensions
	std::vector<const char*> layers;
	std::vector<const char*> instanceExtensions;
	std::vector<const char*> deviceExtensions;
	std::vector<const char*> windowExtensions;

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
	DeviceSystem() = default;
	void Init(const Device::CreateInfo& info);

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
	Device m_Device = {};

	vk::DynamicLoader m_DynamicLoader;
	vk::DebugUtilsMessengerEXT m_DebugUtilMessenger = {};
};

} // namespace BINDLESSVK_NAMESPACE
