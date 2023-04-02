#include "BindlessVk/Context/Device.hpp"

namespace BINDLESSVK_NAMESPACE {

Device::Device(Gpu *gpu)
{
	auto indexing_features = vk::PhysicalDeviceDescriptorIndexingFeaturesEXT {};
	indexing_features.descriptorBindingPartiallyBound = true;
	indexing_features.runtimeDescriptorArray = true;

	auto const dynamic_rendering_features = vk::PhysicalDeviceDynamicRenderingFeatures {
		true,
		&indexing_features,
	};

	auto const queues_info = create_queues_create_infos(gpu);
	auto const requirements = gpu->get_requirements();

	device = gpu->vk().createDevice(vk::DeviceCreateInfo {
	    {},
	    queues_info,
	    {},
	    requirements.logical_device_extensions,
	    &requirements.physical_device_features,
	    &dynamic_rendering_features,
	});

	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
	graphics_queue = device.getQueue(gpu->get_graphics_queue_index(), 0);

	immediate_cmd_pool = device.createCommandPool(vk::CommandPoolCreateInfo {
	    {},
	    gpu->get_graphics_queue_index(),
	});

	immediate_fence = device.createFence(vk::FenceCreateInfo {});
}

Device::Device(Device &&other)
{
	*this = std::move(other);
}

Device &Device::operator=(Device &&other)
{
	this->device = other.device;
	this->graphics_queue = other.graphics_queue;
	this->immediate_cmd_pool = other.immediate_cmd_pool;
	this->immediate_fence = other.immediate_fence;

	other.device = vk::Device {};

	return *this;
}

Device::~Device()
{
	if (!device)
		return;

	device.waitIdle();
	device.destroyFence(immediate_fence);
	device.destroyCommandPool(immediate_cmd_pool);
	device.destroy();
}

void Device::immediate_submit(fn<void(vk::CommandBuffer)> &&func) const
{
	auto const alloc_info = vk::CommandBufferAllocateInfo {
		immediate_cmd_pool,
		vk::CommandBufferLevel::ePrimary,
		1u,
	};

	auto const cmd = device.allocateCommandBuffers(alloc_info)[0];

	auto const beginInfo =
	    vk::CommandBufferBeginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };

	cmd.begin(beginInfo);
	func(cmd);
	cmd.end();

	auto const submit_info = vk::SubmitInfo { 0u, {}, {}, 1u, &cmd, 0u, {}, {} };
	graphics_queue.submit(submit_info, immediate_fence);

	assert_false(device.waitForFences(immediate_fence, true, UINT_MAX));
	device.resetFences(immediate_fence);
	device.resetCommandPool(immediate_cmd_pool);
}

auto Device::create_queues_create_infos(Gpu *gpu) const -> vec<vk::DeviceQueueCreateInfo>
{
	auto static constexpr queue_priority = arr<f32, 1> { 1.0 };

	auto const graphics_queue_index = gpu->get_graphics_queue_index();
	auto const present_queue_index = gpu->get_present_queue_index();

	auto create_infos = vec<vk::DeviceQueueCreateInfo> {
		{
		    {},
		    graphics_queue_index,
		    queue_priority,
		},
	};

	if (graphics_queue_index != present_queue_index)
		create_infos.push_back(vk::DeviceQueueCreateInfo {
		    {},
		    present_queue_index,
		    queue_priority,
		});

	return create_infos;
}


} // namespace BINDLESSVK_NAMESPACE
