#define VMA_IMPLEMENTATION

#include "BindlessVk/VkContext.hpp"

#include "vulkan/vulkan_raii.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace BINDLESSVK_NAMESPACE {

/// Callback function called after successful vkAllocateMemory.
static void vma_allocate_device_memory_callback(
    VmaAllocator const VMA_NOT_NULL allocator,
    u32 const memory_type,
    VkDeviceMemory const VMA_NOT_NULL_NON_DISPATCHABLE memory,
    VkDeviceSize const size,
    void *const VMA_NULLABLE vma_user_data
)
{
	auto const [callback, user_data] = *static_cast<
	    pair<fn<void(DebugCallbackSource, LogLvl, str const &, std::any)>, std::any> *>(
	    vma_user_data
	);

	callback(
	    DebugCallbackSource::eVma,
	    LogLvl::eTrace,
	    fmt::format("Allocate: {}", size),
	    user_data
	);
}

// @todo Send a more thorough string
static void vma_free_device_memory_callback(
    VmaAllocator const VMA_NOT_NULL allocator,
    u32 const memory_type,
    VkDeviceMemory const VMA_NOT_NULL_NON_DISPATCHABLE memory,
    VkDeviceSize const size,
    void *const VMA_NULLABLE vma_user_data
)
{
	auto const [callback, user_data] = *static_cast<
	    pair<fn<void(DebugCallbackSource, LogLvl, str const &, std::any)>, std::any> *>(
	    vma_user_data
	);

	callback(DebugCallbackSource::eVma, LogLvl::eTrace, fmt::format("Free: {}", size), user_data);
}

static auto parse_message_type(VkDebugUtilsMessageTypeFlagsEXT const message_types) -> c_str
{
	if (message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		return "GENERAL";

	if (message_types == (VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT))
		return "VALIDATION | PERFORMANCE";

	if (message_types == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
		return "VALIDATION";

	return "PERFORMANCE";
}

static auto parse_message_severity(VkDebugUtilsMessageSeverityFlagBitsEXT const message_severity)
    -> LogLvl
{
	switch (message_severity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return LogLvl::eTrace;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return LogLvl::eInfo;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return LogLvl::eWarn;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return LogLvl::eError;
	default: assert_fail("Invalid message severity: {}", message_severity);
	}

	return {};
}

static VkBool32 vulkan_debug_message_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT const message_severity,
    VkDebugUtilsMessageTypeFlagsEXT const message_types,
    VkDebugUtilsMessengerCallbackDataEXT const *const callback_data,
    void *const vulkan_user_data
)
{
	auto const [callback, user_data] = *static_cast<
	    pair<fn<void(DebugCallbackSource, LogLvl, str const &, std::any)>, std::any> *>(
	    vulkan_user_data
	);

	auto const type = parse_message_type(message_types);
	auto const level = parse_message_severity(message_severity);

	callback(DebugCallbackSource::eValidationLayers, level, callback_data->pMessage, user_data);

	return static_cast<VkBool32>(VK_FALSE);
}

VkContext::VkContext(
    vec<c_str> layers,
    vec<c_str> instance_extensions,
    vec<c_str> device_extensions,

    vk::PhysicalDeviceFeatures physical_device_features,

    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
    fn<vk::Extent2D()> get_framebuffer_extent_func,
    fn<Gpu(vec<Gpu>)> gpu_picker_func,

    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,

    pair<fn<void(DebugCallbackSource, LogLvl, const str &, std::any)>, std::any>
        debug_callback_and_data
)
    : layers(layers)
    , instance_extensions(instance_extensions)
    , device_extensions(device_extensions)
    , get_framebuffer_extent(get_framebuffer_extent_func)
    , debug_callback_and_data(debug_callback_and_data)
{
	load_vulkan_instance_functions();
	check_layer_support();

	create_vulkan_instance();
	create_debug_messenger(debug_messenger_severities, debug_messenger_types);
	surface.init_with_instance(instance, create_window_surface_func, get_framebuffer_extent);

	pick_gpu(gpu_picker_func);
	queues.init_with_gpu(gpu);
	create_device(physical_device_features);

	load_vulkan_device_functions();

	create_memory_allocator();
	descriptor_allocator.init(device);

	queues.init_with_device(device);
	surface.init_with_gpu(gpu);
	create_command_pools();

	swapchain.init(device, surface, queues);
}

VkContext::~VkContext()
{
	if (static_cast<VkDevice>(device) == VK_NULL_HANDLE)
		return;

	swapchain.destroy();

	device.waitIdle();

	device.destroyFence(immediate_fence);
	device.destroyCommandPool(immediate_cmd_pool);

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; ++i)
		device.destroyCommandPool(dynamic_cmd_pools[i]);

	allocator.destroy();
	descriptor_allocator.destroy();
	device.destroy();

	surface.destroy();
	instance.destroyDebugUtilsMessengerEXT(debug_util_messenger);
	instance.destroy();
}

void VkContext::load_vulkan_instance_functions()
{
	VULKAN_HPP_DEFAULT_DISPATCHER.init(
	    vk::DynamicLoader().getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")
	);
}

void VkContext::load_vulkan_device_functions()
{
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);
}

void VkContext::check_layer_support()
{
	for (auto const layer : layers)
		if (!has_layer(layer))
			assert_fail("Required layer: {} is not supported", layer);
}

auto VkContext::has_layer(c_str const layer) const -> bool
{
	for (auto const &layer_properties : vk::enumerateInstanceLayerProperties())
		if (strcmp(layer, layer_properties.layerName))
			return true;

	return false;
}

void VkContext::create_vulkan_instance()
{
	auto const application_info = vk::ApplicationInfo {
		"BindlessVK",
		VK_MAKE_VERSION(1, 0, 0),
		"BindlessVK", //
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_3,
	};

	auto const instance_info = vk::InstanceCreateInfo {
		{},
		&application_info,
		static_cast<u32>(layers.size()),
		layers.data(),
		static_cast<u32>(instance_extensions.size()),
		instance_extensions.data(),
	};

	assert_false(vk::createInstance(&instance_info, nullptr, &instance));
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
}

void VkContext::create_debug_messenger(
    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types
)
{
	auto const debug_messenger_create_info = vk::DebugUtilsMessengerCreateInfoEXT {
		{},
		debug_messenger_severities,
		debug_messenger_types,
		&vulkan_debug_message_callback,
		&debug_callback_and_data,
	};

	debug_util_messenger = instance.createDebugUtilsMessengerEXT(debug_messenger_create_info);
}

void VkContext::pick_gpu(fn<Gpu(vec<Gpu>)> const gpu_picker_func)
{
	fetch_adequate_gpus();
	gpu = gpu_picker_func(adequate_gpus);

	// Cache the selected physical device's properties
	auto const properties = gpu.get_properties();
	auto const limits = properties.limits;
	// bool hasStencilComponent = false;
	auto format_properties = vk::FormatProperties {};
	for (vk::Format format : {
	         vk::Format::eD32Sfloat,
	         vk::Format::eD32SfloatS8Uint,
	         vk::Format::eD24UnormS8Uint,
	     })
	{
		format_properties = gpu.get_format_properties(format);
		if ((format_properties.optimalTilingFeatures &
		     vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
		    vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			depth_format = format;
		}
	}
}

void VkContext::fetch_adequate_gpus()
{
	for (auto const physical_device : instance.enumeratePhysicalDevices())
		if (auto const gpu = Gpu(physical_device, surface, device_extensions); gpu.is_adequate())
			adequate_gpus.push_back(gpu);

	assert_false(adequate_gpus.empty(), "No adaquate gpu found");
}

void VkContext::create_device(vk::PhysicalDeviceFeatures physical_device_features)
{
	auto const dynamic_rendering_features = vk::PhysicalDeviceDynamicRenderingFeatures { true };
	auto const queues_info = queues.get_create_infos();

	device = gpu.create_device(vk::DeviceCreateInfo {
	    {},
	    queues_info,
	    {},
	    device_extensions,
	    &physical_device_features,
	    &dynamic_rendering_features,
	});
}

void VkContext::create_memory_allocator()
{
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
		&vma_allocate_device_memory_callback,
		&vma_free_device_memory_callback,
		&debug_callback_and_data,
	};

	auto const allocator_info = vma::AllocatorCreateInfo(
	    {},
	    gpu,
	    device,
	    {},
	    {},
	    &device_memory_callbacks,
	    {},
	    &vulkan_funcs,
	    instance,
	    {}
	);

	allocator = vma::createAllocator(allocator_info);
}

void VkContext::create_command_pools()
{
	auto const command_pool_info = vk::CommandPoolCreateInfo {
		{},
		queues.get_graphics_index(),
	};

	for (u32 i = 0; i < num_threads; i++)
	{
		for (u32 j = 0; j < BVK_MAX_FRAMES_IN_FLIGHT; j++)
		{
			dynamic_cmd_pools.push_back(device.createCommandPool(command_pool_info));
		}
	}

	immediate_fence = device.createFence({});
	immediate_cmd_pool = device.createCommandPool(command_pool_info);
}

} // namespace BINDLESSVK_NAMESPACE
