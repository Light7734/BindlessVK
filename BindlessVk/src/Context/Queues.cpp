#include "BindlessVk/Context/Queues.hpp"

#include "Amender/Amender.hpp"

namespace BINDLESSVK_NAMESPACE {

Queues::Queues(Device *device, Gpu *gpu)
    : compute_index(gpu->get_compute_queue_index())
    , graphics_index(gpu->get_graphics_queue_index())
    , present_index(gpu->get_present_queue_index())

    , compute(device->vk().getQueue(compute_index, 0))
    , graphics(device->vk().getQueue(graphics_index, 0))
    , present(device->vk().getQueue(present_index, 0))
{
	ScopeProfiler _;

	log_inf("compute queue index: {}", compute_index);
	log_inf("graphics queue index: {}", graphics_index);
	log_inf("present queue index: {}", present_index);
}

} // namespace BINDLESSVK_NAMESPACE
