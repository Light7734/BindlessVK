#include "BindlessVk/Context/Queues.hpp"

namespace BINDLESSVK_NAMESPACE {

void Queues::init_with_gpu(Gpu const &gpu)
{
	graphics_index = gpu.get_graphics_queue_index();
	present_index = gpu.get_present_queue_index();
}

void Queues::init_with_device(vk::Device device)
{
	graphics = device.getQueue(graphics_index, 0);
	assert_true(graphics, "Failed to fetch graphics queue");

	present = device.getQueue(present_index, 0);
	assert_true(present, "Failed to fetch present queue");
}

auto Queues::get_create_infos() -> vec<vk::DeviceQueueCreateInfo>
{
	auto static constexpr queue_priority = arr<f32, 1> { 1.0 };

	auto create_info = vec<vk::DeviceQueueCreateInfo> {
		{ {}, graphics_index, queue_priority },
	};

	if (!have_same_index())
		create_info.push_back({ {}, present_index, queue_priority });

	return create_info;
}

} // namespace BINDLESSVK_NAMESPACE
