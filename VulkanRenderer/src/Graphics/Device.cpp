#include "Graphics/Device.hpp"
Device::Device(DeviceCreateInfo& createInfo)
    : m_Layers(createInfo.layers), m_Extensions(createInfo.extensions)
{
	/////////////////////////////////////////////////////////////////////////////////
	// Initialize volk
	VKC(volkInitialize());

	/////////////////////////////////////////////////////////////////////////////////
	// Check if the required layers are supported.
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

			// Layer is not supported!
			ASSERT(layerFound, "Required layer not found");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// Create vulkan instance, debug messenger, and load it with volk
	{
		// Application info
		VkApplicationInfo applicationInfo {
			.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName   = "Vulkan Renderer",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName        = "NA",
			.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion         = VK_API_VERSION_1_2
		};

		// Debug messenger create-info
		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {
			.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, // #TODO: Config
			.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pUserData       = nullptr
		};

		debugMessengerCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		                                              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		                                              void* pUserData) {
			LOGVk(trace, "{}", pCallbackData->pMessage); // #TODO: Format this!
			return static_cast<VkBool32>(VK_FALSE);
		};

		// Instance create-info
		VkInstanceCreateInfo createInfo {
			.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext                   = &debugMessengerCreateInfo,
			.pApplicationInfo        = &applicationInfo,
			.enabledLayerCount       = static_cast<uint32_t>(m_Layers.size()),
			.ppEnabledLayerNames     = m_Layers.data(),
			.enabledExtensionCount   = static_cast<uint32_t>(m_Extensions.size()),
			.ppEnabledExtensionNames = m_Extensions.data(),
		};

		// Create and load vulkan
		VKC(vkCreateInstance(&createInfo, nullptr, &m_Instance));
		volkLoadInstance(m_Instance);
	}


	/////////////////////////////////////////////////////////////////////////////////
	// Iterate through physical devices that supports vulkan, and pick the most suitable one
	{
		// Fetch physical devices with vulkan support
		uint32_t deviceCount;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
		ASSERT(deviceCount, "No graphic device with vulkan support found");

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

			// Geometry shader and some queue families are required for rendering and presenting
			if (!features.geometryShader)
				continue;

			/** Check if the device supports all the required queues **/
			{
				// Fetch queue families
				uint32_t queueFamiliesCount;
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);
				if (queueFamiliesCount == 0)
					continue;

				std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
				vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, queueFamiliesProperties.data());

				uint32_t index = 0u;
				for (auto queueFamilyProperties : queueFamiliesProperties)
				{
					// Check if current queue index is a desired queue
					if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					{
						m_GraphicsQueueIndex = index;
					}
					index++;

					// Device does supports all the required queues!
					if (m_GraphicsQueueIndex != UINT32_MAX)
					{
						break;
					}
				}

				// Device doesn't support all the required queues!
				if (m_GraphicsQueueIndex == UINT32_MAX)
				{
					m_GraphicsQueueIndex = UINT32_MAX;
					continue;
				}
			}


		// Discrete gpu is favored
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 69420; // nice

		// Higher image dimensions are favored
		score += properties.limits.maxImageDimension2D;

		// Pick the most suitable device
		m_PhysicalDevice = score > highScore ? device : m_PhysicalDevice;
		}
		ASSERT(m_PhysicalDevice, "No suitable physical device found");

		// Store the selected physical device's properties
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_PhysicalDeviceProperties);
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
	}
}

Device::~Device()
{
	vkDestroyInstance(m_Instance, nullptr);
}
