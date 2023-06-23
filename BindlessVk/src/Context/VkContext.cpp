#define VMA_IMPLEMENTATION

#include "BindlessVk/Context/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

VkContext::VkContext(Instance *instance, Surface *surface, Gpu *gpu, Queues *queues, Device *device)
    : instance(instance)
    , surface(surface)
    , gpu(gpu)
    , queues(queues)
    , device(device)
{
	ZoneScoped;

	tracy_graphics = create_tracy_context_for_queue(
	    queues->get_graphics(),
	    queues->get_graphics_index()
	);

	tracy_compute = create_tracy_context_for_queue(
	    queues->get_compute(),
	    queues->get_compute_index()
	);
}

VkContext::~VkContext()
{
	if (!instance)
		return;

	ZoneScoped;

	device->vk().destroyCommandPool(tracy_graphics.pool);
	device->vk().destroyCommandPool(tracy_compute.pool);

	TracyVkDestroy(tracy_graphics.context);
	TracyVkDestroy(tracy_compute.context);
}

auto VkContext::create_tracy_context_for_queue(vk::Queue queue, u32 queue_index) -> TracyContext
{
	ZoneScoped;

	auto context = TracyContext {};

	auto pool_info = vk::CommandPoolCreateInfo {
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queues->get_graphics_index(),
		{},
	};

	context.pool = device->vk().createCommandPool(pool_info);

	auto const cmd_alloc_info = vk::CommandBufferAllocateInfo {
		context.pool,
		vk::CommandBufferLevel::ePrimary,
		1,
	};
	context.cmd = device->vk().allocateCommandBuffers(cmd_alloc_info)[0];

	context.context = TracyVkContextCalibrated(
	    gpu->vk(),
	    device->vk(),
	    queues->get_graphics(),
	    context.cmd,
	    (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)device->vk().getProcAddr(
	        "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT"
	    ),
	    (PFN_vkGetCalibratedTimestampsEXT)device->vk().getProcAddr("vkGetCalibratedTimestampsEXT")
	);

	return context;
}

} // namespace BINDLESSVK_NAMESPACE
