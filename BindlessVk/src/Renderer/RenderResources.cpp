#include "BindlessVk/Renderer/RenderResources.hpp"

namespace BINDLESSVK_NAMESPACE {

RenderResources::RenderResources(
    VkContext const *const vk_context,
    MemoryAllocator *const memory_allocator,
    Swapchain const *const swapchain
)
    : surface(vk_context->get_surface())
    , debug_utils(vk_context->get_debug_utils())
    , device(vk_context->get_device())
    , memory_allocator(memory_allocator)
{
	auto const extent = surface->get_framebuffer_extent();

	auto container = AttachmentContainer {
		AttachmentContainer::Type::ePerImage, //
		surface->get_color_format(),
		VkExtent3D { extent.width, extent.height, 0 },
		pair<f32, f32> { 1.0, 1.0 },
		Renderpass::SizeType::eSwapchainRelative,
		"",
	};

	auto const images = swapchain->get_images();
	auto const image_views = swapchain->get_image_views();
	for (u32 i = 0; i < images.size(); ++i)
	{
		container.attachments.push_back(Attachment {
		    vk::AccessFlagBits::eNone,
		    vk::ImageLayout::eUndefined,
		    vk::PipelineStageFlagBits::eTopOfPipe,
		    Image { images[i] },
		    image_views[i],
		});
	}

	containers.push_back(std::move(container));
}

RenderResources::RenderResources(RenderResources &&other)
{
	*this = std::move(other);
}

RenderResources &RenderResources::operator=(RenderResources &&other)
{
	this->device = other.device;
	this->surface = other.surface;
	this->debug_utils = other.debug_utils;
	this->memory_allocator = other.memory_allocator;

	this->containers = std::move(other.containers);
	this->attachment_indices = std::move(other.attachment_indices);
	this->transient_attachments = std::move(other.transient_attachments);

	other.device = {};

	return *this;
}

RenderResources::~RenderResources()
{
	if (!device)
		return;

	for (auto i = 1; i < containers.size(); ++i)
		for (auto &attachment : containers[i].attachments)
			device->vk().destroyImageView(attachment.image_view);

	for (auto &attachment : transient_attachments)
		device->vk().destroyImageView(attachment.image_view);
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
	auto const framebuffer_extent = surface->get_framebuffer_extent();

	auto const extent = calculate_attachment_image_extent(blueprint_attachment);
	auto attachment = Attachment {};

	auto image = Image {
		memory_allocator,

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
		},
	};

	debug_utils->set_object_name(
	    device->vk(),
	    image.vk(),
	    fmt::format("{}_image (single)", blueprint_attachment.debug_name)
	);

	auto const image_view = device->vk().createImageView(vk::ImageViewCreateInfo {
	    {},
	    image.vk(),
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

	debug_utils->set_object_name(
	    device->vk(),
	    image_view,
	    fmt::format("{}_image_view (single)", blueprint_attachment.debug_name)
	);

	attachment_indices[blueprint_attachment.hash] = containers.size();

	containers.emplace_back(AttachmentContainer {
	    AttachmentContainer::Type::eSingle,
	    blueprint_attachment.format,
	    vk::Extent3D {
	        framebuffer_extent.width,
	        framebuffer_extent.height,
	        0,
	    },
	    pair<f32, f32> {
	        1.0,
	        1.0,
	    },
	    Renderpass::SizeType::eSwapchainRelative,
	    "",
	    {},
	});

	containers.back().attachments.emplace_back(Attachment {
	    {},
	    vk::ImageLayout::eUndefined,
	    vk::PipelineStageFlagBits::eTopOfPipe,
	    std::move(image),
	    image_view,
	});
}

void RenderResources::create_depth_attachment(
    RenderpassBlueprint::Attachment const &blueprint_attachment,
    vk::SampleCountFlagBits sample_count
)
{
	auto const framebuffer_extent = surface->get_framebuffer_extent();
	auto const extent = calculate_attachment_image_extent(blueprint_attachment);

	auto image = Image {
		memory_allocator,

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
		},
	};

	debug_utils->set_object_name(
	    device->vk(),
	    image.vk(),
	    fmt::format("{}_depth_image", blueprint_attachment.debug_name)
	);

	auto const image_view = device->vk().createImageView(vk::ImageViewCreateInfo {
	    {},
	    image.vk(),
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

	debug_utils->set_object_name(
	    device->vk(),
	    image_view,
	    fmt::format("{}_image_view (single)", blueprint_attachment.debug_name)
	);

	attachment_indices[blueprint_attachment.hash] = containers.size();

	containers.push_back(AttachmentContainer {
	    AttachmentContainer::Type::eSingle,
	    blueprint_attachment.format,
	    vk::Extent3D { framebuffer_extent.width, framebuffer_extent.height, 0 },
	    pair<f32, f32> {
	        1.0,
	        1.0,
	    },
	    Renderpass::SizeType::eSwapchainRelative,
	    "",
	    vec<Attachment> {},
	});

	containers.back().attachments.emplace_back(Attachment {
	    {},
	    vk::ImageLayout::eUndefined,
	    vk::PipelineStageFlagBits::eTopOfPipe,
	    std::move(image),
	    image_view,
	});
}

void RenderResources::create_transient_attachment(
    RenderpassBlueprint::Attachment const &blueprint_attachment,
    vk::SampleCountFlagBits sample_count
)
{
	auto const framebuffer_extent = surface->get_framebuffer_extent();
	auto const extent = calculate_attachment_image_extent(blueprint_attachment);

	auto image = Image {
		memory_allocator,
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
		},
	};

	debug_utils->set_object_name(
	    device->vk(),
	    image.vk(),
	    fmt::format("{}_transient_image", blueprint_attachment.debug_name)
	);

	auto const image_view = device->vk().createImageView(vk::ImageViewCreateInfo {
	    {},
	    image.vk(),
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

	debug_utils->set_object_name(
	    device->vk(),
	    image_view,
	    fmt::format("{}_transient_image_view", blueprint_attachment.debug_name)
	);

	transient_attachments.emplace_back(TransientAttachment {
	    std::move(image),
	    image_view,
	    sample_count,
	    blueprint_attachment.format,
	    extent,
	});
}

} // namespace BINDLESSVK_NAMESPACE
