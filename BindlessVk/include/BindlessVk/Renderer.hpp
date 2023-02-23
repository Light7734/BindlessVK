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
private:
	struct DynamicPassInfo
	{
		vec<vk::RenderingAttachmentInfo> color_attachments;
		vk::RenderingAttachmentInfo depth_attachment;
		vk::RenderingInfo rendering_info;

		inline operator vk::RenderingInfo() const
		{
			return rendering_info;
		}
	};

public:
	Renderer(ref<VkContext> vk_context);
	~Renderer();

	void render_graph(Rendergraph *render_graph);

	inline auto get_resources()
	{
		return &resources;
	}

private:
	void wait_for_frame_fence();

	auto aquire_next_image() -> u32;

	void submit_queue();

	void present_frame(u32 image_index);

	void reset_used_attachment_states();

	void update_pass(Renderpass *pass, u32 image_index);
	void apply_pass_barriers(Renderpass *pass, u32 image_index);
	void apply_present_barriers(Rendergraph *graph, u32 image_index);
	void render_pass(Rendergraph *graph, Renderpass *pass, u32 image_index);

	void create_sync_objects();
	void destroy_sync_objects();

	void allocate_cmd_buffers();
	void free_cmd_buffers();

private:
	ref<VkContext> vk_context = {};
	vec<tuple<u32, u32, u32>> used_attachment_indices = {};

	RenderResources resources;
	arr<DynamicPassInfo, BVK_MAX_FRAMES_IN_FLIGHT> dynamic_pass_info;

	arr<vk::Fence, BVK_MAX_FRAMES_IN_FLIGHT> render_fences = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> render_semaphores = {};
	arr<vk::Semaphore, BVK_MAX_FRAMES_IN_FLIGHT> present_semaphores = {};

	vec<vk::CommandBuffer> cmd_buffers = {};

	u32 frame_index = 0;
};

} // namespace BINDLESSVK_NAMESPACE
