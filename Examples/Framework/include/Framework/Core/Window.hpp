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

	void init(WindowSpecs specs, vec<pair<int, int>> hints);

	vec<c_str> get_required_extensions() const;

	void poll_events();
	bool should_close();

	vk::SurfaceKHR create_surface(vk::Instance instance);
	vk::Extent2D get_framebuffer_size();

	inline GLFWwindow *get_glfw_handle()
	{
		return glfw_window_handle;
	}

private:
	GLFWwindow *glfw_window_handle = {};
	WindowSpecs specs = {};

	void bind_callbacks();
};
