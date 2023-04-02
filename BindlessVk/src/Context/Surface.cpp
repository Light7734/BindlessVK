#include "BindlessVk/Context/Surface.hpp"

namespace BINDLESSVK_NAMESPACE {

Surface::Surface(
    WindowSurface *const window_surface,
    Gpu const *const gpu,
    fn<vk::Extent2D()> get_framebuffer_extent,
    fn<u32(vk::PresentModeKHR)> calculate_present_mode_score,
    fn<u32(vk::SurfaceFormatKHR)> calculate_format_score
)
    : surface(window_surface->surface)
    , capabilities(gpu->vk().getSurfaceCapabilitiesKHR(surface))
    , fn_get_framebuffer_extent(get_framebuffer_extent)
{
	select_best_surface_format(calculate_format_score, gpu);
	select_best_present_mode(calculate_present_mode_score, gpu);
	select_best_depth_format(gpu);
	update_framebuffer_extent();
}

Surface::Surface(Surface &&other)
{
	*this = std::move(other);
}

Surface &Surface::operator=(Surface &&other)
{
	this->surface = other.surface;
	this->capabilities = other.capabilities;
	this->color_format = other.color_format;
	this->depth_format = other.depth_format;
	this->color_space = other.color_space;
	this->present_mode = other.present_mode;
	this->framebuffer_extent = other.framebuffer_extent;
	this->fn_get_framebuffer_extent = other.fn_get_framebuffer_extent;

	return *this;
}

void Surface::select_best_surface_format(
    fn<u32(vk::SurfaceFormatKHR)> calculate_format_score,
    Gpu const *const gpu
)
{
	auto const supported_formats = gpu->vk().getSurfaceFormatsKHR(surface);

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

void Surface::select_best_present_mode(
    fn<u32(vk::PresentModeKHR)> calculate_present_mode,
    Gpu const *const gpu
)
{
	auto const supported_present_modes = gpu->vk().getSurfacePresentModesKHR(surface);
	present_mode = supported_present_modes[0]; // default

	for (auto const &supported_present_mode : supported_present_modes)
		if (present_mode == vk::PresentModeKHR::eFifo)
			present_mode = supported_present_mode;
}

void Surface::select_best_depth_format(Gpu const *const gpu)
{
	auto const properties = gpu->vk().getProperties();
	auto const formats = arr<vk::Format, 3> {
		vk::Format::eD32Sfloat,
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD24UnormS8Uint,
	};

	depth_format = formats[0]; // default
	for (auto const format : formats)
	{
		auto const format_properties = gpu->vk().getFormatProperties(format);
		if ((format_properties.optimalTilingFeatures &
		     vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
		    vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			depth_format = format;
	}
}

void Surface::update_framebuffer_extent()
{
	framebuffer_extent = std::clamp(
	    fn_get_framebuffer_extent(),
	    capabilities.minImageExtent,
	    capabilities.maxImageExtent
	);
}

} // namespace BINDLESSVK_NAMESPACE
