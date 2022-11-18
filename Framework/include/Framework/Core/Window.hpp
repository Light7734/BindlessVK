#pragma once

#include "Framework/Core/Common.hpp"

#include <vulkan/vulkan.hpp>

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
	Window() = default;
	void Init(const WindowCreateInfo& createInfo);

	~Window();

	std::vector<const char*> GetRequiredExtensions();

	void PollEvents();
	bool ShouldClose();

	vk::SurfaceKHR CreateSurface(vk::Instance instance);
	vk::Extent2D GetFramebufferSize();

	inline GLFWwindow* GetGlfwHandle() { return m_GlfwWindowHandle; }

private:
	GLFWwindow* m_GlfwWindowHandle = {};
	WindowSpecs m_Specs            = {};

	void BindCallbacks();
};
