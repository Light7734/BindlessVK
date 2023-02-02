#include "BindlessVk/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

Gpu::Gpu(
    vk::PhysicalDevice const physical_device,
    vk::SurfaceKHR const surface,
    vec<c_str> const &required_extensions
)
    : physical_device(physical_device)
    , surface(surface)
{
	calculate_max_sample_counts();
	calculate_queue_indices();
	check_adequacy(required_extensions);
}

void Gpu::calculate_max_sample_counts()
{
	auto const limits = get_properties().limits;

	auto const color_sample_counts = limits.framebufferColorSampleCounts;
	auto const depth_sample_counts = limits.framebufferDepthSampleCounts;
	auto const depth_color_sample_counts = color_sample_counts & depth_sample_counts;

	max_color_samples =
	    color_sample_counts & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :

	    color_sample_counts & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	    color_sample_counts & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	    color_sample_counts & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	    color_sample_counts & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	    color_sample_counts & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                         vk::SampleCountFlagBits::e1;

	max_depth_samples =
	    depth_sample_counts & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	    depth_sample_counts & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	    depth_sample_counts & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	    depth_sample_counts & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	    depth_sample_counts & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	    depth_sample_counts & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                         vk::SampleCountFlagBits::e1;

	max_color_and_depth_samples =
	    depth_color_sample_counts & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	    depth_color_sample_counts & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	    depth_color_sample_counts & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	    depth_color_sample_counts & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	    depth_color_sample_counts & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	    depth_color_sample_counts & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                               vk::SampleCountFlagBits::e1;
}

void Gpu::calculate_queue_indices()
{
	auto const queue_family_properties = physical_device.getQueueFamilyProperties();

	u32 index = 0u;
	for (auto const &queue_family_property : queue_family_properties)
	{
		if (queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics)
			graphics_index = index;

		if (physical_device.getSurfaceSupportKHR(index, surface))
			present_index = index;

		if (queue_family_property.queueFlags & vk::QueueFlagBits::eCompute)
			compute_index = index;

		index++;

		if (graphics_index != VK_QUEUE_FAMILY_IGNORED && present_index != VK_QUEUE_FAMILY_IGNORED &&
		    compute_index != VK_QUEUE_FAMILY_IGNORED)
			break;
	}
}

void Gpu::check_adequacy(vec<c_str> const &required_extensions)
{
	adequate = has_required_features() && has_required_queues() &&
	           has_required_extensions(required_extensions) && can_present_to_surface();
}

auto Gpu::has_required_features() const -> bool
{
	auto const features = get_features();
	return features.geometryShader && features.samplerAnisotropy;
}

auto Gpu::has_required_queues() const -> bool
{
	return graphics_index != VK_QUEUE_FAMILY_IGNORED && present_index != VK_QUEUE_FAMILY_IGNORED &&
	       compute_index != VK_QUEUE_FAMILY_IGNORED;
}

auto Gpu::has_required_extensions(vec<c_str> const &required_extensions) const -> bool
{
	for (auto const &required_extension : required_extensions)
		if (!has_extension(required_extension))
			return false;

	return true;
}

auto Gpu::has_extension(c_str const extension) const -> bool
{
	auto const available_extensions = physical_device.enumerateDeviceExtensionProperties();

	for (auto const &available_extension : available_extensions)
		if (!strcmp(extension, available_extension.extensionName))
			return true;

	return false;
}

auto Gpu::can_present_to_surface() const -> bool
{
	return !physical_device.getSurfacePresentModesKHR(surface).empty();
}

} // namespace BINDLESSVK_NAMESPACE
