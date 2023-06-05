#include "BindlessVk/Context/Swapchain.hpp"

namespace BINDLESSVK_NAMESPACE {

Swapchain::Swapchain(VkContext const *const vk_context)
    : device(vk_context->get_device())
    , surface(vk_context->get_surface())
    , queues(vk_context->get_queues())
    , invalid(false)
{
	destroy_image_views();

	create_swapchain();
	images = device->vk().getSwapchainImagesKHR(swapchain);
	create_image_views();

	set_object_names();
}

Swapchain::~Swapchain()
{
	if (!device)
		return;

	destroy_image_views();
}

void Swapchain::create_swapchain()
{
	auto const old_swapchain = swapchain;
	auto const queues_indices = queues->get_indices();

	swapchain = device->vk().createSwapchainKHR(vk::SwapchainCreateInfoKHR {
	    {},
	    surface->vk(),
	    calculate_best_image_count(),
	    surface->get_color_format(),
	    surface->get_color_space(),
	    surface->get_framebuffer_extent(),
	    1u,
	    vk::ImageUsageFlagBits::eColorAttachment,
	    queues->have_same_index() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
	    queues_indices,
	    surface->get_capabilities().currentTransform,
	    vk::CompositeAlphaFlagBitsKHR::eOpaque,
	    surface->get_present_mode(),
	    VK_TRUE,
	    swapchain,
	});

	device->vk().destroySwapchainKHR(old_swapchain);
	device->vk().waitIdle();
}

void Swapchain::create_image_views()
{
	image_views.resize(get_image_count());

	for (u32 i = 0; i < get_image_count(); ++i)
	{
		image_views[i] = device->vk().createImageView(vk::ImageViewCreateInfo {
		    {},
		    images[i],
		    vk::ImageViewType::e2D,
		    surface->get_color_format(),
		    vk::ComponentMapping {
		        vk::ComponentSwizzle::eIdentity,
		        vk::ComponentSwizzle::eIdentity,
		        vk::ComponentSwizzle::eIdentity,
		        vk::ComponentSwizzle::eIdentity,
		    },
		    vk::ImageSubresourceRange {
		        vk::ImageAspectFlagBits::eColor,
		        0,
		        1,
		        0,
		        1,
		    },
		});
	}
}

void Swapchain::set_object_names()
{
	for (u32 i = 0; i < get_image_count(); ++i)
	{
		device->set_object_name(images[i], "swap_chain_image_{}", i);
		device->set_object_name(image_views[i], "swapchain_image_view_{}", i);
	}
}

auto Swapchain::calculate_best_image_count() const -> u32
{
	auto const min_image_count = surface->get_capabilities().minImageCount;
	auto const max_image_count = surface->get_capabilities().maxImageCount;

	auto const has_max_limit = max_image_count != 0;

	// Desired image count is in range
	if ((!has_max_limit || max_image_count >= DESIRED_SWAPCHAIN_IMAGES) &&
	    min_image_count <= DESIRED_SWAPCHAIN_IMAGES)
		return DESIRED_SWAPCHAIN_IMAGES;

	// Fall-back to 2 if in ange
	else if (min_image_count <= 2 && max_image_count >= 2)
		return 2;

	// Fall-back to min_image_count
	else
		return min_image_count;
}

void Swapchain::destroy_image_views()
{
	for (auto const image_view : image_views)
		device->vk().destroyImageView(image_view);
}

} // namespace BINDLESSVK_NAMESPACE
