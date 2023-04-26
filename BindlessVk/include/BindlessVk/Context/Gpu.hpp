#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Instance.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan physical device */
class Gpu
{
public:
	/** Requirements  that needs to be met for Gpu to be considered adaquate */
	struct Requirements
	{
		vk::PhysicalDeviceFeatures physical_device_features;
		vec<c_str> logical_device_extensions;
	};

public:
	/** Default constructor */
	Gpu() = default;

	/** Argumented constructor
	 *
	 * @param physiacl_device The vulkan physical device
	 * @param surface The vulkan window surface
	 * @param requirements The requirements that the gpu needs to have to be considered adequate
	 */
	Gpu(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, Requirements const &requirements
	);

	/** Default destructor */
	~Gpu() = default;

	/** Picks the highest scoring gpu.
	 * Loops through available gpus and scores them based on @a calculate_score function,
	 * then returns the highest scoring one.
	 *
	 * @param instance The bvk wrapper around vulkan instance
	 * @param surface The vulkan window surface
	 * @param requirements The requirements that the gpu needs to be considered adequate
	 * @param calculate_score function to calculate a gpu's score
	 *
	 * @returns Highest scoring gpu based on @a calculate_score function that meets all the @a
	 * requirements
	 */
	static auto pick_by_score(
	    Instance *instance,
	    vk::SurfaceKHR surface,
	    Requirements requirements,
	    fn<u32(Gpu)> calculate_score
	) -> Gpu;

	/** @brief Trivial accessor for the wrapped vulkan physical device */
	auto vk() const
	{
		return physical_device;
	}

	/** @brief Trivial accessor for vk suurface (use the Surface class in client code instead) */
	auto get_vulkan_surface() const
	{
		return surface;
	}

	/** @brief Trivial accessor for requirements */
	auto get_requirements() const
	{
		return requirements;
	}

	/** @brief Trivial accessor for adequate */
	auto is_adequate() const
	{
		return adequate;
	}

	/** @brief Trivial accessor for adequate */
	auto get_graphics_queue_index() const
	{
		return graphics_queue_index;
	}

	/** @brief Trivial accessor for present_queue_index */
	auto get_present_queue_index() const
	{
		return present_queue_index;
	}

	/** @brief Trivial accessor for compute_queue_index */
	auto get_compute_queue_index() const
	{
		return compute_queue_index;
	}

	/** @brief Trivial accessor for max_color_samples */
	auto get_max_color_samples() const
	{
		return max_color_samples;
	}

	/** @brief Trivial accessor for max_depth_samples */
	auto get_max_depth_samples() const
	{
		return max_depth_samples;
	}

	/** @brief Trivial accessor for  max_color_and_depth_samples */
	auto get_max_color_and_depth_samples() const
	{
		return max_color_and_depth_samples;
	}

private:
	void calculate_max_sample_counts();
	void calculate_queue_indices();

	void check_adequacy();

	auto has_required_features() const -> bool;
	auto has_required_queues() const -> bool;
	auto has_required_extensions() const -> bool;
	auto has_extension(c_str extension) const -> bool;
	auto can_present_to_surface() const -> bool;

	auto create_queues_create_infos() const -> vec<vk::DeviceQueueCreateInfo>;

private:
	vk::PhysicalDevice physical_device = {};
	vk::SurfaceKHR surface = {};

	Requirements requirements = {};

	vk::SampleCountFlagBits max_color_samples = {};
	vk::SampleCountFlagBits max_depth_samples = {};
	vk::SampleCountFlagBits max_color_and_depth_samples = {};

	u32 graphics_queue_index = VK_QUEUE_FAMILY_IGNORED;
	u32 present_queue_index = VK_QUEUE_FAMILY_IGNORED;
	u32 compute_queue_index = VK_QUEUE_FAMILY_IGNORED;

	bool adequate = {};
};

} // namespace BINDLESSVK_NAMESPACE
