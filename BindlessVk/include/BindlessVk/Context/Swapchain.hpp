#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/Queues.hpp"
#include "BindlessVk/Context/Surface.hpp"

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif

namespace BINDLESSVK_NAMESPACE {

class Swapchain
{
public:
	void init(vk::Device device, Surface surface, Queues queues);
	void destroy();

	void invalidate()
	{
		invalid = true;
	}

	auto get_images() const
	{
		return images;
	}

	auto get_image_views() const
	{
		return image_views;
	}

	auto get_image_count() const
	{
		return images.size();
	}

	auto is_invalid() const
	{
		return invalid;
	}

	operator vk::SwapchainKHR() const
	{
		return swapchain;
	}

private:
	void create_swapchain();
	void create_image_views();

	void set_object_names();

	auto calculate_best_image_count() const -> u32;

	void destroy_image_views();

private:
	vk::Device device = {};
	vk::SwapchainKHR swapchain = {};
	Surface surface = {};
	Queues queues = {};
	vec<vk::Image> images = {};
	vec<vk::ImageView> image_views = {};
	bool invalid = {};
};

} // namespace BINDLESSVK_NAMESPACE
