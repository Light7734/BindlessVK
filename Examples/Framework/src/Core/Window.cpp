#include "Framework/Core/Window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

void Window::init(WindowSpecs specs, vec<pair<int, int>> hints)
{
	this->specs = specs;
	glfwSetErrorCallback([](int code, char const *str) {
		fmt::print(stdout, "glfw error ({}): {}\n", code, str);
	});

	assert_true(glfwInit(), "Failed to initalize glfw");

	for (auto hint : hints)
		glfwWindowHint(hint.first, hint.second);

	glfw_window_handle = glfwCreateWindow(
	    specs.width,
	    specs.height,
	    specs.title.c_str(),
	    nullptr,
	    nullptr
	);
	assert_true(glfw_window_handle, "Failed to create glfw window");

	glfwSetWindowUserPointer(glfw_window_handle, &this->specs);
	bind_callbacks();
}

Window::~Window()
{
	glfwDestroyWindow(glfw_window_handle);
	glfwTerminate();
}

vec<c_str> Window::get_required_extensions() const
{
	c_str *extensions;
	u32 count;

	extensions = glfwGetRequiredInstanceExtensions(&count);
	return vec<c_str>(extensions, extensions + count);
}

void Window::poll_events()
{
	glfwPollEvents();
}

bool Window::should_close()
{
	return glfwWindowShouldClose(glfw_window_handle);
}

vk::SurfaceKHR Window::create_surface(vk::Instance instance)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkResult result = glfwCreateWindowSurface(
	    instance,
	    glfw_window_handle,
	    nullptr,
	    &surface

	);

	assert_true(result == VK_SUCCESS && surface, "Failed to create window surface");
	return surface;
}

vk::Extent2D Window::get_framebuffer_size()
{
	int width, height;
	glfwGetFramebufferSize(glfw_window_handle, &width, &height);
	return vk::Extent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void Window::bind_callbacks()
{
	// @todo: callbacks
	glfwSetKeyCallback(
	    glfw_window_handle,
	    [](GLFWwindow *window, int key, int scanCode, int action, int mods) {
		    if (key == GLFW_KEY_ESCAPE)
			    glfwSetWindowShouldClose(window, true);
		    if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
		    {
			    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
			    {
				    int width, height;
				    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			    }
			    else
			    {
				    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			    }
		    }
	    }
	);

	glfwSetWindowSizeCallback(glfw_window_handle, [](GLFWwindow *window, int width, int height) {
		WindowSpecs *specs = (WindowSpecs *)glfwGetWindowUserPointer(window);
		specs->width = width;
		specs->height = height;
	});
}
