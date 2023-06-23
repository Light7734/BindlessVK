#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Device.hpp"
#include "BindlessVk/Context/Gpu.hpp"
#include "BindlessVk/Context/Instance.hpp"
#include "BindlessVk/Context/Queues.hpp"
#include "BindlessVk/Context/Surface.hpp"

#include <tracy/TracyVulkan.hpp>

namespace BINDLESSVK_NAMESPACE {

struct TracyContext
{
	tracy::VkCtx *context;
	vk::CommandPool pool;
	vk::CommandBuffer cmd;
};

/** Helper class for accessing preliminary vulkan wrapper */
class VkContext
{
public:
	/** Default constructor */
	VkContext() = default;

	/** Argumented constructor
	 *
	 * @param instance The vulkan instance wrapper
	 * @param surface The vulkan surface wrapper
	 * @param gpu The vulkan physical device wrapper
	 * @param queues The vulkan queues wrapper
	 * @param device The vulkan device wrapper
	 */
	VkContext(Instance *instance, Surface *surface, Gpu *gpu, Queues *queues, Device *device);

	/** Default move constructor */
	VkContext(VkContext &&other) = default;

	/** Default move assignment operator */
	VkContext &operator=(VkContext &&other) = default;

	/** Destructor */
	~VkContext();

	/** Trivial accessor for instance */
	auto get_instance() const
	{
		return instance;
	}

	/** Trivial accessor for device */
	auto get_device() const
	{
		return device;
	}

	/** Trivial accessor for surface */
	auto get_surface() const
	{
		return surface;
	}

	/** Trivial accessor for gpu */
	auto get_gpu() const
	{
		return gpu;
	}

	/** Trivial accessor for queues */
	auto get_queues() const
	{
		return queues;
	}

	/** Trivial accessor for tracy_graphics */
	auto get_tracy_graphics() const
	{
		return tracy_graphics;
	}

	/** Trivial accessor for tracy_compute */
	auto get_tracy_compute() const
	{
		return tracy_compute;
	}

	/** Trivial accessor for num_threads */
	auto get_num_threads() const
	{
		return num_threads;
	}

private:
	auto create_tracy_context_for_queue(vk::Queue queue, u32 queue_index) -> TracyContext;

private:
	tidy_ptr<Instance> instance = {};

	Device *device = {};
	Surface *surface = {};
	Gpu *gpu = {};
	Queues *queues = {};

	TracyContext tracy_graphics;
	TracyContext tracy_compute;

	u32 num_threads = 1u;
};


} // namespace BINDLESSVK_NAMESPACE
