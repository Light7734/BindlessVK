#include "Graphics/Device.hpp"

#include "Core/Window.hpp"
#include "Utils/Timer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_core.h>

Device::Device(DeviceCreateInfo& createInfo)
    : m_Layers(createInfo.layers), m_Extensions(createInfo.instanceExtensions)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Initialize volk
	VKC(volkInitialize());

	/////////////////////////////////////////////////////////////////////////////////
	// Check if the required layers exist.
	{
		// Fetch supported layers
		uint32_t layersCount;
		vkEnumerateInstanceLayerProperties(&layersCount, nullptr);
		ASSERT(layersCount, "No instance layer found");

		std::vector<VkLayerProperties> availableLayers(layersCount);
		vkEnumerateInstanceLayerProperties(&layersCount, availableLayers.data());

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
		VkApplicationInfo applicationInfo {
			.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName   = "Vulkan Renderer",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName        = "None",
			.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion         = VK_API_VERSION_1_2,
		};

		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
			.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = createInfo.debugMessageSeverity,
			.messageType     = createInfo.debugMessageTypes,
			.pUserData       = nullptr,
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
		VkInstanceCreateInfo instanceCreateInfo {
			.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext                   = createInfo.enableDebugging ? &debugMessengerCreateInfo : nullptr,
			.pApplicationInfo        = &applicationInfo,
			.enabledLayerCount       = static_cast<uint32_t>(m_Layers.size()),
			.ppEnabledLayerNames     = m_Layers.data(),
			.enabledExtensionCount   = static_cast<uint32_t>(m_Extensions.size()),
			.ppEnabledExtensionNames = m_Extensions.data(),
		};

		VKC(vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance));
		volkLoadInstance(m_Instance);
		m_SurfaceInfo.surface = createInfo.window->CreateSurface(m_Instance);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Iterate through physical devices that supports vulkan, and pick the most suitable one
	{
		// Fetch physical devices with vulkan support
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		ASSERT(deviceCount, "No physical device with vulkan support found");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		// Select the most suitable physical device
		uint32_t highScore = 0u;
		for (const auto& device : devices)
		{
			uint32_t score = 0u;

			// Fetch properties & features
			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceProperties(device, &properties);
			vkGetPhysicalDeviceFeatures(device, &features);

			// Check if device supports required features
			if (!features.geometryShader || !features.samplerAnisotropy)
				continue;

			/** Check if the device supports the required queues **/
			{
				// Fetch queue families
				uint32_t queueFamiliesCount;
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);
				if (!queueFamiliesCount)
					continue;

				std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, queueFamiliesProperties.data());

				uint32_t index = 0u;
				for (const auto& queueFamilyProperties : queueFamiliesProperties)
				{
					// Check if current queue index supports a desired queue
					if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						m_QueueInfo.graphicsQueueIndex = index;
					}

					VkBool32 presentSupport;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_SurfaceInfo.surface, &presentSupport);
					if (presentSupport)
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
				uint32_t extensionsCount;
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
				if (!extensionsCount)
					continue;

				std::vector<VkExtensionProperties> deviceExtensionsProperties(extensionsCount);
				vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, deviceExtensionsProperties.data());

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
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_SurfaceInfo.surface, &m_SurfaceInfo.capabilities);

				// Fetch swap chain formats
				uint32_t formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_SurfaceInfo.surface, &formatCount, nullptr);
				if (!formatCount)
					continue;

				m_SurfaceInfo.supportedFormats.clear();
				m_SurfaceInfo.supportedFormats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_SurfaceInfo.surface, &formatCount, m_SurfaceInfo.supportedFormats.data());

				// Fetch swap chain present modes
				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_SurfaceInfo.surface, &presentModeCount, nullptr);
				if (!presentModeCount)
					continue;

				m_SurfaceInfo.supportedPresentModes.clear();
				m_SurfaceInfo.supportedPresentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_SurfaceInfo.surface, &presentModeCount, m_SurfaceInfo.supportedPresentModes.data());
			}

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				score += 69420; // nice

			score += properties.limits.maxImageDimension2D;

			m_PhysicalDevice = score > highScore ? device : m_PhysicalDevice;
		}
		ASSERT(m_PhysicalDevice, "No suitable physical device found");

		// Store the selected physical device's properties
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);

		// Determine max sample counts
		VkSampleCountFlags counts = m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts & m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts;
		// m_MaxSupportedSampleCount = VK_SAMPLE_COUNT_1_BIT;
		m_MaxSupportedSampleCount = counts & VK_SAMPLE_COUNT_64_BIT ? VK_SAMPLE_COUNT_64_BIT :
		                            counts & VK_SAMPLE_COUNT_32_BIT ? VK_SAMPLE_COUNT_32_BIT :
		                            counts & VK_SAMPLE_COUNT_16_BIT ? VK_SAMPLE_COUNT_16_BIT :
		                            counts & VK_SAMPLE_COUNT_8_BIT  ? VK_SAMPLE_COUNT_8_BIT :
		                            counts & VK_SAMPLE_COUNT_4_BIT  ? VK_SAMPLE_COUNT_4_BIT :
		                            counts & VK_SAMPLE_COUNT_2_BIT  ? VK_SAMPLE_COUNT_2_BIT :
		                                                              VK_SAMPLE_COUNT_1_BIT;
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the logical device.
	{
		std::vector<VkDeviceQueueCreateInfo> queuesCreateInfo;

		float queuePriority = 1.0f; // Always 1.0
		queuesCreateInfo.push_back({
		    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		    .queueFamilyIndex = m_QueueInfo.graphicsQueueIndex,
		    .queueCount       = 1u,
		    .pQueuePriorities = &queuePriority,
		});

		if (m_QueueInfo.presentQueueIndex != m_QueueInfo.graphicsQueueIndex)
		{
			queuesCreateInfo.push_back({
			    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			    .queueFamilyIndex = m_QueueInfo.presentQueueIndex,
			    .queueCount       = 1u,
			    .pQueuePriorities = &queuePriority,
			});
		}

		// No features needed ATM
		VkPhysicalDeviceFeatures physicalDeviceFeatures {
			.samplerAnisotropy = VK_TRUE,
		};

		VkDeviceCreateInfo logicalDeviceCreateInfo {
			.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount    = static_cast<uint32_t>(queuesCreateInfo.size()),
			.pQueueCreateInfos       = queuesCreateInfo.data(),
			.enabledExtensionCount   = static_cast<uint32_t>(createInfo.logicalDeviceExtensions.size()),
			.ppEnabledExtensionNames = createInfo.logicalDeviceExtensions.data(),
			.pEnabledFeatures        = &physicalDeviceFeatures,
		};

		VKC(vkCreateDevice(m_PhysicalDevice, &logicalDeviceCreateInfo, nullptr, &m_LogicalDevice));

		vkGetDeviceQueue(m_LogicalDevice, m_QueueInfo.graphicsQueueIndex, 0u, &m_QueueInfo.graphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueInfo.presentQueueIndex, 0u, &m_QueueInfo.presentQueue);

		ASSERT(m_QueueInfo.graphicsQueue, "Failed to fetch graphics queue");
		ASSERT(m_QueueInfo.presentQueue, "Failed to fetch present queue");
	}
}

SurfaceInfo Device::FetchSurfaceInfo()
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_SurfaceInfo.surface, &m_SurfaceInfo.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_SurfaceInfo.surface, &formatCount, nullptr);
	m_SurfaceInfo.supportedFormats.clear();
	m_SurfaceInfo.supportedFormats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_SurfaceInfo.surface, &formatCount, m_SurfaceInfo.supportedFormats.data());

	// Select surface format
	m_SurfaceInfo.format = m_SurfaceInfo.supportedFormats[0]; // default
	for (const auto& surfaceFormat : m_SurfaceInfo.supportedFormats)
	{
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			m_SurfaceInfo.format = surfaceFormat;
			break;
		}
	}

	// Select present mode
	m_SurfaceInfo.presentMode = m_SurfaceInfo.supportedPresentModes[0]; // default
	for (const auto& presentMode : m_SurfaceInfo.supportedPresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			m_SurfaceInfo.presentMode = presentMode;
		}
	}

	return m_SurfaceInfo;
}

Device::~Device()
{
	vkDeviceWaitIdle(m_LogicalDevice);

	vkDestroyDevice(m_LogicalDevice, nullptr);

	vkDestroySurfaceKHR(m_Instance, m_SurfaceInfo.surface, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
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
		LOG(info, "        deviceType: {}", m_PhysicalDeviceProperties.deviceType); // #todo: Stringify
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
