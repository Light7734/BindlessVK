#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/DescriptorAllocator.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/VkContext.hpp"

#ifndef MAX_FRAMES_IN_FLIGHT
	#define MAX_FRAMES_IN_FLIGHT 3
#endif

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif

namespace BINDLESSVK_NAMESPACE {

class RenderGraph
{
public:
	RenderGraph(
	    VkContext *vk_context,
	    vec<vk::Image> swapchain_images,
	    vec<vk::ImageView> swapchain_image_views
	);

	~RenderGraph();

	/** Destroys the render graph
	 */
	void reset();

	/** Builds the render graph
	 *
	 * @param backbuffer_name output name of one of the renderpasses color
	 * attachments (this is supposed to be swapchain's image resource)
	 *
	 * @param buffer_inputs frame descriptor infos (set = 0), updated
	 * once-per-frame
	 *
	 * @param renderpasses renderpass create infos that make up the graph
	 *
	 * @param on_update function to be called before rendering to update the
	 * graph descriptor sets (set = 0)
	 */
	void build(
	    str backbuffer_name,
	    vec<Renderpass::CreateInfo::BufferInputInfo> buffer_inputs,
	    vec<Renderpass::CreateInfo> renderpasses,
	    void (*on_update)(VkContext *, RenderGraph *, u32, void *),
	    vk::DebugUtilsLabelEXT update_debug_label,
	    vk::DebugUtilsLabelEXT backbuffer_barrier_debug_label
	);

	/** Recreates swapchain-dependent resources
	 * @param swapchain_images newly created (valid) swapchain images
	 * @param swapchain_image_views newly created (valid) swapchain image views
	 */
	void on_swapchain_invalidated(
	    vec<vk::Image> swapchain_images,
	    vec<vk::ImageView> swapchain_iamge_views
	);

	void update_and_render(
	    vk::CommandBuffer primary_cmd,
	    u32 frame_index,
	    u32 image_index,
	    void *user_pointer
	);

	inline void *map_descriptor_buffer(const char *name, u32 frame_index)
	{
		return buffer_inputs[hash_str(name)]->map_block(frame_index);
	}

	inline void unmap_descriptor_buffer(const char *name)
	{
		buffer_inputs[hash_str(name)]->unmap();
	}


private:
	inline auto get_cmd(u32 pass_index, u32 frame_index, u32 thread_index) const
	{
		const u32 numPass = renderpasses.size();
		const u32 numThreads = vk_context->get_num_threads();

		return secondary_cmd_buffers
		    [pass_index + (numPass * (thread_index + (numThreads * frame_index)))];
	}

	void create_cmd_buffers();

	// @todo Implement
	void validate_graph();
	// @todo Implement
	void reorder_passes();

	void build_attachment_resources();

	// @todo Implement
	void build_graph_texture_inputs();
	// @todo Implement
	void build_passes_texture_inputs();

	void build_graph_buffer_inputs();
	void build_passes_buffer_inputs();

	void build_graph_sets();
	void build_passes_sets();

	void write_graph_sets();
	void write_passes_sets();

	void build_pass_cmd_buffer_begin_infos();

	vk::Extent3D calculate_attachment_image_extent(
	    const Renderpass::CreateInfo::AttachmentInfo &attachment_info
	);

	void record_pass_cmds(
	    vk::CommandBuffer cmd,
	    u32 frame_index,
	    u32 image_index,
	    u32 pass_index,
	    void *user_pointer
	);

	struct PassRenderingInfo
	{
		vec<vk::RenderingAttachmentInfo> color_attachments_info;
		vk::RenderingAttachmentInfo depth_attachment_info;
		vk::RenderingInfo rendering_info;
	};

	PassRenderingInfo apply_pass_barriers(
	    vk::CommandBuffer cmd,
	    u32 frame_index,
	    u32 image_index,
	    u32 pass_index
	);

	void apply_backbuffer_barrier(vk::CommandBuffer cmd, u32 frame_index, u32 image_index);

private:
	struct ImageAttachment
	{
		vec<AllocatedImage> image;
		vec<vk::ImageView> image_views;

		// Transinet multi-sampled
		vec<AllocatedImage> transient_ms_image;
		vec<vk::ImageView> transient_ms_image_view;
	};

	struct AttachmentResource
	{
		// State carried from last barrier
		vk::AccessFlags src_access_mask;
		vk::ImageLayout src_image_layout;
		vk::PipelineStageFlags src_stage_mask;

		AllocatedImage image;
		vk::ImageView image_view;
	};

	struct AttachmentResourceContainer
	{
		enum class Type
		{
			ePerImage,
			ePerFrame,
			eSingle,
		} type;

		vk::Format image_format;

		VkExtent3D extent;
		glm::vec2 size;
		Renderpass::CreateInfo::SizeType size_type;
		str relative_size_name;

		// Transient multi-sampled images
		vk::SampleCountFlagBits sample_count;
		vk::ResolveModeFlagBits transient_ms_resolve_mode;
		AllocatedImage transient_ms_image;
		vk::ImageView transient_ms_image_view;

		// Required for aliasing Read-Modify-Write attachments
		str last_write_name;

		vec<AttachmentResource> resources;

		Renderpass::CreateInfo::AttachmentInfo cached_renderpass_info;

		inline AttachmentResource &get_resource(u32 image_index, u32 frame_index)
		{
			return type == Type::ePerImage ? resources[image_index] :
			       type == Type::ePerFrame ? resources[frame_index] :
			                                 resources[0];
		};
	};

	void create_attachment_resource(
	    const Renderpass::CreateInfo::AttachmentInfo &attachment_info,
	    RenderGraph::AttachmentResourceContainer::Type attachment_type,
	    u32 recreate_resource_index
	);

private:
	VkContext *vk_context = {};

	vk::DescriptorPool descriptor_pool = {};
	vec<Renderpass> renderpasses = {};

	vk::PipelineLayout pipeline_layout = {};
	vk::DescriptorSetLayout descriptor_set_layout = {};
	vec<vk::DescriptorSet> sets = {};

	void (*on_update)(VkContext *, RenderGraph *, u32, void *);

	vec<AttachmentResourceContainer> attachment_resources = {};

	std::unordered_map<uint64_t, Buffer *> buffer_inputs = {};

	vec<vk::Image> swapchain_images = {};
	vec<vk::ImageView> swapchain_image_views = {};

	vec<str> swapchain_attachment_names = {};
	str swapchain_resource_names = {};
	u32 swapchain_resource_index = {};

	vec<Renderpass::CreateInfo> renderpasses_info = {};
	vec<Renderpass::CreateInfo::BufferInputInfo> buffer_inputs_info = {};

	vec<vk::CommandBuffer> secondary_cmd_buffers = {};

	vk::SampleCountFlagBits sample_count = {};

	vk::DebugUtilsLabelEXT update_debug_label;
	vk::DebugUtilsLabelEXT backbuffer_barrier_debug_label;
};

} // namespace BINDLESSVK_NAMESPACE
