#pragma once

#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

struct Device
{
	// Layers & Extensions
	vec<c_str> layers;
	vec<c_str> instance_extensions;
	vec<c_str> device_extensions;

	// instance
	PFN_vkGetInstanceProcAddr get_vk_instance_proc_addr_func;
	vk::Instance instance;

	// device
	vk::Device logical;
	vk::PhysicalDevice physical;

	vk::PhysicalDeviceProperties properties;

	vk::Format depth_format;
	bool has_stencil;

	vk::SampleCountFlagBits max_samples;
	vk::SampleCountFlagBits max_color_samples;
	vk::SampleCountFlagBits max_depth_samples;

	// queue
	vk::Queue graphics_queue;
	vk::Queue present_queue;
	vk::Queue compute_queue;

	u32 graphics_queue_index;
	u32 present_queue_index;
	u32 compute_queue_index;

	/** Native platform surface */
	vk::SurfaceKHR surface;

	/** Surface capabilities */
	vk::SurfaceCapabilitiesKHR surface_capabilities;

	/** Chosen surface format */
	vk::SurfaceFormatKHR surface_format;

	/** Chosen surface present mode */
	vk::PresentModeKHR present_mode;

	/** */
	fn<vk::Extent2D()> get_framebuffer_extent_func;

	/** */
	vk::Extent2D framebuffer_extent;

	/** Vulkan memory allocator */
	vma::Allocator allocator;

	/** Max number of recording threads */
	u32 num_threads;

	/** A command pool for every (thread * frame) */
	vec<vk::CommandPool> dynamic_cmd_pools;

	/** Command pool for immediate submission */
	vk::CommandPool immediate_cmd_pool;

	vk::CommandBuffer immediate_cmd;
	vk::Fence immediate_fence;

	fn<void(LogLvl, const str& message)> debug_callback;


	vk::CommandPool get_cmd_pool(u32 frame_index, u32 thread_index = 0)
	{
		return dynamic_cmd_pools[(thread_index * num_threads) + frame_index];
	}

	void immediate_submit(fn<void(vk::CommandBuffer)>&& func)
	{
		vk::CommandBufferAllocateInfo allocInfo {
			immediate_cmd_pool,
			vk::CommandBufferLevel::ePrimary,
			1u,
		};

		vk::CommandBuffer cmd = logical.allocateCommandBuffers(allocInfo)[0];

		vk::CommandBufferBeginInfo beginInfo { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };

		cmd.begin(beginInfo);
		func(cmd);
		cmd.end();

		vk::SubmitInfo submit_info { 0u, {}, {}, 1u, &cmd, 0u, {}, {} };
		graphics_queue.submit(submit_info, immediate_fence);

		assert_false(logical.waitForFences(immediate_fence, true, UINT_MAX));
		logical.resetFences(immediate_fence);
		logical.resetCommandPool(immediate_cmd_pool);
	}

	template<typename T>
	void set_object_name(T object, c_str name)
	{
		logical.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
		    object.objectType,
		    (u64)((typename T::NativeType)(object)),
		    name,
		});
	}
};

class DeviceSystem
{
public:
	DeviceSystem() = default;

	/** @return the bindlessvk device */
	inline Device* get_device()
	{
		return &device;
	}

	/** Initializes the device system
	 * @param layers vulkan layers to attach
	 * @param instance_extensions instance extensions to load
	 * @param device_extensions device extensions to load
	 * @param window_extensions required window surface extensions
	 *
	 * @param create_window_surface_func function to create window surface
	 *  given an instance of vulkan (eg. glfwCreateWindowSurface)
	 * @param get_framebuffer_extent_func function to get window's framebuffer
	 *  extent (eg. glfwGetFramebufferSize)
	 *
	 * @param has_debugging should attach attach validation layers
	 * @param debug_messenger_severities allowed severities for debug messenger
	 * @param debug_messenger_types allowed types for debug messenger
	 * @param debug_messenger_callback_func callback function for debug messenger
	 * @param debug_messenger_userptr user pointer passed to debug messenger
	 */
	void init(
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
	    PFN_vmaFreeDeviceMemoryFunction vma_allocate_device_memory_callback,

	    fn<void(LogLvl, const str& message)> bindlessvk_debug_callback
	);

	/** Destroys the device system */
	void reset();

	/** Updates the surface info, (to be called after swapchain-invalidation) */
	void update_surface_info();

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
	    PFN_vmaFreeDeviceMemoryFunction vma_allocate_device_memory_callback
	);

	void create_command_pools();

private:
	Device device = {};

	vk::DynamicLoader dynamic_loader;
	vk::DebugUtilsMessengerEXT debug_util_messenger = {};
};

} // namespace BINDLESSVK_NAMESPACE
