#pragma once

#include "BindlessVk/Common/Common.hpp"

#include <any>

namespace BINDLESSVK_NAMESPACE {

// @todo move cmd pool management and thread stuff to a dedicated class
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

		inline operator auto() const
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

	    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
	    fn<vk::Extent2D()> get_framebuffer_extent_func,

	    bool has_debugging,
	    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
	    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,
	    PFN_vkDebugUtilsMessengerCallbackEXT vulkan_debug_callback,
	    void* debug_messenger_userptr,

	    PFN_vmaAllocateDeviceMemoryFunction vma_allocate_device_memory_callback,
	    PFN_vmaFreeDeviceMemoryFunction vma_free_device_memory_callback,
	    void* vma_callback_user_data,

	    fn<void(LogLvl, const str& message, std::any user_data)> bindlessvk_debug_callback,
	    std::any bindlessvk_debug_callback_user_data
	);

	VkContext& operator=(VkContext&& other);

	~VkContext();


	fn<void(LogLvl, const str&, std::any)> debug_callback;
	std::any debug_callback_user_data;

	/** Updates the surface info, (to be called after swapchain-invalidation) */
	void update_surface_info();

	void immediate_submit(fn<void(vk::CommandBuffer)>&& func)
	{
		vk::CommandBufferAllocateInfo allocInfo {
			immediate_cmd_pool,
			vk::CommandBufferLevel::ePrimary,
			1u,
		};

		vk::CommandBuffer cmd = device.allocateCommandBuffers(allocInfo)[0];

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
	inline void log(LogLvl lvl, fmt::format_string<Args...> fmt, Args&&... args) const
	{
		debug_callback(
		    lvl,
		    fmt::format(fmt, std::forward<Args>(args)...),
		    debug_callback_user_data
		);
	}

	template<typename T>
	inline void set_object_name(T object, c_str name)
	{
		device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.objectType,
		    (u64)((typename T::NativeType)(object)),
		    name,
		});
	}

	inline auto get_cmd_pool(u32 frame_index, u32 thread_index = 0)
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

	inline const auto& get_available_gpus() const
	{
		return available_gpus;
	}

	inline auto get_allocator() const
	{
		return allocator;
	}

	inline const auto& get_queues() const
	{
		return queues;
	}

	inline const auto& get_surface() const
	{
		return surface;
	}

	inline auto get_depth_format() const
	{
		return depth_format;
	}

	inline auto get_depth_format_has_stencil() const
	{
		return depth_format == vk::Format::eD32SfloatS8Uint
		    || depth_format == vk::Format::eD24UnormS8Uint;
	}

	inline const auto get_max_color_and_depth_samples() const
	{
		return max_color_and_depth_samples;
	}

	inline const auto get_max_color_samples() const
	{
		return max_color_samples;
	}

	inline const auto get_max_depth_samples() const
	{
		return max_depth_samples;
	}

	// @warn
	inline const auto get_num_threads() const
	{
		return num_threads;
	}

	// @todo what the hellll
	PFN_vkGetInstanceProcAddr get_vk_instance_proc_addr;

private:
	void check_layer_support();

	void create_vulkan_instance(
	    fn<vk::SurfaceKHR(vk::Instance)> create_window_surface_func,
	    bool has_debugging,
	    vk::DebugUtilsMessageSeverityFlagsEXT debug_messenger_severities,
	    vk::DebugUtilsMessageTypeFlagsEXT debug_messenger_types,
	    PFN_vkDebugUtilsMessengerCallbackEXT debug_messenger_callback_func,
	    void* debug_messenger_userptr
	);

	void pick_physical_device();

	void create_logical_device();

	void create_allocator(
	    PFN_vmaAllocateDeviceMemoryFunction vma_free_device_memory_callback,
	    PFN_vmaFreeDeviceMemoryFunction vma_allocate_device_memory_callback,
	    void* vma_callback_user_data
	);

	void create_command_pools();

private:
	fn<vk::Extent2D()> get_framebuffer_extent;

private:
	vec<c_str> layers;
	vec<c_str> instance_extensions;
	vec<c_str> device_extensions;

	vk::Instance instance;

	vk::Device device;
	vk::PhysicalDevice selected_gpu;
	vec<vk::PhysicalDevice> available_gpus;


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

	vk::DynamicLoader dynamic_loader;
	vk::DebugUtilsMessengerEXT debug_util_messenger = {};
};

} // namespace BINDLESSVK_NAMESPACE
