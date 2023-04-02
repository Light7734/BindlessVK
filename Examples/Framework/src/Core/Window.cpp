#include "Framework/Core/Window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

Window::Window(Specs const &specs, vec<pair<i32, i32>> const &hints): specs(specs)
{
	glfwSetErrorCallback([](int code, char const *str) {
		fmt::print(stdout, "Glfw error ({}): {}\n", code, str);
	});

	assert_true(glfwInit(), "Failed to initalize glfw");
	glfw_initialized = true;

	for (auto [hint, value] : hints)
		glfwWindowHint(hint, value);

	auto const [width, height] = specs.size;
	glfw_window_handle = glfwCreateWindow(width, height, specs.title.c_str(), {}, {});
	assert_true(glfw_window_handle, "Failed to create glfw window");

	glfwSetWindowUserPointer(glfw_window_handle, &this->specs);
	bind_callbacks();
}

Window::Window(Window &&other)
{
	*this = std::move(other);
}

Window &Window::operator=(Window &&other)
{
	this->glfw_window_handle = other.glfw_window_handle;
	this->specs = other.specs;
	this->glfw_initialized = other.glfw_initialized;

	other.glfw_window_handle = {};
	other.glfw_initialized = {};

	return *this;
}

Window::~Window()
{
	glfwDestroyWindow(glfw_window_handle);

	if (glfw_initialized)
		glfwTerminate();
}

void Window::poll_events()
{
	glfwPollEvents();
}

auto Window::get_required_extensions() const -> vec<c_str>
{
	c_str *extensions;
	u32 count;

	extensions = glfwGetRequiredInstanceExtensions(&count);
	return vec<c_str>(extensions, extensions + count);
}

auto Window::should_close() -> bool
{
	return glfwWindowShouldClose(glfw_window_handle);
}

auto Window::create_vulkan_surface(vk::Instance const instance) -> bvk::Surface::WindowSurface
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	auto const result = glfwCreateWindowSurface(
	    instance,
	    glfw_window_handle,
	    nullptr,
	    &surface

	);

	assert_true(result == VK_SUCCESS && surface, "Failed to create window surface");

	return std::move(bvk::Surface::WindowSurface { surface, instance });
}

auto Window::get_framebuffer_size() -> vk::Extent2D
{
	auto width = i32 {};
	auto height = i32 {};

	glfwGetFramebufferSize(glfw_window_handle, &width, &height);
	return vk::Extent2D { static_cast<u32>(width), static_cast<u32>(height) };
}

void Window::bind_callbacks()
{
	glfwSetKeyCallback(
	    glfw_window_handle,
	    [](GLFWwindow *window, i32 key, i32 scanCode, i32 action, i32 mods) {
		    if (key == GLFW_KEY_ESCAPE)
			    glfwSetWindowShouldClose(window, true);

		    if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
		    {
			    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL)
			    {
				    auto width = i32 {};
				    auto height = i32 {};

				    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			    }
			    else
				    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		    }
	    }
	);

	glfwSetWindowSizeCallback(glfw_window_handle, [](GLFWwindow *window, i32 width, i32 height) {
		Window::Specs *specs = (Window::Specs *)glfwGetWindowUserPointer(window);
		specs->size = { width, height };
	});
}
