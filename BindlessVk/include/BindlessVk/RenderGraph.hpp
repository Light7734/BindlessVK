#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Device.hpp"
#include "BindlessVk/RenderPass.hpp"

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
	RenderGraph();

	/** Initializes the render graph
	 * @param device the bindlessvk device
	 * @param descriptor_pool a descriptor pool
	 * @param swaphain_images the swapchain's images
	 * @param swapchain_iamge_views the swapchain's image views
	 */
	void init(
	    Device* device,
	    vk::DescriptorPool descriptor_pool,
	    std::vector<vk::Image> swapchain_images,
	    std::vector<vk::ImageView> swapchain_image_views
	);

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
	 *
	 * @param on_begin_frame function to be called at the beginning of the frame
	 * (eg. start imgui frame)
	 */
	void build(
	    std::string backbuffer_name,
	    std::vector<Renderpass::CreateInfo::BufferInputInfo> buffer_inputs,
	    std::vector<Renderpass::CreateInfo> renderpasses,
	    void (*on_update)(Device*, RenderGraph*, u32, void*),
	    void (*on_begin_frame)(Device*, RenderGraph*, u32, void*),
	    vk::DebugUtilsLabelEXT update_debug_label,
	    vk::DebugUtilsLabelEXT backbuffer_barrier_debug_label
	);

	/** Recreates swapchain-dependent resources
	 * @param swapchain_images newly created (valid) swapchain images
	 * @param swapchain_image_views newly created (valid) swapchain image views
	 */
	void on_swapchain_invalidated(
	    std::vector<vk::Image> swapchain_images,
	    std::vector<vk::ImageView> swapchain_iamge_views
	);

	/** Calls on_begin frame functions of the graph & passes
	 * @param device:
	 */
	void begin_frame(u32 frame_index, void* user_pointer);

	void end_frame(
	    vk::CommandBuffer primary_cmd,
	    u32 frame_index,
	    u32 image_index,
	    void* user_pointer
	);

	inline void* map_descriptor_buffer(const char* name, u32 frame_index)
	{
		return m_buffer_inputs[HashStr(name)]->map_block(frame_index);
	}

	inline void unmap_descriptor_buffer(const char* name)
	{
		m_buffer_inputs[HashStr(name)]->unmap();
	}

private:
	inline vk::CommandBuffer get_cmd(u32 pass_index, u32 frame_index, u32 thread_index) const
	{
		const u32 numPass = m_renderpasses.size();
		const u32 numThreads = m_device->num_threads;

		return m_secondary_cmd_buffers
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

	void record_pass_cmds(
	    vk::CommandBuffer cmd,
	    u32 frame_index,
	    u32 image_index,
	    u32 pass_index,
	    void* user_pointer
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
		std::vector<AllocatedImage> image;
		std::vector<vk::ImageView> image_views;

		// Transinet multi-sampled
		std::vector<AllocatedImage> transient_ms_image;
		std::vector<vk::ImageView> transient_ms_image_view;
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
		std::string relative_size_name;

		// Transient multi-sampled images
		vk::SampleCountFlagBits sample_count;
		vk::ResolveModeFlagBits transient_ms_resolve_mode;
		AllocatedImage transient_ms_image;
		vk::ImageView transient_ms_image_view;

		// Required for aliasing Read-Modify-Write attachments
		std::string last_write_name;

		std::vector<AttachmentResource> resources;

		Renderpass::CreateInfo::AttachmentInfo cached_renderpass_info;

		inline AttachmentResource& get_resource(u32 image_index, u32 frame_index)
		{
			return type == Type::ePerImage ? resources[image_index] :
			       type == Type::ePerFrame ? resources[frame_index] :
			                                 resources[0];
		};
	};

	void create_attachment_resource(
	    const Renderpass::CreateInfo::AttachmentInfo& attachment_info,
	    RenderGraph::AttachmentResourceContainer::Type attachment_type,
	    u32 recreate_resource_index
	);


private:
	Device* m_device = {};

	vk::DescriptorPool m_descriptor_pool = {};
	std::vector<Renderpass> m_renderpasses = {};

	vk::PipelineLayout m_pipeline_layout = {};
	vk::DescriptorSetLayout m_descriptor_set_layout = {};
	std::vector<vk::DescriptorSet> m_sets = {};

	void (*m_on_update)(Device*, RenderGraph*, u32, void*);
	void (*m_on_begin_frame)(Device*, RenderGraph*, u32, void*);

	std::vector<AttachmentResourceContainer> m_attachment_resources = {};

	std::unordered_map<uint64_t, Buffer*> m_buffer_inputs = {};

	std::vector<vk::Image> m_swapchain_images = {};
	std::vector<vk::ImageView> m_swapchain_image_views = {};

	std::vector<std::string> m_swapchain_attachment_names = {};
	std::string m_swapchain_resource_names = {};
	u32 m_swapchain_resource_index = {};

	std::vector<Renderpass::CreateInfo> m_renderpasses_info = {};
	std::vector<Renderpass::CreateInfo::BufferInputInfo> m_buffer_inputs_info = {};

	std::vector<vk::CommandBuffer> m_secondary_cmd_buffers = {};

	vk::SampleCountFlagBits m_sample_count = {};

	vk::DebugUtilsLabelEXT m_update_debug_label;
	vk::DebugUtilsLabelEXT m_backbuffer_barrier_debug_label;
};

} // namespace BINDLESSVK_NAMESPACE
