#define VMA_IMPLEMENTATION

#include "Graphics/Device.hpp"

#include "Core/Window.hpp"
#include "Utils/Timer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan_core.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

Device::Device(DeviceCreateInfo& createInfo)
    : m_Layers(createInfo.layers), m_Extensions(createInfo.instanceExtensions)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Initialize volk
	vk::DynamicLoader dl;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	/////////////////////////////////////////////////////////////////////////////////
	// Check if the required layers exist.
	{
		std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
		ASSERT(!availableLayers.empty(), "No instance layer found");

		// Check if we support all the required layers
		for (const char* requiredLayerName : m_Layers)
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

			ASSERT(layerFound, "Required layer not found");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create vulkan instance, window surface, debug messenger, and load the instace with volk.
	{
		vk::ApplicationInfo applicationInfo {
			"Vulkan Renderer",        // pApplicationName
			VK_MAKE_VERSION(1, 0, 0), // applicationVersion
			"None",                   // pEngineName
			VK_MAKE_VERSION(1, 0, 0), // engineVersion
			VK_API_VERSION_1_2,       // apiVersion
		};

		vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
			{},                              // flags
			createInfo.debugMessageSeverity, // messageSeverity
			createInfo.debugMessageTypes,    // messageType
			nullptr,                         // pUserData
		};

		// Setup validation layer message callback
		debugMessengerCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		                                              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		                                              void* pUserData) {
			LOGVk(trace, "{}", pCallbackData->pMessage); // #TODO: Format this!
			return static_cast<VkBool32>(VK_FALSE);
		};

		// If debugging is not enabled, remove validation layer from instance layers
		if (!createInfo.enableDebugging)
		{
			auto validationLayerIt = std::find(createInfo.layers.begin(), createInfo.layers.end(), "VK_LAYER_KHRONOS_validation");
			if (validationLayerIt != createInfo.layers.end())
			{
				createInfo.layers.erase(validationLayerIt);
			}
		}

		// Create the vulkan instance

		vk::InstanceCreateInfo instanceCreateInfo {
			{},                                                               // flags
			&applicationInfo,                                                 // pApplicationInfo
			static_cast<uint32_t>(m_Layers.size()),                           // enabledLayerCount
			m_Layers.data(),                                                  // ppEnabledLayerNames
			static_cast<uint32_t>(m_Extensions.size()),                       // enabledExtensionCount
			m_Extensions.data(),                                              // ppEnabledExtensionNames
			createInfo.enableDebugging ? &debugMessengerCreateInfo : nullptr, // pNext
		};

		VKC(vk::createInstance(&instanceCreateInfo, nullptr, &m_Instance));
		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_Instance);
		m_SurfaceInfo.surface = createInfo.window->CreateSurface(m_Instance);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Iterate through physical devices that supports vulkan, and pick the most suitable one
	{
		// Fetch physical devices with vulkan support
		std::vector<vk::PhysicalDevice> devices = m_Instance.enumeratePhysicalDevices();
		ASSERT(devices.size(), "No physical device with vulkan support found");

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
						m_QueueInfo.graphicsQueueIndex = index;
					}

					if (device.getSurfaceSupportKHR(index, m_SurfaceInfo.surface))
					{
						m_QueueInfo.presentQueueIndex = index;
					}

					index++;

					// Device does supports all the required queues!
					if (m_QueueInfo.graphicsQueueIndex != UINT32_MAX && m_QueueInfo.presentQueueIndex != UINT32_MAX)
					{
						break;
					}
				}

				// Device doesn't support all the required queues!
				if (m_QueueInfo.graphicsQueueIndex == UINT32_MAX || m_QueueInfo.presentQueueIndex == UINT32_MAX)
				{
					m_QueueInfo.graphicsQueueIndex = UINT32_MAX;
					m_QueueInfo.presentQueueIndex  = UINT32_MAX;
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
				for (const auto& requiredExtensionName : createInfo.logicalDeviceExtensions)
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

			/** Check if swap chain is adequate **/
			{
				// Fetch swap chain capabilities
				m_SurfaceInfo.capabilities = device.getSurfaceCapabilitiesKHR(m_SurfaceInfo.surface);

				// Fetch swap chain formats
				m_SurfaceInfo.supportedFormats = device.getSurfaceFormatsKHR(m_SurfaceInfo.surface);
				if (m_SurfaceInfo.supportedFormats.empty())
					continue;

				// Fetch swap chain present modes
				m_SurfaceInfo.supportedPresentModes = device.getSurfacePresentModesKHR(m_SurfaceInfo.surface);
				if (m_SurfaceInfo.supportedPresentModes.empty())
					continue;
			}

			if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
				score += 69420; // nice

			score += properties.limits.maxImageDimension2D;

			m_PhysicalDevice = score > highScore ? device : m_PhysicalDevice;
		}
		ASSERT(m_PhysicalDevice, "No suitable physical device found");

		// Cache the selected physical device's properties
		m_PhysicalDeviceProperties = m_PhysicalDevice.getProperties();

		// Determine max sample counts
		vk::Flags<vk::SampleCountFlagBits> counts = m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts & m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts;
		// m_MaxSupportedSampleCount = VK_SAMPLE_COUNT_1_BIT;
		m_MaxSupportedSampleCount = counts & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
		                            counts & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
		                            counts & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
		                            counts & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
		                            counts & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
		                            counts & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
		                                                                    vk::SampleCountFlagBits::e1;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the logical device.
	{
		std::vector<vk::DeviceQueueCreateInfo> queuesCreateInfo;

		float queuePriority = 1.0f; // Always 1.0
		queuesCreateInfo.push_back({
		    {},                             // flags
		    m_QueueInfo.graphicsQueueIndex, // queueFamilyIndex
		    1u,                             // queueCount
		    &queuePriority,                 // pQueuePriorities
		});

		if (m_QueueInfo.presentQueueIndex != m_QueueInfo.graphicsQueueIndex)
		{
			queuesCreateInfo.push_back({
			    {},                            // flags
			    m_QueueInfo.presentQueueIndex, // queueFamilyIndex
			    1u,                            // queueCount
			    &queuePriority,                // pQueuePriorities
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

		vk::DeviceCreateInfo logicalDeviceCreateInfo {
			{},                                                               // flags
			static_cast<uint32_t>(queuesCreateInfo.size()),                   // queueCreateInfoCount
			queuesCreateInfo.data(),                                          // pQueueCreateInfos
			0u,                                                               // enabledLayerCount
			nullptr,                                                          // ppEnabledLayerNames
			static_cast<uint32_t>(createInfo.logicalDeviceExtensions.size()), // enabledExtensionCount
			createInfo.logicalDeviceExtensions.data(),                        // ppEnabledExtensionNames
			&physicalDeviceFeatures,                                          // pEnabledFeatures
		};

		m_LogicalDevice = m_PhysicalDevice.createDevice(logicalDeviceCreateInfo);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_LogicalDevice);

		m_QueueInfo.graphicsQueue = m_LogicalDevice.getQueue(m_QueueInfo.graphicsQueueIndex, 0u);
		m_QueueInfo.presentQueue  = m_LogicalDevice.getQueue(m_QueueInfo.presentQueueIndex, 0u);

		ASSERT(m_QueueInfo.graphicsQueue, "Failed to fetch graphics queue");
		ASSERT(m_QueueInfo.presentQueue, "Failed to fetch present queue");
	}


	////////////////////////////////////////////////////////////////
	/// Initializes VMA
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

		vma::AllocatorCreateInfo allocCreateInfo({},               // flags
		                                         m_PhysicalDevice, // physicalDevice
		                                         m_LogicalDevice,  // device
		                                         {},               // preferredLargeHeapBlockSize
		                                         {},               // pAllocationCallbacks
		                                         {},               // pDeviceMemoryCallbacks
		                                         {},               // pHeapSizeLimit
		                                         &vulkanFunctions, // pVulkanFunctions
		                                         m_Instance,       // instance
		                                         {});              // vulkanApiVersion


		m_Allocator = vma::createAllocator(allocCreateInfo);
	}
}

SurfaceInfo Device::FetchSurfaceInfo()
{
	m_SurfaceInfo.capabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_SurfaceInfo.surface);

	// Select surface format
	m_SurfaceInfo.format = m_SurfaceInfo.supportedFormats[0]; // default
	for (const auto& surfaceFormat : m_SurfaceInfo.supportedFormats)
	{
		if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			m_SurfaceInfo.format = surfaceFormat;
			break;
		}
	}

	// Select present mode
	m_SurfaceInfo.presentMode = m_SurfaceInfo.supportedPresentModes[0]; // default
	for (const auto& presentMode : m_SurfaceInfo.supportedPresentModes)
	{
		if (presentMode == vk::PresentModeKHR::eFifo)
		{
			m_SurfaceInfo.presentMode = presentMode;
		}
	}

	m_SurfaceInfo.supportedFormats      = m_PhysicalDevice.getSurfaceFormatsKHR(m_SurfaceInfo.surface);
	m_SurfaceInfo.supportedPresentModes = m_PhysicalDevice.getSurfacePresentModesKHR(m_SurfaceInfo.surface);

	return m_SurfaceInfo;
}

Device::~Device()
{
	m_LogicalDevice.waitIdle();
	m_Allocator.destroy();

	m_LogicalDevice.destroy(nullptr);

	m_Instance.destroySurfaceKHR(m_SurfaceInfo.surface, nullptr);
	m_Instance.destroy();
}

void Device::LogDebugInfo()
{
	/////////////////////////////////////////////////////////////////////////////////
	// Log debug information about the selected physical device, layers, extensions, etc.
	{
		// #TODO:
		LOG(info, "Device created:");

		LOG(info, "    PhysicalDevice:");
		LOG(info, "        apiVersion: {}", m_PhysicalDeviceProperties.apiVersion);

		LOG(info, "        vendorID: {}", m_PhysicalDeviceProperties.vendorID);
		LOG(info, "        deviceID: {}", m_PhysicalDeviceProperties.deviceID);
		LOG(info, "        deviceType: {}", static_cast<int>(m_PhysicalDeviceProperties.deviceType)); // #todo: Stringify
		LOG(info, "        deviceName: {}", m_PhysicalDeviceProperties.deviceName);

		LOG(info, "    Layers:");
		for (auto layer : m_Layers)
			LOG(info, "        {}", layer);

		LOG(info, "    Extensions:");
		for (auto extension : m_Extensions)
			LOG(info, "        {}", extension);

		LOG(info, "    Queues:");
		LOG(info, "        Graphics: {}", m_QueueInfo.graphicsQueueIndex);
		LOG(info, "        Present: {}", m_QueueInfo.presentQueueIndex);
	}
}
