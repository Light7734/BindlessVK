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

	inline vk::Device create_device(vk::DeviceCreateInfo const &info) const
	{
		return physical_device.createDevice(info);
	}

	inline auto is_adequate() const
	{
		return adequate;
	}

	inline auto get_properties() const
	{
		return physical_device.getProperties();
	}

	inline auto get_features() const
	{
		return physical_device.getFeatures();
	}

	inline auto get_surface_capabilities() const
	{
		return physical_device.getSurfaceCapabilitiesKHR(surface);
	}

	inline auto get_surface_formats() const
	{
		return physical_device.getSurfaceFormatsKHR(surface);
	}

	inline auto get_surface_present_modes() const
	{
		return physical_device.getSurfacePresentModesKHR(surface);
	}

	inline auto get_graphics_queue_index() const
	{
		return graphics_index;
	}

	inline auto get_present_queue_index() const
	{
		return present_index;
	}

	inline auto get_format_properties(vk::Format const format) const
	{
		return physical_device.getFormatProperties(format);
	}

	inline auto get_max_color_samples() const
	{
		return max_color_samples;
	}

	inline auto get_max_depth_samples() const
	{
		return max_depth_samples;
	}

	inline auto get_max_color_and_depth_samples() const
	{
		return max_color_and_depth_samples;
	}

	inline operator vk::PhysicalDevice() const
	{
		return physical_device;
	}

	inline operator VkPhysicalDevice() const
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
