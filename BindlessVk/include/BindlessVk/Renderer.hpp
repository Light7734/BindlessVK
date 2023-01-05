#pragma once

#include "BindlessVk/Common.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/RenderGraph.hpp"

namespace BINDLESSVK_NAMESPACE {

class Renderer
{
public:
	Renderer() = default;

	void init(Device* device);

	void reset();

	void on_swapchain_invalidated();
	void destroy_swapchain_resources();

	void begin_frame(void* user_pointer);
	void end_frame(void* user_pointer);

	void build_render_graph(
	    std::string backbuffer_name,
	    std::vector<Renderpass::CreateInfo::BufferInputInfo> buffer_inputs,
	    std::vector<Renderpass::CreateInfo> renderpasses,
	    void (*on_update)(Device*, RenderGraph*, u32, void*),
	    void (*on_begin_frame)(Device*, RenderGraph*, u32, void*),
	    vk::DebugUtilsLabelEXT graph_update_debug_label,
	    vk::DebugUtilsLabelEXT graph_backbuffer_barrier_debug_label
	);

	/** @return descriptor pool  */
	inline vk::DescriptorPool get_descriptor_pool() const
	{
		return m_descriptor_pool;
	}

	/** @return swapchain image count  */
	inline u32 get_image_count() const
	{
		return m_swapchain_images.size();
	}

	/** @return wether or not swapchain is invalidated  */
	inline bool is_swapchain_invalidated() const
	{
		return m_is_swapchain_invalid;
	}

private:
	void create_sync_objects();
	void create_descriptor_pools();

	void create_swapchain();

	void create_cmd_buffers();

	void submit_queue(
	    vk::Semaphore wait_semaphores,
	    vk::Semaphore signal_semaphore,
	    vk::Fence signal_fence,
	    vk::CommandBuffer cmd
	);
	void present_frame(vk::Semaphore wait_semaphore, uint32_t image_index);

private:
	Device* m_device = {};
	RenderGraph m_render_graph = {};

	// Sync objects
	arr<vk::Fence, BVK_MAX_FRAMES_IN_FLIGHT> m_render_fences = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> m_render_semaphores = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> m_present_semaphores = {};

	bool m_is_swapchain_invalid = false;

	// Swapchain
	vk::SwapchainKHR m_swapchain = {};

	vec<vk::Image> m_swapchain_images {};
	vec<vk::ImageView> m_swapchain_image_views = {};

	// Pools
	vk::DescriptorPool m_descriptor_pool = {};

	vec<vk::CommandBuffer> m_command_buffers = {};

	u32 m_current_frame = 0ul;
};

} // namespace BINDLESSVK_NAMESPACE
