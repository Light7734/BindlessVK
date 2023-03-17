#pragma once

#include "BindlessVk/Buffers/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"

namespace BINDLESSVK_NAMESPACE {

class Rendergraph;

class Renderpass
{
public:
	friend class RenderGraphBuilder;
	friend class RenderpassBlueprint;

public:
	Renderpass(VkContext *vk_context): vk_context(vk_context)
	{
	}

	virtual ~Renderpass() = default;

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

		auto is_color_attachment() const
		{
			return subresource_range.aspectMask & vk::ImageAspectFlagBits::eColor;
		}

		auto is_depth_attachment() const
		{
			return subresource_range.aspectMask & vk::ImageAspectFlagBits::eDepth;
		}
	};

	enum class SizeType : uint8_t
	{
		eSwapchainRelative,
		eRelative,
		eAbsolute,

		nCount,
	};

public:
	void virtual on_update(u32 frame_index, u32 image_index) = 0;
	void virtual on_render(vk::CommandBuffer cmd, u32 frame_index, u32 image_index) = 0;

	auto &get_update_label() const
	{
		return update_label;
	}

	auto &get_barrier_label() const
	{
		return barrier_label;
	}

	auto &get_render_label() const
	{
		return render_label;
	}

	auto &get_attachments() const
	{
		return attachments;
	}

	auto &get_descriptor_sets() const
	{
		return descriptor_sets;
	}

	auto &get_pipeline_layout() const
	{
		return pipeline_layout;
	}

	auto &get_buffer_inputs() const
	{
		return buffer_inputs;
	}

	auto is_multisampled() const
	{
		return sample_count != vk::SampleCountFlagBits::e1;
	}

protected:
	VkContext *vk_context;

	str name;

	std::any user_data;

	vec<Attachment> attachments;

	vec<Buffer> buffer_inputs;

	vec<AllocatedDescriptorSet> descriptor_sets;
	vk::DescriptorSetLayout descriptor_set_layout;
	vk::PipelineLayout pipeline_layout;

	vec<vk::Format> color_attachment_formats;
	vk::Format depth_attachment_format;

	vk::SampleCountFlagBits sample_count;

	vk::CommandBufferInheritanceRenderingInfo cmd_buffer_inheritance_rendering_info;
	vk::CommandBufferInheritanceInfo cmd_buffer_inheritance_info;
	vk::CommandBufferBeginInfo cmd_buffer_begin_info;

	vk::DebugUtilsLabelEXT update_label;
	vk::DebugUtilsLabelEXT barrier_label;
	vk::DebugUtilsLabelEXT render_label;
};

class RenderpassBlueprint
{
private:
	friend class RenderGraphBuilder;

public:
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

		// @warn this isn't the best way to check for validity...
		operator bool() const
		{
			return !!hash;
		}
	};

	struct TextureInput
	{
		str name;
		u32 binding;
		u32 count;
		vk::DescriptorType type;
		vk::ShaderStageFlagBits stage_mask;

		class Texture const *default_texture;
	};

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

	auto set_update_label(vk::DebugUtilsLabelEXT const label) -> RenderpassBlueprint &
	{
		this->update_label = label;
		return *this;
	}

	auto set_render_label(vk::DebugUtilsLabelEXT const label) -> RenderpassBlueprint &
	{
		this->render_label = label;
		return *this;
	}

	auto set_barrier_label(vk::DebugUtilsLabelEXT const label) -> RenderpassBlueprint &
	{
		this->barrier_label = label;
		return *this;
	}

private:
	str name;

	std::any user_data;

	vec<Attachment> color_attachments = {};
	Attachment depth_attachment = {};
	vec<TextureInput> texture_inputs = {};
	vec<BufferInput> buffer_inputs = {};

	vk::SampleCountFlagBits sample_count = {};

	vk::DebugUtilsLabelEXT update_label = {};
	vk::DebugUtilsLabelEXT render_label = {};
	vk::DebugUtilsLabelEXT barrier_label = {};
};


} // namespace BINDLESSVK_NAMESPACE
