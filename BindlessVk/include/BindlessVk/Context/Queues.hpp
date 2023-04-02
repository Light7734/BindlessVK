#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Device.hpp"
#include "BindlessVk/Context/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

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
	Queues(Device *device, Gpu *gpu);

	/** Default destructor */
	~Queues() = default;

	/** Returrns both queue's indices in an array */
	auto get_indices() const
	{
		return arr<u32, 2> { graphics_index, present_index };
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

	/** Checks if both queues are from the same family (index) */
	auto have_same_index() const
	{
		return graphics_index == present_index;
	}

private:
	u32 graphics_index = {};
	u32 present_index = {};

	vk::Queue graphics = {};
	vk::Queue present = {};
};

} // namespace BINDLESSVK_NAMESPACE
