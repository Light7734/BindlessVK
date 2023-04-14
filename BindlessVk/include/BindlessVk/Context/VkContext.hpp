#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/DebugUtils.hpp"
#include "BindlessVk/Context/Device.hpp"
#include "BindlessVk/Context/Gpu.hpp"
#include "BindlessVk/Context/Instance.hpp"
#include "BindlessVk/Context/Queues.hpp"
#include "BindlessVk/Context/Surface.hpp"

#include <any>

namespace BINDLESSVK_NAMESPACE {

class VkContext
{
public:
	/** Default constructor */
	VkContext() = default;

	/** Argumented constructor
	 *
	 * @param instance The vulkan instance wrapper
	 * @param debug_utils The debug utils wrapper
	 * @param surface The vulkan surface wrapper
	 * @param gpu The vulkan physical device wrapper
	 * @param queues The vulkan queues wrapper
	 * @param device The vulkan device wrapper
	 */
	VkContext(
	    Instance *instance,
	    DebugUtils *debug_utils,
	    Surface *surface,
	    Gpu *gpu,
	    Queues *queues,
	    Device *device
	);

	/** Destructor */
	~VkContext();

	/** Trivial accessor for instance */
	auto get_instance() const
	{
		return instance;
	}

	/** Trivial accessor for debug_utils */
	auto get_debug_utils() const
	{
		return debug_utils;
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

	/** Trivial accessor for num_threads */
	auto get_num_threads() const
	{
		return num_threads;
	}

private:
	Instance *instance = {};
	DebugUtils *debug_utils = {};
	Device *device = {};
	Surface *surface = {};
	Gpu *gpu = {};
	Queues *queues = {};

	u32 num_threads = 1u;
};


} // namespace BINDLESSVK_NAMESPACE
