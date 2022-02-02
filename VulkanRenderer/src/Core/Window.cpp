#include "Core/Window.hpp"

#include <GLFW/glfw3.h>

Window::Window(WindowCreateInfo& createInfo)
    : m_Specs(createInfo.specs)
{
	// initialzie glfw
	ASSERT(glfwInit(), "Failed to initalize glfw");

	// hint glfw about the window
	for (auto hint : createInfo.hints)
		glfwWindowHint(hint.first, hint.second);

	glfwWindowHint(GLFW_OPENGL_API, GLFW_NO_API);

	// create window
	m_GlfwWindowHandle = glfwCreateWindow(m_Specs.width, m_Specs.height, m_Specs.title.c_str(), nullptr, nullptr);
	ASSERT(m_GlfwWindowHandle, "Failed to create glfw window");

	// setup callbacks & userpointer
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

void Window::BindCallbacks()
{
	// #todo: callbacks
	glfwSetKeyCallback(m_GlfwWindowHandle, [](GLFWwindow* window, int key, int scanCode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
		LOG(trace, "Key pressed: {}", key);
	});
}
