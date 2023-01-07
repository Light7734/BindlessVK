#pragma once

#include "BindlessVk/Buffer.hpp"
#include "BindlessVk/Common.hpp"
#include "BindlessVk/Types.hpp"

#include <glm/glm.hpp>

namespace BINDLESSVK_NAMESPACE {

struct Renderpass
{
	struct CreateInfo
	{
	public:
		enum class SizeType : uint8_t
		{
			eSwapchainRelative,
			eRelative,
			eAbsolute,

			nCount,
		};

		struct AttachmentInfo
		{
			std::string name;
			glm::vec2 size;
			SizeType size_type;
			vk::Format format;
			vk::SampleCountFlagBits samples;

			std::string input;
			std::string size_relative_name;

			vk::ClearValue clear_value;
		};

		struct TextureInputInfo
		{
			std::string name;
			uint32_t binding;
			uint32_t count;
			vk::DescriptorType type;
			vk::ShaderStageFlagBits stage_mask;

			class Texture* default_texture;
		};

		struct BufferInputInfo
		{
			std::string name;
			uint32_t binding;
			uint32_t count;
			vk::DescriptorType type;
			vk::ShaderStageFlagBits stage_mask;

			size_t size;
			void* initial_data;
		};

		std::string name;

		void (*on_begin_frame)(Device*, class RenderGraph*, Renderpass*, uint32_t, void*);
		void (*on_update)(Device*, class RenderGraph*, Renderpass*, uint32_t, void*);
		void (*on_render
		)(Device*,
		  class RenderGraph*,
		  Renderpass*,
		  vk::CommandBuffer cmd,
		  uint32_t,
		  uint32_t,
		  void*);

		vec<CreateInfo::AttachmentInfo> color_attachments_info;
		CreateInfo::AttachmentInfo depth_stencil_attachment_info;

		vec<CreateInfo::TextureInputInfo> texture_inputs_info;
		vec<CreateInfo::BufferInputInfo> buffer_inputs_info;

		void* user_pointer;

		vk::DebugUtilsLabelEXT update_debug_label;
		vk::DebugUtilsLabelEXT barrier_debug_label;
		vk::DebugUtilsLabelEXT render_debug_label;
	};

	struct Attachment
	{
		vk::PipelineStageFlags stage_mask;
		vk::AccessFlags access_mask;
		vk::ImageLayout layout;
		vk::ImageSubresourceRange subresource_range;

		vk::AttachmentLoadOp load_op;
		vk::AttachmentStoreOp store_op;

		uint32_t resource_index;

		vk::ClearValue clear_value;
	};

	std::string name;

	vec<Attachment> attachments;

	std::unordered_map<uint64_t, class Buffer*> buffer_inputs;

	vec<vk::DescriptorSet> descriptor_sets;
	vk::DescriptorSetLayout descriptor_set_layout;
	vk::PipelineLayout pipeline_layout;

	void (*on_begin_frame)(Device*, class RenderGraph*, Renderpass*, uint32_t, void*);
	void (*on_update)(Device*, class RenderGraph*, Renderpass*, uint32_t, void*);

	void (*on_render
	)(Device*, class RenderGraph*, Renderpass*, vk::CommandBuffer cmd, uint32_t, uint32_t, void*);


	vec<vk::Format> color_attachments_format;
	vk::Format depth_attachment_format;

	vk::CommandBufferInheritanceRenderingInfo cmd_buffer_inheritance_rendering_info;
	vk::CommandBufferInheritanceInfo cmd_buffer_inheritance_info;
	vk::CommandBufferBeginInfo cmd_buffer_begin_info;

	void* user_pointer;

	vk::DebugUtilsLabelEXT update_debug_label;
	vk::DebugUtilsLabelEXT barrier_debug_label;
	vk::DebugUtilsLabelEXT render_debug_label;
};

} // namespace BINDLESSVK_NAMESPACE
