#include "Window.h"
#include "Graphics/Pipeline.h"

#include <glfw/glfw3.h>

extern "C"
{
	// force machine to use dedicated graphics
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001; // NVidia
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;   // AMD
}

Window::Window()
{
	ASSERT(glfwInit(), "Window::Window: failed to initialize glfw");

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_WindowHandle = glfwCreateWindow(800, 600, "Vulkan Renderer", nullptr, nullptr);

	// #todo:
	BindGlfwEvents();
	glfwSetWindowUserPointer(m_WindowHandle, &m_Pipeline);
}

Window::~Window()
{
	glfwDestroyWindow(m_WindowHandle);
	glfwTerminate();
}

bool Window::IsClosed() const
{
	return glfwWindowShouldClose(m_WindowHandle);
}

void Window::BindGlfwEvents()
{
	glfwSetFramebufferSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height)
		{
			Pipeline** pipelinePtr = (Pipeline**)glfwGetWindowUserPointer(window);
			if (Pipeline* pipeline = *pipelinePtr)
				pipeline->InvalidateSwapchain();
		});
}
