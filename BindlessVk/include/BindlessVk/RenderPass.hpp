#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common/Common.hpp"

#include <glm/glm.hpp>
#include <type_traits>

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
		vk::ImageLayout layout;
		vk::ImageSubresourceRange subresource_range;

		vk::AttachmentLoadOp load_op;
		vk::AttachmentStoreOp store_op;

		u32 resource_index;

		vk::ClearValue clear_value;
	};

	enum class SizeType : uint8_t
	{
		eSwapchainRelative,
		eRelative,
		eAbsolute,

		nCount,
	};

	str name;

	vec<Attachment> attachments;

	hash_map<u64, class Buffer *> buffer_inputs;

	vec<AllocatedDescriptorSet> descriptor_sets;
	vk::DescriptorSetLayout descriptor_set_layout;
	vk::PipelineLayout pipeline_layout;

	UpdateFn on_updtae;
	RenderFn on_render;

	vec<vk::Format> color_attachments_format;
	vk::Format depth_attachment_format;

	vk::CommandBufferInheritanceRenderingInfo cmd_buffer_inheritance_rendering_info;
	vk::CommandBufferInheritanceInfo cmd_buffer_inheritance_info;
	vk::CommandBufferBeginInfo cmd_buffer_begin_info;

	void *user_pointer;

	vk::DebugUtilsLabelEXT update_debug_label;
	vk::DebugUtilsLabelEXT barrier_debug_label;
	vk::DebugUtilsLabelEXT render_debug_label;
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
		vk::SampleCountFlagBits samples;

		str input;
		str size_relative_name;

		vk::ClearValue clear_value;
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
	auto set_name(c_str const name) -> RenderpassBlueprint &
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

	auto add_texture_input(TextureInput const input)
	{
		this->texture_inputs.push_back(input);
		return *this;
	}

	auto add_buffer_input(BufferInput const input)
	{
		this->buffer_inputs.push_back(input);
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
	c_str name;

	Renderpass::UpdateFn update_fn;
	Renderpass::RenderFn render_fn;

	vec<Attachment> color_attachments;
	Attachment depth_attachment;
	vec<TextureInput> texture_inputs;
	vec<BufferInput> buffer_inputs;

	vk::DebugUtilsLabelEXT update_label;
	vk::DebugUtilsLabelEXT render_label;
	vk::DebugUtilsLabelEXT barrier_label;
};


} // namespace BINDLESSVK_NAMESPACE
