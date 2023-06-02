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

	/** Default move constructor */
	Renderer(Renderer &&other);

	/** Default move assignment operator */
	Renderer &operator=(Renderer &&other) = default;

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
	 * @warning may block execution to wait for the frame's fence
	 */
	void render_graph(RenderNode *root_node);

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
	struct DynamicPassRenderingInfo
	{
		vec<vk::RenderingAttachmentInfo> color_attachments;
		vk::RenderingAttachmentInfo depth_attachment;
		vk::RenderingInfo rendering_info;

		void reset()
		{
			color_attachments.clear();
			depth_attachment = vk::RenderingAttachmentInfo {};
		}

		auto vk() const
		{
			return rendering_info;
		}
	};

private:
	void create_sync_objects();
	void create_cmds(Gpu const *gpu);

	void destroy_cmds();
	void destroy_sync_objects();

	void wait_for_frame_fence();
	void prepare_frame(RenderNode *graph);
	void compute_frame(RenderNode *graph);
	void graphics_frame(RenderNode *graph);
	void present_frame();

	void cycle_frame_index();
	auto acquire_next_image_index() -> u32;

	void create_frame_sync_objects(u32 index);
	void create_compute_sync_objects(u32 index);
	void create_graphics_sync_objects(u32 index);
	void create_present_sync_objects(u32 index);

	void create_compute_cmds(Gpu const *gpu, u32 index);
	void create_graphics_cmds(Gpu const *gpu, u32 index);

	void prepare_node(RenderNode *graph);
	void compute_node(RenderNode *graph, i32 depth);
	void graphics_node(RenderNode *graph, i32 depth);

	void reset_used_attachment_states();

	void bind_node_compute_descriptors(RenderNode *node, i32 depth);
	void bind_node_graphics_descriptors(RenderNode *node, i32 depth);

	void apply_backbuffer_barrier();
	void apply_node_barriers(RenderNode *node);

	void submit_compute_queue();
	void submit_graphics_queue();
	void submit_present_queue();

	auto try_apply_node_barriers(RenderNode *node) -> bool;

	void parse_node_rendering_info(RenderNode *node);

private:
	tidy_ptr<Device const> device = {};

	Surface const *surface = {};
	Queues const *queues = {};
	DebugUtils const *debug_utils = {};

	Swapchain swapchain = {};

	vec<tuple<u32, u32, u32>> used_attachment_indices = {};

	RenderResources resources = {};
	DynamicPassRenderingInfo dynamic_pass_rendering_info;

	arr<vk::Fence, max_frames_in_flight> frame_fences = {};

	arr<vk::Semaphore, max_frames_in_flight> compute_semaphores = {};

	arr<vk::Semaphore, max_frames_in_flight> graphics_semaphores = {};

	arr<vk::Semaphore, max_frames_in_flight> present_semaphores = {};

	arr<vk::CommandPool, max_frames_in_flight> compute_cmd_pools = {};
	arr<vk::CommandPool, max_frames_in_flight> graphics_cmd_pools = {};

	arr<vk::CommandBuffer, max_frames_in_flight> compute_cmds = {};
	arr<vk::CommandBuffer, max_frames_in_flight> graphics_cmds = {};

	u32 frame_index = 0;
	u32 image_index = std::numeric_limits<u32>::max();

	bool dynamic_render_pass_active = false;
};

} // namespace BINDLESSVK_NAMESPACE
