#include "BindlessVk/Context/Queues.hpp"

namespace BINDLESSVK_NAMESPACE {

Queues::Queues(Device *device, Gpu *gpu, DebugUtils *debug_utils)
    : compute_index(gpu->get_compute_queue_index())
    , graphics_index(gpu->get_graphics_queue_index())
    , present_index(gpu->get_present_queue_index())

    , compute(device->vk().getQueue(compute_index, 0))
    , graphics(device->vk().getQueue(graphics_index, 0))
    , present(device->vk().getQueue(present_index, 0))
{
	debug_utils->log(LogLvl::eInfo, "compute queue index: {}", compute_index);
	debug_utils->log(LogLvl::eInfo, "graphics queue index: {}", graphics_index);
	debug_utils->log(LogLvl::eInfo, "present queue index: {}", present_index);
}

} // namespace BINDLESSVK_NAMESPACE
