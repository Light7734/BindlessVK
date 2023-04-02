#include "BindlessVk/Context/Queues.hpp"

namespace BINDLESSVK_NAMESPACE {

Queues::Queues(Device *device, Gpu *gpu)
    : graphics_index(gpu->get_graphics_queue_index())
    , present_index(gpu->get_present_queue_index())
    , graphics(device->vk().getQueue(graphics_index, 0))
    , present(device->vk().getQueue(present_index, 0))
{
}

} // namespace BINDLESSVK_NAMESPACE
