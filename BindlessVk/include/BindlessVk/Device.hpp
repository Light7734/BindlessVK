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
		std::function<vk::Extent2D()> getWindowFramebufferExtentFunc;

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


	/** Native platform surface */
	vk::SurfaceKHR surface;

	/** Surface capabilities */
	vk::SurfaceCapabilitiesKHR surfaceCapabilities;

	/** Chosen surface format */
	vk::SurfaceFormatKHR surfaceFormat;

	/** Chosen surface present mode */
	vk::PresentModeKHR presentMode;

	/** */
	std::function<vk::Extent2D()> getWindowFramebufferExtentFunc;

	/** */
	vk::Extent2D framebufferExtent;

	/** Vulkan memory allocator */
	vma::Allocator allocator;

	/** Max number of recording threads */
	uint32_t numThreads;

	/** A command pool for every (thread * frame) */
	std::vector<vk::CommandPool> dynamicCmdPools;

	/** Command pool for immediate submission */
	vk::CommandPool immediateCmdPool;

	vk::CommandBuffer immediateCmd;
	vk::Fence immediateFence;

	vk::CommandPool GetCmdPool(uint32_t frameIndex, uint32_t threadIndex = 0)
	{
		return dynamicCmdPools[(threadIndex * numThreads) + frameIndex];
	}

	void ImmediateSubmit(std::function<void(vk::CommandBuffer)>&& func)
	{
		vk::CommandBufferAllocateInfo allocInfo {
			immediateCmdPool,
			vk::CommandBufferLevel::ePrimary,
			1ul,
		};

		vk::CommandBuffer cmd = logical.allocateCommandBuffers(allocInfo)[0];

		vk::CommandBufferBeginInfo beginInfo {
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		};

		cmd.begin(beginInfo);
		func(cmd);
		cmd.end();

		vk::SubmitInfo submitInfo { 0u, {}, {}, 1u, &cmd, 0u, {}, {} };
		graphicsQueue.submit(submitInfo, immediateFence);

		BVK_ASSERT(logical.waitForFences(immediateFence, true, UINT_MAX));
		logical.resetFences(immediateFence);
		logical.resetCommandPool(immediateCmdPool);
	}
};

class DeviceSystem
{
public:
	DeviceSystem() = default;
	void Init(const Device::CreateInfo& info);

	void Reset();

	void UpdateSurfaceInfo();

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
	void CreateCommandPools();

private:
	Device m_Device = {};

	vk::DynamicLoader m_DynamicLoader;
	vk::DebugUtilsMessengerEXT m_DebugUtilMessenger = {};
};

} // namespace BINDLESSVK_NAMESPACE
