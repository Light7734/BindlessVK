#include "DeviceContext.h"

#include <vector>
#include <set>
#include <string>

DeviceContext::DeviceContext(GLFWwindow* windowHandle) :
	m_WindowHandle(windowHandle),
	m_Surface(VK_NULL_HANDLE),
	m_VkInstance(VK_NULL_HANDLE),
	m_PhysicalDevice(VK_NULL_HANDLE),
	m_LogicalDevice(VK_NULL_HANDLE),
	m_GraphicsQueue(VK_NULL_HANDLE),
	m_PresentQueue(VK_NULL_HANDLE),
	m_ValidationLayers{ "VK_LAYER_KHRONOS_validation" },
	m_GlobalExtensions{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME },
	m_DeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME }
{
	// load and setup vulkan
	VKC(volkInitialize());

	FilterValidationLayers();
	FetchGlobalExtensions();
	auto debugMessageCreateInfo = SetupDebugMessageCallback();

	// application info
	VkApplicationInfo appInfo
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,

		.pApplicationName = "VulkanRenderer",
		.applicationVersion = VK_MAKE_VERSION(1u, 0u, 0u),

		.pEngineName = "",
		.engineVersion = VK_MAKE_VERSION(1u, 0u, 0u),

		.apiVersion = VK_API_VERSION_1_2
	};

	// instance create-info
	VkInstanceCreateInfo instanceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = &debugMessageCreateInfo,

		.pApplicationInfo = &appInfo,

		.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size()),
		.ppEnabledLayerNames = m_ValidationLayers.data(),

		.enabledExtensionCount = static_cast<uint32_t>(m_GlobalExtensions.size()),
		.ppEnabledExtensionNames = m_GlobalExtensions.data(),
	};

	// create and load vulkan instance
	VKC(vkCreateInstance(&instanceCreateInfo, nullptr, &m_VkInstance));
	volkLoadInstance(m_VkInstance);

	// pick physical device and get it rolling...
	CreateWindowSurface(windowHandle);
	PickPhysicalDevice();
	CreateLogicalDevice();
	FetchDeviceExtensions();

	m_SharedContext.windowHandle = windowHandle;
	m_SharedContext.physicalDevice = m_PhysicalDevice;
	m_SharedContext.logicalDevice = m_LogicalDevice;
	m_SharedContext.surface = m_Surface;
	m_SharedContext.queueFamilies = m_QueueFamilyIndices;
}

DeviceContext::~DeviceContext()
{
	vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
	vkDestroyDevice(m_LogicalDevice, nullptr);
	vkDestroyInstance(m_VkInstance, nullptr);
}

void DeviceContext::FetchGlobalExtensions()
{
	// fetch required global extensions
	uint32_t glfwExtensionsCount = 0u;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

	// add required extensions to desired extensions
	m_GlobalExtensions.insert(m_GlobalExtensions.end(), glfwExtensions, glfwExtensions + glfwExtensionsCount);
}

void DeviceContext::FetchDeviceExtensions()
{
	// fetch device extensions
	uint32_t deviceExtensionsCount = 0u;
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionsCount);
	vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &deviceExtensionsCount, deviceExtensions.data());

	// check if device supports the required extensions
	std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());

	for (const auto& deviceExtension : deviceExtensions)
		requiredExtensions.erase(std::string(deviceExtension.extensionName));

	if (!requiredExtensions.empty())
	{
		LOG(critical, "GraphicsContext::FetchDeviceExtensions: device does not supprot required extensions: ");

		for (auto requiredExtension : requiredExtensions)
			LOG(critical, "        {}", requiredExtension);

		ASSERT(false, "GraphicsContext::FetchDeviceExtensions: aforementioned device extensinos are not supported");
	}
}

void DeviceContext::FetchSupportedQueueFamilies()
{
	m_QueueFamilyIndices = {};

	// fetch queue families
	uint32_t queueFamilyCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

	// find queue family indices
	uint32_t index = 0u;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			m_QueueFamilyIndices.graphics = index;

		VkBool32 hasPresentSupport;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, index, m_Surface, &hasPresentSupport);

		if (hasPresentSupport)
			m_QueueFamilyIndices.present = index;

		index++;
	}

	m_QueueFamilyIndices.indices = { m_QueueFamilyIndices.graphics.value(), m_QueueFamilyIndices.present.value() };
}

void DeviceContext::PickPhysicalDevice()
{
	// fetch physical devices
	uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);
	ASSERT(deviceCount, "GraphicsContext::PickPhysicalDevice: failed to find a GPU with vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

	// select most suitable physical device
	uint8_t highestDeviceScore = 0u;
	for (const auto& device : devices)
	{
		uint32_t deviceScore = 0u;

		// fetch properties & features
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;

		vkGetPhysicalDeviceProperties(device, &properties);
		vkGetPhysicalDeviceFeatures(device, &features);

		// geometry shader is needed for rendering
		if (!features.geometryShader)
			continue;

		// discrete gpu is favored
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			deviceScore += 1000;

		deviceScore += properties.limits.maxImageDimension2D;

		// more suitable device found
		if (deviceScore > highestDeviceScore)
		{
			m_PhysicalDevice = device;

			// check if device supports required queue families
			FetchSupportedQueueFamilies();
			if (!m_QueueFamilyIndices)
				m_PhysicalDevice = VK_NULL_HANDLE;
			else
				highestDeviceScore = deviceScore;
		}
	}

	ASSERT(m_PhysicalDevice, "GraphicsContext::PickPhysicalDevice: failed to find suitable GPU for vulkan");
}

void DeviceContext::CreateLogicalDevice()
{
	// fetch properties & features
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	deviceQueueCreateInfos.reserve(m_QueueFamilyIndices.indices.size());

	// device queue create-info
	float queuePriority = 1.0f;
	for (const auto& queueFamilyIndex : m_QueueFamilyIndices.indices)
	{
		VkDeviceQueueCreateInfo deviceQueueCreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,

			.queueFamilyIndex = queueFamilyIndex,
			.queueCount = 1u,

			.pQueuePriorities = &queuePriority,
		};

		deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
	}

	// device create-info
	VkDeviceCreateInfo deviceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,

		.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size()),
		.pQueueCreateInfos = deviceQueueCreateInfos.data(),

		.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size()),
		.ppEnabledExtensionNames = m_DeviceExtensions.data(),

		.pEnabledFeatures = &deviceFeatures,
	};

	// create logical device and get it's queue
	VKC(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice));
	vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.graphics.value(), 0u, &m_GraphicsQueue);
}

void DeviceContext::CreateWindowSurface(GLFWwindow* windowHandle)
{
	glfwCreateWindowSurface(m_VkInstance, windowHandle, nullptr, &m_Surface);
}

void DeviceContext::FilterValidationLayers()
{
	// fetch available layers
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// remove requested layers that are not supported
	for (const char* layerName : m_ValidationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			m_ValidationLayers.erase(std::find(m_ValidationLayers.begin(), m_ValidationLayers.end(), layerName));
			ASSERT("GraphicsContext::FilterValidationLayers: failed to find validation layer: {}", layerName);
		}
	}
}

VkDebugUtilsMessengerCreateInfoEXT DeviceContext::SetupDebugMessageCallback()
{
	// debug messenger create-info
	VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pUserData = nullptr
	};

	// debug message callback
	debugMessengerCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		LOGVk(trace, "GraphicsContext::LogDebugData: NO_IMPLEMENT");
		return VK_FALSE;
	};

	return debugMessengerCreateInfo;
}

// #todo: implement
void DeviceContext::LogDebugData()
{
	LOG(err, "GraphicsContext::LogDebugData: NO_IMPLEMENT");
}
