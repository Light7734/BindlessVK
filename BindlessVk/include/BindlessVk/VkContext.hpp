#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Gpu.hpp"

#include <any>

namespace BINDLESSVK_NAMESPACE {

enum class DebugCallbackSource
{
	eBindlessVk,
	eValidationLayers,
	eVma,

	nCount
};

// @todo move cmd pool management and thread stuff to a dedicated class
// @todo compute queue
class VkContext
{
public:
	struct CreateInfo
	{
	};

	// @warn do not reorder fields! they have to be contiguous in memory
	struct Queues
	{
		vk::Queue graphics;
		vk::Queue present;

		u32 graphics_index;
		u32 present_index;

		inline auto have_same_index() const
		{
			return graphics_index == present_index;
		}
	};

	struct Surface
	{
		vk::SurfaceKHR surface_object;
		vk::SurfaceCapabilitiesKHR capabilities;
		vk::Format color_format;
		vk::ColorSpaceKHR color_space;
		vk::PresentModeKHR present_mode;
		vk::Extent2D framebuffer_extent;

		inline operator vk::SurfaceKHR() const
		{
			return surface_object;
		}
	};

	struct Swapchain
	{
		vk::SwapchainKHR swapchain_object;

		vec<vk::Image> images;
		vec<vk::ImageView> image_views;

		u32 image_count;

		inline operator vk::SwapchainKHR() const
		{
			return swapchain_object;
		}
	};

public:
	VkContext() = default;

	VkContext(
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
	);

	~VkContext();

	/** Updates the surface info, (to be called after swapchain-invalidation) */
	void update_surface_info();

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
		queues.graphics.submit(submit_info, immediate_fence);

		assert_false(device.waitForFences(immediate_fence, true, UINT_MAX));
		device.resetFences(immediate_fence);
		device.resetCommandPool(immediate_cmd_pool);
	}

	template<typename... Args>
	inline void log(LogLvl lvl, fmt::format_string<Args...> fmt, Args &&...args) const
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
	inline void set_object_name(T object, c_str name) const
	{
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.objectType,
		    (u64)((typename T::NativeType)(object)),
		    name,
		});
	}

	inline auto get_cmd_pool(u32 const frame_index, u32 const thread_index = 0) const
	{
		return dynamic_cmd_pools[(thread_index * num_threads) + frame_index];
	}

	inline auto get_layers() const
	{
		return layers;
	}

	inline auto get_instance_extensions() const
	{
		return instance_extensions;
	}

	inline auto get_device_extensions() const
	{
		return device_extensions;
	}

	inline auto get_instance() const
	{
		return instance;
	}

	inline auto get_device() const
	{
		return device;
	}

	inline auto get_gpu() const
	{
		return gpu;
	}

	inline auto const &get_adequate_gpus() const
	{
		return adequate_gpus;
	}

	inline auto get_allocator() const
	{
		return allocator;
	}

	inline auto const &get_queues() const
	{
		return queues;
	}

	inline auto const &get_surface() const
	{
		return surface;
	}

	inline auto get_depth_format() const
	{
		return depth_format;
	}

	inline auto get_depth_format_has_stencil() const
	{
		return depth_format == vk::Format::eD32SfloatS8Uint ||
		       depth_format == vk::Format::eD24UnormS8Uint;
	}

	inline auto get_num_threads() const
	{
		return num_threads;
	}

	inline auto get_instance_proc_addr(c_str proc_name) const
	{
		return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(instance, proc_name);
	}

	inline auto get_device_proc_addr(c_str proc_name) const
	{
		return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr(device, proc_name);
	}

private:
	void load_vulkan_instance_functions();
	void load_vulkan_device_functions();

	void check_layer_support();

	void create_vulkan_instance();

	void create_debug_messenger(
	    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
	    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types
	);

	void create_window_surface(fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func);

	void pick_gpu(fn<Gpu(vec<Gpu>)> gpu_picker_func);

	void fetch_adequate_gpus();

	void create_device(vk::PhysicalDeviceFeatures physical_device_features);

	void fetch_queues();

	void create_allocator();

	void create_command_pools();

	auto create_queues_create_info() const -> vec<vk::DeviceQueueCreateInfo>;

	auto has_layer(c_str layer) const -> bool;

private:
	PFN_vkGetInstanceProcAddr pfn_vk_get_instance_proc_addr = {};

	fn<vk::Extent2D()> get_framebuffer_extent = {};

	pair<fn<void(DebugCallbackSource, LogLvl, str const &, std::any)>, std::any>
	    debug_callback_and_data = {};

private:
	vec<c_str> layers = {};

	vk::Instance instance = {};
	vec<c_str> instance_extensions = {};

	vk::Device device = {};
	vec<c_str> device_extensions = {};

	vec<Gpu> adequate_gpus = {};

	vma::Allocator allocator = {};

	Queues queues = {};
	Surface surface = {};
	Swapchain swapchain = {};
	Gpu gpu = {};

	vk::Format depth_format = {};

	// @todo Move the damned threading stuff out of vkcontext!!
	u32 num_threads = 1u;

	vec<vk::CommandPool> dynamic_cmd_pools = {};
	vk::CommandPool immediate_cmd_pool = {};

	vk::CommandBuffer immediate_cmd = {};
	vk::Fence immediate_fence = {};

	vk::DebugUtilsMessengerEXT debug_util_messenger = {};
};

} // namespace BINDLESSVK_NAMESPACE
