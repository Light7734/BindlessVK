#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Gpu.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Wrapper around vulkan logical device */
class Device
{
public:
	/** Default constructor */
	Device() = default;

	/** Argumented constructor
	 *
	 * @param gpu The selected gpu to create the logical device from
	 */
	Device(Gpu *gpu);

	/** Move constructor */
	Device(Device &&other);

	/** Move assignment operator */
	Device &operator=(Device &&other);

	/** Deleted copy constructor */
	Device(Device const &) = delete;

	/** Deleted copy assignment operator */
	Device &operator=(Device const &) = delete;

	/** Desctructor */
	~Device();

	/** Submits the commands recorded to a command buffer by @a func
	 *
	 * @param func A function that writes to a command buffer
	 * @param queue Which queue to submit the commands to (graphics/compute)
	 *
	 * @warn Blocks the execution to wait for the graphics queue to finish executing the
	 * submitted workload
	 */
	void immediate_submit(
	    fn<void(vk::CommandBuffer)> &&func,
	    vk::QueueFlags queue = vk::QueueFlagBits::eGraphics
	) const;

	/** Trivial accessor for the underyling device */
	auto vk() const
	{
		return device;
	}

private:
	auto create_queues_create_infos(Gpu *gpu) const -> vec<vk::DeviceQueueCreateInfo>;

private:
	vk::Device device = {};

	vk::Queue graphics_queue = {};
	vk::Queue compute_queue = {};

	vk::CommandPool immediate_cmd_pool = {};
	vk::Fence immediate_fence = {};
};

} // namespace BINDLESSVK_NAMESPACE
