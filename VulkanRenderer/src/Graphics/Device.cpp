#include "Graphics/Device.hpp"

Device::Device(DeviceCreateInfo& createInfo)
    : m_Layers(createInfo.layers), m_Extensions(createInfo.extensions)
{
	VKC(volkInitialize());

	CheckLayersSupport();

	CreateInstance();
	PickPhysicalDevice();

	// Log debug information about device
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

Device::~Device()
{
	vkDestroyInstance(m_Instance, nullptr);
}

void Device::CreateInstance()
{
	VkApplicationInfo applicationInfo {
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName   = "Vulkan Renderer",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName        = "NA",
		.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion         = VK_API_VERSION_1_2
	};

	auto debugCallbackCreateInfo = CreateDebugCallbackCreateInfo();
	VkInstanceCreateInfo createInfo {
		.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext                   = &debugCallbackCreateInfo,
		.pApplicationInfo        = &applicationInfo,
		.enabledLayerCount       = static_cast<uint32_t>(m_Layers.size()),
		.ppEnabledLayerNames     = m_Layers.data(),
		.enabledExtensionCount   = static_cast<uint32_t>(m_Extensions.size()),
		.ppEnabledExtensionNames = m_Extensions.data(),
	};

	VKC(vkCreateInstance(&createInfo, nullptr, &m_Instance));
	volkLoadInstance(m_Instance);
}

void Device::PickPhysicalDevice()
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
		if (!features.geometryShader || !FetchQueueFamilyIndices(device))
			continue;

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

VkDebugUtilsMessengerCreateInfoEXT Device::CreateDebugCallbackCreateInfo()
{
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {
		.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, // #TODO: Config
		.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pUserData       = nullptr
	};

	debugUtilsCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                          VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	                                          const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                          void* pUserData) {
		LOGVk(trace, "{}", pCallbackData->pMessage); // #TODO: Format this!
		return static_cast<VkBool32>(VK_FALSE);
	};

	return debugUtilsCreateInfo;
}

void Device::CheckLayersSupport()
{
	// Fetch supported layers
	uint32_t layersCount;
	vkEnumerateInstanceLayerProperties(&layersCount, nullptr);

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

bool Device::FetchQueueFamilyIndices(VkPhysicalDevice device)
{
	// Fetch queue families
	uint32_t queueFamiliesCount;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamiliesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamiliesCount, queueFamiliesProperties.data());

	// Check if required queue families exist
	uint32_t index = 0u;
	for (auto queueFamilyProperties : queueFamiliesProperties)
	{
		if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_GraphicsQueueIndex = index;
		}
		index++;

		if (m_GraphicsQueueIndex != UINT32_MAX)
		{
			return true;
		}
	}

	m_GraphicsQueueIndex = UINT32_MAX;
	return false;
}
