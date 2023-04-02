#define VMA_IMPLEMENTATION

#include "BindlessVk/Context/VkContext.hpp"

#include "vulkan/vulkan_raii.hpp"

namespace BINDLESSVK_NAMESPACE {

VkContext::VkContext(
    Instance *instance,
    DebugUtils *debug_utils,
    Surface *surface,
    Gpu *gpu,
    Queues *queues,
    Device *device
)
    : instance(instance)
    , debug_utils(debug_utils)
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
