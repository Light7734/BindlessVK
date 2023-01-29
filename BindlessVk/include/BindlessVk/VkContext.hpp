#pragma once

#include "BindlessVk/Common/Common.hpp"

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
	// @warn do not reorder fields! they have to be contiguous in memory
	struct Queues
	{
		vk::Queue graphics;
		vk::Queue present;
		vk::Queue compute;

		u32 graphics_index;
		u32 present_index;
		u32 compute_index;

		// @note compute index is not considered
		inline auto have_same_index() const
		{
			return graphics_index == present_index == compute_index;
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

public:
	VkContext() = default;
	VkContext(
	    vec<c_str> layers,
	    vec<c_str> instance_extensions,
	    vec<c_str> device_extensions,

	    vk::PhysicalDeviceFeatures physical_device_features,

	    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
	    fn<vk::Extent2D()> get_framebuffer_extent_func,

	    bool has_debugging,
	    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
	    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,

	    fn<void(DebugCallbackSource, LogLvl, str const &, std::any)> bindlessvk_debug_callback,
	    std::any bindlessvk_debug_callback_user_data
	);

	VkContext(VkContext &&other);

	VkContext &operator=(VkContext &&other);

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
		auto [callback, data] = debug_callback_data;

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
		return selected_gpu;
	}

	inline auto const &get_available_gpus() const
	{
		return available_gpus;
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

	inline auto get_max_color_and_depth_samples() const
	{
		return max_color_and_depth_samples;
	}

	inline auto get_max_color_samples() const
	{
		return max_color_samples;
	}

	inline auto get_max_depth_samples() const
	{
		return max_depth_samples;
	}

	inline auto get_num_threads() const
	{
		return num_threads;
	}

	inline auto const get_pfn_get_vk_instance_proc_addr() const
	{
		return pfn_vk_get_instance_proc_addr;
	}

private:
	void check_layer_support();

	void create_vulkan_instance(
	    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
	    bool has_debugging,
	    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
	    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types
	);

	void pick_physical_device();

	void create_logical_device();

	void fetch_queues();

	void create_allocator();

	void create_command_pools();

	auto create_queues_create_info() const -> vec<vk::DeviceQueueCreateInfo>;

private:
	vk::DynamicLoader dynamic_loader;

	PFN_vkGetInstanceProcAddr pfn_vk_get_instance_proc_addr;

	fn<vk::Extent2D()> get_framebuffer_extent;

	static pair<fn<void(DebugCallbackSource, LogLvl, str const &, std::any)>, std::any>
	    debug_callback_data;

private:
	vec<c_str> layers;

	vk::Instance instance;
	vec<c_str> instance_extensions;

	vk::Device device;
	vec<c_str> device_extensions;


	vk::PhysicalDevice selected_gpu;
	vec<vk::PhysicalDevice> available_gpus;
	vk::PhysicalDeviceFeatures physical_device_features;

	vma::Allocator allocator;

	Queues queues;
	Surface surface;

	vk::Format depth_format;

	vk::SampleCountFlagBits max_color_and_depth_samples;
	vk::SampleCountFlagBits max_color_samples;
	vk::SampleCountFlagBits max_depth_samples;

	u32 num_threads;

	vec<vk::CommandPool> dynamic_cmd_pools;
	vk::CommandPool immediate_cmd_pool;

	vk::CommandBuffer immediate_cmd;
	vk::Fence immediate_fence;

	vk::DebugUtilsMessengerEXT debug_util_messenger = {};
};

} // namespace BINDLESSVK_NAMESPACE
