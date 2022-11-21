#define VMA_IMPLEMENTATION

#include "BindlessVk/Device.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace BINDLESSVK_NAMESPACE {


/// Callback function called after successful vkAllocateMemory.
void vmaAllocateDeviceMemoryFunction(
    VmaAllocator VMA_NOT_NULL allocator,
    uint32_t memoryType,
    VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
    VkDeviceSize size,
    void* VMA_NULLABLE pUserData)
{
	BVK_LOG(LogLvl::eTrace, "Allocate device memory -> {}", size);
}

/// Callback function called before vkFreeMemory.
void vmaFreeDeviceMemoryFunction(
    VmaAllocator VMA_NOT_NULL allocator,
    uint32_t memoryType,
    VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE memory,
    VkDeviceSize size,
    void* VMA_NULLABLE pUserData)
{
	BVK_LOG(LogLvl::eTrace, "Free device memory -> {}", size);
}


void DeviceSystem::Init(const Device::CreateInfo& info)
{
	m_Device.layers             = info.layers;
	m_Device.instanceExtensions = info.instanceExtensions;
	m_Device.deviceExtensions   = info.deviceExtensions;
	m_Device.windowExtensions   = info.windowExtensions;

	// Initialize default dispatcher
	m_Device.vkGetInstanceProcAddr = m_DynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device.vkGetInstanceProcAddr);

	CheckLayerSupport();
	CreateVulkanInstance(info);
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateAllocator();
	UpdateSurfaceInfo();
}

void DeviceSystem::Reset()
{
	m_Device.logical.waitIdle();
	m_Device.allocator.destroy();

	m_Device.logical.destroy(nullptr);

	m_Device.instance.destroySurfaceKHR(m_Device.surface, nullptr);

	m_Device.instance.destroyDebugUtilsMessengerEXT(m_DebugUtilMessenger);
	m_Device.instance.destroy();
}

void DeviceSystem::CheckLayerSupport()
{
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
	BVK_ASSERT(availableLayers.empty(), "No instance layer found");

	// Check if we support all the required layers
	for (const char* requiredLayerName : m_Device.layers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(requiredLayerName, layerProperties.layerName))
			{
				layerFound = true;
				break;
			}
		}

		BVK_ASSERT(!layerFound, "Required layer not found");
	}
}

void DeviceSystem::CreateVulkanInstance(const Device::CreateInfo& info)
{
	vk::ApplicationInfo applicationInfo {
		"BindlessVK",             // pApplicationName
		VK_MAKE_VERSION(1, 0, 0), // applicationVersion
		"BindlessVK",             // pEngineName
		VK_MAKE_VERSION(1, 0, 0), // engineVersion
		VK_API_VERSION_1_3,       // apiVersion
	};

	vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
		{},                            // flags
		info.debugMessengerSeverities, // messageSeverity
		info.debugMessengerTypes,      // messageType
		info.debugMessengerCallback,
		info.debugMessengerUserPointer,
	};

	// If debugging is not enabled, remove validation layer from instance layers
	if (!info.enableDebugging)
	{
		auto validationLayerIt = std::find(m_Device.layers.begin(), m_Device.layers.end(), "VK_LAYER_KHRONOS_validation");
		if (validationLayerIt != m_Device.layers.end())
		{
			m_Device.layers.erase(validationLayerIt);
		}
	}

	// Create the vulkan instance
	std::vector<const char*> windowExtensions = info.windowExtensions;
	m_Device.instanceExtensions.insert(m_Device.instanceExtensions.end(), windowExtensions.begin(), windowExtensions.end());

	vk::InstanceCreateInfo instanceCreateInfo {
		{},                                                        // flags
		&applicationInfo,                                          // pApplicationInfo
		static_cast<uint32_t>(m_Device.layers.size()),             // enabledLayerCount
		m_Device.layers.data(),                                    // ppEnabledLayerNames
		static_cast<uint32_t>(m_Device.instanceExtensions.size()), // enabledExtensionCount
		m_Device.instanceExtensions.data(),                        // ppEnabledExtensionNames
	};

	BVK_ASSERT(vk::createInstance(&instanceCreateInfo, nullptr, &m_Device.instance));
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device.instance);
	m_Device.surface = info.createWindowSurfaceFunc(m_Device.instance);


	m_DebugUtilMessenger = m_Device.instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
}

void DeviceSystem::PickPhysicalDevice()
{
	// Fetch physical devices with vulkan support
	std::vector<vk::PhysicalDevice> devices = m_Device.instance.enumeratePhysicalDevices();
	BVK_ASSERT(devices.empty(), "No physical device with vulkan support found");

	// Select the most suitable physical device
	uint32_t highScore = 0u;
	for (const auto& device : devices)
	{
		uint32_t score = 0u;

		// Fetch properties & features
		vk::PhysicalDeviceProperties properties = device.getProperties();
		vk::PhysicalDeviceFeatures features     = device.getFeatures();

		// Check if device supports required features
		if (!features.geometryShader || !features.samplerAnisotropy)
			continue;

		/** Check if the device supports the required queues **/
		{
			// Fetch queue families
			std::vector<vk::QueueFamilyProperties> queueFamiliesProperties = device.getQueueFamilyProperties();
			if (queueFamiliesProperties.empty())
				continue;

			uint32_t index = 0u;
			for (const auto& queueFamilyProperties : queueFamiliesProperties)
			{
				// Check if current queue index supports any or all desired queues
				if (queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
				{
					m_Device.graphicsQueueIndex = index;
				}

				if (device.getSurfaceSupportKHR(index, m_Device.surface))
				{
					m_Device.presentQueueIndex = index;
				}

				index++;

				// Device does supports all the required queues!
				if (m_Device.graphicsQueueIndex != UINT32_MAX && m_Device.presentQueueIndex != UINT32_MAX)
				{
					break;
				}
			}

			// Device doesn't support all the required queues!
			if (m_Device.graphicsQueueIndex == UINT32_MAX || m_Device.presentQueueIndex == UINT32_MAX)
			{
				m_Device.graphicsQueueIndex = UINT32_MAX;
				m_Device.presentQueueIndex  = UINT32_MAX;
				continue;
			}
		}

		/** Check if device supports required extensions **/
		{
			// Fetch extensions

			std::vector<vk::ExtensionProperties> deviceExtensionsProperties = device.enumerateDeviceExtensionProperties();
			if (deviceExtensionsProperties.empty())
				continue;

			bool failed = false;
			for (const auto& requiredExtensionName : m_Device.deviceExtensions)
			{
				bool found = false;
				for (const auto& extension : deviceExtensionsProperties)
				{
					if (!strcmp(requiredExtensionName, extension.extensionName))
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					failed = true;
					break;
				}
			}

			if (failed)
				continue;
		}

		/** Check if surface is adequate **/
		{
			m_Device.surfaceCapabilities = device.getSurfaceCapabilitiesKHR(m_Device.surface);

			if (device.getSurfaceFormatsKHR(m_Device.surface).empty())
				continue;

			if (device.getSurfacePresentModesKHR(m_Device.surface).empty())
				continue;
		}

		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 69420; // nice

		score += properties.limits.maxImageDimension2D;

		m_Device.physical = score > highScore ? device : m_Device.physical;
	}
	BVK_ASSERT(!m_Device.physical, "No suitable physical device found");

	// Cache the selected physical device's properties
	m_Device.properties = m_Device.physical.getProperties();

	// Determine max sample counts
	vk::Flags<vk::SampleCountFlagBits> color      = m_Device.properties.limits.framebufferColorSampleCounts;
	vk::Flags<vk::SampleCountFlagBits> depth      = m_Device.properties.limits.framebufferDepthSampleCounts;
	vk::Flags<vk::SampleCountFlagBits> depthColor = color & depth;

	m_Device.maxColorSamples =
	    color & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	    color & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	    color & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	    color & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	    color & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	    color & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                           vk::SampleCountFlagBits::e1;
	m_Device.maxDepthSamples =
	    depth & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	    depth & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	    depth & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	    depth & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	    depth & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	    depth & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                           vk::SampleCountFlagBits::e1;
	m_Device.maxDepthColorSamples =
	    depthColor & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	    depthColor & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	    depthColor & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	    depthColor & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	    depthColor & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	    depthColor & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                vk::SampleCountFlagBits::e1;

	// Select depth format
	// bool hasStencilComponent = false;
	vk::FormatProperties formatProperties;
	for (vk::Format format : { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint })
	{
		formatProperties = m_Device.physical.getFormatProperties(format);
		if ((formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			m_Device.depthFormat = format;
			m_Device.hasStencil  = format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
		}
	}
}

void DeviceSystem::CreateLogicalDevice()
{
	std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfo;

	float queuePriority = 1.0f; // Always 1.0
	queuesCreateInfo.push_back({
	    {},                          // flags
	    m_Device.graphicsQueueIndex, // queueFamilyIndex
	    1u,                          // queueCount
	    &queuePriority,              // pQueuePriorities
	});

	if (m_Device.presentQueueIndex != m_Device.graphicsQueueIndex)
	{
		queuesCreateInfo.push_back({
		    {},                         // flags
		    m_Device.presentQueueIndex, // queueFamilyIndex
		    1u,                         // queueCount
		    &queuePriority,             // pQueuePriorities
		});
	}

	vk::PhysicalDeviceFeatures physicalDeviceFeatures {
		VK_FALSE, // robustBufferAccess
		VK_FALSE, // fullDrawIndexUint32
		VK_FALSE, // imageCubeArray
		VK_FALSE, // independentBlend
		VK_FALSE, // geometryShader
		VK_FALSE, // tessellationShader
		VK_FALSE, // sampleRateShading
		VK_FALSE, // dualSrcBlend
		VK_FALSE, // logicOp
		VK_FALSE, // multiDrawIndirect
		VK_FALSE, // drawIndirectFirstInstance
		VK_FALSE, // depthClamp
		VK_FALSE, // depthBiasClamp
		VK_FALSE, // fillModeNonSolid
		VK_FALSE, // depthBounds
		VK_FALSE, // wideLines
		VK_FALSE, // largePoints
		VK_FALSE, // alphaToOne
		VK_FALSE, // multiViewport
		VK_TRUE,  // samplerAnisotropy
		VK_FALSE, // textureCompressionETC2
		VK_FALSE, // textureCompressionASTC
		VK_FALSE, // textureCompressionBC
		VK_FALSE, // occlusionQueryPrecise
		VK_FALSE, // pipelineStatisticsQuery
		VK_FALSE, // vertexPipelineStoresAndAtomics
		VK_FALSE, // fragmentStoresAndAtomics
		VK_FALSE, // shaderTessellationAndGeometryPointSize
		VK_FALSE, // shaderImageGatherExtended
		VK_FALSE, // shaderStorageImageExtendedFormats
		VK_FALSE, // shaderStorageImageMultisample
		VK_FALSE, // shaderStorageImageReadWithoutFormat
		VK_FALSE, // shaderStorageImageWriteWithoutFormat
		VK_FALSE, // shaderUniformBufferArrayDynamicIndexing
		VK_FALSE, // shaderSampledImageArrayDynamicIndexing
		VK_FALSE, // shaderStorageBufferArrayDynamicIndexing
		VK_FALSE, // shaderStorageImageArrayDynamicIndexing
		VK_FALSE, // shaderClipDistance
		VK_FALSE, // shaderCullDistance
		VK_FALSE, // shaderFloat64
		VK_FALSE, // shaderInt64
		VK_FALSE, // shaderInt16
		VK_FALSE, // shaderResourceResidency
		VK_FALSE, // shaderResourceMinLod
		VK_FALSE, // sparseBinding
		VK_FALSE, // sparseResidencyBuffer
		VK_FALSE, // sparseResidencyImage2D
		VK_FALSE, // sparseResidencyImage3D
		VK_FALSE, // sparseResidency2Samples
		VK_FALSE, // sparseResidency4Samples
		VK_FALSE, // sparseResidency8Samples
		VK_FALSE, // sparseResidency16Samples
		VK_FALSE, // sparseResidencyAliased
		VK_FALSE, // variableMultisampleRate
		VK_FALSE, // inheritedQueries
	};

	vk::PhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures {
		true, // dynamicRendering
	};

	vk::DeviceCreateInfo logicalDeviceCreateInfo {
		{},                                                      // flags
		static_cast<uint32_t>(queuesCreateInfo.size()),          // queueCreateInfoCount
		queuesCreateInfo.data(),                                 // pQueueCreateInfos
		0u,                                                      // enabledLayerCount
		nullptr,                                                 // ppEnabledLayerNames
		static_cast<uint32_t>(m_Device.deviceExtensions.size()), // enabledExtensionCount
		m_Device.deviceExtensions.data(),                        // ppEnabledExtensionNames
		&physicalDeviceFeatures,                                 // pEnabledFeatures
		&dynamicRenderingFeatures,
	};

	m_Device.logical = m_Device.physical.createDevice(logicalDeviceCreateInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Device.logical);

	m_Device.graphicsQueue = m_Device.logical.getQueue(m_Device.graphicsQueueIndex, 0u);
	m_Device.presentQueue  = m_Device.logical.getQueue(m_Device.presentQueueIndex, 0u);

	BVK_ASSERT(!m_Device.graphicsQueue, "Failed to fetch graphics queue");
	BVK_ASSERT(!m_Device.presentQueue, "Failed to fetch present queue");
}

void DeviceSystem::CreateAllocator()
{
	vma::VulkanFunctions vulkanFunctions(
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceBufferMemoryRequirements,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceImageMemoryRequirements);

	vma::DeviceMemoryCallbacks memCallbacks = {

		&vmaAllocateDeviceMemoryFunction,
		&vmaFreeDeviceMemoryFunction,
		nullptr,
	};

	vma::AllocatorCreateInfo allocCreateInfo(
	    {},                // flags
	    m_Device.physical, // physicalDevice
	    m_Device.logical,  // device
	    {},                // preferredLargeHeapBlockSize
	    {},                // pAllocationCallbacks
	    &memCallbacks,
	    {},                // pHeapSizeLimit
	    &vulkanFunctions,  // pVulkanFunctions
	    m_Device.instance, // instance
	    {}                 // vulkanApiVersion
	);


	m_Device.allocator = vma::createAllocator(allocCreateInfo);
}

void DeviceSystem::UpdateSurfaceInfo()
{
	m_Device.surfaceCapabilities = m_Device.physical.getSurfaceCapabilitiesKHR(m_Device.surface);

	std::vector<vk::SurfaceFormatKHR> supportedFormats    = m_Device.physical.getSurfaceFormatsKHR(m_Device.surface);
	std::vector<vk::PresentModeKHR> supportedPresentModes = m_Device.physical.getSurfacePresentModesKHR(m_Device.surface);

	// Select surface format
	m_Device.surfaceFormat = supportedFormats[0]; // default
	for (const auto& surfaceFormat : supportedFormats)
	{
		if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			m_Device.surfaceFormat = surfaceFormat;
			break;
		}
	}

	// Select present mode
	m_Device.presentMode = supportedPresentModes[0]; // default
	for (const auto& presentMode : supportedPresentModes)
	{
		if (presentMode == vk::PresentModeKHR::eFifo)
		{
			m_Device.presentMode = presentMode;
		}
	}

	m_Device.framebufferExtent = std::clamp(m_Device.framebufferExtent, m_Device.surfaceCapabilities.minImageExtent, m_Device.surfaceCapabilities.maxImageExtent);
}

} // namespace BINDLESSVK_NAMESPACE
