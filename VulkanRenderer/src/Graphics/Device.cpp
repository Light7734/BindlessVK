#include "Graphics/Device.hpp"

Device::Device(DeviceCreateInfo& createInfo)
    : m_Layers(createInfo.layers), m_Extensions(createInfo.extensions)
{
	VKC(volkInitialize());

	CheckLayersSupport();

	auto debugCallbackCreateInfo = CreateDebugCallbackCreateInfo();

	CreateInstance(debugCallbackCreateInfo);
}

Device::~Device()
{
	vkDestroyInstance(m_Instance, nullptr);
}

void Device::CreateInstance(VkDebugUtilsMessengerCreateInfoEXT debugCallbackCreateInfo)
{
	VkApplicationInfo applicationInfo {
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName   = "Vulkan Renderer",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName        = "NA",
		.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion         = VK_API_VERSION_1_2
	};

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

bool Device::CheckLayersSupport()
{
	uint32_t layersCount;
	vkEnumerateInstanceLayerProperties(&layersCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layersCount);
	vkEnumerateInstanceLayerProperties(&layersCount, availableLayers.data());

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

		if (!layerFound)
		{
			LOG(critical, "Layer not supported: {}", requiredLayerName);
			return false;
		}
	}

	return true;
}
