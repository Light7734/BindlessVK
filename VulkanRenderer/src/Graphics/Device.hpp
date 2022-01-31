#pragma once

#include "volk.h"

class Device
{
public:
	Device(const char** extensionNames, uint32_t extensionsCount);
	~Device();

private:
	// Instance
	VkInstance m_Instance = VK_NULL_HANDLE;

	// Extensions & Validation layers
	uint32_t m_ExtensionsCount    = 0u;
	const char** m_ExtensionNames = nullptr;

	void CreateVkInstance(VkApplicationInfo* appInfo);
};
