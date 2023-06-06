#define VMA_IMPLEMENTATION

#include "BindlessVk/Context/VkContext.hpp"

#include "Amender/Amender.hpp"

namespace BINDLESSVK_NAMESPACE {

VkContext::VkContext(Instance *instance, Surface *surface, Gpu *gpu, Queues *queues, Device *device)
    : instance(instance)
    , surface(surface)
    , gpu(gpu)
    , queues(queues)
    , device(device)
{
	ScopeProfiler _;
}

VkContext::~VkContext()
{
	ScopeProfiler _;
}

} // namespace BINDLESSVK_NAMESPACE
