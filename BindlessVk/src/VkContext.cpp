#define VMA_IMPLEMENTATION

#include "BindlessVk/VkContext.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace BINDLESSVK_NAMESPACE {

pair<fn<void(DebugCallbackSource, LogLvl, str const &, std::any)>, std::any>
    VkContext::debug_callback_data = {};

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

    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,

    fn<void(DebugCallbackSource, LogLvl, const str &, std::any)> bindlessvk_debug_callback,
    std::any bindlessvk_debug_callback_user_data
)
    : layers(layers)
    , instance_extensions(instance_extensions)
    , device_extensions(device_extensions)

    , physical_device_features(physical_device_features)

    , queues({
          VK_NULL_HANDLE,
          VK_NULL_HANDLE,
          VK_NULL_HANDLE,
          std::numeric_limits<u32>().max(),
          std::numeric_limits<u32>().max(),
          std::numeric_limits<u32>().max(),
      })

    , get_framebuffer_extent(get_framebuffer_extent_func)
    , num_threads(1)
    , pfn_vk_get_instance_proc_addr(
          dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr")
      )

{
	debug_callback_data = { bindlessvk_debug_callback, bindlessvk_debug_callback_user_data };
	VULKAN_HPP_DEFAULT_DISPATCHER.init(pfn_vk_get_instance_proc_addr);

	check_layer_support();

	create_vulkan_instance(
	    create_window_surface_func,
	    debug_messenger_severities,
	    debug_messenger_types
	);

	pick_physical_device();

	create_logical_device();
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

	fetch_queues();

	create_allocator();

	update_surface_info();

	create_command_pools();
}

VkContext::VkContext(VkContext &&other)
{
	*this = std::move(other);
}

VkContext &VkContext::operator=(VkContext &&other)
{
	if (this == &other)
		return *this;

	this->pfn_vk_get_instance_proc_addr = other.pfn_vk_get_instance_proc_addr;
	this->get_framebuffer_extent = other.get_framebuffer_extent;
	this->layers = other.layers;
	this->instance_extensions = other.instance_extensions;
	this->device_extensions = other.device_extensions;
	this->instance = other.instance;
	this->device = other.device;
	this->selected_gpu = other.selected_gpu;
	this->available_gpus = other.available_gpus;
	this->allocator = other.allocator;
	this->queues = other.queues;
	this->surface = other.surface;
	this->depth_format = other.depth_format;
	this->max_color_and_depth_samples = other.max_color_and_depth_samples;
	this->max_color_samples = other.max_color_samples;
	this->max_depth_samples = other.max_depth_samples;
	this->num_threads = other.num_threads;
	this->dynamic_cmd_pools = other.dynamic_cmd_pools;
	this->immediate_cmd_pool = other.immediate_cmd_pool;
	this->immediate_cmd = other.immediate_cmd;
	this->immediate_fence = other.immediate_fence;
	this->debug_util_messenger = other.debug_util_messenger;
	this->debug_callback_data = other.debug_callback_data;

	other.device = VK_NULL_HANDLE;
	other.allocator = VK_NULL_HANDLE;

	return *this;
}

VkContext::~VkContext()
{
	if (static_cast<VkDevice>(device) == VK_NULL_HANDLE)
	{
		return;
	}

	device.waitIdle();

	device.destroyFence(immediate_fence);
	device.destroyCommandPool(immediate_cmd_pool);

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		device.destroyCommandPool(dynamic_cmd_pools[i]);
	}

	allocator.destroy();
	device.destroy(nullptr);

	instance.destroySurfaceKHR(surface, nullptr);
	instance.destroyDebugUtilsMessengerEXT(debug_util_messenger);
	instance.destroy();
}

void VkContext::check_layer_support()
{
	auto const available_layers = vk::enumerateInstanceLayerProperties();
	assert_false(available_layers.empty(), "No instance layer found");

	// Check if we support all the required layers
	for (c_str required_layer_name : layers)
	{
		bool layer_found = false;

		for (auto const &layer_properties : available_layers)
		{
			if (strcmp(required_layer_name, layer_properties.layerName))
			{
				layer_found = true;
				break;
			}
		}

		assert_true(layer_found, "Required layer not found");
	}
}

void VkContext::create_vulkan_instance(
    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types
)
{
	auto const application_info = vk::ApplicationInfo {
		"BindlessVK",
		VK_MAKE_VERSION(1, 0, 0),
		"BindlessVK", //
		VK_MAKE_VERSION(1, 0, 0),
		VK_API_VERSION_1_3,
	};

	auto const debug_messenger_create_info = vk::DebugUtilsMessengerCreateInfoEXT {
		{},
		debug_messenger_severities,
		debug_messenger_types,
		&vulkan_debug_message_callback,
		&debug_callback_data,
	};

	// Create the vulkan instance
	auto const instance_create_info = vk::InstanceCreateInfo {
		{},
		&application_info,
		static_cast<u32>(layers.size()),
		layers.data(),
		static_cast<u32>(instance_extensions.size()),
		instance_extensions.data(),
	};

	assert_false(vk::createInstance(&instance_create_info, nullptr, &instance));
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
	surface.surface_object = create_window_surface_func(instance);

	debug_util_messenger = instance.createDebugUtilsMessengerEXT(debug_messenger_create_info);
}

void VkContext::pick_physical_device()
{
	// Fetch physical devices with vulkan support
	auto const all_gpus = instance.enumeratePhysicalDevices();
	assert_false(all_gpus.empty(), "No gpu with vulkan support found");

	// Select the most suitable physical device
	u32 high_score = 0u;
	for (auto const &gpu : all_gpus)
	{
		u32 score = 0u;

		// Fetch properties & features
		auto const properties = gpu.getProperties();
		auto const features = gpu.getFeatures();

		// Check if device supports required features
		if (!features.geometryShader || !features.samplerAnisotropy)
			continue;

		/** Check if the device supports the required queues **/
		{
			// Fetch queue families
			auto const queue_families_properties = gpu.getQueueFamilyProperties();
			if (queue_families_properties.empty())
				continue;

			u32 index = 0u;
			for (auto const &queue_family_property : queue_families_properties)
			{
				// Check if current queue index supports any or all desired queues
				if (queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics)
				{
					queues.graphics_index = index;
				}

				if (gpu.getSurfaceSupportKHR(index, surface))
				{
					queues.present_index = index;
				}

				if (queue_family_property.queueFlags & vk::QueueFlagBits::eCompute)
				{
					queues.compute_index = index;
				}

				index++;

				// Device does supports all the required queues!
				if (queues.graphics_index != UINT32_MAX && queues.present_index != UINT32_MAX &&
				    queues.compute_index != UINT32_MAX)
				{
					break;
				}
			}

			// Device doesn't support all the required queues!
			if (queues.graphics_index == UINT32_MAX || queues.present_index == UINT32_MAX ||
			    queues.compute_index == UINT32_MAX)
			{
				queues.graphics_index = UINT32_MAX;
				queues.present_index = UINT32_MAX;
				queues.compute_index = UINT32_MAX;
				continue;
			}
		}

		/** Check if device supports required extensions **/
		{
			// Fetch extensions

			auto const device_extensions_properties = gpu.enumerateDeviceExtensionProperties();
			if (device_extensions_properties.empty())
				continue;

			bool failed = false;
			for (auto const &required_extension_name : device_extensions)
			{
				bool found = false;
				for (auto const &extension : device_extensions_properties)
				{
					if (!strcmp(required_extension_name, extension.extensionName))
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					failed = true;
					break;
				}
			}

			if (failed)
				continue;
		}

		/** Check if surface is adequate **/
		{
			surface.capabilities = gpu.getSurfaceCapabilitiesKHR(surface);

			if (gpu.getSurfaceFormatsKHR(surface).empty())
				continue;

			if (gpu.getSurfacePresentModesKHR(surface).empty())
				continue;
		}

		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 69420; // nice

		score += properties.limits.maxImageDimension2D;

		selected_gpu = score > high_score ? gpu : selected_gpu;
	}
	assert_true(selected_gpu, "No suitable physical device found");

	// Cache the selected physical device's properties
	auto const properties = selected_gpu.getProperties();
	auto const limits = properties.limits;

	// Determine max sample counts
	auto const color = limits.framebufferColorSampleCounts;
	auto const depth = limits.framebufferDepthSampleCounts;

	auto const depth_color = color & depth;

	max_color_samples = color & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	                    color & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	                    color & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	                    color & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	                    color & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	                    color & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                           vk::SampleCountFlagBits::e1;

	max_depth_samples = depth & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	                    depth & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	                    depth & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	                    depth & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	                    depth & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	                    depth & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                           vk::SampleCountFlagBits::e1;

	max_color_and_depth_samples =
	    depth_color & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	    depth_color & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	    depth_color & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	    depth_color & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	    depth_color & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	    depth_color & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                 vk::SampleCountFlagBits::e1;

	// Select depth format
	// bool hasStencilComponent = false;
	auto format_properties = vk::FormatProperties {};
	for (vk::Format format : {
	         vk::Format::eD32Sfloat,
	         vk::Format::eD32SfloatS8Uint,
	         vk::Format::eD24UnormS8Uint,
	     })
	{
		format_properties = selected_gpu.getFormatProperties(format);
		if ((format_properties.optimalTilingFeatures &
		     vk::FormatFeatureFlagBits::eDepthStencilAttachment) ==
		    vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			depth_format = format;
		}
	}
}

void VkContext::create_logical_device()
{
	auto const dynamic_rendering_features = vk::PhysicalDeviceDynamicRenderingFeatures { true };
	auto const queues_info = create_queues_create_info();

	device = selected_gpu.createDevice(vk::DeviceCreateInfo {
	    {},
	    queues_info,
	    {},
	    device_extensions,
	    &physical_device_features, //
	    &dynamic_rendering_features,
	});
}

void VkContext::fetch_queues()
{
	queues.graphics = device.getQueue(queues.graphics_index, 0u);
	queues.present = device.getQueue(queues.present_index, 0u);

	assert_true(queues.graphics, "Failed to fetch graphics queue");
	assert_true(queues.present, "Failed to fetch present queue");
}

void VkContext::create_allocator()
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
		&debug_callback_data,
	};

	auto const allocator_info = vma::AllocatorCreateInfo(
	    {},
	    selected_gpu,
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
		queues.graphics_index,
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

void VkContext::update_surface_info()
{
	surface.capabilities = selected_gpu.getSurfaceCapabilitiesKHR(surface);
	auto const supported_formats = selected_gpu.getSurfaceFormatsKHR(surface);
	auto const supported_present_modes = selected_gpu.getSurfacePresentModesKHR(surface);

	// Select surface format
	auto selected_surface_format = supported_formats[0]; // default
	for (auto const &format : supported_formats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb &&
		    format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			selected_surface_format = format;
			break;
		}
	}
	surface.color_format = selected_surface_format.format;
	surface.color_space = selected_surface_format.colorSpace;

	// Select present mode
	surface.present_mode = supported_present_modes[0]; // default
	for (auto const &present_mode : supported_present_modes)
	{
		if (present_mode == vk::PresentModeKHR::eFifo)
		{
			surface.present_mode = present_mode;
		}
	}

	surface.framebuffer_extent = std::clamp(
	    get_framebuffer_extent(),
	    surface.capabilities.minImageExtent,
	    surface.capabilities.maxImageExtent
	);
}

auto VkContext::create_queues_create_info() const -> vec<vk::DeviceQueueCreateInfo>
{
	static constexpr arr<f32, 1> queue_priority = { 1.0 };

	auto create_info = vec<vk::DeviceQueueCreateInfo> {
		{
		    {},
		    queues.graphics_index,
		    queue_priority,
		},
	};

	if (queues.graphics_index != queues.present_index)
	{
		create_info.push_back({
		    {},
		    queues.present_index,
		    queue_priority,
		});
	}

	if (queues.graphics_index != queues.compute_index &&
	    queues.present_index != queues.compute_index)
	{
		create_info.push_back({
		    {},
		    queues.present_index,
		    queue_priority,
		});
	}

	return create_info;
}

} // namespace BINDLESSVK_NAMESPACE
