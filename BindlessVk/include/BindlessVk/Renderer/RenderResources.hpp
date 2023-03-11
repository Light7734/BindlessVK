#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Renderer/Renderpass.hpp"

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

	struct TransientAttachment
	{
		AllocatedImage image;
		vk::ImageView image_view;
		vk::SampleCountFlagBits sample_count;
		vk::Format format;
		vk::Extent3D extent;
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

		vec<Attachment> attachments;

		auto get_attachment(u32 image_index, u32 frame_index) -> Attachment *
		{
			return type == Type::ePerImage ? &attachments[image_index] :
			       type == Type::ePerFrame ? &attachments[frame_index] :
			                                 &attachments[0];
		};
	};

public:
	RenderResources(ref<VkContext> vk_context);

	auto get_attachment_container(u32 resource_index)
	{
		return &containers[resource_index];
	}

	auto get_backbuffer_attachment(u32 image_index)
	{
		return containers[0].get_attachment(image_index, 0);
	}

	auto get_attachment(u32 resource_index, u32 image_index, u32 frame_index)
	{
		return containers[resource_index].get_attachment(image_index, frame_index);
	}

	auto get_transient_attachment(u32 resource_index) const
	{
		return transient_attachments[resource_index];
	}

	auto try_get_suitable_transient_attachment_index(
	    RenderpassBlueprint::Attachment const &blueprint_attachment,
	    vk::SampleCountFlagBits sample_count
	) const
	{
		for (u32 i = 0; auto const &transient_attachment : transient_attachments)
		{
			if (transient_attachment.sample_count == sample_count &&
			    transient_attachment.format == blueprint_attachment.format &&
			    transient_attachment.extent ==
			        calculate_attachment_image_extent(blueprint_attachment))
				return i;

			i++;
		}

		return std::numeric_limits<u32>::max();
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

	void create_transient_attachment(
	    RenderpassBlueprint::Attachment const &blueprint_attachment,
	    vk::SampleCountFlagBits sample_count
	);

private:
	auto calculate_attachment_image_extent(
	    RenderpassBlueprint::Attachment const &blueprint_attachment
	) const -> vk::Extent3D;

private:
	ref<VkContext> const vk_context;
	vec<AttachmentContainer> containers;
	hash_map<u64, u32> attachment_indices;
	vec<TransientAttachment> transient_attachments;
};

} // namespace BINDLESSVK_NAMESPACE
