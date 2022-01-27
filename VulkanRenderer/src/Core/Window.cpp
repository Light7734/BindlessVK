#include "Core/Window.hpp"

#include <GLFW/glfw3.h>

Window::Window(WindowSpecs& specs)
    : m_Specs(specs)
{
	ASSERT(glfwInit(), "Failed to initalize glfw");

	glfwWindowHint(GLFW_RESIZABLE, specs.resizable ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_FLOATING, specs.floating ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_OPENGL_API, GLFW_NO_API);

	m_GlfwWindowHandle = glfwCreateWindow(specs.width, specs.height, specs.title.c_str(), nullptr, nullptr);
	ASSERT(m_GlfwWindowHandle, "Failed to create glfw window");

	glfwSetWindowUserPointer(m_GlfwWindowHandle, &m_Specs);
	BindCallbacks();
}

Window::~Window()
{
	glfwDestroyWindow(m_GlfwWindowHandle);
	glfwTerminate();
}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(m_GlfwWindowHandle);
}

void Window::BindCallbacks()
{
	glfwSetKeyCallback(m_GlfwWindowHandle, [](GLFWwindow* window, int key, int scanCode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
		LOG(trace, "Key pressed: {}", key);
	});
}
