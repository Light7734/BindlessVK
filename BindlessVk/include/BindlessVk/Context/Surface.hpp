
#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

class Surface
{
public:
	void init_with_instance(
	    vk::Instance instance,
	    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
	    fn<vk::Extent2D()> fn_get_framebuffer_extent
	);

	void init_with_gpu(Gpu const &gpu);

	void destroy();

	auto get_capabilities() const
	{
		return capabilities;
	}

	auto get_color_format() const
	{
		return color_format;
	}

	auto get_color_space() const
	{
		return color_space;
	}

	auto get_present_mode() const
	{
		return present_mode;
	}

	auto get_framebuffer_extent() const
	{
		return framebuffer_extent;
	}

	operator vk::SurfaceKHR() const
	{
		return surface;
	}

private:
	void select_best_present_mode(Gpu const &gpu);
	void select_best_surface_format(Gpu const &gpu);
	void update_framebuffer_extent();

private:
	vk::Instance instance = {};
	vk::SurfaceKHR surface = {};
	vk::SurfaceCapabilitiesKHR capabilities = {};
	vk::Format color_format = {};
	vk::ColorSpaceKHR color_space = {};
	vk::PresentModeKHR present_mode = {};
	vk::Extent2D framebuffer_extent = {};

	fn<vk::Extent2D()> fn_get_framebuffer_extent = {};
};

} // namespace BINDLESSVK_NAMESPACE
