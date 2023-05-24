#pragma once

#include "Framework/Common/Common.hpp"

#include <BindlessVk/Context/Surface.hpp>

class Window
{
public:
	struct Specs
	{
		str title;
		pair<u32, u32> size;
	};

	using Hints = vec<pair<i32, i32>>;

public:
	Window() = default;
	Window(Specs const &specs, Hints const &hints);

	Window(Window &&other);
	Window &operator=(Window &&other);

	Window(Window const &other) = delete;
	Window &operator=(Window const &other) = delete;

	~Window();

	void poll_events();

	auto get_required_extensions() const -> vec<c_str>;

	auto should_close() -> bool;

	auto create_vulkan_surface(vk::Instance instance) -> bvk::Surface::WindowSurface;
	auto get_framebuffer_size() -> vk::Extent2D;

	auto get_glfw_handle()
	{
		return glfw_window_handle;
	}

private:
	void bind_callbacks();

private:
	struct GLFWwindow *glfw_window_handle = {};

	scope<Specs> specs = {};

	bool glfw_initialized = {};
};
