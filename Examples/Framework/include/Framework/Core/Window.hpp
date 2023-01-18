#pragma once

#include "Framework/Common/Common.hpp"

struct GLFWwindow;

struct WindowSpecs
{
	std::string title;
	uint32_t width, height;
};

class Window
{
public:
	Window() = default;
	~Window();

	void init(WindowSpecs specs, std::vector<std::pair<int, int>> hints);

	std::vector<const char*> get_required_extensions();

	void poll_events();
	bool should_close();

	vk::SurfaceKHR create_surface(vk::Instance instance);
	vk::Extent2D get_framebuffer_size();

	inline GLFWwindow* get_glfw_handle()
	{
		return m_glfw_window_handle;
	}

private:
	GLFWwindow* m_glfw_window_handle = {};
	WindowSpecs m_specs              = {};

	void bind_callbacks();
};
