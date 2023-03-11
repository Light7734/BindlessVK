#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

class Gpu
{
public:
	Gpu(vk::PhysicalDevice physical_device,
	    vk::SurfaceKHR surface,
	    vec<c_str> const &required_extensions);

	Gpu() = default;

	~Gpu() = default;

	auto create_device(vk::DeviceCreateInfo const &info) const -> vk::Device;

	auto is_adequate() const
	{
		return adequate;
	}

	auto get_properties() const
	{
		return physical_device.getProperties();
	}

	auto get_features() const
	{
		return physical_device.getFeatures();
	}

	auto get_surface_capabilities() const
	{
		return physical_device.getSurfaceCapabilitiesKHR(surface);
	}

	auto get_surface_formats() const
	{
		return physical_device.getSurfaceFormatsKHR(surface);
	}

	auto get_surface_present_modes() const
	{
		return physical_device.getSurfacePresentModesKHR(surface);
	}

	auto get_graphics_queue_index() const
	{
		return graphics_index;
	}

	auto get_present_queue_index() const
	{
		return present_index;
	}

	auto get_format_properties(vk::Format const format) const
	{
		return physical_device.getFormatProperties(format);
	}

	auto get_max_color_samples() const
	{
		return max_color_samples;
	}

	auto get_max_depth_samples() const
	{
		return max_depth_samples;
	}

	auto get_max_color_and_depth_samples() const
	{
		return max_color_and_depth_samples;
	}

	operator vk::PhysicalDevice() const
	{
		return physical_device;
	}

	operator VkPhysicalDevice() const
	{
		return static_cast<VkPhysicalDevice>(physical_device);
	}

private:
	void calculate_max_sample_counts();
	void calculate_queue_indices();

	void check_adequacy(vec<c_str> const &required_extensions);

	auto has_required_features() const -> bool;
	auto has_required_queues() const -> bool;
	auto has_required_extensions(vec<c_str> const &required_extensions) const -> bool;
	auto has_extension(c_str extension) const -> bool;
	auto can_present_to_surface() const -> bool;

private:
	vk::PhysicalDevice physical_device = {};
	vk::SurfaceKHR surface = {};

	vk::SampleCountFlagBits max_color_samples = {};
	vk::SampleCountFlagBits max_depth_samples = {};
	vk::SampleCountFlagBits max_color_and_depth_samples = {};

	u32 graphics_index = VK_QUEUE_FAMILY_IGNORED;
	u32 compute_index = VK_QUEUE_FAMILY_IGNORED;
	u32 present_index = VK_QUEUE_FAMILY_IGNORED;

	bool adequate = {};
};


} // namespace BINDLESSVK_NAMESPACE
