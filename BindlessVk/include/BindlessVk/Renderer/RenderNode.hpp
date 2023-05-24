#pragma once

#include "BindlessVk/Allocators/Descriptors/DescriptorAllocator.hpp"
#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Shader/DescriptorSet.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Describes a single compute and/or rendering step in the render graph */
class RenderNode
{
public:
	friend class RenderGraphBuilder;
	friend class RenderNodeBlueprint;

public:
	/** Repersents a render color or depth/stencil attachment */
	struct Attachment
	{
		vk::PipelineStageFlags stage_mask;
		vk::AccessFlags access_mask;
		vk::ImageLayout image_layout;
		vk::ImageSubresourceRange subresource_range;

		vk::ClearValue clear_value;
		vk::AttachmentLoadOp load_op;
		vk::AttachmentStoreOp store_op;

		u32 resource_index;
		u32 transient_resource_index;

		vk::ResolveModeFlagBits transient_resolve_mode;

		auto has_resource() const
		{
			return resource_index != std::numeric_limits<u32>::max();
		}

		auto has_transient_resource() const
		{
			return transient_resource_index != std::numeric_limits<u32>::max();
		}

		auto is_compatible_with(void *attachment) const
		{
			return false;
			// return access_mask == attachment->access_mask && //
			//        stage_mask == attachment->stage_mask &&   //
			//        image_layout == attachment->image_layout;
		}

		/** Checks wether or not image's aspect mask has color flag */
		auto is_color_attachment() const
		{
			return subresource_range.aspectMask & vk::ImageAspectFlagBits::eColor;
		}

		/** Checks wether or not image's aspect mask has depth flag */
		auto is_depth_attachment() const
		{
			return subresource_range.aspectMask & vk::ImageAspectFlagBits::eDepth;
		}
	};

	/** Determines how the size of the attachments should be interpreted */
	enum class SizeType : uint8_t
	{
		eSwapchainRelative,
		eRelative,
		eAbsolute,

		nCount,
	};

public:
	/** Default constructor */
	RenderNode() = default;

	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 */
	RenderNode(VkContext const *vk_context);

	/** Default move constructor */
	RenderNode(RenderNode &&other) = default;

	/** Default move assignment operator */
	RenderNode &operator=(RenderNode &&other) = default;

	/** Deleted copy constructor */
	RenderNode(RenderNode const &) = delete;

	/** Deleted copy assignment operator */
	RenderNode &operator=(RenderNode const &) = delete;

	/** Default virtual destuctor */
	virtual ~RenderNode() = default;

	/** Sets up the pass, called only once */
	void virtual on_setup(RenderNode *parent) = 0;

	/** Prepares the pass for rendering
	 *
	 * @param frame_index The index of the current frame
	 * @param image_index The index of the current image
	 */
	void virtual on_frame_prepare(u32 frame_index, u32 image_index) = 0;

	/** Recordrs compute command into @a cmd
	 *
	 * @param cmd A vk cmd buffer from compute queue family for compute commands to be recorded into
	 * @param frame_index The index of the current fame
	 * @param image_index The index of the current image
	 */
	void virtual on_frame_compute(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) = 0;

	/** Recordrs graphics command into @a cmd
	 *
	 * @param cmd A vk cmd buffer from graphics queue family for graphics commands to be recorded
	 * into
	 * @param frame_index The index of the current fame
	 * @param image_index The index of the current image
	 */
	void virtual on_frame_graphics(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) = 0;

	/** Returns null-terminated str view to name */
	auto get_name() const
	{
		return str_view { name };
	}

	auto &get_descriptor_set(vk::PipelineBindPoint bind_point, u32 frame_index) const
	{
		switch (bind_point)
		{
		case vk::PipelineBindPoint::eCompute: return compute_descriptor_sets[frame_index];
		case vk::PipelineBindPoint::eGraphics: return graphics_descriptor_sets[frame_index];
		default:
			assert_fail(
			    "Invalid bind point for RenderNode::get_descriptor_set: {}",
			    vk::to_string(bind_point)
			);
		}
	}

	/** Trivial reference-accessor for buffer_inputs */
	auto &get_buffer_inputs()
	{
		return buffer_inputs;
	}

	/** Checks if sample_count has more than 1 samples */
	auto is_multisampled() const
	{
		return sample_count != vk::SampleCountFlagBits::e1;
	}

	/** Trivial accessor for compute */
	auto has_compute() const
	{
		return compute;
	}

	/** Trivial accessor for graphics */
	auto has_graphics() const
	{
		return graphics;
	}

	/** Trivial reference-accessor for attachments */
	auto &get_attachments() const
	{
		return attachments;
	}

	/** Trivial reference-accessor for compute_label */
	auto &get_compute_label() const
	{
		return compute_label;
	}

	/** Trivial reference-accessor for descriptor_sets */
	auto &get_descriptor_sets() const
	{
		return graphics_descriptor_sets;
	}

	/** Trivial reference-accessor for render_label */
	auto &get_graphics_label() const
	{
		return graphics_label;
	}

	/** Trivial reference-accessor for barrier_label */
	auto &get_barrier_label() const
	{
		return barrier_label;
	}

	/** Trivial reference-accessor for barrier_label */
	auto &get_children() const
	{
		return children;
	}

	/** Trivial reference-accessor for compute_pipeline_layout */
	auto &get_compute_pipeline_layout() const
	{
		return compute_pipeline_layout;
	}

	/** Trivial reference-accessor for compute_descriptor_set_layout */
	auto &get_compute_descriptor_set_layout() const
	{
		return compute_descriptor_set_layout;
	}

	/** Trivial reference-accessor for compute_descriptor_sets */
	auto &get_compute_descriptor_sets() const
	{
		return compute_descriptor_sets;
	}

	/** Trivial reference-accessor for graphics_pipeline_layout */
	auto &get_graphics_pipeline_layout() const
	{
		return graphics_pipeline_layout;
	}

	/** Trivial reference-accessor for graphics_descriptor_set_layout */
	auto &get_graphics_descriptor_set_layout() const
	{
		return graphics_descriptor_set_layout;
	}

	/** Trivial referrence-accessor for graphics_descripto_sets */
	auto &get_graphics_descriptor_sets() const
	{
		return graphics_descriptor_sets;
	}

protected:
	VkContext const *vk_context = {};

	str name = {};

	class RenderResources *resources = {};

	vec<Attachment> attachments = {};

	bool compute = {};
	bool graphics = {};

	std::any user_data = {};

	vec<RenderNode *> children = {};

	hash_map<usize, Buffer> buffer_inputs = {};

	vk::PipelineLayout compute_pipeline_layout = {};
	vk::DescriptorSetLayout compute_descriptor_set_layout = {};
	vec<DescriptorSet> compute_descriptor_sets = {};

	vk::PipelineLayout graphics_pipeline_layout = {};
	vk::DescriptorSetLayout graphics_descriptor_set_layout = {};
	vec<DescriptorSet> graphics_descriptor_sets = {};

	vk::DebugUtilsLabelEXT prepare_label = {};
	vk::DebugUtilsLabelEXT compute_label = {};
	vk::DebugUtilsLabelEXT graphics_label = {};
	vk::DebugUtilsLabelEXT barrier_label = {};

	vec<vk::Format> color_attachment_formats = {};
	vk::Format depth_attachment_format = {};
	vk::SampleCountFlagBits sample_count = {};
};

/** Represents build instructions for a Renderpass, to be used by RenderGraphBuilder */
class RenderNodeBlueprint
{
private:
	friend class RenderGraphBuilder;

public:
	/** Repersents a blueprint render color or depth/stencil attachment */
	struct Attachment
	{
		u64 hash;
		u64 input_hash;
		u64 size_relative_hash;

		pair<f32, f32> size;
		RenderNode::SizeType size_type;
		vk::Format format;

		vk::ClearValue clear_value;

		str debug_name = "";

		// @warning this isn't the best way to check for validity...
		operator bool() const
		{
			return !!hash;
		}
	};

	struct DescriptorInfo
	{
		vk::PipelineBindPoint pipeline_bind_point;
		vk::DescriptorSetLayoutBinding layout;
	};

	/** Repersents a blueprint for a texture input*/
	struct TextureInput
	{
		str name;
		class Texture *default_texture;
		vec<DescriptorInfo> descriptor_infos;
	};

	/** Repersents a blueprint for a buffer input */
	struct BufferInput
	{
		enum class UpdateFrequency
		{
			ePerFrame,
			eSingular,
		};

		str name;
		usize key;
		usize size;
		vk::BufferUsageFlags usage;
		UpdateFrequency update_frequency;
		vma::AllocationCreateInfo allocation_info;
		vec<DescriptorInfo> descriptor_infos;
	};

public:
	auto set_derived_object(RenderNode *const derived_object) -> RenderNodeBlueprint &
	{
		this->derived_object = derived_object;
		return *this;
	}

	auto set_name(str const name) -> RenderNodeBlueprint &
	{
		this->derived_object->name = name;
		return *this;
	}

	auto set_compute(bool const compute) -> RenderNodeBlueprint &
	{
		this->derived_object->compute = compute;
		return *this;
	}

	auto set_graphics(bool const graphics) -> RenderNodeBlueprint &
	{
		this->derived_object->graphics = graphics;
		return *this;
	}

	auto set_user_data(std::any data) -> RenderNodeBlueprint &
	{
		this->derived_object->user_data = data;
		return *this;
	}

	auto set_sample_count(vk::SampleCountFlagBits sample_count) -> RenderNodeBlueprint &
	{
		this->derived_object->sample_count = sample_count;
		return *this;
	}

	auto set_prepare_label(vk::DebugUtilsLabelEXT const label) -> RenderNodeBlueprint &
	{
		this->derived_object->prepare_label = label;
		return *this;
	}

	auto set_graphics_label(vk::DebugUtilsLabelEXT const label) -> RenderNodeBlueprint &
	{
		this->derived_object->graphics_label = label;
		return *this;
	}

	auto set_compute_label(vk::DebugUtilsLabelEXT const label) -> RenderNodeBlueprint &
	{
		this->derived_object->compute_label = label;
		return *this;
	}

	auto set_barrier_label(vk::DebugUtilsLabelEXT const label) -> RenderNodeBlueprint &
	{
		this->derived_object->barrier_label = label;
		return *this;
	}

	auto push_node(RenderNodeBlueprint blueprint) -> RenderNodeBlueprint &
	{
		this->children.push_back(blueprint);
		this->derived_object->children.push_back(blueprint.derived_object);

		return *this;
	}

	auto add_color_output(Attachment const attachment) -> RenderNodeBlueprint &
	{
		this->derived_object->color_attachment_formats.push_back(attachment.format);
		this->color_attachments.push_back(attachment);
		return *this;
	}

	auto set_depth_attachment(Attachment const attachment) -> RenderNodeBlueprint &
	{
		this->derived_object->depth_attachment_format = attachment.format;
		this->depth_attachment = attachment;
		return *this;
	}

	auto add_texture_input(TextureInput const input) -> RenderNodeBlueprint &
	{
		this->texture_inputs.push_back(input);
		return *this;
	}

	auto add_buffer_input(BufferInput const input) -> RenderNodeBlueprint &
	{
		this->buffer_inputs.push_back(input);
		return *this;
	}

private:
	auto has_color_attachment() const
	{
		return !color_attachments.empty();
	}

	auto has_depth_attachment() const
	{
		return !!depth_attachment;
	}

private:
	RenderNode *derived_object = {};

	vec<RenderNodeBlueprint> children {};

	vec<Attachment> color_attachments = {};
	Attachment depth_attachment = {};

	vec<TextureInput> texture_inputs = {};
	vec<BufferInput> buffer_inputs = {};
};


} // namespace BINDLESSVK_NAMESPACE
