#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/DebugUtils.hpp"
#include "BindlessVk/Context/Device.hpp"
#include "BindlessVk/Context/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan queues */
class Queues
{
public:
	/** Default constructor */
	Queues() = default;

	/** Argumented constructor
	 *
	 * @param device The device
	 * @param gpu The selected gpu
	 */
	Queues(Device *device, Gpu *gpu, DebugUtils *debug_utils);

	/** Default destructor */
	~Queues() = default;

	/** Returrns both queue's indices in an array */
	auto get_indices() const
	{
		return arr<u32, 3> { graphics_index, compute_index, present_index };
	}

	/** Trivial accessor for graphics (queue) */
	auto get_graphics() const
	{
		return graphics;
	}

	/** Trivial accessor for present(queue) */
	auto get_present() const
	{
		return present;
	}

	/** Trivial accessor for compute(queue) */
	auto get_compute() const
	{
		return compute;
	}

	/** Trivial accessor for graphics(queue) index */
	auto get_graphics_index() const
	{
		return graphics_index;
	}

	/** Trivial accessor for present(queue) index */
	auto get_present_index() const
	{
		return present_index;
	}

	/** Trivial accessor for compute(queue) index */
	auto get_compute_index() const
	{
		return compute_index;
	}

	/** Checks if all queues are from the same family (index) */
	auto have_same_index() const
	{
		return compute_index == graphics_index && compute_index == present_index;
	}

private:
	u32 graphics_index = {};
	u32 present_index = {};
	u32 compute_index = {};

	vk::Queue graphics = {};
	vk::Queue present = {};
	vk::Queue compute = {};
};

} // namespace BINDLESSVK_NAMESPACE
