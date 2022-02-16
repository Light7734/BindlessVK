
#include "Graphics/Device.hpp"

#include "Core/Window.hpp"

#include <vulkan/vulkan_core.h>

Device::Device(DeviceCreateInfo& createInfo, Window& window)
    : m_Layers(createInfo.layers), m_Extensions(createInfo.extensions)
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
	// Create vulkan instance, window surface, debug messenger, and load the instace with the volk.
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
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, // #TODO: Config
			.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pUserData       = nullptr,
		};
		debugMessengerCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		                                              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		                                              void* pUserData) {
			LOGVk(trace, "{}", pCallbackData->pMessage); // #TODO: Format this!
			return static_cast<VkBool32>(VK_FALSE);
		};

		VkInstanceCreateInfo createInfo {
			.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext                   = &debugMessengerCreateInfo,
			.pApplicationInfo        = &applicationInfo,
			.enabledLayerCount       = static_cast<uint32_t>(m_Layers.size()),
			.ppEnabledLayerNames     = m_Layers.data(),
			.enabledExtensionCount   = static_cast<uint32_t>(m_Extensions.size()),
			.ppEnabledExtensionNames = m_Extensions.data(),
		};

		VKC(vkCreateInstance(&createInfo, nullptr, &m_Instance));
		volkLoadInstance(m_Instance);
		m_Surface = window.CreateSurface(m_Instance);
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

			if (!features.geometryShader)
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
						m_GraphicsQueueIndex = index;
					}

					VkBool32 presentSupport;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_Surface, &presentSupport);
					if (presentSupport)
					{
						m_PresentQueueIndex = index;
					}

					index++;

					// Device does supports all the required queues!
					if (m_GraphicsQueueIndex != UINT32_MAX && m_PresentQueueIndex != UINT32_MAX)
					{
						break;
					}
				}

				// Device doesn't support all the required queues!
				if (m_GraphicsQueueIndex == UINT32_MAX || m_PresentQueueIndex == UINT32_MAX)
				{
					m_GraphicsQueueIndex = UINT32_MAX;
					m_PresentQueueIndex  = UINT32_MAX;
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
				for (const auto& requiredExtensionName : createInfo.deviceExtensions)
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
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &m_SurfcaCapabilities);

				// Fetch swap chain formats
				uint32_t formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
				if (!formatCount)
					continue;

				m_SupportedSurfaceFormats.clear();
				m_SupportedSurfaceFormats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, m_SupportedSurfaceFormats.data());

				// Fetch swap chain present modes
				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
				if (!presentModeCount)
					continue;

				m_SupportedPresentModes.clear();
				m_SupportedPresentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, m_SupportedPresentModes.data());
			}


			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				score += 69420; // nice

			score += properties.limits.maxImageDimension2D;

			m_PhysicalDevice = score > highScore ? device : m_PhysicalDevice;
		}
		ASSERT(m_PhysicalDevice, "No suitable physical device found");

		// Store the selected physical device's properties
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create the logical device.
	{
		std::vector<VkDeviceQueueCreateInfo> queuesCreateInfo;

		float queuePriority = 1.0f; // Always 1.0
		queuesCreateInfo.push_back({
		    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		    .queueFamilyIndex = m_GraphicsQueueIndex,
		    .queueCount       = 1u,
		    .pQueuePriorities = &queuePriority,
		});

		if (m_PresentQueueIndex != m_GraphicsQueueIndex)
		{
			queuesCreateInfo.push_back({
			    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			    .queueFamilyIndex = m_PresentQueueIndex,
			    .queueCount       = 1u,
			    .pQueuePriorities = &queuePriority,
			});
		}

		// No features needed ATM
		VkPhysicalDeviceFeatures physicalDeviceFeatures {}; // #TODO

		VkDeviceCreateInfo logicalDeviceCreateInfo {
			.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount    = static_cast<uint32_t>(queuesCreateInfo.size()),
			.pQueueCreateInfos       = queuesCreateInfo.data(),
			.enabledExtensionCount   = static_cast<uint32_t>(createInfo.deviceExtensions.size()),
			.ppEnabledExtensionNames = createInfo.deviceExtensions.data(),
			.pEnabledFeatures        = &physicalDeviceFeatures,
		};

		VKC(vkCreateDevice(m_PhysicalDevice, &logicalDeviceCreateInfo, nullptr, &m_LogicalDevice));
	}


	/////////////////////////////////////////////////////////////////////////////////
	// Create the swap chain.
	{
		// Select surface format
		m_SurfaceFormat = m_SupportedSurfaceFormats[0]; // default
		for (const auto& surfaceFormat : m_SupportedSurfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				m_SurfaceFormat = surfaceFormat;
				break;
			}
		}

		// Select present mode
		m_PresentMode = m_SupportedPresentModes[0]; // default
		for (const auto& presentMode : m_SupportedPresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				m_PresentMode = presentMode;
			}
		}

		// Select extent
		m_SwapchainExtent = m_SurfcaCapabilities.currentExtent;
		if (m_SwapchainExtent.width == UINT32_MAX)
		{
			VkExtent2D actualExtent = window.GetFramebufferSize();
			actualExtent.width      = std::clamp(actualExtent.width, m_SurfcaCapabilities.minImageExtent.width, m_SurfcaCapabilities.maxImageExtent.width);
			actualExtent.height     = std::clamp(actualExtent.height, m_SurfcaCapabilities.minImageExtent.height, m_SurfcaCapabilities.maxImageExtent.height);
		}

		// Select image count ; one more than minImageCount, if minImageCount +1 is not higher than maxImageCount (maxImageCount of 0 means no limit)
		uint32_t imageCount = m_SurfcaCapabilities.maxImageCount > 0 && m_SurfcaCapabilities.minImageCount + 1 > m_SurfcaCapabilities.maxImageCount ? m_SurfcaCapabilities.maxImageCount : m_SurfcaCapabilities.minImageCount + 1;

		// Create swapchain
		bool sameQueueIndex = m_GraphicsQueueIndex == m_PresentQueueIndex;
		VkSwapchainCreateInfoKHR swapchainCreateInfo {
			.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface               = m_Surface,
			.minImageCount         = imageCount,
			.imageFormat           = m_SurfaceFormat.format,
			.imageColorSpace       = m_SurfaceFormat.colorSpace,
			.imageExtent           = m_SwapchainExtent,
			.imageArrayLayers      = 1u,
			.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // Write directly to the image (we would use a value like VK_IMAGE_USAGE_TRANSFER_DST_BIT if we wanted to do post-processing)
			.imageSharingMode      = sameQueueIndex ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = sameQueueIndex ? 0u : 2u,
			.pQueueFamilyIndices   = sameQueueIndex ? nullptr : &m_GraphicsQueueIndex,
			.preTransform          = m_SurfcaCapabilities.currentTransform,
			.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // No alpha-blending between multiple windows
			.presentMode           = m_PresentMode,
			.clipped               = VK_TRUE, // Don't render the obsecured pixels
			.oldSwapchain          = VK_NULL_HANDLE,
		};

		VKC(vkCreateSwapchainKHR(m_LogicalDevice, &swapchainCreateInfo, nullptr, &m_Swapchain));
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Fetch swap chain images and create image views
	{
		// Fetch images
		uint32_t imageCount;
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &imageCount, nullptr);
		m_Images.resize(imageCount);
		m_ImageViews.resize(imageCount);
		vkGetSwapchainImagesKHR(m_LogicalDevice, m_Swapchain, &imageCount, m_Images.data());

		for (uint32_t i = 0; i < m_Images.size(); i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo {
				.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image      = m_Images[i],
				.viewType   = VK_IMAGE_VIEW_TYPE_2D,
				.format     = m_SurfaceFormat.format,
				.components = {
				    // Don't swizzle the colors around...
				    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
				    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
				    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
				    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange = {
				    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, // Image will be used as color target
				    .baseMipLevel   = 0,                         // No mipmaipping
				    .levelCount     = 1,                         // No levels
				    .baseArrayLayer = 0,                         // No nothin...
				    .layerCount     = 1,
				},
			};

			VKC(vkCreateImageView(m_LogicalDevice, &imageViewCreateInfo, nullptr, &m_ImageViews[i]));
		}
	}


	/////////////////////////////////////////////////////////////////////////////////
	// Log debug information about the selected physical device, layers, extensions, etc.
	{
		LOG(info, "Device created:");

		LOG(info, "    PhysicalDevice:");
		LOG(info, "        apiVersion: {}", m_PhysicalDeviceProperties.apiVersion);
		LOG(info, "        driverVersion: {}", m_PhysicalDeviceProperties.driverVersion);
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
		LOG(info, "        Graphics: {}", m_GraphicsQueueIndex);
		LOG(info, "        Present: {}", m_PresentQueueIndex);

		LOG(info, "    Swapchain:");
		LOG(info, "        imageCount: {}", m_Images.size());
		LOG(info, "        extent: {}x{}", m_SwapchainExtent.width, m_SwapchainExtent.height);
	}
}

Device::~Device()
{
	for (auto imageView : m_ImageViews)
	{
		vkDestroyImageView(m_LogicalDevice, imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_LogicalDevice, m_Swapchain, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
	vkDestroyDevice(m_LogicalDevice, nullptr);

}
