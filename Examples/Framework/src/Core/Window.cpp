#include "Framework/Core/Window.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

void Window::init(WindowSpecs specs, std::vector<std::pair<int, int>> hints)
{
	m_specs = specs;

	glfwSetErrorCallback([](int code, const char* str) { LOG(critical, str); });

	// Initialzie glfw
	ASSERT(glfwInit(), "Failed to initalize glfw");

	// Hint glfw about the window
	for (auto hint : hints) {
		glfwWindowHint(hint.first, hint.second);
		LOG(trace, "{} ==> {}", hint.first, hint.second);
	}

	// Create window
	m_glfw_window_handle = glfwCreateWindow(
	  m_specs.width,
	  m_specs.height,
	  m_specs.title.c_str(),
	  nullptr,
	  nullptr
	);
	ASSERT(m_glfw_window_handle, "Failed to create glfw window");

	// Setup callbacks & userpointer
	glfwSetWindowUserPointer(m_glfw_window_handle, &m_specs);
	bind_callbacks();

	// glfwSetInputMode(m_GlfwWindowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

Window::~Window()
{
	glfwDestroyWindow(m_glfw_window_handle);
	glfwTerminate();
}

std::vector<const char*> Window::get_required_extensions()
{
	const char** extensions;
	uint32_t count;

	extensions = glfwGetRequiredInstanceExtensions(&count);
	return std::vector<const char*>(extensions, extensions + count);
}

void Window::poll_events()
{
	glfwPollEvents();
}

bool Window::should_close()
{
	return glfwWindowShouldClose(m_glfw_window_handle);
}

vk::SurfaceKHR Window::create_surface(vk::Instance instance)
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkResult result = glfwCreateWindowSurface(
	  instance,
	  m_glfw_window_handle,
	  nullptr,
	  &surface

	);
	ASSERT(result == VK_SUCCESS && surface, "Failed to create window surface");
	return surface;
}

vk::Extent2D Window::get_framebuffer_size()
{
	int width, height;
	glfwGetFramebufferSize(m_glfw_window_handle, &width, &height);
	return vk::Extent2D { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void Window::bind_callbacks()
{
	// @todo: callbacks
	glfwSetKeyCallback(
	  m_glfw_window_handle,
	  [](GLFWwindow* window, int key, int scanCode, int action, int mods) {
		  if (key == GLFW_KEY_ESCAPE)
			  glfwSetWindowShouldClose(window, true);
		  if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
			  if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
				  int width, height;
				  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			  }
			  else {
				  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			  }
		  }
		  LOG(trace, "Key pressed: {}", key);
	  }
	);

	glfwSetWindowSizeCallback(m_glfw_window_handle, [](GLFWwindow* window, int width, int height) {
		WindowSpecs* specs = (WindowSpecs*)glfwGetWindowUserPointer(window);
		specs->width       = width;
		specs->height      = height;
	});
}
