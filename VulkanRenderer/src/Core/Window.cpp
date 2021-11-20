#include "Core/Window.h"

#include "Graphics/Renderer.h"

#include <glfw/glfw3.h>

extern "C"
{
    // force machine to use dedicated graphics
    __declspec(dllexport) unsigned long NvOptimusEnablement        = 0x00000001; // NVidia
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;          // AMD
}

Window::Window(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    ASSERT(glfwInit(), "Window::Window: failed to initialize glfw");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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

bool Window::IsClosed() const
{
    return glfwWindowShouldClose(m_WindowHandle);
}

void Window::BindGlfwEvents()
{
    glfwSetFramebufferSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height) {
        bool* wasResized = (bool*)glfwGetWindowUserPointer(window);

        *wasResized = false;
    });

    glfwSetKeyCallback(m_WindowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, true);
    });
}
