#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/RenderPass.hpp"
#include "BindlessVk/VkContext.hpp"

namespace BINDLESSVK_NAMESPACE {

class RenderResources
{
private:
	struct Attachment
	{
		// State carried from last barrier
		vk::AccessFlags access_mask;
		vk::ImageLayout image_layout;
		vk::PipelineStageFlags stage_mask;

		AllocatedImage image;
		vk::ImageView image_view;
	};

	struct TransientMsAttachment
	{
		AllocatedImage image;
		vk::ImageView image_view;
	};

	struct AttachmentContainer
	{
		enum class Type
		{
			ePerImage,
			ePerFrame,
			eSingle,
		} type;

		vk::Format image_format;

		VkExtent3D extent;
		pair<f32, f32> size;
		Renderpass::SizeType size_type;
		str relative_size_name;

		// Transient multi-sampled images
		TransientMsAttachment ms_attachment;
		vk::ResolveModeFlagBits ms_resolve_mode;

		// Required for aliasing Read-Modify-Write attachments
		vec<Attachment> attachments;

		inline auto get_attachment(u32 image_index, u32 frame_index) -> Attachment *
		{
			return type == Type::ePerImage ? &attachments[image_index] :
			       type == Type::ePerFrame ? &attachments[frame_index] :
			                                 &attachments[0];
		};
	};

public:
	RenderResources(ref<VkContext> vk_context);

	inline auto get_attachment_container(u32 resource_index) -> AttachmentContainer *
	{
		return &containers[resource_index];
	}

	inline auto get_backbuffer_attachment(u32 image_index) -> Attachment *
	{
		return containers[0].get_attachment(image_index, 0);
	}

	inline auto get_attachment(u32 resource_index, u32 image_index, u32 frame_index) -> Attachment *
	{
		return containers[resource_index].get_attachment(image_index, frame_index);
	}

	auto try_get_attachment_index(u64 const key) -> u32;
	void add_key_to_attachment_index(u64 const key, u32 const attachment_index);

	void create_color_attachment(
	    RenderpassBlueprint::Attachment const &blueprint_attachment,
	    vk::SampleCountFlagBits sample_count
	);
	void create_depth_attachment(
	    RenderpassBlueprint::Attachment const &blueprint_attachment,
	    vk::SampleCountFlagBits sample_count
	);

private:
	auto calculate_attachment_image_extent(
	    RenderpassBlueprint::Attachment const &blueprint_attachment
	) -> vk::Extent3D;

private:
	ref<VkContext> const vk_context;
	vec<AttachmentContainer> containers;
	hash_map<u64, u32> attachment_indices;
	vec<u64> backbuffer_keys;
};

} // namespace BINDLESSVK_NAMESPACE
