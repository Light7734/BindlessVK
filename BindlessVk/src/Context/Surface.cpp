#include "BindlessVk/Context/Surface.hpp"

namespace BINDLESSVK_NAMESPACE {

void Surface::init_with_instance(
    vk::Instance const instance,
    fn<vk::SurfaceKHR(vk::Instance)> const create_window_surface_func,
    fn<vk::Extent2D()> const fn_get_framebuffer_extent
)
{
	this->instance = instance;
	this->fn_get_framebuffer_extent = fn_get_framebuffer_extent;

	surface = create_window_surface_func(instance);
}

void Surface::init_with_gpu(Gpu const &gpu)
{
	capabilities = gpu.get_surface_capabilities();
	select_best_surface_format(gpu);
	select_best_present_mode(gpu);
	update_framebuffer_extent();
}

void Surface::select_best_surface_format(Gpu const &gpu)
{
	auto const supported_formats = gpu.get_surface_formats();

	auto selected_surface_format = supported_formats[0]; // default
	for (auto const &format : supported_formats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb &&
		    format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			selected_surface_format = format;
			break;
		}
	}
	color_format = selected_surface_format.format;
	color_space = selected_surface_format.colorSpace;
}

void Surface::select_best_present_mode(Gpu const &gpu)
{
	auto const supported_present_modes = gpu.get_surface_present_modes();
	present_mode = supported_present_modes[0]; // default

	for (auto const &supported_present_mode : supported_present_modes)
		if (present_mode == vk::PresentModeKHR::eFifo)
			present_mode = supported_present_mode;
}

void Surface::update_framebuffer_extent()
{
	framebuffer_extent = std::clamp(
	    fn_get_framebuffer_extent(),
	    capabilities.minImageExtent,
	    capabilities.maxImageExtent
	);
}

void Surface::destroy()
{
	instance.destroySurfaceKHR(surface);
}

} // namespace BINDLESSVK_NAMESPACE
