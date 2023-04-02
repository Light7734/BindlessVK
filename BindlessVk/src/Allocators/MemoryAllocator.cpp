#include "BindlessVk/Allocators/MemoryAllocator.hpp"

namespace BINDLESSVK_NAMESPACE {

void MemoryAllocator::allocate_memory_callback(
    VmaAllocator const VMA_NOT_NULL allocator,
    u32 const memory_type,
    VkDeviceMemory const VMA_NOT_NULL_NON_DISPATCHABLE memory,
    VkDeviceSize const size,
    void *const VMA_NULLABLE vma_user_data
)
{
	auto const [callback, user_data] = *reinterpret_cast<DebugUtils::Callback *>(vma_user_data);

	callback(
	    DebugCallbackSource::eVma,
	    LogLvl::eTrace,
	    fmt::format("Allocate: {}", size),
	    user_data
	);
}

void MemoryAllocator::free_memory_callback(
    VmaAllocator const VMA_NOT_NULL allocator,
    u32 const memory_type,
    VkDeviceMemory const VMA_NOT_NULL_NON_DISPATCHABLE memory,
    VkDeviceSize const size,
    void *const VMA_NULLABLE vma_user_data
)
{
	auto const [callback, user_data] = *reinterpret_cast<DebugUtils::Callback *>(vma_user_data);
	callback(DebugCallbackSource::eVma, LogLvl::eTrace, fmt::format("Free: {}", size), user_data);
}


MemoryAllocator::MemoryAllocator(VkContext const *vk_context)
{
	auto const gpu = vk_context->get_gpu();
	auto const debug_utils = vk_context->get_debug_utils();
	auto const device = vk_context->get_device();
	auto const instance = vk_context->get_instance();

	auto const vulkan_funcs = vma::VulkanFunctions(
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkAllocateMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkFreeMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkMapMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkUnmapMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkFlushMappedMemoryRanges,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkInvalidateMappedMemoryRanges,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateImage,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyImage,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdCopyBuffer,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetBufferMemoryRequirements2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetImageMemoryRequirements2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindBufferMemory2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkBindImageMemory2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceMemoryProperties2KHR,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceBufferMemoryRequirements,
	    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceImageMemoryRequirements
	);

	auto const device_memory_callbacks = vma::DeviceMemoryCallbacks {
		&allocate_memory_callback,
		&free_memory_callback,
		debug_utils->get_callback(),
	};

	auto const allocator_info = vma::AllocatorCreateInfo(
	    {},
	    gpu->vk(),
	    device->vk(),
	    {},
	    {},
	    &device_memory_callbacks,
	    {},
	    &vulkan_funcs,
	    instance->vk(),
	    {}
	);

	allocator = vma::createAllocator(allocator_info);
}

MemoryAllocator::MemoryAllocator(MemoryAllocator &&other)
{
	*this = std::move(other);
}

MemoryAllocator &MemoryAllocator::operator=(MemoryAllocator &&other)
{
	this->allocator = other.allocator;
	other.allocator = vma::Allocator {};

	return *this;
}

MemoryAllocator::~MemoryAllocator()
{
	if (!allocator)
		return;

	allocator.destroy();
}

} // namespace BINDLESSVK_NAMESPACE
