#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"

#include <glm/glm.hpp>

namespace BINDLESSVK_NAMESPACE {

class RenderGraph;

struct Renderpass
{
	using UpdateFn = void (*)(VkContext *, RenderGraph *, Renderpass *, u32, void *);
	using RenderFn =
	    void (*)(VkContext *, RenderGraph *, Renderpass *, vk::CommandBuffer, u32, u32, void *);

	struct Attachment
	{
		vk::PipelineStageFlags stage_mask;
		vk::AccessFlags access_mask;
		vk::ImageLayout image_layout;
		vk::ImageSubresourceRange subresource_range;

		vk::AttachmentLoadOp load_op;
		vk::AttachmentStoreOp store_op;

		u32 resource_index;

		vk::ClearValue clear_value;

		inline auto is_color_attachment() const
		{
			return subresource_range.aspectMask & vk::ImageAspectFlagBits::eColor;
		}

		inline auto is_depth_attachment() const
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

	inline auto is_multisampled() const
	{
		return sample_count != vk::SampleCountFlagBits::e1;
	}

	str name;

	UpdateFn on_update;
	RenderFn on_render;

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

// Blueprint to be used by rendergraph builder
class RenderpassBlueprint
{
private:
	friend class RenderGraphBuilder;

public:
	struct Attachment
	{
		str name;
		pair<f32, f32> size;
		Renderpass::SizeType size_type;
		vk::Format format;

		str input;
		str size_relative_name;

		vk::ClearValue clear_value;

		inline operator bool() const
		{
			// @todo: return validity in a better way?
			return !name.empty();
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
		vk::ShaderStageFlagBits stage_mask;

		size_t size;
		void *initial_data;
	};

public:
	auto set_name(str const name) -> RenderpassBlueprint &
	{
		this->name = name;
		return *this;
	}

	auto set_update_fn(Renderpass::UpdateFn const fn) -> RenderpassBlueprint &
	{
		this->update_fn = fn;
		return *this;
	}

	auto set_render_fn(Renderpass::RenderFn const fn) -> RenderpassBlueprint &
	{
		this->render_fn = fn;
		return *this;
	}

	auto add_color_attachment(Attachment const attachment) -> RenderpassBlueprint &
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

	auto set_sample_count(vk::SampleCountFlagBits sample_count) -> RenderpassBlueprint&
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

	Renderpass::UpdateFn update_fn = {};
	Renderpass::RenderFn render_fn = {};

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
