#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/RenderGraph.hpp"
#include "BindlessVk/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

/** @brief
 *
 */
class Renderer
{
public:
	Renderer() = default;

	Renderer(VkContext *vk_context);

	Renderer(Renderer &&other);
	Renderer(const Renderer &rhs) = delete;

	Renderer &operator=(Renderer &&rhs);
	Renderer &operator=(const Renderer &rhs) = delete;

	~Renderer();

	void on_swapchain_invalidated();

	void render_frame(RenderGraph &render_graph, void *user_pointer);

	/** @return swapchain images */
	inline auto get_swapchain_images() const
	{
		return swapchain_images;
	}

	/** @return swapchain image views */
	inline auto get_swapchain_image_views() const
	{
		return swapchain_image_views;
	}

	/** @return swapchain image count */
	inline auto get_image_count() const
	{
		return swapchain_images.size();
	}

	/** @return wether or not swapchain is invalidated */
	inline auto is_swapchain_invalid() const
	{
		return swapchain_invalid;
	}

private:
	void create_sync_objects();
	void create_descriptor_pools();

	void create_swapchain();

	void create_cmd_buffers();

	void destroy_swapchain_resources();
	void destroy_sync_objects();

	void submit_queue(
	    vk::Semaphore wait_semaphores,
	    vk::Semaphore signal_semaphore,
	    vk::Fence signal_fence,
	    vk::CommandBuffer cmd
	);

	void present_frame(u32 image_index);

private:
	VkContext *vk_context = {};

	// Sync objects
	arr<vk::Fence, BVK_MAX_FRAMES_IN_FLIGHT> render_fences = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> render_semaphores = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> present_semaphores = {};

	bool swapchain_invalid = false;

	// Swapchain
	vk::SwapchainKHR swapchain = {};

	vec<vk::Image> swapchain_images {};
	vec<vk::ImageView> swapchain_image_views = {};

	vec<vk::CommandBuffer> cmd_buffers = {};

	u32 current_frame = 0ul;
};

} // namespace BINDLESSVK_NAMESPACE
