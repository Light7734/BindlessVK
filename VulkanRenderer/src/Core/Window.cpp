#include "Core/Window.h"

#include "Graphics/Renderer.h"

#include <GLFW/glfw3.h>

Window::Window(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
	ASSERT(glfwInit(), "Window::Window: failed to initialize glfw");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_WindowHandle = glfwCreateWindow(width, height, "Vulkan Renderer", nullptr, nullptr);

	// #todo:
	BindGlfwEvents();
	glfwSetWindowUserPointer(m_WindowHandle, &m_WasResized);
}

Window::~Window()
{
	glfwDestroyWindow(m_WindowHandle);
	glfwTerminate();
}

void Window::SetWindowUserPointer(class Renderer* renderer)
{
	glfwSetWindowUserPointer(m_WindowHandle, (void*)renderer);
}

bool Window::IsClosed() const
{
	return glfwWindowShouldClose(m_WindowHandle);
}

void Window::BindGlfwEvents()
{
	glfwSetWindowSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height) {
		Renderer* renderer = (Renderer*)glfwGetWindowUserPointer(window);

		if (renderer)
		{
			renderer->Resize(width, height);
		}
	});

	glfwSetKeyCallback(m_WindowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, true);
	});
}
