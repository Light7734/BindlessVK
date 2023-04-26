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
	// 1
	void create_sync_objects();
	void create_cmds(Gpu const *gpu);

	void destroy_cmds();
	void destroy_sync_objects();

	void prepare_frame(Rendergraph *graph);
	void compute_frame(Rendergraph *graph);
	void render_frame(Rendergraph *graph);
	void present_frame(Rendergraph *graph);

	void cycle_frame_index();
	auto acquire_next_image_index() -> u32;

	// 2
	void create_compute_sync_objects(u32 index);
	void create_graphics_sync_objects(u32 index);
	void create_present_sync_objects(u32 index);

	void create_compute_cmds(Gpu const *gpu, u32 index);
	void create_graphics_cmds(Gpu const *gpu, u32 index);

	void reset_used_attachment_states();

	void wait_for_compute_fence();
	void wait_for_graphics_fence();

	void bind_graph_compute_descriptor(Rendergraph *graph);
	void bind_graph_graphics_descriptor(Rendergraph *graph);

	void apply_graph_barriers(Rendergraph *graph);

	void compute_pass(Rendergraph *graph, Renderpass *pass);
	void graphics_pass(Rendergraph *graph, Renderpass *pass);

	void submit_compute_queue();
	void submit_graphics_queue();
	void submit_present_queue();

	// 3

	void bind_pass_compute_descriptor(Renderpass *pass);
	void bind_pass_graphics_descriptor(Renderpass *pass);

	void apply_pass_barriers(Renderpass *pass);

private:
	Device const *device = {};
	Surface const *surface = {};
	Queues const *queues = {};
	DebugUtils const *debug_utils = {};
	Swapchain swapchain = {};

	vec<tuple<u32, u32, u32>> used_attachment_indices = {};

	RenderResources resources = {};
	arr<DynamicPassInfo, max_frames_in_flight> dynamic_pass_info;

	arr<vk::Fence, max_frames_in_flight> compute_fences = {};
	arr<vk::Semaphore, max_frames_in_flight> compute_semaphores = {};

	arr<vk::Fence, max_frames_in_flight> graphics_fences = {};
	arr<vk::Semaphore, max_frames_in_flight> graphics_semaphores = {};

	arr<vk::Semaphore, max_frames_in_flight> present_semaphores = {};

	arr<vk::CommandPool, max_frames_in_flight> compute_cmd_pools = {};
	arr<vk::CommandPool, max_frames_in_flight> graphics_cmd_pools = {};

	arr<vk::CommandBuffer, max_frames_in_flight> compute_cmds = {};
	arr<vk::CommandBuffer, max_frames_in_flight> graphics_cmds = {};

	u32 frame_index = 0;
	u32 image_index = std::numeric_limits<u32>::max();
};

} // namespace BINDLESSVK_NAMESPACE
