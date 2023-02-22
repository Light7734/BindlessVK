#include "BindlessVk/RenderResources.hpp"

namespace BINDLESSVK_NAMESPACE {

RenderResources::RenderResources(ref<VkContext> const vk_context): vk_context(vk_context)
{
	auto *const swapchain = vk_context->get_swapchain();
	auto const &surface = vk_context->get_surface();
	auto const extent = surface.get_framebuffer_extent();

	AttachmentContainer container {};

	container.image_format = surface.get_color_format();
	container.extent = { extent.width, extent.height, 0 };
	container.size_type = Renderpass::SizeType::eSwapchainRelative;
	container.size = { 1.0, 1.0 };
	container.relative_size_name = "";
	container.type = AttachmentContainer::Type::ePerImage;

	auto const images = swapchain->get_images();
	auto const image_views = swapchain->get_image_views();
	for (u32 i = 0; i < images.size(); ++i)
	{
		container.attachments.push_back(Attachment {
		    vk::AccessFlagBits::eNone,
		    vk::ImageLayout::eUndefined,
		    vk::PipelineStageFlagBits::eTopOfPipe,
		    AllocatedImage { images[i] },
		    image_views[i],
		});
	}

	containers.push_back(container);
}

auto RenderResources::try_get_attachment_index(u64 const key) -> u32
{
	if (attachment_indices.contains(key))
		return attachment_indices[key];

	return std::numeric_limits<u32>::max();
}

void RenderResources::add_key_to_attachment_index(u64 const key, u32 const attachment_index)
{
	attachment_indices[key] = attachment_index;
}

void RenderResources::create_color_attachment(
    RenderpassBlueprint::Attachment const &blueprint_attachment,
    vk::SampleCountFlagBits sample_count
)
{
	auto const device = vk_context->get_device();
	auto const allocator = vk_context->get_allocator();
	auto const framebuffer_extent = vk_context->get_surface().get_framebuffer_extent();

	auto const extent = calculate_attachment_image_extent(blueprint_attachment);
	auto attachment = Attachment {};

	auto const image = AllocatedImage {
		allocator.createImage(
		    vk::ImageCreateInfo {
		        {},
		        vk::ImageType::e2D,
		        blueprint_attachment.format,
		        extent,
		        1u,
		        1u,
		        vk::SampleCountFlagBits::e1,
		        vk::ImageTiling::eOptimal,
		        vk::ImageUsageFlagBits::eColorAttachment,
		        vk::SharingMode::eExclusive,
		        0u,
		        nullptr,
		        vk::ImageLayout::eUndefined,
		    },
		    vma::AllocationCreateInfo {
		        {},
		        vma::MemoryUsage::eGpuOnly,
		        vk::MemoryPropertyFlagBits::eDeviceLocal,
		    }
		),
	};
	vk_context->set_object_name(
	    image,
	    fmt::format("{}_image (single)", blueprint_attachment.debug_name).c_str()
	);

	auto const image_view = device.createImageView(vk::ImageViewCreateInfo {
	    {},
	    image,
	    vk::ImageViewType::e2D,
	    blueprint_attachment.format,
	    vk::ComponentMapping {
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	    },
	    { vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u },
	});

	vk_context->set_object_name(
	    image_view,
	    fmt::format("{}_image_view (single)", blueprint_attachment.debug_name).c_str()
	);

	attachment_indices[blueprint_attachment.hash] = containers.size();

	containers.push_back(AttachmentContainer {
	    AttachmentContainer::Type::eSingle,
	    blueprint_attachment.format,
	    vk::Extent3D {
	        framebuffer_extent.width,
	        framebuffer_extent.height,
	        0,
	    },
	    pair<u32, u32> {
	        1.0,
	        1.0,
	    },
	    Renderpass::SizeType::eSwapchainRelative,
	    "",
	    vec<Attachment> {
	        Attachment {
	            {},
	            vk::ImageLayout::eUndefined,
	            vk::PipelineStageFlagBits::eTopOfPipe,
	            image,
	            image_view,
	        },
	    },
	});
}

void RenderResources::create_depth_attachment(
    RenderpassBlueprint::Attachment const &blueprint_attachment,
    vk::SampleCountFlagBits sample_count
)
{
	auto const device = vk_context->get_device();
	auto const allocator = vk_context->get_allocator();
	auto const framebuffer_extent = vk_context->get_surface().get_framebuffer_extent();
	auto const extent = calculate_attachment_image_extent(blueprint_attachment);

	auto const image = AllocatedImage { allocator.createImage(
		vk::ImageCreateInfo {
		    {},
		    vk::ImageType::e2D,
		    blueprint_attachment.format,
		    extent,
		    1u,
		    1u,
		    sample_count,
		    vk::ImageTiling::eOptimal,
		    vk::ImageUsageFlagBits::eDepthStencilAttachment,
		    vk::SharingMode::eExclusive,
		    0u,
		    nullptr,
		    vk::ImageLayout::eUndefined,
		},
		vma::AllocationCreateInfo {
		    {},
		    vma::MemoryUsage::eGpuOnly,
		    vk::MemoryPropertyFlagBits::eDeviceLocal,
		}
	) };

	vk_context->set_object_name(
	    image,
	    fmt::format("{}_depth_image", blueprint_attachment.debug_name).c_str()
	);

	auto const image_view = device.createImageView(vk::ImageViewCreateInfo {
	    {},
	    image,
	    vk::ImageViewType::e2D,
	    blueprint_attachment.format,
	    vk::ComponentMapping {
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	    },
	    vk::ImageSubresourceRange {
	        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
	        0u,
	        1u,
	        0u,
	        1u,
	    },
	});

	vk_context->set_object_name(
	    image_view,
	    fmt::format("{}_image_view (single)", blueprint_attachment.debug_name).c_str()
	);

	attachment_indices[blueprint_attachment.hash] = containers.size();

	containers.push_back(AttachmentContainer {
	    AttachmentContainer::Type::eSingle,
	    blueprint_attachment.format,
	    vk::Extent3D { framebuffer_extent.width, framebuffer_extent.height, 0 },
	    pair<u32, u32> {
	        1.0,
	        1.0,
	    },
	    Renderpass::SizeType::eSwapchainRelative,
	    "",
	    vec<Attachment> {
	        Attachment {
	            {},
	            vk::ImageLayout::eUndefined,
	            vk::PipelineStageFlagBits::eTopOfPipe,
	            image,
	            image_view,
	        },
	    },
	});
}

void RenderResources::create_transient_attachment(
    RenderpassBlueprint::Attachment const &blueprint_attachment,
    vk::SampleCountFlagBits sample_count
)
{
	auto const device = vk_context->get_device();
	auto const allocator = vk_context->get_allocator();
	auto const framebuffer_extent = vk_context->get_surface().get_framebuffer_extent();
	auto const extent = calculate_attachment_image_extent(blueprint_attachment);

	auto const image = AllocatedImage(allocator.createImage(
	    vk::ImageCreateInfo {
	        {},
	        vk::ImageType::e2D,
	        blueprint_attachment.format,
	        extent,
	        1u,
	        1u,
	        sample_count,
	        vk::ImageTiling::eOptimal,
	        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
	        vk::SharingMode::eExclusive,
	        0u,
	        nullptr,
	        vk::ImageLayout::eUndefined,
	    },
	    vma::AllocationCreateInfo {
	        {},
	        vma::MemoryUsage::eGpuOnly,
	        vk::MemoryPropertyFlagBits::eDeviceLocal,
	    }
	));

	vk_context->set_object_name(
	    image,
	    fmt::format("{}_transient_image", blueprint_attachment.debug_name).c_str()
	);

	auto const image_view = device.createImageView(vk::ImageViewCreateInfo {
	    {},
	    image,
	    vk::ImageViewType::e2D,
	    blueprint_attachment.format,

	    vk::ComponentMapping {
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	        vk::ComponentSwizzle::eIdentity,
	    },

	    vk::ImageSubresourceRange {
	        vk::ImageAspectFlagBits::eColor,
	        0u,
	        1u,
	        0u,
	        1u,
	    },
	});

	vk_context->set_object_name(
	    image_view,
	    fmt::format("{}_transient_image_view", blueprint_attachment.debug_name).c_str()
	);

	transient_attachments.emplace_back(TransientAttachment {
	    image,
	    image_view,
	    sample_count,
	    blueprint_attachment.format,
	    extent,
	});
}

} // namespace BINDLESSVK_NAMESPACE
