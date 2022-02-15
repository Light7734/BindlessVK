#include "Core/Window.hpp"

#include <GLFW/glfw3.h>

Window::Window(WindowCreateInfo& createInfo)
    : m_Specs(createInfo.specs)
{
	// Initialzie glfw
	ASSERT(glfwInit(), "Failed to initalize glfw");

	// Hint glfw about the window
	for (auto hint : createInfo.hints)
	{
		glfwWindowHint(hint.first, hint.second);
		LOG(trace, "{} ==> {}", hint.first, hint.second);
	}

	// Create window
	m_GlfwWindowHandle = glfwCreateWindow(m_Specs.width, m_Specs.height, m_Specs.title.c_str(), nullptr, nullptr);
	ASSERT(m_GlfwWindowHandle, "Failed to create glfw window");

	// Setup callbacks & userpointer
	glfwSetWindowUserPointer(m_GlfwWindowHandle, &m_Specs);
	BindCallbacks();
}

Window::~Window()
{
	glfwDestroyWindow(m_GlfwWindowHandle);
	glfwTerminate();
}

std::vector<const char*> Window::GetRequiredExtensions()
{
	const char** extensions;
	uint32_t count;

	extensions = glfwGetRequiredInstanceExtensions(&count);
	return std::vector<const char*>(extensions, extensions + count);
}


bool Window::ShouldClose()
{
	return glfwWindowShouldClose(m_GlfwWindowHandle);
}

VkSurfaceKHR Window::CreateSurface(VkInstance instance)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	ASSERT(m_GlfwWindowHandle, "Invalid glfw handle");
	ASSERT(instance, "Invalid instance");
	VKC(glfwCreateWindowSurface(instance, m_GlfwWindowHandle, nullptr, &surface));
	ASSERT(surface, "Failed to create window surface");
	return surface;
}

VkExtent2D Window::GetFramebufferSize()
{
	int width, height;
	glfwGetFramebufferSize(m_GlfwWindowHandle, &width, &height);
	return VkExtent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void Window::BindCallbacks()
{
	// #todo: callbacks
	glfwSetKeyCallback(m_GlfwWindowHandle, [](GLFWwindow* window, int key, int scanCode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
		LOG(trace, "Key pressed: {}", key);
	});
}
