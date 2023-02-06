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
	Renderer(ref<VkContext> vk_context);
	~Renderer();

	void render_graph(RenderGraph *render_graph, void *user_pointer);

private:
	void wait_for_frame_fence();

	auto aquire_next_image() -> pair<bool, u32>;

	void submit_queue();

	void present_frame(u32 image_index);

	void create_sync_objects();
	void destroy_sync_objects();

	void allocate_cmd_buffers();
	void free_cmd_buffers();

private:
	ref<VkContext> vk_context = {};

	arr<vk::Fence, BVK_MAX_FRAMES_IN_FLIGHT> render_fences = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> render_semaphores = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> present_semaphores = {};

	vec<vk::CommandBuffer> cmd_buffers = {};

	u32 current_frame = 0ul;
};

} // namespace BINDLESSVK_NAMESPACE
