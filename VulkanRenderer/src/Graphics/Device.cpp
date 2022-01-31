#include "Graphics/Device.hpp"

Device::Device(const char** extensionNames, uint32_t extensionsCount)
    : m_ExtensionNames(extensionNames), m_ExtensionsCount(extensionsCount)
{
	VkApplicationInfo applicationInfo {
		.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName   = "Vulkan Renderer",
		.applicationVersion = VK_MAKE_VERSION(0, 1, 1),
		.pEngineName        = "NA",
		.engineVersion      = VK_MAKE_VERSION(0, 0, 0),
		.apiVersion         = VK_API_VERSION_1_2
	};

	CreateVkInstance(&applicationInfo);
}

Device::~Device()
{
	vkDestroyInstance(m_Instance, nullptr);
}

void Device::CreateVkInstance(VkApplicationInfo* appInfo)
{
	VkInstanceCreateInfo createInfo {
		.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo        = appInfo,
		.enabledExtensionCount   = m_ExtensionsCount,
		.ppEnabledExtensionNames = m_ExtensionNames,
	};

	vkCreateInstance(&createInfo, nullptr, &m_Instance);
}
