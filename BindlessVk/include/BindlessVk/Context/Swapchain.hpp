#pragma once

#include "BindlessVk/Common/Common.hpp"
#include "BindlessVk/Context/VkContext.hpp"

#ifndef DESIRED_SWAPCHAIN_IMAGES
	#define DESIRED_SWAPCHAIN_IMAGES 3
#endif

namespace BINDLESSVK_NAMESPACE {

/** Wrapper aound vulkan swapchain */
class Swapchain
{
public:
	/** Default constructor */
	Swapchain() = default;

	/** Argumented constructor
	 *
	 * @param vk_context Pointer to the vk context
	 */
	Swapchain(VkContext const *vk_context);

	/** Default move constructor */
	Swapchain(Swapchain &&other) = default;

	/** Default move assignment operator */
	Swapchain &operator=(Swapchain &&other) = default;

	/** Deleted copy constructor */
	Swapchain(Swapchain const &) = delete;

	/** Deleted copy assingment operator */
	Swapchain &operator=(Swapchain const &) = delete;

	/** Destructor */
	~Swapchain();

	/** Trivial setter for invalid (always sets to true) */
	void invalidate()
	{
		invalid = true;
	}

	/** Trivial accessor for the underyling swapchain */
	auto vk() const
	{
		return swapchain;
	}

	/** Trivial accessor for images */
	auto get_images() const
	{
		return images;
	}

	/** Trivial accessor for image_views */
	auto get_image_views() const
	{
		return image_views;
	}

	/** Returns images.size(), should be the same as get_image_views().size() */
	auto get_image_count() const
	{
		return images.size();
	}

	/** Trivial accessor for invalid */
	auto is_invalid() const
	{
		return invalid;
	}

private:
	void create_swapchain();
	void create_image_views();

	void set_object_names();

	auto calculate_best_image_count() const -> u32;

	void destroy_image_views();

private:
	tidy_ptr<Device const> device = {};

	Surface const *surface = {};
	Queues const *queues = {};

	vk::SwapchainKHR swapchain = {};

	vec<vk::Image> images = {};
	vec<vk::ImageView> image_views = {};

	bool invalid = {};
};

} // namespace BINDLESSVK_NAMESPACE
