#include "BindlessVk/Context/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

Gpu::Gpu(
    vk::PhysicalDevice const physical_device,
    vk::SurfaceKHR const surface,
    Requirements const &requirements
)
    : physical_device(physical_device)
    , surface(surface)
    , requirements(requirements)
{
	calculate_max_sample_counts();
	calculate_queue_indices();
	check_adequacy();
}

/* static */
auto Gpu::pick_by_score(
    Instance *instance,
    vk::SurfaceKHR const surface,
    Requirements const requirements,
    fn<u32(Gpu)> const calculate_score
) -> Gpu
{
	auto adequate_gpus = vec<Gpu> {};
	auto scores = vec<u32> {};

	auto high_score = u32 { 0 };
	auto high_score_index = u32 { 0 };

	auto const gpus = instance->vk().enumeratePhysicalDevices();
	for (u32 i = 0; auto const physical_device : gpus)
	{
		auto const gpu = Gpu(physical_device, surface, requirements);
		if (!gpu.is_adequate())
			continue;

		adequate_gpus.push_back(gpu);
		auto const score = calculate_score(gpu);
		if (score > high_score)
		{
			high_score = score;
			high_score_index = i;
		}

		++i;
	}

	assert_false(adequate_gpus.empty(), "No suitable gpu found");

	return adequate_gpus[high_score_index];
}

void Gpu::calculate_max_sample_counts()
{
	auto const limits = physical_device.getProperties().limits;

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
			graphics_queue_index = index;

		if (physical_device.getSurfaceSupportKHR(index, surface))
			present_queue_index = index;

		index++;

		if (graphics_queue_index != VK_QUEUE_FAMILY_IGNORED &&
		    present_queue_index != VK_QUEUE_FAMILY_IGNORED)
			break;
	}
}

void Gpu::check_adequacy()
{
	adequate = has_required_features() && has_required_queues() && has_required_extensions() &&
	           can_present_to_surface();
}

auto Gpu::has_required_features() const -> bool
{
	auto const required_features = requirements.physical_device_features;
	auto const required_features_arr = vec<vk::Bool32>(
	    reinterpret_cast<vk::Bool32 const *>(&required_features.robustBufferAccess),
	    reinterpret_cast<vk::Bool32 const *>(&required_features.inheritedQueries)
	);

	auto supported_features = physical_device.getFeatures();
	auto const supported_features_arr = vec<vk::Bool32>(
	    reinterpret_cast<vk::Bool32 const *>(&supported_features.robustBufferAccess),
	    reinterpret_cast<vk::Bool32 const *>(&supported_features.inheritedQueries)
	);

	for (u32 i = 0; i < required_features_arr.size(); i++)
		if (required_features_arr[i] && !supported_features_arr[i])
			return false;

	return true;
}

auto Gpu::has_required_queues() const -> bool
{
	return graphics_queue_index != VK_QUEUE_FAMILY_IGNORED &&
	       present_queue_index != VK_QUEUE_FAMILY_IGNORED;
}

auto Gpu::has_required_extensions() const -> bool
{
	for (auto const &required_extension : requirements.logical_device_extensions)
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
