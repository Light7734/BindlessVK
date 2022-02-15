#pragma once

#include "Core/Base.hpp"

#include <volk.h>

struct GLFWwindow;

struct WindowSpecs
{
	std::string title;
	uint32_t width, height;
};

struct WindowCreateInfo
{
	WindowSpecs specs;
	std::vector<std::pair<int, int>> hints;
};

class Window
{
public:
	Window(WindowCreateInfo& createInfo);
	~Window();

	std::vector<const char*> GetRequiredExtensions();

	bool ShouldClose();

	VkSurfaceKHR CreateSurface(VkInstance instance);
	VkExtent2D GetFramebufferSize();

private:
	GLFWwindow* m_GlfwWindowHandle = nullptr;
	WindowSpecs m_Specs;

	void BindCallbacks();
};
