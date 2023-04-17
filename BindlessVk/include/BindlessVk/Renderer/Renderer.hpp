#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Swapchain.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Renderer/Rendergraph.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Renders and presents render graphs */
class Renderer
{
public:
	/** Default constructor */
	Renderer() = default;

	/** Argumented constructor
	 *
	 * @param vk_context Pointer to the vk context
	 * @param memory_allocator Pointer to the memory allocator
	 */
	Renderer(VkContext const *vk_context, MemoryAllocator *memory_allocator);

	/** Move constructor */
	Renderer(Renderer &&other);

	/** Move assignment operator */
	Renderer &operator=(Renderer &&other);

	/** Deleted copy constructor */
	Renderer(Renderer const &) = delete;

	/** Deleted copy assignment operator */
	Renderer &operator=(Renderer const &) = delete;

	/** Destructor */
	~Renderer();

	/** Renders a graph and presents it
	 *
	 * @param render_graph The graph to render
	 *
	 * @note also calls on_update for the graph's passes
	 *
	 * @warn may block execution to wait for the frame's fence
	 */
	void render_graph(Rendergraph *render_graph);

	/** Returns pointer to resources */
	auto get_resources()
	{
		return &resources;
	}

	/** Checks swapchain validity */
	bool is_swapchain_invalid() const
	{
		return swapchain.is_invalid();
	}

private:
	struct DynamicPassInfo
	{
		vec<vk::RenderingAttachmentInfo> color_attachments;
		vk::RenderingAttachmentInfo depth_attachment;
		vk::RenderingInfo rendering_info;

		operator vk::RenderingInfo() const
		{
			return rendering_info;
		}
	};

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

	void create_cmds(Gpu const *gpu);
	void destroy_cmds();

private:
	Device const *device = {};
	Surface const *surface = {};
	Queues const *queues = {};
	DebugUtils const *debug_utils = {};
	Swapchain swapchain = {};

	vec<tuple<u32, u32, u32>> used_attachment_indices = {};

	RenderResources resources = {};
	arr<DynamicPassInfo, max_frames_in_flight> dynamic_pass_info;

	arr<vk::Fence, max_frames_in_flight> render_fences = {};
	arr<vk::Semaphore, max_frames_in_flight> render_semaphores = {};
	arr<vk::Semaphore, max_frames_in_flight> present_semaphores = {};

	arr<vk::CommandPool, max_frames_in_flight> cmd_pools = {};
	arr<vk::CommandBuffer, max_frames_in_flight> cmd_buffers = {};

	u32 frame_index = 0;
};

} // namespace BINDLESSVK_NAMESPACE
