#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

class Surface
{
public:
	/** We need this so that we can clean up surface in case gpu selection fails */
	class WindowSurface
	{
	public:
		vk::SurfaceKHR surface = {};
		vk::Instance instance = {};

		WindowSurface() = default;

		WindowSurface(vk::SurfaceKHR const surface, vk::Instance const instance)
		    : surface(surface)
		    , instance(instance)
		{
		}

		WindowSurface(WindowSurface &&other)
		{
			*this = std::move(other);
		}

		WindowSurface &operator=(WindowSurface &&other)
		{
			this->surface = other.surface;
			this->instance = other.instance;

			other.instance = vk::Instance {};

			return *this;
		}

		WindowSurface(WindowSurface const &) = delete;
		WindowSurface &operator=(WindowSurface const &) = delete;

		~WindowSurface()
		{
			if (instance)
				instance.destroySurfaceKHR(surface);
		}
	};

public:
	/** Default constructor */
	Surface() = default;

	/** Argumented constructor
	 * Highest scoring present mode and surface format is selected using the *_score function's
	 * calculated score
	 *
	 * @param window_surface Pointer to the window surface
	 * @param gpu Pointer to the selected gpu
	 * @param calculate_present_mode_score Function that returns the window's framebuffer extent
	 * @param calculate_present_mode_score Function that calculates the score of a present mode
	 * @param calculate_present_mode_score Function that calculates the score of a surface format
	 * */
	Surface(
	    WindowSurface *window_surface,
	    Gpu const *gpu,
	    fn<vk::Extent2D()> get_framebuffer_extent,
	    fn<u32(vk::PresentModeKHR)> calculate_present_mode_score,
	    fn<u32(vk::SurfaceFormatKHR)> calculate_format_score
	);

	/** Move constructor */
	Surface(Surface &&other);

	/** Move assignment operator */
	Surface &operator=(Surface &&other);

	/** Deleted copy constructor */
	Surface(Surface const &) = delete;

	/** Deleted copy assignment operator */
	Surface &operator=(Surface const &) = delete;

	/** Trivial accessor for the underyling surface */
	auto vk() const
	{
		return surface;
	}

	/** Trivial accessor for capablities */
	auto get_capabilities() const
	{
		return capabilities;
	}

	/** Trivial accessor for color_format */
	auto get_color_format() const
	{
		return color_format;
	}

	/** Trivial accessor for depth_format */
	auto get_depth_format() const
	{
		return depth_format;
	}

	/** Trivial accessor for color_space */
	auto get_color_space() const
	{
		return color_space;
	}

	/** Trivial accessor for present_mode */
	auto get_present_mode() const
	{
		return present_mode;
	}

	/** Trivial accessor for framebuffer_extent */
	auto get_framebuffer_extent() const
	{
		return framebuffer_extent;
	}

	/** Returns wether or not the depth format has a stencil component */
	auto get_depth_format_has_stencil() const
	{
		return depth_format == vk::Format::eD32SfloatS8Uint ||
		       depth_format == vk::Format::eD24UnormS8Uint;
	}

private:
	void select_best_present_mode(
	    fn<u32(vk::PresentModeKHR)> calculate_present_mode,
	    Gpu const *gpu
	);
	void select_best_surface_format(
	    fn<u32(vk::SurfaceFormatKHR)> calculate_format_score,
	    Gpu const *gpu
	);
	void select_best_depth_format(Gpu const *gpu);
	void update_framebuffer_extent();

private:
	vk::SurfaceKHR surface = {};
	vk::SurfaceCapabilitiesKHR capabilities = {};
	vk::Format color_format = {};
	vk::Format depth_format = {};
	vk::ColorSpaceKHR color_space = {};
	vk::PresentModeKHR present_mode = {};
	vk::Extent2D framebuffer_extent = {};

	fn<vk::Extent2D()> fn_get_framebuffer_extent = {};
};

} // namespace BINDLESSVK_NAMESPACE
