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
}

VkContext::~VkContext()
{
}

} // namespace BINDLESSVK_NAMESPACE
