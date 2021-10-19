#pragma once

#include <volk.h>

struct GLFWwindow;

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphics;
	std::optional<uint32_t> present;

	std::vector<uint32_t> indices;

	inline bool IsSuitableForRendering() const { return graphics.has_value() && present.has_value(); }

	operator bool() const { return IsSuitableForRendering(); };
};

struct SharedContext
{
	GLFWwindow* windowHandle = nullptr;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	QueueFamilyIndices queueFamilies;

	inline operator bool() const { return logicalDevice; }
};