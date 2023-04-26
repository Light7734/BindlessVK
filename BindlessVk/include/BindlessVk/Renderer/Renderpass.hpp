#pragma once

#include "BindlessVk/Allocators/Descriptors/DescriptorAllocator.hpp"
#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Shader/DescriptorSet.hpp"

namespace BINDLESSVK_NAMESPACE {

class Rendergraph;

/** Describes a single compute and/or rendering step in the render graph */
class Renderpass
{
public:
	friend class RenderGraphBuilder;
	friend class RenderpassBlueprint;

public:
	/** Repersents a render color or depth/stencil attachment */
	struct Attachment
	{
		vk::PipelineStageFlags stage_mask;
		vk::AccessFlags access_mask;
		vk::ImageLayout image_layout;
		vk::ImageSubresourceRange subresource_range;

		vk::AttachmentLoadOp load_op;
		vk::AttachmentStoreOp store_op;

		vk::ResolveModeFlagBits transient_resolve_moode;

		u32 resource_index;
		u32 transient_resource_index;

		vk::ClearValue clear_value;

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
	/** Argumented constructor
	 *
	 * @param vk_context The vulkan context
	 */
	Renderpass(VkContext const *vk_context): vk_context(vk_context)
	{
	}

	/** Destuctor */
	virtual ~Renderpass() = default;

public:
	/** Sets up the pass, called only once */
	void virtual on_setup() = 0;

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

	/** Trivial reference-accessor for update_label */
	auto &get_prepare_label() const
	{
		return prepare_label;
	}

	/** Trivial reference-accessor for compute_label */
	auto &get_compute_label() const
	{
		return compute_label;
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

	/** Trivial reference-accessor for attachments */
	auto &get_attachments() const
	{
		return attachments;
	}

	/** Trivial reference-accessor for descriptor_sets */
	auto &get_descriptor_sets() const
	{
		return descriptor_sets;
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

	/** Trivial reference-accessor for pipeline_layout */
	auto &get_pipeline_layout() const
	{
		return pipeline_layout;
	}

	/** Trivial reference-accessor for buffer_inputs */
	auto &get_buffer_inputs() const
	{
		return buffer_inputs;
	}

	/** Checks if sample_count has more than 1 samples */
	auto is_multisampled() const
	{
		return sample_count != vk::SampleCountFlagBits::e1;
	}

protected:
	VkContext const *vk_context = {};

	str name = {};

	bool compute = {};
	bool graphics = {};

	std::any user_data = {};

	vec<Attachment> attachments = {};

	vec<Buffer> buffer_inputs = {};

	vec<DescriptorSet> compute_descriptor_sets = {};
	vk::DescriptorSetLayout compute_descriptor_set_layout = {};
	vk::PipelineLayout compute_pipeline_layout = {};

	vec<DescriptorSet> descriptor_sets = {};
	vk::DescriptorSetLayout descriptor_set_layout = {};
	vk::PipelineLayout pipeline_layout = {};

	vec<vk::Format> color_attachment_formats = {};
	vk::Format depth_attachment_format = {};

	vk::SampleCountFlagBits sample_count = {};

	vk::CommandBufferInheritanceRenderingInfo cmd_buffer_inheritance_rendering_info = {};
	vk::CommandBufferInheritanceInfo cmd_buffer_inheritance_info = {};
	vk::CommandBufferBeginInfo cmd_buffer_begin_info = {};

	vk::DebugUtilsLabelEXT prepare_label = {};
	vk::DebugUtilsLabelEXT compute_label = {};
	vk::DebugUtilsLabelEXT graphics_label = {};
	vk::DebugUtilsLabelEXT barrier_label = {};
};

/** Represents build instructions for a Renderpass, to be used by RenderGraphBuilder */
class RenderpassBlueprint
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
		Renderpass::SizeType size_type;
		vk::Format format;

		vk::ClearValue clear_value;

		str debug_name = "";

		// @warning this isn't the best way to check for validity...
		operator bool() const
		{
			return !!hash;
		}
	};

	/** Repersents a blueprint for a texture input*/
	struct TextureInput
	{
		str name;
		u32 binding;
		u32 count;
		vk::DescriptorType type;
		vk::ShaderStageFlagBits stage_mask;

		class Texture const *default_texture;
	};

	/** Repersents a blueprint for a buffer input */
	struct BufferInput
	{
		str name;
		u32 binding;
		u32 count;
		vk::DescriptorType type;
		vk::ShaderStageFlags stage_mask;

		size_t size;
		void *initial_data;
	};

public:
	auto set_name(str const name) -> RenderpassBlueprint &
	{
		this->name = name;
		return *this;
	}

	auto set_compute(bool const compute) -> RenderpassBlueprint &
	{
		this->compute = compute;
		return *this;
	}

	auto set_graphics(bool const graphics) -> RenderpassBlueprint &
	{
		this->graphics = graphics;
		return *this;
	}

	auto add_color_output(Attachment const attachment) -> RenderpassBlueprint &
	{
		this->color_attachments.push_back(attachment);
		return *this;
	}

	auto set_depth_attachment(Attachment const attachment) -> RenderpassBlueprint &
	{
		this->depth_attachment = attachment;
		return *this;
	}

	auto add_texture_input(TextureInput const input) -> RenderpassBlueprint &
	{
		this->texture_inputs.push_back(input);
		return *this;
	}

	auto add_buffer_input(BufferInput const input) -> RenderpassBlueprint &
	{
		this->buffer_inputs.push_back(input);
		return *this;
	}

	auto set_user_data(std::any data) -> RenderpassBlueprint &
	{
		this->user_data = data;
		return *this;
	}

	auto set_sample_count(vk::SampleCountFlagBits sample_count) -> RenderpassBlueprint &
	{
		this->sample_count = sample_count;
		return *this;
	}

	auto set_prepare_label(vk::DebugUtilsLabelEXT const label) -> RenderpassBlueprint &
	{
		this->prepare_label = label;
		return *this;
	}

	auto set_graphics_label(vk::DebugUtilsLabelEXT const label) -> RenderpassBlueprint &
	{
		this->compute_label = label;
		return *this;
	}

	auto set_compute_label(vk::DebugUtilsLabelEXT const label) -> RenderpassBlueprint &
	{
		this->graphics_label = label;
		return *this;
	}

	auto set_barrier_label(vk::DebugUtilsLabelEXT const label) -> RenderpassBlueprint &
	{
		this->barrier_label = label;
		return *this;
	}

private:
	str name = {};

	bool compute = {};
	bool graphics = {};

	std::any user_data = {};

	vec<Attachment> color_attachments = {};
	Attachment depth_attachment = {};
	vec<TextureInput> texture_inputs = {};
	vec<BufferInput> buffer_inputs = {};

	vk::SampleCountFlagBits sample_count = {};

	vk::DebugUtilsLabelEXT prepare_label = {};
	vk::DebugUtilsLabelEXT compute_label = {};
	vk::DebugUtilsLabelEXT graphics_label = {};
	vk::DebugUtilsLabelEXT barrier_label = {};
};


} // namespace BINDLESSVK_NAMESPACE
