#pragma once

#include "BindlessVk/Allocators/DescriptorAllocator.hpp"
#include "BindlessVk/Allocators/LayoutAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Gpu.hpp"
#include "BindlessVk/Context/Instance.hpp"
#include "BindlessVk/Context/Queues.hpp"
#include "BindlessVk/Context/Surface.hpp"
#include "BindlessVk/Context/Swapchain.hpp"

#include <any>

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

namespace BINDLESSVK_NAMESPACE {

enum class DebugCallbackSource
{
	eBindlessVk,
	eValidationLayers,
	eVma,

	nCount
};

class VkContext
{
public:
	VkContext() = default;

	VkContext(
	    Instance *instance,
	    vec<c_str> device_extensions,
	    vk::PhysicalDeviceFeatures physical_device_features,

	    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
	    fn<vk::Extent2D()> get_framebuffer_extent_func,
	    fn<Gpu(vec<Gpu>)> gpu_picker_func,

	    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
	    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,
	    pair<fn<void(DebugCallbackSource, LogLvl, const str &, std::any)>, std::any>
	        debug_callback_and_data
	);

	~VkContext();

	void immediate_submit(fn<void(vk::CommandBuffer)> &&func) const
	{
		auto const alloc_info = vk::CommandBufferAllocateInfo {
			immediate_cmd_pool,
			vk::CommandBufferLevel::ePrimary,
			1u,
		};

		vk::CommandBuffer cmd = device.allocateCommandBuffers(alloc_info)[0];

		vk::CommandBufferBeginInfo beginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };

		cmd.begin(beginInfo);
		func(cmd);
		cmd.end();

		vk::SubmitInfo submit_info { 0u, {}, {}, 1u, &cmd, 0u, {}, {} };
		queues.get_graphics().submit(submit_info, immediate_fence);

		assert_false(device.waitForFences(immediate_fence, true, UINT_MAX));
		device.resetFences(immediate_fence);
		device.resetCommandPool(immediate_cmd_pool);
	}

	template<typename... Args>
	void log(LogLvl lvl, fmt::format_string<Args...> fmt, Args &&...args) const
	{
		auto const [callback, data] = debug_callback_and_data;

		callback(
		    DebugCallbackSource::eBindlessVk,
		    lvl,
		    fmt::format(fmt, std::forward<Args>(args)...),
		    data
		);
	}

	template<typename T>
	void set_object_name(T object, str_view name) const
	{
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.objectType,
		    (u64)((typename T::NativeType)(object)),
		    name.data(),
		});
	}

	void set_object_name(AllocatedImage object, str_view name) const
	{
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.image.objectType,
		    (u64)((VkImage)(object.image)),
		    name.data(),
		});
	}

	void set_object_name(AllocatedDescriptorSet object, str_view name) const
	{
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.descriptor_set.objectType,
		    (u64)((VkDescriptorSet)(object.descriptor_set)),
		    name.data(),
		});
	}

	void set_object_name(DescriptorSetLayoutWithHash object, str_view name) const
	{
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.descriptor_set_layout.objectType,
		    (u64)((VkDescriptorSetLayout)(object.descriptor_set_layout)),
		    name.data(),
		});
	}

	auto get_cmd_pool(u32 const frame_index, u32 const thread_index = 0) const
	{
		return dynamic_cmd_pools[(thread_index * num_threads) + frame_index];
	}

	auto get_device_extensions() const
	{
		return device_extensions;
	}

	auto get_instance() const
	{
		return instance;
	}

	auto get_device() const
	{
		return device;
	}

	auto get_gpu() const
	{
		return gpu;
	}

	auto &get_adequate_gpus() const
	{
		return adequate_gpus;
	}

	auto get_layout_allocator()
	{
		return &layout_allocator;
	}

	auto get_allocator() const
	{
		return allocator;
	}

	auto get_swapchain()
	{
		return &swapchain;
	}

	auto get_descriptor_allocator()
	{
		return &descriptor_allocator;
	}

	auto &get_queues() const
	{
		return queues;
	}

	auto &get_surface() const
	{
		return surface;
	}

	auto get_depth_format() const
	{
		return depth_format;
	}

	auto get_depth_format_has_stencil() const
	{
		return depth_format == vk::Format::eD32SfloatS8Uint ||
		       depth_format == vk::Format::eD24UnormS8Uint;
	}

	auto get_num_threads() const
	{
		return num_threads;
	}

	auto get_device_proc_addr(str_view proc_name) const
	{
		return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr(device, proc_name.data());
	}

private:
	void load_vulkan_device_functions();

	void create_debug_messenger(
	    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
	    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types
	);

	void create_window_surface(fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func);

	void pick_gpu(fn<Gpu(vec<Gpu>)> gpu_picker_func);

	void fetch_adequate_gpus();

	void create_device(vk::PhysicalDeviceFeatures physical_device_features);

	void create_memory_allocator();

	void create_command_pools();

	auto calculate_swapchain_image_count() const -> u32;

	auto has_layer(c_str layer) const -> bool;

private:
	PFN_vkGetInstanceProcAddr pfn_vk_get_instance_proc_addr = {};

	fn<vk::Extent2D()> get_framebuffer_extent = {};

	pair<fn<void(DebugCallbackSource, LogLvl, str const &, std::any)>, std::any>
	    debug_callback_and_data = {};

private:
	Instance *instance = {};

	vk::Device device = {};
	vec<c_str> device_extensions = {};

	vec<Gpu> adequate_gpus = {};

	vma::Allocator allocator = {};
	DescriptorAllocator descriptor_allocator = {};
	LayoutAllocator layout_allocator = {};

	Queues queues = {};
	Surface surface = {};
	Swapchain swapchain = {};
	Gpu gpu = {};

	vk::Format depth_format = {};

	u32 num_threads = 1u;

	vec<vk::CommandPool> dynamic_cmd_pools = {};
	vk::CommandPool immediate_cmd_pool = {};

	vk::CommandBuffer immediate_cmd = {};
	vk::Fence immediate_fence = {};

	vk::DebugUtilsMessengerEXT debug_util_messenger = {};
};


} // namespace BINDLESSVK_NAMESPACE
