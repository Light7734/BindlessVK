#define VMA_IMPLEMENTATION

#include "BindlessVk/Device.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace BINDLESSVK_NAMESPACE {

void DeviceSystem::init(
    vec<c_str> layers,
    vec<c_str> instance_extensions,
    vec<c_str> device_extensions,
    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
    fn<vk::Extent2D()> get_framebuffer_extent_func,
    bool has_debugging,
    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,
    PFN_vkDebugUtilsMessengerCallbackEXT debug_messenger_callback_func,
    void* debug_messenger_userptr,
    PFN_vmaAllocateDeviceMemoryFunction vma_free_device_memory_callback,
    PFN_vmaFreeDeviceMemoryFunction vma_allocate_device_memory_callback
)
{
	device.layers = layers;
	device.instance_extensions = instance_extensions;
	device.device_extensions = device_extensions;

	device.graphics_queue_index = UINT32_MAX;
	device.present_queue_index = UINT32_MAX;
	device.compute_queue_index = UINT32_MAX;

	device.get_framebuffer_extent_func = get_framebuffer_extent_func;

	// @todo: multi-threading support
	device.num_threads = 1u;

	// Initialize default dispatcher
	//
	device.get_vk_instance_proc_addr_func =
	    dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

	VULKAN_HPP_DEFAULT_DISPATCHER.init(device.get_vk_instance_proc_addr_func);

	check_layer_support();

	create_vulkan_instance(
	    create_window_surface_func,
	    has_debugging,
	    debug_messenger_severities,
	    debug_messenger_types,
	    debug_messenger_callback_func,
	    debug_messenger_userptr
	);

	pick_physical_device();
	create_logical_device();
	create_allocator(vma_free_device_memory_callback, vma_allocate_device_memory_callback);
	update_surface_info();

	create_command_pools();
}

void DeviceSystem::reset()
{
	device.logical.waitIdle();

	device.logical.destroyFence(device.immediate_fence);
	device.logical.destroyCommandPool(device.immediate_cmd_pool);

	for (u32 i = 0; i < BVK_MAX_FRAMES_IN_FLIGHT; i++)
	{
		device.logical.destroyCommandPool(device.dynamic_cmd_pools[i]);
	}

	device.allocator.destroy();
	device.logical.destroy(nullptr);

	device.instance.destroySurfaceKHR(device.surface, nullptr);
	device.instance.destroyDebugUtilsMessengerEXT(debug_util_messenger);
	device.instance.destroy();
}

void DeviceSystem::check_layer_support()
{
	vec<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();
	assert_false(available_layers.empty(), "No instance layer found");

	// Check if we support all the required layers
	for (c_str required_layer_name : device.layers)
	{
		bool layer_found = false;

		for (const auto& layer_properties : available_layers)
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

void DeviceSystem::create_vulkan_instance(
    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
    bool has_debugging,
    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,
    PFN_vkDebugUtilsMessengerCallbackEXT debug_messenger_callback_func,
    void* debug_messenger_userptr
)
{
	vk::ApplicationInfo application_info {
		"BindlessVK",             // pApplicationName
		VK_MAKE_VERSION(1, 0, 0), // applicationVersion
		"BindlessVK",             // pEngineName
		VK_MAKE_VERSION(1, 0, 0), // engineVersion
		VK_API_VERSION_1_3,       // apiVersion
	};

	vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info {
		{},                         // flags
		debug_messenger_severities, // messageSeverity
		debug_messenger_types,      // messageType
		debug_messenger_callback_func,
		debug_messenger_userptr,
	};

	// If debugging is not enabled, remove validation layer from instance layers
	if (!has_debugging)
	{
		auto validation_layer_it =
		    std::find(device.layers.begin(), device.layers.end(), "VK_LAYER_KHRONOS_validation");
		if (validation_layer_it != device.layers.end())
		{
			device.layers.erase(validation_layer_it);
		}
	}

	// Create the vulkan instance

	vk::InstanceCreateInfo instance_create_info {
		{},                                                  // flags
		&application_info,                                   // pApplicationInfo
		static_cast<u32>(device.layers.size()),              // enabledLayerCount
		device.layers.data(),                                // ppEnabledLayerNames
		static_cast<u32>(device.instance_extensions.size()), // enabledExtensionCount
		device.instance_extensions.data(),                   // ppEnabledExtensionNames
	};

	assert_false(vk::createInstance(&instance_create_info, nullptr, &device.instance));
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device.instance);
	device.surface = create_window_surface_func(device.instance);

	debug_util_messenger =
	    device.instance.createDebugUtilsMessengerEXT(debug_messenger_create_info);
}

void DeviceSystem::pick_physical_device()
{
	// Fetch physical devices with vulkan support
	vec<vk::PhysicalDevice> physical_devices = device.instance.enumeratePhysicalDevices();

	assert_false(physical_devices.empty(), "No physical device with vulkan support found");

	// Select the most suitable physical device
	u32 high_score = 0u;
	for (const auto& physical_device : physical_devices)
	{
		u32 score = 0u;

		// Fetch properties & features
		vk::PhysicalDeviceProperties properties = physical_device.getProperties();
		vk::PhysicalDeviceFeatures features = physical_device.getFeatures();

		// Check if device supports required features
		if (!features.geometryShader || !features.samplerAnisotropy)
			continue;

		/** Check if the device supports the required queues **/
		{
			// Fetch queue families
			vec<vk::QueueFamilyProperties> queue_families_properties =
			    physical_device.getQueueFamilyProperties();
			if (queue_families_properties.empty())
				continue;

			u32 index = 0u;
			for (const auto& queue_family_property : queue_families_properties)
			{
				// Check if current queue index supports any or all desired queues
				if (queue_family_property.queueFlags & vk::QueueFlagBits::eGraphics)
				{
					device.graphics_queue_index = index;
				}

				if (physical_device.getSurfaceSupportKHR(index, device.surface))
				{
					device.present_queue_index = index;
				}

				if (queue_family_property.queueFlags & vk::QueueFlagBits::eCompute)
				{
					device.compute_queue_index = index;
				}

				index++;

				// Device does supports all the required queues!
				if (device.graphics_queue_index != UINT32_MAX
				    && device.present_queue_index != UINT32_MAX
				    && device.present_queue_index != UINT32_MAX)
				{
					break;
				}
			}

			// Device doesn't support all the required queues!
			if (device.graphics_queue_index == UINT32_MAX
			    || device.present_queue_index == UINT32_MAX
			    || device.compute_queue_index == UINT32_MAX)
			{
				device.graphics_queue_index = UINT32_MAX;
				device.present_queue_index = UINT32_MAX;
				device.compute_queue_index = UINT32_MAX;
				continue;
			}
		}

		/** Check if device supports required extensions **/
		{
			// Fetch extensions

			vec<vk::ExtensionProperties> device_extensions_properties =
			    physical_device.enumerateDeviceExtensionProperties();
			if (device_extensions_properties.empty())
				continue;

			bool failed = false;
			for (const auto& required_extension_name : device.device_extensions)
			{
				bool found = false;
				for (const auto& extension : device_extensions_properties)
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
			device.surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(device.surface);

			if (physical_device.getSurfaceFormatsKHR(device.surface).empty())
				continue;

			if (physical_device.getSurfacePresentModesKHR(device.surface).empty())
				continue;
		}

		if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 69420; // nice

		score += properties.limits.maxImageDimension2D;

		device.physical = score > high_score ? physical_device : device.physical;
	}
	assert_true(device.physical, "No suitable physical device found");

	// Cache the selected physical device's properties
	device.properties = device.physical.getProperties();
	auto limits = device.properties.limits;

	// Determine max sample counts
	vk::Flags<vk::SampleCountFlagBits> color = limits.framebufferColorSampleCounts;
	vk::Flags<vk::SampleCountFlagBits> depth = limits.framebufferDepthSampleCounts;

	vk::Flags<vk::SampleCountFlagBits> depth_color = color & depth;

	device.max_color_samples = color & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	                           color & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	                           color & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	                           color & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	                           color & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	                           color & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                                  vk::SampleCountFlagBits::e1;

	device.max_depth_samples = depth & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	                           depth & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	                           depth & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	                           depth & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	                           depth & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	                           depth & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                                  vk::SampleCountFlagBits::e1;

	device.max_samples = depth_color & vk::SampleCountFlagBits::e64 ? vk::SampleCountFlagBits::e64 :
	                     depth_color & vk::SampleCountFlagBits::e32 ? vk::SampleCountFlagBits::e32 :
	                     depth_color & vk::SampleCountFlagBits::e16 ? vk::SampleCountFlagBits::e16 :
	                     depth_color & vk::SampleCountFlagBits::e8  ? vk::SampleCountFlagBits::e8 :
	                     depth_color & vk::SampleCountFlagBits::e4  ? vk::SampleCountFlagBits::e4 :
	                     depth_color & vk::SampleCountFlagBits::e2  ? vk::SampleCountFlagBits::e2 :
	                                                                  vk::SampleCountFlagBits::e1;

	// Select depth format
	// bool hasStencilComponent = false;
	vk::FormatProperties format_properties;
	for (vk::Format format :
	     { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint })
	{
		format_properties = device.physical.getFormatProperties(format);
		if ((format_properties.optimalTilingFeatures
		     & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		    == vk::FormatFeatureFlagBits::eDepthStencilAttachment)
		{
			device.depth_format = format;
			device.has_stencil =
			    format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
		}
	}
}

void DeviceSystem::create_logical_device()
{
	vec<vk::DeviceQueueCreateInfo> queues_info;

	float queue_priority = 1.0f; // Always 1.0
	queues_info.push_back({
	    {},                          // flags
	    device.graphics_queue_index, // queueFamilyIndex
	    1u,                          // queueCount
	    &queue_priority,             // pQueuePriorities
	});

	if (device.present_queue_index != device.graphics_queue_index)
	{
		queues_info.push_back({
		    {},                         // flags
		    device.present_queue_index, // queueFamilyIndex
		    1u,                         // queueCount
		    &queue_priority,            // pQueuePriorities
		});
	}

	vk::PhysicalDeviceFeatures physical_device_features {
		VK_FALSE, // robustBufferAccess
		VK_FALSE, // fullDrawIndexUint32
		VK_FALSE, // imageCubeArray
		VK_FALSE, // independentBlend
		VK_FALSE, // geometryShader
		VK_FALSE, // tessellationShader
		VK_FALSE, // sampleRateShading
		VK_FALSE, // dualSrcBlend
		VK_FALSE, // logicOp
		VK_TRUE,  // multiDrawIndirect
		VK_TRUE,  // drawIndirectFirstInstance
		VK_FALSE, // depthClamp
		VK_FALSE, // depthBiasClamp
		VK_FALSE, // fillModeNonSolid
		VK_FALSE, // depthBounds
		VK_FALSE, // wideLines
		VK_FALSE, // largePoints
		VK_FALSE, // alphaToOne
		VK_FALSE, // multiViewport
		VK_TRUE,  // samplerAnisotropy
		VK_FALSE, // textureCompressionETC2
		VK_FALSE, // textureCompressionASTC
		VK_FALSE, // textureCompressionBC
		VK_FALSE, // occlusionQueryPrecise
		VK_FALSE, // pipelineStatisticsQuery
		VK_FALSE, // vertexPipelineStoresAndAtomics
		VK_FALSE, // fragmentStoresAndAtomics
		VK_FALSE, // shaderTessellationAndGeometryPointSize
		VK_FALSE, // shaderImageGatherExtended
		VK_FALSE, // shaderStorageImageExtendedFormats
		VK_FALSE, // shaderStorageImageMultisample
		VK_FALSE, // shaderStorageImageReadWithoutFormat
		VK_FALSE, // shaderStorageImageWriteWithoutFormat
		VK_FALSE, // shaderUniformBufferArrayDynamicIndexing
		VK_FALSE, // shaderSampledImageArrayDynamicIndexing
		VK_FALSE, // shaderStorageBufferArrayDynamicIndexing
		VK_FALSE, // shaderStorageImageArrayDynamicIndexing
		VK_FALSE, // shaderClipDistance
		VK_FALSE, // shaderCullDistance
		VK_FALSE, // shaderFloat64
		VK_FALSE, // shaderInt64
		VK_FALSE, // shaderInt16
		VK_FALSE, // shaderResourceResidency
		VK_FALSE, // shaderResourceMinLod
		VK_FALSE, // sparseBinding
		VK_FALSE, // sparseResidencyBuffer
		VK_FALSE, // sparseResidencyImage2D
		VK_FALSE, // sparseResidencyImage3D
		VK_FALSE, // sparseResidency2Samples
		VK_FALSE, // sparseResidency4Samples
		VK_FALSE, // sparseResidency8Samples
		VK_FALSE, // sparseResidency16Samples
		VK_FALSE, // sparseResidencyAliased
		VK_FALSE, // variableMultisampleRate
		VK_FALSE, // inheritedQueries
	};

	vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features {
		true, // dynamicRendering
	};

	vk::DeviceCreateInfo logical_device_info {
		{},
		static_cast<u32>(queues_info.size()),
		queues_info.data(),
		0u,
		nullptr,
		static_cast<u32>(device.device_extensions.size()),
		device.device_extensions.data(),
		&physical_device_features,
		&dynamic_rendering_features,
	};

	device.logical = device.physical.createDevice(logical_device_info);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device.logical);

	device.graphics_queue = device.logical.getQueue(device.graphics_queue_index, 0u);
	device.present_queue = device.logical.getQueue(device.present_queue_index, 0u);

	assert_true(device.graphics_queue, "Failed to fetch graphics queue");
	assert_true(device.present_queue, "Failed to fetch present queue");
}

void DeviceSystem::create_allocator(
    PFN_vmaAllocateDeviceMemoryFunction vma_free_device_memory_callback,
    PFN_vmaFreeDeviceMemoryFunction vma_allocate_device_memory_callback
)
{
	vma::VulkanFunctions vulkan_funcs(
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

	vma::DeviceMemoryCallbacks mem_callbacks = {
		vma_free_device_memory_callback,
		vma_allocate_device_memory_callback,
		&device,
	};

	vma::AllocatorCreateInfo allocator_info(
	    {},
	    device.physical,
	    device.logical,
	    {},
	    {},
	    &mem_callbacks,
	    {},
	    &vulkan_funcs,
	    device.instance,
	    {}
	);

	device.allocator = vma::createAllocator(allocator_info);
}

void DeviceSystem::create_command_pools()
{
	vk::CommandPoolCreateInfo command_pool_info {
		{},
		device.graphics_queue_index,
	};

	for (u32 i = 0; i < device.num_threads; i++)
	{
		for (u32 j = 0; j < BVK_MAX_FRAMES_IN_FLIGHT; j++)
		{
			device.dynamic_cmd_pools.push_back(device.logical.createCommandPool(command_pool_info));
		}
	}

	device.immediate_fence = device.logical.createFence({});
	device.immediate_cmd_pool = device.logical.createCommandPool(command_pool_info);
}

void DeviceSystem::update_surface_info()
{
	device.surface_capabilities = device.physical.getSurfaceCapabilitiesKHR(device.surface);

	vec<vk::SurfaceFormatKHR> supported_formats =
	    device.physical.getSurfaceFormatsKHR(device.surface);

	vec<vk::PresentModeKHR> supported_present_modes =
	    device.physical.getSurfacePresentModesKHR(device.surface);

	// Select surface format
	device.surface_format = supported_formats[0]; // default
	for (const auto& surface_format : supported_formats)
	{
		if (surface_format.format == vk::Format::eB8G8R8A8Srgb
		    && surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
		{
			device.surface_format = surface_format;
			break;
		}
	}

	// Select present mode
	device.present_mode = supported_present_modes[0]; // default
	for (const auto& present_mode : supported_present_modes)
	{
		if (present_mode == vk::PresentModeKHR::eFifo)
		{
			device.present_mode = present_mode;
		}
	}

	vk::Extent2D framebuffer_extent = device.get_framebuffer_extent_func();

	device.framebuffer_extent = std::clamp(
	    framebuffer_extent,
	    device.surface_capabilities.minImageExtent,
	    device.surface_capabilities.maxImageExtent
	);
}


} // namespace BINDLESSVK_NAMESPACE
