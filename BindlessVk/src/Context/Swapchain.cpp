#include "BindlessVk/Context/Swapchain.hpp"

namespace BINDLESSVK_NAMESPACE {

void Swapchain::init(vk::Device const device, Surface const surface, Queues const queues)
{
	this->device = device;
	this->surface = surface;
	this->queues = queues;
	invalid = false;

	destroy_image_views();

	create_swapchain();
	images = device.getSwapchainImagesKHR(swapchain);
	create_image_views();

	set_object_names();
}

void Swapchain::destroy()
{
	device.waitIdle();
	destroy_image_views();
	device.destroySwapchainKHR(swapchain);
}

void Swapchain::create_swapchain()
{
	auto const old_swapchain = swapchain;
	auto const queues_indices = queues.get_indices();

	swapchain = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR {
	    {},
	    surface,
	    calculate_best_image_count(),
	    surface.get_color_format(),
	    surface.get_color_space(),
	    surface.get_framebuffer_extent(),
	    1u,
	    vk::ImageUsageFlagBits::eColorAttachment,
	    queues.have_same_index() ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent,
	    queues_indices,
	    surface.get_capabilities().currentTransform,
	    vk::CompositeAlphaFlagBitsKHR::eOpaque,
	    surface.get_present_mode(),
	    VK_TRUE,
	    swapchain,
	});

	device.destroySwapchainKHR(old_swapchain);
	device.waitIdle(); // @todo: should we wait idle here???
}

void Swapchain::create_image_views()
{
	image_views.resize(get_image_count());

	for (u32 i = 0; i < get_image_count(); ++i)
	{
		image_views[i] = device.createImageView({
		    {},
		    images[i],
		    vk::ImageViewType::e2D,
		    surface.get_color_format(),
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
		device.setDebugUtilsObjectNameEXT({
		    vk::ObjectType::eImage,
		    (u64)(VkImage)images[i],
		    fmt::format("swap_chain_image_{}", i).c_str(),
		});

		device.setDebugUtilsObjectNameEXT({
		    vk::ObjectType::eImageView,
		    (u64)(VkImageView)image_views[i],
		    fmt::format("swapchain_image_view_{}", i).c_str(),
		});
	}
}

auto Swapchain::calculate_best_image_count() const -> u32
{
	auto const min_image_count = surface.get_capabilities().minImageCount;
	auto const max_image_count = surface.get_capabilities().maxImageCount;

	auto const has_max_limit = max_image_count != 0;

	// desired image count is in range
	if ((!has_max_limit || max_image_count >= DESIRED_SWAPCHAIN_IMAGES) &&
	    min_image_count <= DESIRED_SWAPCHAIN_IMAGES)
		return DESIRED_SWAPCHAIN_IMAGES;

	// fall-back to 2 if in ange
	else if (min_image_count <= 2 && max_image_count >= 2)
		return 2;

	// fall-back to min_image_count
	else
		return min_image_count;
}

void Swapchain::destroy_image_views()
{
	device.waitIdle(); // @todo should we wait idle here???
	for (auto const &image_view : image_views)
		device.destroyImageView(image_view);
}

} // namespace BINDLESSVK_NAMESPACE
