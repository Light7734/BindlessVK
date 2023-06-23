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
	ZoneScoped;

	select_best_surface_format(calculate_format_score, gpu);
	select_best_present_mode(calculate_present_mode_score, gpu);
	select_best_depth_format(gpu);
	update_framebuffer_extent();
}

void Surface::select_best_surface_format(
    fn<u32(vk::SurfaceFormatKHR)> calculate_format_score,
    Gpu const *const gpu
)
{
	ZoneScoped;

	auto const supported_formats = gpu->vk().getSurfaceFormatsKHR(surface);

	u32 high_score = 0;
	color_format = supported_formats[0].format;
	color_space = supported_formats[0].colorSpace;

	for (auto const &format : supported_formats)
	{
		if (u32 new_score = calculate_format_score(format); new_score > high_score)
		{
			high_score = new_score;
			color_format = format.format;
			color_space = format.colorSpace;
		}
	}
}

void Surface::select_best_present_mode(
    fn<u32(vk::PresentModeKHR)> calculate_present_mode,
    Gpu const *const gpu
)
{
	ZoneScoped;

	auto const supported_present_modes = gpu->vk().getSurfacePresentModesKHR(surface);

	u32 high_score = 0;
	present_mode = supported_present_modes[0];

	for (auto const &mode : supported_present_modes)
	{
		if (u32 new_score = calculate_present_mode(mode); new_score > high_score)
		{
			high_score = new_score;
			present_mode = mode;
		}
	}
}

void Surface::select_best_depth_format(Gpu const *const gpu)
{
	ZoneScoped;

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
		if ((format_properties.optimalTilingFeatures
		     & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		    == vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			depth_format = format;
	}
}

void Surface::update_framebuffer_extent()
{
	ZoneScoped;

	framebuffer_extent = std::clamp(
	    fn_get_framebuffer_extent(),
	    capabilities.minImageExtent,
	    capabilities.maxImageExtent
	);
}

} // namespace BINDLESSVK_NAMESPACE
