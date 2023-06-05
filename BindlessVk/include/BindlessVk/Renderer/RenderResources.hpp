#pragma once

#include "BindlessVk/Allocators/MemoryAllocator.hpp"
#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Swapchain.hpp"
#include "BindlessVk/Context/VkContext.hpp"
#include "BindlessVk/Renderer/RenderNode.hpp"
#include "BindlessVk/Texture/Image.hpp"

namespace BINDLESSVK_NAMESPACE {

/** Manages render garph attachments */
class RenderResources
{
public:
	struct Attachment
	{
		vk::AccessFlags access_mask;
		vk::ImageLayout image_layout;
		vk::PipelineStageFlags stage_mask;

		Image image;
		vk::ImageView image_view;
	};

	struct TransientAttachment
	{
		Image image;
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
		};

		Type type;

		vk::Format image_format;

		VkExtent3D extent;
		pair<f32, f32> size;
		RenderNode::SizeType size_type;
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
	/** Default constructor */
	RenderResources() = default;

	/** Argumented constructor
	 *
	 * @param vk_context Pointer to the vk context
	 * @param memory_allocator Pointer to the memory allocator
	 * @param swapchain Pointer to the swapchain
	 * */
	RenderResources(
	    VkContext const *vk_context,
	    MemoryAllocator *memory_allocator,
	    Swapchain const *swapchain
	);

	/** Default move constructor */
	RenderResources(RenderResources &&other) = default;

	/** Default move assignment operator */
	RenderResources &operator=(RenderResources &&other) = default;

	/** Deleted copy constructor */
	RenderResources(RenderResources const &) = delete;

	/** Deleted copy assignment operator */
	RenderResources &operator=(RenderResources const &) = delete;

	/** Destructor */
	~RenderResources();

	/** Address accessor for containers[resource_index] */
	auto get_attachment_container(u32 resource_index)
	{
		return &containers[resource_index];
	}

	/** Returns the backbuffer attachment at @a image_index
	 *
	 * @param image_index Index of the swapchain image to retrive
	 */
	auto get_backbuffer_attachment(u32 image_index)
	{
		return containers[0].get_attachment(image_index, 0);
	}

	/** Returns the attachment at @a image_index OR frame_index of container at @a resource_index.
	 * Depending on wether the attachment's type is ePerImage or ePerFrame.
	 * Returns the first(and only) attachment if attachment's type is eSingle
	 *
	 * @param resource_index Attachment container's index
	 * @param image_index Index of the attachment with type ePerImage
	 * @param frame_index Index of the attachment with type ePerFrame
	 */
	auto get_attachment(u32 resource_index, u32 image_index, u32 frame_index)
	{
		return containers[resource_index].get_attachment(image_index, frame_index);
	}

	auto get_transient_attachment(u32 resource_index) const
	{
		return &transient_attachments[resource_index];
	}

	auto try_get_suitable_transient_attachment_index(
	    RenderNodeBlueprint::Attachment const &blueprint_attachment,
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

			++i;
		}

		return std::numeric_limits<u32>::max();
	}

	auto try_get_attachment_index(u64 const key) -> u32;

	void add_key_to_attachment_index(u64 const key, u32 const attachment_index);

	void create_color_attachment(
	    RenderNodeBlueprint::Attachment const &blueprint_attachment,
	    vk::SampleCountFlagBits sample_count
	);

	void create_depth_attachment(
	    RenderNodeBlueprint::Attachment const &blueprint_attachment,
	    vk::SampleCountFlagBits sample_count
	);

	void create_transient_attachment(
	    RenderNodeBlueprint::Attachment const &blueprint_attachment,
	    vk::SampleCountFlagBits sample_count
	);

private:
	auto calculate_attachment_image_extent(
	    RenderNodeBlueprint::Attachment const &blueprint_attachment
	) const -> vk::Extent3D;

private:
	tidy_ptr<Device> device = {};
	Surface *surface = {};
	MemoryAllocator *memory_allocator = {};

	vec<AttachmentContainer> containers = {};
	hash_map<u64, u32> attachment_indices = {};
	vec<TransientAttachment> transient_attachments = {};
};

} // namespace BINDLESSVK_NAMESPACE
